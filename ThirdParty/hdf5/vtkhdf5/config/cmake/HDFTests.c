/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://www.hdfgroup.org/licenses.               *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


/* A simple test program to see if a function "works" */
#define SIMPLE_TEST(x) int main(void){ x; return 0; }

#ifdef HAVE___FLOAT128

/* Check if __float128 works (only used in the Fortran interface) */
int
main ()
{
    __float128 x;

    return 0;
}

#endif /* HAVE___FLOAT128 */

#ifdef HAVE_BUILTIN_EXPECT

int
main ()
{
    void *ptr = (void*) 0;

    if (__builtin_expect (ptr != (void*) 0, 1))
        return 0;

    return 0;
}

#endif /* HAVE_BUILTIN_EXPECT */

#ifdef HAVE_ATTRIBUTE

int
main ()
{
    int __attribute__((unused)) x;

    return 0;
}

#endif /* HAVE_ATTRIBUTE */

#ifdef HAVE_TIMEZONE

#ifdef HAVE_SYS_TIME_H
#   include <sys/time.h>
#endif
#include <time.h>
SIMPLE_TEST(timezone = 0);

#endif /* HAVE_TIMEZONE */

#ifdef SYSTEM_SCOPE_THREADS
#include <stdlib.h>
#include <pthread.h>

int main(void)
{
    pthread_attr_t attribute;
    int ret;

    pthread_attr_init(&attribute);
    ret = pthread_attr_setscope(&attribute, PTHREAD_SCOPE_SYSTEM);
    if (ret == 0)
        return 0;
    return 1;
}

#endif /* SYSTEM_SCOPE_THREADS */

#ifdef HAVE_SOCKLEN_T

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#   include <unistd.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#   include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#   include <sys/socket.h>
#endif

SIMPLE_TEST(socklen_t foo);

#endif /* HAVE_SOCKLEN_T */

#ifdef DEV_T_IS_SCALAR

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

int main ()
{
    dev_t d1, d2;
    if (d1 == d2)
        return 0;
    return 1;
}

#endif /* DEV_T_IS_SCALAR */

#ifdef HAVE_DEFAULT_SOURCE
/* Check default source */
#include <features.h>

int
main(void)
{
#ifdef __GLIBC_PREREQ
    return __GLIBC_PREREQ(2,19);
#else
    return 0;
#endif
}
#endif
