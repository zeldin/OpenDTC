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
#include <string.h>
#include <stdlib.h>

struct usbimpl_libusb_async_struct {
  int bufcnt;
  uint32_t bufsize;
  unsigned submitted;
  bool (*callback)(const uint8_t *, uint32_t);
  struct libusb_transfer *transfers[];
};

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

bool usbapi_claim_interface(usbapi_handle hdl, int ifc)
{
  int ret = libusb_claim_interface(hdl, ifc);
  if (ret) {
    fprintf(stderr, "Claim interface failed: %s.\n", libusb_error_name(ret));
    return false;
  } else {
    return true;
  }
}

bool usbapi_release_interface(usbapi_handle hdl, int ifc)
{
  int ret = libusb_release_interface(hdl, ifc);
  if (ret) {
    fprintf(stderr, "Release interface failed: %s.\n", libusb_error_name(ret));
    return false;
  } else {
    return true;
  }
}

bool usbapi_sync_bulk_out(usbapi_handle hdl, int ep, uint8_t *buf,
				 uint32_t len, unsigned timeout)
{
  int xferred = 0;
  int ret = libusb_bulk_transfer(hdl,
				 (ep & LIBUSB_ENDPOINT_ADDRESS_MASK) |
				 LIBUSB_ENDPOINT_OUT,
				 buf, len, &xferred, timeout);
  if (!ret) {
    if (xferred == len)
      return true;
    fprintf(stderr, "Bulk out truncated transfer: %u != %u\n",
	    (unsigned)xferred, (unsigned)len);
    return false;
  } else {
    fprintf(stderr, "Bulk out transfer failed: %s.\n", libusb_error_name(ret));
    return false;
  }
}

int32_t usbapi_sync_bulk_in(usbapi_handle hdl, int ep, uint8_t *buf,
			    uint32_t len, unsigned timeout)
{
  int xferred = 0;
  int ret = libusb_bulk_transfer(hdl,
				 (ep & LIBUSB_ENDPOINT_ADDRESS_MASK) |
				 LIBUSB_ENDPOINT_IN,
				 buf, len, &xferred, timeout);
  if (!ret)
    return xferred;
  else {
    fprintf(stderr, "Bulk in transfer failed: %s.\n", libusb_error_name(ret));
    return -1;
  }
}

int32_t usbapi_sync_control_in(usbapi_handle hdl, uint8_t reqtype,
			       uint8_t request, uint16_t value, uint16_t index,
			       uint8_t *buf, uint32_t len, unsigned timeout,
			       bool silent_nak)
{
  int ret = libusb_control_transfer(hdl, reqtype|LIBUSB_ENDPOINT_IN,
				    request, value, index,
				    buf, len, timeout);
  if (ret >= 0)
    return ret;
  else if(silent_nak && ret == LIBUSB_ERROR_PIPE) {
    return -2;
  } else {
    fprintf(stderr, "Bulk in transfer failed: %s.\n", libusb_error_name(ret));
    return -1;
  }
}

static void usbapi_async_callback(struct libusb_transfer *xfer)
{ 
  uint8_t *buffer;
  uint32_t length;
  usbapi_async_handle async = xfer->user_data;

  if (xfer->status == LIBUSB_TRANSFER_CANCELLED) {
    --async->submitted;
    return;
  }
  if (xfer->status == LIBUSB_TRANSFER_COMPLETED) {
    buffer = xfer->buffer;
    length = xfer->actual_length;
  } else {
    switch (xfer->status) {
    case LIBUSB_TRANSFER_ERROR:
      fprintf(stderr, "Transfer failed\n");
      break;
    case LIBUSB_TRANSFER_TIMED_OUT:
      fprintf(stderr, "Transfer timed out\n");
      break;
    case LIBUSB_TRANSFER_STALL:
      fprintf(stderr, "Halt condition detected\n");
      break;
    case LIBUSB_TRANSFER_NO_DEVICE:
      fprintf(stderr, "Device was disconnected\n");
      break;
    case LIBUSB_TRANSFER_OVERFLOW:
      fprintf(stderr, "Device sent more data than requested\n");
      break;
    default:
      fprintf(stderr, "Unknown status %d\n", xfer->status);
      break;
    }
    buffer = NULL;
    length = 0;
  }
  if (async->callback(buffer, length)) {
    int ret = libusb_submit_transfer(xfer);
    if (ret) {
      fprintf(stderr, "Failed to resubmit transfer: %s.",
	      libusb_error_name(ret));
    } else {
      return;
    }
  } else
    usbapi_async_cancel(xfer->dev_handle, async);
  --async->submitted;
}

static bool usbapi_async_bulk_in_start(usbapi_handle hdl,
				       usbapi_async_handle async,
				       int ep, unsigned timeout)
{
  int i;
  for (i=0; i<async->bufcnt; i++) {
    uint8_t *buffer = malloc(async->bufsize);
    if (buffer == NULL) {
      fprintf(stderr, "Out of memory!\n");
      return false;
    }
    async->transfers[i] = libusb_alloc_transfer(0);
    if (async->transfers[i] == NULL) {
      fprintf(stderr, "Out of memory!\n");
      free(buffer);
      return false;
    }
    libusb_fill_bulk_transfer(async->transfers[i], hdl,
			      (ep & LIBUSB_ENDPOINT_ADDRESS_MASK) |
			      LIBUSB_ENDPOINT_IN,
			      buffer, async->bufsize,
			      usbapi_async_callback, async, timeout);
  }
  for (i=0; i<async->bufcnt; i++) {
    int ret = libusb_submit_transfer(async->transfers[i]);
    if (ret) {
      fprintf(stderr, "Failed to submit transfer: %s.",
	      libusb_error_name(ret));
      return false;
    }
    async->submitted++;
  }
  return true;
}

static bool usbapi_async_check(void)
{
  struct timeval tv = { 1, 0 };
  int ret = libusb_handle_events_timeout_completed(libusb_ctx,
						   &tv, NULL);
  if (ret) {
    fprintf(stderr, "Failed to handle events: %s.", libusb_error_name(ret));
    return false;
  } else {
    return true;
  }
}

usbapi_async_handle usbapi_async_bulk_in(usbapi_handle hdl, int ep,
					 int bufcnt, uint32_t bufsize,
					 unsigned timeout,
					 bool (*callback)(const uint8_t *, uint32_t))
{
  int i;
  struct usbimpl_libusb_async_struct *async =
    malloc(sizeof(struct usbimpl_libusb_async_struct) +
	   bufcnt * sizeof(struct libusb_transfer *));
  if (!async)
    return NULL;
  memset(async, 0, sizeof(*async));
  async->bufcnt = bufcnt;
  async->bufsize = bufsize;
  async->callback = callback;
  async->submitted = 0;
  for (i=0; i<bufcnt; i++)
    async->transfers[i] = NULL;
  if (!usbapi_async_bulk_in_start(hdl, async, ep, timeout)) {
    usbapi_async_cancel(hdl, async);
    usbapi_async_finish(hdl, async);
    async = NULL;
  }
  return async;
}

bool usbapi_async_finish(usbapi_handle hdl, usbapi_async_handle async)
{
  bool r = true;
  int i;
  if (async != NULL) {
    while (async->submitted)
      if (!usbapi_async_check())
	r = false;
    for (i=0; i<async->bufcnt; i++) {
      if (async->transfers[i] != NULL) {
	free(async->transfers[i]->buffer);
	libusb_free_transfer(async->transfers[i]);
	async->transfers[i] = NULL;
      }
    }
    free(async);
  }
  return r;
}

bool usbapi_async_cancel(usbapi_handle hdl, usbapi_async_handle async)
{
  bool r = true;
  int i, ret;
  for (i=0; i<async->bufcnt; i++) {
    if (async->transfers[i] != NULL) {
      ret = libusb_cancel_transfer(async->transfers[i]);
      if (ret && ret != LIBUSB_ERROR_NOT_FOUND) {
	fprintf(stderr, "Failed to cancel transfer: %s.",
		libusb_error_name(ret));
	r = false;
      }
    }
  }
  return r;
}
