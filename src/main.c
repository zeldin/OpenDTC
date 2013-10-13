/* main.c -- main function

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
#include <stdbool.h>
#include <alloca.h>

static int opt_device = 0;
static int opt_density = 0;
static int opt_mintrack = 0;
static int opt_maxtrack = 83;
static int opt_starttrack = -1;
static int opt_endtrack = -1;
static int opt_side_mode = 2;
static int opt_track_distance = 1;
static const char *opt_filename = NULL;

static bool parse_intoption(char *optstr, int offs, int *optval,
			    int lo_limit, int hi_limit)
{
  char *e;
  long v = strtol(optstr+offs, &e, 10);
  if (!optstr[offs] || *e || v < lo_limit || v > hi_limit) {
    fprintf(stderr, "Out of range %d...%d: %s\n", lo_limit, hi_limit, optstr);
    return false;
  }
  *optval = v;
  return true;
}

static bool parse_options(int argc, char **argv)
{
  int i;
  for (i=1; i<argc; i++)
    if (argv[i][0] != '-') {
      fprintf(stderr, "Syntax error: %s\n", argv[i]);
      return false;
    } else switch(argv[i][1]) {
    case 'h':
      printf("Commands:\n"
	     "-f<name>: set filename\n"
	     "-d<id>  : select drive (default 0)\n"
	     "-dd<val>: set drive density line (default 0)\n"
	     "          0=L, 1=H\n"
	     "-s<trk> : set start track (default at least 0)\n"
	     "-e<trk> : set end track (default at most 83)\n"
	     "-g<side>: set single sided mode\n"
	     "          0=side 0, 1=side 1, 2=both sides\n"
	     "-k<step>: set track distance\n"
	     "          1=80 tracks, 2=40 tracks (default 1)\n");
      exit(0);
      break;
    case 'f':
      opt_filename = argv[i]+2;
      break;
    case 'd':
      if (argv[i][2] == 'd') {
	if (!parse_intoption(argv[i], 3, &opt_density, 0, 1))
	  return false;
      } else {
	if (!parse_intoption(argv[i], 2, &opt_device, 0, 1))
	  return false;
      }
      break;
    case 's':
      if (!parse_intoption(argv[i], 2, &opt_starttrack, 0, 83))
	return false;
      break;
    case 'e':
      if (!parse_intoption(argv[i], 2, &opt_endtrack, 0, 83))
	return false;
      break;
    case 'g':
      if (!parse_intoption(argv[i], 2, &opt_side_mode, 0, 2))
	return false;
      break;
    case 'k':
      if (!parse_intoption(argv[i], 2, &opt_track_distance, 1, 2))
	return false;
      break;
    default:
      fprintf(stderr, "Invalid command: %s\n", argv[i]);
      return false;
    }
  return true;
}

static bool capture_tracks(const char *filename_base, int start_track,
			   int end_track, int side_mode, int track_distance)
{
  int track, side;
  int fnbufsize = strlen(filename_base)+10;
  char *fnbuf = alloca(fnbufsize);
  for (track = start_track; track <= end_track; track += track_distance) {
    for (side = 0; side < 2; side ++) {
      if (side_mode < 2 && side != side_mode)
	continue;
      printf("%02d.%d    : ", track, side);
      fflush(stdout);
      snprintf(fnbuf, fnbufsize, "%s%02d.%d.raw", filename_base, track, side);
      if (!device_motor_on(side, track))
	return false;
      if (!stream_capture(fnbuf))
	return false;
      printf("ok\n");
    }
  }
  return device_motor_off();
}

int main (int argc, char *argv[])
{
  printf("Open DiskTool Console v" VERSION "\n");
  printf("This program is free software: you can redistribute it and/or modify\n"
	 "it under the terms of the GNU General Public License as published by\n"
	 "the Free Software Foundation, either version 3 of the License, or\n"
	 "(at your option) any later version.\n\n");

  if (!parse_options(argc, argv))
    return 1;
  if (opt_filename == NULL) {
    fprintf(stderr, "No filename specified\n");
    return 1;
  }
  if (!device_init())
    return 1;
  if (!device_configure(opt_device, opt_density, opt_mintrack, opt_maxtrack))
    return 1;
  if (opt_starttrack < 0)
    opt_starttrack = opt_mintrack;
  if (opt_endtrack < 0)
    opt_endtrack = opt_maxtrack;
  if (!capture_tracks(opt_filename, opt_starttrack, opt_endtrack,
		      opt_side_mode, opt_track_distance))
    return 1;

  printf("\nEnjoy your shiny new disk image!\n");
  printf("Please consider helping SPS to preserve media:\n");
  printf("www.softpres.org/donate\n");

  return 0;
}
