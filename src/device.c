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
#include <string.h>
#include <stdlib.h>
#include <alloca.h>

#define KRYOFLUX_VID       0x03eb
#define KRYOFLUX_PID       0x6124
#define KRYOFLUX_INTERFACE 1

#define FW_FILENAME "firmware.bin"

#define FW_LOAD_ADDRESS     0x00202000UL
#define FW_WRITE_CHUNK_SIZE 16384
#define FW_READ_CHUNK_SIZE  6400

#define REQTYPE_IN_VENDOR_OTHER 0xc3

#define REQUEST_RESET     0x05
#define REQUEST_DEVICE    0x06
#define REQUEST_DENSITY   0x08
#define REQUEST_MIN_TRACK 0x0c
#define REQUEST_MAX_TRACK 0x0d
#define REQUEST_STATUS    0x80
#define REQUEST_INFO      0x81

static usbapi_handle usbhdl = USBAPI_INVALID_HANDLE;
static bool usbifcclaimed = false;

static void device_close(void)
{
  if (usbhdl != USBAPI_INVALID_HANDLE) {
    if (usbifcclaimed) {
      usbapi_release_interface(usbhdl, KRYOFLUX_INTERFACE);
      usbifcclaimed = false;
    }
    usbapi_close(usbhdl);
    usbhdl = USBAPI_INVALID_HANDLE;
  }
}

static void device_exit(void)
{
  device_close();
  usbapi_exit();
}

static bool device_claim_interface(void)
{
  return (usbifcclaimed = usbapi_claim_interface(usbhdl, KRYOFLUX_INTERFACE));
}

static bool device_send_bl_string(const char *s)
{
  unsigned l = strlen(s);
  uint8_t *outbuf = alloca(l);
  memcpy(outbuf, s, l);
  return usbapi_sync_bulk_out(usbhdl, 1, outbuf, l, 1000);
}

static bool device_recv_bl_string(char *buf, unsigned size)
{
  unsigned tot = 0;
  while (tot < size) {
    int32_t l = usbapi_sync_bulk_in(usbhdl, 2, (uint8_t *)buf+tot,
				    size-tot, 1000);
    if (l<0)
      return false;
    tot += l;
    if (tot >= 2 && buf[tot-1]==0xd && buf[tot-2]==0xa)
      break;
  }
  if (tot >= size)
    tot = size-1;
  buf[tot] = 0;
  return true;
}

static int32_t device_control_in(uint8_t request, uint16_t index, bool silent)
{
  uint8_t buf[512];
  int32_t l;
  l = usbapi_sync_control_in(usbhdl, REQTYPE_IN_VENDOR_OTHER, request,
			     0, index, buf, sizeof(buf), 5000, silent);
  if (l<0)
    return l;
  printf("Device says: %s\n", buf);
  return l;
}

static bool device_try_check_status(void)
{
  return device_control_in(REQUEST_STATUS, 0, true)>=0;
}

static bool device_do_request(uint8_t request, uint16_t index)
{
  return device_control_in(request, index, false)>=0;
}

static bool device_check_fw_present(void)
{
  bool last_present, present = device_try_check_status();
  do {
    last_present = present;
    present = device_try_check_status();
  } while(present != last_present);
  return present;
}

static bool device_query_fw(const char *id, char *buf, unsigned size)
{
  if (device_send_bl_string(id) && device_recv_bl_string(buf, 512)) {
    printf("Device response to %s: %s", id, buf);
    return true;
  } else
    return false;
}

static bool device_upload_firmware(uint8_t *fw, uint32_t fw_size)
{
  char buf[512];
  uint32_t offs;
  bool vfy_failed;
  char *fw_vfy;

  if (!device_query_fw("N#", buf, 512) ||
      !device_query_fw("V#", buf, 512)) {
    return false;
  }

  snprintf(buf, sizeof(buf), "S%08lx,%08lx#",
	   (unsigned long)FW_LOAD_ADDRESS, (unsigned long)fw_size);
  if (!device_send_bl_string(buf))
    return false;

  for (offs = 0; offs < fw_size; offs += FW_WRITE_CHUNK_SIZE) {
    uint32_t chunk = (offs+FW_WRITE_CHUNK_SIZE >= fw_size?
		      fw_size-offs : FW_WRITE_CHUNK_SIZE);
    if (!usbapi_sync_bulk_out(usbhdl, 1, fw+offs, chunk, 2000))
      return false;
  }

  snprintf(buf, sizeof(buf), "R%08lx,%08lx#",
	   (unsigned long)FW_LOAD_ADDRESS, (unsigned long)fw_size);
  if (!device_send_bl_string(buf))
    return false;

  fw_vfy = malloc(FW_READ_CHUNK_SIZE);
  if (!fw_vfy) {
    fprintf(stderr, "Out of memory!\n");
    return false;
  }

  vfy_failed = false;
  for (offs = 0; offs < fw_size; ) {
    uint32_t chunk = (offs+FW_READ_CHUNK_SIZE >= fw_size?
		      fw_size-offs : FW_READ_CHUNK_SIZE);
    int32_t l = usbapi_sync_bulk_in(usbhdl, 2, fw_vfy, chunk, 2000);
    if (l<0) {
      free (fw_vfy);
      return false;
    }
    if (l>0) {
      if (memcmp(fw_vfy, fw+offs, l))
	vfy_failed = true;
    }
    offs += l;
  }

  free(fw_vfy);
  if (vfy_failed) {
    fprintf(stderr, "Firmware verify failed!\n");
    return false;
  }

  snprintf(buf, sizeof(buf), "G%08lx#", (unsigned long)FW_LOAD_ADDRESS);
  if (!device_send_bl_string(buf))
    return false;

  return true;
}

static uint8_t *device_load_firmware(uint32_t *psize)
{
  uint32_t size = 0, allocsize = 0;
  uint8_t *buf = NULL;
  size_t l;
  const char *filename = FW_FILENAME;
  FILE *f;

  f = fopen(filename, "rb");
  if (!f) {
    perror(filename);
    return NULL;
  }
  do {
    if (size >= allocsize) {
      uint8_t *newbuf;
      allocsize = (size? size + (size>>1) : 8192);
      newbuf = realloc(buf, allocsize);
      if (newbuf == NULL) {
	free(buf);
	return NULL;
      } else {
	buf = newbuf;
      }
    }
    l = fread(buf+size, 1, allocsize-size, f);
    size += l;
    if (ferror(f)) {
      perror(filename);
      free(buf);
      return NULL;
    }
  } while (!feof(f));
  *psize = size;
  return buf;
}

static bool device_install_firmware(void)
{
  bool ret;
  uint32_t fw_size;
  uint8_t *fw = device_load_firmware(&fw_size);
  if (!fw)
    return false;
  ret = device_upload_firmware(fw, fw_size);
  free(fw);
  return ret;
}

static bool device_reset(void)
{
  return
    device_do_request(REQUEST_RESET, 0) &&
    device_do_request(REQUEST_INFO, 1) &&
    device_do_request(REQUEST_INFO, 2);
}

bool device_init(void)
{
  if (!usbapi_init())
    return false;
  atexit(device_exit);
  usbhdl = usbapi_open(KRYOFLUX_VID, KRYOFLUX_PID, 0);
  if (usbhdl == USBAPI_INVALID_HANDLE ||
      !device_claim_interface())
    return false;

  if (device_check_fw_present()) {
    printf("Device has FW already\n");
  } else {
    printf("No FW uploaded in device\n");
    if (!device_install_firmware())
      return false;

    /* Need to reopen the device after renumeration */
    device_close();
    sleep(1);
    usbhdl = usbapi_open(KRYOFLUX_VID, KRYOFLUX_PID, 0);
    if (usbhdl == USBAPI_INVALID_HANDLE ||
	!device_claim_interface())
      return false;

    if (!device_check_fw_present()) {
      fprintf(stderr, "Device renumerated without working firmware!\n");
      return false;
    }
  }

  return device_reset();
}

bool device_configure(int device, int density, int min_track, int max_track)
{
  return
    device_do_request(REQUEST_DEVICE, device) &&
    device_do_request(REQUEST_DENSITY, density) &&
    device_do_request(REQUEST_MIN_TRACK, min_track) &&
    device_do_request(REQUEST_MAX_TRACK, max_track);
}
