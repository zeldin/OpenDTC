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
#include <stdbool.h>
#include <alloca.h>

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
      snprintf(fnbuf, fnbufsize, "%s%02d.%d.raw", filename_base, track, side);
      if (!device_motor_on(side, track))
	return false;
      if (!stream_capture(fnbuf))
	return false;
    }
  }
  return device_motor_off();
}

int main (int argc, char *argv[])
{
  if (!device_init())
    return 1;
  if (!device_configure(0, 0, 0, 83))
    return 1;
  if (!capture_tracks("out/track", 4, 7, 2, 1))
    return 1;

  return 0;
}
