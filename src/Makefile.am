## Makefile for program src directory in GNU Shar utilities package.
##
## Copyright (C) 1995-2013 Free Software Foundation, Inc.
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
## You should have received a copy of the GNU General Public License along
## with this program.  If not, see <http://www.gnu.org/licenses/>.

## Process this file with automake to produce Makefile.in.

# stolen from fileutils-4.1.9:
# Sometimes, the expansion of $(LIBINTL) includes -lc which may
# include modules defining variables like `optind', so libfetish.a
# must precede $(LIBINTL) in order to ensure we use GNU getopt.
# But libgnu.a must also follow $(LIBINTL), since libintl uses
# replacement functions defined in libgnu.a.

LDADD = $(top_builddir)/libopts/libopts.a $(top_builddir)/lib/libgnu.a \
	$(LIBINTL) $(top_builddir)/lib/libgnu.a $(GETHOSTNAME_LIB) $(LIBSOCKET)

shar_LDADD      = $(LDADD)
unshar_LDADD    = $(LDADD)
bin_PROGRAMS    = shar unshar

shar_opt_code   = shar-opts.c     shar-opts.h
unshar_opt_code = unshar-opts.c   unshar-opts.h
encode_opt_code = uuencode-opts.c uuencode-opts.h
decode_opt_code = uudecode-opts.c uudecode-opts.h

shar_opts       = shar-opts.def
unshar_opts     = unshar-opts.def
encode_opts	= uuencode-opts.def
decode_opts	= uudecode-opts.def
options_defs    = shar-std.def $(shar_opts) $(unshar_opts) \
	$(encode_opts) $(decode_opts)
usage_texts     = \
	shar-full-help.txt shar-short-help.txt \
	unshar-full-help.txt unshar-short-help.txt \
	uudecode-full-help.txt uudecode-short-help.txt \
	uuencode-full-help.txt uuencode-short-help.txt

if AMDEP
if UUCODE
ENCODE_DEP_ARG  = -MF$(DEPDIR)/uuencode-opts.dep -MP -MT$@
@am__include@ @am__quote@./$(DEPDIR)/uuencode-opts.dep@am__quote@

DECODE_DEP_ARG  = -MF$(DEPDIR)/uudecode-opts.dep -MP -MT$@
@am__include@ @am__quote@./$(DEPDIR)/uudecode-opts.dep@am__quote@
endif
# UUCODE

SHAR_DEP_ARG  = -MF$(DEPDIR)/shar-opts.dep -MP
@am__include@ @am__quote@./$(DEPDIR)/shar-opts.dep@am__quote@

UNSHAR_DEP_ARG  = -MF$(DEPDIR)/unshar-opts.dep -MP
@am__include@ @am__quote@./$(DEPDIR)/unshar-opts.dep@am__quote@

else
# AMDEP
if UUCODE
ENCODE_DEP_ARG  =
DECODE_DEP_ARG  =
endif
# UUCODE
SHAR_DEP_ARG    =
UNSHAR_DEP_ARG  =
endif
# AMDEP

if UUCODE
  bin_PROGRAMS += uuencode uudecode
  uuencode_LDADD = $(LDADD)
  uudecode_LDADD = $(LDADD)
endif

EXTRA_SCRIPTS   = compress-dummy
EXTRA_DIST      = scripts.def scripts.tpl $(options_defs) $(usage_texts)
DISTCLEANFILES  = $(bin_SCRIPTS)

localedir   = $(datadir)/locale

AOINC       = -I$(top_srcdir)/libopts

# `autoopts-config cflags`
AM_CPPFLAGS = $(AOINC) -I$(srcdir) -I$(top_builddir) \
	-I$(top_srcdir)/lib -I$(top_builddir)/lib -I$(top_builddir)/intl

DEFS        = -DLOCALEDIR=\"$(localedir)\" @DEFS@

# Source dependecies.

if UUCODE
uudecode_SOURCES = uudecode.c local.h $(decode_opt_code)
uuencode_SOURCES = uuencode.c local.h $(encode_opt_code)

if AMDEP
uuencode-opts.c uuencode-opts.h : $(DEPDIR)/uuencode-opts.dep
$(DEPDIR)/uuencode-opts.dep : uuencode-opts.def shar-std.def
	$(AUTOGEN) $(ENCODE_DEP_ARG) uuencode-opts.def

uudecode-opts.c uudecode-opts.h : $(DEPDIR)/uudecode-opts.dep
$(DEPDIR)/uudecode-opts.dep : uudecode-opts.def shar-std.def
	$(AUTOGEN) $(DECODE_DEP_ARG) uudecode-opts.def
endif
# AMDEP
endif
# UUCODE

shar_SOURCES    = shar.c   local.h encode.c scripts.x $(shar_opt_code)
unshar_SOURCES  = unshar.c local.h encode.c $(unshar_opt_code)

if AMDEP
shar-opts.c shar-opts.h : $(DEPDIR)/shar-opts.dep
$(DEPDIR)/shar-opts.dep : shar-opts.def shar-std.def
	$(AUTOGEN) $(SHAR_DEP_ARG) shar-opts.def

unshar-opts.c unshar-opts.h : $(DEPDIR)/unshar-opts.dep
$(DEPDIR)/unshar-opts.dep : unshar-opts.def shar-std.def
	$(AUTOGEN) $(UNSHAR_DEP_ARG) unshar-opts.def
endif
# AMDEP

$(bin_PROGRAMS): $(top_builddir)/lib/libgnu.a

install-exec-hook: $(INSTALL_COMPRESS_LINK)

compress-link:
	test -f $(DESTDIR)$(bindir)/compress-dummy \
	  && ln -sf compress-dummy $(DESTDIR)$(bindir)/compress

help-text : mk-help.sh $(bin_PROGRAMS)
	touch $@
	$(SHELL) mk-help.sh $@ $(bin_PROGRAMS)

## src/Makefile.am ends here
