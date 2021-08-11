/*
 *	Copyright 2018, University Corporation for Atmospheric Research
 *      See netcdf/COPYRIGHT file for copying and redistribution conditions.
 */
#ifndef _NCPATHMGR_H_
#define _NCPATHMGR_H_

#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include "ncexternl.h"

#ifndef WINPATH
#ifdef _WIN32
#define WINPATH 1
#endif
#ifdef __MINGW32__
#define WINPATH 1
#endif
#endif

/* Define wrapper constants for use with NCaccess */
/* Define wrapper constants for use with NCaccess */
#ifdef _WIN32
#define ACCESS_MODE_EXISTS 0
#define ACCESS_MODE_R 4
#define ACCESS_MODE_W 2
#define ACCESS_MODE_RW 6
#ifndef O_RDONLY
#define O_RDONLY _O_RDONLY
#define O_RDWR _O_RDWR
#define O_APPEND _O_APPEND
#define O_BINARY _O_BINARY
#define O_CREAT _O_CREAT
#define O_EXCL _O_EXCL
#endif
#else
#define ACCESS_MODE_EXISTS (F_OK)
#define ACCESS_MODE_R (R_OK)
#define ACCESS_MODE_W (W_OK)
#define ACCESS_MODE_RW (R_OK|W_OK)
#endif

#ifdef _WIN32
#ifndef S_IFDIR
#define S_IFDIR _S_IFDIR
#define S_IFREG _S_IFREG
#endif
#ifndef S_ISDIR
#define S_ISDIR(mode) ((mode) & _S_IFDIR)
#define S_ISREG(mode) ((mode) & _S_IFREG)
#endif
#endif /*_WIN32*/

/*
WARNING: you should never need to explictly call this function;
rather it is invoked as part of the wrappers for e.g. NCfopen, etc.

This function attempts to take an arbitrary path and convert
it to a canonical form.
Assumptions about Input path:
1. It is a relative or absolute path
2. It is not a URL
3. It conforms to the format expected by one of the following:
       Linux (/x/y/...), Cygwin (/cygdrive/D/...),
       Windows (D:/...), or MSYS (/D/...), or relative (x/y...)
4. It is encoded in the local platform character set.
   Note that for most systems, this is utf-8. But for Windows,
   the encoding is most likely some form of ANSI code page, probably
   the windows 1252 encoding.
   Note that in any case, the path must be representable in the
   local Code Page.

On output it produces a re-written path that has the following
properties:
1. The path is normalized to match the platform on which the code
   is running (e.g. cygwin, windows, msys, linux). So for example
   using a cygwin path under visual studio will convert e.g.
   /cygdrive/d/x/y to d:\x\y. See ../unit_test/test_pathcvt.c
   for example conversions.
It returns the converted path.

Note that this function is intended to be Idempotent: f(f(x) == f(x).
This means it is ok to call it repeatedly with no harm.
*/
EXTERNL char* NCpathcvt(const char* path);

EXTERNL int NChasdriveletter(const char* path);

/* Canonicalize and make absolute by prefixing the current working directory */
EXTERNL char* NCpathabsolute(const char* name);

/* Convert from the local coding (e.g. ANSI) to utf-8;
   note that this can produce unexpected results for Windows
   because it first converts to wide character and then to utf8. */
EXTERNL int NCpath2utf8(const char* path, char** u8p);

/* Wrap various stdio and unistd IO functions.
It is especially important to use for windows so that
NCpathcvt (above) is invoked on the path.
*/
#if defined(WINPATH)
/* path converter wrappers*/
EXTERNL FILE* NCfopen(const char* path, const char* flags);
EXTERNL int NCopen3(const char* path, int flags, int mode);
EXTERNL int NCopen2(const char* path, int flags);
EXTERNL int NCaccess(const char* path, int mode);
EXTERNL int NCremove(const char* path);
EXTERNL int NCmkdir(const char* path, int mode);
EXTERNL int NCrmdir(const char* path);
EXTERNL char* NCcwd(char* cwdbuf, size_t len);
EXTERNL char* NCcwd(char* cwdbuf, size_t len);
#ifdef HAVE_SYS_STAT_H
EXTERNL int NCstat(char* path, struct stat* buf);
#endif
#ifdef HAVE_DIRENT_H
EXTERNL DIR* NCopendir(const char* path);
EXTERNL int NCclosedir(DIR* ent);
#endif
#else /*!WINPATH*/
#define NCfopen(path,flags) fopen((path),(flags))
#define NCopen3(path,flags,mode) open((path),(flags),(mode))
#define NCopen2(path,flags) open((path),(flags))
#define NCremove(path) remove(path)
#define NCaccess(path,mode) access(path,mode)
#define NCmkdir(path,mode) mkdir(path,mode)
#define NCgetcwd(buf,len) getcwd(buf,len)
#ifdef HAVE_SYS_STAT_H
#define NCstat(path,buf) stat(path,buf)
#endif
#define NCcwd(buf, len) getcwd(buf,len)
#define NCrmdir(path) rmdir(path)
#ifdef HAVE_DIRENT_H
#define NCopendir(path) opendir(path)
#define NCclosedir(ent) closedir(ent)
#endif
#endif /*!WINPATH*/

/* Platform independent */
#define NCclose(fd) close(fd)
#define NCfstat(fd,buf) fstat(fd,buf)

/**************************************************/
/* Following definitions are for testing only */

/* Possible Kinds Of Output */
#define NCPD_UNKNOWN 0
#define NCPD_NIX 1
#define NCPD_MSYS 2
#define NCPD_CYGWIN 3
#define NCPD_WIN 4
#define NCPD_REL 5 /* actual kind is unknown */

EXTERNL char* NCpathcvt_test(const char* path, int ukind, int udrive);

EXTERNL void printutf8hex(const char* s, char* sx);

/**************************************************/
/* From dutil.c */
EXTERNL char* NC_backslashEscape(const char* s);
EXTERNL char* NC_backslashUnescape(const char* esc);

#endif /* _NCPATHMGR_H_ */
