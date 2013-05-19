
static const char cright_years_z[] =

/* Handle so called `shell archives'.
   Copyright (C) */ "1994-2013";

/* Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
   A note about translated strings:

   There are several categories of English text that get emitted by this
   program:

   1)  Error messages by this program.  These should all be literals
       in this program text and should be surrounded by _() macro calls.

   2)  Error messages emitted by the shell script emitted by this
       program.  These messages *MUST ALL BE DEFINED* in the 'shar-msg'
       table defined in scripts.def.  The emitted script will try to
       translate them before echoing them out to the user.

   3)  Shell script comments that are inserted into the output.
       These English comments are *NEVER* translated.  They appear both
       as literal text in this program, and as literal script text in
       the 'text' table in "scripts.def".  Localization of "shar" has
       no effect on these comment strings.
 */
#define  SHAR_C  1
#include "shar-opts.h"

#include <ctype.h>
#ifdef HAVE_LIMITS_H
# include <limits.h>
#endif
#include <time.h>

#include "inttostr.h"
#include "liballoca.h"
#include "md5.c"
#include "md5.h"
#include "quotearg.h"
#include "xalloc.h"
#include "xgetcwd.h"

#if HAVE_LOCALE_H
#else
# define setlocale(Category, Locale)
#endif

#ifndef NUL
#  define NUL '\0'
#endif

#include "scripts.x"

/* Character which goes in front of each line.  */
#define DEFAULT_LINE_PREFIX_1 'X'

/* Character which goes in front of each line if here_delimiter[0] ==
   DEFAULT_LINE_PREFIX_1.  */
#define DEFAULT_LINE_PREFIX_2 'Y'

/* Maximum length for a text line before it is considered binary.  */
#define MAXIMUM_NON_BINARY_LINE 200

#define LOG10_MAX_INT  11

/* System related declarations.  */

#if STDC_HEADERS
# define ISASCII(Char) 1
#else
# ifdef isascii
#  define ISASCII(Char) isascii (Char)
# else
#  if HAVE_ISASCII
#   define ISASCII(Char) isascii (Char)
#  else
#   define ISASCII(Char) ((Char) & 0x7f == (unsigned char) (Char))
#  endif
# endif
#endif

#ifdef isgraph
#define IS_GRAPH(_c) isgraph (_c)
#else
#define IS_GRAPH(_c) (isprint (_c) && !isspace (_c))
#endif

struct tm *localtime ();

#if MSDOS
          /* 1 extra for CR.  */
#  define CRLF_STRLEN(_s)  (strlen (_s) + 1)
#else
#  define CRLF_STRLEN(_s)  (strlen (_s))
#endif

#if !NO_WALKTREE

  /* Declare directory reading routines and structures.  */

#  ifdef __MSDOS__
#   include "msd_dir.h"
#  else
#   include DIRENT_HEADER
#  endif

#  if HAVE_DIRENT_H
#   define NAMLEN(dirent) (strlen((dirent)->d_name))
#  else
#   define NAMLEN(dirent) ((dirent)->d_namlen)
#   ifndef __MSDOS__
#    define dirent direct
#   endif
#  endif

#endif /* !NO_WALKTREE */

/* Option variables.  */

/* Determine whether an integer type is signed, and its bounds.
   This code assumes two's (or one's!) complement with no holes.  */

/* The extra casts work around common compiler bugs,
   e.g. Cray C 5.0.3.0 when t == time_t.  */
#ifndef TYPE_SIGNED
# define TYPE_SIGNED(t) (! ((t) 0 < (t) -1))
#endif
#ifndef TYPE_MINIMUM
# define TYPE_MINIMUM(t) ((t) (TYPE_SIGNED (t) \
			       ? ~ (t) 0 << (sizeof (t) * CHAR_BIT - 1) \
			       : (t) 0))
#endif
#ifndef TYPE_MAXIMUM
# define TYPE_MAXIMUM(t) ((t) (~ (t) 0 - TYPE_MINIMUM (t)))
#endif

static char explain_text_fmt[sizeof(explain_fmt_fmt_z)];
static int  const explain_1_len = sizeof(explain_1_z) - 1;
static int  const explain_2_len = sizeof(explain_2_z) - 1;

typedef enum {
  QUOT_ID_LNAME,
  QUOT_ID_RNAME,
  QUOT_ID_PATH,
  QUOT_ID_GOOD_STATUS,
  QUOT_ID_BAD_STATUS
} quot_id_t;

typedef struct {
  char const *  cmpr_name;
  char const *  cmpr_cmd_fmt;
  char const *  cmpr_title;
  char const *  cmpr_mode;  /* this must match text after ${lock_dir}/ */
  char const *  cmpr_unpack;
  char const *  cmpr_unnote;
  unsigned long cmpr_level;
} compact_state_t;

compact_state_t gzip_compaction = {
  .cmpr_name    = "gzip",
  .cmpr_cmd_fmt = "gzip -c -%u %s",
  .cmpr_title   = "gzipped",
  .cmpr_mode    = "gzi",
  .cmpr_unpack  = "gzip -dc ${lock_dir}/gzi > %s && \\\n",
  .cmpr_unnote  = "gunzipping file %s"
};

compact_state_t xz_compaction = {
  .cmpr_name    = "xz",
  .cmpr_cmd_fmt = "xz -zc -%u %s",
  .cmpr_title   = "xz-compressed",
  .cmpr_mode    = "xzi",
  .cmpr_unpack  = "xz -dc ${lock_dir}/xzi > %s && \\\n",
  .cmpr_unnote  = "xz-decompressing file %s"
};

compact_state_t bzip2_compaction = {
  .cmpr_name    = "bzip2",
  .cmpr_cmd_fmt = "bzip2 -zkc -%u %s",
  .cmpr_title   = "bzipped",
  .cmpr_mode    = "bzi",
  .cmpr_unpack  = "bzip2 -dkc ${lock_dir}/bzi > %s && \\\n",
  .cmpr_unnote  = "bunzipping file %s"
};

#ifdef HAVE_COMPRESS
compact_state_t compress_compaction = {
  .cmpr_name    = "compress",
  .cmpr_cmd_fmt = "compress -b%u < %s",
  .cmpr_title   = "compressed",
  .cmpr_mode    = "cmp",
  .cmpr_unpack  = "compress -d < ${lock_dir}/cmp > %s && \\\n",
  .cmpr_unnote  = "uncompressing file %s"
};
#endif

compact_state_t * const compaction[] = {
  &gzip_compaction,
  &xz_compaction,
#ifdef HAVE_COMPRESS
  &compress_compaction,
#endif
  &bzip2_compaction
};
compact_state_t * cmpr_state = NULL;

static int const compact_ct = sizeof(compaction)/sizeof(compaction[0]);

/* Character to get at the beginning of each line.  */
static int line_prefix = '\0';

/* Value of strlen (here_delimiter).  */
static size_t here_delimiter_length = 0;

/* Switch for debugging on.  */
#if DEBUG
static int debugging_mode = 0;
#endif

/* Other global variables.  */

typedef enum {
  fail = ~1, // disambiguate, in case "true" is -1.
  doue_true = true,
  doue_false = false
} do_uue_t;

do_uue_t uuencode_file = fail;
int opt_idx = 0;

/* File onto which the shar script is being written.  */
static FILE *output = NULL;

/* Position for archive type message.  */
static off_t archive_type_position = 0;

/* Position for first file in the shar file.  */
static off_t first_file_position = 0;

/* Actual output filename.  */
static char *output_filename = NULL;

/* Output file ordinal.  FIXME: also flag for -o.  */
static int part_number = 0;

/* Table saying whether each character is binary or not.  */
static unsigned char byte_is_binary[256];

/* For checking file type and access modes.  */
static struct stat struct_stat;

/* The number used to make the intermediate files unique.  */
static int sharpid = 0;

/* scribble space. */
static size_t scribble_size = 1024 + (BUFSIZ * 2);
static char* scribble = NULL;

static int translate_script = 0;

static int    mkdir_alloc_ct = 0;
static int    mkdir_already_ct = 0;
static char** mkdir_already;

#if DEBUG
# define DEBUG_PRINT(Format, Value) \
    if (debugging_mode)					\
      {							\
	static char const _f[] = Format;		\
	char buf[INT_BUFSIZE_BOUND (off_t)];		\
	printf (_f, offtostr (Value, buf));		\
      }
#else
# define DEBUG_PRINT(Format, Value)
#endif

static void open_output (void);
static void close_output (int next_part_no);

/* Walking tree routines.  */

/* Define a type just for easing ansi2knr's life.  */
typedef int (*walker_t) (const char *, const char *);

static void
init_shar_msg(void)
{
  int ix;
  struct quoting_options * alwaysq, * doubleq;

  if (translate_script)
    {
      for (ix = 0; ix < SHAR_MSG_CT; ix++)
        shar_msg_table[ix] = gettext (shar_msg_table[ix]);
    }

  alwaysq  = clone_quoting_options (NULL);
  set_quoting_style (alwaysq, shell_always_quoting_style);

  doubleq  = clone_quoting_options (NULL);
  set_quoting_style (doubleq, c_quoting_style);
  set_char_quoting (doubleq, '"', 1); // ");

  for (ix = 0; ix < SHAR_MSG_CT; ix++)
    {
      char const * pz = shar_msg_table[ix];

      switch (shar_msg_xform[ix])
        {
        case XFORM_PLAIN: continue;

        case XFORM_APOSTROPHE:
          pz = quotearg_alloc (pz, shar_msg_size[ix], alwaysq);
          break;

        case XFORM_DBL_QUOTE:
          pz = quotearg_alloc (pz, shar_msg_size[ix], doubleq);
          break;
        }

      shar_msg_table[ix] = pz;
    }

  free (alwaysq);
  free (doubleq);
}

static char const *
format_report(quot_id_t type, char const * fmt, char const * what)
{
  int off = strlen(scribble);
  size_t sz;

  if (off > 0)
    off++;
  sz = scribble_size - off;

  if (fmt == NULL)
    return NULL;

  for (;;)
    {
      int len = snprintf (scribble + off, sz, fmt, what);
      if ((unsigned)len < sz)
        break;

      if (len < 0)
        die (SHAR_EXIT_BUG, _("printf formatting error:  %s\n"), fmt);

      scribble_size = (len + off + 4096) & 0x0FFF;
      scribble = xrealloc (scribble, scribble_size);
      sz = scribble_size - off;
    }

  return scribble + off;
}

static void
echo_status (const char*  test,
	     const char*  ok_fmt,
	     const char*  bad_fmt,
	     const char*  what,
	     int die_on_failure )
{
  char const * good_quot;
  char const * bad_quot;
  char const * die_str;

  /*
     NOTE TO DEVELOPERS:  The two format arguments "ok_fmt" and "bad_fmt" are
     expected to be correctly quoted for use by the shell command, "echo".
     That is to say, the status strings contain an unadorned:
         echo %s
     and the input has to work correctly.

     These formatting strings will normally have a "%s" in them somewhere to
     fill in the value from "what".  Those strings are then used in the real
     output formatting with show_all_status_z or show_good_status_z or
     show_bad_status_z.  Not all do, so "what" can sometimes be NULL.
   */
  *scribble = NUL;
  good_quot = format_report (QUOT_ID_GOOD_STATUS, ok_fmt, what);
  bad_quot  = format_report (QUOT_ID_BAD_STATUS, bad_fmt, what);
  die_str   = die_on_failure ? show_status_dies_z : "";

  if (good_quot != NULL)
    {
      if (bad_quot != NULL)
        fprintf (output, show_all_status_z, test, good_quot,
                 bad_quot, die_str);
      else
        fprintf (output, show_good_status_z, test, good_quot);
    }

  else if (bad_quot != NULL)
    fprintf (output, show_bad_status_z, test, bad_quot, die_str);

  else
    die (SHAR_EXIT_BUG, _("sharutils bug - no status"));
}

static void
echo_text (const char* format_pz, const char* arg_pz, int cascade)
{
  static char const continue_z[] = " &&\n";
  unsigned int len = (unsigned)snprintf (
    scribble, scribble_size - sizeof (continue_z), format_pz, arg_pz);

  if (cascade && (len < scribble_size - sizeof (continue_z)))
    {
      char * pz = scribble + len;
      /* replace newline with the continuation string */
      strcpy (pz, continue_z);
    }
  fprintf (output, echo_string_z, scribble);
}

#if !NO_WALKTREE

/*--------------------------------------------------------------------------.
| Recursively call ROUTINE on each entry, down the directory tree.  NAME    |
| is the path to explore.  RESTORE_NAME is the name that will be later	    |
| relative to the unsharing directory.  ROUTINE may also assume		    |
| struct_stat is set, it accepts updated values for NAME and RESTORE_NAME.  |
`--------------------------------------------------------------------------*/

static int
walkdown (
     walker_t routine,
     const char *local_name,
     const char *restore_name)
{
  DIR *directory;		/* directory being scanned */
  int status;			/* status to return */

  char *local_name_copy;	/* writeable copy of local_name */
  size_t local_name_length;	/* number of characters in local_name_copy */
  size_t sizeof_local_name;	/* allocated size of local_name_copy */

  char *restore_name_copy;	/* writeable copy of restore_name */
  int    restore_offset;	/* passdown copy of restore_name */
  size_t restore_name_length;	/* number of characters in restore_name_copy */
  size_t sizeof_restore_name;	/* allocated size of restore_name_copy */

  if (stat (local_name, &struct_stat))
    {
      error (0, errno, local_name);
      return SHAR_EXIT_FILE_NOT_FOUND;
    }

  if (!S_ISDIR (struct_stat.st_mode & S_IFMT))
    return (*routine) (local_name, restore_name);

  if (directory = opendir (local_name), !directory)
    {
      error (0, errno, local_name);
      return SHAR_EXIT_CANNOT_OPENDIR;
    }

  status = 0;

  /* include trailing '/' in length */

  local_name_length = strlen (local_name) + 1;
  sizeof_local_name = local_name_length + 32;
  local_name_copy   = xmalloc (sizeof_local_name);
  memcpy (local_name_copy, local_name, local_name_length-1);
  local_name_copy[ local_name_length-1 ] = '/';
  local_name_copy[ local_name_length   ] = NUL;

  restore_name_length = strlen (restore_name) + 1;
  sizeof_restore_name = restore_name_length + 32;
  restore_name_copy   = xmalloc (sizeof_restore_name);
  memcpy (restore_name_copy, restore_name, restore_name_length-1);
  restore_name_copy[ restore_name_length-1 ] = '/';
  restore_name_copy[ restore_name_length   ] = NUL;

  {
    size_t sz = (sizeof_local_name < sizeof_restore_name)
      ? sizeof_restore_name : sizeof_local_name;
    sz = 1024 + (sz * 2);
    if (scribble_size < sz)
      {
        scribble_size = (sz + 4096) & 0x0FFF;
        scribble = xrealloc (scribble, scribble_size);
      }
  }

  if ((restore_name_copy[0] == '.') && (restore_name_copy[1] == '/'))
    restore_offset = 2;
  else
    restore_offset = 0;

  for (;;)
    {
      struct dirent *entry = readdir (directory);
      const char* pzN;
      int space_need;

      if (entry == NULL)
	break;

      /* append the new file name after the trailing '/' char.
         If we need more space, add in a buffer so we needn't
         allocate over and over.  */

      pzN = entry->d_name;
      if (*pzN == '.')
	{
	  if (pzN[1] == NUL)
	    continue;
	  if ((pzN[1] == '.') && (pzN[2] == NUL))
	    continue;
	}

      space_need = 1 + NAMLEN (entry);
      if (local_name_length + space_need > sizeof_local_name)
	{
	  sizeof_local_name = local_name_length + space_need + 16;
	  local_name_copy = (char *)
	    xrealloc (local_name_copy, sizeof_local_name);
	}
      strcpy (local_name_copy + local_name_length, pzN);

      if (restore_name_length + space_need > sizeof_restore_name)
	{
	  sizeof_restore_name = restore_name_length + space_need + 16;
	  restore_name_copy = (char *)
	    xrealloc (restore_name_copy, sizeof_restore_name);
	}
      strcpy (restore_name_copy + restore_name_length, pzN);

      status = walkdown (routine, local_name_copy,
			 restore_name_copy + restore_offset);
      if (status != 0)
	break;
    }

  /* Clean up.  */

  free (local_name_copy);
  free (restore_name_copy);

#if CLOSEDIR_VOID
  closedir (directory);
#else
  if (closedir (directory))
    {
      error (0, errno, local_name);
      return SHAR_EXIT_CANNOT_OPENDIR;
    }
#endif

  return status;
}

#endif /* !NO_WALKTREE */

/*------------------------------------------------------------------.
| Walk through the directory tree, calling ROUTINE for each entry.  |
| ROUTINE may also assume struct_stat is set.			    |
`------------------------------------------------------------------*/

static int
walktree (walker_t routine, const char *local_name)
{
  const char *restore_name;
  char *local_name_copy;

  /* Remove crumb at end.  */
  {
    int len = strlen (local_name);
    char *cursor;

    local_name_copy = (char *) alloca (len + 1);
    memcpy (local_name_copy, local_name, len + 1);
    cursor = local_name_copy + len - 1;

    while (*cursor == '/' && cursor > local_name_copy)
      *(cursor--) = NUL;
  }

  /* Remove crumb at beginning.  */

  if (HAVE_OPT(BASENAME))
    restore_name = basename (local_name_copy);
  else if (!strncmp (local_name_copy, "./", 2))
    restore_name = local_name_copy + 2;
  else
    restore_name = local_name_copy;

#if NO_WALKTREE

  /* Just act on current entry.  */

  {
    int status = stat (local_name_copy, &struct_stat);

    if (status != 0)
      {
        error (0, errno, local_name_copy);
        status = SHAR_EXIT_FILE_NOT_FOUND;
      }
    else
      status = (*routine) (local_name_copy, restore_name);

    return status;
  }

#else

  /* Walk recursively.  */

  return walkdown (routine, local_name_copy, restore_name);

#endif
}

/* Generating parts of shar file.  */

/*---------------------------------------------------------------------.
| Build a `drwxrwxrwx' string corresponding to MODE into MODE_STRING.  |
`---------------------------------------------------------------------*/

static char *
mode_string (unsigned mode)
{
  static char const modes[] = "-rwxrwxrwx";
  static char result[12];
  int ix  = 1;
  int msk = 0400;

  strcpy (result, "----------");

  do  {
    if (mode & msk)
      result[ix] = modes[ix];
    ix++;
    msk >>= 1;
  } while (msk != 0);

  if (mode & 04000)
    result[3] = 's';

  if (mode & 02000)
    result[6] = 's';

  return result;
}

/*-----------------------------------------------------------------------.
| Generate shell code which, at *unshar* time, will study the properties |
| of the unpacking system and set some variables accordingly.		 |
`-----------------------------------------------------------------------*/

static void
generate_configure (void)
{
  if (! HAVE_OPT(NO_MD5_DIGEST))
    fprintf (output, md5check_z, SM_not_verifying_sums);

  fputs (clobber_check_z, output);

  if (! HAVE_OPT(NO_I18N))
    {
      fputs (i18n_z, output);
      /* Above the name of the program of the package which supports the
	 --print-text-domain-dir option has to be given.  */
    }

  if (! HAVE_OPT(QUIET_UNSHAR))
    {
      if (HAVE_OPT(VANILLA_OPERATION))
	fputs (dev_tty_nocheck_z, output);
      else
	{
	  if (HAVE_OPT(QUERY_USER))
	    /* Check if /dev/tty exists.  If yes, define shar_tty to
	       '/dev/tty', else, leave it empty.  */

	    fputs (dev_tty_check_z, output);

	  /* Try to find a way to echo a message without newline.  Set
	     shar_n to '-n' or nothing for an echo option, and shar_c
	     to '\c' or nothing for a string terminator.  */

	  fputs (echo_checks_z, output);
	}
    }

  if (! HAVE_OPT(NO_TIMESTAMP))
    {
      fprintf (output, timestamp_z, SM_time_not_set);
    }

  if ((! HAVE_OPT(WHOLE_SIZE_LIMIT)) || (part_number == 1))
    {
      echo_status (ck_lockdir_z, NULL, SM_lock_dir_exists, lock_dir_z, 1);

      /* Create locking directory.  */
      if (HAVE_OPT(VANILLA_OPERATION))
	echo_status (make_lock_dir_z, NULL, SM_no_lock_dir, lock_dir_z, 1);
      else
	echo_status (make_lock_dir_z, SM_x_lock_dir_created,
                     SM_x_no_lock_dir, lock_dir_z, 1);
    }
  else
    {
      fprintf (output, seq_check_z,
               SM_unpack_part_1, part_number,
               SM_unpack_next_part);
    }

  if (HAVE_OPT(QUERY_USER))
    {
      fprintf (output, query_answers_z,
	       SM_ans_yes,    SM_yes_means,
	       SM_ans_no,     SM_no_means,
	       SM_ans_all,    SM_all_means,
	       SM_ans_none,   SM_none_means,
	       SM_ans_help,   SM_help_means,
	       SM_ans_quit,   SM_quit_means);
    }
}

/*----------------------------------------------.
| generate_mkdir                                |
| Make sure it is done only once for each dir   |
`----------------------------------------------*/

static void
generate_mkdir (const char *path)
{
  const char *quoted_path;

  /* If already generated code for this dir creation, don't do again.  */

  {
    int    ct = mkdir_already_ct;
    char** pp = mkdir_already;

    while (--ct > 0)
      {
        if (strcmp (*(pp++), path) == 0)
          return;
      }
  }

  /* Haven't done this one.  */

  if (++mkdir_already_ct > mkdir_alloc_ct)
    {
      /*
       *  We need more name space.  Get larger and larger chunks of space.
       *  The bound is when integers go negative.  Too many directories.  :)
       *
       *  16, 40, 76, 130, 211, 332, 514, 787, 1196, 1810, 2731, 4112, ...
       */
      mkdir_alloc_ct += 16 + (mkdir_alloc_ct/2);
      if (mkdir_alloc_ct < 0)
        die (SHAR_EXIT_FAILED,
             _("Too many directories for mkdir generation"));

      if (mkdir_already != NULL)
        mkdir_already =
          xrealloc (mkdir_already, mkdir_alloc_ct * sizeof (char*));
      else
        mkdir_already = xmalloc (mkdir_alloc_ct * sizeof (char*));
    }

  /* Add the directory into our "we've done this already" table */

  mkdir_already[ mkdir_already_ct-1 ] = xstrdup (path);

  /* Generate the text.  */

  quoted_path = quotearg_n_style (
    QUOT_ID_PATH, shell_always_quoting_style, path);
  fprintf (output, dir_check_z, quoted_path);
  if (! HAVE_OPT(QUIET_UNSHAR))
    {
      fprintf (output, dir_create_z, quoted_path);
      echo_status (aok_check_z, SM_x_dir_created, SM_x_no_dir, path, 1);
    }
  else
    fprintf (output, "  mkdir %s || exit 1\n", quoted_path);
  fputs ("fi\n", output);
}

static void
clear_mkdir_already (void)
{
  char** pp = mkdir_already;
  int    ct = mkdir_already_ct;

  mkdir_already_ct = 0;
  while (--ct >= 0)
    {
      free (*pp);
      *(pp++) = NULL;
    }
}

/*---.
| ?  |
`---*/

static void
generate_mkdir_script (const char * path)
{
  char *cursor;

  for (cursor = strchr (path, '/'); cursor; cursor = strchr (cursor + 1, '/'))
    {

      /* Avoid empty string if leading or double '/'.  */

      if (cursor == path || *(cursor - 1) == '/')
	continue;

      /* Omit '.'.  */

      if (cursor[-1] == '.' && (cursor == path + 1 || cursor[-2] == '/'))
	continue;

      /* Temporarily terminate string.  FIXME!  */

      *cursor = 0;
      generate_mkdir (path);
      *cursor = '/';
    }
}

/* Walking routines.  */

/*---.
| ?  |
`---*/

static int
check_accessibility (const char *local_name, const char *restore_name)
{
  if (access (local_name, 4))
    {
      error (0, errno, _("Cannot access %s"), local_name);
      return SHAR_EXIT_FILE_NOT_FOUND;
    }

  return SHAR_EXIT_SUCCESS;
}

/*---.
| ?  |
`---*/

static int
generate_one_header_line (const char *local_name, const char *restore_name)
{
  char buf[INT_BUFSIZE_BOUND (off_t)];
  fprintf (output, "# %6s %s %s\n", offtostr (struct_stat.st_size, buf),
	   mode_string (struct_stat.st_mode), restore_name);
  return 0;
}

static void
print_caution_notes (FILE * fp)
{
  {
    char const * msg;

    if (! HAVE_OPT(NO_CHECK_EXISTING))
      msg = exist_keep_z;
    else if (HAVE_OPT(QUERY_USER))
      msg = exist_ask_z;
    else
      msg = exist_kill_z;

    fprintf (fp, exist_note_z, msg);
  }

  if (HAVE_OPT(WHOLE_SIZE_LIMIT))
    {
      int len = snprintf (explain_text_fmt, sizeof (explain_text_fmt),
                          explain_fmt_fmt_z, explain_1_len, explain_2_len);
      if ((unsigned)len >= sizeof (explain_text_fmt))
        strcpy (explain_text_fmt, "#%-256s\n#%-256s\n");

      /* May be split, provide for white space for an explanation.  */

      fputs ("#\n", output);
      archive_type_position = ftell (output);
      fprintf (fp, explain_text_fmt, "", "");
    }
}

static void
print_header_stamp (FILE * fp)
{
  {
    char const * pz = HAVE_OPT(ARCHIVE_NAME) ? OPT_ARG(ARCHIVE_NAME) : "";
    char const * ch = HAVE_OPT(ARCHIVE_NAME) ? ", a shell" : "a shell";

    fprintf (fp, file_leader_z, pz, ch, PACKAGE, VERSION, sharpid);
  }

  {
    static char ftime_fmt[] = "%Y-%m-%d %H:%M %Z";

    /*
     * All fields are two characters, except %Y is four and
     * %Z may be up to 30 (?!?!).  Anyway, if that still fails,
     * we'll drop back to "%z".  We'll give up if that fails.
     */
    char buffer[sizeof (ftime_fmt) + 64];
    time_t now;
    struct tm * local_time;
    time (&now);
    local_time = localtime (&now);
    {
      size_t l =
        strftime (buffer, sizeof (buffer) - 1, ftime_fmt, local_time);
      if (l == 0)
        {
          ftime_fmt[sizeof(ftime_fmt) - 2] = 'z';
          l = strftime (buffer, sizeof (buffer) - 1, ftime_fmt, local_time);
        }
      if (l > 0)
        fprintf (fp, made_on_comment_z, buffer, OPT_ARG(SUBMITTER));
    }
  }

  {
    char * c_dir = xgetcwd ();
    if (c_dir != NULL)
      {
        fprintf (fp, source_dir_comment_z, c_dir);
        free (c_dir);
      }
    else
      error (0, errno, _("Cannot get current directory name"));
  }
}

/*---.
| ?  |
`---*/

static void
generate_full_header (int argc, char * const * argv)
{
  int counter;

  for (counter = 0; counter < argc; counter++)
    {
      struct stat sb;
      /* If we cannot stat it, it is either a valid option or we have
         already errored out.  */
      if (stat (argv[counter], &sb) != 0)
        continue;

      if (walktree (check_accessibility, argv[counter]))
	exit (SHAR_EXIT_FAILED);
    }

  if (HAVE_OPT(NET_HEADERS))
    {
      static char const by[] =
        "Submitted-by: %s\nArchive-name: %s%s%02d\n\n";
      bool has_slash = (strchr (OPT_ARG(ARCHIVE_NAME), '/') != NULL);
      int  part = (part_number > 0) ? part_number : 1;

      fprintf (output, by, OPT_ARG(SUBMITTER), OPT_ARG(ARCHIVE_NAME),
               has_slash ? "" : "/part", part);
    }

  if (HAVE_OPT(CUT_MARK))
    fputs (cut_mark_line_z, output);

  print_header_stamp (output);
  print_caution_notes (output);
  fputs (contents_z, output);

  for (counter = 0; counter < argc; counter++)
    {
      struct stat sb;
      /* If we cannot stat it, it is either a valid option or we have
         already errored out.  */
      if (stat (argv[counter], &sb) != 0)
        continue;

      (void) walktree (generate_one_header_line, argv[counter]);
    }
  fputs ("#\n", output);

  generate_configure ();
}

void
change_files (const char * restore_name, off_t * remaining_size)
{
  /* Change to another file.  */

  DEBUG_PRINT ("New file, remaining %s, ", *remaining_size);
  DEBUG_PRINT ("Limit still %s\n", OPT_VALUE_WHOLE_SIZE_LIMIT);

  /* Close the "&&" and report an error if any of the above
     failed.  */

  fputs (" :\n", output);
  echo_status ("test $? -ne 0", SM_restore_failed, NULL, restore_name, 0);

  snprintf (scribble, scribble_size,
            SM_end_of_part, part_number, part_number+1);
  fprintf (output, echo_string_z, scribble);

  close_output (part_number + 1);

  /* Clear mkdir_already in case the user unshars out of order.  */

  clear_mkdir_already ();

  /* Form the next filename.  */

  open_output ();
  if (! HAVE_OPT(QUIET))
    fprintf (stderr, _("Starting file %s\n"), output_filename);

  if (HAVE_OPT(NET_HEADERS))
    {
      fprintf (output, "Submitted-by: %s\n", OPT_ARG(SUBMITTER));
      fprintf (output, "Archive-name: %s%s%02d\n\n", OPT_ARG(ARCHIVE_NAME),
	       strchr (OPT_ARG(ARCHIVE_NAME), '/') ? "" : "/part",
	       part_number ? part_number : 1);
    }

  if (HAVE_OPT(CUT_MARK))
    fputs (cut_mark_line_z, output);

  {
    static const char part_z[] = "part %02d of %s ";
    char const * nm = HAVE_OPT(ARCHIVE_NAME) ? OPT_ARG(ARCHIVE_NAME) :
      "a multipart";
    off_t len = sizeof(part_z) + strlen(nm) + 16;
    if (scribble_size < len)
      {
        scribble_size = (scribble_size + len + 0x0FFF) & ~0x0FFF;
        scribble = xrealloc (scribble, scribble_size);
      }

    sprintf (scribble, part_z, part_number, nm);
    fprintf (output, file_leader_z, scribble, "", PACKAGE, VERSION, sharpid);
  }

  generate_configure ();

  first_file_position = ftell (output);
}

static void
read_byte_size (char * wc, size_t wc_sz, FILE * pfp)
{
  char * pz = wc;

  /* Read to the first digit or EOF */
  for (;;)
    {
      int ch = getc (pfp);
      if (ch == EOF)
        goto bogus_number; /* no digits were found */

      if (isdigit (ch))
        {
          *(pz++) = ch;
          break;
        }
    }

  for (;;)
    {
      int ch = getc (pfp);
      if (! isdigit (ch))
        break;
      *(pz++) = ch;
      if (pz >= wc + wc_sz)
        goto bogus_number; /* number is waaay too large */
    }

  *pz = NUL;
  return;

 bogus_number:
  wc[0] = '0'; /* assume zero length */
  wc[1] = NUL;
}

/* Emit shell script text to validate the restored file size
   Validate the transferred file using simple 'wc' command. */
static void
emit_char_ct_validation (
     char const * local_name,
     char const * quoted_name,
     char const * restore_name,
     int did_md5)
{
  /* Shell command able to count characters from its standard input.
     We have to take care for the locale setting because wc in multi-byte
     character environments gets different results.  */

  char wc[1 + LOG10_MAX_INT * 2]; // enough for 64 bit size
  char * command;

#ifndef __MINGW32__
  static char const cct_cmd[] = "LC_ALL=C wc -c < %s";
#else
  static char const cct_cmd[] = "set LC_ALL=C & wc -c \"%s\"";
  quoted_name = local_name;
#endif

  command = alloca (sizeof(cct_cmd) + strlen (quoted_name));
  sprintf (command, cct_cmd, quoted_name);

  {
    FILE * pfp = popen (command, "r");
    if (pfp == NULL)
      die (SHAR_EXIT_FAILED, _("Could not popen command"), command);

    /*  Read from stdin white space followed by digits.  That ought to be
        followed by a newline or a NUL.  */
    read_byte_size (wc, sizeof(wc), pfp);
    pclose (pfp);
  }

  if (did_md5)
    fputs (otherwise_z, output);

  snprintf (scribble, scribble_size, SM_bad_size, restore_name, wc);
  fprintf (output, ck_chct_z, restore_name, wc, scribble);
}

/**
 * Determine if file needs encoding.  A file needs encoding if any byte
 * falls outside the range of 0x20 through 0x7E, plus tabs and newlines.
 * Also encode files that have lines that start with "From " because
 * mail handlers will often insert spurious ">" characters when found.
 * This function also pays attention of the --text-files and --uuencode
 * options, forcing the result to be 0 and 1 respectively.
 *
 * @param[in] fname  input file name
 * @returns 0 if the file is a simple text file -- no encoding needed.
 * @returns 1 when the file must be encoded.
 */
static do_uue_t
file_needs_encoding (char const * fname)
{
#ifdef __CHAR_UNSIGNED__
#  define BYTE_IS_BINARY(_ch)  (byte_is_binary[_ch])
#else
#  define BYTE_IS_BINARY(_ch)  (byte_is_binary[(_ch) & 0xFF])
#endif

  FILE * infp;
  int    line_length;

  if (cmpr_state != NULL)
    return true; // compression always implies encoding

  switch (WHICH_OPT_MIXED_UUENCODE) {
  case VALUE_OPT_TEXT_FILES: return false;
  case VALUE_OPT_UUENCODE:   return true;
  default: break;
  }

  /* Read the input file, seeking for one non-ASCII character.  Considering the
     average file size, even reading the whole file (if it is text) would
     usually be faster than invoking 'file'.  */

  infp = fopen (fname, freadonly_mode);

  if (infp == NULL)
    {
      error (0, errno, _("Cannot open file %s"), fname);
      return fail;
    }

  /* Assume initially that the input file is text.  Then try to prove
     it is binary by looking for binary characters or long lines.  */

  line_length = 0;

  for (;;)
    {
      int ch = getc (infp);

    retest_char:
      switch (ch) {
      case EOF:  goto loop_done;
      case '\n': line_length = 0; break;
      case 'F':
      case 'f':
        if (line_length > 0)
          {
            line_length++;
            break;
          }

        {
          /*
           * Mail handlers like to mutilate lines beginning with "from ".
           * Therefore, if a line starts with "From " or "from ", deem
           * the file to need encoding.
           */
          static char const from[] = "rom ";
          char const * p = from;
          for (;;)
            {
              line_length++;
              ch = getc (infp);
              if (ch != *p)
                goto retest_char;
              if (*++p == NUL)
                {
                  line_length = MAXIMUM_NON_BINARY_LINE;
                  goto loop_done;
                }
            }
          /* NOTREACHED */
        }

      default:
        if (BYTE_IS_BINARY(ch))
          {
            line_length = MAXIMUM_NON_BINARY_LINE;
            goto loop_done;
          }

        line_length++;
      } /* switch (ch) */

      if (line_length >= MAXIMUM_NON_BINARY_LINE)
        break;
    } loop_done:;

  fclose (infp);

  /* Text files should terminate with an end of line.  */

  return (line_length != 0) ? true : false;
#undef BYTE_IS_BINARY
}

#ifdef HAVE_WORKING_FORK

static void
encode_file_to_pipe (
    int out_fd,
    const char *  local_name,
    const char *  q_local_name,
    const char *  restore_name)
{
  /* Start writing the pipe with encodes.  */

  FILE * in_fp;
  FILE * out_fp;
  char * cmdline  = alloca (strlen (q_local_name) + 64);
  char const * open_txt = cmdline;
  char const * open_fmt = "popen";

  if (cmpr_state != NULL)
    {
      sprintf (cmdline, cmpr_state->cmpr_cmd_fmt,
               cmpr_state->cmpr_level, q_local_name);
      in_fp = popen (cmdline, freadonly_mode);
    }
  else
    {
      in_fp = fopen (local_name, freadonly_mode);
      open_fmt = "fopen";
      open_txt = local_name;
    }

  if (in_fp == NULL)
    fserr (SHAR_EXIT_FAILED, open_fmt, open_txt);

  out_fp = fdopen (out_fd, fwriteonly_mode);

  fprintf (out_fp, mode_fmt_z, restore_name);

  copy_file_encoded (in_fp, out_fp);
  fprintf (out_fp, "end\n");
  if (cmpr_state != NULL)
    pclose (in_fp);
  else
    fclose (in_fp);

  exit (EXIT_SUCCESS);
}

static FILE *
open_encoded_file (char const * local_name, char const * q_local_name,
               const char *  restore_name)
{
  int pipex[2];

  /* Fork a uuencode process.  */

  if (pipe (pipex) < 0)
    fserr (SHAR_EXIT_FAILED, _("call"), "pipe(2)");
  fflush (output);

  switch (fork ())
    {
    case 0:
      close (pipex[0]);
      encode_file_to_pipe (pipex[1], local_name, q_local_name, restore_name);
      /* NOTREACHED */

    case -1:
      fserr (SHAR_EXIT_FAILED, _("call"), "fork");
      return NULL;

    default:
      /* Parent, create a file to read.  */
      break;
    }
  close (pipex[1]);

  {
    FILE * fp = fdopen (pipex[0], freadonly_mode);
    if (fp == NULL)
      fserr (SHAR_EXIT_FAILED, "fdopen", _("pipe fd"));
    return fp;
  }
}

#else /* ! HAVE_WORKING_FORK */

#ifdef __MINGW32__
static char *
win_cmd_quote (char const * fname)
{
  static size_t blen = 0;
  static char   buf  = NULL;
  size_t        nlen = strlen (fname);
  if (nlen + 3 > blen)
    {
      blen = nlen + 3;
      buf = buf ? malloc (blen) : realloc (blen, buf);
      if (fp == NULL)
        fserr (SHAR_EXIT_FAILED, "malloc", fname);
    }
  *buf = '"';
  memcpy (buf+1, fname, nlen);
  buf[nlen + 1] = '"';
  buf[nlen + 2] = NUL;
}
#endif

static FILE *
open_encoded_file (char const * local_name,
               char const * q_local_name,
               char const * restore_name)
{
  static char const rm_temp[]  = "\nrm -f %s";
  /* Start writing the pipe with encodes.  */

  char * cmdline, char * p;
  static char uu_cmd_fmt[] = "uuencode %s %s";
  size_t sz = sizeof (uu_cmd_fmt) + strlen (q_local_name)
    + strlen (restore_name);
  /* A command to use for encoding an uncompressed text file.  */
#ifdef __MINGW32__
  q_local_name = win_cmd_quote (local_name);
#endif

  if (cmpr_state == NULL)
    p = cmdline = alloca (sz);
  else
    {
      /* Before uuencoding the file, we compress it.  The compressed output
         goes into a temporary file.  */
      char * shartemp = tmpnam(NULL);
      sz += strlen (cmpr_state->cmpr_cmd_fmt) + LOG10_MAX_INT
        + 4 + sizeof (rm_temp) + 3 * strlen (shartemp);

      p = cmdline = alloca (sz);
      sprintf (p, cmpr_state->cmpr_cmd_fmt, cmpr_state->cmpr_level,
               q_local_name);
      p += strlen (p);
      *(p++) = '>';
      strcpy(p, shartemp);
      p += sizeof (shartemp) - 1;
      *(p++) = '\n';
      q_local_name = shartemp;
    }

  /* Insert the uuencode command.  It may be reading from the original file,
     or from a temporary file containing the compressed original.
     If we compressed it, then we'll need to remove the temp, too.   */
  sprintf (p, uu_cmd_fmt, q_local_name, restore_name);
  if (cmpr_state != NULL)
    sprintf (p + strlen (p), rm_temp, q_local_name);

  /* Don't use freadonly_mode because it might be "rb", while we need
     text-mode read here, because we will be reading pure text from
     uuencode, and we want to drop any CR characters from the CRLF
     line endings, when we write the result into the shar.  */
  {
    FILE * in_fp = popen (cmdline, "r");

    if (in_fp == NULL)
      fserr (SHAR_EXIT_FAILED, "popen", cmdline);

    return in_fp;
  }
}

#endif /* HAVE_WORKING_FORK */

static FILE *
open_shar_input (
     const char *  local_name,
     const char *  q_local_name,
     const char *  restore_name,
     const char *  q_restore_name,
     const char ** file_type_p,
     const char ** file_type_remote_p)
{
  FILE * infp;

  uuencode_file = file_needs_encoding (local_name);
  if (uuencode_file == fail)
    return NULL;

  /* If mixed, determine the file type.  */

  if (! uuencode_file)
    {
      *file_type_p = _("text");
      *file_type_remote_p = SM_type_text;

      infp = fopen (local_name, freadonly_mode);
      if (infp == NULL)
        fserr (SHAR_EXIT_FAILED, "fopen", local_name);
    }
  else
    {
      if (cmpr_state != NULL)
        *file_type_p = *file_type_remote_p = cmpr_state->cmpr_title;
      else
        {
          *file_type_p        = _("text");
          *file_type_remote_p = _("(text)");
        }

      infp = open_encoded_file (local_name, q_local_name, restore_name);
    }

  return infp;
}

/**
 * Change to another file.
 */
static void
split_shar_ed_file (char const * restore, off_t * size_left, int * split_flag)
{
  DEBUG_PRINT ("New file, remaining %s, ", (*size_left));
  DEBUG_PRINT ("Limit still %s\n", OPT_VALUE_WHOLE_SIZE_LIMIT);

  fprintf (output, "%s\n", OPT_ARG(HERE_DELIMITER));

  /* Close the "&&" and report an error if any of the above
     failed.  */

  fputs (" :\n", output);
  echo_status ("test $? -ne 0", SM_restore_failed, NULL, restore, 0);

  if (! HAVE_OPT(NO_CHECK_EXISTING))
    fputs ("fi\n", output);

  if (HAVE_OPT(QUIET_UNSHAR))
    {
      snprintf (scribble, scribble_size, SM_end_of_part,
               part_number, part_number + 1);
      fprintf (output, echo_string_z, scribble);
    }
  else
    {
      snprintf (scribble, scribble_size, SM_s_end_of_part,
               HAVE_OPT(ARCHIVE_NAME) ? OPT_ARG(ARCHIVE_NAME) : SM_word_archive,
               part_number);
      fprintf (output, echo_string_z, scribble);

      snprintf (scribble, scribble_size, SM_contin_in_part,
               restore, (long)part_number + 1);
      fprintf (output, echo_string_z, scribble);
    }

  fwrite (split_file_z, sizeof (split_file_z) - 1, 1, output);

  if (part_number == 1)
    {
      /* Rewrite the info lines on the first header.  */

      fseek (output, archive_type_position, SEEK_SET);
      fprintf (output, explain_text_fmt, explain_1_z, explain_2_z);
      fseek (output, 0, SEEK_END);
    }
  close_output (part_number + 1);

  /* Next! */

  open_output ();

  if (HAVE_OPT(NET_HEADERS))
    {
      fprintf (output, "Submitted-by: %s\n", OPT_ARG(SUBMITTER));
      fprintf (output, "Archive-name: %s%s%02d\n\n",
               OPT_ARG(ARCHIVE_NAME),
               strchr (OPT_ARG(ARCHIVE_NAME), '/') ? "" : "/part",
               part_number ? part_number : 1);
    }

  if (HAVE_OPT(CUT_MARK))
    fputs (cut_mark_line_z, output);

  fprintf (output, continue_archive_z,
           basename (output_filename), part_number,
           HAVE_OPT(ARCHIVE_NAME)
           ? OPT_ARG(ARCHIVE_NAME) : "a multipart archive",
           restore, sharpid);

  generate_configure ();

  if (! HAVE_OPT(NO_CHECK_EXISTING))
    {
      if (HAVE_OPT(QUIET_UNSHAR))
        fputs (split_continue_quietly_z, output);
      else
        {
          fputs (split_continue_z, output);
          fprintf (output, SM_still_skipping, restore);
          fputs (otherwise_z, output);
        }
    }

  if (! HAVE_OPT(QUIET))
    fprintf (stderr, _("Continuing file %s\n"), output_filename);
  if (! HAVE_OPT(QUIET_UNSHAR))
    echo_text (SM_continuing, restore, 0);

  fprintf (output, split_resume_z,
           line_prefix, OPT_ARG(HERE_DELIMITER),
           uuencode_file ? "${lock_dir}/uue" : restore);

  (*size_left) = OPT_VALUE_WHOLE_SIZE_LIMIT - ftell (output);
  *split_flag  = 1;
}

static void
process_shar_input (FILE * input, off_t * size_left, int * split_flag,
                    char const * restore, char const * q_restore)
{
  if (uuencode_file && (cmpr_state != NULL))
    {
      char * p = fgets (scribble, BUFSIZ, input);
      char * e;
      if ((p == NULL) || (strncmp (p, mode_fmt_z, 6) != 0))
        return;
      /*
       * Find the start of the last token
       */
      e = p + strlen(p);
      while (  isspace (e[-1]) && (e > p))  e--;
      while (! isspace (e[-1]) && (e > p))  e--;
      fwrite (p, e - p, 1, output);
      fprintf (output, "_sh%05d/%s\n", (int)sharpid, cmpr_state->cmpr_mode);
    }

  while (fgets (scribble, scribble_size, input))
    {
      /* Output a line and test the length.  */

      if (!HAVE_OPT(FORCE_PREFIX)
          && ISASCII (scribble[0])
          && IS_GRAPH (scribble[0])

          /* Protect lines already starting with the prefix.  */
          && scribble[0] != line_prefix

          /* Old mail programs interpret ~ directives.  */
          && scribble[0] != '~'

          /* Avoid mailing lines which are just '.'.  */
          && scribble[0] != '.'

#if STRNCMP_IS_FAST
          && strncmp (scribble, OPT_ARG(HERE_DELIMITER), here_delimiter_length)

          /* unshar -e: avoid 'exit 0'.  */
          && strncmp (scribble, "exit 0", 6)

          /* Don't let mail prepend a '>'.  */
          && strncmp (scribble, "From", 4)
#else
          && (scribble[0] != OPT_ARG(HERE_DELIMITER)[0]
              || strncmp (scribble, OPT_ARG(HERE_DELIMITER),
                          here_delimiter_length))

          /* unshar -e: avoid 'exit 0'.  */
          && (scribble[0] != 'e' || strncmp (scribble, "exit 0", 6))

          /* Don't let mail prepend a '>'.  */
          && (scribble[0] != 'F' || strncmp (scribble, "From", 4))
#endif
          )
        fputs (scribble, output);
      else
        {
          fprintf (output, "%c%s", line_prefix, scribble);
          (*size_left)--;
        }

      /* Try completing an incomplete line, but not if the incomplete
         line contains no character.  This might occur with -T for
         incomplete files, or sometimes when switching to a new file.  */

      if (*scribble && scribble[strlen (scribble) - 1] != '\n')
        {
          putc ('\n', output);
          (*size_left)--;
        }

      (*size_left) -= CRLF_STRLEN (scribble);
      if (WHICH_OPT_WHOLE_SIZE_LIMIT != VALUE_OPT_SPLIT_SIZE_LIMIT)
        continue;

      if ((int)(*size_left) >= 0)
        continue;
      split_shar_ed_file (restore, size_left, split_flag);
    }
}

/* Prepare a shar script.  */

static int
start_sharing_file (char const ** lnameq_p, char const ** rnameq_p,
                    FILE ** fpp, off_t * size_left_p)
{
  char const * lname = *lnameq_p;
  char const * rname = *rnameq_p;
  char const * file_type;         /* text or binary */
  char const * file_type_remote;  /* text or binary, avoiding locale */

  /* Check to see that this is still a regular file and readable.  */

  if (!S_ISREG (struct_stat.st_mode & S_IFMT))
    {
      error (0, 0, _("%s: Not a regular file"), lname);
      return 0;
    }
  if (access (lname, R_OK))
    {
      error (0, 0, _("Cannot access %s"), lname);
      return 0;
    }

  *lnameq_p =
    quotearg_n_style (QUOT_ID_LNAME, shell_always_quoting_style, lname);
  *rnameq_p =
    quotearg_n_style (QUOT_ID_RNAME, shell_always_quoting_style, rname);

  /*
   * If file size is limited, either splitting files or not,
   * get the current output length.  Switch files if we split on file
   * boundaries and there may not be enough space.
   */
  if (HAVE_OPT(WHOLE_SIZE_LIMIT))
    {
      off_t current_size = ftell (output);
      off_t encoded_size = 1024 + (uuencode_file
               ? (struct_stat.st_size + struct_stat.st_size / 3)
               : struct_stat.st_size);

      *size_left_p = OPT_VALUE_WHOLE_SIZE_LIMIT - current_size;
      DEBUG_PRINT ("In shar: remaining size %s\n", *size_left_p);

      if (  (WHICH_OPT_WHOLE_SIZE_LIMIT != VALUE_OPT_SPLIT_SIZE_LIMIT)
         && (current_size > first_file_position)
         && (encoded_size > *size_left_p))
        {
          change_files (*rnameq_p, size_left_p);
          current_size = ftell (output);
          *size_left_p = OPT_VALUE_WHOLE_SIZE_LIMIT - current_size;
        }
    }

  else
    *size_left_p = ~0;		/* give some value to the variable */

  fprintf (output, break_line_z, rname);

  generate_mkdir_script (rname);

  if (struct_stat.st_size == 0)
    {
      file_type = _("empty");
      file_type_remote = SM_is_empty;
      *fpp = NULL;		/* give some value to the variable */
    }
  else
    {
      *fpp = open_shar_input (lname, *lnameq_p, rname, *rnameq_p,
                               &file_type, &file_type_remote);
      if (*fpp == NULL)
        return 0;
    }

  /* Protect existing files.  */

  if (! HAVE_OPT(NO_CHECK_EXISTING))
    {
      fprintf (output, pre_exist_z, *rnameq_p);

      if (HAVE_OPT(QUERY_USER))
	{
          char * tmp_str;

          sprintf (scribble, SM_overwriting, rname);

          tmp_str = scribble + strlen(scribble) + 2;
          sprintf (tmp_str, SM_overwrite, rname);
          fprintf (output, query_user_z, scribble, tmp_str);

          sprintf (scribble, SM_skipping, rname);
          fprintf (output, query_check_z, SM_extract_aborted,
		   scribble, scribble);
	}
      else
        echo_text (SM_skip_exist, rname, 0);

      fputs (otherwise_z, output);
    }

  if (! HAVE_OPT(QUIET))
    error (0, 0, _("Saving %s (%s)"), lname, file_type);

  if (! HAVE_OPT(QUIET_UNSHAR))
    {
      sprintf (scribble, SM_x_extracting, rname, file_type_remote);
      fprintf (output, echo_string_z, scribble);
    }

  return 1;
}

static void
finish_sharing_file (const char * lname, const char * lname_q,
                     const char * rname, const char * rname_q)
{
  if (! HAVE_OPT(NO_TIMESTAMP))
    {
      struct tm * restore_time;
      /* Set the dates as they were.  */

      restore_time = localtime (&struct_stat.st_mtime);
      fprintf (output, shar_touch_z,
	       (restore_time->tm_year + 1900) / 100,
	       (restore_time->tm_year + 1900) % 100,
	       restore_time->tm_mon + 1, restore_time->tm_mday,
	       restore_time->tm_hour, restore_time->tm_min,
	       restore_time->tm_sec, rname_q);
    }

  if (HAVE_OPT(VANILLA_OPERATION))
    {
      /* Close the "&&" and report an error if any of the above
	 failed.  */
      fputs (":\n", output);
      echo_status ("test $? -ne 0", SM_restore_failed, NULL, rname, 0);
    }

  else
    {
      unsigned char md5buffer[16];
      FILE *fp = NULL;
      int did_md5 = 0;

      /* Set the permissions as they were.  */

      fprintf (output, SM_restore_mode,
	       (unsigned) (struct_stat.st_mode & 0777), rname_q);

      /* Report an error if any of the above failed.  */

      echo_status ("test $? -ne 0", SM_restore_failed, NULL, rname, 0);

      if (   ! HAVE_OPT(NO_MD5_DIGEST)
          && (fp = fopen (lname, freadonly_mode)) != NULL
	  && md5_stream (fp, md5buffer) == 0)
	{
	  /* Validate the transferred file using 'md5sum' command.  */
	  size_t cnt;
	  did_md5 = 1;

	  fprintf (output, md5test_z, rname_q,
		   SM_md5_check_failed, OPT_ARG(HERE_DELIMITER));

	  for (cnt = 0; cnt < 16; ++cnt)
	    fprintf (output, "%02x", md5buffer[cnt]);

	  fprintf (output, " %c%s\n%s\n",
		   ' ', rname, OPT_ARG(HERE_DELIMITER));
	  /* This  ^^^ space is not necessarily a parameter now.  But it
	     is a flag for binary/text mode and will perhaps be used later.  */
	}

      if (fp != NULL)
	fclose (fp);

      if (! HAVE_OPT(NO_CHARACTER_COUNT))
        emit_char_ct_validation (lname, lname_q, rname_q, did_md5);

      if (did_md5)
	fputs ("  fi\n", output);
    }

  /* If the exists option is in place close the if.  */

  if (! HAVE_OPT(NO_CHECK_EXISTING))
    fputs ("fi\n", output);
}

/*---.
| ?  |
`---*/

static int
shar (const char * lname, const char * rname)
{
  FILE * input;
  off_t  size_left;
  int    split_flag = 0;          /* file split flag */
  char const * lname_q = lname;
  char const * rname_q = rname;

  if (! start_sharing_file (&lname_q, &rname_q, &input, &size_left))
    return SHAR_EXIT_FAILED;

  if (struct_stat.st_size == 0)
    {
      /* Just touch the file, or empty it if it exists.  */

      fprintf (output, "  > %s &&\n", rname_q);
    }

  else
    {
      /* Run sed for non-empty files.  */

      if (uuencode_file)
	{

	  /* Run sed through uudecode (via temp file if might get split).  */

	  fprintf (output, "  sed 's/^%c//' << '%s' ",
		   line_prefix, OPT_ARG(HERE_DELIMITER));
	  if (HAVE_OPT(NO_PIPING))
	    fprintf (output, "> ${lock_dir}/uue &&\n");
	  else
	    fputs ("| uudecode &&\n", output);
	}

      else
	{
	  /* Just run it into the file.  */

	  fprintf (output, "  sed 's/^%c//' << '%s' > %s &&\n",
		   line_prefix, OPT_ARG(HERE_DELIMITER), rname_q);
	}

      process_shar_input (input, &size_left, &split_flag, rname, rname_q);

      fclose (input);
      while (wait (NULL) >= 0)
	;

      fprintf (output, "%s\n", OPT_ARG(HERE_DELIMITER));
      if (split_flag && ! HAVE_OPT(QUIET_UNSHAR))
        echo_text (SM_file_complete, rname, 1);

      /* If this file was uuencoded w/Split, decode it and drop the temp.  */

      if (uuencode_file && HAVE_OPT(NO_PIPING))
	{
	  if (! HAVE_OPT(QUIET_UNSHAR))
            echo_text (SM_uudec_file, rname, 1);

          fwrite (shar_decode_z, sizeof (shar_decode_z) - 1, 1, output);
	}

      /* If this file was compressed, uncompress it and drop the temp.  */
      if (cmpr_state != NULL)
        {
	  if (! HAVE_OPT(QUIET_UNSHAR))
            echo_text (cmpr_state->cmpr_unnote, rname, 1);
          fprintf (output, cmpr_state->cmpr_unpack, rname_q);
        }
    }

  finish_sharing_file (lname, lname_q, rname, rname_q);

  return EXIT_SUCCESS;
}

/**
 * Look over the base name for our output files.  If it has a formatting
 * element in it, ensure that it is an integer format ("diouxX") and that
 * there is only one formatting element.  Allow that element to consume
 * 128 bytes and allow all the remaining characters to consume 1 byte of
 * output.  Allocate a buffer that large and set the global
 * output_filename to point to that buffer.
 *
 * Finally, if no formatting element was found in this base name string,
 * then append ".%02d" to the base name and point the --output-prefix
 * option argument to this new string.
 */
static void
parse_output_base_name (char const * arg)
{
  char * bad_fmt = _("Invalid format for output file names (%s): %s");
  int c;
  int hadarg = 0;
  char const *p;
  int base_name_len = 128;

  for (p = arg ; (c = *p++) != 0; )
    {
      base_name_len++;
      if (c != '%')
	continue;
      c = *p++;
      if (c == '%')
	continue;
      if (hadarg)
        usage_message(bad_fmt, _("more than one format element"), arg);

      while (c != 0 && strchr("#0+- 'I", c) != 0)
	c = *p++;
      if (c == 0)
	usage_message(bad_fmt, _("no conversion character"), arg);

      if (c >= '0' && c <= '9')
	{
	  long v;
          char const * skp;
	  errno = 0;
	  v = strtol((void *)(p-1), (void *)&skp, 10);
	  if ((v == 0) || (v > 16) || (errno != 0))
            usage_message(bad_fmt, _("format is too wide"), arg);
	  p = skp;
	  c = *p++;
	  base_name_len += v;
	}
      if (c == '.')
	{
	  c = *p++;
	  while (c != 0 && c >= '0' && c <= '9')
	    c = *p++;
	}
      if (c == 0 || strchr("diouxX", c) == 0)
	usage_message(bad_fmt, _("invalid conversion character"), arg);
      hadarg = 1;
    }
  output_filename = xmalloc(base_name_len);
  if (! hadarg)
    {
      static char const sfx[] = ".%02d";
      size_t len = strlen (arg);
      char * fmt = xmalloc(len + sizeof (sfx));
      bool   svd = initialization_done;
      memcpy (fmt, arg, len);
      memcpy (fmt + len, sfx, sizeof (sfx));

      /* this is allowed to happen after initialization. */
      initialization_done = false;
      SET_OPT_OUTPUT_PREFIX(fmt);
      initialization_done = svd;
    }
}

/**
 * Open the next output file, or die trying.  The output-prefix argument
 * must have been specified and has been parsed to ensure it has exactly
 * one printf integer argument in it.  (We append it, if necessary.)
 * "output_filename" is a buffer allocated by parse_output_base_name()
 * that is guaranteed large enough to format 100,000 output files.
 */
static void
open_output (void)
{
  if (! HAVE_OPT(OUTPUT_PREFIX))
    {
      output = stdout;
#ifdef __MINGW32__
      _setmode (fileno (stdout) , _O_BINARY);
#endif
      return;
    }

  if (output_filename == NULL)
    parse_output_base_name (OPT_ARG(OUTPUT_PREFIX));
  sprintf (output_filename, OPT_ARG(OUTPUT_PREFIX), ++part_number);
  output = fopen (output_filename, fwriteonly_mode);

  if (output == NULL)
    fserr (SHAR_EXIT_FAILED, _("Opening"), output_filename);
}

/**
 * Close the current output file, or die trying.
 */
static void
close_output (int part)
{
  if (part > 0)
    fprintf (output, "echo %d > ${lock_dir}/seq\n", part);

  fputs ("exit 0\n", output);

  if (fclose (output) != 0)
    fserr (SHAR_EXIT_FAILED, _("Closing"), output_filename);
}

/**
 * Trim an input line.  Remove trailing white space and return the first
 * non-whitespace character.  Blank lines or lines starting with the hash
 * character ("#") cause NULL to be returned and are ignored.
 *
 * @param[in,out] pz   string to trim
 * @returns the first non-blank character in "pz".
 */
static char *
trim (char * pz)
{
  char * res;
  while (isspace (*pz))  pz++;
  switch (*pz)
    {
    case NUL:
    case '#':
      return NULL;
    }
  res = pz;
  pz += strlen (pz);
  while (isspace (pz[-1]))  pz--;
  *pz = NUL;
  return res;
}

/**
 * Guess at a submitter email address.  Get the current login name and
 * current host name and hope that that is reasonable.
 *
 * @returns an allocated string containing "login@thishostname".
 */
static void
set_submitter (void)
{
  char * buffer;
  char * uname = getuser (getuid ());
  size_t len   = strlen (uname);
  if (uname == NULL)
    fserr (SHAR_EXIT_FAILED, "getpwuid", "getuid()");
  buffer = xmalloc (len + 2 + HOST_NAME_MAX);
  memcpy (buffer, uname, len);
  buffer[len++] = '@';
  gethostname (buffer + len, HOST_NAME_MAX);
  SET_OPT_SUBMITTER(buffer);
}

/**
 * post-option processing configuring.  Verifies global state,
 * sets up reference tables and reads up the input list, as required.
 *
 * @param[in,out] argcp  pointer to argument count
 * @param[in,out] argvp  pointer to arg vector pointer
 */
static void
configure_shar (int * argc_p, char *** argv_p)
{
  line_prefix = (OPT_ARG(HERE_DELIMITER)[0] == DEFAULT_LINE_PREFIX_1
		 ? DEFAULT_LINE_PREFIX_2
		 : DEFAULT_LINE_PREFIX_1);

  here_delimiter_length = strlen (OPT_ARG(HERE_DELIMITER));
  gzip_compaction.cmpr_level =
    xz_compaction.cmpr_level =
    bzip2_compaction.cmpr_level = DESC(LEVEL_OF_COMPRESSION).optArg.argInt;
#ifdef HAVE_COMPRESS
  compress_compaction.cmpr_level = DESC(BITS_PER_CODE).optArg.argInt;
#endif

  scribble = xmalloc (scribble_size);

  /* Set defaults for unset options.  */
  if (! HAVE_OPT(SUBMITTER))
    set_submitter ();

  open_output ();
  if (isatty (fileno (output)) && isatty (STDERR_FILENO))
    {
      /*
       * Output is going to a TTY device, and so is stderr.
       * Redirect stderr to /dev/null in that case so that
       * the results are not cluttered with chatter.
       */
      FILE * fp = freopen ("/dev/null", fwriteonly_mode, stderr);
      if (fp != stderr)
        error (SHAR_EXIT_FAILED, errno,
               _("reopening stderr to /dev/null"));
    }

  memset ((char *) byte_is_binary, 1, sizeof (byte_is_binary));
  /* \n ends an input line, and \v and \r disrupt the output  */
  byte_is_binary['\b'] = 0; /* BS back space   */
  byte_is_binary['\t'] = 0; /* HT horiz. tab   */
  byte_is_binary['\f'] = 0; /* FF form feed    */
  /* bytes 0x20 through and including 0x7E --> 0x7E - 0x20 + 1 */
  memset ((char *) byte_is_binary + 0x20, 0, 0x7F - 0x20);

  /* Maybe read file list from standard input.  */

  if (HAVE_OPT(INPUT_FILE_LIST))
    {
      char ** list;
      int max_argc = 32;

      *argc_p = 0;

      list = (char **) xmalloc (max_argc * sizeof (char *));
      *scribble = NUL;

      for (;;)
        {
          char * pz = fgets (scribble, scribble_size, stdin);
          if (pz == NULL)
            break;

          pz = trim (pz);
          if (pz == NULL)
            continue;

	  if (*argc_p == max_argc)
            {
              max_argc += max_argc / 2;
              list = (char **) xrealloc (list, max_argc * sizeof (*list));
            }

	  list[(*argc_p)++] = xstrdup (pz);
	}
      *argv_p = list;
      opt_idx = 0;
    }

  /* Diagnose various usage errors.  */

  if (opt_idx >= *argc_p)
    usage_message (_("No input files"));

  if (HAVE_OPT(WHOLE_SIZE_LIMIT))
    {
      if (OPT_VALUE_WHOLE_SIZE_LIMIT < 4096)
        OPT_VALUE_WHOLE_SIZE_LIMIT *= 1024;
      if (WHICH_OPT_WHOLE_SIZE_LIMIT == VALUE_OPT_SPLIT_SIZE_LIMIT)
        SET_OPT_NO_PIPING;
    }

  /* Start making the archive file.  */

  generate_full_header (*argc_p - opt_idx, &(*argv_p)[opt_idx]);

  if (HAVE_OPT(QUERY_USER))
    {
      if (HAVE_OPT(NET_HEADERS))
	error (0, 0, _("PLEASE avoid -X shars on Usenet or public networks"));

      fputs ("shar_wish=\n", output);
    }

  first_file_position = ftell (output);
}

/**
 * Ensure intermixing is okay.  Called for options that may be intermixed.
 * Verify that either we are still initializing or that intermixing has been
 * enabled.
 *
 * @param opts unused
 * @param od   unused
 */
void
check_intermixing (tOptions * opts, tOptDesc * od)
{
  (void)opts;
  (void)od;

  if (initialization_done && ! HAVE_OPT(INTERMIX_TYPE))
    usage_message (
      _("The '%s' option may not be intermixed with file names\n\
unless the --intermix-type option has been specified."), od->pz_Name);
}

/**
 * Ensure we are initializing.  Called for options that may not be
 * intermixed with input names.
 *
 * @param opts unused
 * @param od   unused
 */
void
validate_opt_context (tOptions * opts, tOptDesc * od)
{
  (void)opts;
  (void)od;

  if (initialization_done)
    usage_message(_("The '%s' option must appear before any file names"),
                  od->pz_Name);
}

/**
 * Some form of compaction has been requested.
 * Check for either "none" or one of the known compression types.
 * Sets the global variable \a cmpr_state to either NULL or
 * the correct element from the array of compactors.
 *
 * @param[in] opts  program option descriptor
 * @param[in] od    compaction option descriptor
 */
void
set_compaction (tOptions * opts, tOptDesc * od)
{
  char const * c_type = od->optArg.argString;
  int  ix = 0;

  (void)opts;
  (void)od;

  check_intermixing (opts, od);

  if (strcmp (c_type, "none") == 0)
    {
      cmpr_state = NULL;
      return;
    }

  for (;;) {
    if (strcmp (c_type, compaction[ix]->cmpr_name) == 0)
      break;
    if (++ix >= compact_ct)
      {
        fprintf (stderr,
                 _("invalid compaction type:  %s\nthe known types are:\n"),
                 c_type);
        for (ix = 0; ix < compact_ct; ix++)
          fprintf (stderr, "\t%s\n", compaction[ix]->cmpr_name);
        USAGE (SHAR_EXIT_OPTION_ERROR);
      }
  }
  cmpr_state = compaction[ix];
}

/**
 * Do all configuring to be done.  After this runs, only compaction and
 * encoding options may be intermixes with input names.  And then only if
 * the --intermix-type option has been specified.
 *
 * @param[in,out] argcp  pointer to argument count
 * @param[in,out] argvp  pointer to arg vector pointer
 */
static void
initialize(int * argcp, char *** argvp)
{
  sharpid = (int) getpid ();
  setlocale (LC_ALL, "");

  /* Set the text message domain.  */
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  opt_idx = optionProcess (&sharOptions, *argcp, *argvp);
  if (opt_idx == *argcp)
    {
      if (! HAVE_OPT(INPUT_FILE_LIST))
        SET_OPT_INPUT_FILE_LIST("-");
    }
  else
    {
      if (HAVE_OPT(INPUT_FILE_LIST) && (opt_idx != *argcp))
        usage_message(
          _("files on command line and --input-file-list specified"));
    }

  init_shar_msg ();
  configure_shar (argcp, argvp);
  initialization_done = true;
}

/**
 * Process a file hierarchy into a shell archive.
 */

int
main (int argc, char ** argv)
{
  shar_exit_code_t status = SHAR_EXIT_SUCCESS;
  initialize (&argc, &argv);

  /* Process positional parameters and files.  */

  while (opt_idx < argc)
    {
      char * arg = argv[opt_idx++];
      struct stat sb;
      if (stat (arg, &sb) != 0)
        {
          if (HAVE_OPT(INTERMIX_TYPE) && (*arg == '-'))
            {
              while (*++arg == '-')  ;
              optionLoadLine (&sharOptions, arg);
            }
          else
            error (0, errno, arg);
          continue;
        }

      {
        shar_exit_code_t s = walktree (shar, arg);
        if (status == SHAR_EXIT_SUCCESS)
          status = s;
      }
    }

  /* Delete the sequence file, if any.  */

  if (HAVE_OPT(WHOLE_SIZE_LIMIT) && part_number > 1)
    {
      fprintf (output, echo_string_z, SM_you_are_done);
      if (HAVE_OPT(QUIET))
	fprintf (stderr, _("Created %d files\n"), part_number);
    }

  echo_status ("rm -fr ${lock_dir}", SM_x_rem_lock_dir, SM_x_no_rem_lock_dir,
               "${lock_dir}", 1);

  close_output (0);
  exit (status);
}
/*
 * Local Variables:
 * mode: C
 * c-file-style: "gnu"
 * indent-tabs-mode: nil
 * End:
 * end of shar.c */
