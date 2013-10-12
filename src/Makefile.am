bin_PROGRAMS = opendtc

USBIMPL_SOURCES = usbimpl_libusb.c
USBIMPL_CFLAGS = $(libusb_CFLAGS)
USBIMPL_LIBS = $(libusb_LIBS)

opendtc_SOURCES = main.c device.c $(USBIMPL_SOURCES)

opendtc_CFLAGS = $(USBIMPL_CFLAGS)
opendtc_LDADD = $(USBIMPL_LIBS)
