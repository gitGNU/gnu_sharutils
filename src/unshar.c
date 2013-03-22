
/* Handle so called 'shell archives'.
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
   along with this program; if not, see:  http://www.gnu.org/licenses.
*/

/* Unpackage one or more shell archive files.  The 'unshar' program is a
   filter which removes the front part of a file and passes the rest to
   the 'sh' command.  It understands phrases like "cut here", and also
   knows about shell comment characters and the Unix commands 'echo',
   'cat', and 'sed'.  */

#define UNSHAR_C 1
#include "unshar-opts.h"

#include <sys/stat.h>

#include <ctype.h>
#include <fcntl.h>

#include "xgetcwd.h"

/*
 * FIXME: actually configure this stuff:
 */
#if defined(_SC_PAGESIZE)
# define GET_PAGE_SIZE  sysconf (_SC_PAGESIZE)
#elif defined(_SC_PAGE_SIZE)
# define GET_PAGE_SIZE  sysconf (_SC_PAGE_SIZE)
#elif defined(HAVE_GETPAGESIZE)
# define GET_PAGE_SIZE  getpagesize()
#else
# define GET_PAGE_SIZE  8192
#endif

#ifndef HAVE_LOCALE_H
# define setlocale(Category, Locale)
#endif

#define EOL '\n'

/** amount of data to read and write at once */
size_t       rw_base_size = 0;

/** a buffer of that size */
char *       rw_buffer    = NULL;

/**
 * see if the line looks like C program code.
 * Check for start of comment ("//" or "/*") and directives.
 *
 * @param[in] buf buffer containing current input line
 * @returns true or false
 */
static bool
looks_like_c_code (char const * buf)
{
  static char const * const directives[] = {
    "include", "define", "ifdef", "ifndef", "if",
    "pragma", "undef", "elif", "error", "line", NULL };

  while (isspace ((int) *buf))  buf++;
  switch (*(buf++))
    {
    case '#':
      {
        int ix = 0;
        while (isspace ((int) *buf))  buf++;
        for (;;) {
          char const * dir = directives[ix++];
          size_t ln;
          if (dir == NULL)
            return false;
          ln = strlen (dir);
          if ((strncmp (buf, dir, ln) == 0) && isspace ((int)buf[ln]))
            return true;
        }
      }

    case '/':
      return ((*buf == '*') || (*buf == '/'));

    default: return false;
    }
}

/**
 * see if the line looks like shell program code.
 * Check for start of comment ("#") and a few commands (":" and others).
 *
 * @param[in] buf buffer containing current input line
 * @returns true or false
 */
static bool
looks_like_shell_code (char const * buf)
{
  while (isspace ((int) *buf))  buf++;
  switch (*buf)
    {
    case '#': case ':':
      return true;
    default:
      if (islower ((int)*buf))
        break;
      return false;
    }

  {
    static char const * const cmn_cmds[] = {
      "echo", "sed", "cat", "if", NULL };

    int ix = 0;
    for (;;) {
      char const * cmd = cmn_cmds[ix++];
      size_t ln;
      if (cmd == NULL)
        return false;
      ln = strlen (cmd);
      if ((strncmp (buf, cmd, ln) == 0) && isspace ((int)buf[ln]))
        return true;
    }
  }
}

/**
 * see if the line contains a cut mark.
 * "cut" can be spelled either that way or as "tear" and is either
 * all lower case or all upper case.  That word must be followed by either
 * another copy of that word or the word "here", and both must be in the
 * same case.  (All upper or all lower.)
 *
 * @param[in] buf buffer containing current input line
 * @returns true or false
 */
static bool
this_is_cut_line (char const * buf)
{
  static char const * const kwds[] = {
    "cut", "CUT", "tear", "TEAR" };
  static char const * const here_here[] = {
    "here", "HERE" };
  int ix = 0;

  for (;;)
    {
      char const * kw  = kwds[ix];
      char const * fnd = strstr (buf, kw);
      if (fnd != NULL)
        {
          buf = fnd + strlen (kw);
          break;
        }

      if (++ix >= 4)
        return false;
    }

  {
    char const * fnd = strstr (buf, kwds[ix]);
    if (fnd != NULL)
      return true;
    fnd = strstr (buf, here_here[ix & 1]);
    return (fnd != NULL);
  }
}

/**
 * Check the line following a cut line.  The next non-blank line
 * must start with a command or a shell comment character ('#').
 *
 * @param[in]  name     input file name (for error message)
 * @param[in]  file     file pointer of input
 *
 * @returns true if valid, false otherwise
 */
static bool
next_line_is_valid (char const * name, FILE * file)
{
  off_t position;

  /* Read next line after "cut here", skipping blank lines.  */

  for (;;)
    {
      char * buf;
      position = ftell (file);
      buf = fgets (rw_buffer, rw_base_size, file);
      if (buf == NULL)
        {
          error (0, 0, _("Found no shell commands after cut line in %s"), name);
          return false;
        }

      while (isspace ((int)*buf))  buf++;
      if (*buf != '\0')
        break;
    }

  /* Win if line starts with a comment character of lower case letter.  */
  if (islower ((int)*rw_buffer) || (*rw_buffer == '#') || (*rw_buffer == ':'))
    {
      fseek (file, position, SEEK_SET);
      return true;
    }

  /* Cut here message lied to us.  */

  error (0, 0, _("%s is probably not a shell archive"), name);
  error (0, 0, _("The 'cut line' was followed by: %s"), rw_buffer);
  return false;
}

/**
 * Associated with a given file NAME, position FILE at the start of the
 * shell command portion of a shell archive file.  Scan file from position
 * START.
 */
static bool
find_archive (char const * name, FILE * file, off_t start)
{
  fseek (file, start, SEEK_SET);

  while (1)
    {
      /* Record position of the start of this line.  */
      off_t position = ftell (file);

      /* Read next line, fail if no more and no previous process.  */
      if (!fgets (rw_buffer, BUFSIZ, file))
	{
	  if (!start)
	    error (0, 0, _("Found no shell commands in %s"), name);
	  return false;
	}

      /*
       * Bail out if we see C preprocessor commands or C comments.
       */
      if (looks_like_c_code (rw_buffer))
	{
	  error (0, 0, _("%s looks like raw C code, not a shell archive"),
		 name);
	  return false;
	}

      /* Does this line start with a shell command or comment.  */

      if (looks_like_shell_code (rw_buffer))
	{
	  fseek (file, position, SEEK_SET);
	  return true;
	}

      /*
       * Does this line say "Cut here"?
       */
      if (this_is_cut_line (rw_buffer))
        return next_line_is_valid (name, file);
    }
}

/**
 * load a file that does not support ftell/seek.
 * Create a temporary file and suck up all the file data, writing it out
 * to the temporary file.  Return a file pointer to this temporary file,
 * and a pointer to the allocated string containing the file name.
 * The file must be removed and the name pointer deallocated.
 *
 * @param[out] tmp_fname    temporary file name
 * @param[in]  infp         input data file pointer (usually stdin)
 *
 * @returns a file pointer to the temporary file
 */
static FILE *
load_file (char const ** tmp_fname, FILE * infp)
{
  static const char z_tmpfile[] = "unsh.XXXXXX";
  char * pz_fname;
  FILE * outfp;

  {
    size_t name_size;
    char *pz_tmp = getenv ("TMPDIR");

    if (pz_tmp == NULL)
      pz_tmp = "/tmp";

    name_size = strlen (pz_tmp) + sizeof (z_tmpfile) + 1;
    *tmp_fname = pz_fname = malloc (name_size);

    if (pz_fname == NULL)
      fserr (UNSHAR_EXIT_NOMEM, "malloc", _("file name buffer"));
    
    sprintf (pz_fname, "%s/%s", pz_tmp, z_tmpfile);
  }

  {
    int fd = mkstemp (pz_fname);
    if (fd < 0)
      fserr (UNSHAR_EXIT_CANNOT_CREATE, "mkstemp", z_tmpfile);
    
    outfp = fdopen (fd, "w+");
  }

  if (outfp == NULL)
    fserr (UNSHAR_EXIT_CANNOT_CREATE, "fdopen", pz_fname);

  for (;;)
    {
      size_t size_read = fread (rw_buffer, 1, rw_base_size, infp);
      if (size_read == 0)
        break;
      fwrite (rw_buffer, size_read, 1, outfp);
    }

  rewind (outfp);

  return outfp;
}

/**
 * Unarchive a shar file provided on file NAME.  The file itself is
 * provided on the already opened FILE pointer.  If the file position
 * cannot be determined, we suck up all the data and use a temporary.
 */
int
unshar_file (const char * name, FILE * file)
{
  char const * tmp_fname = NULL;
  off_t curr_pos = ftell (file);

  if (curr_pos < 0)
    {
      file = load_file (&tmp_fname, file);
      curr_pos = ftell (file);
      if (curr_pos < 0)
        fserr (UNSHAR_EXIT_CANNOT_CREATE, "ftell", tmp_fname);
    }
      
  while (find_archive (name, file, curr_pos))
    {
      char const * cmd = HAVE_OPT(OVERWRITE) ? "sh -s - -c" : "sh";
      FILE * shell_fp = popen (cmd, "w");

      if (shell_fp == NULL)
	fserr (UNSHAR_EXIT_POPEN_PROBLEM, "popen", cmd);
      if (HAVE_OPT(DIRECTORY))
        fprintf (shell_fp, "cd %s >/dev/null || exit 1\n", OPT_ARG(DIRECTORY));
      printf ("%s:\n", name);
      if (! HAVE_OPT(SPLIT_AT))
	{
          for (;;)
            {
              size_t len = fread (rw_buffer, 1, rw_base_size, file);
              if (len == 0)
                break;
              fwrite (rw_buffer, 1, len, shell_fp);
            }

	  pclose (shell_fp);
	  break;
	}
      else
	{
          char * text_in;

	  while (text_in = fgets (rw_buffer, rw_base_size, file),
		 text_in != NULL)
	    {
	      fputs (rw_buffer, shell_fp);
	      if (!strncmp (OPT_ARG(SPLIT_AT), rw_buffer, separator_str_len))
		break;
	    }
	  pclose (shell_fp);

	  if (text_in == NULL)
	    break;

          curr_pos = ftell (file);
	}
    }

  if (tmp_fname != NULL)
    {
      unlink (tmp_fname);
      free ((void *)tmp_fname);
    }

  return UNSHAR_EXIT_SUCCESS;
}

/**
 * unshar initialization.  Do all the initializations necessary to be ready
 * to start processing files.  I18N stuff, file descriptors for source and
 * destination files, and allocation of I/O buffers.
 *
 * This routine works or does not return.
 */
void
init_unshar (void)
{
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);
#ifdef __MSDOS__
  setbuf (stdout, NULL);
  setbuf (stderr, NULL);
#endif

  rw_base_size = GET_PAGE_SIZE;
  rw_buffer    = malloc (rw_base_size);
  if (rw_buffer == NULL)
    fserr (UNSHAR_EXIT_NOMEM, "malloc", _("read/write buffer"));
}

/*
 * Local Variables:
 * mode: C
 * c-file-style: "gnu"
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 * end of unshar.c */
