/* system-dependent definitions for CVS.
   Copyright (C) 1989-1992 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* @(#)system.h 1.14 92/04/10 */

#ifndef alloca
#ifdef __GNUC__
#define alloca __builtin_alloca
#else
#ifdef sparc
#include <alloca.h>
#else
#ifndef _AIX
/* AIX alloca decl has to be the first thing in the file, bletch! */
char *alloca ();
#endif
#endif
#endif
#endif

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#ifndef S_ISREG			/* Doesn't have POSIX.1 stat stuff. */
#ifndef mode_t
#define mode_t unsigned short
#endif
#endif
#if !defined(S_ISBLK) && defined(S_IFBLK)
#define	S_ISBLK(m) (((m) & S_IFMT) == S_IFBLK)
#endif
#if !defined(S_ISCHR) && defined(S_IFCHR)
#define	S_ISCHR(m) (((m) & S_IFMT) == S_IFCHR)
#endif
#if !defined(S_ISDIR) && defined(S_IFDIR)
#define	S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif
#if !defined(S_ISREG) && defined(S_IFREG)
#define	S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#endif
#if !defined(S_ISFIFO) && defined(S_IFIFO)
#define	S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
#endif
#if !defined(S_ISLNK) && defined(S_IFLNK)
#define	S_ISLNK(m) (((m) & S_IFMT) == S_IFLNK)
#endif
#if !defined(S_ISSOCK) && defined(S_IFSOCK)
#define	S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)
#endif
#if !defined(S_ISMPB) && defined(S_IFMPB) /* V7 */
#define S_ISMPB(m) (((m) & S_IFMT) == S_IFMPB)
#define S_ISMPC(m) (((m) & S_IFMT) == S_IFMPC)
#endif
#if !defined(S_ISNWK) && defined(S_IFNWK) /* HP/UX */
#define S_ISNWK(m) (((m) & S_IFMT) == S_IFNWK)
#endif
#if defined(MKFIFO_MISSING)
#define mkfifo(path, mode) (mknod ((path), (mode) | S_IFIFO, 0))
#endif

#ifdef POSIX
#include <unistd.h>
#include <limits.h>
#ifndef PATH_MAX
#define PATH_MAX pathconf ("/", _PC_PATH_MAX)
#endif
#else
off_t lseek ();
#endif

#ifdef TM_IN_SYS_TIME
#include <sys/time.h>
#else
#include <time.h>
#endif

#ifndef HAVE_SYS_TIMEB_H
struct timeb {
    time_t		time;		/* Seconds since the epoch	*/
    unsigned short	millitm;	/* Field not used		*/
#ifdef timezone
    short		tzone;
#else
    short		timezone;
#endif
    short		dstflag;	/* Field not used		*/
};
#else
#include <sys/timeb.h>
#endif

#if defined(FTIME_MISSING) && !defined(HAVE_TIMEZONE)
#if !defined(timezone)
extern long timezone;
#endif
#endif

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifndef _POSIX_PATH_MAX
#define _POSIX_PATH_MAX 255
#endif

#ifdef __linux__
#include <linux/limits.h>
#endif

#ifndef PATH_MAX
#ifdef MAXPATHLEN
#define PATH_MAX MAXPATHLEN
#else
#define PATH_MAX _POSIX_PATH_MAX
#endif
#endif

#if defined(POSIX) && !defined(__msdos__)
#include <utime.h>
#else
#ifndef ALTOS
struct utimbuf
{
  long actime;
  long modtime;
};
#endif
int utime ();
#endif

#if STDC_HEADERS || HAVE_STRING_H
#include <string.h>
/* An ANSI string.h and pre-ANSI memory.h might conflict.  */
#if !STDC_HEADERS && HAVE_MEMORY_H
#include <memory.h>
#endif /* not STDC_HEADERS and HAVE_MEMORY_H */
#ifndef index
#define index strchr
#endif
#ifndef rindex
#define rindex strrchr
#endif
#ifndef bcopy
#define bcopy(from, to, len) memcpy ((to), (from), (len))
#endif
#ifndef bzero
#define bzero(s, n) (void) memset ((s), 0, (n))
#endif
#ifndef bcmp
#define	bcmp(s1, s2, n) memcmp((s1), (s2), (n))
#endif
#else /* not STDC_HEADERS and not HAVE_STRING_H */
#include <strings.h>
/* memory.h and strings.h conflict on some systems.  */
#endif /* not STDC_HEADERS and not HAVE_STRING_H */

#include <errno.h>
#if defined(USG) || defined(STDC_HEADERS)
#include <stdlib.h>
#else
char *getenv ();
#ifndef malloc
char *malloc ();
char *realloc ();
char *calloc ();
#endif
extern int errno;
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#ifndef O_XOS				/* conflicts with macro */
char *getcwd ();
#endif
#else
#ifdef HAVE_SYS_FILE_H			/* added (JW) */
#include <sys/file.h>
char *getwd ();
#endif
#endif

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif
#ifndef F_OK
#define F_OK 0
#define X_OK 1
#define W_OK 2
#define R_OK 4
#endif

#ifdef __WATCOMC__
#include <direct.h>
#else /*__WATCOMC__*/
#if HAVE_DIRENT_H
# include <dirent.h>
# define NAMLEN(dirent) strlen((dirent)->d_name)
#else
# define dirent direct
# define NAMLEN(dirent) (dirent)->d_namlen
# if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif
# if HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif
# if HAVE_NDIR_H
#  include <ndir.h>
# endif
#endif
#endif /*__WATCOMC__*/

/* Convert B 512-byte blocks to kilobytes if K is nonzero,
   otherwise return it unchanged. */
#define convert_blocks(b, k) ((k) ? ((b) + 1) / 2 : (b))

#ifndef S_ISLNK
#define lstat stat
#endif

#ifndef SIGTYPE
#define SIGTYPE void
#endif
