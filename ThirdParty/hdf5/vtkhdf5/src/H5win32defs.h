/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the LICENSE file, which can be found at the root of the source code       *
 * distribution tree, or in https://www.hdfgroup.org/licenses.               *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* Purpose: This file deals with Windows compatibility. MSVC is largely C99
 *          compliant now, but we still need work-arounds for some POSIX
 *          things and MinGW.
 *
 *          This file must be included before the H5private.h HD mappings, so
 *          the definitions here will supersede them.
 */

#ifdef H5_HAVE_WIN32_API

/* Win32 platform-independent definition for struct stat. For POSIX, see
 * H5private.h.
 */
typedef struct _stati64 h5_stat_t;

#ifdef H5_HAVE_VISUAL_STUDIO
struct timezone {
    int tz_minuteswest;
    int tz_dsttime;
};
#endif

#define HDcreat(S, M)        Wopen(S, O_CREAT | O_TRUNC | O_RDWR, M)
#define HDflock(F, L)        Wflock(F, L)
#define HDfstat(F, B)        _fstati64(F, B)
#define HDftell(F)           _ftelli64(F)
#define HDgetdcwd(D, S, Z)   _getdcwd(D, S, Z)
#define HDgetdrive()         _getdrive()
#define HDgettimeofday(V, Z) Wgettimeofday(V, Z)
#define HDlseek(F, O, W)     _lseeki64(F, O, W)
#define HDlstat(S, B)        _lstati64(S, B)
#define HDmkdir(S, M)        _mkdir(S)

/* Windows has a variant of qsort_r with a different signature */
#define HDqsort_r(B, N, S, C, A) HDqsort_context(B, N, S, C, A)

/* We only support the standards conformant preprocessor */
#define HDopen(S, F, ...) Wopen(S, F, ##__VA_ARGS__)

#define HDremove(S)           Wremove(S)
#define HDsetenv(N, V, O)     Wsetenv(N, V, O)
#define HDsetvbuf(F, S, M, Z) setvbuf(F, S, M, (Z > 1 ? Z : 2))
#define HDsleep(S)            Sleep(S * 1000)
#define HDstat(S, B)          _stati64(S, B)
#define HDstrcasecmp(A, B)    _stricmp(A, B)
#define HDstrcasestr(A, B)    Wstrcasestr_wrap(A, B)
#define HDstrndup(S, N)       H5_strndup(S, N)
#define HDstrtok_r(X, Y, Z)   strtok_s(X, Y, Z)
#define HDunsetenv(N)         Wsetenv(N, "", 1)

#ifndef H5_HAVE_MINGW
#define HDftruncate(F, L) _chsize_s(F, L)
#define HDfseek(F, O, W)  _fseeki64(F, O, W)
#endif

#if defined(H5_HAVE_COMPLEX_NUMBERS) && !defined(H5_HAVE_C99_COMPLEX_NUMBERS)
/*
 * MSVC uses its own types for complex numbers that are separate from the
 * C99 standard types, so we must use a typedef. These types are structure
 * types, so we also need some wrapper functions for interacting with them,
 * as the arithmetic operators can't be used on them. These types also may
 * not be used for casts (other than pointer casts) anywhere in the library
 * that will be compiled by MSVC, as casts can't be made between structure
 * types and other types and MSVC will fail to compile.
 */
typedef _Fcomplex H5_float_complex;
typedef _Dcomplex H5_double_complex;
typedef _Lcomplex H5_ldouble_complex;
#define H5_CMPLXF _FCbuild
#define H5_CMPLX  _Cbuild
#define H5_CMPLXL _LCbuild
#endif

#ifdef __cplusplus
extern "C" {
#endif
H5_DLL int    Wgettimeofday(struct timeval *tv, struct timezone *tz);
H5_DLL int    Wsetenv(const char *name, const char *value, int overwrite);
H5_DLL int    Wflock(int fd, int operation);
H5_DLL herr_t H5_expand_windows_env_vars(char **env_var);
H5_DLL herr_t H5_get_utf16_str(const char *s, wchar_t **wstring, uint32_t *win_error);
H5_DLL int    Wopen(const char *path, int oflag, ...);
H5_DLL int    Wremove(const char *path);
H5_DLL int    H5_get_win32_times(H5_timevals_t *tvs);
H5_DLL char  *H5_strndup(const char *s, size_t n);
H5_DLL char  *Wstrcasestr_wrap(const char *haystack, const char *needle);
#ifdef __cplusplus
}
#endif

#endif /* H5_HAVE_WIN32_API */
