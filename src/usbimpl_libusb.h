/* usbimpl_libusb.h: USB implementation using libusb

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

#ifndef OPENDTC_USBIMPL_LIBUSB_H
# define OPENDTC_USBIMPL_LIBUSB_H

# include <libusb.h>

typedef struct libusb_device_handle *usbapi_handle;
typedef struct usbimpl_libusb_async_struct *usbapi_async_handle;

#define USBAPI_INVALID_HANDLE       NULL
#define USBAPI_INVALID_ASYNC_HANDLE NULL

#endif /* OPENDTC_USBIMPL_LIBUSB_H */
