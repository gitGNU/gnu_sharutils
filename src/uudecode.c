
static char const cright_years_z[] =

/* uudecode utility.
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

#define  UUDECODE_C  1
#include "uudecode-opts.h"
#include "liballoca.h"

#if HAVE_LOCALE_H
#else
# define setlocale(Category, Locale)
#endif
#ifndef _
# define _(str) (str)
#endif

/*=====================================================================\
| uudecode [FILE ...]						       |
| 								       |
| Create the specified FILE, decoding as you go.  Used with uuencode.  |
\=====================================================================*/

#include <pwd.h>

#define UU_MODE_BITS(_M)        ((_M) & (S_IRWXU | S_IRWXG | S_IRWXO))
#if HAVE_FCHMOD
#define UU_CHMOD(_n, _fd, _m)   fchmod ((_fd), UU_MODE_BITS(_m))
#else
#define UU_CHMOD(_n, _fd, _m)   chmod ((_n), UU_MODE_BITS(_m))
#endif

struct passwd *getpwnam ();

static uudecode_exit_code_t read_stduu (const char *inname, const char *outname);
static uudecode_exit_code_t read_base64(const char *inname, const char *outname);
static uudecode_exit_code_t decode (const char * in_name);

#ifndef NUL
#define NUL '\0'
#endif

#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif

/* Single character decode.  */
#define	DEC(Char) (((Char) - ' ') & 077)

#if !defined S_ISLNK && defined S_IFLNK
# define S_ISLNK(m) (((m) & S_IFMT) == S_IFLNK)
#endif

#define TRY_PUTCHAR(c)                                          \
  do {                                                          \
    if (putchar (c) == EOF)                                     \
      fserr (UUDECODE_EXIT_NO_OUTPUT, "putchar", outname);      \
  } while (0)

static uudecode_exit_code_t
read_stduu (char const * inname, char const * outname)
{
  char buf[2 * BUFSIZ];

  while (1)
    {
      int n;
      char * p = buf;

      if (fgets ((char *) buf, sizeof(buf), stdin) == NULL)
        die (UUDECODE_EXIT_INVALID, _("%s: Short file"), inname);

      /* N is used to avoid writing out all the characters at the end of
	 the file.  */

      n = DEC (*p);
      if (n <= 0)
	break;
      for (++p; n >= 3; p += 4, n -= 3)
	{
	  TRY_PUTCHAR (DEC (p[0]) << 2 | DEC (p[1]) >> 4);
	  TRY_PUTCHAR (DEC (p[1]) << 4 | DEC (p[2]) >> 2);
	  TRY_PUTCHAR (DEC (p[2]) << 6 | DEC (p[3]));
        }
      switch (n)
        {
        case 0: break;
        case 1:
	  TRY_PUTCHAR (DEC (p[0]) << 2 | DEC (p[1]) >> 4);
          break;

        case 2:
	  TRY_PUTCHAR (DEC (p[0]) << 2 | DEC (p[1]) >> 4);
          TRY_PUTCHAR (DEC (p[1]) << 4 | DEC (p[2]) >> 2);
          break;
        }
    }

  /*
   * We must now find an "end" line, with or without a '\r' character.
   */
  do {
    if (fgets (buf, sizeof(buf), stdin) == NULL)
      break;

    if (buf[0] != 'e') break;
    if (buf[1] != 'n') break;
    if (buf[2] != 'd') break;
    if (buf[3] == '\n')
      return UUDECODE_EXIT_SUCCESS;

    if (buf[3] != '\r') break;
    if (buf[4] == '\n')
      return UUDECODE_EXIT_SUCCESS;
  } while (0);

  die (UUDECODE_EXIT_INVALID, _("%s: No `end' line"), inname);
  /* NOTREACHED */
  return UUDECODE_EXIT_INVALID;
}

static uudecode_exit_code_t
read_base64 (char const * inname, char const * outname)
{
  char buf_in[2 * BUFSIZ];
  char buf_out[2 * BUFSIZ];
  struct base64_decode_context ctx;
  base64_decode_ctx_init (&ctx);

  for (;;)
    {
      size_t outlen = sizeof(buf_out);

      if (fgets (buf_in, sizeof(buf_in), stdin) == NULL)
	fserr (UUDECODE_EXIT_INVALID, _("%s: Short file"), inname);

      if (memcmp (buf_in, "====", 4) == 0)
	break;

      if (! base64_decode_ctx (&ctx, buf_in, strlen(buf_in), buf_out, &outlen))
        die (UUDECODE_EXIT_INVALID, _("%s: invalid input"), inname);

      if (fwrite (buf_out, outlen, 1, stdout) != 1)
        fserr (UUDECODE_EXIT_NO_OUTPUT, "fwrite", outname);
    }

  return UUDECODE_EXIT_SUCCESS;
}

/**
 * replace a leading tilde character with either the home directory of
 * the login name that follows it, or with the home directory of the
 * current user if a directory separator follows it.
 *
 * @param[in] buf  the original file name
 * @returns either the expanded name or the original buffer address.
 */
static char *
expand_tilde (char * buf)
{
  char * outname;
  char const * homedir;

  if (buf[1] == '/')
    {
      homedir = getenv("HOME");
      if (homedir == NULL)
        die (UUDECODE_EXIT_INVALID, _("cannot expand $HOME"));
      buf += 2;
    }
  else
    {
      struct passwd *pw;
      char * pz = buf + 1;
      while (*pz != '/')
        ++pz;
      if (*pz == NUL)
        {
          error (0, 0, _("%s: Illegal file name: %s"), program_name, buf);
          return NULL;
        }
      *pz++ = NUL;
      pw = getpwnam (buf + 1);
      if (pw == NULL)
        {
          error (0, 0, _("No user '%s'"), buf + 1);
          return NULL;
        }
      homedir = pw->pw_dir;
      buf = pz;
    }

  {
    size_t sz = strlen (homedir) + strlen (buf) + 2;
    outname = (char *) malloc (sz);
    if (outname == NULL)
      fserr (UUDECODE_EXIT_NO_MEM, "malloc", _("output file name"));
  }

  sprintf (outname, "%s/%s", homedir, buf);
  return outname;
}

/**
 * Create output file and set mode.
 */
static uudecode_exit_code_t
reopen_output (char const * outname, int mode)
{
  /* Check out file if it exists */
  if (!access(outname, F_OK))
    {
      struct stat attr;
      if (lstat(outname, &attr) == -1)
        {
          error (0, errno, _("cannot access %s"), outname);
          return UUDECODE_EXIT_NO_OUTPUT;
        }
    }

  {
    FILE * fp = freopen (outname, FOPEN_WRITE_BINARY, stdout);
    if (fp != stdout)
      fserr (UUDECODE_EXIT_NO_OUTPUT, "freopen", outname);
  }

  if (UU_CHMOD(outname, STDOUT_FILENO, mode) != 0)
    {
      error (0, errno, _("chmod of %s"), outname);
      /*
       * http://austingroupbugs.net/view.php?id=635
       * if the mode bits cannot be set, uudecode shall
       * not treat this as an error.
       */
      if (! HAVE_OPT(IGNORE_CHMOD))
        {
          char const * p = getenv("POSIXLY_CORRECT");
          if (p == NULL)
            return UUDECODE_EXIT_NO_OUTPUT;
        }
    }

  return UUDECODE_EXIT_SUCCESS;
}

static void
decode_fname (char * buf)
{
  size_t sz  = strlen (buf);
  char * out = malloc (2*sz + 4);

  if (sz == 0)
    die (UUDECODE_EXIT_INVALID, _("output name is empty"));

  {
    char * tmp = out + sz + 4;

    if (out == NULL)
      fserr(UUDECODE_EXIT_NO_MEM, "malloc", _("output file name"));

    memcpy(out, buf, sz);
    out[sz] = '\0';
    if (! base64_decode (out, sz, tmp, &sz))
      die (UUDECODE_EXIT_INVALID, _("invalid base64 encoded name: %s"), buf);
    memcpy (buf, tmp, sz);
    buf[sz] = '\0';
  }
  free (out);
}

/**
 * Decode one file.
 *
 * @param[in] inname  name of encoded file.  "stdin" means either a file
 *                    by that name, or the unnamed standard input.
 *
 * @returns one of the enumerated values of uudecode_exit_code_t.
 */
static uudecode_exit_code_t
decode (char const * inname)
{
  char *pz;
  int   mode;
  char  buf[2 * BUFSIZ] = { NUL };
  char *outname;
  bool  allocated_outname = false;
  bool  encoded_fname = false;
  uudecode_exit_code_t rval;
  bool  do_base64 = false;

  /* Search for header line.  */

  while (1)
    {
      if (fgets (buf, sizeof (buf), stdin) == NULL)
	{
        bad_beginning:
	  die (UUDECODE_EXIT_INVALID,
               _("%s: Invalid or missing 'begin' line"), inname);
	}

      if (strncmp (buf, "begin", 5) == 0)
	{
          char * scan = buf+5;
          if (*scan == '-')
            {
              static char const base64[]  = "ase64";
              static char const encoded[] = "encoded";
            check_begin_option:
              if (*++scan == 'b')
                {
                  if (strncmp (scan+1, base64, sizeof (base64) - 1) != 0)
                    goto bad_beginning;
                  if (do_base64)
                    goto bad_beginning;
                  do_base64 = true;
                  scan += sizeof (base64); /* chars + 'b' */
                }
              else
                {
                  if (strncmp (scan, encoded, sizeof (encoded) - 1) != 0)
                    goto bad_beginning;
                  if (encoded_fname)
                    goto bad_beginning;
                  encoded_fname = true;
                  scan += sizeof (encoded) - 1; /* 'e' is included */
                }

              switch (*scan) {
              case ' ': break; /* no more begin options */
              case '-': goto check_begin_option;
              default:  goto bad_beginning;
              }
	    }

	  if (sscanf (scan, " %o %[^\n]", &mode, buf) == 2)
	    break;
          goto bad_beginning;
	}
    }

  /* If the output file name is given on the command line this rules.  */
  if (HAVE_OPT(OUTPUT_FILE))
    outname = (char *) OPT_ARG(OUTPUT_FILE);
  else
    {
      if (encoded_fname)
        decode_fname (buf);

      if (buf[0] != '~')
	outname = buf;
      else
        {
          /* Handle ~user/file format.  */

          outname = expand_tilde (buf);
          if (outname == NULL)
            return UUDECODE_EXIT_NO_OUTPUT;
          allocated_outname = true;
        }

      /* Be certain there is no trailing white space */
      pz = outname + strlen (outname);
      while ((pz > outname) && isspace ((int)pz[-1]))
        pz--;
      *pz = NUL;
    }

  if (  (strcmp (outname, "/dev/stdout") != 0)
     && (strcmp (outname, "-") != 0) )
    {
      rval = reopen_output (outname, mode);
      if (rval != UUDECODE_EXIT_SUCCESS)
        goto fail_return;
    }

  /* We use different functions for different encoding methods.
     A common function would slow down the program.  */

  /* For each input line:  */
  if (do_base64)
    rval = read_base64 (inname, outname);
  else
    rval = read_stduu (inname, outname);

  if (  (rval == UUDECODE_EXIT_SUCCESS)
     && (ferror(stdout) || fflush(stdout) != 0))
    {
      error (0, 0, _("%s: Write error"), outname);
      rval = UUDECODE_EXIT_NO_OUTPUT;
    }

 fail_return:

  if (allocated_outname)
    free (outname);
  return rval;
}

/**
 * The main procedure.
 */
int
main (int argc, char const * const * argv)
{
  uudecode_exit_code_t exit_status = UUDECODE_EXIT_SUCCESS;

  setlocale (LC_ALL, "");

  /* Set the text message domain.  */
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  {
    int ct = optionProcess (&uudecodeOptions, argc, (char **)(void *)argv);
    argc -= ct;
    argv += ct;
  }

  switch (argc)
    {
    case 0:
      exit_status = decode (_("standard input"));
      break;

    default:
      if (HAVE_OPT(OUTPUT_FILE))
        {
          usage_message(_("You cannot specify an output file when processing\n\
multiple input files.\n"));
          /* NOTREACHED */
        }
      /* FALLTHROUGH */

    case 1:
      while (--argc >= 0)
        {
          char const * f = *(argv++);

          if (freopen (f, "r", stdin) != NULL)
            {
              exit_status |= decode (f);
            }
          else
            {
              error (0, errno, "%s", f);
              exit_status |= UUDECODE_EXIT_NO_INPUT;
            }
        }
    }

  exit (exit_status);
}

/*
 * Local Variables:
 * mode: C
 * c-file-style: "gnu"
 * indent-tabs-mode: nil
 * End:
 * end of src/uudecode.c */
