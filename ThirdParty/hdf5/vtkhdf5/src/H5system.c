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

/*-------------------------------------------------------------------------
 *
 * Created:     H5system.c
 *              Aug 21 2006
 *              Quincey Koziol
 *
 * Purpose:     System call wrapper implementations.
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/
#include "H5module.h" /* This source code file is part of the H5 module */

/***********/
/* Headers */
/***********/
#include "H5private.h"   /* Generic Functions        */
#include "H5Eprivate.h"  /* Error handling           */
#include "H5Fprivate.h"  /* File access              */
#include "H5MMprivate.h" /* Memory management        */

/****************/
/* Local Macros */
/****************/

/******************/
/* Local Typedefs */
/******************/

/********************/
/* Package Typedefs */
/********************/

/********************/
/* Local Prototypes */
/********************/

/*********************/
/* Package Variables */
/*********************/

/*****************************/
/* Library Private Variables */
/*****************************/

/*******************/
/* Local Variables */
/*******************/

/* Track whether tzset routine was called */
static hbool_t H5_ntzset = FALSE;

#ifndef H5_HAVE_VASPRINTF
/* HDvasprintf provides vasprintf-like function on targets where it is
 * unavailable.
 */
int
HDvasprintf(char **bufp, const char *fmt, va_list _ap)
{
    char * buf;   /* buffer to receive formatted string */
    size_t bufsz; /* size of buffer to allocate */

    for (bufsz = 32; (buf = HDmalloc(bufsz)) != NULL;) {
        int     ret;
        va_list ap;

        HDva_copy(ap, _ap);
        ret = HDvsnprintf(buf, bufsz, fmt, ap);
        va_end(ap);
        if (ret >= 0 && (size_t)ret < bufsz) {
            *bufp = buf;
            return ret;
        }
        HDfree(buf);
        if (ret < 0)
            return ret;
        bufsz = (size_t)ret + 1;
    }
    return -1;
}
#endif /* H5_HAVE_VASPRINTF */

/*-------------------------------------------------------------------------
 * Function:  HDstrtoll
 *
 * Purpose:  Converts the string S to an int64_t value according to the
 *    given BASE, which must be between 2 and 36 inclusive, or be
 *    the special value zero.
 *
 *    The string must begin with an arbitrary amount of white space
 *    (as determined by isspace(3c)) followed by a single optional
 *              `+' or `-' sign.  If BASE is zero or 16 the string may then
 *    include a `0x' or `0X' prefix, and the number will be read in
 *    base 16; otherwise a zero BASE is taken as 10 (decimal)
 *    unless the next character is a `0', in which case it is taken
 *    as 8 (octal).
 *
 *    The remainder of the string is converted to an int64_t in the
 *    obvious manner, stopping at the first character which is not
 *    a valid digit in the given base.  (In bases above 10, the
 *    letter `A' in either upper or lower case represetns 10, `B'
 *    represents 11, and so forth, with `Z' representing 35.)
 *
 *    If REST is not null, the address of the first invalid
 *    character in S is stored in *REST.  If there were no digits
 *    at all, the original value of S is stored in *REST.  Thus, if
 *    *S is not `\0' but **REST is `\0' on return the entire string
 *    was valid.
 *
 * Return:  Success:  The result.
 *
 *    Failure:  If the input string does not contain any
 *        digits then zero is returned and REST points
 *        to the original value of S.  If an overflow
 *        or underflow occurs then the maximum or
 *        minimum possible value is returned and the
 *        global `errno' is set to ERANGE.  If BASE is
 *        incorrect then zero is returned.
 *
 * Programmer:  Robb Matzke
 *              Thursday, April  9, 1998
 *
 *-------------------------------------------------------------------------
 */
#ifndef HDstrtoll
int64_t
HDstrtoll(const char *s, const char **rest, int base)
{
    int64_t sign = 1, acc = 0;
    hbool_t overflow = FALSE;

    errno = 0;
    if (!s || (base && (base < 2 || base > 36))) {
        if (rest)
            *rest = s;
        return 0;
    }

    /* Skip white space */
    while (HDisspace(*s))
        s++;

    /* Optional minus or plus sign */
    if ('+' == *s) {
        s++;
    }
    else if ('-' == *s) {
        sign = -1;
        s++;
    }

    /* Zero base prefix */
    if (0 == base && '0' == *s && ('x' == s[1] || 'X' == s[1])) {
        base = 16;
        s += 2;
    }
    else if (0 == base && '0' == *s) {
        base = 8;
        s++;
    }
    else if (0 == base) {
        base = 10;
    }

    /* Digits */
    while ((base <= 10 && *s >= '0' && *s < '0' + base) ||
           (base > 10 && ((*s >= '0' && *s <= '9') || (*s >= 'a' && *s < 'a' + base - 10) ||
                          (*s >= 'A' && *s < 'A' + base - 10)))) {
        if (!overflow) {
            int64_t digit = 0;

            if (*s >= '0' && *s <= '9')
                digit = *s - '0';
            else if (*s >= 'a' && *s <= 'z')
                digit = (*s - 'a') + 10;
            else
                digit = (*s - 'A') + 10;

            if (acc * base + digit < acc) {
                overflow = TRUE;
            }
            else {
                acc = acc * base + digit;
            }
        }
        s++;
    }

    /* Overflow */
    if (overflow) {
        if (sign > 0) {
            acc = ((uint64_t)1 << (8 * sizeof(int64_t) - 1)) - 1;
        }
        else {
            acc = (int64_t)((uint64_t)1 << (8 * sizeof(int64_t) - 1));
        }
        errno = ERANGE;
    }

    /* Return values */
    acc *= sign;
    if (rest)
        *rest = s;
    return acc;
} /* end HDstrtoll() */
#endif

/*-------------------------------------------------------------------------
 * Function:  HDrand/HDsrand
 *
 * Purpose:  Wrapper function for rand.  If rand_r exists on this system,
 *     use it.
 *
 *     Wrapper function for srand.  If rand_r is available, it will keep
 *     track of the seed locally instead of using srand() which modifies
 *     global state and can break other programs.
 *
 * Return:  Success:  Random number from 0 to RAND_MAX
 *
 *    Failure:  Cannot fail.
 *
 * Programmer:  Leon Arber
 *              March 6, 2006.
 *
 *-------------------------------------------------------------------------
 */
#ifdef H5_HAVE_RAND_R

static unsigned int g_seed = 42;

int
HDrand(void)
{
    return rand_r(&g_seed);
}

void
HDsrand(unsigned int seed)
{
    g_seed = seed;
}
#endif /* H5_HAVE_RAND_R */

/*-------------------------------------------------------------------------
 * Function:    Pflock
 *
 * Purpose:     Wrapper function for POSIX systems where flock(2) is not
 *              available.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
/* NOTE: Compile this all the time on POSIX systems, even when flock(2) is
 *       present so that it's less likely to become dead code.
 */
#ifdef H5_HAVE_FCNTL
int
Pflock(int fd, int operation)
{

    struct flock flk;

    /* Set the lock type */
    if (operation & LOCK_UN)
        flk.l_type = F_UNLCK;
    else if (operation & LOCK_SH)
        flk.l_type = F_RDLCK;
    else
        flk.l_type = F_WRLCK;

    /* Set the other flock struct values */
    flk.l_whence = SEEK_SET;
    flk.l_start  = 0;
    flk.l_len    = 0; /* to EOF */
    flk.l_pid    = 0; /* not used with set */

    /* Lock or unlock */
    if (HDfcntl(fd, F_SETLK, &flk) < 0)
        return -1;

    return 0;

} /* end Pflock() */
#endif /* H5_HAVE_FCNTL */

/*-------------------------------------------------------------------------
 * Function:    Nflock
 *
 * Purpose:     Wrapper function for systems where no file locking is
 *              available.
 *
 * Return:      0 (success)
 *
 *-------------------------------------------------------------------------
 */
int H5_ATTR_CONST
Nflock(int H5_ATTR_UNUSED fd, int H5_ATTR_UNUSED operation)
{
    /* just succeed */
    return 0;
} /* end Nflock() */

/*-------------------------------------------------------------------------
 * Function:    H5_make_time
 *
 * Purpose:    Portability routine to abstract converting a 'tm' struct into
 *        a time_t value.
 *
 * Note:    This is a little problematic because mktime() operates on
 *        local times.  We convert to local time and then figure out the
 *        adjustment based on the local time zone and daylight savings
 *        setting.
 *
 * Return:    Success:  The value of timezone
 *        Failure:  -1
 *
 * Programmer:  Quincey Koziol
 *              November 18, 2015
 *
 *-------------------------------------------------------------------------
 */
time_t
H5_make_time(struct tm *tm)
{
    time_t the_time;                                     /* The converted time */
#if defined(H5_HAVE_VISUAL_STUDIO) && (_MSC_VER >= 1900) /* VS 2015 */
    /* In gcc and in Visual Studio prior to VS 2015 'timezone' is a global
     * variable declared in time.h. That variable was deprecated and in
     * VS 2015 is removed, with _get_timezone replacing it.
     */
    long timezone = 0;
#endif                    /* defined(H5_HAVE_VISUAL_STUDIO) && (_MSC_VER >= 1900) */
    time_t ret_value = 0; /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Sanity check */
    HDassert(tm);

    /* Initialize timezone information */
    if (!H5_ntzset) {
        HDtzset();
        H5_ntzset = TRUE;
    } /* end if */

    /* Perform base conversion */
    if ((time_t)-1 == (the_time = HDmktime(tm)))
        HGOTO_ERROR(H5E_INTERNAL, H5E_CANTCONVERT, FAIL, "badly formatted modification time message")

        /* Adjust for timezones */
#if defined(H5_HAVE_TM_GMTOFF)
    /* BSD-like systems */
    the_time += tm->tm_gmtoff;
#elif defined(H5_HAVE_TIMEZONE)
#if defined(H5_HAVE_VISUAL_STUDIO) && (_MSC_VER >= 1900) /* VS 2015 */
    /* In gcc and in Visual Studio prior to VS 2015 'timezone' is a global
     * variable declared in time.h. That variable was deprecated and in
     * VS 2015 is removed, with _get_timezone replacing it.
     */
    _get_timezone(&timezone);
#endif /* defined(H5_HAVE_VISUAL_STUDIO) && (_MSC_VER >= 1900) */

    the_time -= timezone - (tm->tm_isdst ? 3600 : 0);
#else
    /*
     * The catch-all.  If we can't convert a character string universal
     * coordinated time to a time_t value reliably then we can't decode the
     * modification time message. This really isn't as bad as it sounds -- the
     * only way a user can get the modification time is from our internal
     * query routines, which can gracefully recover.
     */
    HGOTO_ERROR(H5E_INTERNAL, H5E_UNSUPPORTED, FAIL, "unable to obtain local timezone information")
#endif

    /* Set return value */
    ret_value = the_time;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5_make_time() */

#ifdef H5_HAVE_WIN32_API

/* Offset between 1/1/1601 and 1/1/1970 in 100 nanosecond units */
#define _W32_FT_OFFSET (116444736000000000ULL)

/*-------------------------------------------------------------------------
 * Function:  Wgettimeofday
 *
 * Purpose:  Wrapper function for gettimeofday on Windows systems
 *
 *     This function can get the time as well as a timezone
 *
 * Return:  0
 *
 *      This implementation is taken from the Cygwin source distribution at
 *          src/winsup/mingw/mingwex/gettimeofday.c
 *
 *      The original source code was contributed by
 *          Danny Smith <dannysmith@users.sourceforge.net>
 *      and released in the public domain.
 *
 * Programmer:  Scott Wegner
 *              May 19, 2009
 *
 *-------------------------------------------------------------------------
 */
int
Wgettimeofday(struct timeval *tv, struct timezone *tz)
{
    union {
        unsigned long long ns100; /*time since 1 Jan 1601 in 100ns units */
        FILETIME           ft;
    } _now;

    static int tzsetflag;

    if (tv) {
        GetSystemTimeAsFileTime(&_now.ft);
        tv->tv_usec = (long)((_now.ns100 / 10ULL) % 1000000ULL);
        tv->tv_sec  = (long)((_now.ns100 - _W32_FT_OFFSET) / 10000000ULL);
    }

    if (tz) {
        if (!tzsetflag) {
            _tzset();
            tzsetflag = 1;
        }
        tz->tz_minuteswest = _timezone / 60;
        tz->tz_dsttime     = _daylight;
    }

    /* Always return 0 as per Open Group Base Specifications Issue 6.
       Do not set errno on error.  */
    return 0;
} /* end Wgettimeofday() */

/*-------------------------------------------------------------------------
 * Function:    Wsetenv
 *
 * Purpose:     Wrapper function for setenv on Windows systems.
 *              Interestingly, getenv *is* available in the Windows
 *              POSIX layer, just not setenv.
 *
 * Return:      Success:    0
 *              Failure:    non-zero error code
 *
 * Programmer:  Dana Robinson
 *              February 2016
 *
 *-------------------------------------------------------------------------
 */
int
Wsetenv(const char *name, const char *value, int overwrite)
{
    size_t  bufsize;
    errno_t err;

    /* If we're not overwriting, check if the environment variable exists.
     * If it does (i.e.: the required buffer size to store the variable's
     * value is non-zero), then return an error code.
     */
    if (!overwrite) {
        err = getenv_s(&bufsize, NULL, 0, name);
        if (err || bufsize)
            return (int)err;
    } /* end if */

    return (int)_putenv_s(name, value);
} /* end Wsetenv() */

#ifdef H5_HAVE_WINSOCK2_H
#pragma comment(lib, "advapi32.lib")
#endif

/*-------------------------------------------------------------------------
 * Function:    H5_get_win32_times
 *
 * Purpose:     Gets the elapsed, system and user times on Windows platforms.
 *              All time values are in seconds.
 *
 * Return:      Success:  0
 *              Failure:  -1
 *
 * Programmer:  Dana Robinson
 *              May 2011
 *
 *-------------------------------------------------------------------------
 */
#ifdef H5_HAVE_WIN32_API
int
H5_get_win32_times(H5_timevals_t *tvs /*in,out*/)
{
    static HANDLE        process_handle;
    ULARGE_INTEGER       kernel_start;
    ULARGE_INTEGER       user_start;
    FILETIME             KernelTime;
    FILETIME             UserTime;
    FILETIME             CreationTime;
    FILETIME             ExitTime;
    LARGE_INTEGER        counts_start;
    static LARGE_INTEGER counts_freq;
    static hbool_t       is_initialized = FALSE;
    BOOL                 err;

    HDassert(tvs);

    if (!is_initialized) {
        /* NOTE: This is just a pseudo handle and does not need to be closed. */
        process_handle = GetCurrentProcess();
        err            = QueryPerformanceFrequency(&counts_freq);
        if (0 == err)
            return -1;
        is_initialized = TRUE;
    } /* end if */

    /*************************
     * System and user times *
     *************************/

    err = GetProcessTimes(process_handle, &CreationTime, &ExitTime, &KernelTime, &UserTime);
    if (0 == err)
        return -1;

    /* The 1.0E7 factor seems strange but it's due to the clock
     * ticking in 100 ns increments.
     */
    kernel_start.HighPart = KernelTime.dwHighDateTime;
    kernel_start.LowPart  = KernelTime.dwLowDateTime;
    tvs->system           = (double)(kernel_start.QuadPart / 1.0E7F);

    user_start.HighPart = UserTime.dwHighDateTime;
    user_start.LowPart  = UserTime.dwLowDateTime;
    tvs->user           = (double)(user_start.QuadPart / 1.0E7F);

    /****************
     * Elapsed time *
     ****************/

    err = QueryPerformanceCounter(&counts_start);
    if (0 == err)
        return -1;

    tvs->elapsed = (double)(counts_start.QuadPart) / (double)counts_freq.QuadPart;

    return 0;
} /* end H5_get_win32_times() */
#endif

#define WloginBuffer_count 256
static char Wlogin_buffer[WloginBuffer_count];

char *
Wgetlogin(void)
{

#ifdef H5_HAVE_WINSOCK2_H
    DWORD bufferCount = WloginBuffer_count;
    if (GetUserName(Wlogin_buffer, &bufferCount) != 0)
        return (Wlogin_buffer);
    else
#endif /* H5_HAVE_WINSOCK2_H */
        return NULL;
}

/*-------------------------------------------------------------------------
 * Function:    Wflock
 *
 * Purpose:     Wrapper function for flock on Windows systems
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
int
Wflock(int fd, int operation)
{

    HANDLE hFile;
    DWORD  dwFlags    = LOCKFILE_FAIL_IMMEDIATELY;
    DWORD  dwReserved = 0;
    /* MAXDWORD locks the entire file */
    DWORD nNumberOfBytesToLockLow  = MAXDWORD;
    DWORD nNumberOfBytesToLockHigh = MAXDWORD;
    /* Must initialize OVERLAPPED struct */
    OVERLAPPED overlapped = {0};

    /* Get Windows HANDLE */
    if (INVALID_HANDLE_VALUE == (hFile = (HANDLE)_get_osfhandle(fd)))
        return -1;

    /* Convert to Windows flags */
    if (operation & LOCK_EX)
        dwFlags |= LOCKFILE_EXCLUSIVE_LOCK;

    /* Lock or unlock */
    if (operation & LOCK_UN) {
        if (0 ==
            UnlockFileEx(hFile, dwReserved, nNumberOfBytesToLockLow, nNumberOfBytesToLockHigh, &overlapped)) {
            /* Attempting to unlock an already unlocked file will fail and this can happen
             * in H5Fstart_swmr_write(). For now, just ignore the "error" (error code: 0x9e / 158).
             */
            if (GetLastError() != 158)
                return -1;
        }
    }
    else {
        if (0 == LockFileEx(hFile, dwFlags, dwReserved, nNumberOfBytesToLockLow, nNumberOfBytesToLockHigh,
                            &overlapped))
            return -1;
    }

    return 0;
} /* end Wflock() */

/*-------------------------------------------------------------------------
 * Function:     H5_get_utf16_str
 *
 * Purpose:      Gets a UTF-16 string from an UTF-8 (or ASCII) string.
 *
 * Return:       Success:    A pointer to a UTF-16 string
 *                           This must be freed by the caller using H5MM_xfree()
 *               Failure:    NULL
 *
 * Programmer:  Dana Robinson
 *              Spring 2019
 *
 *-------------------------------------------------------------------------
 */
wchar_t *
H5_get_utf16_str(const char *s)
{
    int      nwchars = -1;   /* Length of the UTF-16 buffer */
    wchar_t *ret_s   = NULL; /* UTF-16 version of the string */

    /* Get the number of UTF-16 characters needed */
    if (0 == (nwchars = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0)))
        goto error;

    /* Allocate a buffer for the UTF-16 string */
    if (NULL == (ret_s = (wchar_t *)H5MM_calloc(sizeof(wchar_t) * (size_t)nwchars)))
        goto error;

    /* Convert the input UTF-8 string to UTF-16 */
    if (0 == MultiByteToWideChar(CP_UTF8, 0, s, -1, ret_s, nwchars))
        goto error;

    return ret_s;

error:
    if (ret_s)
        H5MM_xfree((void *)ret_s);
    return NULL;
} /* end H5_get_utf16_str() */

/*-------------------------------------------------------------------------
 * Function:     Wopen_utf8
 *
 * Purpose:      UTF-8 equivalent of open(2) for use on Windows.
 *               Converts a UTF-8 input path to UTF-16 and then opens the
 *               file via _wopen() under the hood
 *
 * Return:       Success:    A POSIX file descriptor
 *               Failure:    -1
 *
 * Programmer:  Dana Robinson
 *              Spring 2019
 *
 *-------------------------------------------------------------------------
 */
int
Wopen_utf8(const char *path, int oflag, ...)
{
    int      fd    = -1;   /* POSIX file descriptor to be returned */
    wchar_t *wpath = NULL; /* UTF-16 version of the path */
    int      pmode = 0;    /* mode (optionally set via variable args) */

    /* Convert the input UTF-8 path to UTF-16 */
    if (NULL == (wpath = H5_get_utf16_str(path)))
        goto done;

    /* _O_BINARY must be set in Windows to avoid CR-LF <-> LF EOL
     * transformations when performing I/O. Note that this will
     * produce Unix-style text files, though.
     */
    oflag |= _O_BINARY;

    /* Get the mode, if O_CREAT was specified */
    if (oflag & O_CREAT) {
        va_list vl;

        HDva_start(vl, oflag);
        pmode = HDva_arg(vl, int);
        HDva_end(vl);
    }

    /* Open the file */
    fd = _wopen(wpath, oflag, pmode);

done:
    if (wpath)
        H5MM_xfree((void *)wpath);

    return fd;
} /* end Wopen_utf8() */

/*-------------------------------------------------------------------------
 * Function:     Wremove_utf8
 *
 * Purpose:      UTF-8 equivalent of remove(3) for use on Windows.
 *               Converts a UTF-8 input path to UTF-16 and then opens the
 *               file via _wremove() under the hood
 *
 * Return:       Success:    0
 *               Failure:    -1
 *
 * Programmer:  Dana Robinson
 *              Spring 2019
 *
 *-------------------------------------------------------------------------
 */
int
Wremove_utf8(const char *path)
{
    wchar_t *wpath = NULL; /* UTF-16 version of the path */
    int      ret   = -1;

    /* Convert the input UTF-8 path to UTF-16 */
    if (NULL == (wpath = H5_get_utf16_str(path)))
        goto done;

    /* Open the file */
    ret = _wremove(wpath);

done:
    if (wpath)
        H5MM_xfree((void *)wpath);

    return ret;
} /* end Wremove_utf8() */

#endif /* H5_HAVE_WIN32_API */

/*-------------------------------------------------------------------------
 * Function:    H5_build_extpath
 *
 * Purpose:     To build the path for later searching of target file for external
 *              links and external files.  This path can be either:
 *                  1. The absolute path of NAME
 *                      or
 *                  2. The current working directory + relative path of NAME
 *
 * Return:      SUCCEED/FAIL
 *
 * Programmer:  Vailin Choi
 *              April 2, 2008
 *
 *-------------------------------------------------------------------------
 */
#define MAX_PATH_LEN 1024

herr_t
H5_build_extpath(const char *name, char **extpath /*out*/)
{
    char * full_path = NULL;    /* Pointer to the full path, as built or passed in */
    char * cwdpath   = NULL;    /* Pointer to the current working directory path */
    char * new_name  = NULL;    /* Pointer to the name of the file */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Sanity check */
    HDassert(name);
    HDassert(extpath);

    /* Clear external path pointer to begin with */
    *extpath = NULL;

    /*
     * Unix: name[0] is a "/"
     * Windows: name[0-2] is "<drive letter>:\" or "<drive-letter>:/"
     */
    if (H5_CHECK_ABSOLUTE(name)) {
        if (NULL == (full_path = (char *)H5MM_strdup(name)))
            HGOTO_ERROR(H5E_INTERNAL, H5E_NOSPACE, FAIL, "memory allocation failed")
    }      /* end if */
    else { /* relative pathname */
        char * retcwd;
        size_t name_len;
        int    drive;

        if (NULL == (cwdpath = (char *)H5MM_malloc(MAX_PATH_LEN)))
            HGOTO_ERROR(H5E_INTERNAL, H5E_NOSPACE, FAIL, "memory allocation failed")
        name_len = HDstrlen(name) + 1;
        if (NULL == (new_name = (char *)H5MM_malloc(name_len)))
            HGOTO_ERROR(H5E_INTERNAL, H5E_NOSPACE, FAIL, "memory allocation failed")

        /*
         * Windows: name[0-1] is "<drive-letter>:"
         *   Get current working directory on the drive specified in NAME
         * Unix: does not apply
         */
        if (H5_CHECK_ABS_DRIVE(name)) {
            drive  = HDtoupper(name[0]) - 'A' + 1;
            retcwd = HDgetdcwd(drive, cwdpath, MAX_PATH_LEN);
            HDstrncpy(new_name, &name[2], name_len);
        } /* end if */
          /*
           * Windows: name[0] is a '/' or '\'
           *  Get current drive
           * Unix: does not apply
           */
        else if (H5_CHECK_ABS_PATH(name) && (0 != (drive = HDgetdrive()))) {
            HDsnprintf(cwdpath, MAX_PATH_LEN, "%c:%c", (drive + 'A' - 1), name[0]);
            retcwd = cwdpath;
            HDstrncpy(new_name, &name[1], name_len);
        }
        /* totally relative for Unix and Windows: get current working directory  */
        else {
            retcwd = HDgetcwd(cwdpath, MAX_PATH_LEN);
            HDstrncpy(new_name, name, name_len);
        } /* end if */

        if (retcwd != NULL) {
            size_t cwdlen;
            size_t path_len;

            HDassert(cwdpath);
            cwdlen = HDstrlen(cwdpath);
            HDassert(cwdlen);
            HDassert(new_name);
            path_len = cwdlen + HDstrlen(new_name) + 2;
            if (NULL == (full_path = (char *)H5MM_malloc(path_len)))
                HGOTO_ERROR(H5E_INTERNAL, H5E_NOSPACE, FAIL, "memory allocation failed")

            HDstrncpy(full_path, cwdpath, cwdlen + 1);
            if (!H5_CHECK_DELIMITER(cwdpath[cwdlen - 1]))
                HDstrcat(full_path, H5_DIR_SEPS);
            HDstrcat(full_path, new_name);
        } /* end if */
    }     /* end else */

    /* strip out the last component (the file name itself) from the path */
    if (full_path) {
        char *ptr = NULL;

        H5_GET_LAST_DELIMITER(full_path, ptr)
        HDassert(ptr);
        *++ptr   = '\0';
        *extpath = full_path;
    } /* end if */

done:
    /* Release resources */
    if (cwdpath)
        H5MM_xfree(cwdpath);
    if (new_name)
        H5MM_xfree(new_name);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5_build_extpath() */

/*--------------------------------------------------------------------------
 * Function:    H5_combine_path
 *
 * Purpose:     If path2 is relative, interpret path2 as relative to path1
 *              and store the result in full_name. Otherwise store path2
 *              in full_name.
 *
 * Return:      SUCCEED/FAIL
 *
 * Programmer:  Steffen Kiess
 *              June 22, 2015
 *--------------------------------------------------------------------------
 */
herr_t
H5_combine_path(const char *path1, const char *path2, char **full_name /*out*/)
{
    size_t path1_len = 0;       /* length of path1 */
    size_t path2_len;           /* length of path2 */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    HDassert(path2);

    if (path1)
        path1_len = HDstrlen(path1);
    path2_len = HDstrlen(path2);

    if (path1 == NULL || *path1 == '\0' || H5_CHECK_ABSOLUTE(path2)) {

        /* If path1 is empty or path2 is absolute, simply use path2 */
        if (NULL == (*full_name = (char *)H5MM_strdup(path2)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")

    } /* end if */
    else if (H5_CHECK_ABS_PATH(path2)) {

        /* On windows path2 is a path absolute name */
        if (H5_CHECK_ABSOLUTE(path1) || H5_CHECK_ABS_DRIVE(path1)) {
            /* path1 is absolute or drive absolute and path2 is path absolute.
             * Use the drive letter of path1 + path2
             */
            if (NULL == (*full_name = (char *)H5MM_malloc(path2_len + 3)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "unable to allocate path2 buffer")
            HDsnprintf(*full_name, (path2_len + 3), "%c:%s", path1[0], path2);
        } /* end if */
        else {
            /* On windows path2 is path absolute name ("\foo\bar"),
             * path1 does not have a drive letter (i.e. is "a\b" or "\a\b").
             * Use path2.
             */
            if (NULL == (*full_name = (char *)H5MM_strdup(path2)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")
        } /* end else */

    } /* end else if */
    else {

        /* Relative path2:
         * Allocate a buffer to hold path1 + path2 + possibly the delimiter
         *      + terminating null byte
         */
        if (NULL ==
            (*full_name = (char *)H5MM_malloc(path1_len + path2_len + 2 +
                                              2))) /* Extra "+2" to quiet GCC warning - 2019/07/05, QAK */
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "unable to allocate filename buffer")

        /* Compose the full file name */
        HDsnprintf(*full_name, (path1_len + path2_len + 2 + 2), "%s%s%s",
                   path1, /* Extra "+2" to quiet GCC warning - 2019/07/05, QAK */
                   (H5_CHECK_DELIMITER(path1[path1_len - 1]) ? "" : H5_DIR_SEPS), path2);
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5_combine_path() */

/*--------------------------------------------------------------------------
 * Function:    H5_nanosleep
 *
 * Purpose:     Sleep for a given # of nanoseconds
 *
 *              Note that commodity hardware is probably going to have a
 *              resolution of milliseconds, not nanoseconds.
 *
 * Return:      SUCCEED/FAIL
 *
 * Programmer:  Quincey Koziol
 *              October 01, 2016
 *--------------------------------------------------------------------------
 */
void
H5_nanosleep(uint64_t nanosec)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

#ifdef H5_HAVE_WIN32_API

    /* On Windows, Sleep() is in milliseconds. Passing 0 to Sleep()
     * causes the thread to relinquish the rest of its time slice.
     */
    Sleep(nanosec / (1000 * 1000));

#else
    {
        struct timespec sleeptime; /* Struct to hold time to sleep */

        /* Set up time to sleep */
        sleeptime.tv_sec  = 0;
        sleeptime.tv_nsec = (long)nanosec;

        HDnanosleep(&sleeptime, NULL);
    }
#endif

    FUNC_LEAVE_NOAPI_VOID
} /* end H5_nanosleep() */

#ifdef H5_HAVE_WIN32_API

#define H5_WIN32_ENV_VAR_BUFFER_SIZE 32767

/*-------------------------------------------------------------------------
 * Function:    H5_expand_windows_env_vars()
 *
 * Purpose:     Replaces windows environment variables of the form %foo%
 *              with user-specific values.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5_expand_windows_env_vars(char **env_var)
{
    long   n_chars   = 0;
    char * temp_buf  = NULL;
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Allocate buffer for expanded environment variable string */
    if (NULL == (temp_buf = (char *)H5MM_calloc((size_t)H5_WIN32_ENV_VAR_BUFFER_SIZE)))
        HGOTO_ERROR(H5E_PLUGIN, H5E_CANTALLOC, FAIL, "can't allocate memory for expanded path")

    /* Expand the environment variable string */
    if ((n_chars = ExpandEnvironmentStringsA(*env_var, temp_buf, H5_WIN32_ENV_VAR_BUFFER_SIZE)) >
        H5_WIN32_ENV_VAR_BUFFER_SIZE)
        HGOTO_ERROR(H5E_PLUGIN, H5E_NOSPACE, FAIL, "expanded path is too long")

    if (0 == n_chars)
        HGOTO_ERROR(H5E_PLUGIN, H5E_CANTGET, FAIL, "failed to expand path")

    *env_var = (char *)H5MM_xfree(*env_var);
    *env_var = temp_buf;

done:
    if (FAIL == ret_value && temp_buf)
        temp_buf = (char *)H5MM_xfree(temp_buf);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5_expand_windows_env_vars() */
#endif /* H5_HAVE_WIN32_API */
