/* stream.c -- stream operations

   Copyright 2013 Marcus Comstedt

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <config.h>
#include <device.h>
#include <stream.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

static FILE *stream_file = NULL;

static bool stream_complete = false, stream_failed = false;
static bool result_found = false;
static unsigned long current_streampos;
static uint32_t skipcount = 0;

static bool stream_validate_data(const uint8_t *data, uint32_t len)
{
  if (skipcount) {
    uint32_t n = (skipcount > len? len : skipcount);
    skipcount -= n;
    data += n;
    len -= n;
    current_streampos += n;
  }
  while (len > 0) {
    if (*data <= 7) {
      /* Value */
      if (len < 2) {
	skipcount += 2-len;
	current_streampos += len;
	return true;
      }
      current_streampos += 2;
      data += 2;
      len -= 2;
    } else if (*data >= 0xe) {
      /* Sample */
      data ++;
      --len;
      current_streampos ++;
    } else switch(*data) {
    default:
      /* Nop1-Nop3 */
      ; int noffset = *data - 7;
      if (len < noffset) {
	skipcount += noffset-len;
	current_streampos += len;
	return true;
      }
      current_streampos += noffset;
      data += noffset;
      len -= noffset;
      break;
    case 0x0b:
      /* Overflow16 */
      data ++;
      --len;
      current_streampos ++;
      break;
    case 0x0c:
      /* Value16 */
      if (len < 3) {
	skipcount += 3-len;
	current_streampos += len;
	return true;
      }
      data += 3;
      len -= 3;
      current_streampos += 3;
      break;
    case 0x0d:
      if (len < 4) {
	fprintf(stderr, "No room for OOB header\n");
	return false;
      }
      unsigned type = data[1];
      unsigned size = data[2] | (data[3] << 8);
      if (type == 0x0d && size == 0x0d0d) {
	if (!result_found) {
	  fprintf(stderr, "End of data marker encountered before end of stream marker\n");
	  return false;
	}
	stream_complete = true;
	return true;
      }
      if (len-4 < size) {
	fprintf(stderr, "No room for OOB data\n");
	return false;
      }
      if (type == 1 || type == 3) {
	unsigned long streampos;
	if (size < 4) {
	  fprintf(stderr, "No room for stream position\n");
	  return false;
	}
	streampos = data[4] | (data[5]<<8) | (data[6]<<16) | (data[7]<<24);
	if (streampos != current_streampos) {
	  fprintf(stderr, "Bad stream position %lu != %lu\n",
		  streampos, current_streampos);
	  return false;
	}
      }
      if (type == 3) {
	unsigned long result;
	if (size < 8) {
	  fprintf(stderr, "No room for result value\n");
	  return false;
	}
	result_found = true;
	result = data[8] | (data[9]<<8) | (data[10]<<16) | (data[11]<<24);
	if (result != 0) {
	  switch(result) {
	  case 1:
	    fprintf(stderr, "Buffering problem - data transfer delivery "
		    "to host could not keep up with disk read\n");
	    break;
	  case 2:
	    fprintf(stderr, "No index signal detected\n");
	    break;
	  default:
	    fprintf(stderr, "Unknown stream end result %lu\n", result);
	    break;
	  }
	  return false;
	}
      }
      data += size+4;
      len -= size+4;
      break;
    }
  }
  return true;
}

static bool stream_callback(const uint8_t *data, uint32_t len)
{
  if (stream_complete || stream_failed || !stream_file)
    return false;
  if (!data) {
    stream_failed = true;
    return false;
  }
  if (!len)
    return true;
  if (!stream_validate_data(data, len)) {
    stream_failed = true;
    return false;
  }
  if (fwrite(data, 1, len, stream_file) != len) {
    fprintf(stderr, "Failed to write data to file\n");
    stream_failed = true;
    return false;
  }
  return !stream_complete;
}

static bool stream_device_capture(void)
{
  stream_complete = stream_failed = false;
  result_found = false;
  current_streampos = 0;
  skipcount = 0;

  if (!device_start_async_read(stream_callback))
    return false;

  if (!device_stream_on())
    return false;

  if (!device_finish_async_read())
    return false;

  if (!device_stream_off())
    return false;

  return true;
}

static bool stream_write_preamble(void)
{
  uint8_t buf[128];
  time_t t = time(NULL);
  struct tm *tm = localtime(&t);
  unsigned l;
  snprintf((char *)buf+4, sizeof(buf)-4,
	   "host_date=%04d.%02d.%02d, host_time=%02d:%02d:%02d",
	   tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
	   tm->tm_hour, tm->tm_min, tm->tm_sec);
  l = strlen((char *)buf+4)+1;
  buf[0] = 0x0d;
  buf[1] = 4;
  buf[2] = l;
  buf[3] = 0;
  if (fwrite(buf, 1, l+4, stream_file) != l+4) {
    fprintf(stderr, "Failed to write data to file\n");
    return false;
  }
  return true;
}

bool stream_capture(const char *filename)
{
  bool r;
  stream_file = fopen(filename, "wb");
  if (!stream_file) {
    perror(filename);
    return false;
  }
  r = stream_write_preamble();
  if (r)
    r = stream_device_capture();
  if (fclose(stream_file)) {
    perror(filename);
    r = false;
  }
  stream_file = NULL;
  return r && stream_complete && !stream_failed;
}
