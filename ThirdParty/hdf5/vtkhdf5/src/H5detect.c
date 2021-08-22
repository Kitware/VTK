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

/*keep this declaration near the top of this file -RPM*/
static const char *FileHeader = "\n\
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\n\
 * Copyright by The HDF Group.                                               *\n\
 * Copyright by the Board of Trustees of the University of Illinois.         *\n\
 * All rights reserved.                                                      *\n\
 *                                                                           *\n\
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *\n\
 * terms governing use, modification, and redistribution, is contained in    *\n\
 * the COPYING file, which can be found at the root of the source code       *\n\
 * distribution tree, or in https://www.hdfgroup.org/licenses.               *\n\
 * If you do not have access to either file, you may request a copy from     *\n\
 * help@hdfgroup.org.                                                        *\n\
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *";
/*
 * Purpose:    This code was borrowed heavily from the `detect.c'
 *        program in the AIO distribution from Lawrence
 *        Livermore National Laboratory.
 *
 *        Detects machine byte order and floating point
 *        format and generates a C source file (H5Tinit.c)
 *        to describe those parameters.
 *
 * Assumptions: We have an ANSI compiler.  We're on a Unix like
 *        system or configure has detected those Unix
 *        features which aren't available.  We're not
 *        running on a Vax or other machine with mixed
 *        endianness.
 *-------------------------------------------------------------------------
 */
#undef NDEBUG
#include "H5private.h"
/* Do NOT use HDfprintf in this file as it is not linked with the library,
 * which contains the H5system.c file in which the function is defined.
 */
#include "H5Tpublic.h"
#include "H5Rpublic.h"

/* Disable warning about cast increasing the alignment of the target type,
 * that's _exactly_ what this code is probing.  -QAK
 */
H5_GCC_DIAG_OFF("cast-align")

#if defined(__has_attribute)
#if __has_attribute(no_sanitize_address)
#define HDF_NO_UBSAN __attribute__((no_sanitize_address))
#else
#define HDF_NO_UBSAN
#endif
#else
#define HDF_NO_UBSAN
#endif

#define MAXDETECT 64

/* The ALIGNMENT test code may generate the SIGBUS, SIGSEGV, or SIGILL signals.
 * We use setjmp/longjmp in the signal handlers for recovery. But setjmp/longjmp
 * do not necessary restore the signal blocking status while sigsetjmp/siglongjmp
 * do. If sigsetjmp/siglongjmp are not supported, need to use sigprocmask to
 * unblock the signal before doing longjmp.
 */
/* Define H5SETJMP/H5LONGJMP depending on if sigsetjmp/siglongjmp are */
/* supported. */
#if defined(H5_HAVE_SIGSETJMP) && defined(H5_HAVE_SIGLONGJMP)
/* Always save blocked signals to be restored by siglongjmp. */
#define H5JMP_BUF           sigjmp_buf
#define H5SETJMP(buf)       HDsigsetjmp(buf, 1)
#define H5LONGJMP(buf, val) HDsiglongjmp(buf, val)
#define H5HAVE_SIGJMP       /* sigsetjmp/siglongjmp are supported. */
#elif defined(H5_HAVE_LONGJMP)
#define H5JMP_BUF           jmp_buf
#define H5SETJMP(buf)       HDsetjmp(buf)
#define H5LONGJMP(buf, val) HDlongjmp(buf, val)
#endif

/* ALIGNMENT and signal-handling status codes */
#define STA_NoALIGNMENT     0x0001 /* No ALIGNMENT Test */
#define STA_NoHandlerVerify 0x0002 /* No signal handler Tests */

/*
 * This structure holds information about a type that
 * was detected.
 */
typedef struct detected_t {
    const char *  varname;
    unsigned int  size;             /* total byte size                  */
    unsigned int  precision;        /* meaningful bits                  */
    unsigned int  offset;           /* bit offset to meaningful bits    */
    int           perm[32];         /* for detection of byte order      */
    hbool_t       is_vax;           /* for vax (float & double) only    */
    unsigned int  sign;             /* location of sign bit             */
    unsigned int  mpos, msize, imp; /* information about mantissa       */
    unsigned int  epos, esize;      /* information about exponent       */
    unsigned long bias;             /* exponent bias for floating pt    */
    unsigned int  align;            /* required byte alignment          */
    unsigned int  comp_align;       /* alignment for structure          */
} detected_t;

/* This structure holds structure alignment for pointers, vlen and reference
 * types. */
typedef struct malign_t {
    const char * name;
    unsigned int comp_align; /* alignment for structure          */
} malign_t;

FILE *rawoutstream = NULL;

/* global variables types detection code */
H5_GCC_DIAG_OFF("larger-than=")
static detected_t d_g[MAXDETECT];
H5_GCC_DIAG_ON("larger-than=")
static malign_t     m_g[MAXDETECT];
static volatile int nd_g = 0, na_g = 0;

static void         print_results(int nd, detected_t *d, int na, malign_t *m);
static void         iprint(detected_t *);
static int          byte_cmp(int, const void *, const void *, const unsigned char *);
static unsigned int bit_cmp(unsigned int, int *, void *, void *, const unsigned char *);
static void         fix_order(int, int, int *, const char **);
static unsigned int imp_bit(unsigned int, int *, void *, void *, const unsigned char *);
static unsigned int find_bias(unsigned int, unsigned int, int *, void *);
static void         precision(detected_t *);
static void         print_header(void);
static void         detect_C89_integers(void);
static void         detect_C89_floats(void);
static void         detect_C99_integers(void);
static void         detect_C99_floats(void);
static void         detect_C99_integers8(void);
static void         detect_C99_integers16(void);
static void         detect_C99_integers32(void);
static void         detect_C99_integers64(void);
static void         detect_alignments(void);
static unsigned int align_g[]                = {1, 2, 4, 8, 16};
static int          align_status_g           = 0; /* ALIGNMENT Signal Status */
static int          sigbus_handler_called_g  = 0; /* how many times called */
static int          sigsegv_handler_called_g = 0; /* how many times called */
static int          sigill_handler_called_g  = 0; /* how many times called */
static int          signal_handler_tested_g  = 0; /* how many times tested */
#if defined(H5SETJMP) && defined(H5_HAVE_SIGNAL)
static int verify_signal_handlers(int signum, void (*handler)(int));
#endif
#ifdef H5JMP_BUF
static H5JMP_BUF jbuf_g;
#endif

/*-------------------------------------------------------------------------
 * Function:    precision
 *
 * Purpose:     Determine the precision and offset.
 *
 * Return:      void
 *-------------------------------------------------------------------------
 */
static void
precision(detected_t *d)
{
    unsigned int n;

    if (0 == d->msize) {
        /*
         * An integer.    The permutation can have negative values at the
         * beginning or end which represent padding of bytes.  We must adjust
         * the precision and offset accordingly.
         */
        if (d->perm[0] < 0) {
            /*
             * Lower addresses are padded.
             */
            for (n = 0; n < d->size && d->perm[n] < 0; n++)
                /*void*/;
            d->precision = 8 * (d->size - n);
            d->offset    = 0;
        }
        else if (d->perm[d->size - 1] < 0) {
            /*
             * Higher addresses are padded.
             */
            for (n = 0; n < d->size && d->perm[d->size - (n + 1)]; n++)
                /*void*/;
            d->precision = 8 * (d->size - n);
            d->offset    = 8 * n;
        }
        else {
            /*
             * No padding.
             */
            d->precision = 8 * d->size;
            d->offset    = 0;
        }
    }
    else {
        /* A floating point */
        d->offset    = MIN3(d->mpos, d->epos, d->sign);
        d->precision = d->msize + d->esize + 1;
    }
}

/*-------------------------------------------------------------------------
 * Function:    DETECT_I/DETECT_BYTE
 *
 * Purpose:     These macro takes a type like `int' and a base name like
 *              `nati' and detects the byte order.  The VAR is used to
 *              construct the names of the C variables defined.
 *
 *              DETECT_I is used for types that are larger than one byte,
 *              DETECT_BYTE is used for types that are exactly one byte.
 *
 * Return:      void
 *
 *-------------------------------------------------------------------------
 */
#define DETECT_I_BYTE_CORE(TYPE, VAR, INFO, DETECT_TYPE)                                                     \
    {                                                                                                        \
        DETECT_TYPE    _v;                                                                                   \
        int            _i, _j;                                                                               \
        unsigned char *_x;                                                                                   \
                                                                                                             \
        HDmemset(&INFO, 0, sizeof(INFO));                                                                    \
        INFO.varname = #VAR;                                                                                 \
        INFO.size    = sizeof(TYPE);                                                                         \
                                                                                                             \
        for (_i = sizeof(DETECT_TYPE), _v = 0; _i > 0; --_i)                                                 \
            _v = (DETECT_TYPE)((DETECT_TYPE)(_v << 8) + (DETECT_TYPE)_i);                                    \
                                                                                                             \
        for (_i = 0, _x = (unsigned char *)&_v; _i < (signed)sizeof(DETECT_TYPE); _i++) {                    \
            _j = (*_x++) - 1;                                                                                \
            HDassert(_j < (signed)sizeof(DETECT_TYPE));                                                      \
            INFO.perm[_i] = _j;                                                                              \
        } /* end for */                                                                                      \
                                                                                                             \
        INFO.sign = ('U' != *(#VAR));                                                                        \
        precision(&(INFO));                                                                                  \
        ALIGNMENT(TYPE, INFO);                                                                               \
        if (!HDstrcmp(INFO.varname, "SCHAR") || !HDstrcmp(INFO.varname, "SHORT") ||                          \
            !HDstrcmp(INFO.varname, "INT") || !HDstrcmp(INFO.varname, "LONG") ||                             \
            !HDstrcmp(INFO.varname, "LLONG")) {                                                              \
            COMP_ALIGNMENT(TYPE, INFO.comp_align);                                                           \
        }                                                                                                    \
    }

#define DETECT_BYTE(TYPE, VAR, INFO)                                                                         \
    {                                                                                                        \
        HDcompile_assert(sizeof(TYPE) == 1);                                                                 \
                                                                                                             \
        DETECT_I_BYTE_CORE(TYPE, VAR, INFO, int)                                                             \
    }

#define DETECT_I(TYPE, VAR, INFO)                                                                            \
    {                                                                                                        \
        HDcompile_assert(sizeof(TYPE) > 1);                                                                  \
                                                                                                             \
        DETECT_I_BYTE_CORE(TYPE, VAR, INFO, TYPE)                                                            \
    }

/*-------------------------------------------------------------------------
 * Function:    DETECT_F
 *
 * Purpose:     This macro takes a floating point type like `double' and
 *              a base name like `natd' and detects byte order, mantissa
 *              location, exponent location, sign bit location, presence or
 *              absence of implicit mantissa bit, and exponent bias and
 *              initializes a detected_t structure with those properties.
 *-------------------------------------------------------------------------
 */
#define DETECT_F(TYPE, VAR, INFO)                                                                            \
    {                                                                                                        \
        TYPE          _v1, _v2, _v3;                                                                         \
        unsigned char _buf1[sizeof(TYPE)], _buf3[sizeof(TYPE)];                                              \
        unsigned char _pad_mask[sizeof(TYPE)];                                                               \
        unsigned char _byte_mask;                                                                            \
        int           _i, _j, _last = (-1);                                                                  \
        const char *  _mesg;                                                                                 \
                                                                                                             \
        HDmemset(&INFO, 0, sizeof(INFO));                                                                    \
        INFO.varname = #VAR;                                                                                 \
        INFO.size    = sizeof(TYPE);                                                                         \
                                                                                                             \
        /* Initialize padding mask */                                                                        \
        HDmemset(_pad_mask, 0, sizeof(_pad_mask));                                                           \
                                                                                                             \
        /* Padding bits.  Set a variable to 4.0, then flip each bit and see if                               \
         * the modified variable is equal ("==") to the original.  Build a                                   \
         * padding bitmask to indicate which bits in the type are padding (i.e.                              \
         * have no effect on the value and should be ignored by subsequent                                   \
         * steps).  This is necessary because padding bits can change arbitrarily                            \
         * and interfere with detection of the various properties below unless we                            \
         * know to ignore them. */                                                                           \
        _v1 = (TYPE)4.0L;                                                                                    \
        HDmemcpy(_buf1, (const void *)&_v1, sizeof(TYPE));                                                   \
        for (_i = 0; _i < (int)sizeof(TYPE); _i++)                                                           \
            for (_byte_mask = (unsigned char)1; _byte_mask; _byte_mask = (unsigned char)(_byte_mask << 1)) { \
                _buf1[_i] ^= _byte_mask;                                                                     \
                HDmemcpy((void *)&_v2, (const void *)_buf1, sizeof(TYPE));                                   \
                H5_GCC_DIAG_OFF("float-equal")                                                               \
                if (_v1 != _v2)                                                                              \
                    _pad_mask[_i] |= _byte_mask;                                                             \
                H5_GCC_DIAG_ON("float-equal")                                                                \
                _buf1[_i] ^= _byte_mask;                                                                     \
            } /* end for */                                                                                  \
                                                                                                             \
        /* Byte Order */                                                                                     \
        for (_i = 0, _v1 = (TYPE)0.0L, _v2 = (TYPE)1.0L; _i < (int)sizeof(TYPE); _i++) {                     \
            _v3 = _v1;                                                                                       \
            _v1 += _v2;                                                                                      \
            _v2 /= (TYPE)256.0L;                                                                             \
            HDmemcpy(_buf1, (const void *)&_v1, sizeof(TYPE));                                               \
            HDmemcpy(_buf3, (const void *)&_v3, sizeof(TYPE));                                               \
            _j = byte_cmp(sizeof(TYPE), _buf3, _buf1, _pad_mask);                                            \
            if (_j >= 0) {                                                                                   \
                INFO.perm[_i] = _j;                                                                          \
                _last         = _i;                                                                          \
            }                                                                                                \
        }                                                                                                    \
        fix_order(sizeof(TYPE), _last, INFO.perm, (const char **)&_mesg);                                    \
                                                                                                             \
        if (!HDstrcmp(_mesg, "VAX"))                                                                         \
            INFO.is_vax = TRUE;                                                                              \
                                                                                                             \
        /* Implicit mantissa bit */                                                                          \
        _v1      = (TYPE)0.5L;                                                                               \
        _v2      = (TYPE)1.0L;                                                                               \
        INFO.imp = imp_bit(sizeof(TYPE), INFO.perm, &_v1, &_v2, _pad_mask);                                  \
                                                                                                             \
        /* Sign bit */                                                                                       \
        _v1       = (TYPE)1.0L;                                                                              \
        _v2       = (TYPE)-1.0L;                                                                             \
        INFO.sign = bit_cmp(sizeof(TYPE), INFO.perm, &_v1, &_v2, _pad_mask);                                 \
                                                                                                             \
        /* Mantissa */                                                                                       \
        INFO.mpos = 0;                                                                                       \
                                                                                                             \
        _v1        = (TYPE)1.0L;                                                                             \
        _v2        = (TYPE)1.5L;                                                                             \
        INFO.msize = bit_cmp(sizeof(TYPE), INFO.perm, &_v1, &_v2, _pad_mask);                                \
        INFO.msize += 1 + (unsigned int)(INFO.imp ? 0 : 1) - INFO.mpos;                                      \
                                                                                                             \
        /* Exponent */                                                                                       \
        INFO.epos = INFO.mpos + INFO.msize;                                                                  \
                                                                                                             \
        INFO.esize = INFO.sign - INFO.epos;                                                                  \
                                                                                                             \
        _v1       = (TYPE)1.0L;                                                                              \
        INFO.bias = find_bias(INFO.epos, INFO.esize, INFO.perm, &_v1);                                       \
        precision(&(INFO));                                                                                  \
        ALIGNMENT(TYPE, INFO);                                                                               \
        if (!HDstrcmp(INFO.varname, "FLOAT") || !HDstrcmp(INFO.varname, "DOUBLE") ||                         \
            !HDstrcmp(INFO.varname, "LDOUBLE")) {                                                            \
            COMP_ALIGNMENT(TYPE, INFO.comp_align);                                                           \
        }                                                                                                    \
    }

/*-------------------------------------------------------------------------
 * Function:    DETECT_M
 *
 * Purpose:     This macro takes only miscellaneous structures or pointer.
 *              It constructs the names and decides the alignment in structure.
 *
 * Return:      void
 *-------------------------------------------------------------------------
 */
#define DETECT_M(TYPE, VAR, INFO)                                                                            \
    {                                                                                                        \
        INFO.name = #VAR;                                                                                    \
        COMP_ALIGNMENT(TYPE, INFO.comp_align);                                                               \
    }

/* Detect alignment for C structure */
#define COMP_ALIGNMENT(TYPE, COMP_ALIGN)                                                                     \
    {                                                                                                        \
        struct {                                                                                             \
            char c;                                                                                          \
            TYPE x;                                                                                          \
        } s;                                                                                                 \
                                                                                                             \
        COMP_ALIGN = (unsigned int)((char *)(&(s.x)) - (char *)(&s));                                        \
    }

#if defined(H5SETJMP) && defined(H5_HAVE_SIGNAL)
#define ALIGNMENT(TYPE, INFO)                                                                                \
    {                                                                                                        \
        char *volatile _buf    = NULL;                                                                       \
        TYPE            _val   = 1, _val2;                                                                   \
        volatile size_t _ano   = 0;                                                                          \
        void (*_handler)(int)  = HDsignal(SIGBUS, sigbus_handler);                                           \
        void (*_handler2)(int) = HDsignal(SIGSEGV, sigsegv_handler);                                         \
        void (*_handler3)(int) = HDsignal(SIGILL, sigill_handler);                                           \
                                                                                                             \
        _buf = (char *)HDmalloc(sizeof(TYPE) + align_g[NELMTS(align_g) - 1]);                                \
        if (H5SETJMP(jbuf_g))                                                                                \
            _ano++;                                                                                          \
        if (_ano < NELMTS(align_g)) {                                                                        \
            *((TYPE *)(_buf + align_g[_ano])) = _val;  /*possible SIGBUS or SEGSEGV*/                        \
            _val2 = *((TYPE *)(_buf + align_g[_ano])); /*possible SIGBUS or SEGSEGV*/                        \
            /* Cray Check: This section helps detect alignment on Cray's */                                  \
            /*              vector machines (like the SV1) which mask off */                                 \
            /*              pointer values when pointing to non-word aligned */                              \
            /*              locations with pointers that are supposed to be */                               \
            /*              word aligned. -QAK */                                                            \
            HDmemset(_buf, 0xff, sizeof(TYPE) + align_g[NELMTS(align_g) - 1]);                               \
            /*How to handle VAX types?*/                                                                     \
            if (INFO.perm[0]) /* Big-Endian */                                                               \
                HDmemcpy(_buf + align_g[_ano] + (INFO.size - ((INFO.offset + INFO.precision) / 8)),          \
                         ((char *)&_val) + (INFO.size - ((INFO.offset + INFO.precision) / 8)),               \
                         (size_t)(INFO.precision / 8));                                                      \
            else /* Little-Endian */                                                                         \
                HDmemcpy(_buf + align_g[_ano] + (INFO.offset / 8), ((char *)&_val) + (INFO.offset / 8),      \
                         (size_t)(INFO.precision / 8));                                                      \
            _val2 = *((TYPE *)(_buf + align_g[_ano]));                                                       \
            H5_GCC_DIAG_OFF("float-equal")                                                                   \
            if (_val != _val2)                                                                               \
                H5LONGJMP(jbuf_g, 1);                                                                        \
            H5_GCC_DIAG_ON("float-equal")                                                                    \
            /* End Cray Check */                                                                             \
            (INFO.align) = align_g[_ano];                                                                    \
        }                                                                                                    \
        else {                                                                                               \
            (INFO.align) = 0;                                                                                \
            fprintf(stderr, "unable to calculate alignment for %s\n", #TYPE);                                \
        }                                                                                                    \
        HDfree(_buf);                                                                                        \
        HDsignal(SIGBUS, _handler);   /*restore original handler*/                                           \
        HDsignal(SIGSEGV, _handler2); /*restore original handler*/                                           \
        HDsignal(SIGILL, _handler3);  /*restore original handler*/                                           \
    }
#else
#define ALIGNMENT(TYPE, INFO)                                                                                \
    {                                                                                                        \
        align_status_g |= STA_NoALIGNMENT;                                                                   \
        (INFO.align) = 0;                                                                                    \
    }
#endif

#if defined(H5LONGJMP) && defined(H5_HAVE_SIGNAL)

/*-------------------------------------------------------------------------
 * Function:    sigsegv_handler
 *
 * Purpose:     Handler for SIGSEGV. We use signal() instead of sigaction()
 *              because it's more portable to non-Posix systems. Although
 *              it's not nearly as nice to work with, it does the job for
 *              this simple stuff.
 *
 * Return:      Returns via H5LONGJMP to jbuf_g.
 *-------------------------------------------------------------------------
 */
static void
sigsegv_handler(int H5_ATTR_UNUSED signo)
{
#if !defined(H5HAVE_SIGJMP) && defined(H5_HAVE_SIGPROCMASK)
    /* Use sigprocmask to unblock the signal if sigsetjmp/siglongjmp are not */
    /* supported. */
    sigset_t set;

    HDsigemptyset(&set);
    HDsigaddset(&set, SIGSEGV);
    HDsigprocmask(SIG_UNBLOCK, &set, NULL);
#endif

    sigsegv_handler_called_g++;
    HDsignal(SIGSEGV, sigsegv_handler);
    H5LONGJMP(jbuf_g, SIGSEGV);
}
#endif

#if defined(H5LONGJMP) && defined(H5_HAVE_SIGNAL)

/*-------------------------------------------------------------------------
 * Function:    sigbus_handler
 *
 * Purpose:     Handler for SIGBUS. We use signal() instead of sigaction()
 *              because it's more portable to non-Posix systems. Although
 *              it's not nearly as nice to work with, it does the job for
 *              this simple stuff.
 *
 * Return:      Returns via H5LONGJMP to jbuf_g.
 *-------------------------------------------------------------------------
 */
static void
sigbus_handler(int H5_ATTR_UNUSED signo)
{
#if !defined(H5HAVE_SIGJMP) && defined(H5_HAVE_SIGPROCMASK)
    /* Use sigprocmask to unblock the signal if sigsetjmp/siglongjmp are not */
    /* supported. */
    sigset_t set;

    HDsigemptyset(&set);
    HDsigaddset(&set, SIGBUS);
    HDsigprocmask(SIG_UNBLOCK, &set, NULL);
#endif

    sigbus_handler_called_g++;
    HDsignal(SIGBUS, sigbus_handler);
    H5LONGJMP(jbuf_g, SIGBUS);
}
#endif

#if defined(H5LONGJMP) && defined(H5_HAVE_SIGNAL)

/*-------------------------------------------------------------------------
 * Function:    sigill_handler
 *
 * Purpose:     Handler for SIGILL. We use signal() instead of sigaction()
 *              because it's more portable to non-Posix systems. Although
 *              it's not nearly as nice to work with, it does the job for
 *              this simple stuff.
 *
 * Return:      Returns via H5LONGJMP to jbuf_g.
 *-------------------------------------------------------------------------
 */
static void
sigill_handler(int H5_ATTR_UNUSED signo)
{
#if !defined(H5HAVE_SIGJMP) && defined(H5_HAVE_SIGPROCMASK)
    /* Use sigprocmask to unblock the signal if sigsetjmp/siglongjmp are not */
    /* supported. */
    sigset_t set;

    HDsigemptyset(&set);
    HDsigaddset(&set, SIGILL);
    HDsigprocmask(SIG_UNBLOCK, &set, NULL);
#endif

    sigill_handler_called_g++;
    HDsignal(SIGILL, sigill_handler);
    H5LONGJMP(jbuf_g, SIGILL);
}
#endif

/*-------------------------------------------------------------------------
 * Function:    print_results
 *
 * Purpose:     Prints information about the detected data types.
 *
 * Return:      void
 *-------------------------------------------------------------------------
 */
static void
print_results(int nd, detected_t *d, int na, malign_t *misc_align)
{
    int byte_order = 0; /*byte order of data types*/
    int i, j;

    /* Include files */
    fprintf(rawoutstream, "\
/****************/\n\
/* Module Setup */\n\
/****************/\n\
\n\
#include \"H5Tmodule.h\"          /* This source code file is part of the H5T module */\n\
\n\
\n\
/***********/\n\
/* Headers */\n\
/***********/\n\
#include \"H5private.h\"        /* Generic Functions            */\n\
#include \"H5Eprivate.h\"        /* Error handling              */\n\
#include \"H5FLprivate.h\"    /* Free Lists                */\n\
#include \"H5Iprivate.h\"        /* IDs                      */\n\
#include \"H5Tpkg.h\"        /* Datatypes                 */\n\
\n\
\n\
/****************/\n\
/* Local Macros */\n\
/****************/\n\
\n\
\n\
/******************/\n\
/* Local Typedefs */\n\
/******************/\n\
\n\
\n\
/********************/\n\
/* Package Typedefs */\n\
/********************/\n\
\n\
\n\
/********************/\n\
/* Local Prototypes */\n\
/********************/\n\
\n\
\n\
/********************/\n\
/* Public Variables */\n\
/********************/\n\
\n\
\n\
/*****************************/\n\
/* Library Private Variables */\n\
/*****************************/\n\
\n\
\n\
/*********************/\n\
/* Package Variables */\n\
/*********************/\n\
\n\
\n");
    fprintf(rawoutstream, "\n\
/*******************/\n\
/* Local Variables */\n\
/*******************/\n\
\n");

    /* The interface initialization function */
    fprintf(rawoutstream, "\n\
\n\
/*-------------------------------------------------------------------------\n\
 * Function:    H5T__init_native\n\
 *\n\
 * Purpose:    Initialize pre-defined native datatypes from code generated\n\
 *              during the library configuration by H5detect.\n\
 *\n\
 * Return:    Success:    non-negative\n\
 *        Failure:    negative\n\
 *\n\
 * Programmer:    Robb Matzke\n\
 *              Wednesday, December 16, 1998\n\
 *\n\
 *-------------------------------------------------------------------------\n\
 */\n\
herr_t\n\
H5T__init_native(void)\n\
{\n\
    H5T_t    *dt = NULL;\n\
    herr_t    ret_value = SUCCEED;\n\
\n\
    FUNC_ENTER_PACKAGE\n");

    for (i = 0; i < nd; i++) {
        /* The native endianness of this machine */
        /* The INFO.perm now contains `-1' for bytes that aren't used and
         * are always zero.  This happens on the Cray for `short' where
         * sizeof(short) is 8, but only the low-order 4 bytes are ever used.
         */
        if (d[i].is_vax) /* the type is a VAX floating number */
            byte_order = -1;
        else {
            for (j = 0; j < 32; j++) {
                /*Find the 1st containing valid data*/
                if (d[i].perm[j] > -1) {
                    byte_order = d[i].perm[j];
                    break;
                }
            }
        }

        /* Print a comment to describe this section of definitions. */
        fprintf(rawoutstream, "\n   /*\n");
        iprint(d + i);
        fprintf(rawoutstream, "    */\n");

        /* The part common to fixed and floating types */
        fprintf(rawoutstream, "\
    if(NULL == (dt = H5T__alloc()))\n\
        HGOTO_ERROR(H5E_DATATYPE, H5E_NOSPACE, FAIL, \"datatype allocation failed\")\n\
    dt->shared->state = H5T_STATE_IMMUTABLE;\n\
    dt->shared->type = H5T_%s;\n\
    dt->shared->size = %d;\n",
                d[i].msize ? "FLOAT" : "INTEGER", /*class            */
                d[i].size);                       /*size            */

        if (byte_order == -1)
            fprintf(rawoutstream, "\
    dt->shared->u.atomic.order = H5T_ORDER_VAX;\n");
        else if (byte_order == 0)
            fprintf(rawoutstream, "\
    dt->shared->u.atomic.order = H5T_ORDER_LE;\n");
        else
            fprintf(rawoutstream, "\
    dt->shared->u.atomic.order = H5T_ORDER_BE;\n");

        fprintf(rawoutstream, "\
    dt->shared->u.atomic.offset = %d;\n\
    dt->shared->u.atomic.prec = %d;\n\
    dt->shared->u.atomic.lsb_pad = H5T_PAD_ZERO;\n\
    dt->shared->u.atomic.msb_pad = H5T_PAD_ZERO;\n",
                d[i].offset,                            /*offset        */
                d[i].precision);                        /*precision        */
        /*HDassert((d[i].perm[0]>0)==(byte_order>0));*/ /* Double-check that byte-order doesn't change */

        if (0 == d[i].msize) {
            /* The part unique to fixed point types */
            fprintf(rawoutstream, "\
    dt->shared->u.atomic.u.i.sign = H5T_SGN_%s;\n",
                    d[i].sign ? "2" : "NONE");
        }
        else {
            /* The part unique to floating point types */
            fprintf(rawoutstream, "\
    dt->shared->u.atomic.u.f.sign = %d;\n\
    dt->shared->u.atomic.u.f.epos = %d;\n\
    dt->shared->u.atomic.u.f.esize = %d;\n\
    dt->shared->u.atomic.u.f.ebias = 0x%08lx;\n\
    dt->shared->u.atomic.u.f.mpos = %d;\n\
    dt->shared->u.atomic.u.f.msize = %d;\n\
    dt->shared->u.atomic.u.f.norm = H5T_NORM_%s;\n\
    dt->shared->u.atomic.u.f.pad = H5T_PAD_ZERO;\n",
                    d[i].sign,                      /*sign location */
                    d[i].epos,                      /*exponent loc    */
                    d[i].esize,                     /*exponent size */
                    (unsigned long)(d[i].bias),     /*exponent bias */
                    d[i].mpos,                      /*mantissa loc    */
                    d[i].msize,                     /*mantissa size */
                    d[i].imp ? "IMPLIED" : "NONE"); /*normalization */
        }

        /* Atomize the type */
        fprintf(rawoutstream, "\
    if((H5T_NATIVE_%s_g = H5I_register(H5I_DATATYPE, dt, FALSE)) < 0)\n\
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, \"can't register ID for built-in datatype\")\n",
                d[i].varname);
        fprintf(rawoutstream, "    H5T_NATIVE_%s_ALIGN_g = %lu;\n", d[i].varname,
                (unsigned long)(d[i].align));

        /* Variables for alignment of compound datatype */
        if (!HDstrcmp(d[i].varname, "SCHAR") || !HDstrcmp(d[i].varname, "SHORT") ||
            !HDstrcmp(d[i].varname, "INT") || !HDstrcmp(d[i].varname, "LONG") ||
            !HDstrcmp(d[i].varname, "LLONG") || !HDstrcmp(d[i].varname, "FLOAT") ||
            !HDstrcmp(d[i].varname, "DOUBLE") || !HDstrcmp(d[i].varname, "LDOUBLE")) {
            fprintf(rawoutstream, "    H5T_NATIVE_%s_COMP_ALIGN_g = %lu;\n", d[i].varname,
                    (unsigned long)(d[i].comp_align));
        }
    }

    /* Consider VAX a little-endian machine */
    if (byte_order == 0 || byte_order == -1) {
        fprintf(rawoutstream, "\n\
    /* Set the native order for this machine */\n\
    H5T_native_order_g = H5T_ORDER_%s;\n",
                "LE");
    }
    else {
        fprintf(rawoutstream, "\n\
    /* Set the native order for this machine */\n\
    H5T_native_order_g = H5T_ORDER_%s;\n",
                "BE");
    }

    /* Structure alignment for pointers, vlen and reference types */
    fprintf(rawoutstream, "\n    /* Structure alignment for pointers, vlen and reference types */\n");
    for (j = 0; j < na; j++)
        fprintf(rawoutstream, "    H5T_%s_COMP_ALIGN_g = %lu;\n", misc_align[j].name,
                (unsigned long)(misc_align[j].comp_align));

    fprintf(rawoutstream, "\
\n\
done:\n\
    if(ret_value < 0) {\n\
        if(dt != NULL) {\n\
            dt->shared = H5FL_FREE(H5T_shared_t, dt->shared);\n\
            dt = H5FL_FREE(H5T_t, dt);\n\
        } /* end if */\n\
    } /* end if */\n\
\n\
    FUNC_LEAVE_NOAPI(ret_value);\n} /* end H5T__init_native() */\n");

    /* Print the ALIGNMENT and signal-handling status as comments */
    fprintf(rawoutstream, "\n"
                          "/****************************************/\n"
                          "/* ALIGNMENT and signal-handling status */\n"
                          "/****************************************/\n");
    if (align_status_g & STA_NoALIGNMENT)
        fprintf(rawoutstream, "/* ALIGNAMENT test is not available */\n");
    if (align_status_g & STA_NoHandlerVerify)
        fprintf(rawoutstream, "/* Signal handlers verify test is not available */\n");
        /* The following is available in H5pubconf.h. Printing them here for */
        /* convenience. */
#ifdef H5_HAVE_SIGNAL
    fprintf(rawoutstream, "/* Signal() support: yes */\n");
#else
    fprintf(rawoutstream, "/* Signal() support: no */\n");
#endif
#ifdef H5_HAVE_SETJMP
    fprintf(rawoutstream, "/* setjmp() support: yes */\n");
#else
    fprintf(rawoutstream, "/* setjmp() support: no */\n");
#endif
#ifdef H5_HAVE_LONGJMP
    fprintf(rawoutstream, "/* longjmp() support: yes */\n");
#else
    fprintf(rawoutstream, "/* longjmp() support: no */\n");
#endif
#ifdef H5_HAVE_SIGSETJMP
    fprintf(rawoutstream, "/* sigsetjmp() support: yes */\n");
#else
    fprintf(rawoutstream, "/* sigsetjmp() support: no */\n");
#endif
#ifdef H5_HAVE_SIGLONGJMP
    fprintf(rawoutstream, "/* siglongjmp() support: yes */\n");
#else
    fprintf(rawoutstream, "/* siglongjmp() support: no */\n");
#endif
#ifdef H5_HAVE_SIGPROCMASK
    fprintf(rawoutstream, "/* sigprocmask() support: yes */\n");
#else
    fprintf(rawoutstream, "/* sigprocmask() support: no */\n");
#endif

    /* Print the statics of signal handlers called for debugging */
    fprintf(rawoutstream, "\n"
                          "/******************************/\n"
                          "/* signal handlers statistics */\n"
                          "/******************************/\n");
    fprintf(rawoutstream, "/* signal_handlers tested: %d times */\n", signal_handler_tested_g);
    fprintf(rawoutstream, "/* sigbus_handler called: %d times */\n", sigbus_handler_called_g);
    fprintf(rawoutstream, "/* sigsegv_handler called: %d times */\n", sigsegv_handler_called_g);
    fprintf(rawoutstream, "/* sigill_handler called: %d times */\n", sigill_handler_called_g);
} /* end print_results() */

/*-------------------------------------------------------------------------
 * Function:    iprint
 *
 * Purpose:     Prints information about the fields of a floating point format.
 *
 * Return:      void

 *-------------------------------------------------------------------------
 */
static void
iprint(detected_t *d)
{
    unsigned int pass;

    for (pass = (d->size - 1) / 4;; --pass) {
        unsigned int i, k;
        /*
         * Print the byte ordering above the bit fields.
         */
        fprintf(rawoutstream, "    * ");
        for (i = MIN(pass * 4 + 3, d->size - 1); i >= pass * 4; --i) {
            fprintf(rawoutstream, "%4d", d->perm[i]);
            if (i > pass * 4)
                HDfputs("     ", stdout);
            if (!i)
                break;
        }

        /*
         * Print the bit fields
         */
        fprintf(rawoutstream, "\n    * ");
        for (i = MIN(pass * 4 + 3, d->size - 1), k = MIN(pass * 32 + 31, 8 * d->size - 1); i >= pass * 4;
             --i) {
            unsigned int j;

            for (j = 8; j > 0; --j) {
                if (k == d->sign && d->msize) {
                    HDfputc('S', rawoutstream);
                }
                else if (k >= d->epos && k < d->epos + d->esize) {
                    HDfputc('E', rawoutstream);
                }
                else if (k >= d->mpos && k < d->mpos + d->msize) {
                    HDfputc('M', rawoutstream);
                }
                else if (d->msize) {
                    HDfputc('?', rawoutstream); /*unknown floating point bit */
                }
                else if (d->sign) {
                    HDfputc('I', rawoutstream);
                }
                else {
                    HDfputc('U', rawoutstream);
                }
                --k;
            }
            if (i > pass * 4)
                HDfputc(' ', rawoutstream);
            if (!i)
                break;
        }
        HDfputc('\n', rawoutstream);
        if (!pass)
            break;
    }

    /*
     * Is there an implicit bit in the mantissa.
     */
    if (d->msize) {
        fprintf(rawoutstream, "    * Implicit bit? %s\n", d->imp ? "yes" : "no");
    }

    /*
     * Alignment
     */
    if (0 == d->align) {
        fprintf(rawoutstream, "    * Alignment: NOT CALCULATED\n");
    }
    else if (1 == d->align) {
        fprintf(rawoutstream, "    * Alignment: none\n");
    }
    else {
        fprintf(rawoutstream, "    * Alignment: %lu\n", (unsigned long)(d->align));
    }
}

/*-------------------------------------------------------------------------
 * Function:    byte_cmp
 *
 * Purpose:     Compares two chunks of memory A and B and returns the
 *              byte index into those arrays of the first byte that
 *              differs between A and B.  Ignores differences where the
 *              corresponding bit in pad_mask is set to 0.
 *
 * Return:      Success:    Index of differing byte.
 *              Failure:    -1 if all bytes are the same.
 *-------------------------------------------------------------------------
 */
static int
byte_cmp(int n, const void *_a, const void *_b, const unsigned char *pad_mask)
{
    int                  i;
    const unsigned char *a = (const unsigned char *)_a;
    const unsigned char *b = (const unsigned char *)_b;

    for (i = 0; i < n; i++)
        if ((a[i] & pad_mask[i]) != (b[i] & pad_mask[i]))
            return i;

    return -1;
}

/*-------------------------------------------------------------------------
 * Function:    bit_cmp
 *
 * Purpose:     Compares two bit vectors and returns the index for the
 *              first bit that differs between the two vectors.     The
 *              size of the vector is NBYTES.  PERM is a mapping from
 *              actual order to little endian.  Ignores differences where
 *              the corresponding bit in pad_mask is set to 0.
 *
 * Return:      Index of first differing bit.
 *
 *-------------------------------------------------------------------------
 */
static unsigned int
bit_cmp(unsigned int nbytes, int *perm, void *_a, void *_b, const unsigned char *pad_mask)
{
    unsigned int   i;
    unsigned char *a = (unsigned char *)_a;
    unsigned char *b = (unsigned char *)_b;
    unsigned char  aa, bb;

    for (i = 0; i < nbytes; i++) {
        HDassert(perm[i] < (int)nbytes);
        if ((aa = (unsigned char)(a[perm[i]] & pad_mask[perm[i]])) !=
            (bb = (unsigned char)(b[perm[i]] & pad_mask[perm[i]]))) {
            unsigned int j;

            for (j = 0; j < 8; j++, aa >>= 1, bb >>= 1) {
                if ((aa & 1) != (bb & 1))
                    return i * 8 + j;
            }
            fprintf(stderr, "INTERNAL ERROR");
            HDabort();
        }
    }
    fprintf(stderr, "INTERNAL ERROR");
    HDabort();
    return 0;
}

/*-------------------------------------------------------------------------
 * Function:    fix_order
 *
 * Purpose:      Given an array PERM with elements FIRST through LAST
 *              initialized with zero origin byte numbers, this function
 *              creates a permutation vector that maps the actual order
 *              of a floating point number to little-endian.
 *
 *              This function assumes that the mantissa byte ordering
 *              implies the total ordering.
 *
 * Return:      void
 *-------------------------------------------------------------------------
 */
static void
fix_order(int n, int last, int *perm, const char **mesg)
{
    int i;

    if (last > 1) {
        /*
         * We have at least three points to consider.
         */
        if (perm[last] < perm[last - 1] && perm[last - 1] < perm[last - 2]) {
            /*
             * Little endian.
             */
            if (mesg)
                *mesg = "Little-endian";
            for (i = 0; i < n; i++)
                perm[i] = i;
        }
        else if (perm[last] > perm[last - 1] && perm[last - 1] > perm[last - 2]) {
            /*
             * Big endian.
             */
            if (mesg)
                *mesg = "Big-endian";
            for (i = 0; i < n; i++)
                perm[i] = (n - 1) - i;
        }
        else {
            /*
             * Bi-endian machines like VAX.
             * (NOTE: This is not an actual determination of the VAX-endianness.
             *          It could have some other endianness and fall into this
             *          case - JKM & QAK)
             */
            HDassert(0 == n % 2);
            if (mesg)
                *mesg = "VAX";
            for (i = 0; i < n; i += 2) {
                perm[i]     = (n - 2) - i;
                perm[i + 1] = (n - 1) - i;
            }
        }
    }
    else {
        fprintf(stderr, "Failed to detect byte order of %d-byte floating point.\n", n);
        HDexit(1);
    }
}

/*-------------------------------------------------------------------------
 * Function:    imp_bit
 *
 * Purpose:     Looks for an implicit bit in the mantissa.  The value
 *              of _A should be 1.0 and the value of _B should be 0.5.
 *              Some floating-point formats discard the most significant
 *              bit of the mantissa after normalizing since it will always
 *              be a one (except for 0.0).  If this is true for the native
 *              floating point values stored in _A and _B then the function
 *              returns non-zero.
 *
 *              This function assumes that the exponent occupies higher
 *              order bits than the mantissa and that the most significant
 *              bit of the mantissa is next to the least significant bit
 *              of the exponent.
 *
 *
 * Return:      Success:    Non-zero if the most significant bit
 *                          of the mantissa is discarded (ie, the
 *                          mantissa has an implicit `one' as the
 *                          most significant bit).    Otherwise,
 *                          returns zero.
 *
 *              Failure:    1
 *
 *-------------------------------------------------------------------------
 */
static unsigned int
imp_bit(unsigned int n, int *perm, void *_a, void *_b, const unsigned char *pad_mask)
{
    unsigned char *a = (unsigned char *)_a;
    unsigned char *b = (unsigned char *)_b;
    unsigned int   changed, major, minor;
    unsigned int   msmb; /* most significant mantissa bit */

    /*
     * Look for the least significant bit that has changed between
     * A and B.  This is the least significant bit of the exponent.
     */
    changed = bit_cmp(n, perm, a, b, pad_mask);

    /*
     * The bit to the right (less significant) of the changed bit should
     * be the most significant bit of the mantissa.  If it is non-zero
     * then the format does not remove the leading `1' of the mantissa.
     */
    msmb  = changed - 1;
    major = msmb / 8;
    minor = msmb % 8;

    return (a[perm[major]] >> minor) & 0x01 ? 0 : 1;
}

/*-------------------------------------------------------------------------
 * Function:  find_bias
 *
 * Purpose:   Determines the bias of the exponent.  This function should
 *            be called with _A having a value of `1'.
 *
 * Return:    The exponent bias.
 *
 *-------------------------------------------------------------------------
 */
H5_ATTR_PURE static unsigned int
find_bias(unsigned int epos, unsigned int esize, int *perm, void *_a)
{
    unsigned char *a = (unsigned char *)_a;
    unsigned char  mask;
    unsigned int   b, shift = 0, nbits, bias = 0;

    while (esize > 0) {
        nbits = MIN(esize, (8 - epos % 8));
        mask  = (unsigned char)((1 << nbits) - 1);
        b     = (unsigned int)(a[perm[epos / 8]] >> (epos % 8)) & mask;
        bias |= b << shift;

        shift += nbits;
        esize -= nbits;
        epos += nbits;
    }
    return bias;
}

/*-------------------------------------------------------------------------
 * Function:    print_header
 *
 * Purpose:     Prints the C file header for the generated file.
 *
 * Return:      void
 *-------------------------------------------------------------------------
 */
static void
print_header(void)
{

    time_t      now = HDtime(NULL);
    struct tm * tm  = HDlocaltime(&now);
    char        real_name[30];
    char        host_name[256];
    int         i;
    const char *s;
#ifdef H5_HAVE_GETPWUID
    struct passwd *pwd = NULL;
#else
    int pwd      = 1;
#endif
    static const char *month_name[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                       "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    static const char *purpose      = "\
This machine-generated source code contains\n\
information about the various integer and\n\
floating point numeric formats found on this\n\
architecture.  The parameters below should be\n\
checked carefully and errors reported to the\n\
HDF5 maintainer.\n\
\n\
Each of the numeric formats listed below are\n\
printed from most significant bit to least\n\
significant bit even though the actual bytes\n\
might be stored in a different order in\n\
memory.     The integers above each binary byte\n\
indicate the relative order of the bytes in\n\
memory; little-endian machines have\n\
decreasing numbers while big-endian machines\n\
have increasing numbers.\n\
\n\
The fields of the numbers are printed as\n\
letters with `S' for the mantissa sign bit,\n\
`M' for the mantissa magnitude, and `E' for\n\
the exponent.  The exponent has an associated\n\
bias which can be subtracted to find the\n\
true exponent.    The radix point is assumed\n\
to be before the first `M' bit.     Any bit\n\
of a floating-point value not falling into one\n\
of these categories is printed as a question\n\
mark.  Bits of integer types are printed as\n\
`I' for 2's complement and `U' for magnitude.\n\
\n\
If the most significant bit of the normalized\n\
mantissa (always a `1' except for `0.0') is\n\
not stored then an `implicit=yes' appears\n\
under the field description.  In this case,\n\
the radix point is still assumed to be\n\
before the first `M' but after the implicit\n\
bit.\n";

    /*
     * The real name is the first item from the passwd gecos field.
     */
#ifdef H5_HAVE_GETPWUID
    {
        size_t n;
        char * comma;
        if ((pwd = HDgetpwuid(HDgetuid()))) {
            if ((comma = HDstrchr(pwd->pw_gecos, ','))) {
                n = MIN(sizeof(real_name) - 1, (unsigned)(comma - pwd->pw_gecos));
                HDstrncpy(real_name, pwd->pw_gecos, n);
                real_name[n] = '\0';
            }
            else {
                HDstrncpy(real_name, pwd->pw_gecos, sizeof(real_name));
                real_name[sizeof(real_name) - 1] = '\0';
            }
        }
        else
            real_name[0] = '\0';
    }
#else
    real_name[0] = '\0';
#endif

    /*
     * The FQDM of this host or the empty string.
     */
#ifdef H5_HAVE_GETHOSTNAME
    if (HDgethostname(host_name, sizeof(host_name)) < 0) {
        host_name[0] = '\0';
    }
#else
    host_name[0] = '\0';
#endif

    /*
     * The file header: warning, copyright notice, build information.
     */
    fprintf(rawoutstream, "/* Generated automatically by H5detect -- do not edit */\n\n\n");
    HDfputs(FileHeader, rawoutstream); /*the copyright notice--see top of this file */

    fprintf(rawoutstream, " *\n * Created:\t\t%s %2d, %4d\n", month_name[tm->tm_mon], tm->tm_mday,
            1900 + tm->tm_year);
    if (pwd || real_name[0] || host_name[0]) {
        fprintf(rawoutstream, " *\t\t\t");
        if (real_name[0])
            fprintf(rawoutstream, "%s <", real_name);
#ifdef H5_HAVE_GETPWUID
        if (pwd)
            HDfputs(pwd->pw_name, rawoutstream);
#endif
        if (host_name[0])
            fprintf(rawoutstream, "@%s", host_name);
        if (real_name[0])
            fprintf(rawoutstream, ">");
        HDfputc('\n', rawoutstream);
    }
    fprintf(rawoutstream, " *\n * Purpose:\t\t");
    for (s = purpose; *s; s++) {
        HDfputc(*s, rawoutstream);
        if ('\n' == *s && s[1])
            fprintf(rawoutstream, " *\t\t\t");
    }

    fprintf(rawoutstream, " *\n * Modifications:\n *\n");
    fprintf(rawoutstream, " *\tDO NOT MAKE MODIFICATIONS TO THIS FILE!\n");
    fprintf(rawoutstream, " *\tIt was generated by code in `H5detect.c'.\n");

    fprintf(rawoutstream, " *\n *");
    for (i = 0; i < 73; i++)
        HDfputc('-', rawoutstream);
    fprintf(rawoutstream, "\n */\n\n");
}

/*-------------------------------------------------------------------------
 * Function:    detect_C89_integers
 *
 * Purpose:     Detect C89 integer types
 *
 * Return:      void
 *-------------------------------------------------------------------------
 */
static void HDF_NO_UBSAN
detect_C89_integers(void)
{
    DETECT_BYTE(signed char, SCHAR, d_g[nd_g]);
    nd_g++;
    DETECT_BYTE(unsigned char, UCHAR, d_g[nd_g]);
    nd_g++;
    DETECT_I(short, SHORT, d_g[nd_g]);
    nd_g++;
    DETECT_I(unsigned short, USHORT, d_g[nd_g]);
    nd_g++;
    DETECT_I(int, INT, d_g[nd_g]);
    nd_g++;
    DETECT_I(unsigned int, UINT, d_g[nd_g]);
    nd_g++;
    DETECT_I(long, LONG, d_g[nd_g]);
    nd_g++;
    DETECT_I(unsigned long, ULONG, d_g[nd_g]);
    nd_g++;
}

/*-------------------------------------------------------------------------
 * Function:    detect_C89_floats
 *
 * Purpose:     Detect C89 floating point types
 *
 * Return:      void
 *-------------------------------------------------------------------------
 */
static void HDF_NO_UBSAN
detect_C89_floats(void)
{
    DETECT_F(float, FLOAT, d_g[nd_g]);
    nd_g++;
    DETECT_F(double, DOUBLE, d_g[nd_g]);
    nd_g++;
}

/*-------------------------------------------------------------------------
 * Function:    detect_C99_integers8
 *
 * Purpose:     Detect C99 8 bit integer types
 *
 * Return:      void
 *-------------------------------------------------------------------------
 */
static void HDF_NO_UBSAN
detect_C99_integers8(void)
{
#if H5_SIZEOF_INT8_T > 0
#if H5_SIZEOF_INT8_T == 1
    DETECT_BYTE(int8_t, INT8, d_g[nd_g]);
    nd_g++;
#else
    DETECT_I(int8_t, INT8, d_g[nd_g]);
    nd_g++;
#endif
#endif
#if H5_SIZEOF_UINT8_T > 0
#if H5_SIZEOF_UINT8_T == 1
    DETECT_BYTE(uint8_t, UINT8, d_g[nd_g]);
    nd_g++;
#else
    DETECT_I(uint8_t, UINT8, d_g[nd_g]);
    nd_g++;
#endif
#endif
#if H5_SIZEOF_INT_LEAST8_T > 0
#if H5_SIZEOF_INT_LEAST8_T == 1
    DETECT_BYTE(int_least8_t, INT_LEAST8, d_g[nd_g]);
    nd_g++;
#else
    DETECT_I(int_least8_t, INT_LEAST8, d_g[nd_g]);
    nd_g++;
#endif
#endif
#if H5_SIZEOF_UINT_LEAST8_T > 0
#if H5_SIZEOF_UINT_LEAST8_T == 1
    DETECT_BYTE(uint_least8_t, UINT_LEAST8, d_g[nd_g]);
    nd_g++;
#else
    DETECT_I(uint_least8_t, UINT_LEAST8, d_g[nd_g]);
    nd_g++;
#endif
#endif
#if H5_SIZEOF_INT_FAST8_T > 0
#if H5_SIZEOF_INT_FAST8_T == 1
    DETECT_BYTE(int_fast8_t, INT_FAST8, d_g[nd_g]);
    nd_g++;
#else
    DETECT_I(int_fast8_t, INT_FAST8, d_g[nd_g]);
    nd_g++;
#endif
#endif
#if H5_SIZEOF_UINT_FAST8_T > 0
#if H5_SIZEOF_UINT_FAST8_T == 1
    DETECT_BYTE(uint_fast8_t, UINT_FAST8, d_g[nd_g]);
    nd_g++;
#else
    DETECT_I(uint_fast8_t, UINT_FAST8, d_g[nd_g]);
    nd_g++;
#endif
#endif
}

/*-------------------------------------------------------------------------
 * Function:    detect_C99_integers16
 *
 * Purpose:     Detect C99 16 bit integer types
 *
 * Return:      void
 *-------------------------------------------------------------------------
 */
static void HDF_NO_UBSAN
detect_C99_integers16(void)
{
#if H5_SIZEOF_INT16_T > 0
    DETECT_I(int16_t, INT16, d_g[nd_g]);
    nd_g++;
#endif
#if H5_SIZEOF_UINT16_T > 0
    DETECT_I(uint16_t, UINT16, d_g[nd_g]);
    nd_g++;
#endif
#if H5_SIZEOF_INT_LEAST16_T > 0
    DETECT_I(int_least16_t, INT_LEAST16, d_g[nd_g]);
    nd_g++;
#endif
#if H5_SIZEOF_UINT_LEAST16_T > 0
    DETECT_I(uint_least16_t, UINT_LEAST16, d_g[nd_g]);
    nd_g++;
#endif
#if H5_SIZEOF_INT_FAST16_T > 0
    DETECT_I(int_fast16_t, INT_FAST16, d_g[nd_g]);
    nd_g++;
#endif
#if H5_SIZEOF_UINT_FAST16_T > 0
    DETECT_I(uint_fast16_t, UINT_FAST16, d_g[nd_g]);
    nd_g++;
#endif
}

/*-------------------------------------------------------------------------
 * Function:    detect_C99_integers32
 *
 * Purpose:     Detect C99 32 bit integer types
 *
 * Return:      void
 *-------------------------------------------------------------------------
 */
static void HDF_NO_UBSAN
detect_C99_integers32(void)
{
#if H5_SIZEOF_INT32_T > 0
    DETECT_I(int32_t, INT32, d_g[nd_g]);
    nd_g++;
#endif
#if H5_SIZEOF_UINT32_T > 0
    DETECT_I(uint32_t, UINT32, d_g[nd_g]);
    nd_g++;
#endif
#if H5_SIZEOF_INT_LEAST32_T > 0
    DETECT_I(int_least32_t, INT_LEAST32, d_g[nd_g]);
    nd_g++;
#endif
#if H5_SIZEOF_UINT_LEAST32_T > 0
    DETECT_I(uint_least32_t, UINT_LEAST32, d_g[nd_g]);
    nd_g++;
#endif
#if H5_SIZEOF_INT_FAST32_T > 0
    DETECT_I(int_fast32_t, INT_FAST32, d_g[nd_g]);
    nd_g++;
#endif
#if H5_SIZEOF_UINT_FAST32_T > 0
    DETECT_I(uint_fast32_t, UINT_FAST32, d_g[nd_g]);
    nd_g++;
#endif
}

/*-------------------------------------------------------------------------
 * Function:    detect_C99_integers64
 *
 * Purpose:     Detect C99 64 bit integer types
 *
 * Return:      void
 *
 *-------------------------------------------------------------------------
 */
static void HDF_NO_UBSAN
detect_C99_integers64(void)
{
#if H5_SIZEOF_INT64_T > 0
    DETECT_I(int64_t, INT64, d_g[nd_g]);
    nd_g++;
#endif
#if H5_SIZEOF_UINT64_T > 0
    DETECT_I(uint64_t, UINT64, d_g[nd_g]);
    nd_g++;
#endif
#if H5_SIZEOF_INT_LEAST64_T > 0
    DETECT_I(int_least64_t, INT_LEAST64, d_g[nd_g]);
    nd_g++;
#endif
#if H5_SIZEOF_UINT_LEAST64_T > 0
    DETECT_I(uint_least64_t, UINT_LEAST64, d_g[nd_g]);
    nd_g++;
#endif
#if H5_SIZEOF_INT_FAST64_T > 0
    DETECT_I(int_fast64_t, INT_FAST64, d_g[nd_g]);
    nd_g++;
#endif
#if H5_SIZEOF_UINT_FAST64_T > 0
    DETECT_I(uint_fast64_t, UINT_FAST64, d_g[nd_g]);
    nd_g++;
#endif

#if H5_SIZEOF_LONG_LONG > 0
    DETECT_I(long long, LLONG, d_g[nd_g]);
    nd_g++;
    DETECT_I(unsigned long long, ULLONG, d_g[nd_g]);
    nd_g++;
#else
    /*
     * This architecture doesn't support an integer type larger than `long'
     * so we'll just make H5T_NATIVE_LLONG the same as H5T_NATIVE_LONG since
     * `long long' is probably equivalent to `long' here anyway.
     */
    DETECT_I(long, LLONG, d_g[nd_g]);
    nd_g++;
    DETECT_I(unsigned long, ULLONG, d_g[nd_g]);
    nd_g++;
#endif
}

/*-------------------------------------------------------------------------
 * Function:    detect_C99_integers
 *
 * Purpose:     Detect C99 integer types
 *
 * Return:      void
 *-------------------------------------------------------------------------
 */
static void HDF_NO_UBSAN
detect_C99_integers(void)
{
    /* break it down to more subroutines so that each module subroutine */
    /* is smaller and takes less time to compile with optimization on.  */
    detect_C99_integers8();
    detect_C99_integers16();
    detect_C99_integers32();
    detect_C99_integers64();
}

/*-------------------------------------------------------------------------
 * Function:    detect_C99_floats
 *
 * Purpose:     Detect C99 floating point types
 *
 * Return:      void
 *-------------------------------------------------------------------------
 */
static void HDF_NO_UBSAN
detect_C99_floats(void)
{
#if H5_SIZEOF_DOUBLE == H5_SIZEOF_LONG_DOUBLE
    /*
     * If sizeof(double)==sizeof(long double) then assume that `long double'
     * isn't supported and use `double' instead.  This suppresses warnings on
     * some systems and `long double' is probably the same as `double' here
     * anyway.
     */
    DETECT_F(double, LDOUBLE, d_g[nd_g]);
    nd_g++;
#elif H5_SIZEOF_LONG_DOUBLE != 0
    DETECT_F(long double, LDOUBLE, d_g[nd_g]);
    nd_g++;
#endif
}

/*-------------------------------------------------------------------------
 * Function:    detect_alignments
 *
 * Purpose:     Detect structure alignments
 *
 * Return:      void
 *-------------------------------------------------------------------------
 */
static void HDF_NO_UBSAN
detect_alignments(void)
{
    /* Detect structure alignment for pointers, vlen and reference types */
    DETECT_M(void *, POINTER, m_g[na_g]);
    na_g++;
    DETECT_M(hvl_t, HVL, m_g[na_g]);
    na_g++;
    DETECT_M(hobj_ref_t, HOBJREF, m_g[na_g]);
    na_g++;
    DETECT_M(hdset_reg_ref_t, HDSETREGREF, m_g[na_g]);
    na_g++;
    DETECT_M(H5R_ref_t, REF, m_g[na_g]);
    na_g++;
}

#if defined(H5SETJMP) && defined(H5_HAVE_SIGNAL)
/* Verify the signal handler for signal signum works correctly multiple times.
 * One possible cause of failure is that the signal handling is blocked or
 * changed to SIG_DFL after H5LONGJMP.
 * Return  0 for success, -1 for failure.
 */
static int
verify_signal_handlers(int signum, void (*handler)(int))
{
#if defined(__has_feature) /* Clang */
#if __has_feature(address_sanitizer) || __has_feature(thread_sanitizer)
    /* Under the address and thread sanitizers, don't raise any signals. */
    return 0;
#endif
#elif defined(__SANITIZE_ADDRESS__) || defined(__SANITIZE_THREAD__) /* GCC */
    return 0;
#endif
    void (*save_handler)(int) = HDsignal(signum, handler);
    volatile int i, val;
    int          ntries     = 5;
    volatile int nfailures  = 0;
    volatile int nsuccesses = 0;

    for (i = 0; i < ntries; i++) {
        val = H5SETJMP(jbuf_g);
        if (val == 0) {
            /* send self the signal to trigger the handler */
            signal_handler_tested_g++;
            HDraise(signum);
            /* Should not reach here. Record error. */
            nfailures++;
        }
        else {
            if (val == signum) {
                /* return from signum handler. Record a sucess. */
                nsuccesses++;
            }
            else {
                fprintf(stderr, "Unknown return value (%d) from H5SETJMP", val);
                nfailures++;
            }
        }
    }
    /* restore save handler, check results and report failures */
    HDsignal(signum, save_handler);
    if (nfailures > 0 || nsuccesses != ntries) {
        fprintf(stderr,
                "verify_signal_handlers for signal %d did %d tries. "
                "Found %d failures and %d successes\n",
                signum, ntries, nfailures, nsuccesses);
        return -1;
    }
    else {
        /* all succeeded */
        return 0;
    }
}
#endif

/*-------------------------------------------------------------------------
 * Function:    main
 *
 * Purpose:     Main entry point.
 *
 * Return:      Success:    EXIT_SUCCESS
 *
 *-------------------------------------------------------------------------
 */
int HDF_NO_UBSAN
main(int argc, char *argv[])
{
    char *fname = NULL;
    FILE *f; /* temporary holding place for the stream pointer
              * so that rawoutstream is changed only when succeeded */

    if (argc > 1)
        fname = argv[1];

    /* First check if filename is string "NULL" */
    if (fname != NULL) {
        /* binary output */
        if ((f = HDfopen(fname, "w")) != NULL)
            rawoutstream = f;
    }
    if (!rawoutstream)
        rawoutstream = stdout;

#if defined(H5_HAVE_SETSYSINFO) && defined(SSI_NVPAIRS)
#if defined(UAC_NOPRINT) && defined(UAC_SIGBUS)
    /*
     * Make sure unaligned access generates SIGBUS and doesn't print warning
     * messages so that we can detect alignment constraints on the DEC Alpha.
     */
    int nvpairs[2];
    nvpairs[0] = SSIN_UACPROC;
    nvpairs[1] = UAC_NOPRINT | UAC_SIGBUS;
    if (setsysinfo(SSI_NVPAIRS, nvpairs, 1, 0, 0) < 0) {
        fprintf(stderr, "H5detect: unable to turn off UAC handling: %s\n", HDstrerror(errno));
    }
#endif
#endif

#if defined(H5SETJMP) && defined(H5_HAVE_SIGNAL)
    /* verify the SIGBUS and SIGSEGV handlers work properly */
    if (verify_signal_handlers(SIGBUS, sigbus_handler) != 0) {
        fprintf(stderr, "Signal handler %s for signal %d failed\n", "sigbus_handler", SIGBUS);
    }
    if (verify_signal_handlers(SIGSEGV, sigsegv_handler) != 0) {
        fprintf(stderr, "Signal handler %s for signal %d failed\n", "sigsegv_handler", SIGSEGV);
    }
    if (verify_signal_handlers(SIGILL, sigill_handler) != 0) {
        fprintf(stderr, "Signal handler %s for signal %d failed\n", "sigill_handler", SIGILL);
    }
#else
    align_status_g |= STA_NoHandlerVerify;
#endif

    print_header();

    /* C89 integer types */
    detect_C89_integers();

    /* C99 integer types */
    detect_C99_integers();

    /* C89 floating point types */
    detect_C89_floats();

    /* C99 floating point types */
    detect_C99_floats();

    /* Detect structure alignment */
    detect_alignments();

    print_results(nd_g, d_g, na_g, m_g);

    if (rawoutstream && rawoutstream != stdout) {
        if (HDfclose(rawoutstream))
            fprintf(stderr, "closing rawoutstream");
        else
            rawoutstream = NULL;
    }

    return EXIT_SUCCESS;
}

H5_GCC_DIAG_ON("cast-align")
