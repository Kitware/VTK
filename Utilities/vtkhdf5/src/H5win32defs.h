/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the files COPYING and Copyright.html.  COPYING can be found at the root   *
 * of the source code distribution tree; Copyright.html can be found at the  *
 * root level of an installed copy of the electronic HDF5 document set and   *
 * is linked from the top-level documents page.  It can also be found at     *
 * http://hdfgroup.org/HDF5/doc/Copyright.html.  If you do not have          *
 * access to either file, you may request a copy from help@hdfgroup.org.     *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* Programmer:  Scott Wegner
 *              June 3, 2008
 *
 * Purpose: This file is used to map HDF macros to Windows functions.  This
 *          should get included H5private mappings, so as to override them.
 *          Any macro not mapped here, however, will receive a similar mapping
 *          inside H5private.h
 *
 */

#ifdef _WIN32

typedef struct _stati64     h5_stat_t;
typedef __int64             h5_stat_size_t;

#define HDaccess(F,M)       _access(F,M)
#define HDclose(F)          _close(F)
#define HDdup(F)            _dup(F)
#define HDfdopen(N,S)       _fdopen(N,S)
#define HDfileno(F)         _fileno(F)
#if _MSC_VER > 1310 /* Newer than VS.NET 2003 */
#define HDftruncate(F,L)    _chsize_s(F,L)
#else
#define HDftruncate(F,L)    chsize(F,L)
#endif
#define HDfstat(F,B)        _fstati64(F,B)
#define HDisatty(F)         _isatty(F)
#define HDlstat(S,B)        _lstati64(S,B)
#define HDstat(S,B)         _stati64(S,B)
#define HDgetcwd(S,Z)       _getcwd(S,Z)
#define HDgetdcwd(D,S,Z)    _getdcwd(D,S,Z)
#ifndef H5_HAVE_GETTIMEOFDAY
    #ifdef __cplusplus
        extern "C" {
    #endif /* __cplusplus */
    H5_DLL int HDgettimeofday(struct timeval *tv, void *tz);
    #ifdef __cplusplus
        }
    #endif /* __cplusplus */
    #define HDgettimeofday(V,Z) HDgettimeofday(V,Z)
#endif /* H5_HAVE_GETTIMEOFDAY */
#define HDgetdrive()        _getdrive()
#define HDlseek(F,O,W)      _lseeki64(F,O,W)
#define HDmemset(X,C,Z)     memset((void*)(X),C,Z)
#define HDmkdir(S,M)        _mkdir(S)
#define HDopen(S,F,M)       _open(S,F|_O_BINARY,M)
#define HDread(F,M,Z)       _read(F,M,Z)
#define HDsetvbuf(F,S,M,Z)  setvbuf(F,S,M,(Z>1?Z:2))
#define HDsleep(S)          Sleep(S*1000)
#define HDstrcasecmp(A,B)   _stricmp(A,B)
#define HDstrtoull(S,R,N)   _strtoui64(S,R,N)
#define HDstrdup(S)         _strdup(S)
#define HDsnprintf          _snprintf /*varargs*/
#define HDtzset()           _tzset()
#define HDunlink(S)         _unlink(S)
#define HDvsnprintf(S,N,FMT,A) _vsnprintf(S,N,FMT,A)
#define HDwrite(F,M,Z)      _write(F,M,Z)

/* Non-POSIX functions */

/* Don't use actual pthread_self on Windows because the return
 * type cannot be cast as a ulong like other systems. */
#define HDpthread_self_ulong() ((unsigned long)GetCurrentThreadId())


#endif /* _WIN32 */
