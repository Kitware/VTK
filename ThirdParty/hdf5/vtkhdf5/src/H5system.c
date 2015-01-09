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
#include "H5private.h"    /* Generic Functions      */
#include "H5Fprivate.h"    /* File access        */
#include "H5MMprivate.h"  /* Memory management      */
#include "H5Eprivate.h"



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
                fwidth = va_arg (ap, int);
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
                        short x = (short)va_arg (ap, int);
                        n = fprintf (stream, format_templ, x);
                    } else if(!*modifier) {
                        int x = va_arg (ap, int);
                        n = fprintf (stream, format_templ, x);
                    } else if(!HDstrcmp (modifier, "l")) {
                        long x = va_arg (ap, long);
                        n = fprintf (stream, format_templ, x);
                    } else {
                        int64_t x = va_arg(ap, int64_t);
                        n = fprintf (stream, format_templ, x);
                    }
                    break;

                case 'o':
                case 'u':
                case 'x':
                case 'X':
                    if(!HDstrcmp(modifier, "h")) {
                        unsigned short x = (unsigned short)va_arg (ap, unsigned int);
                        n = fprintf(stream, format_templ, x);
                    } else if(!*modifier) {
                        unsigned int x = va_arg (ap, unsigned int); /*lint !e732 Loss of sign not really occuring */
                        n = fprintf(stream, format_templ, x);
                    } else if(!HDstrcmp(modifier, "l")) {
                        unsigned long x = va_arg (ap, unsigned long); /*lint !e732 Loss of sign not really occuring */
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
                        n = fprintf(stream, format_templ, x);
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
                        haddr_t x = va_arg (ap, haddr_t); /*lint !e732 Loss of sign not really occuring */

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
                            fprintf (stream, "TRUE");
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
#endif


/*-------------------------------------------------------------------------
 * Function:  HDremove_all
 *
 * Purpose:  Wrapper function for remove on VMS systems
 *
 *     This function deletes all versions of a file
 *
 * Return:  Success:        0;
 *
 *    Failure:  -1
 *
 * Programmer:  Elena Pourmal
 *              March 22, 2006
 *
 *-------------------------------------------------------------------------
 */
#ifdef H5_VMS
int
HDremove_all(const char *fname)
{
    int ret_value = -1;
    size_t fname_len;
    char *_fname;

    fname_len = HDstrlen(fname) + 3;    /* to accomodate ";*" and null terminator */
    _fname = (char *)H5MM_malloc(fname_len);
    if(_fname) {
        HDsnprintf(_fname, fname_len, "%s;*", fname);
        /* Do not use HDremove; function becomes recursive (see H5private.h file)*/
        remove(_fname);
        H5MM_xfree(_fname);
        ret_value = 0;
    }
    return ret_value;
}
#endif

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
#ifdef H5_HAVE_VISUAL_STUDIO

/* Offset between 1/1/1601 and 1/1/1970 in 100 nanosecond units */
#define _W32_FT_OFFSET (116444736000000000ULL)

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
}

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

int HDsnprintf(char* str, size_t size, const char* format, ...)
{
    int count;
    va_list ap;

    va_start(ap, format);
    count = HDvsnprintf(str, size, format, ap);
    va_end(ap);

    return count;
}

int HDvsnprintf(char* str, size_t size, const char* format, va_list ap)
{
    int count = -1;

    if (size != 0)
    {
#if (_MSC_VER > 1310)
    count = _vsnprintf_s(str, size, _TRUNCATE, format, ap);
#else
    /* Visual Studio 2003 7.1 does not define vsnprintf_s */
    size_t c = size - 1;
    count = _vsnprintf(str, c, format, ap);
#endif
    }
    if (count == -1)
        count = _vscprintf(format, ap);

    return count;
}

#endif


/*-------------------------------------------------------------------------
 * Function: H5_build_extpath
 *
 * Purpose:  To build the path for later searching of target file for external
 *    link.  This path can be either:
 *                  1. The absolute path of NAME
 *                      or
 *                  2. The current working directory + relative path of NAME
 *
 * Return:  Success:        0
 *    Failure:  -1
 *
 * Programmer:  Vailin Choi
 *    April 2, 2008
 *
 *-------------------------------------------------------------------------
 */
#define MAX_PATH_LEN     1024

herr_t
H5_build_extpath(const char *name, char **extpath/*out*/)
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
     * OpenVMS: <disk name>$<partition>:[path]<file name>
     *     i.g. SYS$SYSUSERS:[LU.HDF5.SRC]H5system.c
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
         * OpenVMS: does not apply
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
        * OpenVMS: does not apply
        */
        else if(H5_CHECK_ABS_PATH(name) && (0 != (drive = HDgetdrive()))) {
            HDsnprintf(cwdpath, MAX_PATH_LEN, "%c:%c", (drive + 'A' - 1), name[0]);
            retcwd = cwdpath;
            HDstrncpy(new_name, &name[1], name_len);
        }
        /* totally relative for Unix, Windows, and OpenVMS: get current working directory  */
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
#ifdef H5_VMS
            /* If the file name contains relative path, cut off the beginning bracket.  Also cut off the
             * ending bracket of CWDPATH to combine the full path name. i.g.
             *     cwdpath = SYS$SYSUSERS:[LU.HDF5.TEST]
             *     new_name = [.tmp]extlinks.h5
             *     full_path = SYS$SYSUSERS:[LU.HDF5.TEST.tmp]extlinks.h5
             */
            if(new_name[0] == '[') {
                char *tmp = new_name;

                full_path[cwdlen - 1] = '\0';
                HDstrncat(full_path, ++tmp, HDstrlen(tmp));
            } /* end if */
            else
                HDstrncat(full_path, new_name, HDstrlen(new_name));
#else
            if(!H5_CHECK_DELIMITER(cwdpath[cwdlen - 1]))
                HDstrncat(full_path, H5_DIR_SEPS, HDstrlen(H5_DIR_SEPS));
            HDstrncat(full_path, new_name, HDstrlen(new_name));
#endif
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
} /* H5_build_extpath() */

