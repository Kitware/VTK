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

/*
 * Purpose: This file is included by all HDF5 library source files to
 *          define common things which are not defined in the HDF5 API.
 *          The configuration constants like H5_HAVE_UNISTD_H etc. are
 *          defined in H5config.h which is included by H5public.h.
 */

#ifndef H5private_H
#define H5private_H

/* Define __STDC_WANT_IEC_60559_TYPES_EXT__ for _FloatN support, if available.
 * Do that before including any other headers in case they include <float.h>
 * implicitly. */
#define __STDC_WANT_IEC_60559_TYPES_EXT__

#include "H5public.h" /* Include Public Definitions    */

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <fenv.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <float.h>
#include <math.h>

#ifdef H5_HAVE_COMPLEX_NUMBERS
#include <complex.h>
#endif

/* POSIX headers */
#ifdef H5_HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef H5_HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef H5_HAVE_PWD_H
#include <pwd.h>
#endif
#ifdef H5_HAVE_WAITPID
#include <sys/wait.h>
#endif

/* Include the threading header, if necessary */
#if defined(H5_HAVE_THREADS)
/* C11 threads */
#if defined(H5_HAVE_THREADS_H)
#include <threads.h>
#endif

/* Pthreads */
#if defined(H5_HAVE_PTHREAD_H)
#include <pthread.h>
#endif
#endif

/* C11 atomics */
#if defined(H5_HAVE_STDATOMIC_H) && !defined(__cplusplus)
#include <stdatomic.h>
#endif

/*
 * The `struct stat' data type for stat() and fstat(). This is a POSIX file
 * but often appears on non-POSIX systems also.  The `struct stat' is required
 * for HDF5 to compile, although only a few fields are actually used.
 */
#ifdef H5_HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

/*
 * flock() in sys/file.h is used for the implementation of file locking.
 */
#if defined(H5_HAVE_FLOCK) && defined(H5_HAVE_SYS_FILE_H)
#include <sys/file.h>
#endif

/*
 * Resource usage is not Posix.1 but HDF5 uses it anyway for some performance
 * and debugging code if available.
 */
#ifdef H5_HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

/*
 * Unix ioctls.   These are used by h5ls (and perhaps others) to determine a
 * reasonable output width.
 */
#ifdef H5_HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
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
#define H5_DEFAULT_VFD_NAME "sec2"

/* Define the default VOL connector */
#define H5_DEFAULT_VOL H5VL_NATIVE_conn_g

#ifdef H5_HAVE_WIN32_API

/* The following two defines must be before any windows headers are included */
#define WIN32_LEAN_AND_MEAN /* Exclude rarely-used stuff from Windows headers */
#define NOGDI               /* Exclude Graphic Display Interface macros */

/* InitOnceExecuteOnce() requires 0x0600 to work on MinGW w/ Win32 threads */
#if defined(H5_HAVE_MINGW) && defined(H5_HAVE_THREADS)
#if !defined(_WIN32_WINNT) || (_WIN32_WINNT < 0x0600)
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#endif

#include <windows.h>

#include <direct.h>   /* For _getcwd() */
#include <io.h>       /* POSIX I/O */
#include <winsock2.h> /* For GetUserName() */
#include <shlwapi.h>  /* For StrStrIA */

#endif /*H5_HAVE_WIN32_API*/

/* Macros for suppressing warnings */
#include "H5warnings.h"

#ifndef F_OK
#define F_OK 00
#define W_OK 02
#define R_OK 04
#endif

/* uthash is an external, header-only hash table implementation.
 *
 * We include the file directly in src/ and #define a few functions
 * to use our internal memory calls.
 */
#define uthash_malloc(sz)    H5MM_malloc(sz)
#define uthash_free(ptr, sz) H5MM_free(ptr) /* Ignoring sz is intentional */
#define HASH_NONFATAL_OOM    1              /* Don't abort() on out-of-memory */
#include "uthash.h"

/*
 * Does the compiler support the __attribute__(()) syntax?  It's no
 * big deal if we don't.
 *
 * When using H5_ATTR_FALLTHROUGH, you should also include a comment that
 * says FALLTHROUGH to reduce warnings on compilers that don't use
 * attributes but do respect fall-through comments.
 *
 * H5_ATTR_CONST is redefined in tools/h5repack/dynlib_rpk.c to quiet
 * gcc warnings (it has to use the public API and can't include this
 * file). Be sure to update that file if the #ifdefs change here.
 */
/* clang-format off */
#if defined(H5_HAVE_ATTRIBUTE)
#   define H5_ATTR_FORMAT(X, Y, Z) __attribute__((format(X, Y, Z)))
#   define H5_ATTR_UNUSED          __attribute__((unused))

#   ifdef H5_HAVE_PARALLEL
#       define H5_ATTR_PARALLEL_UNUSED __attribute__((unused))
#       define H5_ATTR_PARALLEL_USED   /*void*/
#   else
#       define H5_ATTR_PARALLEL_UNUSED /*void*/
#       define H5_ATTR_PARALLEL_USED   __attribute__((unused))
#   endif

#   ifdef H5_NO_DEPRECATED_SYMBOLS
#       define H5_ATTR_DEPRECATED_USED H5_ATTR_UNUSED
#   else
#       define H5_ATTR_DEPRECATED_USED /*void*/
#   endif

#   ifndef NDEBUG
#       define H5_ATTR_NDEBUG_UNUSED /*void*/
#   else
#       define H5_ATTR_NDEBUG_UNUSED H5_ATTR_UNUSED
#   endif

#   define H5_ATTR_NORETURN __attribute__((noreturn))
#   define H5_ATTR_CONST    __attribute__((const))
#   define H5_ATTR_PURE     __attribute__((pure))

#   if defined(__clang__) || defined(__GNUC__) && __GNUC__ >= 7 && !defined(__INTEL_COMPILER)
#       define H5_ATTR_FALLTHROUGH __attribute__((fallthrough));
#   else
#       define H5_ATTR_FALLTHROUGH /* FALLTHROUGH */
#   endif

#  if defined(__GNUC__) && !defined(__INTEL_COMPILER)
#       define H5_ATTR_MALLOC __attribute__((malloc))
#  else
#       define H5_ATTR_MALLOC /*void*/
#  endif

/* Turns off optimizations for a function. Goes after the return type.
 * Not generally needed in the library, but ancient versions of clang
 * (7.3.3, possibly others) have trouble with some of the onion VFD decode
 * functions and need the optimizer turned off. This macro can go away when
 * we figure out what's going on and can engineer another solution.
 */
#  if defined(__clang__)
#       define H5_ATTR_NO_OPTIMIZE __attribute__((optnone))
#  else
#       define H5_ATTR_NO_OPTIMIZE /*void*/
#  endif

/* Enable thread-safety annotations when built with clang */
#  if defined(__clang__)
#       define H5_ATTR_THREAD_ANNOT(X) __attribute__((X))
#  else
#       define H5_ATTR_THREAD_ANNOT(X) /*void*/
#  endif

#else
#   define H5_ATTR_FORMAT(X, Y, Z) /*void*/
#   define H5_ATTR_UNUSED          /*void*/
#   define H5_ATTR_NDEBUG_UNUSED   /*void*/
#   define H5_ATTR_DEPRECATED_USED /*void*/
#   define H5_ATTR_PARALLEL_UNUSED /*void*/
#   define H5_ATTR_PARALLEL_USED   /*void*/
#   define H5_ATTR_NORETURN        /*void*/
#   define H5_ATTR_CONST           /*void*/
#   define H5_ATTR_PURE            /*void*/
#   define H5_ATTR_FALLTHROUGH     /*void*/
#   define H5_ATTR_MALLOC          /*void*/
#   define H5_ATTR_NO_OPTIMIZE     /*void*/
#   define H5_ATTR_THREAD_ANNOT(X) /*void*/
#endif
/* clang-format on */

/*
 * Networking headers used by the mirror VFD and related tests and utilities.
 */
#ifdef H5_HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef H5_HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef H5_HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef H5_HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

/*
 * Status return values for the `herr_t' type.
 * Since some unix/c routines use 0 and -1 (or more precisely, non-negative
 * vs. negative) as their return code, and some assumption had been made in
 * the code about that, it is important to keep these constants the same
 * values.  When checking the success or failure of an integer-valued
 * function, remember to compare against zero and not one of these two
 * values.
 */
#define SUCCEED 0
#define FAIL    (-1)

/* number of members in an array */
#ifndef NELMTS
#define NELMTS(X) (sizeof(X) / sizeof(X[0]))
#endif

/* minimum of two, three, or four values */
#undef MIN
#define MIN(a, b)        (((a) < (b)) ? (a) : (b))
#define MIN2(a, b)       MIN(a, b)
#define MIN3(a, b, c)    MIN(a, MIN(b, c))
#define MIN4(a, b, c, d) MIN(MIN(a, b), MIN(c, d))

/* maximum of two, three, or four values */
#undef MAX
#define MAX(a, b)        (((a) > (b)) ? (a) : (b))
#define MAX2(a, b)       MAX(a, b)
#define MAX3(a, b, c)    MAX(a, MAX(b, c))
#define MAX4(a, b, c, d) MAX(MAX(a, b), MAX(c, d))

/* limit the middle value to be within a range (inclusive) */
#define RANGE(LO, X, HI) MAX(LO, MIN(X, HI))

/* Macro for checking if two ranges overlap one another */
/*
 * Check for the inverse of whether the ranges are disjoint.  If they are
 * disjoint, then the low bound of one of the ranges must be greater than the
 * high bound of the other.
 */
/* (Assumes that low & high bounds are _inclusive_) */
#define H5_RANGE_OVERLAP(L1, H1, L2, H2) (!((L1) > (H2) || (L2) > (H1)))

/* absolute value */
#ifndef ABS
#define ABS(a) (((a) >= 0) ? (a) : -(a))
#endif

/* test for number that is a power of 2 */
/* (from: http://graphics.stanford.edu/~seander/bithacks.html#DetermineIfPowerOf2) */
#define POWER_OF_TWO(n) (!(n & (n - 1)) && n)

/* Raise an integer to a power of 2 */
#define H5_EXP2(n) (1 << (n))

/* Check if a read of size bytes starting at ptr would overflow past
 * the last valid byte, pointed to by buffer_end. Note that 'size'
 * is expected to be of type size_t. Providing values of other
 * datatypes may cause warnings due to the comparison against
 * PTRDIFF_MAX and comparison of < 0 after conversion to ptrdiff_t.
 * For the time being, these can be suppressed with
 * H5_WARN_USELESS_COMPARISON_(OFF|ON).
 */
/* clang-format off */
#define H5_IS_BUFFER_OVERFLOW(ptr, size, buffer_end)                                                         \
    (                                                                                                        \
      /* Trivial case */                                                                                     \
      ((size) != 0) &&                                                                                       \
      (                                                                                                      \
        /* Bad precondition */                                                                               \
        ((ptr) > (buffer_end)) ||                                                                            \
        /* Account for (likely unintentional) negative 'size' */                                             \
        (((size_t)(size) <= PTRDIFF_MAX) && ((ptrdiff_t)(size) < 0)) ||                                      \
        /* Typical overflow */                                                                               \
        ((size_t)(size) > (size_t)((((const uint8_t *)buffer_end) - ((const uint8_t *)ptr)) + 1))            \
      )                                                                                                      \
    )
/* clang-format on */

/* Variant of H5_IS_BUFFER_OVERFLOW, used with functions such as H5Tdecode()
 * that don't take a size parameter, where we need to skip the bounds checks.
 *
 * This is a separate macro since we don't want to inflict that behavior on
 * the entire library.
 */
#define H5_IS_KNOWN_BUFFER_OVERFLOW(skip, ptr, size, buffer_end)                                             \
    (skip ? false : H5_IS_BUFFER_OVERFLOW(ptr, size, buffer_end))

/*
 * The max value for ssize_t.
 *
 * Only needed where ssize_t isn't a thing (e.g., Windows)
 */
#ifndef SSIZE_MAX
#define SSIZE_MAX SSIZE_T_MAX
#endif

/*
 * Maximum & minimum values for HDF5 typedefs.
 */
#define HSIZET_MAX  ((hsize_t)ULLONG_MAX)
#define HSSIZET_MAX ((hssize_t)LLONG_MAX)
#define HSSIZET_MIN (~(HSSIZET_MAX))

#ifdef H5_HAVE_PARALLEL

/* Define a type for safely sending size_t values with MPI */
#if SIZE_MAX == UCHAR_MAX
#define H5_SIZE_T_AS_MPI_TYPE MPI_UNSIGNED_CHAR
#elif SIZE_MAX == USHRT_MAX
#define H5_SIZE_T_AS_MPI_TYPE MPI_UNSIGNED_SHORT
#elif SIZE_MAX == UINT_MAX
#define H5_SIZE_T_AS_MPI_TYPE MPI_UNSIGNED
#elif SIZE_MAX == ULONG_MAX
#define H5_SIZE_T_AS_MPI_TYPE MPI_UNSIGNED_LONG
#elif SIZE_MAX == ULLONG_MAX
#define H5_SIZE_T_AS_MPI_TYPE MPI_UNSIGNED_LONG_LONG
#else
#error "no suitable MPI type for size_t"
#endif

#endif /* H5_HAVE_PARALLEL */

/*
 * Types and max sizes for POSIX I/O.
 * OS X (Darwin) is odd since the max I/O size does not match the types.
 */
#if defined(H5_HAVE_WIN32_API)
#define h5_posix_io_t         unsigned int
#define h5_posix_io_ret_t     int
#define H5_POSIX_MAX_IO_BYTES INT_MAX
#elif defined(H5_HAVE_DARWIN)
#define h5_posix_io_t         size_t
#define h5_posix_io_ret_t     ssize_t
#define H5_POSIX_MAX_IO_BYTES INT_MAX
#else
#define h5_posix_io_t         size_t
#define h5_posix_io_ret_t     ssize_t
#define H5_POSIX_MAX_IO_BYTES SSIZE_MAX
#endif

/* POSIX I/O mode used as the third parameter to open/_open
 * when creating a new file (O_CREAT is set).
 */
#if defined(H5_HAVE_WIN32_API)
#define H5_POSIX_CREATE_MODE_RW (_S_IREAD | _S_IWRITE)
#else
#define H5_POSIX_CREATE_MODE_RW 0666
#endif

/* Represents an empty asynchronous request handle.
 * Used in the VOL code.
 */
#define H5_REQUEST_NULL NULL

/* clang-format off */
/* Address-related macros */
#define H5_addr_overflow(X,Z)    (HADDR_UNDEF == (X) ||                     \
                                  HADDR_UNDEF == (X) + (haddr_t)(Z) ||      \
                                  (X) + (haddr_t)(Z) < (X))
#define H5_addr_defined(X)       ((X) != HADDR_UNDEF)
/* The H5_addr_eq() macro guarantees that Y is not HADDR_UNDEF by making
 * certain that X is not HADDR_UNDEF and then checking that X equals Y
 */
#define H5_addr_eq(X,Y)          ((X) != HADDR_UNDEF && (X) == (Y))
#define H5_addr_ne(X,Y)          (!H5_addr_eq((X),(Y)))
#define H5_addr_lt(X,Y)          ((X) != HADDR_UNDEF &&                     \
                                  (Y) != HADDR_UNDEF &&                     \
                                  (X) < (Y))
#define H5_addr_le(X,Y)          ((X) != HADDR_UNDEF &&                     \
                                  (Y) != HADDR_UNDEF &&                     \
                                  (X) <= (Y))
#define H5_addr_gt(X,Y)          ((X) != HADDR_UNDEF &&                     \
                                  (Y) != HADDR_UNDEF &&                     \
                                  (X) > (Y))
#define H5_addr_ge(X,Y)          ((X) != HADDR_UNDEF &&                     \
                                  (Y) != HADDR_UNDEF &&                     \
                                  (X) >= (Y))
#define H5_addr_cmp(X,Y)         (H5_addr_eq((X), (Y)) ? 0 :                \
                                 (H5_addr_lt((X), (Y)) ? -1 : 1))
#define H5_addr_overlap(O1,L1,O2,L2) H5_RANGE_OVERLAP(O1, ((O1)+(L1)-1), O2, ((O2)+(L2)-1))
/* clang-format on */

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
#define H5_FLT_ABS_EQUAL(X, Y)  (fabsf((X) - (Y)) < FLT_EPSILON)
#define H5_DBL_ABS_EQUAL(X, Y)  (fabs((X) - (Y)) < DBL_EPSILON)
#define H5_LDBL_ABS_EQUAL(X, Y) (fabsl((X) - (Y)) < LDBL_EPSILON)

#ifdef H5_HAVE__FLOAT16
#ifdef H5_HAVE_FABSF16
#define H5_FLT16_ABS_EQUAL(X, Y) (fabsf16((X) - (Y)) < FLT16_EPSILON)
#else
#define H5_FLT16_ABS_EQUAL(X, Y) H5_FLT_ABS_EQUAL((float)X, (float)Y)
#endif
#endif

#define H5_FLT_REL_EQUAL(X, Y, M)  (fabsf(((Y) - (X)) / (X)) < (M))
#define H5_DBL_REL_EQUAL(X, Y, M)  (fabs(((Y) - (X)) / (X)) < (M))
#define H5_LDBL_REL_EQUAL(X, Y, M) (fabsl(((Y) - (X)) / (X)) < (M))

#ifdef H5_HAVE__FLOAT16
#ifdef H5_HAVE_FABSF16
#define H5_FLT16_REL_EQUAL(X, Y, M) (fabsf16(((Y) - (X)) / (X)) < (M))
#else
#define H5_FLT16_REL_EQUAL(X, Y, M) H5_FLT_REL_EQUAL((float)X, (float)Y, M)
#endif
#endif

#ifndef H5_HAVE_FLOCK
/* flock() operations. Used in the source so we have to define them when
 * the call is not available (e.g.: Windows). These should NOT be used
 * with system-provided flock() calls since the values will come from the
 * header file.
 */
#define LOCK_SH 0x01
#define LOCK_EX 0x02
#define LOCK_NB 0x04
#define LOCK_UN 0x08
#endif /* H5_HAVE_FLOCK */

/* Private typedefs */

/* Union for const/non-const pointer for use by functions that manipulate
 * pointers but do not write to their targets or return pointers to const
 * specified locations.  Also used for I/O functions that work for read and
 * write - these functions are expected to never write to these locations in the
 * write case.  This helps us avoid compiler warnings. */
typedef union {
    void       *vp;
    const void *cvp;
} H5_flexible_const_ptr_t;

/* If necessary, create a typedef for library usage of the
 * _Float16 type to avoid issues when compiling the library
 * with the -pedantic flag or similar where we get warnings
 * about _Float16 not being an ISO C type.
 *
 * Due to the inclusion of H5private.h from the C++ wrappers,
 * this typedef creation must be avoided when __cplusplus is
 * defined to avoid build failures on ARM64 Macs. GCC and
 * Clang either do not currently provide a _Float16 type for
 * C++ on ARM64, or may need an additional compile-time flag.
 */
#if defined(H5_HAVE__FLOAT16) && !defined(__cplusplus)
#if defined(__GNUC__)
__extension__ typedef _Float16 H5__Float16;
#else
typedef _Float16 H5__Float16;
#endif
#endif

/* Function pointer typedef for qsort */
typedef int (*H5_sort_func_cb_t)(const void *, const void *);

/* Typedefs and functions for timing certain parts of the library. */
#include "H5timer.h"

/* Substitute for strcasestr() when that doesn't exist on the platform */
H5_DLL char *H5_strcasestr(const char *haystack, const char *needle);

/* Depth of object copy */
typedef enum {
    H5_COPY_SHALLOW, /* Shallow copy from source to destination, just copy field pointers */
    H5_COPY_DEEP     /* Deep copy from source to destination, including duplicating fields pointed to */
} H5_copy_depth_t;

/* Common object copying udata (right now only used for groups and datasets) */
typedef struct H5O_copy_file_ud_common_t {
    struct H5O_pline_t *src_pline; /* Copy of filter pipeline for object */
} H5O_copy_file_ud_common_t;

/* Unique object "position" */
typedef struct {
    unsigned long fileno; /* The unique identifier for the file of the object */
    haddr_t       addr;   /* The unique address of the object's header in that file */
} H5_obj_t;

#define H5_SIZEOF_H5_STAT_SIZE_T H5_SIZEOF_OFF_T

/* Put all Windows-specific definitions in H5win32defs.h so we
 * can (mostly) assume a POSIX platform. Not all of the POSIX calls
 * will have a Windows equivalent so some #ifdef protection is still
 * necessary (e.g., fork()).
 */
#include "H5win32defs.h"

/* Platform-independent definition for struct stat. For Win32, see
 * H5win32defs.h.
 */
#ifndef H5_HAVE_WIN32_API
typedef struct stat h5_stat_t;
#endif

/* Platform-independent definition for complex number types. For Win32,
 * see H5win32defs.h. Note that these types cannot be used for casts
 * (other than pointer casts) anywhere in the library that will be
 * compiled by MSVC. MSVC will fail to compile since it uses structure
 * types for complex numbers and casts can't be made between structure
 * types and other types.
 */
#if defined(H5_HAVE_COMPLEX_NUMBERS) && defined(H5_HAVE_C99_COMPLEX_NUMBERS)
typedef float _Complex H5_float_complex;
typedef double _Complex H5_double_complex;
typedef long double _Complex H5_ldouble_complex;
#endif

/* __int64 is the correct type for the st_size field of the _stati64
 * struct on Windows (MSDN isn't very clear about this). POSIX systems use
 * off_t. Both of these are typedef'd to HDoff_t in H5public.h.
 */
typedef HDoff_t h5_stat_size_t;

/* Redefinions of some POSIX and C functions (mainly to deal with Windows) */

#ifndef HDaccess
#define HDaccess(F, M) access(F, M)
#endif
#ifndef HDchdir
#define HDchdir(S) chdir(S)
#endif
#ifndef HDclose
#define HDclose(F) close(F)
#endif
#ifndef HDclosedir
#define HDclosedir(D) closedir(D)
#endif
#ifndef HDcreat
#define HDcreat(S, M) creat(S, M)
#endif
#ifndef HDfdopen
#define HDfdopen(N, S) fdopen(N, S)
#endif
#ifndef HDfileno
#define HDfileno(F) fileno(F)
#endif

/* Since flock is so prevalent, always build these functions
 * when possible to avoid them becoming dead code.
 */
#ifdef H5_HAVE_FCNTL
H5_DLL int Pflock(int fd, int operation);
#endif
H5_DLL H5_ATTR_CONST int Nflock(int fd, int operation);

#ifndef HDflock
/* NOTE: flock(2) is not present on all POSIX systems.
 * If it is not present, we try a flock() equivalent based on
 * fcntl(2), then fall back to a function that always succeeds
 * if it is not present at all (Windows uses a separate Wflock()
 * function).
 */
#if defined(H5_HAVE_FLOCK)
#define HDflock(F, L) flock(F, L)
#elif defined(H5_HAVE_FCNTL)
#define HDflock(F, L) Pflock(F, L)
#else
#define HDflock(F, L) Nflock(F, L)
#endif

#endif /* HDflock */

#if defined(H5_HAVE_WIN32_API) || defined(H5_HAVE_DARWIN) || (defined(__FreeBSD__) && __FreeBSD__ < 14)
H5_DLL herr_t HDqsort_context(void *base, size_t nel, size_t size,
                              int (*compar)(const void *, const void *, void *), void *arg);
#endif

#ifndef H5_HAVE_QSORT_REENTRANT
H5_DLL herr_t HDqsort_fallback(void *base, size_t nel, size_t size,
                               int (*compar)(const void *, const void *, void *), void *arg);
#endif

#ifndef HDfseek
#define HDfseek(F, O, W) fseeko(F, O, W)
#endif
#ifndef HDfstat
#define HDfstat(F, B) fstat(F, B)
#endif
#ifndef HDftell
#define HDftell(F) ftello(F)
#endif
#ifndef HDftruncate
#define HDftruncate(F, L) ftruncate(F, L)
#endif
#ifndef HDgetcwd
#define HDgetcwd(S, Z) getcwd(S, Z)
#endif
#ifndef HDgetdcwd
#define HDgetdcwd(D, S, Z) getcwd(S, Z)
#endif

/* Windows only - set to zero on other systems */
#ifndef HDgetdrive
#define HDgetdrive() 0
#endif

#ifndef HDgetpid
#define HDgetpid() getpid()
#endif
#ifndef HDgettimeofday
#define HDgettimeofday(S, P) gettimeofday(S, P)
#endif
#ifndef HDisatty
#define HDisatty(F) isatty(F)
#endif
#ifndef HDlseek
#define HDlseek(F, O, W) lseek(F, O, W)
#endif
#ifndef HDlstat
#define HDlstat(S, B) lstat(S, B)
#endif
#ifndef HDmkdir
#define HDmkdir(S, M) mkdir(S, M)
#endif
#ifndef HDnanosleep
#define HDnanosleep(N, O) nanosleep(N, O)
#endif
#ifndef HDopen
#define HDopen(F, ...) open(F, __VA_ARGS__)
#endif
#ifndef HDopendir
#define HDopendir(S) opendir(S)
#endif
#ifndef HDpread
#define HDpread(F, B, C, O) pread(F, B, C, O)
#endif
#ifndef HDpwrite
#define HDpwrite(F, B, C, O) pwrite(F, B, C, O)
#endif
#ifndef HDread
#define HDread(F, M, Z) read(F, M, Z)
#endif
#ifndef HDreaddir
#define HDreaddir(D) readdir(D)
#endif
#ifndef HDrealpath
#define HDrealpath(F1, F2) realpath(F1, F2)
#endif
#ifndef HDremove
#define HDremove(S) remove(S)
#endif
#ifndef HDrmdir
#define HDrmdir(S) rmdir(S)
#endif
#ifndef HDsetenv
#define HDsetenv(N, V, O) setenv(N, V, O)
#endif
#ifndef HDsetvbuf
#define HDsetvbuf(F, S, M, Z) setvbuf(F, S, M, Z)
#endif
#ifndef HDshutdown
#define HDshutdown(A, B) shutdown((A), (B))
#endif
#ifndef HDsigaction
#define HDsigaction(S, A, O) sigaction((S), (A), (O))
#endif
#ifndef HDsigemptyset
#define HDsigemptyset(S) sigemptyset(S)
#endif
#ifndef HDsleep
#define HDsleep(N) sleep(N)
#endif
#ifndef HDstat
#define HDstat(S, B) stat(S, B)
#endif
#ifndef HDstrcasestr
#if defined(H5_HAVE_STRCASESTR)
#define HDstrcasestr(X, Y) strcasestr(X, Y)
#else
#define HDstrcasestr(X, Y) H5_strcasestr(X, Y)
#endif
#endif
#ifndef HDstrcasecmp
#define HDstrcasecmp(X, Y) strcasecmp(X, Y)
#endif
#ifndef HDstrndup
#define HDstrndup(S, N) strndup(S, N)
#endif
#ifndef HDstrtok_r
#define HDstrtok_r(X, Y, Z) strtok_r(X, Y, Z)
#endif
#ifndef HDunlink
#define HDunlink(S) unlink(S)
#endif
#ifndef HDunsetenv
#define HDunsetenv(S) unsetenv(S)
#endif
#ifndef HDqsort_r
#ifdef H5_HAVE_QSORT_REENTRANT
#if defined(H5_HAVE_DARWIN) || (defined(__FreeBSD__) && __FreeBSD__ < 14)
/* Darwin and FreeBSD < 14 use BSD-style qsort_r with different signature/argument order */
#define HDqsort_r(B, N, S, C, A) HDqsort_context(B, N, S, C, A)
#else
/* Wrap native GNU qsort_r to vacuously return success */
#define HDqsort_r(B, N, S, C, A) (qsort_r(B, N, S, C, A), SUCCEED)
#endif
#else
/* No native qsort_r/qsort_s available - use fallback implementation */
#define HDqsort_r(B, N, S, C, A) HDqsort_fallback(B, N, S, C, A)
#endif
#endif
#ifndef HDvasprintf
#ifdef H5_HAVE_VASPRINTF
#define HDvasprintf(RET, FMT, A) vasprintf(RET, FMT, A)
#else
H5_DLL int HDvasprintf(char **bufp, const char *fmt, va_list _ap);
#endif
#endif

#ifndef HDwrite
#define HDwrite(F, M, Z) write(F, M, Z)
#endif

/* Simple macros to construct complex numbers. Necessary since MSVC's use
 * of structure types for complex numbers means that arithmetic operators
 * can't be used directly on variables of complex number types. These macros
 * abstract away the construction of complex numbers across platforms.
 *
 * Note that the use of _Complex_I means that an imaginary part of -0 may
 * be converted to +0. If the minimum required C standard is moved to C11
 * or later, these can be simplified to the standard CMPLXF/CMPLX/CMPLXL
 * macros, which don't have this problem.
 */
#ifdef H5_HAVE_COMPLEX_NUMBERS
#ifndef H5_CMPLXF
#define H5_CMPLXF(real, imag)                                                                                \
    ((H5_float_complex)((float)(real) + (float)(imag) * (H5_float_complex)_Complex_I))
#endif
#ifndef H5_CMPLX
#define H5_CMPLX(real, imag)                                                                                 \
    ((H5_double_complex)((double)(real) + (double)(imag) * (H5_double_complex)_Complex_I))
#endif
#ifndef H5_CMPLXL
#define H5_CMPLXL(real, imag)                                                                                \
    ((H5_ldouble_complex)((long double)(real) + (long double)(imag) * (H5_ldouble_complex)_Complex_I))
#endif
#endif

/* Macro for "stringizing" an integer in the C preprocessor (use H5_TOSTRING) */
/* (use H5_TOSTRING, H5_STRINGIZE is just part of the implementation) */
#define H5_STRINGIZE(x) #x
#define H5_TOSTRING(x)  H5_STRINGIZE(x)

/* Macro for "gluing" together items, for re-scanning macros */
#define H5_GLUE(x, y)        x##y
#define H5_GLUE3(x, y, z)    x##y##z
#define H5_GLUE4(w, x, y, z) w##x##y##z

/*
 * A macro for detecting over/under-flow when casting between types
 */
#ifndef NDEBUG
#define H5_CHECK_OVERFLOW(var, vartype, casttype)                                                            \
    do {                                                                                                     \
        casttype _tmp_overflow = (casttype)(var);                                                            \
        assert((var) == (vartype)_tmp_overflow);                                                             \
    } while (0)
#else /* NDEBUG */
#define H5_CHECK_OVERFLOW(var, vartype, casttype)
#endif /* NDEBUG */

/*
 * A macro for detecting over/under-flow when assigning between types
 */
#ifndef NDEBUG
#define ASSIGN_TO_SMALLER_SIZE(dst, dsttype, src, srctype)                                                   \
    {                                                                                                        \
        srctype _tmp_src = (srctype)(src);                                                                   \
        dsttype _tmp_dst = (dsttype)(_tmp_src);                                                              \
        assert(_tmp_src == (srctype)_tmp_dst);                                                               \
        (dst) = _tmp_dst;                                                                                    \
    }

#define ASSIGN_TO_LARGER_SIZE_SAME_SIGNED(dst, dsttype, src, srctype) (dst) = (dsttype)(src);

#define ASSIGN_TO_LARGER_SIZE_SIGNED_TO_UNSIGNED(dst, dsttype, src, srctype)                                 \
    {                                                                                                        \
        srctype _tmp_src = (srctype)(src);                                                                   \
        dsttype _tmp_dst = (dsttype)(_tmp_src);                                                              \
        assert(_tmp_src >= 0);                                                                               \
        assert(_tmp_src == (srctype)_tmp_dst);                                                               \
        (dst) = _tmp_dst;                                                                                    \
    }

#define ASSIGN_TO_LARGER_SIZE_UNSIGNED_TO_SIGNED(dst, dsttype, src, srctype) (dst) = (dsttype)(src);

#define ASSIGN_TO_SAME_SIZE_UNSIGNED_TO_SIGNED(dst, dsttype, src, srctype)                                   \
    {                                                                                                        \
        srctype _tmp_src = (srctype)(src);                                                                   \
        dsttype _tmp_dst = (dsttype)(_tmp_src);                                                              \
        assert(_tmp_dst >= 0);                                                                               \
        assert(_tmp_src == (srctype)_tmp_dst);                                                               \
        (dst) = _tmp_dst;                                                                                    \
    }

#define ASSIGN_TO_SAME_SIZE_SIGNED_TO_UNSIGNED(dst, dsttype, src, srctype)                                   \
    {                                                                                                        \
        srctype _tmp_src = (srctype)(src);                                                                   \
        dsttype _tmp_dst = (dsttype)(_tmp_src);                                                              \
        assert(_tmp_src >= 0);                                                                               \
        assert(_tmp_src == (srctype)_tmp_dst);                                                               \
        (dst) = _tmp_dst;                                                                                    \
    }

#define ASSIGN_TO_SAME_SIZE_SAME_SIGNED(dst, dsttype, src, srctype) (dst) = (dsttype)(src);

/* Include the generated overflow header file */
#include "H5overflow.h"

/* Assign a variable to one of a different size (think safer dst = (dsttype)src").
 * The code generated by the macro checks for overflows.
 *
 * Use w##x##y##z instead of H5_GLUE4(w, x, y, z) because srctype
 * or dsttype on some systems (e.g., NetBSD 8 and earlier) may
 * supply some standard types using a macro---e.g.,
 * #define uint8_t __uint8_t.  The preprocessor will expand the
 * macros before it evaluates H5_GLUE4(), and that will generate
 * an unexpected name such as ASSIGN___uint8_t_TO___uint16_t.
 * The preprocessor does not expand macros in w##x##y##z, so
 * that will always generate the expected name.
 */
#define H5_CHECKED_ASSIGN(dst, dsttype, src, srctype)                                                        \
    do {                                                                                                     \
        ASSIGN_##srctype##_TO_##dsttype(dst, dsttype, src, srctype)                                          \
    } while (0)

#else /* NDEBUG */
#define H5_CHECKED_ASSIGN(dst, dsttype, src, srctype)                                                        \
    do {                                                                                                     \
        (dst) = (dsttype)(src);                                                                              \
    } while (0)
#endif /* NDEBUG */

#if defined(H5_HAVE_WINDOW_PATH)

/* directory delimiter for Windows: slash and backslash are acceptable on Windows */
#define H5_DIR_SLASH_SEPC      '/'
#define H5_DIR_SEPC            '\\'
#define H5_DIR_SEPS            "\\"
#define H5_CHECK_DELIMITER(SS) ((SS == H5_DIR_SEPC) || (SS == H5_DIR_SLASH_SEPC))
#define H5_CHECK_ABSOLUTE(NAME)                                                                              \
    ((isalpha((unsigned char)NAME[0])) && (NAME[1] == ':') && (H5_CHECK_DELIMITER(NAME[2])))
#define H5_CHECK_ABS_DRIVE(NAME) ((isalpha((unsigned char)NAME[0])) && (NAME[1] == ':'))
#define H5_CHECK_ABS_PATH(NAME)  (H5_CHECK_DELIMITER(NAME[0]))

#define H5_GET_LAST_DELIMITER(NAME, ptr)                                                                     \
    {                                                                                                        \
        char *slash, *backslash;                                                                             \
                                                                                                             \
        slash     = strrchr(NAME, H5_DIR_SLASH_SEPC);                                                        \
        backslash = strrchr(NAME, H5_DIR_SEPC);                                                              \
        if (backslash > slash)                                                                               \
            (ptr = backslash);                                                                               \
        else                                                                                                 \
            (ptr = slash);                                                                                   \
    }

#else /* H5_HAVE_WINDOW_PATH */

#define H5_DIR_SEPC                      '/'
#define H5_DIR_SEPS                      "/"
#define H5_CHECK_DELIMITER(SS)           (SS == H5_DIR_SEPC)
#define H5_CHECK_ABSOLUTE(NAME)          (H5_CHECK_DELIMITER(*NAME))
#define H5_CHECK_ABS_DRIVE(NAME)         (0)
#define H5_CHECK_ABS_PATH(NAME)          (0)
#define H5_GET_LAST_DELIMITER(NAME, ptr) ptr = strrchr(NAME, H5_DIR_SEPC);

#endif /* H5_HAVE_WINDOW_PATH */

#define H5_COLON_SEPC ':'

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
    H5_PKG_A,  /* Attributes               */
    H5_PKG_AC, /* Metadata cache           */
    H5_PKG_B,  /* B-trees                  */
    H5_PKG_D,  /* Datasets                 */
    H5_PKG_E,  /* Error handling           */
    H5_PKG_F,  /* Files                    */
    H5_PKG_G,  /* Groups                   */
    H5_PKG_HG, /* Global heaps             */
    H5_PKG_HL, /* Local heaps              */
    H5_PKG_I,  /* IDs                      */
    H5_PKG_M,  /* Maps                     */
    H5_PKG_MF, /* File memory management   */
    H5_PKG_MM, /* Core memory management   */
    H5_PKG_O,  /* Object headers           */
    H5_PKG_P,  /* Property lists           */
    H5_PKG_S,  /* Dataspaces               */
    H5_PKG_T,  /* Datatypes                */
    H5_PKG_V,  /* Vector functions         */
    H5_PKG_VL, /* VOL functions            */
    H5_PKG_Z,  /* Raw data filters         */
    H5_NPKGS   /* Must be last             */
} H5_pkg_t;

typedef struct H5_debug_open_stream_t {
    FILE                          *stream; /* Open output stream */
    struct H5_debug_open_stream_t *next;   /* Next open output stream */
} H5_debug_open_stream_t;

typedef struct H5_debug_t {
    FILE *trace;  /*API trace output stream  */
    bool  ttop;   /*Show only top-level calls?    */
    bool  ttimes; /*Show trace event times?       */
    struct {
        const char *name;   /*package name      */
        FILE       *stream; /*output stream  or NULL    */
    } pkg[H5_NPKGS];
    H5_debug_open_stream_t *open_stream; /* Stack of open output streams */
} H5_debug_t;

#ifdef H5_HAVE_PARALLEL

/*
 * Check that the MPI library version is at least version
 * `mpi_version` and subversion `mpi_subversion`
 */
#define H5_CHECK_MPI_VERSION(mpi_version, mpi_subversion)                                                    \
    ((MPI_VERSION > (mpi_version)) ||                                                                        \
     ((MPI_VERSION == (mpi_version)) && (MPI_SUBVERSION >= (mpi_subversion))))

extern bool H5_coll_api_sanity_check_g;
#endif /* H5_HAVE_PARALLEL */

extern H5_debug_t H5_debug_g;
#define H5DEBUG(X) (H5_debug_g.pkg[H5_PKG_##X].stream)

/* Embedded build information */
extern const char H5build_settings[];

/* Prepare to call / return from user callback */
#include "H5Eprivate.h"
typedef struct H5_user_cb_state_t {
    H5E_user_cb_state_t h5e_state; /* State for H5E package */
} H5_user_cb_state_t;

#define H5_BEFORE_USER_CB(err)                                                                               \
    {                                                                                                        \
        H5_user_cb_state_t state;                                                                            \
                                                                                                             \
        if (H5_user_cb_prepare(&state) < 0)                                                                  \
            HGOTO_ERROR(H5E_LIB, H5E_CANTSET, (err), "preparation for user callback failed");

#define H5_AFTER_USER_CB(err)                                                                                \
    if (H5_user_cb_restore(&state) < 0)                                                                      \
        HGOTO_ERROR(H5E_LIB, H5E_CANTRESTORE, (err), "preparation for user callback failed");                \
    }

#define H5_BEFORE_USER_CB_NOERR(err)                                                                         \
    {                                                                                                        \
        H5_user_cb_state_t state;                                                                            \
                                                                                                             \
        if (H5_user_cb_prepare(&state) < 0)                                                                  \
            ret_value = (err);                                                                               \
        else {

#define H5_AFTER_USER_CB_NOERR(err)                                                                          \
    if (H5_user_cb_restore(&state) < 0)                                                                      \
        ret_value = (err);                                                                                   \
    } /* end else */                                                                                         \
    }

#define H5_BEFORE_USER_CB_NOCHECK                                                                            \
    {                                                                                                        \
        H5_user_cb_state_t state;                                                                            \
                                                                                                             \
        H5_user_cb_prepare(&state);

#define H5_AFTER_USER_CB_NOCHECK                                                                             \
    H5_user_cb_restore(&state);                                                                              \
    }

/*-------------------------------------------------------------------------
 * Purpose: These macros are used to track arguments in event sets and are
 *          inserted automatically into H5ES_insert() by the bin/trace script
 *
 * Arguments:   C  - Caller
 *              T  - Argument types encoded as a string
 *              A0-An  - Arguments.  The number at the end of the macro name
 *                                   indicates the number of arguments.
 *
 *-------------------------------------------------------------------------
 */
#define H5ARG_TRACE0(C, T)                         C, T
#define H5ARG_TRACE1(C, T, A0)                     C, T, #A0, A0
#define H5ARG_TRACE2(C, T, A0, A1)                 C, T, #A0, A0, #A1, A1
#define H5ARG_TRACE3(C, T, A0, A1, A2)             C, T, #A0, A0, #A1, A1, #A2, A2
#define H5ARG_TRACE4(C, T, A0, A1, A2, A3)         C, T, #A0, A0, #A1, A1, #A2, A2, #A3, A3
#define H5ARG_TRACE5(C, T, A0, A1, A2, A3, A4)     C, T, #A0, A0, #A1, A1, #A2, A2, #A3, A3, #A4, A4
#define H5ARG_TRACE6(C, T, A0, A1, A2, A3, A4, A5) C, T, #A0, A0, #A1, A1, #A2, A2, #A3, A3, #A4, A4, #A5, A5
#define H5ARG_TRACE7(C, T, A0, A1, A2, A3, A4, A5, A6)                                                       \
    C, T, #A0, A0, #A1, A1, #A2, A2, #A3, A3, #A4, A4, #A5, A5, #A6, A6
#define H5ARG_TRACE8(C, T, A0, A1, A2, A3, A4, A5, A6, A7)                                                   \
    C, T, #A0, A0, #A1, A1, #A2, A2, #A3, A3, #A4, A4, #A5, A5, #A6, A6, #A7, A7
#define H5ARG_TRACE9(C, T, A0, A1, A2, A3, A4, A5, A6, A7, A8)                                               \
    C, T, #A0, A0, #A1, A1, #A2, A2, #A3, A3, #A4, A4, #A5, A5, #A6, A6, #A7, A7, #A8, A8
#define H5ARG_TRACE10(C, T, A0, A1, A2, A3, A4, A5, A6, A7, A8, A9)                                          \
    C, T, #A0, A0, #A1, A1, #A2, A2, #A3, A3, #A4, A4, #A5, A5, #A6, A6, #A7, A7, #A8, A8, #A9, A9
#define H5ARG_TRACE11(C, T, A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10)                                     \
    C, T, #A0, A0, #A1, A1, #A2, A2, #A3, A3, #A4, A4, #A5, A5, #A6, A6, #A7, A7, #A8, A8, #A9, A9, #A10, A10
#define H5ARG_TRACE12(C, T, A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11)                                \
    C, T, #A0, A0, #A1, A1, #A2, A2, #A3, A3, #A4, A4, #A5, A5, #A6, A6, #A7, A7, #A8, A8, #A9, A9, #A10,    \
        A10, #A11, A11

struct H5RS_str_t;
H5_DLL double H5_trace(const double *calltime, const char *func, const char *type, ...);
H5_DLL herr_t H5_trace_args(struct H5RS_str_t *rs, const char *type, va_list ap);

/*-------------------------------------------------------------------------
 * Purpose:  Register function entry for library initialization and code
 *    profiling.
 *
 * Notes:  Every file must have a file-scope variable called
 *    `initialize_interface_g' of type bool which is initialized
 *    to false.
 *
 *    Don't use local variable initializers which contain
 *    calls to other library functions since the initializer
 *    would happen before the FUNC_ENTER() gets called.  Don't
 *    use initializers that require special cleanup code to
 *    execute if FUNC_ENTER() fails since a failing FUNC_ENTER()
 *    returns immediately without branching to the `done' label.
 *-------------------------------------------------------------------------
 */

/* global library version information string */
extern char H5_lib_vers_info_g[];

/* Both the 'threadsafe' and 'concurrency' options provide threadsafely for
 * API calls.
 */
#if defined(H5_HAVE_THREADSAFE) || defined(H5_HAVE_CONCURRENCY)
#define H5_HAVE_THREADSAFE_API
#endif

#ifdef H5_HAVE_THREADSAFE_API

/* Lock headers */
#include "H5TSprivate.h"

/* Thread cancellation is only possible w/pthreads */
#if defined(H5_HAVE_PTHREAD_H)

/* Local variable for saving cancellation state */
#define H5CANCEL_DECL int oldstate = 0;

/* Disable & restore canceling the thread */
#define H5TS_DISABLE_CANCEL                                                                                  \
    do {                                                                                                     \
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);                                           \
    } while (0)
#define H5TS_RESTORE_CANCEL                                                                                  \
    do {                                                                                                     \
        pthread_setcancelstate(oldstate, NULL);                                                              \
    } while (0)
#else
/* Local variable for saving cancellation state */
#define H5CANCEL_DECL /* */

/* Disable & restore canceling the thread */
#define H5TS_DISABLE_CANCEL                                                                                  \
    do {                                                                                                     \
    } while (0) /* no-op */
#define H5TS_RESTORE_CANCEL                                                                                  \
    do {                                                                                                     \
    } while (0) /* no-op */
#endif

#ifdef H5_HAVE_THREADSAFE
/* Local variable for 'disable locking for this thread' (DLFTT) state */
#define H5DLFTT_DECL /* */

/* Macros for entering & leaving an API routine in a threadsafe manner */
#define H5_API_LOCK                                                                                          \
    /* Acquire the API lock */                                                                               \
    H5TS_api_lock();                                                                                         \
                                                                                                             \
    /* Set thread cancellation state to 'disable', and remember previous state */                            \
    H5TS_DISABLE_CANCEL;
#define H5_API_UNLOCK                                                                                        \
    /* Release the API lock */                                                                               \
    H5TS_api_unlock();                                                                                       \
                                                                                                             \
    /* Restore previous thread cancellation state */                                                         \
    H5TS_RESTORE_CANCEL;
#else /* H5_HAVE_CONCURRENCY */
/* Local variable for 'disable locking for this thread' (DLFTT) state */
#define H5DLFTT_DECL unsigned dlftt = 0;

/* Macros for entering & leaving an API routine in a threadsafe manner */
#define H5_API_LOCK                                                                                          \
    /* Acquire the API lock */                                                                               \
    H5TS_api_lock(&dlftt);                                                                                   \
                                                                                                             \
    /* Set thread cancellation state to 'disable', and remember previous state */                            \
    if (0 == dlftt)                                                                                          \
        H5TS_DISABLE_CANCEL;
#define H5_API_UNLOCK                                                                                        \
    if (0 == dlftt) {                                                                                        \
        /* Release the API lock */                                                                           \
        H5TS_api_unlock();                                                                                   \
                                                                                                             \
        /* Restore previous thread cancellation state */                                                     \
        H5TS_RESTORE_CANCEL;                                                                                 \
    }
#endif
#else                 /* H5_HAVE_THREADSAFE_API */

/* Local variable for saving cancellation state */
#define H5CANCEL_DECL /* */

/* Local variable for 'disable locking for this thread' (DLFTT) state */
#define H5DLFTT_DECL  /* */

/* No locks (non-threadsafe builds) */
#define H5_API_LOCK   /* no-op */
#define H5_API_UNLOCK /* no-op */

#endif /* H5_HAVE_THREADSAFE_API */

/* Macros for accessing the global variables */
#define H5_INIT_GLOBAL (H5_libinit_g)
#define H5_TERM_GLOBAL (H5_libterm_g)

/* Macros for referencing package initialization symbols */
#define H5_PACKAGE_INIT_VAR(x)  H5_GLUE(x, _init_g)
#define H5_PACKAGE_INIT_FUNC(x) H5_GLUE(x, __init_package)

/* Macros for defining package initialization routines */
#ifdef H5_MY_PKG
#define H5_PKG_INIT_VAR  H5_PACKAGE_INIT_VAR(H5_MY_PKG)
#define H5_PKG_INIT_FUNC H5_PACKAGE_INIT_FUNC(H5_MY_PKG)
#define H5_PACKAGE_YES_INIT(err)                                                                             \
    /* Initialize this interface or bust */                                                                  \
    if (H5_UNLIKELY(!H5_PKG_INIT_VAR && !H5_TERM_GLOBAL)) {                                                  \
        H5_PKG_INIT_VAR = true;                                                                              \
        if (H5_PKG_INIT_FUNC() < 0) {                                                                        \
            H5_PKG_INIT_VAR = false;                                                                         \
            HGOTO_ERROR(H5E_FUNC, H5E_CANTINIT, err, "interface initialization failed");                     \
        }                                                                                                    \
    }
#define H5_PACKAGE_NO_INIT(err)                                                                              \
    /* Mark a package without an init interface call as initialized */                                       \
    if (H5_UNLIKELY(!H5_PKG_INIT_VAR && !H5_TERM_GLOBAL))                                                    \
        H5_PKG_INIT_VAR = true;
#define H5_PACKAGE_INIT(pkg_init, err) H5_GLUE3(H5_PACKAGE_, pkg_init, _INIT)(err)
#else /* H5_MY_PKG */
#define H5_PKG_INIT_VAR (true)
#define H5_PACKAGE_INIT(pkg_init, err)
#endif /* H5_MY_PKG */

#include "H5CXprivate.h" /* API Contexts */

/* clang-format off */

/* ----------------------------------------------------------------------------
 * Macros to check function names for appropriate form. Called from the
 * FUNC_ENTER macros, below.
 *
 * - public:         H5X(Y)foo()
 *
 * - private:        H5X(Y)_foo()
 *
 * - package/static: H5X(Y)__foo()
 *
 * NOTE: These will generate somewhat cryptic errors when APIs with incorrectly
 *       formed names are called at runtime. The H5_CHECK_FUNCTION_NAME()
 *       macro will emit a helpful error message if it detects badness.
 * ----------------------------------------------------------------------------
 */

/* Macro to check if a function call is a public HDF5 API call
 *
 * `S' is the name of a function which is being tested to check if it's
 * an API function.
 *
 *  UNDERSCORE CHECKS:
 *      - Underscore at positions 2 or 3 (0-indexed string). Handles
 *        H5_ and H5X_.
 *      - Underscore at position 4 if position 3 is uppercase or a digit.
 *        Handles H5XY_.
 */
#define H5_IS_PUBLIC(S)                                                                                      \
    ('_' != ((const char *)S)[2]                        /* underscore at position 2     */                   \
     && '_' != ((const char *)S)[3]                     /* underscore at position 3     */                   \
     && !(                                              /* NOT              */                               \
          ((const char *)S)[4]                          /* pos 4 exists     */                               \
          && (isupper((int)S[3]) || isdigit((int)S[3])) /* pos 3 dig | uc   */                               \
          && '_' == ((const char *)S)[4]                /* pos 4 underscore */                               \
          ))

/* Macro to check if a function call is a library private HDF5 API call
 *
 * These have the form H5X(Y)_foo() <--- single underscore
 *
 * `S' is the name of a function which is being tested to check if it's a private function
 */
#define H5_IS_PRIVATE(S)                                                                                     \
    (((isdigit((int)S[1]) || isupper((int)S[1])) && '_' == S[2] && islower((int)S[3])) ||                    \
     ((isdigit((int)S[2]) || isupper((int)S[2])) && '_' == S[3] && islower((int)S[4])) ||                    \
     ((isdigit((int)S[3]) || isupper((int)S[3])) && '_' == S[4] && islower((int)S[5])))

/* Macro to check if a function call is a package internal HDF5 API call
 *
 * These have the form H5X(Y)__foo() <--- two underscores
 *
 * `S' is the name of a function which is being tested to check if it's a package function
 */
#define H5_IS_PKG(S)                                                                                         \
    (((isdigit((int)S[1]) || isupper((int)S[1])) && '_' == S[2] && '_' == S[3] && islower((int)S[4])) ||     \
     ((isdigit((int)S[2]) || isupper((int)S[2])) && '_' == S[3] && '_' == S[4] && islower((int)S[5])) ||     \
     ((isdigit((int)S[3]) || isupper((int)S[3])) && '_' == S[4] && '_' == S[5] && islower((int)S[6])))

/* Macro to check that an API call is using a correctly formed name.
 *
 * The name checks only occur in debug builds to avoid performance degradation and only take
 * place once per API call per library initialization.
 */
/* XXX(kitware): Our mangling breaks the rules for this; assume upstream did their job. */
/* #ifndef NDEBUG */
#if 0
#define H5_CHECK_FUNCTION_NAME(asrt)                                                                         \
    {                                                                                                        \
        static bool func_check = false;                                                                      \
                                                                                                             \
        if (H5_UNLIKELY(!func_check)) {                                                                      \
            /* Check function naming status */                                                               \
            assert(asrt &&                                                                                   \
                   "Function naming conventions are incorrect (see H5private.h)"                             \
                   "(this is usually due to an incorrect number of underscores in the function name)");      \
                                                                                                             \
            /* Don't check again */                                                                          \
            func_check = true;                                                                               \
        }                                                                                                    \
    }
#else
#define H5_CHECK_FUNCTION_NAME(asrt)
#endif
/* ----------------------------------------------------------------------------
 * Macros that things up upon entering an HDF5 API call
 *
 * These are all of the form `H5_API_SETUP_<thing>`
 * ----------------------------------------------------------------------------
 */

/* Error setup for FUNC_ENTER macros that report an error */
#define H5_API_SETUP_ERROR_HANDLING                                                                          \
    bool err_occurred = false;

/* Entry setup for public API call variables */
#define H5_API_SETUP_PUBLIC_API_VARS                                                                         \
    H5CANCEL_DECL /* thread cancellation */                 \
    H5DLFTT_DECL  /* user callback protection */

/* Macro to initialize the library, if some other package hasn't already done that */
#define H5_API_SETUP_INIT_LIBRARY(err)                                                                       \
    do {                                                                                                     \
        if (H5_UNLIKELY(!H5_INIT_GLOBAL && !H5_TERM_GLOBAL)) {                                               \
            if (H5_UNLIKELY(H5_init_library() < 0))                                                          \
                HGOTO_ERROR(H5E_FUNC, H5E_CANTINIT, err, "library initialization failed");                   \
        }                                                                                                    \
        /* Initialize the package, if appropriate */                                                         \
        H5_PACKAGE_INIT(H5_MY_PKG_INIT, err)                                                                 \
    } while (0)

/* Macro to push the API context */
#define H5_API_SETUP_PUSH_CONTEXT(err)                                                                       \
    /* The library context variable can't go in this macro since then it might                               \
     * be uninitialized if the library init fails.                                                           \
     */                                                                                                      \
    do {                                                                                                     \
        if (H5_UNLIKELY(H5CX_push(&api_ctx) < 0))                                                            \
            HGOTO_ERROR(H5E_FUNC, H5E_CANTSET, err, "can't set API context");                                \
        else                                                                                                 \
            api_ctx_pushed = true;                                                                           \
    } while (0)

/* ----------------------------------------------------------------------------
 * HDF5 API call entry macros
 *
 * These are all of the form `FUNC_ENTER_*`. Every HDF5 API call will begin
 * with one of these macros immediately after the variable declarations.
 * ----------------------------------------------------------------------------
 */

/* Use this macro for all "normal" public API functions */
#define FUNC_ENTER_API(err)                                                                                  \
    {                                                                                                        \
        {                                                                                                    \
            H5CX_node_t api_ctx        = {{0}, NULL};                                                        \
            bool        api_ctx_pushed = false;                                                              \
                                                                                                             \
            H5_CHECK_FUNCTION_NAME(H5_IS_PUBLIC(__func__));                                                  \
                                                                                                             \
            H5_API_SETUP_PUBLIC_API_VARS                                                                     \
            H5_API_SETUP_ERROR_HANDLING                                                                      \
            H5_API_LOCK                                                                                      \
            H5_API_SETUP_INIT_LIBRARY(err);                                                                  \
            H5_API_SETUP_PUSH_CONTEXT(err);                                                                  \
                                                                                                             \
            /* Clear thread error stack entering public functions */                                         \
            H5E_clear_stack();                                                                               \
            {

/*
 * Use this macro for public API functions that shouldn't clear the error stack
 * like H5Eprint and H5Ewalk.
 */
#define FUNC_ENTER_API_NOCLEAR(err)                                                                          \
    {                                                                                                        \
        {                                                                                                    \
            H5CX_node_t api_ctx        = {{0}, NULL};                                                        \
            bool        api_ctx_pushed = false;                                                              \
                                                                                                             \
            H5_CHECK_FUNCTION_NAME(H5_IS_PUBLIC(__func__));                                                  \
                                                                                                             \
            H5_API_SETUP_PUBLIC_API_VARS                                                                     \
            H5_API_SETUP_ERROR_HANDLING                                                                      \
            H5_API_LOCK                                                                                      \
            H5_API_SETUP_INIT_LIBRARY(err);                                                                  \
            H5_API_SETUP_PUSH_CONTEXT(err);                                                                  \
            {

/*
 * Use this macro for public API functions that shouldn't perform _any_
 * initialization of the library or an interface, just perform tracing, etc.
 * Examples are: H5is_library_threadsafe, H5VLretrieve_lib_state, etc.
 */
#define FUNC_ENTER_API_NOINIT                                                                                \
    {                                                                                                        \
        {                                                                                                    \
            {                                                                                                \
                H5_CHECK_FUNCTION_NAME(H5_IS_PUBLIC(__func__));                                              \
                                                                                                             \
                H5_API_SETUP_PUBLIC_API_VARS                                                                 \
                H5_API_SETUP_ERROR_HANDLING                                                                  \
                H5_API_LOCK                                                                                  \
                {

/*
 * Use this macro for public API functions that shouldn't perform _any_
 * initialization of the library or an interface or push themselves on the
 * function stack, just perform tracing, etc. Examples are: H5dont_atexit,
 * H5check_version, etc.
 */
#define FUNC_ENTER_API_NOINIT_NOERR                                                                          \
    {                                                                                                        \
        {                                                                                                    \
            {                                                                                                \
                {                                                                                            \
                    H5_CHECK_FUNCTION_NAME(H5_IS_PUBLIC(__func__));                                          \
                                                                                                             \
                    H5_API_SETUP_PUBLIC_API_VARS                                                             \
                    H5_API_LOCK                                                                              \
                    {

/*
 * Use this macro for public API functions that should only perform
 * initialization of the library or an interface, but not push any state (API
 * context, function name, etc.). Examples are: H5open.
 */
#define FUNC_ENTER_API_NOPUSH(err)                                                                           \
    {                                                                                                        \
        {                                                                                                    \
            {                                                                                                \
                {                                                                                            \
                    {                                                                                        \
                        H5_CHECK_FUNCTION_NAME(H5_IS_PUBLIC(__func__));                                      \
                                                                                                             \
                        H5_API_SETUP_PUBLIC_API_VARS                                                         \
                        H5_API_SETUP_ERROR_HANDLING                                                          \
                        H5_API_LOCK                                                                          \
                        H5_API_SETUP_INIT_LIBRARY(err);                                                      \
                        {

/*
 * Use this macro for public API functions that shouldn't perform _any_
 * initialization of the library or an interface, or push themselves on the
 * function stack, or perform tracing, etc.  This macro _only_ sanity checks
 * the API name itself. Examples are: H5TSmutex_acquire
 */
#define FUNC_ENTER_API_NAMECHECK_ONLY                                                                        \
    {                                                                                                        \
        {                                                                                                    \
            {                                                                                                \
                {                                                                                            \
                    {                                                                                        \
                        {                                                                                    \
                            H5_CHECK_FUNCTION_NAME(H5_IS_PUBLIC(__func__));                                  \
                            {

/* Note: This macro only works when there's _no_ interface initialization routine for the module */
#define FUNC_ENTER_NOAPI_INIT(err)                                                                           \
    /* Initialize the package, if appropriate */                                                             \
    H5_PACKAGE_INIT(H5_MY_PKG_INIT, err)

/* Use this macro for all "normal" non-API functions */
#define FUNC_ENTER_NOAPI(err)                                                                                \
    {                                                                                                        \
        H5_CHECK_FUNCTION_NAME(H5_IS_PRIVATE(__func__));                                                     \
                                                                                                             \
        H5_API_SETUP_ERROR_HANDLING                                                                          \
        FUNC_ENTER_NOAPI_INIT(err)                                                                           \
        if (H5_LIKELY(H5_PKG_INIT_VAR || !H5_TERM_GLOBAL))                                                   \
        {

/* Use this macro for all non-API functions, which propagate errors, but don't issue them */
#define FUNC_ENTER_NOAPI_NOERR                                                                               \
    {                                                                                                        \
        H5_CHECK_FUNCTION_NAME(H5_IS_PRIVATE(__func__));                                                     \
                                                                                                             \
        FUNC_ENTER_NOAPI_INIT(-)                                                                             \
        if (H5_LIKELY(H5_PKG_INIT_VAR || !H5_TERM_GLOBAL))                                                   \
        {

/*
 * Use this macro for non-API functions which are called during library
 * shutdown, since we don't want to re-initialize the library.
 */
#define FUNC_ENTER_NOAPI_NOINIT                                                                              \
    {                                                                                                        \
        H5_CHECK_FUNCTION_NAME(H5_IS_PRIVATE(__func__));                                                     \
                                                                                                             \
        H5_API_SETUP_ERROR_HANDLING                                                                          \
        if (H5_LIKELY(H5_PKG_INIT_VAR || !H5_TERM_GLOBAL))                                                   \
        {

/*
 * Use this macro for non-API functions that propagate, but do not issue
 * errors, which are called during library shutdown, since we don't want to
 * re-initialize the library.
 */
#define FUNC_ENTER_NOAPI_NOINIT_NOERR                                                                        \
    {                                                                                                        \
        H5_CHECK_FUNCTION_NAME(H5_IS_PRIVATE(__func__));                                                     \
                                                                                                             \
        if (H5_LIKELY(H5_PKG_INIT_VAR || !H5_TERM_GLOBAL))                                                   \
        {

/*
 * Use this macro for non-API functions that shouldn't perform _any_
 * initialization of the library or an interface, or push themselves on the
 * function stack, or perform tracing, etc.  This macro _only_ sanity checks
 * the API name itself. Examples are private routines in the H5TS package.
 */
#define FUNC_ENTER_NOAPI_NAMECHECK_ONLY                                                                      \
    {                                                                                                        \
        H5_CHECK_FUNCTION_NAME(H5_IS_PRIVATE(__func__));

/* Use the following two macros as replacements for the FUNC_ENTER_NOAPI
 * and FUNC_ENTER_NOAPI_NOINIT macros when the function needs to set
 * up a metadata tag.
 */
#define FUNC_ENTER_NOAPI_TAG(tag, err)                                                                       \
    {                                                                                                        \
        haddr_t prev_tag = HADDR_UNDEF;                                                                      \
                                                                                                             \
        H5_CHECK_FUNCTION_NAME(H5_IS_PRIVATE(__func__));                                                     \
                                                                                                             \
        H5_API_SETUP_ERROR_HANDLING                                                                          \
                                                                                                             \
        H5AC_tag(tag, &prev_tag);                                                                            \
        FUNC_ENTER_NOAPI_INIT(err)                                                                           \
        if (H5_LIKELY(H5_PKG_INIT_VAR || !H5_TERM_GLOBAL))                                                   \
        {

#define FUNC_ENTER_NOAPI_NOINIT_TAG(tag)                                                                     \
    {                                                                                                        \
        haddr_t prev_tag = HADDR_UNDEF;                                                                      \
                                                                                                             \
        H5_CHECK_FUNCTION_NAME(H5_IS_PRIVATE(__func__));                                                     \
                                                                                                             \
        H5_API_SETUP_ERROR_HANDLING                                                                          \
                                                                                                             \
        H5AC_tag(tag, &prev_tag);                                                                            \
        if (H5_LIKELY(H5_PKG_INIT_VAR || !H5_TERM_GLOBAL))                                                   \
        {

/* Use this macro for all "normal" package-level and static functions */
#define FUNC_ENTER_PACKAGE                                                                                   \
    {                                                                                                        \
        H5_CHECK_FUNCTION_NAME(H5_IS_PKG(__func__));                                                         \
                                                                                                             \
        H5_API_SETUP_ERROR_HANDLING                                                                          \
        if (H5_LIKELY(H5_PKG_INIT_VAR || !H5_TERM_GLOBAL))                                                   \
        {

/* Use this macro for package-level and static functions which propagate
 * errors, but don't issue them
 */
#define FUNC_ENTER_PACKAGE_NOERR                                                                             \
    {                                                                                                        \
        H5_CHECK_FUNCTION_NAME(H5_IS_PKG(__func__));                                                         \
        if (H5_LIKELY(H5_PKG_INIT_VAR || !H5_TERM_GLOBAL))                                                   \
        {

/* Use the following macro as replacement for the FUNC_ENTER_PACKAGE
 * macro when the function needs to set up a metadata tag.
 */
#define FUNC_ENTER_PACKAGE_TAG(tag)                                                                          \
    {                                                                                                        \
        haddr_t prev_tag = HADDR_UNDEF;                                                                      \
                                                                                                             \
        H5_CHECK_FUNCTION_NAME(H5_IS_PKG(__func__));                                                         \
                                                                                                             \
        H5_API_SETUP_ERROR_HANDLING                                                                          \
                                                                                                             \
        H5AC_tag(tag, &prev_tag);                                                                            \
        if (H5_LIKELY(H5_PKG_INIT_VAR || !H5_TERM_GLOBAL))                                                   \
        {

/*
 * Use this macro for package-level or static functions that shouldn't perform
 * _any_ initialization of the library or an interface, or push themselves on
 * the function stack, or perform tracing, etc.  This macro _only_ sanity
 * checks the API name itself. Examples are static routines in the H5TS package.
 */
#define FUNC_ENTER_PACKAGE_NAMECHECK_ONLY                                                                    \
    {                                                                                                        \
        H5_CHECK_FUNCTION_NAME(H5_IS_PKG(__func__));

/* ----------------------------------------------------------------------------
 * HDF5 API call leave macros
 *
 * These are all of the form `FUNC_LEAVE_*` and need to match the FUNC_ENTER
 * macro used when entering. The PACKAGE FUNC_ENTER macros use the NOAPI
 * FUNC_LEAVE macros.
 *
 * The FUNC_LEAVE macro will be the last statement in an HDF5 API call.
 *
 * NOTE: All FUNC_LEAVE macros begin with a semicolon to prevent compiler
 *       warnings from having the done: target right before the scope-closing
 *       bracket. Labels at the end of compound statements is a C23 extension.
 * ----------------------------------------------------------------------------
 */

#define FUNC_LEAVE_API(ret_value)                                                                            \
    ;                                                                                                        \
    } /* end scope from end of FUNC_ENTER */                                                                 \
    if (H5_LIKELY(api_ctx_pushed)) {                                                                         \
        (void)H5CX_pop(true);                                                                                \
        api_ctx_pushed = false;                                                                              \
    }                                                                                                        \
    if (H5_UNLIKELY(err_occurred))                                                                           \
        (void)H5E_dump_api_stack();                                                                          \
    H5_API_UNLOCK                                                                                            \
    return (ret_value);                                                                                      \
    }                                                                                                        \
    } /* end scope from beginning of FUNC_ENTER */

/* Use this macro to match the FUNC_ENTER_API_NOINIT macro */
#define FUNC_LEAVE_API_NOINIT(ret_value)                                                                     \
    ;                                                                                                        \
    } /* end scope from end of FUNC_ENTER */                                                                 \
    if (H5_UNLIKELY(err_occurred))                                                                           \
        (void)H5E_dump_api_stack();                                                                          \
    H5_API_UNLOCK                                                                                            \
    return (ret_value);                                                                                      \
    }                                                                                                        \
    }                                                                                                        \
    } /* end scope from beginning of FUNC_ENTER */

/* Use this macro to match the FUNC_ENTER_API_NOINIT_NOERR macro */
#define FUNC_LEAVE_API_NOERR(ret_value)                                                                      \
    ;                                                                                                        \
    } /* end scope from end of FUNC_ENTER */                                                                 \
    H5_API_UNLOCK                                                                                            \
    return (ret_value);                                                                                      \
    }                                                                                                        \
    }                                                                                                        \
    }                                                                                                        \
    } /* end scope from beginning of FUNC_ENTER */

/* Use this macro to match the FUNC_ENTER_API_NOPUSH macro */
#define FUNC_LEAVE_API_NOPUSH(ret_value)                                                                     \
    ;                                                                                                        \
    } /* end scope from end of FUNC_ENTER */                                                                 \
    if (H5_UNLIKELY(err_occurred))                                                                           \
        (void)H5E_dump_api_stack();                                                                          \
    H5_API_UNLOCK                                                                                            \
    return (ret_value);                                                                                      \
    }                                                                                                        \
    }                                                                                                        \
    }                                                                                                        \
    }                                                                                                        \
    } /* end scope from beginning of FUNC_ENTER */

/* Use this macro to match the FUNC_ENTER_API_NAMECHECK_ONLY macro */
#define FUNC_LEAVE_API_NAMECHECK_ONLY(ret_value)                                                             \
    ;                                                                                                        \
    } /* end scope from end of FUNC_ENTER */                                                                 \
    return (ret_value);                                                                                      \
    }                                                                                                        \
    }                                                                                                        \
    }                                                                                                        \
    }                                                                                                        \
    }                                                                                                        \
    } /* end scope from beginning of FUNC_ENTER */

/* Use this macro to match NOAPI and PACKAGE macros which return a value */
#define FUNC_LEAVE_NOAPI(ret_value)                                                                          \
    ;                                                                                                        \
    } /* end scope from end of FUNC_ENTER */                                                                 \
    return (ret_value);                                                                                      \
    } /* end scope from beginning of FUNC_ENTER */

/* Use this macro to match NOAPI and PACKAGE macros which do not return a value */
#define FUNC_LEAVE_NOAPI_VOID                                                                                \
    ;                                                                                                        \
    } /* end scope from end of FUNC_ENTER */                                                                 \
    return;                                                                                                  \
    } /* end scope from beginning of FUNC_ENTER */

/* Use these macros to match the FUNC_ENTER_NOAPI_NAMECHECK_ONLY macro */
#define FUNC_LEAVE_NOAPI_NAMECHECK_ONLY(ret_value)                                                           \
    return (ret_value);                                                                                      \
    } /* end scope from beginning of FUNC_ENTER */
#define FUNC_LEAVE_NOAPI_VOID_NAMECHECK_ONLY                                                                 \
    return;                                                                                                  \
    } /* end scope from beginning of FUNC_ENTER */

/* Use this macro when exiting a function that sets up a metadata tag */
#define FUNC_LEAVE_NOAPI_TAG(ret_value)                                                                      \
    ;                                                                                                        \
    } /* end scope from end of FUNC_ENTER */                                                                 \
    H5AC_tag(prev_tag, NULL);                                                                                \
    return (ret_value);                                                                                      \
    } /* end scope from beginning of FUNC_ENTER */

/* clang-format on */

/* Macros to declare package initialization function, if a package initialization routine is defined */
#define H5_PKG_DECLARE_YES_FUNC(pkg) extern herr_t H5_PACKAGE_INIT_FUNC(pkg)(void);
#define H5_PKG_DECLARE_NO_FUNC(pkg)

/* Declare package initialization symbols (if in a package) */
#define H5_PKG_DECLARE_VAR(pkg)            extern bool H5_PACKAGE_INIT_VAR(pkg);
#define H5_PKG_DECLARE_FUNC(pkg_init, pkg) H5_GLUE3(H5_PKG_DECLARE_, pkg_init, _FUNC)(pkg)

#ifdef H5_MY_PKG
H5_PKG_DECLARE_VAR(H5_MY_PKG)
H5_PKG_DECLARE_FUNC(H5_MY_PKG_INIT, H5_MY_PKG)
#endif

/* ----------------------------------------------------------------------------
 * Metadata cache tagging macros (when FUNC_ENTER_*TAG macros are insufficient)
 *
 * Make sure to use HGOTO_ERROR_TAG and HGOTO_DONE_TAG between these macros!
 * ----------------------------------------------------------------------------
 */

/* Macro to begin tagging */
#define H5_BEGIN_TAG(tag)                                                                                    \
    {                                                                                                        \
        haddr_t prv_tag = HADDR_UNDEF;                                                                       \
        H5AC_tag(tag, &prv_tag);

/* Macro to end tagging */
#define H5_END_TAG                                                                                           \
    H5AC_tag(prv_tag, NULL);                                                                                 \
    }

/* Compile-time "assert" macro */
#define HDcompile_assert(e) ((void)sizeof(char[!!(e) ? 1 : -1]))
/* Variants that are correct, but generate compile-time warnings in some circumstances:
  #define HDcompile_assert(e)     do { enum { compile_assert__ = 1 / (e) }; } while(0)
  #define HDcompile_assert(e)     do { typedef struct { unsigned int b: (e); } x; } while(0)
*/

/* File-independent encode/decode routines */
#include "H5encode.h"

/* Private functions, not part of the publicly documented API */
H5_DLL herr_t H5_init_library(void);
H5_DLL void   H5_term_library(void);

/* Functions to terminate interfaces */
H5_DLL int H5A_term_package(void);
H5_DLL int H5A_top_term_package(void);
H5_DLL int H5AC_term_package(void);
H5_DLL int H5CX_term_package(void);
H5_DLL int H5D_term_package(void);
H5_DLL int H5D_top_term_package(void);
H5_DLL int H5E_term_package(void);
H5_DLL int H5ES_term_package(void);
H5_DLL int H5F_term_package(void);
H5_DLL int H5FD_term_package(void);
H5_DLL int H5FL_term_package(void);
H5_DLL int H5FS_term_package(void);
H5_DLL int H5G_term_package(void);
H5_DLL int H5G_top_term_package(void);
H5_DLL int H5I_term_package(void);
H5_DLL int H5L_term_package(void);
H5_DLL int H5M_term_package(void);
H5_DLL int H5M_top_term_package(void);
H5_DLL int H5P_term_package(void);
H5_DLL int H5PL_term_package(void);
H5_DLL int H5R_term_package(void);
H5_DLL int H5R_top_term_package(void);
H5_DLL int H5S_term_package(void);
H5_DLL int H5S_top_term_package(void);
H5_DLL int H5SL_term_package(void);
H5_DLL int H5T_term_package(void);
H5_DLL int H5T_top_term_package(void);
H5_DLL int H5VL_term_package(void);
H5_DLL int H5Z_term_package(void);

/* Checksum functions */
H5_DLL uint32_t H5_checksum_fletcher32(const void *data, size_t len);
H5_DLL uint32_t H5_checksum_crc(const void *data, size_t len);
H5_DLL uint32_t H5_checksum_lookup3(const void *data, size_t len, uint32_t initval);
H5_DLL uint32_t H5_checksum_metadata(const void *data, size_t len, uint32_t initval);
H5_DLL uint32_t H5_hash_string(const char *str);

/* Time related routines */
H5_DLL time_t H5_make_time(struct tm *tm);
H5_DLL void   H5_nanosleep(uint64_t nanosec);
H5_DLL double H5_get_time(void);

/* Functions for building paths, etc. */
H5_DLL herr_t H5_build_extpath(const char *name, char **extpath /*out*/);
H5_DLL herr_t H5_combine_path(const char *path1, const char *path2, char **full_name /*out*/);
H5_DLL herr_t H5_dirname(const char *path, char **dirname /*out*/);
H5_DLL herr_t H5_basename(const char *path, char **basename /*out*/);

/* getopt(3) equivalent that papers over the lack of long options on BSD
 * and lack of Windows support.
 */
H5_DLLVAR int         H5_opterr; /* get_option prints errors if this is on */
H5_DLLVAR int         H5_optind; /* token pointer */
H5_DLLVAR const char *H5_optarg; /* flag argument (or value) */

enum h5_arg_level {
    no_arg = 0,  /* doesn't take an argument     */
    require_arg, /* requires an argument          */
    optional_arg /* argument is optional         */
};

/*
 * get_option determines which options are specified on the command line and
 * returns a pointer to any arguments possibly associated with the option in
 * the ``H5_optarg'' variable. get_option returns the shortname equivalent of
 * the option. The long options are specified in the following way:
 *
 * struct h5_long_options foo[] = {
 *   { "filename", require_arg, 'f' },
 *   { "append", no_arg, 'a' },
 *   { "width", require_arg, 'w' },
 *   { NULL, 0, 0 }
 * };
 *
 * Long named options can have arguments specified as either:
 *
 *   ``--param=arg'' or ``--param arg''
 *
 * Short named options can have arguments specified as either:
 *
 *   ``-w80'' or ``-w 80''
 *
 * and can have more than one short named option specified at one time:
 *
 *   -aw80
 *
 * in which case those options which expect an argument need to come at the
 * end.
 */
struct h5_long_options {
    const char       *name;     /* Name of the long option */
    enum h5_arg_level has_arg;  /* Whether we should look for an arg */
    char              shortval; /* The shortname equivalent of long arg
                                 * this gets returned from get_option
                                 */
};

H5_DLL int H5_get_option(int argc, const char *const *argv, const char *opt,
                         const struct h5_long_options *l_opt);

#ifdef H5_HAVE_PARALLEL
/* Generic MPI functions */
H5_DLL hsize_t H5_mpi_set_bigio_count(hsize_t new_count);
H5_DLL hsize_t H5_mpi_get_bigio_count(void);
H5_DLL herr_t  H5_mpi_comm_dup(MPI_Comm comm, MPI_Comm *comm_new);
H5_DLL herr_t  H5_mpi_info_dup(MPI_Info info, MPI_Info *info_new);
H5_DLL herr_t  H5_mpi_comm_free(MPI_Comm *comm);
H5_DLL herr_t  H5_mpi_info_free(MPI_Info *info);
H5_DLL herr_t  H5_mpi_comm_cmp(MPI_Comm comm1, MPI_Comm comm2, int *result);
H5_DLL herr_t  H5_mpi_info_cmp(MPI_Info info1, MPI_Info info2, int *result);
H5_DLL herr_t  H5_mpio_create_large_type(hsize_t num_elements, MPI_Aint stride_bytes, MPI_Datatype old_type,
                                         MPI_Datatype *new_type);
H5_DLL herr_t  H5_mpio_gatherv_alloc(void *send_buf, int send_count, MPI_Datatype send_type,
                                     const int recv_counts[], const int displacements[],
                                     MPI_Datatype recv_type, bool allgather, int root, MPI_Comm comm,
                                     int mpi_rank, int mpi_size, void **out_buf, size_t *out_buf_num_entries);
H5_DLL herr_t  H5_mpio_gatherv_alloc_simple(void *send_buf, int send_count, MPI_Datatype send_type,
                                            MPI_Datatype recv_type, bool allgather, int root, MPI_Comm comm,
                                            int mpi_rank, int mpi_size, void **out_buf,
                                            size_t *out_buf_num_entries);
H5_DLL herr_t  H5_mpio_get_file_sync_required(MPI_File fh, bool *file_sync_required);
#endif /* H5_HAVE_PARALLEL */

/* Functions for debugging */
H5_DLL herr_t H5_buffer_dump(FILE *stream, int indent, const uint8_t *buf, const uint8_t *marker,
                             size_t buf_offset, size_t buf_size);

/* Functions for preparing for / returning from user callbacks */
H5_DLL herr_t H5_user_cb_prepare(H5_user_cb_state_t *state);
H5_DLL herr_t H5_user_cb_restore(const H5_user_cb_state_t *state);
#endif /* H5private_H */
