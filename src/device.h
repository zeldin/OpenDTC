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
extern bool device_motor_on(int side, int track);
extern bool device_motor_off(void);
extern bool device_stream_on(void);
extern bool device_stream_off(void);
extern bool device_start_async_read(bool (*callback)(const uint8_t *, uint32_t));
extern bool device_finish_async_read(void);

#endif /* OPENDTC_DEVICE_H */
