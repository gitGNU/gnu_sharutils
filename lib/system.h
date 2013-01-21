/* Shared definitions for GNU shar utilities.

   Copyright (C) 1994-2013 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef HAVE_CONFIG_H
choke me -- I need config.h
#endif
#include "config.h"

#ifndef USE_UNLOCKED_IO
#  define USE_UNLOCKED_IO 1
#endif

#include <sys/types.h>
#include <sys/stat.h>

#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <stdlib.h>

#if HAVE_INTTYPES_H
# include <inttypes.h>
#endif

#ifndef HAVE_INTMAX_T
#define HAVE_INTMAX_T
typedef long intmax_t;
#endif

#ifdef HAVE_STDBOOL_H
#include <stdbool.h>
#else
typedef enum {false = 0, true = 1} bool;
#endif

#if !HAVE_DECL_STRTOIMAX && !defined strtoimax
intmax_t strtoimax ();
#endif

#if HAVE_STRING_H
# include <string.h>
# if !STDC_HEADERS && HAVE_MEMORY_H
#  include <memory.h>
# endif
#else
# include <strings.h>
#endif

#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif

/* System functions.  Even if we usually avoid declaring them, we cannot
   avoid them all.  */

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

FILE *fdopen ();
long ftell ();
FILE *popen ();

/* Global functions of the shar package.  */

void copy_file_encoded (FILE *, FILE *);
char *get_submitter (char *);

/* Debugging the memory allocator.  */

#if WITH_DMALLOC
# define MALLOC_FUNC_CHECK
# include <dmalloc.h>
#endif

/* Some gcc specials.  */
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 6)
# define ATTRIBUTE(list) __attribute__ (list)
#else
# define ATTRIBUTE(list)
#endif

#if __CYGWIN__
# include <fcntl.h>
# include <io.h> /* for setmode */
#endif

#if ! defined(O_BINARY) || (O_BINARY == 0)
# define  FOPEN_READ_BINARY   "r"
# define  FOPEN_WRITE_BINARY  "w"
#else
# define  FOPEN_READ_BINARY   "rb"
# define  FOPEN_WRITE_BINARY  "wb"
#endif
