/* device.h: device operations

   Copyright 2013 Marcus Comstedt

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef OPENDTC_DEVICE_H
# define OPENDTC_DEVICE_H

# include <stdint.h>
# include <stdbool.h>

extern bool device_init(void);
extern bool device_configure(int device, int density,
			     int min_track, int max_track);

#endif /* OPENDTC_DEVICE_H */
