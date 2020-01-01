/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://support.hdfgroup.org/ftp/HDF5/releases.  *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* Programmer:  Robb Matzke <matzke@llnl.gov>
 *    Friday, October 30, 1998
 *
 * Purpose:  This file is included by all HDF5 library source files to
 *    define common things which are not defined in the HDF5 API.
 *    The configuration constants like H5_HAVE_UNISTD_H etc. are
 *    defined in H5config.h which is included by H5public.h.
 *
 */

#ifndef _H5private_H
#define _H5private_H

#include "H5public.h"    /* Include Public Definitions    */

/* include the pthread header */
#ifdef H5_HAVE_THREADSAFE
 #ifdef H5_HAVE_WIN32_API
  #ifndef H5_HAVE_WIN_THREADS
   #ifdef H5_HAVE_PTHREAD_H
    #include <pthread.h>
   #endif /* H5_HAVE_PTHREAD_H */
  #endif /* H5_HAVE_WIN_THREADS */
 #else /* H5_HAVE_WIN32_API */
  #ifdef H5_HAVE_PTHREAD_H
   #include <pthread.h>
  #endif /* H5_HAVE_PTHREAD_H */
 #endif /* H5_HAVE_WIN32_API */
#endif /* H5_HAVE_THREADSAFE */

/*
 * Include ANSI-C header files.
 */
#ifdef H5_STDC_HEADERS
#   include <assert.h>
#   include <ctype.h>
#   include <errno.h>
#   include <fcntl.h>
#   include <float.h>
#   include <limits.h>
#   include <math.h>
#   include <signal.h>
#   include <stdio.h>
#   include <stdlib.h>
#   include <string.h>
#endif

/*
 * If _POSIX_VERSION is defined in unistd.h then this system is Posix.1
 * compliant. Otherwise all bets are off.
 */
#ifdef H5_HAVE_UNISTD_H
#   include <sys/types.h>
#   include <unistd.h>
#endif
#ifdef _POSIX_VERSION
#   include <sys/wait.h>
#   include <pwd.h>
#endif

/*
 * C9x integer types
 */
#ifndef __cplusplus
#ifdef H5_HAVE_STDINT_H
#   include <stdint.h>
#endif
#endif

/*
 * The `struct stat' data type for stat() and fstat(). This is a Posix file
 * but often apears on non-Posix systems also.  The `struct stat' is required
 * for hdf5 to compile, although only a few fields are actually used.
 */
#ifdef H5_HAVE_SYS_STAT_H
#   include <sys/stat.h>
#endif

/*
 * If a program may include both `time.h' and `sys/time.h' then
 * TIME_WITH_SYS_TIME is defined (see AC_HEADER_TIME in configure.ac).
 * On some older systems, `sys/time.h' includes `time.h' but `time.h' is not
 * protected against multiple inclusion, so programs should not explicitly
 * include both files. This macro is useful in programs that use, for example,
 * `struct timeval' or `struct timezone' as well as `struct tm'.  It is best
 * used in conjunction with `HAVE_SYS_TIME_H', whose existence is checked
 * by `AC_CHECK_HEADERS(sys/time.h)' in configure.ac.
 */
#if defined(H5_TIME_WITH_SYS_TIME)
#   include <sys/time.h>
#   include <time.h>
#elif defined(H5_HAVE_SYS_TIME_H)
#   include <sys/time.h>
#else
#   include <time.h>
#endif

/*
 * Longjumps are used to detect alignment constrants
 */
#ifdef H5_HAVE_SETJMP_H
#   include <setjmp.h>
#endif

/*
 * flock() in sys/file.h is used for the implementation of file locking.
 */
#if defined(H5_HAVE_FLOCK) && defined(H5_HAVE_SYS_FILE_H)
#   include <sys/file.h>
#endif

/*
 * Resource usage is not Posix.1 but HDF5 uses it anyway for some performance
 * and debugging code if available.
 */
#ifdef H5_HAVE_SYS_RESOURCE_H
#   include <sys/resource.h>
#endif

/*
 * Unix ioctls.   These are used by h5ls (and perhaps others) to determine a
 * reasonable output width.
 */
#ifdef H5_HAVE_SYS_IOCTL_H
#   include <sys/ioctl.h>
#endif

/*
 * System information. These are needed on the DEC Alpha to turn off fixing
 * of unaligned accesses by the operating system during detection of
 * alignment constraints in H5detect.c:main().
 */
#ifdef H5_HAVE_IO_H
#   include <io.h>
#endif

/*
 * Dynamic library handling.  These are needed for dynamically loading I/O
 * filters and VFDs.
 */
#ifdef H5_HAVE_DLFCN_H
#include <dlfcn.h>
#endif
#ifdef H5_HAVE_DIRENT_H
#include <dirent.h>
#endif

/* Define the default VFD for this platform.
 * Since the removal of the Windows VFD, this is sec2 for all platforms.
 */
#define H5_DEFAULT_VFD      H5FD_SEC2

#ifdef H5_HAVE_WIN32_API
/* The following two defines must be before any windows headers are included */
#define WIN32_LEAN_AND_MEAN    /* Exclude rarely-used stuff from Windows headers */
#define NOGDI                  /* Exclude Graphic Display Interface macros */

#ifdef H5_HAVE_WINSOCK2_H
#include <winsock2.h>
#endif

#ifdef H5_HAVE_THREADSAFE
#include <process.h>        /* For _beginthread() */
#endif

#include <windows.h>
#include <direct.h>         /* For _getcwd() */

#endif /*H5_HAVE_WIN32_API*/

/* Various ways that inline functions can be declared */
#if defined(H5_HAVE___INLINE__)
    /* GNU (alternative form) */
    #define H5_INLINE __inline__
#elif defined(H5_HAVE___INLINE)
    /* Visual Studio */
    #define H5_INLINE __inline
#elif defined(H5_HAVE_INLINE)
    /* GNU, C++
     * Use "inline" as a last resort on the off-chance that there will
     * be C++ problems.
     */
    #define H5_INLINE inline
#else
    #define H5_INLINE
#endif /* inline choices */

#ifndef F_OK
#   define F_OK  00
#   define W_OK 02
#   define R_OK 04
#endif

/*
 * MPE Instrumentation support
 */
#ifdef H5_HAVE_MPE
/*------------------------------------------------------------------------
 * Purpose:    Begin to collect MPE log information for a function. It should
 *             be ahead of the actual function's process.
 *
 * Programmer: Long Wang
 *
 *------------------------------------------------------------------------
 */
#include "mpe.h"
/*
 * #define eventa(func_name)   h5_mpe_ ## func_name ## _a
 * #define eventb(func_name)   h5_mpe_ ## func_name ## _b
 */
#define eventa(func_name)   h5_mpe_eventa
#define eventb(func_name)   h5_mpe_eventb
#define MPE_LOG_VARS                                                    \
    static int eventa(FUNC) = -1;                                       \
    static int eventb(FUNC) = -1;                                       \
    char p_event_start[128];

/* Hardwire the color to "red", since that's what all the routines are using
 * now.  In the future, if we want to change that color for a given routine,
 * we should define a "FUNC_ENTER_API_COLOR" macro which takes an extra 'color'
 * parameter and then make additional FUNC_ENTER_<foo>_COLOR macros to get that
 * color information down to the BEGIN_MPE_LOG macro (which should have a new
 * BEGIN_MPE_LOG_COLOR variant). -QAK
 */
#define BEGIN_MPE_LOG                                                   \
    if (H5_MPEinit_g){                                                  \
        sprintf(p_event_start, "start %s", FUNC);                       \
        if (eventa(FUNC) == -1 && eventb(FUNC) == -1) {                 \
            const char* p_color = "red";                                \
            eventa(FUNC)=MPE_Log_get_event_number();                    \
            eventb(FUNC)=MPE_Log_get_event_number();                    \
            MPE_Describe_state(eventa(FUNC), eventb(FUNC), FUNC, p_color); \
        }                                                               \
        MPE_Log_event(eventa(FUNC), 0, p_event_start);                  \
    }


/*------------------------------------------------------------------------
 * Purpose:   Finish the collection of MPE log information for a function.
 *            It should be after the actual function's process.
 *
 * Programmer: Long Wang
 */
#define FINISH_MPE_LOG                                                  \
    if (H5_MPEinit_g) {                                                 \
        MPE_Log_event(eventb(FUNC), 0, FUNC);                           \
    }

#else /* H5_HAVE_MPE */
#define MPE_LOG_VARS /* void */
#define BEGIN_MPE_LOG /* void */
#define FINISH_MPE_LOG   /* void */

#endif /* H5_HAVE_MPE */

/*
 * dmalloc (debugging malloc) support
 */
#ifdef H5_HAVE_DMALLOC_H
#include "dmalloc.h"
#endif /* H5_HAVE_DMALLOC_H */

/*
 * NT doesn't define SIGBUS, but since NT only runs on processors
 * that do not have alignment constraints a SIGBUS would never be
 * raised, so we just replace it with SIGILL (which also should
 * never be raised by the hdf5 library).
 */
#ifndef SIGBUS
#  define SIGBUS SIGILL
#endif

/*
 * Does the compiler support the __attribute__(()) syntax?  It's no
 * big deal if we don't.
 *
 * Note that Solaris Studio supports attribute, but does not support the
 * attributes we use.
 *
 * H5_ATTR_CONST is redefined in tools/h5repack/dynlib_rpk.c to quiet
 * gcc warnings (it has to use the public API and can't include this
 * file). Be sure to update that file if the #ifdefs change here.
 */
#ifdef __cplusplus
#   define H5_ATTR_FORMAT(X,Y,Z)  /*void*/
#   define H5_ATTR_UNUSED       /*void*/
#   define H5_ATTR_NORETURN     /*void*/
#   define H5_ATTR_CONST        /*void*/
#   define H5_ATTR_PURE         /*void*/
#else /* __cplusplus */
#if defined(H5_HAVE_ATTRIBUTE) && !defined(__SUNPRO_C)
#   define H5_ATTR_FORMAT(X,Y,Z)  __attribute__((format(X, Y, Z)))
#   define H5_ATTR_UNUSED       __attribute__((unused))
#   define H5_ATTR_NORETURN     __attribute__((noreturn))
#   define H5_ATTR_CONST        __attribute__((const))
#   define H5_ATTR_PURE         __attribute__((pure))
#if defined(__GNUC__) && __GNUC__ >= 7 && !defined(__INTEL_COMPILER)
#   define H5_ATTR_FALLTHROUGH  __attribute__((fallthrough));
#else
#   define H5_ATTR_FALLTHROUGH  /*void*/
#endif
#else
#   define H5_ATTR_FORMAT(X,Y,Z)  /*void*/
#   define H5_ATTR_UNUSED       /*void*/
#   define H5_ATTR_NORETURN     /*void*/
#   define H5_ATTR_CONST        /*void*/
#   define H5_ATTR_PURE         /*void*/
#endif
#endif /* __cplusplus */

/*
 * Status return values for the `herr_t' type.
 * Since some unix/c routines use 0 and -1 (or more precisely, non-negative
 * vs. negative) as their return code, and some assumption had been made in
 * the code about that, it is important to keep these constants the same
 * values.  When checking the success or failure of an integer-valued
 * function, remember to compare against zero and not one of these two
 * values.
 */
#define SUCCEED    0
#define FAIL    (-1)
#define UFAIL    (unsigned)(-1)

/* number of members in an array */
#ifndef NELMTS
#    define NELMTS(X)    (sizeof(X)/sizeof(X[0]))
#endif

/* minimum of two, three, or four values */
#undef MIN
#define MIN(a,b)    (((a)<(b)) ? (a) : (b))
#define MIN2(a,b)    MIN(a,b)
#define MIN3(a,b,c)    MIN(a,MIN(b,c))
#define MIN4(a,b,c,d)    MIN(MIN(a,b),MIN(c,d))

/* maximum of two, three, or four values */
#undef MAX
#define MAX(a,b)    (((a)>(b)) ? (a) : (b))
#define MAX2(a,b)    MAX(a,b)
#define MAX3(a,b,c)    MAX(a,MAX(b,c))
#define MAX4(a,b,c,d)    MAX(MAX(a,b),MAX(c,d))

/* limit the middle value to be within a range (inclusive) */
#define RANGE(LO,X,HI)    MAX(LO,MIN(X,HI))

/* absolute value */
#ifndef ABS
#   define ABS(a)    (((a)>=0) ? (a) : -(a))
#endif

/* sign of argument */
#ifndef SIGN
#   define SIGN(a)    ((a)>0 ? 1 : (a)<0 ? -1 : 0)
#endif

/* test for number that is a power of 2 */
/* (from: http://graphics.stanford.edu/~seander/bithacks.html#DetermineIfPowerOf2) */
#  define POWER_OF_TWO(n)  (!(n & (n - 1)) && n)

/* Raise an integer to a power of 2 */
#  define H5_EXP2(n)    (1 << (n))

/*
 * HDF Boolean type.
 */
#ifndef FALSE
  #define FALSE false
#endif
#ifndef TRUE
  #define TRUE true
#endif

/*
 * Numeric data types.  Some of these might be defined in Posix.1g, otherwise
 * we define them with the closest available type which is at least as large
 * as the number of bits indicated in the type name.  The `int8' types *must*
 * be exactly one byte wide because we use it for pointer calculations to
 * void* memory.
 */
#if H5_SIZEOF_INT8_T==0
    typedef signed char int8_t;
#   undef H5_SIZEOF_INT8_T
#   define H5_SIZEOF_INT8_T H5_SIZEOF_CHAR
#elif H5_SIZEOF_INT8_T==1
#else
#   error "the int8_t type must be 1 byte wide"
#endif

#if H5_SIZEOF_UINT8_T==0
    typedef unsigned char uint8_t;
#   undef H5_SIZEOF_UINT8_T
#   define H5_SIZEOF_UINT8_T H5_SIZEOF_CHAR
#elif H5_SIZEOF_UINT8_T==1
#else
#   error "the uint8_t type must be 1 byte wide"
#endif

#if H5_SIZEOF_INT16_T>=2
#elif H5_SIZEOF_SHORT>=2
    typedef short int16_t;
#   undef H5_SIZEOF_INT16_T
#   define H5_SIZEOF_INT16_T H5_SIZEOF_SHORT
#elif H5_SIZEOF_INT>=2
    typedef int int16_t;
#   undef H5_SIZEOF_INT16_T
#   define H5_SIZEOF_INT16_T H5_SIZEOF_INT
#else
#   error "nothing appropriate for int16_t"
#endif

#if H5_SIZEOF_UINT16_T>=2
#elif H5_SIZEOF_SHORT>=2
    typedef unsigned short uint16_t;
#   undef H5_SIZEOF_UINT16_T
#   define H5_SIZEOF_UINT16_T H5_SIZEOF_SHORT
#elif H5_SIZEOF_INT>=2
    typedef unsigned uint16_t;
#   undef H5_SIZEOF_UINT16_T
#   define H5_SIZEOF_UINT16_T H5_SIZEOF_INT
#else
#   error "nothing appropriate for uint16_t"
#endif

#if H5_SIZEOF_INT32_T>=4
#elif H5_SIZEOF_SHORT>=4
    typedef short int32_t;
#   undef H5_SIZEOF_INT32_T
#   define H5_SIZEOF_INT32_T H5_SIZEOF_SHORT
#elif H5_SIZEOF_INT>=4
    typedef int int32_t;
#   undef H5_SIZEOF_INT32_T
#   define H5_SIZEOF_INT32_T H5_SIZEOF_INT
#elif H5_SIZEOF_LONG>=4
    typedef long int32_t;
#   undef H5_SIZEOF_INT32_T
#   define H5_SIZEOF_INT32_T H5_SIZEOF_LONG
#else
#   error "nothing appropriate for int32_t"
#endif

/*
 * Maximum and minimum values.  These should be defined in <limits.h> for the
 * most part.
 */
#ifndef LLONG_MAX
#   define LLONG_MAX  ((long long)(((unsigned long long)1          \
              <<(8*sizeof(long long)-1))-1))
#   define LLONG_MIN    ((long long)(-LLONG_MAX)-1)
#endif
#ifndef ULLONG_MAX
#   define ULLONG_MAX  ((unsigned long long)((long long)(-1)))
#endif
#ifndef SIZET_MAX
#   define SIZET_MAX  ((size_t)(ssize_t)(-1))
#   define SSIZET_MAX  ((ssize_t)(((size_t)1<<(8*sizeof(ssize_t)-1))-1))
#endif

/*
 * Maximum & minimum values for our typedefs.
 */
#define HSIZET_MAX   ((hsize_t)ULLONG_MAX)
#define HSSIZET_MAX  ((hssize_t)LLONG_MAX)
#define HSSIZET_MIN  (~(HSSIZET_MAX))

/*
 * Types and max sizes for POSIX I/O.
 * OS X (Darwin) is odd since the max I/O size does not match the types.
 */
#if defined(H5_HAVE_WIN32_API)
#   define h5_posix_io_t                unsigned int
#   define h5_posix_io_ret_t            int
#   define H5_POSIX_MAX_IO_BYTES        INT_MAX
#elif defined(H5_HAVE_DARWIN)
#   define h5_posix_io_t                size_t
#   define h5_posix_io_ret_t            ssize_t
#   define H5_POSIX_MAX_IO_BYTES        INT_MAX
#else
#   define h5_posix_io_t                size_t
#   define h5_posix_io_ret_t            ssize_t
#   define H5_POSIX_MAX_IO_BYTES        SSIZET_MAX
#endif

/* POSIX I/O mode used as the third parameter to open/_open
 * when creating a new file (O_CREAT is set).
 */
#if defined(H5_HAVE_WIN32_API)
#   define H5_POSIX_CREATE_MODE_RW      (_S_IREAD | _S_IWRITE)
#else
#   define H5_POSIX_CREATE_MODE_RW      0666
#endif

/*
 * A macro to portably increment enumerated types.
 */
#ifndef H5_INC_ENUM
#  define H5_INC_ENUM(TYPE,VAR) (VAR)=((TYPE)((VAR)+1))
#endif

/*
 * A macro to portably decrement enumerated types.
 */
#ifndef H5_DEC_ENUM
#  define H5_DEC_ENUM(TYPE,VAR) (VAR)=((TYPE)((VAR)-1))
#endif

/* Double constant wrapper
 *
 * Quiets gcc warnings from -Wunsuffixed-float-constants.
 *
 * This is a really annoying warning since the standard specifies that
 * constants of type double do NOT get a suffix so there's no way
 * to specify a constant of type double. To quiet gcc, we specify floating
 * point constants as type long double and cast to double.
 *
 * Note that this macro only needs to be used where using a double
 * is important. For most code, suffixing constants with F will quiet the
 * compiler and not produce erroneous code.
 */
#define H5_DOUBLE(S) ((double) S ## L)

/*
 * Methods to compare the equality of floating-point values:
 *
 *    1. H5_XXX_ABS_EQUAL - check if the difference is smaller than the
 *       Epsilon value.  The Epsilon values, FLT_EPSILON, DBL_EPSILON,
 *       and LDBL_EPSILON, are defined by compiler in float.h.
 *
 *    2. H5_XXX_REL_EQUAL - check if the relative difference is smaller than a
 *       predefined value M.  See if two values are relatively equal.
 *       It's the developer's responsibility not to pass in the value 0, which
 *       may cause the equation to fail.
 */
#define H5_FLT_ABS_EQUAL(X,Y)       (HDfabsf((X)-(Y)) < FLT_EPSILON)
#define H5_DBL_ABS_EQUAL(X,Y)       (HDfabs ((X)-(Y)) < DBL_EPSILON)
#define H5_LDBL_ABS_EQUAL(X,Y)      (HDfabsl((X)-(Y)) < LDBL_EPSILON)

#define H5_FLT_REL_EQUAL(X,Y,M)     (HDfabsf(((Y)-(X)) / (X)) < (M))
#define H5_DBL_REL_EQUAL(X,Y,M)     (HDfabs (((Y)-(X)) / (X)) < (M))
#define H5_LDBL_REL_EQUAL(X,Y,M)    (HDfabsl(((Y)-(X)) / (X)) < (M))

/* KiB, MiB, GiB, TiB, PiB, EiB - Used in profiling and timing code */
#define H5_KB (1024.0F)
#define H5_MB (1024.0F * 1024.0F)
#define H5_GB (1024.0F * 1024.0F * 1024.0F)
#define H5_TB (1024.0F * 1024.0F * 1024.0F * 1024.0F)
#define H5_PB (1024.0F * 1024.0F * 1024.0F * 1024.0F * 1024.0F)
#define H5_EB (1024.0F * 1024.0F * 1024.0F * 1024.0F * 1024.0F * 1024.0F)

#ifndef H5_HAVE_FLOCK
/* flock() operations. Used in the source so we have to define them when
 * the call is not available (e.g.: Windows). These should NOT be used
 * with system-provided flock() calls since the values will come from the
 * header file.
 */
#define LOCK_SH     0x01
#define LOCK_EX     0x02
#define LOCK_NB     0x04
#define LOCK_UN     0x08
#endif /* H5_HAVE_FLOCK */

/*
 * Data types and functions for timing certain parts of the library.
 */
typedef struct {
    double  utime;    /*user time      */
    double  stime;    /*system time      */
    double  etime;    /*elapsed wall-clock time  */
} H5_timer_t;

H5_DLL void H5_timer_reset (H5_timer_t *timer);
H5_DLL void H5_timer_begin (H5_timer_t *timer);
H5_DLL void H5_timer_end (H5_timer_t *sum/*in,out*/,
         H5_timer_t *timer/*in,out*/);
H5_DLL void H5_bandwidth(char *buf/*out*/, double nbytes, double nseconds);
H5_DLL time_t H5_now(void);

/* Depth of object copy */
typedef enum {
    H5_COPY_SHALLOW,    /* Shallow copy from source to destination, just copy field pointers */
    H5_COPY_DEEP        /* Deep copy from source to destination, including duplicating fields pointed to */
} H5_copy_depth_t;

/* Common object copying udata (right now only used for groups and datasets) */
typedef struct H5O_copy_file_ud_common_t {
    struct H5O_pline_t *src_pline;      /* Copy of filter pipeline for object */
} H5O_copy_file_ud_common_t;

/* Unique object "position" */
typedef struct {
    unsigned long fileno;       /* The unique identifier for the file of the object */
    haddr_t addr;               /* The unique address of the object's header in that file */
} H5_obj_t;

/*
 * Redefine all the POSIX functions.  We should never see a POSIX
 * function (or any other non-HDF5 function) in the source!
 */

/* Put all platform-specific definitions in the following file */
/* so that the following definitions are platform free. */
#include "H5win32defs.h"  /* For Windows-specific definitions */

#ifndef HDabort
    #define HDabort()    abort()
#endif /* HDabort */
#ifndef HDabs
    #define HDabs(X)    abs(X)
#endif /* HDabs */
#ifndef HDaccess
    #define HDaccess(F,M)    access(F, M)
#endif /* HDaccess */
#ifndef HDacos
    #define HDacos(X)    acos(X)
#endif /* HDacos */
#ifndef HDalarm
    #ifdef H5_HAVE_ALARM
        #define HDalarm(N)              alarm(N)
    #else /* H5_HAVE_ALARM */
        #define HDalarm(N)              (0)
    #endif /* H5_HAVE_ALARM */
#endif /* HDalarm */
#ifndef HDasctime
    #define HDasctime(T)    asctime(T)
#endif /* HDasctime */
#ifndef HDasin
    #define HDasin(X)    asin(X)
#endif /* HDasin */
#ifndef HDasprintf
    #define HDasprintf    asprintf /*varargs*/
#endif /* HDasprintf */
#ifndef HDassert
    #define HDassert(X)    assert(X)
#endif /* HDassert */
#ifndef HDatan
    #define HDatan(X)    atan(X)
#endif /* HDatan */
#ifndef HDatan2
    #define HDatan2(X,Y)    atan2(X,Y)
#endif /* HDatan2 */
#ifndef HDatexit
    #define HDatexit(F)    atexit(F)
#endif /* HDatexit */
#ifndef HDatof
    #define HDatof(S)    atof(S)
#endif /* HDatof */
#ifndef HDatoi
    #define HDatoi(S)    atoi(S)
#endif /* HDatoi */
#ifndef HDatol
    #define HDatol(S)    atol(S)
#endif /* HDatol */
#ifndef HDatoll
    #define HDatoll(S)   atoll(S)
#endif /* HDatol */
#ifndef HDbsearch
    #define HDbsearch(K,B,N,Z,F)  bsearch(K,B,N,Z,F)
#endif /* HDbsearch */
#ifndef HDcalloc
    #define HDcalloc(N,Z)    calloc(N,Z)
#endif /* HDcalloc */
#ifndef HDceil
    #define HDceil(X)    ceil(X)
#endif /* HDceil */
#ifndef HDcfgetispeed
    #define HDcfgetispeed(T)  cfgetispeed(T)
#endif /* HDcfgetispeed */
#ifndef HDcfgetospeed
    #define HDcfgetospeed(T)  cfgetospeed(T)
#endif /* HDcfgetospeed */
#ifndef HDcfsetispeed
    #define HDcfsetispeed(T,S)  cfsetispeed(T,S)
#endif /* HDcfsetispeed */
#ifndef HDcfsetospeed
    #define HDcfsetospeed(T,S)  cfsetospeed(T,S)
#endif /* HDcfsetospeed */
#ifndef HDchdir
    #define HDchdir(S)    chdir(S)
#endif /* HDchdir */
#ifndef HDchmod
    #define HDchmod(S,M)    chmod(S,M)
#endif /* HDchmod */
#ifndef HDchown
    #define HDchown(S,O,G)    chown(S,O,G)
#endif /* HDchown */
#ifndef HDclearerr
    #define HDclearerr(F)    clearerr(F)
#endif /* HDclearerr */
#ifndef HDclock
    #define HDclock()    clock()
#endif /* HDclock */
#ifndef HDclose
    #define HDclose(F)    close(F)
#endif /* HDclose */
#ifndef HDclosedir
    #define HDclosedir(D)    closedir(D)
#endif /* HDclosedir */
#ifndef HDcos
    #define HDcos(X)    cos(X)
#endif /* HDcos */
#ifndef HDcosh
    #define HDcosh(X)    cosh(X)
#endif /* HDcosh */
#ifndef HDcreat
    #define HDcreat(S,M)    creat(S,M)
#endif /* HDcreat */
#ifndef HDctermid
    #define HDctermid(S)    ctermid(S)
#endif /* HDctermid */
#ifndef HDctime
    #define HDctime(T)    ctime(T)
#endif /* HDctime */
#ifndef HDcuserid
    #define HDcuserid(S)    cuserid(S)
#endif /* HDcuserid */
#ifndef HDdifftime
    #ifdef H5_HAVE_DIFFTIME
        #define HDdifftime(X,Y)    difftime(X,Y)
    #else /* H5_HAVE_DIFFTIME */
        #define HDdifftime(X,Y)    ((double)(X)-(double)(Y))
    #endif /* H5_HAVE_DIFFTIME */
#endif /* HDdifftime */
#ifndef HDdiv
    #define HDdiv(X,Y)    div(X,Y)
#endif /* HDdiv */
#ifndef HDdup
    #define HDdup(F)    dup(F)
#endif /* HDdup */
#ifndef HDdup2
    #define HDdup2(F,I)    dup2(F,I)
#endif /* HDdup2 */
/* execl() variable arguments */
/* execle() variable arguments */
/* execlp() variable arguments */
#ifndef HDexecv
    #define HDexecv(S,AV)    execv(S,AV)
#endif /* HDexecv */
#ifndef HDexecve
    #define HDexecve(S,AV,E)  execve(S,AV,E)
#endif /* HDexecve */
#ifndef HDexecvp
    #define HDexecvp(S,AV)    execvp(S,AV)
#endif /* HDexecvp */
#ifndef HDexit
    #define HDexit(N)    exit(N)
#endif /* HDexit */
#ifndef HD_exit
    #define HD_exit(N)    _exit(N)
#endif /* HD_exit */
#ifndef HDexp
    #define HDexp(X)    exp(X)
#endif /* HDexp */
#ifndef HDexp2
    #define HDexp2(X)    exp2(X)
#endif /* HDexp2 */
#ifndef HDfabs
    #define HDfabs(X)    fabs(X)
#endif /* HDfabs */
/* use ABS() because fabsf() fabsl() are not common yet. */
#ifndef HDfabsf
    #define HDfabsf(X)    ABS(X)
#endif /* HDfabsf */
#ifndef HDfabsl
    #define HDfabsl(X)    ABS(X)
#endif /* HDfabsl */
#ifndef HDfclose
    #define HDfclose(F)    fclose(F)
#endif /* HDfclose */
#ifdef H5_HAVE_FCNTL
    #ifndef HDfcntl
        #define HDfcntl(F,C,...)    fcntl(F,C,__VA_ARGS__)
    #endif /* HDfcntl */
#endif /* H5_HAVE_FCNTL */
#ifndef HDfdopen
    #define HDfdopen(N,S)    fdopen(N,S)
#endif /* HDfdopen */
#ifndef HDfeof
    #define HDfeof(F)    feof(F)
#endif /* HDfeof */
#ifndef HDferror
    #define HDferror(F)    ferror(F)
#endif /* HDferror */
#ifndef HDfflush
    #define HDfflush(F)    fflush(F)
#endif /* HDfflush */
#ifndef HDfgetc
    #define HDfgetc(F)    fgetc(F)
#endif /* HDfgetc */
#ifndef HDfgetpos
    #define HDfgetpos(F,P)    fgetpos(F,P)
#endif /* HDfgetpos */
#ifndef HDfgets
    #define HDfgets(S,N,F)    fgets(S,N,F)
#endif /* HDfgets */
#ifndef HDfileno
    #define HDfileno(F)    fileno(F)
#endif /* HDfileno */
/* Since flock is so prevalent, always build these functions
 * when possible to avoid them becoming dead code.
 */
#ifdef H5_HAVE_FCNTL
H5_DLL int Pflock(int fd, int operation);
#endif /* H5_HAVE_FCNTL */
H5_DLL H5_ATTR_CONST int Nflock(int fd, int operation);
#ifndef HDflock
    /* NOTE: flock(2) is not present on all POSIX systems.
     * If it is not present, we try a flock() equivalent based on
     * fcntl(2), then fall back to a function that always fails if
     * it is not present at all (Windows uses a separate Wflock()
     * function).
     */
    #if defined(H5_HAVE_FLOCK)
        #define HDflock(F,L)    flock(F,L)
    #elif defined(H5_HAVE_FCNTL)
        #define HDflock(F,L)    Pflock(F,L)
    #else
        #define HDflock(F,L)    Nflock(F,L)
    #endif /* H5_HAVE_FLOCK */
#endif /* HDflock */
#ifndef HDfloor
    #define HDfloor(X)    floor(X)
#endif /* HDfloor */
#ifndef HDfmod
    #define HDfmod(X,Y)    fmod(X,Y)
#endif /* HDfmod */
#ifndef HDfopen
    #define HDfopen(S,M)    fopen(S,M)
#endif /* HDfopen */
#ifndef HDfork
    #define HDfork()    fork()
#endif /* HDfork */
#ifndef HDfpathconf
    #define HDfpathconf(F,N)  fpathconf(F,N)
#endif /* HDfpathconf */
H5_DLL int HDfprintf (FILE *stream, const char *fmt, ...);
#ifndef HDfputc
    #define HDfputc(C,F)    fputc(C,F)
#endif /* HDfputc */
#ifndef HDfputs
    #define HDfputs(S,F)    fputs(S,F)
#endif /* HDfputs */
#ifndef HDfread
    #define HDfread(M,Z,N,F)  fread(M,Z,N,F)
#endif /* HDfread */
#ifndef HDfree
    #define HDfree(M)    free(M)
#endif /* HDfree */
#ifndef HDfreopen
    #define HDfreopen(S,M,F)  freopen(S,M,F)
#endif /* HDfreopen */
#ifndef HDfrexp
    #define HDfrexp(X,N)    frexp(X,N)
#endif /* HDfrexp */
/* Check for Cray-specific 'frexpf()' and 'frexpl()' routines */
#ifndef HDfrexpf
    #ifdef H5_HAVE_FREXPF
        #define HDfrexpf(X,N)    frexpf(X,N)
    #else /* H5_HAVE_FREXPF */
        #define HDfrexpf(X,N)    frexp(X,N)
    #endif /* H5_HAVE_FREXPF */
#endif /* HDfrexpf */
#ifndef HDfrexpl
    #ifdef H5_HAVE_FREXPL
        #define HDfrexpl(X,N)    frexpl(X,N)
    #else /* H5_HAVE_FREXPL */
        #define HDfrexpl(X,N)    frexp(X,N)
    #endif /* H5_HAVE_FREXPL */
#endif /* HDfrexpl */
/* fscanf() variable arguments */
#ifndef HDfseek
    #define HDfseek(F,O,W)  fseeko(F,O,W)
#endif /* HDfseek */
#ifndef HDfsetpos
    #define HDfsetpos(F,P)    fsetpos(F,P)
#endif /* HDfsetpos */
#ifndef HDfstat
    #define HDfstat(F,B)        fstat(F,B)
#endif /* HDfstat */
#ifndef HDlstat
    #define HDlstat(S,B)    lstat(S,B)
#endif /* HDlstat */
#ifndef HDstat
    #define HDstat(S,B)    stat(S,B)
#endif /* HDstat */

#ifndef H5_HAVE_WIN32_API
/* These definitions differ in Windows and are defined in
 * H5win32defs for that platform.
 */
typedef struct stat         h5_stat_t;
typedef off_t               h5_stat_size_t;
#define HDoff_t             off_t
#endif /* H5_HAVE_WIN32_API */

#define H5_SIZEOF_H5_STAT_SIZE_T H5_SIZEOF_OFF_T

#ifndef HDftell
    #define HDftell(F)    ftello(F)
#endif /* HDftell */
#ifndef HDftruncate
    #define HDftruncate(F,L)        ftruncate(F,L)
#endif /* HDftruncate */
#ifndef HDfwrite
    #define HDfwrite(M,Z,N,F)  fwrite(M,Z,N,F)
#endif /* HDfwrite */
#ifndef HDgetc
    #define HDgetc(F)    getc(F)
#endif /* HDgetc */
#ifndef HDgetchar
    #define HDgetchar()    getchar()
#endif /* HDgetchar */
#ifndef HDgetcwd
    #define HDgetcwd(S,Z)    getcwd(S,Z)
#endif /* HDgetcwd */
#ifndef HDgetdcwd
    #define HDgetdcwd(D,S,Z)  getcwd(S,Z)
#endif /* HDgetdcwd */
#ifndef HDgetdrive
    #define HDgetdrive()    0
#endif /* HDgetdrive */
#ifndef HDgetegid
    #define HDgetegid()    getegid()
#endif /* HDgetegid() */
#ifndef HDgetenv
    #define HDgetenv(S)    getenv(S)
#endif /* HDgetenv */
#ifndef HDgeteuid
    #define HDgeteuid()    geteuid()
#endif /* HDgeteuid */
#ifndef HDgetgid
    #define HDgetgid()    getgid()
#endif /* HDgetgid */
#ifndef HDgetgrgid
    #define HDgetgrgid(G)    getgrgid(G)
#endif /* HDgetgrgid */
#ifndef HDgetgrnam
    #define HDgetgrnam(S)    getgrnam(S)
#endif /* HDgetgrnam */
#ifndef HDgetgroups
    #define HDgetgroups(Z,G)  getgroups(Z,G)
#endif /* HDgetgroups */
#ifndef HDgethostname
    #define HDgethostname(N,L)    gethostname(N,L)
#endif /* HDgetlogin */
#ifndef HDgetlogin
    #define HDgetlogin()    getlogin()
#endif /* HDgetlogin */
#ifndef HDgetpgrp
    #define HDgetpgrp()    getpgrp()
#endif /* HDgetpgrp */
#ifndef HDgetpid
    #define HDgetpid()    getpid()
#endif /* HDgetpid */
#ifndef HDgetppid
    #define HDgetppid()    getppid()
#endif /* HDgetppid */
#ifndef HDgetpwnam
    #define HDgetpwnam(S)    getpwnam(S)
#endif /* HDgetpwnam */
#ifndef HDgetpwuid
    #define HDgetpwuid(U)    getpwuid(U)
#endif /* HDgetpwuid */
#ifndef HDgetrusage
    #define HDgetrusage(X,S)  getrusage(X,S)
#endif /* HDgetrusage */
#ifndef HDgets
    #define HDgets(S)    gets(S)
#endif /* HDgets */
#ifndef HDgettimeofday
    #define HDgettimeofday(S,P)  gettimeofday(S,P)
#endif /* HDgettimeofday */
#ifndef HDgetuid
    #define HDgetuid()    getuid()
#endif /* HDgetuid */
#ifndef HDgmtime
    #define HDgmtime(T)    gmtime(T)
#endif /* HDgmtime */
#ifndef HDisalnum
    #define HDisalnum(C)    isalnum((int)(C)) /*cast for solaris warning*/
#endif /* HDisalnum */
#ifndef HDisalpha
    #define HDisalpha(C)    isalpha((int)(C)) /*cast for solaris warning*/
#endif /* HDisalpha */
#ifndef HDisatty
    #define HDisatty(F)    isatty(F)
#endif /* HDisatty */
#ifndef HDiscntrl
    #define HDiscntrl(C)    iscntrl((int)(C)) /*cast for solaris warning*/
#endif /* HDiscntrl */
#ifndef HDisdigit
    #define HDisdigit(C)    isdigit((int)(C)) /*cast for solaris warning*/
#endif /* HDisdigit */
#ifndef HDisgraph
    #define HDisgraph(C)    isgraph((int)(C)) /*cast for solaris warning*/
#endif /* HDisgraph */
#ifndef HDislower
    #define HDislower(C)    islower((int)(C)) /*cast for solaris warning*/
#endif /* HDislower */
#ifndef HDisprint
    #define HDisprint(C)    isprint((int)(C)) /*cast for solaris warning*/
#endif /* HDisprint */
#ifndef HDispunct
    #define HDispunct(C)    ispunct((int)(C)) /*cast for solaris warning*/
#endif /* HDispunct */
#ifndef HDisspace
    #define HDisspace(C)    isspace((int)(C)) /*cast for solaris warning*/
#endif /* HDisspace */
#ifndef HDisupper
    #define HDisupper(C)    isupper((int)(C)) /*cast for solaris warning*/
#endif /* HDisupper */
#ifndef HDisxdigit
    #define HDisxdigit(C)    isxdigit((int)(C)) /*cast for solaris warning*/
#endif /* HDisxdigit */
#ifndef HDkill
    #define HDkill(P,S)    kill(P,S)
#endif /* HDkill */
#ifndef HDlabs
    #define HDlabs(X)    labs(X)
#endif /* HDlabs */
#ifndef HDldexp
    #define HDldexp(X,N)    ldexp(X,N)
#endif /* HDldexp */
#ifndef HDldiv
    #define HDldiv(X,Y)    ldiv(X,Y)
#endif /* HDldiv */
#ifndef HDlink
    #define HDlink(OLD,NEW)    link(OLD,NEW)
#endif /* HDlink */
#ifndef HDllround
    #define HDllround(V)     llround(V)
#endif /* HDround */
#ifndef HDllroundf
    #define HDllroundf(V)    llroundf(V)
#endif /* HDllroundf */
#ifndef HDllroundl
    #define HDllroundl(V)    llroundl(V)
#endif /* HDllroundl */
#ifndef HDlocaleconv
    #define HDlocaleconv()    localeconv()
#endif /* HDlocaleconv */
#ifndef HDlocaltime
    #define HDlocaltime(T)    localtime(T)
#endif /* HDlocaltime */
#ifndef HDlog
    #define HDlog(X)    log(X)
#endif /* HDlog */
#ifndef HDlog10
    #define HDlog10(X)    log10(X)
#endif /* HDlog10 */
#ifndef HDlongjmp
    #define HDlongjmp(J,N)    longjmp(J,N)
#endif /* HDlongjmp */
#ifndef HDlround
    #define HDlround(V)     lround(V)
#endif /* HDround */
#ifndef HDlroundf
    #define HDlroundf(V)    lroundf(V)
#endif /* HDlroundf */
#ifndef HDlroundl
    #define HDlroundl(V)    lroundl(V)
#endif /* HDroundl */
#ifndef HDlseek
    #define HDlseek(F,O,W)  lseek(F,O,W)
#endif /* HDlseek */
#ifndef HDmalloc
    #define HDmalloc(Z)    malloc(Z)
#endif /* HDmalloc */
#ifndef HDposix_memalign
    #define HDposix_memalign(P,A,Z) posix_memalign(P,A,Z)
#endif /* HDposix_memalign */
#ifndef HDmblen
    #define HDmblen(S,N)    mblen(S,N)
#endif /* HDmblen */
#ifndef HDmbstowcs
    #define HDmbstowcs(P,S,Z)  mbstowcs(P,S,Z)
#endif /* HDmbstowcs */
#ifndef HDmbtowc
    #define HDmbtowc(P,S,Z)    mbtowc(P,S,Z)
#endif /* HDmbtowc */
#ifndef HDmemchr
    #define HDmemchr(S,C,Z)    memchr(S,C,Z)
#endif /* HDmemchr */
#ifndef HDmemcmp
    #define HDmemcmp(X,Y,Z)    memcmp(X,Y,Z)
#endif /* HDmemcmp */
/*
 * The (char*) casts are required for the DEC when optimizations are turned
 * on and the source and/or destination are not aligned.
 */
#ifndef HDmemcpy
    #define HDmemcpy(X,Y,Z)    memcpy((char*)(X),(const char*)(Y),Z)
#endif /* HDmemcpy */
#ifndef HDmemmove
    #define HDmemmove(X,Y,Z)  memmove((char*)(X),(const char*)(Y),Z)
#endif /* HDmemmove */
#ifndef HDmemset
    #define HDmemset(X,C,Z)    memset(X,C,Z)
#endif /* HDmemset */
#ifndef HDmkdir
    #define HDmkdir(S,M)    mkdir(S,M)
#endif /* HDmkdir */
#ifndef HDmkfifo
    #define HDmkfifo(S,M)    mkfifo(S,M)
#endif /* HDmkfifo */
#ifndef HDmktime
    #define HDmktime(T)    mktime(T)
#endif /* HDmktime */
#ifndef HDmodf
    #define HDmodf(X,Y)    modf(X,Y)
#endif /* HDmodf */
#ifndef HDnanosleep
    #define HDnanosleep(N, O)    nanosleep(N, O)
#endif /* HDnanosleep */
#ifndef HDopen
    #define HDopen(F,...)    open(F,__VA_ARGS__)
#endif /* HDopen */
#ifndef HDopendir
    #define HDopendir(S)    opendir(S)
#endif /* HDopendir */
#ifndef HDpathconf
    #define HDpathconf(S,N)    pathconf(S,N)
#endif /* HDpathconf */
#ifndef HDpause
    #define HDpause()    pause()
#endif /* HDpause */
#ifndef HDperror
    #define HDperror(S)    perror(S)
#endif /* HDperror */
#ifndef HDpipe
    #define HDpipe(F)    pipe(F)
#endif /* HDpipe */
#ifndef HDpow
    #define HDpow(X,Y)    pow(X,Y)
#endif /* HDpow */
#ifndef HDpowf
    #define HDpowf(X,Y)   powf(X,Y)
#endif /* HDpowf */
#ifndef HDpread
    #define HDpread(F,B,C,O)    pread(F,B,C,O)
#endif /* HDpread */
#ifndef HDprintf
    #define HDprintf(...)   HDfprintf(stdout, __VA_ARGS__)
#endif /* HDprintf */
#ifndef HDputc
    #define HDputc(C,F)    putc(C,F)
#endif /* HDputc*/
#ifndef HDputchar
    #define HDputchar(C)    putchar(C)
#endif /* HDputchar */
#ifndef HDputs
    #define HDputs(S)    puts(S)
#endif /* HDputs */
#ifndef HDpwrite
    #define HDpwrite(F,B,C,O)    pwrite(F,B,C,O)
#endif /* HDpwrite */
#ifndef HDqsort
    #define HDqsort(M,N,Z,F)  qsort(M,N,Z,F)
#endif /* HDqsort*/
#ifndef HDraise
    #define HDraise(N)    raise(N)
#endif /* HDraise */

#ifdef H5_HAVE_RAND_R
    #ifndef HDrandom
        #define HDrandom()    HDrand()
    #endif /* HDrandom */
    H5_DLL int HDrand(void);
#elif H5_HAVE_RANDOM
    #ifndef HDrand
        #define HDrand()    random()
    #endif /* HDrand */
    #ifndef HDrandom
        #define HDrandom()    random()
    #endif /* HDrandom */
#else /* H5_HAVE_RANDOM */
    #ifndef HDrand
        #define HDrand()    rand()
    #endif /* HDrand */
    #ifndef HDrandom
        #define HDrandom()    rand()
    #endif /* HDrandom */
#endif /* H5_HAVE_RANDOM */

#ifndef HDread
    #define HDread(F,M,Z)    read(F,M,Z)
#endif /* HDread */
#ifndef HDreaddir
    #define HDreaddir(D)    readdir(D)
#endif /* HDreaddir */
#ifndef HDrealloc
    #define HDrealloc(M,Z)    realloc(M,Z)
#endif /* HDrealloc */
#ifndef HDrealpath
    #define HDrealpath(F1,F2)    realpath(F1,F2)
#endif /* HDrealloc */
#ifndef HDremove
    #define HDremove(S)    remove(S)
#endif /* HDremove */
#ifndef HDrename
    #define HDrename(OLD,NEW)  rename(OLD,NEW)
#endif /* HDrename */
#ifndef HDrewind
    #define HDrewind(F)    rewind(F)
#endif /* HDrewind */
#ifndef HDrewinddir
    #define HDrewinddir(D)    rewinddir(D)
#endif /* HDrewinddir */
#ifndef HDround
    #define HDround(V)    round(V)
#endif /* HDround */
#ifndef HDroundf
    #define HDroundf(V)    roundf(V)
#endif /* HDroundf */
#ifndef HDroundl
    #define HDroundl(V)    roundl(V)
#endif /* HDroundl */
#ifndef HDrmdir
    #define HDrmdir(S)    rmdir(S)
#endif /* HDrmdir */
/* scanf() variable arguments */
#ifndef HDselect
    #define HDselect(N,RD,WR,ER,T)    select(N,RD,WR,ER,T)
#endif /* HDsetbuf */
#ifndef HDsetbuf
    #define HDsetbuf(F,S)    setbuf(F,S)
#endif /* HDsetbuf */
#ifndef HDsetenv
    #define HDsetenv(N,V,O)    setenv(N,V,O)
#endif /* HDsetenv */
#ifndef HDsetgid
    #define HDsetgid(G)    setgid(G)
#endif /* HDsetgid */
#ifndef HDsetjmp
    #define HDsetjmp(J)    setjmp(J)
#endif /* HDsetjmp */
#ifndef HDsetlocale
    #define HDsetlocale(N,S)  setlocale(N,S)
#endif /* HDsetlocale */
#ifndef HDsetpgid
    #define HDsetpgid(P,PG)    setpgid(P,PG)
#endif /* HDsetpgid */
#ifndef HDsetsid
    #define HDsetsid()    setsid()
#endif /* HDsetsid */
#ifndef HDsetuid
    #define HDsetuid(U)    setuid(U)
#endif /* HDsetuid */
#ifndef HDsetvbuf
    #define HDsetvbuf(F,S,M,Z)  setvbuf(F,S,M,Z)
#endif /* HDsetvbuf */
#ifndef HDsigaddset
    #define HDsigaddset(S,N)  sigaddset(S,N)
#endif /* HDsigaddset */
#ifndef HDsigdelset
    #define HDsigdelset(S,N)  sigdelset(S,N)
#endif /* HDsigdelset */
#ifndef HDsigemptyset
    #define HDsigemptyset(S)  sigemptyset(S)
#endif /* HDsigemptyset */
#ifndef HDsigfillset
    #define HDsigfillset(S)    sigfillset(S)
#endif /* HDsigfillset */
#ifndef HDsigismember
    #define HDsigismember(S,N)  sigismember(S,N)
#endif /* HDsigismember */
#ifndef HDsiglongjmp
    #define HDsiglongjmp(J,N)  siglongjmp(J,N)
#endif /* HDsiglongjmp */
#ifndef HDsignal
    #define HDsignal(N,F)    signal(N,F)
#endif /* HDsignal */
#ifndef HDsigpending
    #define HDsigpending(S)    sigpending(S)
#endif /* HDsigpending */
#ifndef HDsigprocmask
    #define HDsigprocmask(H,S,O)  sigprocmask(H,S,O)
#endif /* HDsigprocmask */
#ifndef HDsigsetjmp
    #define HDsigsetjmp(J,N)  sigsetjmp(J,N)
#endif /* HDsigsetjmp */
#ifndef HDsigsuspend
    #define HDsigsuspend(S)    sigsuspend(S)
#endif /* HDsigsuspend */
#ifndef HDsin
    #define HDsin(X)    sin(X)
#endif /* HDsin */
#ifndef HDsinh
    #define HDsinh(X)    sinh(X)
#endif /* HDsinh */
#ifndef HDsleep
    #define HDsleep(N)    sleep(N)
#endif /* HDsleep */
#ifndef HDsnprintf
    #define HDsnprintf    snprintf /*varargs*/
#endif /* HDsnprintf */
#ifndef HDsprintf
    #define HDsprintf    sprintf /*varargs*/
#endif /* HDsprintf */
#ifndef HDsqrt
    #define HDsqrt(X)    sqrt(X)
#endif /* HDsqrt */
#ifdef H5_HAVE_RAND_R
    H5_DLL void HDsrand(unsigned int seed);
    #ifndef HDsrandom
        #define HDsrandom(S)    HDsrand(S)
    #endif /* HDsrandom */
#elif H5_HAVE_RANDOM
    #ifndef HDsrand
        #define HDsrand(S)    srandom(S)
    #endif /* HDsrand */
    #ifndef HDsrandom
        #define HDsrandom(S)    srandom(S)
    #endif /* HDsrandom */
#else /* H5_HAVE_RAND_R */
    #ifndef HDsrand
        #define HDsrand(S)    srand(S)
    #endif /* HDsrand */
    #ifndef HDsrandom
        #define HDsrandom(S)    srand(S)
    #endif /* HDsrandom */
#endif /* H5_HAVE_RAND_R */
#ifndef HDsscanf
    #define HDsscanf(S,FMT,...)   sscanf(S,FMT,__VA_ARGS__)
#endif /* HDsscanf */
#ifndef HDstrcat
    #define HDstrcat(X,Y)    strcat(X,Y)
#endif /* HDstrcat */
#ifndef HDstrchr
    #define HDstrchr(S,C)    strchr(S,C)
#endif /* HDstrchr */
#ifndef HDstrcmp
    #define HDstrcmp(X,Y)       strcmp(X,Y)
#endif /* HDstrcmp */
#ifndef HDstrcasecmp
    #define HDstrcasecmp(X,Y)       strcasecmp(X,Y)
#endif /* HDstrcasecmp */
#ifndef HDstrcoll
    #define HDstrcoll(X,Y)    strcoll(X,Y)
#endif /* HDstrcoll */
#ifndef HDstrcpy
    #define HDstrcpy(X,Y)    strcpy(X,Y)
#endif /* HDstrcpy */
#ifndef HDstrcspn
    #define HDstrcspn(X,Y)    strcspn(X,Y)
#endif /* HDstrcspn */
#ifndef HDstrerror
    #define HDstrerror(N)    strerror(N)
#endif /* HDstrerror */
#ifndef HDstrftime
    #define HDstrftime(S,Z,F,T)  strftime(S,Z,F,T)
#endif /* HDstrftime */
#ifndef HDstrlen
    #define HDstrlen(S)    strlen(S)
#endif /* HDstrlen */
#ifndef HDstrncat
    #define HDstrncat(X,Y,Z)  strncat(X,Y,Z)
#endif /* HDstrncat */
#ifndef HDstrncmp
    #define HDstrncmp(X,Y,Z)  strncmp(X,Y,Z)
#endif /* HDstrncmp */
#ifndef HDstrncpy
    #define HDstrncpy(X,Y,Z)  strncpy(X,Y,Z)
#endif /* HDstrncpy */
#ifndef HDstrpbrk
    #define HDstrpbrk(X,Y)    strpbrk(X,Y)
#endif /* HDstrpbrk */
#ifndef HDstrrchr
    #define HDstrrchr(S,C)    strrchr(S,C)
#endif /* HDstrrchr */
#ifndef HDstrspn
    #define HDstrspn(X,Y)    strspn(X,Y)
#endif /* HDstrspn */
#ifndef HDstrstr
    #define HDstrstr(X,Y)    strstr(X,Y)
#endif /* HDstrstr */
#ifndef HDstrtod
    #define HDstrtod(S,R)    strtod(S,R)
#endif /* HDstrtod */
#ifndef HDstrtok
    #define HDstrtok(X,Y)    strtok(X,Y)
#endif /* HDstrtok */
#ifndef HDstrtok_r
    #define HDstrtok_r(X,Y,Z) strtok_r(X,Y,Z)
#endif /* HDstrtok */
#ifndef HDstrtol
    #define HDstrtol(S,R,N)    strtol(S,R,N)
#endif /* HDstrtol */
#ifndef HDstrtoll
    #ifdef H5_HAVE_STRTOLL
        #define HDstrtoll(S,R,N)  strtoll(S,R,N)
    #else
        H5_DLL int64_t HDstrtoll (const char *s, const char **rest, int base);
    #endif /* H5_HAVE_STRTOLL */
#endif /* HDstrtoll */
#ifndef HDstrtoul
    #define HDstrtoul(S,R,N)  strtoul(S,R,N)
#endif /* HDstrtoul */
#ifndef HDstrtoull
    #define HDstrtoull(S,R,N)  strtoull(S,R,N)
#endif /* HDstrtoul */
#ifndef HDstrxfrm
    #define HDstrxfrm(X,Y,Z)  strxfrm(X,Y,Z)
#endif /* HDstrxfrm */
#ifdef H5_HAVE_SYMLINK
    #ifndef HDsymlink
        #define HDsymlink(F1,F2)  symlink(F1,F2)
    #endif /* HDsymlink */
#endif /* H5_HAVE_SYMLINK */
#ifndef HDsysconf
    #define HDsysconf(N)    sysconf(N)
#endif /* HDsysconf */
#ifndef HDsystem
    #define HDsystem(S)    system(S)
#endif /* HDsystem */
#ifndef HDtan
    #define HDtan(X)    tan(X)
#endif /* HDtan */
#ifndef HDtanh
    #define HDtanh(X)    tanh(X)
#endif /* HDtanh */
#ifndef HDtcdrain
    #define HDtcdrain(F)    tcdrain(F)
#endif /* HDtcdrain */
#ifndef HDtcflow
    #define HDtcflow(F,A)    tcflow(F,A)
#endif /* HDtcflow */
#ifndef HDtcflush
    #define HDtcflush(F,N)    tcflush(F,N)
#endif /* HDtcflush */
#ifndef HDtcgetattr
    #define HDtcgetattr(F,T)  tcgetattr(F,T)
#endif /* HDtcgetattr */
#ifndef HDtcgetpgrp
    #define HDtcgetpgrp(F)    tcgetpgrp(F)
#endif /* HDtcgetpgrp */
#ifndef HDtcsendbreak
    #define HDtcsendbreak(F,N)  tcsendbreak(F,N)
#endif /* HDtcsendbreak */
#ifndef HDtcsetattr
    #define HDtcsetattr(F,O,T)  tcsetattr(F,O,T)
#endif /* HDtcsetattr */
#ifndef HDtcsetpgrp
    #define HDtcsetpgrp(F,N)  tcsetpgrp(F,N)
#endif /* HDtcsetpgrp */
#ifndef HDtime
    #define HDtime(T)    time(T)
#endif /* HDtime */
#ifndef HDtimes
    #define HDtimes(T)    times(T)
#endif /* HDtimes*/
#ifndef HDtmpfile
    #define HDtmpfile()    tmpfile()
#endif /* HDtmpfile */
#ifndef HDtmpnam
    #define HDtmpnam(S)    tmpnam(S)
#endif /* HDtmpnam */
#ifndef HDtolower
    #define HDtolower(C)    tolower(C)
#endif /* HDtolower */
#ifndef HDtoupper
    #define HDtoupper(C)    toupper(C)
#endif /* HDtoupper */
#ifndef HDttyname
    #define HDttyname(F)    ttyname(F)
#endif /* HDttyname */
#ifndef HDtzset
    #define HDtzset()    tzset()
#endif /* HDtzset */
#ifndef HDumask
    #define HDumask(N)    umask(N)
#endif /* HDumask */
#ifndef HDuname
    #define HDuname(S)    uname(S)
#endif /* HDuname */
#ifndef HDungetc
    #define HDungetc(C,F)    ungetc(C,F)
#endif /* HDungetc */
#ifndef HDunlink
    #define HDunlink(S)    unlink(S)
#endif /* HDunlink */
#ifndef HDutime
    #define HDutime(S,T)    utime(S,T)
#endif /* HDutime */
#ifndef HDva_arg
    #define HDva_arg(A,T)    va_arg(A,T)
#endif /* HDva_arg */
#ifndef HDva_copy
#define HDva_copy(D,S)    va_copy(D,S)
#endif /* HDva_copy */
#ifndef HDva_end
    #define HDva_end(A)    va_end(A)
#endif /* HDva_end */
#ifndef HDva_start
    #define HDva_start(A,P)    va_start(A,P)
#endif /* HDva_start */
#ifndef HDvasprintf
    #define HDvasprintf(RET,FMT,A)  vasprintf(RET,FMT,A)
#endif /* HDvasprintf */
#ifndef HDvfprintf
    #define HDvfprintf(F,FMT,A)  vfprintf(F,FMT,A)
#endif /* HDvfprintf */
#ifndef HDvprintf
    #define HDvprintf(FMT,A)  vprintf(FMT,A)
#endif /* HDvprintf */
#ifndef HDvsprintf
    #define HDvsprintf(S,FMT,A)  vsprintf(S,FMT,A)
#endif /* HDvsprintf */
#ifndef HDvsnprintf
    #define HDvsnprintf(S,N,FMT,A) vsnprintf(S,N,FMT,A)
#endif /* HDvsnprintf */
#ifndef HDwait
    #define HDwait(W)    wait(W)
#endif /* HDwait */
#ifndef HDwaitpid
    #define HDwaitpid(P,W,O)  waitpid(P,W,O)
#endif /* HDwaitpid */
#ifndef HDwcstombs
    #define HDwcstombs(S,P,Z)  wcstombs(S,P,Z)
#endif /* HDwcstombs */
#ifndef HDwctomb
    #define HDwctomb(S,C)    wctomb(S,C)
#endif /* HDwctomb */
#ifndef HDwrite
    #define HDwrite(F,M,Z)    write(F,M,Z)
#endif /* HDwrite */

/*
 * And now for a couple non-Posix functions...  Watch out for systems that
 * define these in terms of macros.
 */
#if !defined strdup && !defined H5_HAVE_STRDUP
extern char *strdup(const char *s);
#endif

#ifndef HDstrdup
    #define HDstrdup(S)     strdup(S)
#endif /* HDstrdup */

#ifndef HDpthread_self
    #define HDpthread_self()    pthread_self()
#endif /* HDpthread_self */

/* Use this version of pthread_self for printing the thread ID */
#ifndef HDpthread_self_ulong
    #define HDpthread_self_ulong()    ((unsigned long)pthread_self())
#endif /* HDpthread_self_ulong */

/* Macro for "stringizing" an integer in the C preprocessor (use H5_TOSTRING) */
/* (use H5_TOSTRING, H5_STRINGIZE is just part of the implementation) */
#define H5_STRINGIZE(x) #x
#define H5_TOSTRING(x) H5_STRINGIZE(x)

/* Macro for "glueing" together items, for re-scanning macros */
#define H5_GLUE(x,y)       x##y
#define H5_GLUE3(x,y,z)    x##y##z
#define H5_GLUE4(w,x,y,z)  w##x##y##z

/*
 * A macro for detecting over/under-flow when casting between types
 */
#ifndef NDEBUG
#define H5_CHECK_OVERFLOW(var, vartype, casttype) \
{                                                 \
    casttype _tmp_overflow = (casttype)(var);     \
    HDassert((var) == (vartype)_tmp_overflow);      \
}
#else /* NDEBUG */
#define H5_CHECK_OVERFLOW(var, vartype, casttype)
#endif /* NDEBUG */

/*
 * A macro for detecting over/under-flow when assigning between types
 */
#ifndef NDEBUG
#define ASSIGN_TO_SMALLER_SIZE(dst, dsttype, src, srctype)       \
{                                                       \
    srctype _tmp_src = (srctype)(src);  \
    dsttype _tmp_dst = (dsttype)(_tmp_src);  \
    HDassert(_tmp_src == (srctype)_tmp_dst);   \
    (dst) = _tmp_dst;                             \
}

#define ASSIGN_TO_LARGER_SIZE_SAME_SIGNED(dst, dsttype, src, srctype)       \
    (dst) = (dsttype)(src);

#define ASSIGN_TO_LARGER_SIZE_SIGNED_TO_UNSIGNED(dst, dsttype, src, srctype)       \
{                                                       \
    srctype _tmp_src = (srctype)(src);  \
    dsttype _tmp_dst = (dsttype)(_tmp_src);  \
    HDassert(_tmp_src >= 0);   \
    HDassert(_tmp_src == _tmp_dst);   \
    (dst) = _tmp_dst;                             \
}

#define ASSIGN_TO_LARGER_SIZE_UNSIGNED_TO_SIGNED(dst, dsttype, src, srctype)       \
    (dst) = (dsttype)(src);

#define ASSIGN_TO_SAME_SIZE_UNSIGNED_TO_SIGNED(dst, dsttype, src, srctype)       \
{                                                       \
    srctype _tmp_src = (srctype)(src);  \
    dsttype _tmp_dst = (dsttype)(_tmp_src);  \
    HDassert(_tmp_dst >= 0);   \
    HDassert(_tmp_src == (srctype)_tmp_dst);   \
    (dst) = _tmp_dst;                             \
}

#define ASSIGN_TO_SAME_SIZE_SIGNED_TO_UNSIGNED(dst, dsttype, src, srctype)       \
{                                                       \
    srctype _tmp_src = (srctype)(src);  \
    dsttype _tmp_dst = (dsttype)(_tmp_src);  \
    HDassert(_tmp_src >= 0);   \
    HDassert(_tmp_src == (srctype)_tmp_dst);   \
    (dst) = _tmp_dst;                             \
}

#define ASSIGN_TO_SAME_SIZE_SAME_SIGNED(dst, dsttype, src, srctype)       \
    (dst) = (dsttype)(src);

/* Include the generated overflow header file */
#include "H5overflow.h"

/* Assign a variable to one of a different size (think safer dst = (dsttype)src").
 * The code generated by the macro checks for overflows.
 */
#define H5_CHECKED_ASSIGN(dst, dsttype, src, srctype)  \
    H5_GLUE4(ASSIGN_,srctype,_TO_,dsttype)(dst,dsttype,src,srctype)\

#else /* NDEBUG */
#define H5_CHECKED_ASSIGN(dst, dsttype, src, srctype)  \
    (dst) = (dsttype)(src);
#endif /* NDEBUG */

#if defined(H5_HAVE_WINDOW_PATH)

/* directory delimiter for Windows: slash and backslash are acceptable on Windows */
#define H5_DIR_SLASH_SEPC       '/'
#define H5_DIR_SEPC             '\\'
#define H5_DIR_SEPS             "\\"
#define H5_CHECK_DELIMITER(SS)     ((SS == H5_DIR_SEPC) || (SS == H5_DIR_SLASH_SEPC))
#define H5_CHECK_ABSOLUTE(NAME)    ((HDisalpha(NAME[0])) && (NAME[1] == ':') && (H5_CHECK_DELIMITER(NAME[2])))
#define H5_CHECK_ABS_DRIVE(NAME)   ((HDisalpha(NAME[0])) && (NAME[1] == ':'))
#define H5_CHECK_ABS_PATH(NAME)    (H5_CHECK_DELIMITER(NAME[0]))

#define H5_GET_LAST_DELIMITER(NAME, ptr) {                 \
    char        *slash, *backslash;                     \
                                                        \
    slash = HDstrrchr(NAME, H5_DIR_SLASH_SEPC);         \
    backslash = HDstrrchr(NAME, H5_DIR_SEPC);           \
    if(backslash > slash)                               \
        (ptr = backslash);                              \
    else                                                \
        (ptr = slash);                                  \
}

#else /* H5_HAVE_WINDOW_PATH */

#define H5_DIR_SEPC             '/'
#define H5_DIR_SEPS             "/"
#define H5_CHECK_DELIMITER(SS)     (SS == H5_DIR_SEPC)
#define H5_CHECK_ABSOLUTE(NAME)    (H5_CHECK_DELIMITER(*NAME))
#define H5_CHECK_ABS_DRIVE(NAME)   (0)
#define H5_CHECK_ABS_PATH(NAME)    (0)
#define H5_GET_LAST_DELIMITER(NAME, ptr)   ptr = HDstrrchr(NAME, H5_DIR_SEPC);

#endif /* H5_HAVE_WINDOW_PATH */

#define   H5_COLON_SEPC  ':'


/* Use FUNC to safely handle variations of C99 __func__ keyword handling */
#ifdef H5_HAVE_C99_FUNC
#define FUNC __func__
#elif defined(H5_HAVE_FUNCTION)
#define FUNC __FUNCTION__
#else
#error "We need __func__ or __FUNCTION__ to test function names!"
#endif

/*
 * These macros check whether debugging has been requested for a certain
 * package at run-time.   Code for debugging is conditionally compiled by
 * defining constants like `H5X_DEBUG'.   In order to see the output though
 * the code must be enabled at run-time with an environment variable
 * HDF5_DEBUG which is a list of packages to debug.
 *
 * Note:  If you add/remove items from this enum then be sure to update the
 *    information about the package in H5_init_library().
 */
typedef enum {
    H5_PKG_A,       /* Attributes               */
    H5_PKG_AC,      /* Metadata cache           */
    H5_PKG_B,       /* B-trees                  */
    H5_PKG_D,       /* Datasets                 */
    H5_PKG_E,       /* Error handling           */
    H5_PKG_F,       /* Files                    */
    H5_PKG_G,       /* Groups                   */
    H5_PKG_HG,      /* Global heaps             */
    H5_PKG_HL,      /* Local heaps              */
    H5_PKG_I,       /* IDs                      */
    H5_PKG_MF,      /* File memory management   */
    H5_PKG_MM,      /* Core memory management   */
    H5_PKG_O,       /* Object headers           */
    H5_PKG_P,       /* Property lists           */
    H5_PKG_S,       /* Dataspaces               */
    H5_PKG_T,       /* Datatypes                */
    H5_PKG_V,       /* Vector functions         */
    H5_PKG_Z,       /* Raw data filters         */
    H5_NPKGS        /* Must be last             */
} H5_pkg_t;

typedef struct H5_debug_open_stream_t {
    FILE        *stream;                /* Open output stream */
    struct H5_debug_open_stream_t *next; /* Next open output stream */
} H5_debug_open_stream_t;

typedef struct H5_debug_t {
    FILE    *trace;    /*API trace output stream  */
    hbool_t             ttop;           /*Show only top-level calls?    */
    hbool_t             ttimes;         /*Show trace event times?       */
    struct {
  const char  *name;    /*package name      */
  FILE    *stream;  /*output stream  or NULL    */
    } pkg[H5_NPKGS];
    H5_debug_open_stream_t *open_stream; /* Stack of open output streams */
} H5_debug_t;

#ifdef H5_HAVE_PARALLEL
extern hbool_t H5_coll_api_sanity_check_g;
#endif /* H5_HAVE_PARALLEL */

extern H5_debug_t    H5_debug_g;
#define H5DEBUG(X)    (H5_debug_g.pkg[H5_PKG_##X].stream)
/* Do not use const else AIX strings does not show it. */
extern char H5libhdf5_settings[]; /* embedded library information */

/*-------------------------------------------------------------------------
 * Purpose: These macros are inserted automatically just after the
 *          FUNC_ENTER() macro of API functions and are used to trace
 *          application program execution. Unless H5_DEBUG_API has been
 *          defined they are no-ops.
 *
 * Arguments:   R  - Return type encoded as a string
 *              T  - Argument types encoded as a string
 *              A0-An  - Arguments.  The number at the end of the macro name
 *                                   indicates the number of arguments.
 *
 *-------------------------------------------------------------------------
 */
#ifdef H5_DEBUG_API

#define H5TRACE_DECL                                                                                    \
            const char *RTYPE = NULL;                                                                   \
            double CALLTIME;

#define H5TRACE0(R,T)                                                                                   \
            RTYPE=R;                                                                                    \
            CALLTIME=H5_trace(NULL,FUNC,T)
#define H5TRACE1(R,T,A0)                                                                                \
            RTYPE=R;                                                                                    \
            CALLTIME=H5_trace(NULL,FUNC,T,#A0,A0)
#define H5TRACE2(R,T,A0,A1)                                                                             \
            RTYPE=R;                                                                                    \
            CALLTIME=H5_trace(NULL,FUNC,T,#A0,A0,#A1,A1)
#define H5TRACE3(R,T,A0,A1,A2)                                                                          \
            RTYPE=R;                                                                                    \
            CALLTIME=H5_trace(NULL,FUNC,T,#A0,A0,#A1,A1,#A2,A2)
#define H5TRACE4(R,T,A0,A1,A2,A3)                                                                       \
            RTYPE=R;                                                                                    \
            CALLTIME=H5_trace(NULL,FUNC,T,#A0,A0,#A1,A1,#A2,A2,#A3,A3)
#define H5TRACE5(R,T,A0,A1,A2,A3,A4)                                                                    \
            RTYPE=R;                                                                                    \
            CALLTIME=H5_trace(NULL,FUNC,T,#A0,A0,#A1,A1,#A2,A2,#A3,A3,#A4,A4)
#define H5TRACE6(R,T,A0,A1,A2,A3,A4,A5)                                                                 \
            RTYPE=R;                                                                                    \
            CALLTIME=H5_trace(NULL,FUNC,T,#A0,A0,#A1,A1,#A2,A2,#A3,A3,#A4,A4,#A5,A5)
#define H5TRACE7(R,T,A0,A1,A2,A3,A4,A5,A6)                                                              \
            RTYPE=R;                                                                                    \
            CALLTIME=H5_trace(NULL,FUNC,T,#A0,A0,#A1,A1,#A2,A2,#A3,A3,#A4,A4,#A5,A5,#A6,A6)
#define H5TRACE8(R,T,A0,A1,A2,A3,A4,A5,A6,A7)                                                           \
            RTYPE=R;                                                                                    \
            CALLTIME=H5_trace(NULL,FUNC,T,#A0,A0,#A1,A1,#A2,A2,#A3,A3,#A4,A4,#A5,A5,#A6,A6,#A7,A7)
#define H5TRACE9(R,T,A0,A1,A2,A3,A4,A5,A6,A7,A8)                                                        \
            RTYPE=R;                                                                                    \
            CALLTIME=H5_trace(NULL,FUNC,T,#A0,A0,#A1,A1,#A2,A2,#A3,A3,#A4,A4,#A5,A5,#A6,A6,#A7,A7,      \
                                          #A8,A8)
#define H5TRACE10(R,T,A0,A1,A2,A3,A4,A5,A6,A7,A8,A9)                                                    \
            RTYPE=R;                                                                                    \
            CALLTIME=H5_trace(NULL,FUNC,T,#A0,A0,#A1,A1,#A2,A2,#A3,A3,#A4,A4,#A5,A5,#A6,A6,#A7,A7,      \
                                          #A8,A8,#A9,A9)
#define H5TRACE11(R,T,A0,A1,A2,A3,A4,A5,A6,A7,A8,A9,A10)                                                \
            RTYPE=R;                                                                                    \
            CALLTIME=H5_trace(NULL,FUNC,T,#A0,A0,#A1,A1,#A2,A2,#A3,A3,#A4,A4,#A5,A5,#A6,A6,#A7,A7,      \
                                          #A8,A8,#A9,A9,#A10,A10)
#define H5TRACE12(R,T,A0,A1,A2,A3,A4,A5,A6,A7,A8,A9,A10,A11)                                            \
            RTYPE=R;                                                                                    \
            CALLTIME=H5_trace(NULL,FUNC,T,#A0,A0,#A1,A1,#A2,A2,#A3,A3,#A4,A4,#A5,A5,#A6,A6,#A7,A7,      \
                                          #A8,A8,#A9,A9,#A10,A10,#A11,A11)

#define H5TRACE_RETURN(V)                                                                               \
            if (RTYPE) {                                                                                \
                H5_trace(&CALLTIME, FUNC, RTYPE, NULL, V);                                              \
                RTYPE = NULL;                                                                           \
            }
#else
#define H5TRACE_DECL                                            /*void*/
#define H5TRACE0(R,T)                                           /*void*/
#define H5TRACE1(R,T,A0)                                        /*void*/
#define H5TRACE2(R,T,A0,A1)                                     /*void*/
#define H5TRACE3(R,T,A0,A1,A2)                                  /*void*/
#define H5TRACE4(R,T,A0,A1,A2,A3)                               /*void*/
#define H5TRACE5(R,T,A0,A1,A2,A3,A4)                            /*void*/
#define H5TRACE6(R,T,A0,A1,A2,A3,A4,A5)                         /*void*/
#define H5TRACE7(R,T,A0,A1,A2,A3,A4,A5,A6)                      /*void*/
#define H5TRACE8(R,T,A0,A1,A2,A3,A4,A5,A6,A7)                   /*void*/
#define H5TRACE9(R,T,A0,A1,A2,A3,A4,A5,A6,A7,A8)                /*void*/
#define H5TRACE10(R,T,A0,A1,A2,A3,A4,A5,A6,A7,A8,A9)            /*void*/
#define H5TRACE11(R,T,A0,A1,A2,A3,A4,A5,A6,A7,A8,A9,A10)        /*void*/
#define H5TRACE12(R,T,A0,A1,A2,A3,A4,A5,A6,A7,A8,A9,A10,A11)    /*void*/
#define H5TRACE_RETURN(V)                                       /*void*/
#endif /* H5_DEBUG_API */

H5_DLL double H5_trace(const double *calltime, const char *func, const char *type, ...);


/*-------------------------------------------------------------------------
 * Purpose:  Register function entry for library initialization and code
 *    profiling.
 *
 * Notes:  Every file must have a file-scope variable called
 *    `initialize_interface_g' of type hbool_t which is initialized
 *    to FALSE.
 *
 *    Don't use local variable initializers which contain
 *    calls to other library functions since the initializer
 *    would happen before the FUNC_ENTER() gets called.  Don't
 *    use initializers that require special cleanup code to
 *    execute if FUNC_ENTER() fails since a failing FUNC_ENTER()
 *    returns immediately without branching to the `done' label.
 *
 * Programmer:  Quincey Koziol
 *
 *-------------------------------------------------------------------------
 */

/* `S' is the name of a function which is being tested to check if it's
 *  an API function.
 *
 *  BADNESS:
 *      - Underscore at positions 2 or 3 (0-indexed string). Handles
 *        H5_ and H5X_.
 *      - Underscore at position 4 if position 3 is uppercase or a digit.
 *        Handles H5XY_.
 */
#define H5_IS_API(S) (\
    '_'!=((const char *)S)[2]       /* underscore at position 2     */  \
    && '_'!=((const char *)S)[3]    /* underscore at position 3     */  \
    && !(                                       /* NOT              */  \
        ((const char *)S)[4]                    /* pos 4 exists     */  \
        && (HDisupper(S[3]) || HDisdigit(S[3])) /* pos 3 dig | uc   */  \
        && '_'==((const char *)S)[4]            /* pos 4 underscore */  \
    )\
)

/* `S' is the name of a function which is being tested to check if it's */
/*      a public API function */
#define H5_IS_PUB(S) (((HDisdigit(S[1]) || HDisupper(S[1])) && HDislower(S[2])) || \
    ((HDisdigit(S[2]) || HDisupper(S[2])) && HDislower(S[3])) || \
    (!S[4] || ((HDisdigit(S[3]) || HDisupper(S[3])) && HDislower(S[4]))))

/* `S' is the name of a function which is being tested to check if it's */
/*      a private library function */
#define H5_IS_PRIV(S) (((HDisdigit(S[1]) || HDisupper(S[1])) && '_' == S[2] && HDislower(S[3])) || \
    ((HDisdigit(S[2]) || HDisupper(S[2])) && '_' == S[3] && HDislower(S[4])) || \
    ((HDisdigit(S[3]) || HDisupper(S[3])) && '_' == S[4] && HDislower(S[5])))

/* `S' is the name of a function which is being tested to check if it's */
/*      a package private function */
#define H5_IS_PKG(S) (((HDisdigit(S[1]) || HDisupper(S[1])) && '_' == S[2] && '_' == S[3] && HDislower(S[4])) || \
    ((HDisdigit(S[2]) || HDisupper(S[2])) && '_' == S[3] && '_' == S[4] && HDislower(S[5])) || \
    ((HDisdigit(S[3]) || HDisupper(S[3])) && '_' == S[4] && '_' == S[5] && HDislower(S[6])))

/* global library version information string */
extern char  H5_lib_vers_info_g[];

/* Lock headers */
#ifdef H5_HAVE_THREADSAFE

/* Include required thread-safety header */
#include "H5TSprivate.h"

/* replacement structure for original global variable */
typedef struct H5_api_struct {
    H5TS_mutex_t init_lock;  /* API entrance mutex */
    hbool_t H5_libinit_g;    /* Has the library been initialized? */
    hbool_t H5_libterm_g;    /* Is the library being shutdown? */
} H5_api_t;

/* Macros for accessing the global variables */
#define H5_INIT_GLOBAL (H5_g.H5_libinit_g)
#define H5_TERM_GLOBAL (H5_g.H5_libterm_g)

/* Macro for first thread initialization */
#ifdef H5_HAVE_WIN_THREADS
#define H5_FIRST_THREAD_INIT InitOnceExecuteOnce(&H5TS_first_init_g, H5TS_win32_process_enter, NULL, NULL);
#else
#define H5_FIRST_THREAD_INIT pthread_once(&H5TS_first_init_g, H5TS_pthread_first_thread_init);
#endif

/* Macros for threadsafe HDF-5 Phase I locks */
#define H5_API_LOCK                                                           \
     H5TS_mutex_lock(&H5_g.init_lock);
#define H5_API_UNLOCK                                                         \
     H5TS_mutex_unlock(&H5_g.init_lock);

/* Macros for thread cancellation-safe mechanism */
#define H5_API_UNSET_CANCEL                                                   \
    H5TS_cancel_count_inc();

#define H5_API_SET_CANCEL                                                     \
    H5TS_cancel_count_dec();

extern H5_api_t H5_g;

#else /* H5_HAVE_THREADSAFE */

/* disable any first thread init mechanism */
#define H5_FIRST_THREAD_INIT

/* disable locks (sequential version) */
#define H5_API_LOCK
#define H5_API_UNLOCK

/* disable cancelability (sequential version) */
#define H5_API_UNSET_CANCEL
#define H5_API_SET_CANCEL

/* extern global variables */
extern hbool_t H5_libinit_g;    /* Has the library been initialized? */
extern hbool_t H5_libterm_g;    /* Is the library being shutdown? */

/* Macros for accessing the global variables */
#define H5_INIT_GLOBAL (H5_libinit_g)
#define H5_TERM_GLOBAL (H5_libterm_g)

#endif /* H5_HAVE_THREADSAFE */

#ifdef H5_HAVE_CODESTACK

/* Include required function stack header */
#include "H5CSprivate.h"

#define H5_PUSH_FUNC            H5CS_push(FUNC);
#define H5_POP_FUNC             H5CS_pop();
#else /* H5_HAVE_CODESTACK */
#define H5_PUSH_FUNC            /* void */
#define H5_POP_FUNC             /* void */
#endif /* H5_HAVE_CODESTACK */

#ifdef H5_HAVE_MPE
extern hbool_t H5_MPEinit_g;   /* Has the MPE Library been initialized? */
#endif

/* Macros for referencing package initialization symbols */
#define H5_PACKAGE_INIT_VAR(x)    H5_GLUE(x, _init_g)
#define H5_PACKAGE_INIT_FUNC(x)   H5_GLUE(x, __init_package)

/* Macros for defining package initialization routines */
#ifdef H5_MY_PKG
#define H5_PKG_INIT_VAR                 H5_PACKAGE_INIT_VAR(H5_MY_PKG)
#define H5_PKG_INIT_FUNC                H5_PACKAGE_INIT_FUNC(H5_MY_PKG)
#define H5_PACKAGE_YES_INIT(err)                                              \
    /* Initialize this interface or bust */                                   \
    if(!H5_PKG_INIT_VAR && !H5_TERM_GLOBAL) {                                 \
        H5_PKG_INIT_VAR = TRUE;                                               \
        if(H5_PKG_INIT_FUNC() < 0) {                                          \
            H5_PKG_INIT_VAR = FALSE;                                          \
            HGOTO_ERROR(H5E_FUNC, H5E_CANTINIT, err, "interface initialization failed") \
        }                                                                     \
    }
#define H5_PACKAGE_NO_INIT(err)                                               \
    /* Initialize this interface or bust */                                   \
    if(!H5_PKG_INIT_VAR && !H5_TERM_GLOBAL)                                   \
        H5_PKG_INIT_VAR = TRUE;
#define H5_PACKAGE_INIT(pkg_init, err)  H5_GLUE3(H5_PACKAGE_, pkg_init, _INIT)(err)
#else /* H5_MY_PKG */
#define H5_PKG_INIT_VAR                 (TRUE)
#define H5_PACKAGE_INIT(pkg_init, err)
#endif /* H5_MY_PKG */

/* Forward declaration of H5CXpush() / H5CXpop() */
/* (Including H5CXprivate.h creates bad circular dependencies - QAK, 3/18/2018) */
H5_DLL herr_t H5CX_push(void);
H5_DLL herr_t H5CX_pop(void);

/* XXX(kitware): Our mangling breaks the rules for this; assume upstream did their job. */
/* #ifndef NDEBUG */
#if 0
#define FUNC_ENTER_CHECK_NAME(asrt)                                           \
    {                                                                         \
        static hbool_t func_check = FALSE;                                    \
                                                                              \
        if(!func_check) {                                                     \
            /* Check function naming status */                                \
            HDassert(asrt && "Function naming conventions are incorrect - check H5_IS_API|PUB|PRIV|PKG macros in H5private.h (this is usually due to an incorrect number of underscores)");                                                   \
                                                                              \
            /* Don't check again */                                           \
            func_check = TRUE;                                                \
        } /* end if */                                                        \
    } /* end scope */
#else /* NDEBUG */
#define FUNC_ENTER_CHECK_NAME(asrt)
#endif /* NDEBUG */


#define FUNC_ENTER_COMMON(asrt)                                               \
    hbool_t err_occurred = FALSE;                                             \
                                                                              \
    FUNC_ENTER_CHECK_NAME(asrt);

#define FUNC_ENTER_COMMON_NOERR(asrt)                                         \
    FUNC_ENTER_CHECK_NAME(asrt);

/* Threadsafety initialization code for API routines */
#define FUNC_ENTER_API_THREADSAFE                                             \
   /* Initialize the thread-safe code */                                      \
   H5_FIRST_THREAD_INIT                                                       \
                                                                              \
   /* Grab the mutex for the library */                                       \
   H5_API_UNSET_CANCEL                                                        \
   H5_API_LOCK

/* Local variables for API routines */
#define FUNC_ENTER_API_VARS                                                   \
    MPE_LOG_VARS                                                              \
    H5TRACE_DECL

#define FUNC_ENTER_API_COMMON                                                 \
    FUNC_ENTER_API_VARS                                                       \
    FUNC_ENTER_COMMON(H5_IS_API(FUNC));                                       \
    FUNC_ENTER_API_THREADSAFE;

#define FUNC_ENTER_API_INIT(err)                                              \
    /* Initialize the library */                                              \
    if(!H5_INIT_GLOBAL && !H5_TERM_GLOBAL) {                                  \
        H5_INIT_GLOBAL = TRUE;                                                \
        if(H5_init_library() < 0)                                             \
            HGOTO_ERROR(H5E_FUNC, H5E_CANTINIT, err, "library initialization failed") \
    } /* end if */                                                            \
                                                                              \
    /* Initialize the package, if appropriate */                              \
    H5_PACKAGE_INIT(H5_MY_PKG_INIT, err)                                      \
                                                                              \
    /* Push the name of this function on the function stack */                \
    H5_PUSH_FUNC                                                              \
                                                                              \
    /* Push the API context */                                                \
    if(H5CX_push() < 0)                                                       \
        HGOTO_ERROR(H5E_FUNC, H5E_CANTSET, err, "can't set API context")      \
                                                                              \
    BEGIN_MPE_LOG

/* Use this macro for all "normal" API functions */
#define FUNC_ENTER_API(err) {{                                                \
    FUNC_ENTER_API_COMMON                                                     \
    FUNC_ENTER_API_INIT(err);                                                 \
    /* Clear thread error stack entering public functions */                  \
    H5E_clear_stack(NULL);                                                    \
    {

/*
 * Use this macro for API functions that shouldn't clear the error stack
 *      like H5Eprint and H5Ewalk.
 */
#define FUNC_ENTER_API_NOCLEAR(err) {{                                        \
    FUNC_ENTER_API_COMMON                                                     \
    FUNC_ENTER_API_INIT(err);                                                 \
    {

/*
 * Use this macro for API functions that shouldn't perform _any_ initialization
 *      of the library or an interface, just perform tracing, etc.  Examples
 *      are: H5allocate_memory, H5is_library_threadsafe, etc.
 *
 */
#define FUNC_ENTER_API_NOINIT {{                                              \
    FUNC_ENTER_API_COMMON                                                     \
    H5_PUSH_FUNC                                                              \
    BEGIN_MPE_LOG                                                             \
    {

/*
 * Use this macro for API functions that shouldn't perform _any_ initialization
 *      of the library or an interface or push themselves on the function
 *      stack, just perform tracing, etc.  Examples
 *      are: H5close, H5check_version, etc.
 *
 */
#define FUNC_ENTER_API_NOINIT_NOERR_NOFS {{                                   \
    FUNC_ENTER_API_VARS                                                       \
    FUNC_ENTER_COMMON_NOERR(H5_IS_API(FUNC));                                 \
    FUNC_ENTER_API_THREADSAFE;                                                \
    BEGIN_MPE_LOG                                                             \
    {

/* Note: this macro only works when there's _no_ interface initialization routine for the module */
#define FUNC_ENTER_NOAPI_INIT(err)                                            \
    /* Initialize the package, if appropriate */                              \
    H5_PACKAGE_INIT(H5_MY_PKG_INIT, err)                                      \
                                                                              \
    /* Push the name of this function on the function stack */                \
    H5_PUSH_FUNC

/* Use this macro for all "normal" non-API functions */
#define FUNC_ENTER_NOAPI(err) {                                               \
    FUNC_ENTER_COMMON(!H5_IS_API(FUNC));                                      \
    FUNC_ENTER_NOAPI_INIT(err)                                                \
    if(H5_PKG_INIT_VAR || !H5_TERM_GLOBAL) {

/* Use this macro for all non-API functions, which propagate errors, but don't issue them */
#define FUNC_ENTER_NOAPI_NOERR {                                              \
    FUNC_ENTER_COMMON_NOERR(!H5_IS_API(FUNC));                                \
    FUNC_ENTER_NOAPI_INIT(-)                                                  \
    if(H5_PKG_INIT_VAR || !H5_TERM_GLOBAL) {

/*
 * Use this macro for non-API functions which fall into these categories:
 *      - static functions, since they must be called from a function in the
 *              interface, the library and interface must already be
 *              initialized.
 *      - functions which are called during library shutdown, since we don't
 *              want to re-initialize the library.
 */
#define FUNC_ENTER_NOAPI_NOINIT {                                             \
    FUNC_ENTER_COMMON(!H5_IS_API(FUNC));                                      \
    H5_PUSH_FUNC                                                              \
    if(H5_PKG_INIT_VAR || !H5_TERM_GLOBAL) {

/*
 * Use this macro for non-API functions which fall into these categories:
 *      - static functions, since they must be called from a function in the
 *              interface, the library and interface must already be
 *              initialized.
 *      - functions which are called during library shutdown, since we don't
 *              want to re-initialize the library.
 *      - functions that propagate, but don't issue errors
 */
#define FUNC_ENTER_NOAPI_NOINIT_NOERR {                                       \
    FUNC_ENTER_COMMON_NOERR(!H5_IS_API(FUNC));                                \
    H5_PUSH_FUNC                                                              \
    if(H5_PKG_INIT_VAR || !H5_TERM_GLOBAL) {

/*
 * Use this macro for non-API functions which fall into these categories:
 *      - functions which shouldn't push their name on the function stack
 *              (so far, just the H5CS routines themselves)
 *
 */
#define FUNC_ENTER_NOAPI_NOFS {                                               \
    FUNC_ENTER_COMMON(!H5_IS_API(FUNC));                                      \
                                                                              \
    /* Initialize the package, if appropriate */                              \
    H5_PACKAGE_INIT(H5_MY_PKG_INIT, err)                                      \
    if(H5_PKG_INIT_VAR || !H5_TERM_GLOBAL) {

/*
 * Use this macro for non-API functions which fall into these categories:
 *      - functions which shouldn't push their name on the function stack
 *              (so far, just the H5CS routines themselves)
 *
 * This macro is used for functions which fit the above categories _and_
 * also don't use the 'FUNC' variable (i.e. don't push errors on the error stack)
 *
 */
#define FUNC_ENTER_NOAPI_NOERR_NOFS {                                         \
    FUNC_ENTER_COMMON_NOERR(!H5_IS_API(FUNC));                                \
    if(H5_PKG_INIT_VAR || !H5_TERM_GLOBAL) {

/* Use the following two macros as replacements for the FUNC_ENTER_NOAPI
 * and FUNC_ENTER_NOAPI_NOINIT macros when the function needs to set
 * up a metadata tag. */
#define FUNC_ENTER_NOAPI_TAG(tag, err) {                                      \
    haddr_t prev_tag = HADDR_UNDEF;                                           \
                                                                              \
    FUNC_ENTER_COMMON(!H5_IS_API(FUNC));                                      \
    H5AC_tag(tag, &prev_tag);                                                 \
    FUNC_ENTER_NOAPI_INIT(err)                                                \
    if(H5_PKG_INIT_VAR || !H5_TERM_GLOBAL) {

#define FUNC_ENTER_NOAPI_NOINIT_TAG(tag) {                                    \
    haddr_t prev_tag = HADDR_UNDEF;                                           \
                                                                              \
    FUNC_ENTER_COMMON(!H5_IS_API(FUNC));                                      \
    H5AC_tag(tag, &prev_tag);                                                 \
    H5_PUSH_FUNC                                                              \
    if(H5_PKG_INIT_VAR || !H5_TERM_GLOBAL) {

/* Use this macro for all "normal" package-level functions */
#define FUNC_ENTER_PACKAGE {                                                  \
    FUNC_ENTER_COMMON(H5_IS_PKG(FUNC));                                       \
    H5_PUSH_FUNC                                                              \
    if(H5_PKG_INIT_VAR || !H5_TERM_GLOBAL) {

/* Use this macro for package-level functions which propgate errors, but don't issue them */
#define FUNC_ENTER_PACKAGE_NOERR {                                            \
    FUNC_ENTER_COMMON_NOERR(H5_IS_PKG(FUNC));                                 \
    H5_PUSH_FUNC                                                              \
    if(H5_PKG_INIT_VAR || !H5_TERM_GLOBAL) {

/* Use the following macro as replacement for the FUNC_ENTER_PACKAGE
 * macro when the function needs to set up a metadata tag. */
#define FUNC_ENTER_PACKAGE_TAG(tag) {                                         \
    haddr_t prev_tag = HADDR_UNDEF;                                           \
                                                                              \
    FUNC_ENTER_COMMON(H5_IS_PKG(FUNC));                                       \
    H5AC_tag(tag, &prev_tag);                                                 \
    H5_PUSH_FUNC                                                              \
    if(H5_PKG_INIT_VAR || !H5_TERM_GLOBAL) {

/* Use this macro for all "normal" staticly-scoped functions */
#define FUNC_ENTER_STATIC {                                                   \
    FUNC_ENTER_COMMON(H5_IS_PKG(FUNC));                                       \
    H5_PUSH_FUNC                                                              \
    if(H5_PKG_INIT_VAR || !H5_TERM_GLOBAL) {

/* Use this macro for staticly-scoped functions which propgate errors, but don't issue them */
#define FUNC_ENTER_STATIC_NOERR {                                             \
    FUNC_ENTER_COMMON_NOERR(H5_IS_PKG(FUNC));                                 \
    H5_PUSH_FUNC                                                              \
    if(H5_PKG_INIT_VAR || !H5_TERM_GLOBAL) {

/* Use this macro for staticly-scoped functions which propgate errors, but don't issue them */
/* And that shouldn't push their name on the function stack */
#define FUNC_ENTER_STATIC_NOERR_NOFS {                                        \
    FUNC_ENTER_COMMON_NOERR(H5_IS_PKG(FUNC));                                 \
    if(H5_PKG_INIT_VAR || !H5_TERM_GLOBAL) {

/* Use the following macro as replacement for the FUNC_ENTER_STATIC
 * macro when the function needs to set up a metadata tag. */
#define FUNC_ENTER_STATIC_TAG(tag) {                                          \
    haddr_t prev_tag = HADDR_UNDEF;                                           \
                                                                              \
    FUNC_ENTER_COMMON(H5_IS_PKG(FUNC));                                       \
    H5AC_tag(tag, &prev_tag);                                                 \
    H5_PUSH_FUNC                                                              \
    if(H5_PKG_INIT_VAR || !H5_TERM_GLOBAL) {


/*-------------------------------------------------------------------------
 * Purpose:  Register function exit for code profiling.  This should be
 *    the last statement executed by a function.
 *
 * Programmer:  Quincey Koziol
 *
 *-------------------------------------------------------------------------
 */
/* Threadsafety termination code for API routines */
#define FUNC_LEAVE_API_THREADSAFE                                             \
    H5_API_UNLOCK                                                             \
    H5_API_SET_CANCEL

#define FUNC_LEAVE_API_COMMON(ret_value)                                      \
        ;                                                                     \
    } /*end scope from end of FUNC_ENTER*/                                    \
    FINISH_MPE_LOG                                                            \
    H5TRACE_RETURN(ret_value);

#define FUNC_LEAVE_API(ret_value)                                             \
    FUNC_LEAVE_API_COMMON(ret_value);                                         \
    (void)H5CX_pop();                                                         \
    H5_POP_FUNC                                                               \
    if(err_occurred)                                                          \
       (void)H5E_dump_api_stack(TRUE);                                        \
    FUNC_LEAVE_API_THREADSAFE                                                 \
    return(ret_value);                                                        \
}} /*end scope from beginning of FUNC_ENTER*/

/* Use this macro to match the FUNC_ENTER_API_NOINIT macro */
#define FUNC_LEAVE_API_NOINIT(ret_value)                                      \
    FUNC_LEAVE_API_COMMON(ret_value);                                         \
    H5_POP_FUNC                                                               \
    if(err_occurred)                                                          \
       (void)H5E_dump_api_stack(TRUE);                                        \
    FUNC_LEAVE_API_THREADSAFE                                                 \
    return(ret_value);                                                        \
}} /*end scope from beginning of FUNC_ENTER*/

/* Use this macro to match the FUNC_ENTER_API_NOINIT_NOERR_NOFS macro */
#define FUNC_LEAVE_API_NOFS(ret_value)                                        \
    FUNC_LEAVE_API_COMMON(ret_value);                                         \
    FUNC_LEAVE_API_THREADSAFE                                                 \
    return(ret_value);                                                        \
}} /*end scope from beginning of FUNC_ENTER*/

#define FUNC_LEAVE_NOAPI(ret_value)                                           \
        ;                                                                     \
    } /*end scope from end of FUNC_ENTER*/                                    \
    H5_POP_FUNC                                                               \
    return(ret_value);                                                        \
} /*end scope from beginning of FUNC_ENTER*/

#define FUNC_LEAVE_NOAPI_VOID                                                 \
        ;                                                                     \
    } /*end scope from end of FUNC_ENTER*/                                    \
    H5_POP_FUNC                                                               \
    return;                                                                   \
} /*end scope from beginning of FUNC_ENTER*/

/*
 * Use this macro for non-API functions which fall into these categories:
 *      - functions which didn't push their name on the function stack
 *              (so far, just the H5CS routines themselves)
 */
#define FUNC_LEAVE_NOAPI_NOFS(ret_value)                                      \
        ;                                                                     \
    } /*end scope from end of FUNC_ENTER*/                                    \
    return(ret_value);                                                        \
} /*end scope from beginning of FUNC_ENTER*/

/* Use this macro when exiting a function that set up a metadata tag */
#define FUNC_LEAVE_NOAPI_TAG(ret_value)                                       \
        ;                                                                     \
    } /*end scope from end of FUNC_ENTER*/                                    \
    H5AC_tag(prev_tag, NULL);                                                 \
    H5_POP_FUNC                                                               \
    return(ret_value);                                                        \
} /*end scope from beginning of FUNC_ENTER*/


/****************************************/
/* Revisions to FUNC_ENTER/LEAVE Macros */
/****************************************/

/* Macros to check if a package is initialized */
#define H5_CHECK_PACKAGE_INIT_REG_YES(asrt)   HDassert(H5_PACKAGE_INIT_VAR(pkg));
#define H5_CHECK_PACKAGE_INIT_REG_NO(asrt)
#define H5_CHECK_PACKAGE_INIT_INIT_YES(asrt)
#define H5_CHECK_PACKAGE_INIT_INIT_NO(asrt)
#define H5_CHECK_PACKAGE_INIT(pkg, pkg_init, init)  H5_GLUE4(H5_CHECK_PACKAGE_INIT_, init, _, pkg_init)(pkg)

/* Macros to initialize package, if a package initialization routine is defined */
#define H5_PKG_YES_INIT(pkg)                                                  \
    if(!H5_PACKAGE_INIT_VAR(pkg) && !H5_TERM_GLOBAL) {                        \
        H5_PACKAGE_INIT_VAR(pkg) = TRUE;                                      \
        if(H5_PACKAGE_INIT_FUNC(pkg)() < 0) {                                 \
            H5_PACKAGE_INIT_VAR(pkg) = FALSE;                                 \
            /* (Can't use H5E_THROW here) */                                  \
            H5E_PRINTF(H5E_CANTINIT, "interface initialization failed");      \
            ret_value = fail_value;                                           \
            goto func_init_failed;                                            \
        } /* end if */                                                        \
    } /* end if */
#define H5_PKG_NO_INIT(pkg)                                                   \
    if(!H5_PACKAGE_INIT_VAR(pkg) && !H5_TERM_GLOBAL)                          \
        H5_PACKAGE_INIT_VAR(pkg) = TRUE;
#define H5_PKG_INIT(pkg_init, pkg)      H5_GLUE3(H5_PKG_, pkg_init, _INIT)(pkg)

/* Macros to declare package initialization function, if a package initialization routine is defined */
#ifdef H5_PKG_SINGLE_SOURCE
#define H5_PKG_DECLARE_YES_FUNC(pkg) static herr_t H5_PACKAGE_INIT_FUNC(pkg)(void);
#else  /* H5_PKG_SINGLE_SOURCE */
#define H5_PKG_DECLARE_YES_FUNC(pkg) extern herr_t H5_PACKAGE_INIT_FUNC(pkg)(void);
#endif /* H5_PKG_SINGLE_SOURCE */
#define H5_PKG_DECLARE_NO_FUNC(pkg)

/* Declare package initialization symbols (if in a package) */
#ifdef H5_PKG_SINGLE_SOURCE
#define H5_PKG_DECLARE_VAR(pkg)         static hbool_t H5_PACKAGE_INIT_VAR(pkg);
#else  /* H5_PKG_SINGLE_SOURCE */
#define H5_PKG_DECLARE_VAR(pkg)         extern hbool_t H5_PACKAGE_INIT_VAR(pkg);
#endif  /* H5_PKG_SINGLE_SOURCE */
#define H5_PKG_DECLARE_FUNC(pkg_init, pkg) H5_GLUE3(H5_PKG_DECLARE_, pkg_init, _FUNC)(pkg)
#ifdef H5_MY_PKG
H5_PKG_DECLARE_VAR(H5_MY_PKG)
H5_PKG_DECLARE_FUNC(H5_MY_PKG_INIT, H5_MY_PKG)
#endif /* H5_MY_PKG */

/* API re-entrance variable */
extern hbool_t H5_api_entered_g;    /* Has library already been entered through API? */

/* Macros for entering different scopes of routines */
#define H5_PACKAGE_ENTER(pkg, pkg_init, init)                                 \
    FUNC_ENTER_CHECK_NAME(H5_IS_PKG(FUNC))                                    \
                                                                              \
    /* The library should be initialized already */                           \
    HDassert(H5_INIT_GLOBAL);                                                 \
                                                                              \
    /* This interface should be initialized already */                        \
    /* (except for package initialization routines :-) */                     \
    H5_CHECK_PACKAGE_INIT(pkg, pkg_init, init)                                \
                                                                              \
    /* Push the name of this function on the function stack */                \
    H5_PUSH_FUNC                                                              \
                                                                              \
    /* Enter scope for this type of function */                               \
    {

#define H5_PRIVATE_ENTER(pkg, pkg_init)                                       \
    FUNC_ENTER_CHECK_NAME(H5_IS_PRIV(FUNC))                                   \
                                                                              \
    /* The library should be initialized already */                           \
    HDassert(H5_INIT_GLOBAL);                                                 \
                                                                              \
    /* Initialize this interface if desired */                                \
    H5_PKG_INIT(pkg_init, pkg)                                                \
                                                                              \
    /* Push the name of this function on the function stack */                \
    H5_PUSH_FUNC                                                              \
                                                                              \
    /* Enter scope for this type of function */                               \
    {{

#define H5_PUBLIC_ENTER(pkg, pkg_init)                                        \
    FUNC_ENTER_API_VARS                                                       \
    FUNC_ENTER_API_THREADSAFE;                                                \
    FUNC_ENTER_CHECK_NAME(H5_IS_PUB(FUNC))                                    \
                                                                              \
    /* Clear thread error stack when entering public functions */             \
    H5E_clear_stack(NULL);                                                    \
                                                                              \
    /* Initialize the library or bust */                                      \
    if(!H5_INIT_GLOBAL && !H5_TERM_GLOBAL) {                                  \
        H5_INIT_GLOBAL = TRUE;                                                \
        if(H5_init_library() < 0) {                                           \
            /* (Can't use H5E_THROW here) */                                  \
            H5E_PRINTF(H5E_CANTINIT, "interface initialization failed");      \
            ret_value = fail_value;                                           \
            goto func_init_failed;                                            \
        } /* end if */                                                        \
    } /* end if */                                                            \
                                                                              \
    /* Initialize this interface if desired */                                \
    H5_PKG_INIT(pkg_init, pkg)                                                \
                                                                              \
    /* Check for re-entering API routine */                                   \
    HDassert(!H5_api_entered_g);                                              \
    H5_api_entered_g = TRUE;                                                  \
                                                                              \
    /* Start logging MPI's MPE information */                                 \
    BEGIN_MPE_LOG                                                             \
                                                                              \
    /* Push the name of this function on the function stack */                \
    H5_PUSH_FUNC                                                              \
                                                                              \
    /* Enter scope for this type of function */                               \
    {{{

/* Macros for substituting the package name */
#define FUNC_ENT_STATIC(pkg, pkg_init)  H5_PACKAGE_ENTER(pkg, pkg_init, REG)
#define FUNC_ENT_PKGINIT(pkg, pkg_init)  H5_PACKAGE_ENTER(pkg, pkg_init, INIT)
#define FUNC_ENT_PKG(pkg, pkg_init)    H5_PACKAGE_ENTER(pkg, pkg_init, REG)
#define FUNC_ENT_PRIV(pkg, pkg_init)    H5_PRIVATE_ENTER(pkg, pkg_init)
#define FUNC_ENT_PUB(pkg, pkg_init)    H5_PUBLIC_ENTER(pkg, pkg_init)

/* Macros for substituting a function prefix */
#define FUNC_PREFIX_STATIC      static
#define FUNC_PREFIX_PKGINIT
#define FUNC_PREFIX_PKG
#define FUNC_PREFIX_PRIV
#define FUNC_PREFIX_PUB

/* Macros for declaring error variables */
/* Function can detect errors and has a specific error return value */
#define FUNC_ERR_VAR_ERR(ret_typ, err)                                        \
    hbool_t past_catch = FALSE;                                               \
    ret_typ fail_value = err;
/* Function can detect errors but cannot return an error value (Cleanup only) */
#define FUNC_ERR_VAR_ERRCATCH(ret_typ, err)                                   \
    hbool_t past_catch = FALSE;
/* Function has no need to detect or clean up from errors */
#define FUNC_ERR_VAR_NOERR(ret_typ, err)

/* Use this macro when entering all functions */
#define BEGIN_FUNC(scope, use_err, ret_typ, ret_init, err, func)              \
H5_GLUE(FUNC_PREFIX_, scope)                                                  \
ret_typ                                                                       \
func                                                                          \
/* Open function */                                                           \
{                                                                             \
    ret_typ ret_value = ret_init;                                             \
    H5_GLUE(FUNC_ERR_VAR_, use_err)(ret_typ, err)                             \
    H5_GLUE(FUNC_ENT_, scope)(H5_MY_PKG, H5_MY_PKG_INIT)

/* Use this macro when entering functions that have no return value */
#define BEGIN_FUNC_VOID(scope, use_err, func)                               \
H5_GLUE(FUNC_PREFIX_, scope)                                                \
void                                                                        \
func                                                                        \
/* Open function */                                                         \
{                                                                           \
    H5_GLUE(FUNC_ERR_VAR_, use_err)(void, -, -)                             \
    H5_GLUE(FUNC_ENT_, scope)

/* Macros for label when a function initialization can fail */
#define H5_PRIV_YES_FUNC_INIT_FAILED func_init_failed:
#define H5_PRIV_NO_FUNC_INIT_FAILED
#define H5_PRIV_FUNC_INIT_FAILED(pkg_init) H5_GLUE3(H5_PRIV_, pkg_init, _FUNC_INIT_FAILED)

/* Macros for leaving different scopes of routines */
#define FUNC_LEAVE_PKGINIT                                                    \
    /* Leave scope for this type of function */                               \
    }                                                                         \
                                                                              \
    /* Pop the name of this function off the function stack */                \
    H5_POP_FUNC

#define FUNC_LEAVE_STATIC                                                     \
    /* Leave scope for this type of function */                               \
    }                                                                         \
                                                                              \
    /* Pop the name of this function off the function stack */                \
    H5_POP_FUNC

#define FUNC_LEAVE_PKG                                                        \
    /* Leave scope for this type of function */                               \
    }                                                                         \
                                                                              \
    /* Pop the name of this function off the function stack */                \
    H5_POP_FUNC

#define FUNC_LEAVE_PRIV                                                       \
    /* Leave scope for this type of function */                               \
    }}                                                                        \
                                                                              \
    /* Label for errors during FUNC_ENTER */                                  \
    H5_PRIV_FUNC_INIT_FAILED(H5_MY_PKG_INIT)                                  \
                                                                              \
    /* Pop the name of this function off the function stack */                \
    H5_POP_FUNC

#define FUNC_LEAVE_PUB                                                        \
    /* Leave scope for this type of function */                               \
    }}}                                                                       \
                                                                              \
    /* Label for errors during FUNC_ENTER */                                  \
func_init_failed:                                                             \
                                                                              \
    /* Dump error stack if an error occurred during API routine */            \
    if(ret_value == fail_value)                                               \
        (void)H5E_dump_api_stack(TRUE);                                       \
                                                                              \
    /* Finish the API tracing info */                                         \
    H5TRACE_RETURN(ret_value);                                                \
                                                                              \
    /* Pop the name of this function off the function stack */                \
    H5_POP_FUNC                                                               \
                                                                              \
    /* Finish the MPE tracing info */                                         \
    FINISH_MPE_LOG                                                            \
                                                                              \
    /* Check for leaving API routine */                                       \
    HDassert(H5_api_entered_g);                                               \
    H5_api_entered_g = FALSE;                                                 \
                                                                              \
    /* Release thread-safety semaphore */                                     \
    FUNC_LEAVE_API_THREADSAFE

/* Use this macro when leaving all functions */
#define END_FUNC(scope)                                                       \
    /* Scope-specific function conclusion */                                  \
    H5_GLUE(FUNC_LEAVE_, scope)                                               \
                                                                              \
    /* Leave routine */                                                       \
    return(ret_value);                                                        \
                                                                              \
    /* Close Function */                                                      \
}

/* Use this macro when leaving void functions */
#define END_FUNC_VOID(scope)                                                \
    /* Scope-specific function conclusion */                                \
    H5_GLUE(FUNC_LEAVE_, scope)                                             \
                                                                            \
    /* Leave routine */                                                     \
    return;                                                                 \
                                                                            \
    /* Close Function */                                                    \
}

/* Macro to begin/end tagging (when FUNC_ENTER_*TAG macros are insufficient).
 * Make sure to use HGOTO_ERROR_TAG and HGOTO_DONE_TAG between these macros! */
#define H5_BEGIN_TAG(tag) {                                                 \
    haddr_t prv_tag = HADDR_UNDEF;                                          \
    H5AC_tag(tag, &prv_tag);                                                \

#define H5_END_TAG                                                          \
    H5AC_tag(prv_tag, NULL);                                                \
}

/* Compile-time "assert" macro */
#define HDcompile_assert(e)     ((void)sizeof(char[ !!(e) ? 1 : -1]))
/* Variants that are correct, but generate compile-time warnings in some circumstances:
  #define HDcompile_assert(e)     do { enum { compile_assert__ = 1 / (e) }; } while(0)
  #define HDcompile_assert(e)     do { typedef struct { unsigned int b: (e); } x; } while(0)
*/

/* Private functions, not part of the publicly documented API */
H5_DLL herr_t H5_init_library(void);
H5_DLL void H5_term_library(void);

/* Functions to terminate interfaces */
H5_DLL int H5A_term_package(void);
H5_DLL int H5A_top_term_package(void);
H5_DLL int H5AC_term_package(void);
H5_DLL int H5CX_term_package(void);
H5_DLL int H5D_term_package(void);
H5_DLL int H5D_top_term_package(void);
H5_DLL int H5E_term_package(void);
H5_DLL int H5F_term_package(void);
H5_DLL int H5FD_term_package(void);
H5_DLL int H5FL_term_package(void);
H5_DLL int H5FS_term_package(void);
H5_DLL int H5G_term_package(void);
H5_DLL int H5G_top_term_package(void);
H5_DLL int H5I_term_package(void);
H5_DLL int H5L_term_package(void);
H5_DLL int H5P_term_package(void);
H5_DLL int H5PL_term_package(void);
H5_DLL int H5R_term_package(void);
H5_DLL int H5R_top_term_package(void);
H5_DLL int H5S_term_package(void);
H5_DLL int H5S_top_term_package(void);
H5_DLL int H5SL_term_package(void);
H5_DLL int H5T_term_package(void);
H5_DLL int H5T_top_term_package(void);
H5_DLL int H5Z_term_package(void);

/* Checksum functions */
H5_DLL uint32_t H5_checksum_fletcher32(const void *data, size_t len);
H5_DLL uint32_t H5_checksum_crc(const void *data, size_t len);
H5_DLL uint32_t H5_checksum_lookup3(const void *data, size_t len, uint32_t initval);
H5_DLL uint32_t H5_checksum_metadata(const void *data, size_t len, uint32_t initval);
H5_DLL uint32_t H5_hash_string(const char *str);

/* Time related routines */
H5_DLL time_t H5_make_time(struct tm *tm);
H5_DLL void H5_nanosleep(uint64_t nanosec);
H5_DLL double H5_get_time(void);

/* Functions for building paths, etc. */
H5_DLL herr_t   H5_build_extpath(const char *name, char **extpath /*out*/);
H5_DLL herr_t   H5_combine_path(const char *path1, const char *path2, char **full_name /*out*/);

/* Functions for debugging */
H5_DLL herr_t H5_buffer_dump(FILE *stream, int indent, const uint8_t *buf,
    const uint8_t *marker, size_t buf_offset, size_t buf_size);

#endif /* _H5private_H */

