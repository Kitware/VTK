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

#ifdef PTHREAD_BARRIER
#include <pthread.h>

int main(void)
{
    pthread_barrier_t barr;
    int ret;

    ret = pthread_barrier_init(&barr, NULL, 1);
    if (ret == 0)
        return 0;
    return 1;
}

#endif /* PTHREAD_BARRIER */

#ifdef HAVE_SOCKLEN_T

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#   include <unistd.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#   include <sys/socket.h>
#endif

SIMPLE_TEST(socklen_t foo);

#endif /* HAVE_SOCKLEN_T */

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

#ifdef HAVE_STDC_NO_COMPLEX
#ifndef __STDC_NO_COMPLEX__
#error "__STDC_NO_COMPLEX__ not defined"
#else
int
main(void)
{
    return 0;
}
#endif
#endif

#ifdef HAVE_COMPLEX_NUMBERS
#include <complex.h>

#if defined(_MSC_VER) && !defined(__llvm__) && !defined(__INTEL_LLVM_COMPILER)

typedef _Fcomplex H5_float_complex;
typedef _Dcomplex H5_double_complex;
typedef _Lcomplex H5_ldouble_complex;
#define H5_make_fcomplex _FCbuild
#define H5_make_dcomplex _Cbuild
#define H5_make_lcomplex _LCbuild

#else

typedef float _Complex H5_float_complex;
typedef double _Complex H5_double_complex;
typedef long double _Complex H5_ldouble_complex;
static float _Complex
H5_make_fcomplex(float real, float imaginary)
{
    return real + imaginary * (float _Complex)_Complex_I;
}
static double _Complex
H5_make_dcomplex(double real, double imaginary)
{
    return real + imaginary * (double _Complex)_Complex_I;
}
static long double _Complex
H5_make_lcomplex(long double real, long double imaginary)
{
    return real + imaginary * (long double _Complex)_Complex_I;
}
#endif

int
main(void)
{
    H5_float_complex z1   = H5_make_fcomplex(1.0f, 1.0f);
    H5_double_complex z2  = H5_make_dcomplex(2.0, 4.0);
    H5_ldouble_complex z3 = H5_make_lcomplex(3.0L, 5.0L);
    float r1              = crealf(z1);
    float i1              = cimagf(z1);
    double r2             = creal(z2);
    double i2             = cimag(z2);
    long double r3        = creall(z3);
    long double i3        = cimagl(z3);
    return 0;
}
#endif
