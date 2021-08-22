/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://www.hdfgroup.org/licenses.               *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* Purpose: This file is used to map HDF macros to Windows functions.  This
 *          should get included H5private mappings, so as to override them.
 *          Any macro not mapped here, however, will receive a similar mapping
 *          inside H5private.h
 *
 */

/* _MSC_VER = 192x  VS2019
 * _MSC_VER = 191x  VS2017
 * _MSC_VER = 1900  VS2015
 * _MSC_VER = 1800  VS2013
 * _MSC_VER = 1700  VS2012
 */
#ifdef H5_HAVE_WIN32_API

typedef struct _stati64 h5_stat_t;
typedef __int64         h5_stat_size_t;

#define HDaccess(F, M) _access(F, M)
#define HDchdir(S)     _chdir(S)
#define HDclose(F)     _close(F)
#define HDcreat(S, M)  Wopen_utf8(S, O_CREAT | O_TRUNC | O_RDWR, M)
#define HDdup(F)       _dup(F)
#define HDfdopen(N, S) _fdopen(N, S)
#define HDfileno(F)    _fileno(F)
#define HDfstat(F, B)  _fstati64(F, B)
#define HDisatty(F)    _isatty(F)

#define HDgetcwd(S, Z)     _getcwd(S, Z)
#define HDgetdcwd(D, S, Z) _getdcwd(D, S, Z)
#define HDgetdrive()       _getdrive()
#define HDlseek(F, O, W)   _lseeki64(F, O, W)
#define HDlstat(S, B)      _lstati64(S, B)
#define HDmkdir(S, M)      _mkdir(S)
#define HDnanosleep(N, O)  Wnanosleep(N, O)
#define HDoff_t            __int64

/* Note that the variadic HDopen macro is using a VC++ extension
 * where the comma is dropped if nothing is passed to the ellipsis.
 */
#ifndef H5_HAVE_MINGW
#define HDopen(S, F, ...) Wopen_utf8(S, F, __VA_ARGS__)
#else
#define HDopen(S, F, ...) Wopen_utf8(S, F, ##__VA_ARGS__)
#endif
#define HDread(F, M, Z)       _read(F, M, Z)
#define HDremove(S)           Wremove_utf8(S)
#define HDrmdir(S)            _rmdir(S)
#define HDsetvbuf(F, S, M, Z) setvbuf(F, S, M, (Z > 1 ? Z : 2))
#define HDsleep(S)            Sleep(S * 1000)
#define HDstat(S, B)          _stati64(S, B)
#define HDstrcasecmp(A, B)    _stricmp(A, B)
#define HDstrdup(S)           _strdup(S)
#define HDstrtok_r(X, Y, Z)   strtok_s(X, Y, Z)
#define HDtzset()             _tzset()
#define HDunlink(S)           _unlink(S)
#define HDwrite(F, M, Z)      _write(F, M, Z)

#ifdef H5_HAVE_VISUAL_STUDIO

/*
 * The (void*) cast just avoids a compiler warning in MSVC
 */
#define HDmemset(X, C, Z) memset((void *)(X), C, Z)

struct timezone {
    int tz_minuteswest;
    int tz_dsttime;
};

#endif /* H5_HAVE_VISUAL_STUDIO */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
H5_DLL int    Wgettimeofday(struct timeval *tv, struct timezone *tz);
H5_DLL int    Wsetenv(const char *name, const char *value, int overwrite);
H5_DLL int    Wflock(int fd, int operation);
H5_DLL char * Wgetlogin(void);
H5_DLL herr_t H5_expand_windows_env_vars(char **env_var);
H5_DLL wchar_t *H5_get_utf16_str(const char *s);
H5_DLL int      Wopen_utf8(const char *path, int oflag, ...);
H5_DLL int      Wremove_utf8(const char *path);
H5_DLL int      H5_get_win32_times(H5_timevals_t *tvs);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#define HDgettimeofday(V, Z) Wgettimeofday(V, Z)
#define HDsetenv(N, V, O)    Wsetenv(N, V, O)
#define HDflock(F, L)        Wflock(F, L)
#define HDgetlogin()         Wgetlogin()

/* Non-POSIX functions */

#ifndef H5_HAVE_MINGW
#define HDftruncate(F, L) _chsize_s(F, L)
#define HDfseek(F, O, W)  _fseeki64(F, O, W)
#endif /* H5_HAVE_MINGW */

#endif /* H5_HAVE_WIN32_API */
