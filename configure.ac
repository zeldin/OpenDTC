dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)

AC_INIT([opendtc], [0.1], [marcus@mc.pp.se])

AM_INIT_AUTOMAKE([1.13 gnu])
AC_CONFIG_HEADERS([config.h])

AC_PROG_CC

PKG_CHECK_MODULES([libusb], [libusb-1.0 >= 1.0.9], [],
	[AC_MSG_ERROR([This program needs libusb-1.0 (1.0.9 or higher)])])

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
