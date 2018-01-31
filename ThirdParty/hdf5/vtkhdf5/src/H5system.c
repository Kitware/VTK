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

/*-------------------------------------------------------------------------
 *
 * Created:    H5system.c
 *      Aug 21 2006
 *      Quincey Koziol <koziol@hdfgroup.org>
 *
 * Purpose:    System call wrapper implementations.
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/


/***********/
/* Headers */
/***********/
#include "H5private.h"        /* Generic Functions            */
#include "H5Eprivate.h"        /* Error handling              */
#include "H5Fprivate.h"        /* File access                */
#include "H5MMprivate.h"    /* Memory management            */


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


/*-------------------------------------------------------------------------
 * Function:  HDfprintf
 *
 * Purpose:  Prints the optional arguments under the control of the format
 *    string FMT to the stream STREAM.  This function takes the
 *    same format as fprintf(3c) with a few added features:
 *
 *    The conversion modifier `H' refers to the size of an
 *    `hsize_t' or `hssize_t' type.  For instance, "0x%018Hx"
 *    prints an `hsize_t' value as a hex number right justified and
 *    zero filled in an 18-character field.
 *
 *    The conversion `a' refers to an `haddr_t' type.
 *
 * Return:  Success:  Number of characters printed
 *
 *    Failure:  -1
 *
 * Programmer:  Robb Matzke
 *              Thursday, April  9, 1998
 *
 * Modifications:
 *    Robb Matzke, 1999-07-27
 *    The `%a' format refers to an argument of `haddr_t' type
 *    instead of `haddr_t*' and the return value is correct.
 *-------------------------------------------------------------------------
 */
int
HDfprintf(FILE *stream, const char *fmt, ...)
{
    int    n=0, nout = 0;
    int    fwidth, prec;
    int    zerofill;
    int    leftjust;
    int    plussign;
    int    ldspace;
    int    prefix;
    char  modifier[8];
    int    conv;
    char  *rest, format_templ[128];
    int    len;
    const char  *s;
    va_list  ap;

    HDassert(stream);
    HDassert(fmt);

    va_start (ap, fmt);
    while (*fmt) {
        fwidth = prec = 0;
        zerofill = 0;
        leftjust = 0;
        plussign = 0;
        prefix = 0;
        ldspace = 0;
        modifier[0] = '\0';

        if ('%'==fmt[0] && '%'==fmt[1]) {
            HDputc ('%', stream);
            fmt += 2;
            nout++;
        } else if ('%'==fmt[0]) {
            s = fmt + 1;

            /* Flags */
            while(HDstrchr("-+ #", *s)) {
                switch(*s) {
                    case '-':
                        leftjust = 1;
                        break;

                    case '+':
                        plussign = 1;
                        break;

                    case ' ':
                        ldspace = 1;
                        break;

                    case '#':
                        prefix = 1;
                        break;

                    default:
                        HDassert(0 && "Unknown format flag");
                } /* end switch */ /*lint !e744 Switch statement doesn't _need_ default */
                s++;
            } /* end while */

            /* Field width */
            if(HDisdigit(*s)) {
                zerofill = ('0' == *s);
                fwidth = (int)HDstrtol (s, &rest, 10);
                s = rest;
            } /* end if */
            else if ('*'==*s) {
                fwidth = va_arg(ap, int);
                if(fwidth < 0) {
                    leftjust = 1;
                    fwidth = -fwidth;
                }
                s++;
            }

            /* Precision */
            if('.'==*s) {
                s++;
                if(HDisdigit(*s)) {
                    prec = (int)HDstrtol(s, &rest, 10);
                    s = rest;
                } else if('*'==*s) {
                    prec = va_arg(ap, int);
                    s++;
                }
                if(prec < 1)
                    prec = 1;
            }

            /* Extra type modifiers */
            if(HDstrchr("zZHhlqLI", *s)) {
                switch(*s) {
                    /*lint --e{506} Don't issue warnings about constant value booleans */
                    /*lint --e{774} Don't issue warnings boolean within 'if' always evaluates false/true */
                    case 'H':
                        if(sizeof(hsize_t) < sizeof(long))
                            modifier[0] = '\0';
                        else if(sizeof(hsize_t) == sizeof(long)) {
                            HDstrncpy(modifier, "l", sizeof(modifier));
                            modifier[sizeof(modifier) - 1] = '\0';
                        } /* end if */
                        else {
                            HDstrncpy(modifier, H5_PRINTF_LL_WIDTH, sizeof(modifier));
                            modifier[sizeof(modifier) - 1] = '\0';
                        } /* end else */
                        break;

                    case 'Z':
                    case 'z':
                        if(sizeof(size_t) < sizeof(long))
                            modifier[0] = '\0';
                        else if(sizeof(size_t) == sizeof(long)) {
                            HDstrncpy(modifier, "l", sizeof(modifier));
                            modifier[sizeof(modifier) - 1] = '\0';
                        } /* end if */
                        else {
                            HDstrncpy(modifier, H5_PRINTF_LL_WIDTH, sizeof(modifier));
                            modifier[sizeof(modifier) - 1] = '\0';
                        } /* end else */
                        break;

                    default:
                        /* Handle 'I64' modifier for Microsoft's "__int64" type */
                        if(*s=='I' && *(s+1)=='6' && *(s+2)=='4') {
                            modifier[0] = *s;
                            modifier[1] = *(s+1);
                            modifier[2] = *(s+2);
                            modifier[3] = '\0';
                            s += 2; /* Increment over 'I6', the '4' is taken care of below */
                        } /* end if */
                        else {
                            /* Handle 'll' for long long types */
                            if(*s=='l' && *(s+1)=='l') {
                                modifier[0] = *s;
                                modifier[1] = *s;
                                modifier[2] = '\0';
                                s++; /* Increment over first 'l', second is taken care of below */
                            } /* end if */
                            else {
                                modifier[0] = *s;
                                modifier[1] = '\0';
                            } /* end else */
                        } /* end else */
                        break;
                }
                s++;
            }

            /* Conversion */
            conv = *s++;

            /* Create the format template */
            len = 0;
            len += HDsnprintf(format_templ, (sizeof(format_templ) - (size_t)(len + 1)), "%%%s%s%s%s%s", (leftjust ? "-" : ""),
                    (plussign ? "+" : ""), (ldspace ? " " : ""),
                    (prefix ? "#" : ""), (zerofill ? "0" : ""));
            if(fwidth > 0)
                len += HDsnprintf(format_templ + len, (sizeof(format_templ) - (size_t)(len + 1)), "%d", fwidth);
            if(prec > 0)
                len += HDsnprintf(format_templ + len, (sizeof(format_templ) - (size_t)(len + 1)), ".%d", prec);
            if(*modifier)
                len += HDsnprintf(format_templ + len, (sizeof(format_templ) - (size_t)(len + 1)), "%s", modifier);
            HDsnprintf(format_templ + len, (sizeof(format_templ) - (size_t)(len + 1)), "%c", conv);

            /* Conversion */
            switch (conv) {
                case 'd':
                case 'i':
                    if(!HDstrcmp(modifier, "h")) {
                        short x = (short)va_arg(ap, int);
                        n = fprintf(stream, format_templ, x);
                    } else if(!*modifier) {
                        int x = va_arg(ap, int);
                        n = fprintf(stream, format_templ, x);
                    } else if(!HDstrcmp(modifier, "l")) {
                        long x = va_arg(ap, long);
                        n = fprintf(stream, format_templ, x);
                    } else {
                        int64_t x = va_arg(ap, int64_t);
                        n = fprintf(stream, format_templ, x);
                    }
                    break;

                case 'o':
                case 'u':
                case 'x':
                case 'X':
                    if(!HDstrcmp(modifier, "h")) {
                        unsigned short x = (unsigned short)va_arg(ap, unsigned int);
                        n = fprintf(stream, format_templ, x);
                    } else if(!*modifier) {
                        unsigned int x = va_arg(ap, unsigned int); /*lint !e732 Loss of sign not really occuring */
                        n = fprintf(stream, format_templ, x);
                    } else if(!HDstrcmp(modifier, "l")) {
                        unsigned long x = va_arg(ap, unsigned long); /*lint !e732 Loss of sign not really occuring */
                        n = fprintf(stream, format_templ, x);
                    } else {
                        uint64_t x = va_arg(ap, uint64_t); /*lint !e732 Loss of sign not really occuring */
                        n = fprintf(stream, format_templ, x);
                    }
                    break;

                case 'f':
                case 'e':
                case 'E':
                case 'g':
                case 'G':
                    if(!HDstrcmp(modifier, "h")) {
                        float x = (float)va_arg(ap, double);
                        n = fprintf(stream, format_templ, (double)x);
                    } else if(!*modifier || !HDstrcmp(modifier, "l")) {
                        double x = va_arg(ap, double);
                        n = fprintf(stream, format_templ, x);
                    } else {
                    /*
                    * Some compilers complain when `long double' and
                    * `double' are the same thing.
                    */
#if H5_SIZEOF_LONG_DOUBLE != H5_SIZEOF_DOUBLE
                        long double x = va_arg(ap, long double);
                        n = fprintf(stream, format_templ, x);
#else
                        double x = va_arg(ap, double);
                        n = fprintf(stream, format_templ, x);
#endif
                    }
                    break;

                case 'a':
                    {
                        haddr_t x = va_arg(ap, haddr_t); /*lint !e732 Loss of sign not really occuring */

                        if(H5F_addr_defined(x)) {
                            len = 0;
                            len += HDsnprintf(format_templ, (sizeof(format_templ) - (size_t)(len + 1)), "%%%s%s%s%s%s",
                                (leftjust ? "-" : ""), (plussign ? "+" : ""),
                                (ldspace ? " " : ""), (prefix ? "#" : ""),
                                (zerofill ? "0" : ""));
                            if(fwidth > 0)
                                len += HDsnprintf(format_templ + len, (sizeof(format_templ) - (size_t)(len + 1)), "%d", fwidth);

                            /*lint --e{506} Don't issue warnings about constant value booleans */
                            /*lint --e{774} Don't issue warnings boolean within 'if' always evaluates false/true */
                            if(sizeof(x) == H5_SIZEOF_INT) {
                                HDstrncat(format_templ, "u", (sizeof(format_templ) - (size_t)(len + 1)));
                                len++;
                            } /* end if */
                            else if(sizeof(x) == H5_SIZEOF_LONG) {
                                HDstrncat(format_templ, "lu", (sizeof(format_templ) - (size_t)(len + 1)));
                                len++;
                            } /* end if */
                            else if(sizeof(x) == H5_SIZEOF_LONG_LONG) {
                                HDstrncat(format_templ, H5_PRINTF_LL_WIDTH, (sizeof(format_templ) - (size_t)(len + 1)));
                                len += (int)sizeof(H5_PRINTF_LL_WIDTH);
                                HDstrncat(format_templ, "u", (sizeof(format_templ) - (size_t)(len + 1)));
                                len++;
                            }
                            n = fprintf(stream, format_templ, x);
                        } else {
                            len = 0;
                            HDstrncpy(format_templ, "%", (sizeof(format_templ) - (size_t)(len + 1)));
                            len++;
                            if(leftjust) {
                                HDstrncat(format_templ, "-", (sizeof(format_templ) - (size_t)(len + 1)));
                                len++;
                            } /* end if */
                            if(fwidth)
                                len += HDsnprintf(format_templ + len, (sizeof(format_templ) - (size_t)(len + 1)),  "%d", fwidth);
                            HDstrncat(format_templ, "s", (sizeof(format_templ) - (size_t)(len + 1)));
                            fprintf(stream, format_templ, "UNDEF");
                        }
                    }
                    break;

                case 'c':
                    {
                        char x = (char)va_arg(ap, int);
                        n = fprintf(stream, format_templ, x);
                    }
                    break;

                case 's':
                case 'p':
                    {
                        char *x = va_arg(ap, char*); /*lint !e64 Type mismatch not really occuring */
                        n = fprintf(stream, format_templ, x);
                    }
                    break;

                case 'n':
                    format_templ[HDstrlen(format_templ) - 1] = 'u';
                    n = fprintf(stream, format_templ, nout);
                    break;

                case 't':
                    {
                        htri_t tri_var = va_arg(ap, htri_t);

                        if(tri_var > 0)
                            fprintf(stream, "TRUE");
                        else if(!tri_var)
                            fprintf(stream, "FALSE");
                        else
                            fprintf(stream, "FAIL(%d)", (int)tri_var);
                    }
                    break;

                default:
                    HDfputs(format_templ, stream);
                    n = (int)HDstrlen(format_templ);
                    break;
            }
            nout += n;
            fmt = s;
        } else {
            HDputc(*fmt, stream);
            fmt++;
            nout++;
        }
    }
    va_end(ap);
    return nout;
} /* end HDfprintf() */


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
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
#ifndef HDstrtoll
int64_t
HDstrtoll(const char *s, const char **rest, int base)
{
    int64_t  sign=1, acc=0;
    hbool_t  overflow = FALSE;

    errno = 0;
    if (!s || (base && (base<2 || base>36))) {
        if (rest)
            *rest = s;
        return 0;
    }

    /* Skip white space */
    while (HDisspace (*s)) s++;

    /* Optional minus or plus sign */
    if ('+'==*s) {
        s++;
    } else if ('-'==*s) {
        sign = -1;
        s++;
    }

    /* Zero base prefix */
    if (0==base && '0'==*s && ('x'==s[1] || 'X'==s[1])) {
        base = 16;
        s += 2;
    } else if (0==base && '0'==*s) {
        base = 8;
        s++;
    } else if (0==base) {
        base = 10;
    }

    /* Digits */
    while ((base<=10 && *s>='0' && *s<'0'+base) ||
     (base>10 && ((*s>='0' && *s<='9') ||
      (*s>='a' && *s<'a'+base-10) ||
      (*s>='A' && *s<'A'+base-10)))) {
        if (!overflow) {
            int64_t digit = 0;

            if (*s>='0' && *s<='9')
                digit = *s - '0';
            else if (*s>='a' && *s<='z')
                digit = (*s-'a')+10;
            else
                digit = (*s-'A')+10;

            if (acc*base+digit < acc) {
                overflow = TRUE;
            } else {
                acc = acc*base + digit;
            }
        }
        s++;
    }

    /* Overflow */
    if (overflow) {
        if (sign>0) {
            acc = ((uint64_t)1<<(8*sizeof(int64_t)-1))-1;
        } else {
            acc = (int64_t)((uint64_t)1<<(8*sizeof(int64_t)-1));
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

int HDrand(void)
{
    return rand_r(&g_seed);
}

void HDsrand(unsigned int seed)
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
Pflock(int fd, int operation) {

    struct flock    flk;

    /* Set the lock type */
    if(operation & LOCK_UN)
        flk.l_type = F_UNLCK;
    else if(operation & LOCK_SH)
        flk.l_type = F_RDLCK;
    else
        flk.l_type = F_WRLCK;

    /* Set the other flock struct values */
    flk.l_whence = SEEK_SET;
    flk.l_start = 0;
    flk.l_len = 0;              /* to EOF */
    flk.l_pid = 0;              /* not used with set */

    /* Lock or unlock */
    if(HDfcntl(fd, F_SETLK, flk) < 0)
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
 * Return:      Failure:    -1 (always fails)
 *
 *-------------------------------------------------------------------------
 */
int H5_ATTR_CONST
Nflock(int H5_ATTR_UNUSED fd, int H5_ATTR_UNUSED operation) {
    /* just fail */
    return -1;
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
    time_t the_time;    /* The converted time */
#if defined(H5_HAVE_VISUAL_STUDIO) && (_MSC_VER >= 1900)  /* VS 2015 */
    /* In gcc and in Visual Studio prior to VS 2015 'timezone' is a global
     * variable declared in time.h. That variable was deprecated and in
     * VS 2015 is removed, with _get_timezone replacing it.
     */
    long timezone = 0;
#endif /* defined(H5_HAVE_VISUAL_STUDIO) && (_MSC_VER >= 1900) */
    time_t ret_value;   /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Sanity check */
    HDassert(tm);

    /* Initialize timezone information */
    if(!H5_ntzset) {
        HDtzset();
        H5_ntzset = TRUE;
    } /* end if */

    /* Perform base conversion */
    if((time_t)-1 == (the_time = HDmktime(tm)))
        HGOTO_ERROR(H5E_INTERNAL, H5E_CANTCONVERT, FAIL, "badly formatted modification time message")

    /* Adjust for timezones */
#if defined(H5_HAVE_TM_GMTOFF)
    /* BSD-like systems */
    the_time += tm->tm_gmtoff;
#elif defined(H5_HAVE_TIMEZONE)
#if defined(H5_HAVE_VISUAL_STUDIO) && (_MSC_VER >= 1900)  /* VS 2015 */
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
    FILETIME ft;
  }  _now;

    static int tzsetflag;

    if(tv) {
      GetSystemTimeAsFileTime (&_now.ft);
      tv->tv_usec=(long)((_now.ns100 / 10ULL) % 1000000ULL );
      tv->tv_sec= (long)((_now.ns100 - _W32_FT_OFFSET) / 10000000ULL);
    }

    if(tz) {
        if(!tzsetflag) {
            _tzset();
            tzsetflag = 1;
        }
        tz->tz_minuteswest = _timezone / 60;
        tz->tz_dsttime = _daylight;
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
    size_t bufsize;
    errno_t err;

    /* If we're not overwriting, check if the environment variable exists.
     * If it does (i.e.: the required buffer size to store the variable's
     * value is non-zero), then return an error code.
     */
    if(!overwrite) {
        err = getenv_s(&bufsize, NULL, 0, name);
        if (err || bufsize)
            return (int)err;
    } /* end if */

    return (int)_putenv_s(name, value);
} /* end Wsetenv() */

#ifdef H5_HAVE_WINSOCK2_H
#pragma comment(lib, "advapi32.lib")
#endif

#define WloginBuffer_count 256
static char Wlogin_buffer[WloginBuffer_count];

char*
Wgetlogin()
{

#ifdef H5_HAVE_WINSOCK2_H
    long bufferCount = WloginBuffer_count;
    if (GetUserName(Wlogin_buffer, &bufferCount) == 0)
        return (Wlogin_buffer);
    else
#endif /* H5_HAVE_WINSOCK2_H */
        return NULL;
}

int c99_snprintf(char* str, size_t size, const char* format, ...)
{
    int count;
    va_list ap;

    va_start(ap, format);
    count = c99_vsnprintf(str, size, format, ap);
    va_end(ap);

    return count;
}

int c99_vsnprintf(char* str, size_t size, const char* format, va_list ap)
{
    int count = -1;

    if (size != 0)
        count = _vsnprintf_s(str, size, _TRUNCATE, format, ap);
    if (count == -1)
        count = _vscprintf(format, ap);

    return count;
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
Wflock(int H5_ATTR_UNUSED fd, int H5_ATTR_UNUSED operation) {

/* This is a no-op while we implement a Win32 VFD */
#if 0
int
Wflock(int fd, int operation) {

    HANDLE          hFile;
    DWORD           dwFlags = LOCKFILE_FAIL_IMMEDIATELY;
    DWORD           dwReserved = 0;
                    /* MAXDWORD for entire file */
    DWORD           nNumberOfBytesToLockLow = MAXDWORD;
    DWORD           nNumberOfBytesToLockHigh = MAXDWORD;
                    /* Must initialize OVERLAPPED struct */
    OVERLAPPED      overlapped = {0};

    /* Get Windows HANDLE */
    hFile = _get_osfhandle(fd);

    /* Convert to Windows flags */
    if(operation & LOCK_EX)
        dwFlags |= LOCKFILE_EXCLUSIVE_LOCK;

    /* Lock or unlock */
    if(operation & LOCK_UN)
        if(0 == UnlockFileEx(hFile, dwReserved, nNumberOfBytesToLockLow,
                            nNumberOfBytesToLockHigh, &overlapped))
            return -1;
    else
        if(0 == LockFileEx(hFile, dwFlags, dwReserved, nNumberOfBytesToLockLow,
                            nNumberOfBytesToLockHigh, &overlapped))
            return -1;
#endif /* 0 */
    return 0;
} /* end Wflock() */


 /*--------------------------------------------------------------------------
  * Function:    Wnanosleep
  *
  * Purpose:     Sleep for a given # of nanoseconds (Windows version)
  *
  * Return:      SUCCEED/FAIL
  *
  * Programmer:  Dana Robinson
  *              Fall 2016
  *--------------------------------------------------------------------------
  */
int
Wnanosleep(const struct timespec *req, struct timespec *rem)
{
    /* XXX: Currently just a placeholder */
    return 0;

} /* end Wnanosleep() */


/*-------------------------------------------------------------------------
 * Function:    Wllround, Wllroundf, Wlround, Wlroundf, Wround, Wroundf
 *
 * Purpose:     Wrapper function for round functions for use with VS2012
 *              and earlier.
 *
 * Return:      The rounded value that was passed in.
 *
 * Programmer:  Dana Robinson
 *              December 2016
 *
 *-------------------------------------------------------------------------
 */
long long
Wllround(double arg)
{
    return (long long)(arg < 0.0 ? HDceil(arg - 0.5) : HDfloor(arg + 0.5));
}

long long
Wllroundf(float arg)
{
    return (long long)(arg < 0.0F ? HDceil(arg - 0.5F) : HDfloor(arg + 0.5F));
}

long
Wlround(double arg)
{
    return (long)(arg < 0.0 ? HDceil(arg - 0.5) : HDfloor(arg + 0.5));
}

long
Wlroundf(float arg)
{
    return (long)(arg < 0.0F ? HDceil(arg - 0.5F) : HDfloor(arg + 0.5F));
}

double
Wround(double arg)
{
    return arg < 0.0 ? HDceil(arg - 0.5) : HDfloor(arg + 0.5);
}

float
Wroundf(float arg)
{
    return arg < 0.0F ? HDceil(arg - 0.5F) : HDfloor(arg + 0.5F);
}

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
#define MAX_PATH_LEN     1024

herr_t
H5_build_extpath(const char *name, char **extpath /*out*/)
{
    char        *full_path = NULL;      /* Pointer to the full path, as built or passed in */
    char        *cwdpath = NULL;        /* Pointer to the current working directory path */
    char        *new_name = NULL;       /* Pointer to the name of the file */
    herr_t      ret_value = SUCCEED;    /* Return value */

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
    if(H5_CHECK_ABSOLUTE(name)) {
        if(NULL == (full_path = (char *)H5MM_strdup(name)))
            HGOTO_ERROR(H5E_INTERNAL, H5E_NOSPACE, FAIL, "memory allocation failed")
    } /* end if */
    else { /* relative pathname */
        char *retcwd;
        size_t name_len;
        int drive;

        if(NULL == (cwdpath = (char *)H5MM_malloc(MAX_PATH_LEN)))
            HGOTO_ERROR(H5E_INTERNAL, H5E_NOSPACE, FAIL, "memory allocation failed")
        name_len = HDstrlen(name) + 1;
        if(NULL == (new_name = (char *)H5MM_malloc(name_len)))
            HGOTO_ERROR(H5E_INTERNAL, H5E_NOSPACE, FAIL, "memory allocation failed")

        /*
         * Windows: name[0-1] is "<drive-letter>:"
         *   Get current working directory on the drive specified in NAME
         * Unix: does not apply
         */
        if(H5_CHECK_ABS_DRIVE(name)) {
            drive = name[0] - 'A' + 1;
            retcwd = HDgetdcwd(drive, cwdpath, MAX_PATH_LEN);
            HDstrncpy(new_name, &name[2], name_len);
        } /* end if */
       /*
        * Windows: name[0] is a '/' or '\'
        *  Get current drive
        * Unix: does not apply
        */
        else if(H5_CHECK_ABS_PATH(name) && (0 != (drive = HDgetdrive()))) {
            HDsnprintf(cwdpath, MAX_PATH_LEN, "%c:%c", (drive + 'A' - 1), name[0]);
            retcwd = cwdpath;
            HDstrncpy(new_name, &name[1], name_len);
        }
        /* totally relative for Unix and Windows: get current working directory  */
        else {
            retcwd = HDgetcwd(cwdpath, MAX_PATH_LEN);
            HDstrncpy(new_name, name, name_len);
        } /* end if */

        if(retcwd != NULL) {
            size_t cwdlen;
            size_t path_len;

            HDassert(cwdpath);
            cwdlen = HDstrlen(cwdpath);
            HDassert(cwdlen);
            HDassert(new_name);
            path_len = cwdlen + HDstrlen(new_name) + 2;
            if(NULL == (full_path = (char *)H5MM_malloc(path_len)))
                HGOTO_ERROR(H5E_INTERNAL, H5E_NOSPACE, FAIL, "memory allocation failed")

            HDstrncpy(full_path, cwdpath, cwdlen + 1);
            if(!H5_CHECK_DELIMITER(cwdpath[cwdlen - 1]))
                HDstrncat(full_path, H5_DIR_SEPS, HDstrlen(H5_DIR_SEPS));
            HDstrncat(full_path, new_name, HDstrlen(new_name));
        } /* end if */
    } /* end else */

    /* strip out the last component (the file name itself) from the path */
    if(full_path) {
        char *ptr = NULL;

        H5_GET_LAST_DELIMITER(full_path, ptr)
        HDassert(ptr);
        *++ptr = '\0';
        *extpath = full_path;
    } /* end if */

done:
    /* Release resources */
    if(cwdpath)
        H5MM_xfree(cwdpath);
    if(new_name)
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
H5_combine_path(const char* path1, const char* path2, char **full_name /*out*/)
{
    size_t      path1_len;            /* length of path1 */
    size_t      path2_len;            /* length of path2 */
    herr_t      ret_value = SUCCEED;  /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    HDassert(path1);
    HDassert(path2);

    path1_len = HDstrlen(path1);
    path2_len = HDstrlen(path2);

    if(*path1 == '\0' || H5_CHECK_ABSOLUTE(path2)) {

        /* If path1 is empty or path2 is absolute, simply use path2 */
        if(NULL == (*full_name = (char *)H5MM_strdup(path2)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")

    } /* end if */
    else if(H5_CHECK_ABS_PATH(path2)) {

        /* On windows path2 is a path absolute name */
        if (H5_CHECK_ABSOLUTE(path1) || H5_CHECK_ABS_DRIVE(path1)) {
            /* path1 is absolute or drive absolute and path2 is path absolute.
             * Use the drive letter of path1 + path2
             */
            if(NULL == (*full_name = (char *)H5MM_malloc(path2_len + 3)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "unable to allocate path2 buffer")
            HDsnprintf(*full_name, (path2_len + 3), "%c:%s", path1[0], path2);
        } /* end if */
        else {
            /* On windows path2 is path absolute name ("\foo\bar"),
             * path1 does not have a drive letter (i.e. is "a\b" or "\a\b").
             * Use path2.
             */
            if(NULL == (*full_name = (char *)H5MM_strdup(path2)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")
        } /* end else */

    } /* end else if */
    else {

        /* Relative path2:
         * Allocate a buffer to hold path1 + path2 + possibly the delimiter
         *      + terminating null byte
         */
        if(NULL == (*full_name = (char *)H5MM_malloc(path1_len + path2_len + 2)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "unable to allocate filename buffer")

        /* Compose the full file name */
        HDsnprintf(*full_name, (path1_len + path2_len + 2), "%s%s%s", path1,
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
 * Return:      SUCCEED/FAIL
 *
 * Programmer:  Quincey Koziol
 *              October 01, 2016
 *--------------------------------------------------------------------------
 */
void
H5_nanosleep(uint64_t nanosec)
{
    struct timespec sleeptime;  /* Struct to hold time to sleep */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Set up time to sleep */
    sleeptime.tv_sec = 0;
    sleeptime.tv_nsec = (long)nanosec;

    HDnanosleep(&sleeptime, NULL);

    FUNC_LEAVE_NOAPI_VOID
} /* end H5_nanosleep() */


/*--------------------------------------------------------------------------
 * Function:    H5_get_time
 *
 * Purpose:     Get the current time, as the time of seconds after the UNIX epoch
 *
 * Return:      SUCCEED/FAIL
 *
 * Programmer:  Quincey Koziol
 *              October 05, 2016
 *--------------------------------------------------------------------------
 */
double
H5_get_time(void)
{
#ifdef H5_HAVE_GETTIMEOFDAY
    struct timeval curr_time;
#endif /* H5_HAVE_GETTIMEOFDAY */
    double ret_value = (double)0.0f;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

#ifdef H5_HAVE_GETTIMEOFDAY
    HDgettimeofday(&curr_time, NULL);

    ret_value = (double)curr_time.tv_sec + ((double)curr_time.tv_usec / (double)1000000.0f);
#endif /* H5_HAVE_GETTIMEOFDAY */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5_get_time() */


