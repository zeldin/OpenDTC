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

static FILE *stream_file = NULL;

static bool stream_complete = false, stream_failed = false;

static void stream_validate_data(const uint8_t *data, uint32_t len)
{
  if (*data == 0xd) {
    if (len == 7 && data[1] == 0xd) {
      int i;
      for (i = 0; i<7; i++)
	if (data[i] != 0xd)
	  break;
      if (i == 7) {
	stream_complete = true;
	return;
      }
    }
  }
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
  stream_validate_data(data, len);
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

bool stream_capture(const char *filename)
{
  bool r;
  stream_file = fopen(filename, "wb");
  if (!stream_file) {
    perror(filename);
    return false;
  }
  r = stream_device_capture();
  if (fclose(stream_file)) {
    perror(filename);
    r = false;
  }
  stream_file = NULL;
  return r && stream_complete && !stream_failed;
}
