
static const char cright_years_z[] =

/* uuencode utility.
   Copyright (C) */ "1994-2013";

/* Free Software Foundation, Inc.

   This product is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This product is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see:  http://www.gnu.org/licenses.
*/

/* Copyright (c) 1983 Regents of the University of California.
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
   3. All advertising materials mentioning features or use of this software
      must display the following acknowledgement:
	 This product includes software developed by the University of
	 California, Berkeley and its contributors.
   4. Neither the name of the University nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
   FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
   OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
   OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
   SUCH DAMAGE.  */

/* Reworked to GNU style by Ian Lance Taylor, ian@airs.com, August 93.  */

/*=======================================================\
| uuencode [INPUT] OUTPUT				 |
| 							 |
| Encode a file so it can be mailed to a remote system.	 |
\=======================================================*/

#define  UUENCODE_C  1
#include "local.h"
#include "uuencode-opts.h"

#if HAVE_LOCALE_H
#else
# define setlocale(Category, Locale)
#endif

#if __CYGWIN__
# include <fcntl.h>
# include <io.h>
#endif

#define	IRWALL_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)
char * output_name = NULL;

static inline void try_putchar (int);
static void encode (void);

/* The name this program was run with. */
const char *program_name;

/* The two currently defined translation tables.  The first is the
   standard uuencoding, the second is base64 encoding.  */
const char uu_std[64] =
{
  '`', '!', '"', '#', '$', '%', '&', '\'',
  '(', ')', '*', '+', ',', '-', '.', '/',
  '0', '1', '2', '3', '4', '5', '6', '7',
  '8', '9', ':', ';', '<', '=', '>', '?',
  '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
  'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
  'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
  'X', 'Y', 'Z', '[', '\\', ']', '^', '_'
};

const char uu_base64[64] =
{
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
  'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
  'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
  'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
  'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
  'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
  'w', 'x', 'y', 'z', '0', '1', '2', '3',
  '4', '5', '6', '7', '8', '9', '+', '/'
};

/* Pointer to the translation table we currently use.  */
const char * trans_ptr = uu_std;

/* ENC is the basic 1 character encoding function to make a char printing.  */
#define ENC(Char) (trans_ptr[(Char) & 077])

static inline void
try_putchar (int c)
{
  if (putchar (c) == EOF)
    error (EXIT_FAILURE, 0, _("Write error"));
}

/*------------------------------------------------.
| Copy from IN to OUT, encoding as you go along.  |
`------------------------------------------------*/

static size_t
encode_block (char * out, unsigned char const * in, size_t in_len)
{
  char * start = out;

  while (in_len >= 3)
    {
      *(out++) = ENC (in[0] >> 2);
      *(out++) = ENC (((in[0] & 0x03) << 4) + ((in[1] >> 4) & 0x0F));
      *(out++) = ENC (((in[1] & 0x0F) << 2) + ((in[2] >> 6) & 0x03));
      *(out++) = ENC (in[2] & 0x3F);
      in_len -= 3;
      in += 3;
    }

  if (in_len > 0)
    {
      unsigned char lc = (in_len > 1) ? in[1] : '\0';
      *(out++) = ENC (in[0] >> 2);
      *(out++) = ENC (((in[0] & 0x03) << 4) + ((lc >> 4) & 0x0F));

      if (HAVE_OPT(BASE64))
        {
          *(out++) = (in_len > 1) ? ENC ((lc & 0x0F) << 2) : '=';
          *(out++) = '=';
        }
      else
        {
          *(out++) = ENC ((lc & 0x0F) << 2);
          *(out++) = ENC (0);
        }
    }

  *(out++) = '\n';
  return out - start;
}

/*------------------------------------------------.
| Copy from IN to OUT, encoding as you go along.  |
`------------------------------------------------*/

static void
encode (void)
{
  register int n;
  int finishing = 0;
  unsigned char buf[45];
  char buf_out[60];

  while ( !finishing && (n = fread (buf, 1, sizeof(buf), stdin)) > 0 )
    {
      if (n < 45)
        {
          if (feof (stdin))
            finishing = 1;
          else
            error (EXIT_FAILURE, 0, _("Read error"));
        }

      if (! HAVE_OPT(BASE64))
        try_putchar (ENC (n));

      n = encode_block (buf_out, buf, n);
      if (fwrite (buf_out, 1, n, stdout) != n)
        error (EXIT_FAILURE, errno, _("Write error"));
    }

  if (ferror (stdin))
    error (EXIT_FAILURE, 0, _("Read error"));
  if (fclose (stdin) != 0)
    error (EXIT_FAILURE, errno, _("Read error"));

  if (! HAVE_OPT(BASE64))
    {
      try_putchar (ENC ('\0'));
      try_putchar ('\n');
    }
}

static void
process_opts (int argc, char ** argv, int * mode)
{
  int ct = optionProcess (&uuencodeOptions, argc, argv);
  argc -= ct;
  argv += ct;

  switch (argc)
    {
    case 2:
      /* Optional first argument is input file.  */
      {
        struct stat sb;
#if S_IRWXU != 0700
        choke me - Must translate mode argument
#endif

	FILE * fp = freopen (*argv, FOPEN_READ_BINARY, stdin);
	if (fp != stdin)
	  error (EXIT_FAILURE, errno, _("fopen-ing %s"), *argv);
	if (fstat (fileno (stdin), &sb) != 0)
	  error (EXIT_FAILURE, errno, _("fstat-ing %s"), *argv);
	*mode = sb.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO);
        output_name = argv[1];
	break;
      }

    case 1:
#if __CYGWIN__
      if (! isatty (STDIN_FILENO))
       setmode (STDIN_FILENO, O_BINARY);
#endif

      *mode = IRWALL_MODE & ~umask (IRWALL_MODE);
      output_name = *argv;
      break;

    case 0:
    default:
      USAGE (EXIT_FAILURE);
    }

  if (HAVE_OPT(ENCODE_FILE_NAME))
    {
      size_t nmlen = strlen (output_name);
      char * p = malloc (nmlen + (nmlen > 1) + 3);
      if (p == NULL)
        error (EXIT_FAILURE, ENOMEM, _("Allocation failure"));
      nmlen = encode_block (p, (unsigned char *)output_name, nmlen);
      if (HAVE_OPT(BASE64))
        {
          while ((nmlen > 0) && (p[nmlen-1] == '='))
            nmlen--;
        }
      p[nmlen] = '\0';
      output_name = p;
    }
}

int
main (int argc, char ** argv)
{
  int mode;
  /* Set global variables.  */
  program_name = argv[0];
  setlocale (LC_ALL, "");

  /* Set the text message domain.  */
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  process_opts (argc, argv, &mode);

  if (printf ("begin%s%s %o %s\n",
              HAVE_OPT(BASE64) ? "-base64" : "",
              HAVE_OPT(ENCODE_FILE_NAME) ? "-encoded" : "",
	      mode, output_name) < 0)
    error (EXIT_FAILURE, errno, _("Write error"));

  encode ();

  if (ferror (stdout) ||
      puts (HAVE_OPT(BASE64) ? "====" : "end") == EOF ||
      fclose (stdout) != 0)
    error (EXIT_FAILURE, errno, _("Write error"));

  exit (EXIT_SUCCESS);
}
/*
 * Local Variables:
 * mode: C
 * c-file-style: "gnu"
 * indent-tabs-mode: nil
 * End:
 * end of src/uuencode.c */
