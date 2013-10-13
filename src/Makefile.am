bin_PROGRAMS = opendtc

USBIMPL_SOURCES = usbimpl_libusb.c
USBIMPL_CFLAGS = $(libusb_CFLAGS)
USBIMPL_LIBS = $(libusb_LIBS)

opendtc_SOURCES = main.c stream.c device.c $(USBIMPL_SOURCES)

noinst_HEADERS = stream.h device.h usbapi.h usbimpl.h usbimpl_libusb.h 

opendtc_CFLAGS = $(USBIMPL_CFLAGS) '-DVERSION="$(VERSION)"'
opendtc_LDADD = $(USBIMPL_LIBS)
