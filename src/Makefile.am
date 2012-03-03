## Makefile for program src directory in GNU Shar utilities package.
##
## Copyright (C) 1995, 1996, 2002, 2007, 2011 Free Software Foundation, Inc.
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 3, or (at your option)
## any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

## Process this file with automake to produce Makefile.in.

# stolen from fileutils-4.1.9:
# Sometimes, the expansion of $(LIBINTL) includes -lc which may
# include modules defining variables like `optind', so libfetish.a
# must precede $(LIBINTL) in order to ensure we use GNU getopt.
# But libgnu.a must also follow $(LIBINTL), since libintl uses
# replacement functions defined in libgnu.a.
LDADD = $(GETHOSTNAME_LIB) $(LIBSOCKET) \
	../lib/libgnu.a $(LIBINTL) ../lib/libgnu.a
shar_LDADD      = $(LDADD)
unshar_LDADD    = $(LDADD)
bin_PROGRAMS    = shar unshar

if UUCODE
  bin_PROGRAMS += uuencode uudecode
  uuencode_LDADD = $(LDADD)
  uudecode_LDADD = $(LDADD)
endif

EXTRA_SCRIPTS   = compress-dummy
EXTRA_DIST      = scripts.def scripts.tpl
DISTCLEANFILES  = $(bin_SCRIPTS)

localedir   = $(datadir)/locale

AM_CPPFLAGS = -I$(srcdir) -I.. -I$(top_srcdir)/lib -I../intl
DEFS        = -DLOCALEDIR=\"$(localedir)\" @DEFS@

# Source dependecies.
shar_SOURCES     = shar.c     local.h encode.c scripts.x
unshar_SOURCES   = unshar.c   local.h encode.c
uudecode_SOURCES = uudecode.c local.h
uuencode_SOURCES = uuencode.c local.h

$(bin_PROGRAMS): ../lib/libgnu.a

install-exec-hook: $(INSTALL_COMPRESS_LINK)

compress-link:
	test -f $(DESTDIR)$(bindir)/compress-dummy \
	  && ln -sf compress-dummy $(DESTDIR)$(bindir)/compress

## src/Makefile.am ends here
