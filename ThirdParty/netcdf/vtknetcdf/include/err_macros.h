/* This is part of the netCDF package.
   Copyright 2018 University Corporation for Atmospheric Research/Unidata
   See COPYRIGHT file for conditions of use.

   Common includes, defines, etc., for test code in the libsrc4 and
   nc_test4 directories.

   Ed Hartnett, Russ Rew, Dennis Heimbigner
*/

#ifndef _ERR_MACROS_H
#define _ERR_MACROS_H

#include "config.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Err is used to keep track of errors within each set of tests,
 * total_err is the number of errors in the entire test program, which
 * generally cosists of several sets of tests. */
static int total_err = 0, err = 0;

/* This macro prints an error message with line number and name of
 * test program. */
#define ERR do {                                                        \
        fflush(stdout); /* Make sure our stdout is synced with stderr. */ \
        err++;                                                          \
        fprintf(stderr, "Sorry! Unexpected result, %s, line: %d\n",     \
                __FILE__, __LINE__);                                    \
        fflush(stderr);                                                 \
        return 2;                                                       \
    } while (0)

/* Duplicate with different name. */
#define ERR_RET ERR

#define ERR_GOTO do {                                                   \
        fflush(stdout); /* Make sure our stdout is synced with stderr. */ \
        fprintf(stderr, "Sorry! Unexpected result, %s, line: %d\n",     \
                __FILE__, __LINE__);                                    \
        goto error;                                                     \
    } while (0)

int ERR_report(int stat, const char* file, int line)
{
    fflush(stdout);
    fprintf(stderr, "Sorry! Unexpected result, %s, line: %d; status=%d\n",
            file,line,stat);
    fflush(stderr);
    return 1;
}

#define ERRSTAT(stat) {err+=ERR_report(stat,__FILE__,__LINE__);}

/* After a set of tests, report the number of errors, and increment
 * total_err. */
#define SUMMARIZE_ERR do {                      \
        if (err)                                \
        {                                       \
            printf("%d failures\n", err);       \
            total_err += err;                   \
            err = 0;                            \
        }                                       \
        else                                    \
            printf("ok.\n");                    \
    } while (0)

/* This macro prints out our total number of errors, if any, and exits
 * with a 0 if there are not, or a 2 if there were errors. Make will
 * stop if a non-zero value is returned from a test program. */
#define FINAL_RESULTS do {                                      \
        if (total_err)                                          \
        {                                                       \
            printf("%d errors detected! Sorry!\n", total_err);  \
            return 2;                                           \
        }                                                       \
        printf("*** Tests successful!\n");                      \
        return 0;                                               \
    } while (0)

/* This macro does the same as FINAL_RESULTS, but without the success
 * message. */
#define FINAL_RESULTS_QUIET do {                                \
        if (total_err)                                          \
        {                                                       \
            printf("%d errors detected! Sorry!\n", total_err);  \
            return 2;                                           \
        }                                                       \
        return 0;                                               \
    } while (0)

#endif /* _ERR_MACROS_H */
