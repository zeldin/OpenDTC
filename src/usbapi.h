/* usbapi.h: USB functionality

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

#ifndef OPENDTC_USBAPI_H
# define OPENDTC_USBAPI_H

#include <stdint.h>
#include <stdbool.h>
#include "usbimpl.h"

extern bool usbapi_init(void);
extern void usbapi_exit(void);
extern usbapi_handle usbapi_open(uint16_t vid, uint16_t pid, unsigned num);
extern void usbapi_close(usbapi_handle hdl);

#endif /* OPENDTC_USBAPI_H */
