
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
char * input_name = NULL;

static inline void try_putchar (int);
static void encode (void);

/* standard uuencoding translation table  */
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

/* Pointer to the translation table we currently use.  */
const char * trans_ptr = uu_std;

/* ENC is the basic 1 character encoding function to make a char printing.  */
#define ENC(Char) (trans_ptr[(Char) & 077])

static inline void
try_putchar (int c)
{
  if (putchar (c) == EOF)
    fserr (UUENCODE_EXIT_FAILURE, "putchar", _("standard output"));
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
      *(out++) = ENC ((lc & 0x0F) << 2);
      *(out++) = ENC (0);
    }

  return out - start;
}

/*------------------------------------------.
| Copy from IN to OUT, encoding as you go.  |
`------------------------------------------*/

static void
encode (void)
{
  int finishing = 0;

  do
    {
      unsigned char buf[45];
      char   buf_out[64];
      int    rdct = fread (buf, 1, sizeof(buf), stdin);
      size_t wrct = sizeof (buf_out);

      if (rdct <= 0)
        {
          if (ferror (stdin))
            fserr (UUENCODE_EXIT_FAILURE, "fread", input_name);
          break;
        }

      if (rdct < 45)
        {
          if (! feof (stdin))
            fserr (UUENCODE_EXIT_FAILURE, "fread", input_name);
          finishing = 1;
        }

      if (! HAVE_OPT(BASE64))
        {
          try_putchar (ENC ((unsigned int)rdct));
          wrct = encode_block (buf_out, buf, rdct);
        }
      else
        {
          base64_encode ((char *)buf, (size_t)rdct, buf_out, wrct);
          wrct = strlen(buf_out);
        }

      buf_out[wrct++] = '\n';
      if (fwrite (buf_out, 1, wrct, stdout) != wrct)
        fserr (UUENCODE_EXIT_FAILURE, "fwrite", _("standard output"));
    } while (! finishing);

  if (fclose (stdin) != 0)
    fserr (UUENCODE_EXIT_FAILURE, "fclose", input_name);

  if (! HAVE_OPT(BASE64))
    {
      try_putchar (ENC ('\0'));
      try_putchar ('\n');
    }
}

static void
process_opts (int argc, char ** argv, int * mode)
{
  /* Set global variables.  */
  setlocale (LC_ALL, "");

  /* Set the text message domain.  */
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  input_name = _("standard input");

  {
    int ct = optionProcess (&uuencodeOptions, argc, argv);
    argc -= ct;
    argv += ct;
  }

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
        input_name = *argv;
	if (fp != stdin)
          fserr (UUENCODE_EXIT_FAILURE, _("freopen of stdin"), input_name);
	if (fstat (fileno (stdin), &sb) != 0)
          fserr (UUENCODE_EXIT_FAILURE, "fstat", input_name);
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
      USAGE (UUENCODE_EXIT_USAGE_ERROR);
    }

  if (HAVE_OPT(ENCODE_FILE_NAME))
    {
      size_t nmlen = strlen (output_name);
      size_t bfsz  = nmlen + (nmlen / 3) + 4;;
      char * p = malloc (bfsz);
      if (p == NULL)
        fserr (UUENCODE_EXIT_FAILURE, "malloc", _("file name"));
      base64_encode ((char *)output_name, nmlen, p, bfsz);
      output_name = p;
    }
}

int
main (int argc, char ** argv)
{
  int mode;
  process_opts (argc, argv, &mode);

  if (printf ("begin%s%s %o %s\n",
              HAVE_OPT(BASE64) ? "-base64" : "",
              HAVE_OPT(ENCODE_FILE_NAME) ? "-encoded" : "",
	      mode, output_name) < 0)
    fserr (UUENCODE_EXIT_FAILURE, "printf", _("standard output"));

  encode ();

  if (ferror (stdout))
    fserr (UUENCODE_EXIT_FAILURE, "ferror", _("standard output"));
  if (puts (HAVE_OPT(BASE64) ? "====" : "end") == EOF)
    fserr (UUENCODE_EXIT_FAILURE, "puts", _("standard output"));
  if (fclose (stdout) != 0)
    fserr (UUENCODE_EXIT_FAILURE, "fclose", _("standard output"));

  exit (UUENCODE_EXIT_SUCCESS);
}
/*
 * Local Variables:
 * mode: C
 * c-file-style: "gnu"
 * indent-tabs-mode: nil
 * End:
 * end of src/uuencode.c */
