## Makefile for the doc subdirectory of the GNU NLS Utilities
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
## with this program; if not, write to the Free Software Foundation, Inc.,
## 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

## Process this file with automake to produce Makefile.in.

info_TEXINFOS   = sharutils.texi
texi_files      = fdl.texi version.texi \
	invoke-shar.texi invoke-unshar.texi \
	invoke-uudecode.texi invoke-uuencode.texi

cmds            = shar unshar uuencode uudecode

dist_man1_MANS  = shar.1 unshar.1
dist_man5_MANS  =
man_files       = $(cmds:=.1) uuencode.5

if UUCODE
  dist_man1_MANS += uuencode.1 uudecode.1
  dist_man5_MANS += uuencode.5
endif

CLEANFILES  = $(man1_MANS)
EXTRA_DIST  = $(texi_files) $(man_files)

sharutils.txt :
	makeinfo --no-split --fill-column=70 --paragraph-indent=0 --no-headers \
		--output=$@ sharutils.texi

gnudocs:
	title=`sed -n '/^@settitle/{;s/@settitle *//;p;q;}' sharutils.texi` ; \
	$(SHELL) $(top_srcdir)/gendocs.sh sharutils "$$title"

.PHONY : gnudocs
