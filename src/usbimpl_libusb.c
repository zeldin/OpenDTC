/* usbimpl_libusb.c -- USB implementation using libusb

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

static libusb_context *libusb_ctx = NULL;

bool usbapi_init(void)
{
  int ret;
  if (!(ret = libusb_init(&libusb_ctx)))
    return true;
  fprintf(stderr, "Failed to init libusb: %s.", libusb_error_name(ret));
  return false;
}

void usbapi_exit(void)
{
  if (libusb_ctx != NULL) {
    libusb_exit(libusb_ctx);
    libusb_ctx = NULL;
  }
}

usbapi_handle usbapi_open(uint16_t vid, uint16_t pid, unsigned num)
{
  libusb_device **devlist;
  libusb_device *dev;
  struct libusb_device_descriptor des;
  struct libusb_device_handle *hdl;
  int i, ret;

  libusb_get_device_list(libusb_ctx, &devlist);
  for (i = 0; (dev = devlist[i]) != NULL; i++) {
    if ((ret = libusb_get_device_descriptor(dev, &des)) != 0) {
      fprintf(stderr, "Failed to get device descriptor: %s.",
	      libusb_error_name(ret));
      continue;
    }
    if (des.idVendor != vid || des.idProduct != pid) {
      continue;
    }
    if (num > 0) {
      --num;
      continue;
    }
    if (!(ret = libusb_open(dev, &hdl))) {
      libusb_free_device_list(devlist, 1);
      return hdl;
    } else {
      fprintf(stderr, "Failed to open device: %s.", libusb_error_name(ret));
      libusb_free_device_list(devlist, 1);
      return NULL;
    }
  }
  fprintf(stderr, "No device with vendor id 0x%04x and product id 0x%04x found\n",
	  vid, pid);
  libusb_free_device_list(devlist, 1);
  return NULL;
}

void usbapi_close(usbapi_handle hdl)
{
  libusb_close(hdl);
}