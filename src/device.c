/* device.c -- device operations

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
#include <usbapi.h>
#include <stdio.h>
#include <stdlib.h>

#define KRYOFLUX_VID 0x03eb
#define KRYOFLUX_PID 0x6124

static usbapi_handle usbhdl = USBAPI_INVALID_HANDLE;

static void device_exit(void)
{
  if (usbhdl != USBAPI_INVALID_HANDLE) {
    usbapi_close(usbhdl);
    usbhdl = USBAPI_INVALID_HANDLE;
  }
  usbapi_exit();
}

bool device_init(void)
{
  if (!usbapi_init())
    return false;
  atexit(device_exit);
  usbhdl = usbapi_open(KRYOFLUX_VID, KRYOFLUX_PID, 0);
  if (usbhdl == USBAPI_INVALID_HANDLE)
    return false;

  return true;
}
