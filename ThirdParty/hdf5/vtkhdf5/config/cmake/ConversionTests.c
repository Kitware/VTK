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

#if defined(__has_attribute)
#   if __has_attribute(no_sanitize)
#       define HDF_NO_UBSAN __attribute__((no_sanitize("undefined")))
#   else
#       define HDF_NO_UBSAN
#   endif
#else
#   define HDF_NO_UBSAN
#endif

#ifdef H5_LDOUBLE_TO_LONG_SPECIAL_TEST

#include <string.h>
#include <stdlib.h>

int HDF_NO_UBSAN main(void)
{
    long double         ld = 20041683600089727.779961L;
    long                ll;
    unsigned long       ull;
    unsigned char       s[16];
    unsigned char       s2[8];
    int                 ret = 1;

    if (sizeof(long double) == 16 && sizeof(long) == 8) {
        /* Make sure the long double type has 16 bytes in size and
         * 11 bits of exponent.  If it is, the bit sequence should be
         * like below.  It's not a decent way to check but this info
         * isn't available.
         */
        memcpy(s, &ld, 16);
        if (s[0]==0x43 && s[1]==0x51 && s[2]==0xcc && s[3]==0xf3 &&
            s[4]==0x85 && s[5]==0xeb && s[6]==0xc8 && s[7]==0xa0 &&
            s[8]==0xbf && s[9]==0xcc && s[10]==0x2a && s[11]==0x3c) {

            /* Assign the hexadecimal value of long double type. */
            s[0]=0x43; s[1]=0x51; s[2]=0xcc; s[3]=0xf3;
            s[4]=0x85; s[5]=0xeb; s[6]=0xc8; s[7]=0xa0;
            s[8]=0xbf; s[9]=0xcc; s[10]=0x2a; s[11]=0x3c;
            s[12]=0x3d; s[13]=0x85; s[14]=0x56; s[15]=0x20;

            memcpy(&ld, s, 16);

            ll = (long)ld;
            memcpy(s2, &ll, 8);

            /* The library's algorithm converts it to 0x00 47 33 ce 17 af 22 82
             * and gets wrong value 20041683600089730 on Linux on IBM Power
             * architecture.
             *
             * But Linux on IBM Power converts it to 0x00 47 33 ce 17 af 22 7f
             * and gets the correct value 20041683600089727.  It uses some special
             * algorithm.  We're going to define the macro and skip the test until
             * we can figure out how they do it.
             */
            if (s2[0]==0x00 && s2[1]==0x47 && s2[2]==0x33 && s2[3]==0xce &&
                s2[4]==0x17 && s2[5]==0xaf && s2[6]==0x22 && s2[7]==0x7f)

                ret = 0;

            ull = (unsigned long)ld;
            memcpy(s2, &ull, 8);

            /* The unsigned long is the same as signed long */
            if(s2[0]==0x00 && s2[1]==0x47 && s2[2]==0x33 && s2[3]==0xce &&
               s2[4]==0x17 && s2[5]==0xaf && s2[6]==0x22 && s2[7]==0x7f)

                ret = 0;
        }
    }

done:
    exit(ret);
}

#endif

#ifdef H5_LONG_TO_LDOUBLE_SPECIAL_TEST

#include <string.h>
#include <stdlib.h>

int HDF_NO_UBSAN main(void)
{
    long double         ld;
    long                ll;
    unsigned long       ull;
    unsigned char       s[16];
    int                 flag=0, ret=1;

    /* Determine if long double has 16 byte in size, 11 bit exponent, and
     * the bias is 0x3ff
     */
    if (sizeof(long double) == 16) {
        ld = 1.0L;
        memcpy(s, &ld, 16);

        if (s[0]==0x3f && s[1]==0xf0 && s[2]==0x00 && s[3]==0x00 &&
            s[4]==0x00 && s[5]==0x00 && s[6]==0x00 && s[7]==0x00)

            flag = 1;
    }

    if (flag==1 && sizeof(long)==8) {
        ll = 0x003fffffffffffffL;
        ld = (long double)ll;
        memcpy(s, &ld, 16);

        /* The library converts the value to 0x434fffffffffffff8000000000000000.
         * In decimal it is 18014398509481982.000000, one value short of the original.
         *
         * Linux on IBM Power architecture converts it to
         * 0x4350000000000000bff0000000000000. The value is correct in decimal.
         * It uses some special algorithm.  We're going to define the macro and
         * skip the test until we can figure out how they do it.
         */
        if (s[0]==0x43 && s[1]==0x50 && s[2]==0x00 && s[3]==0x00 &&
            s[4]==0x00 && s[5]==0x00 && s[6]==0x00 && s[7]==0x00 &&
            s[8]==0xbf && s[9]==0xf0 && s[10]==0x00 && s[11]==0x00 &&
            s[12]==0x00 && s[13]==0x00 && s[14]==0x00 && s[15]==0x00)

            ret = 0;
    }

    if (flag==1 && sizeof(unsigned long)==8) {
        ull = 0xffffffffffffffffUL;
        ld = (long double)ull;
        memcpy(s, &ld, 16);

        /* Use a different value from signed long to test. The problem is the
         * same for both long and unsigned long. The value is 18446744073709551615.
         * The library converts the value to 0x43effffffffffffffe000000000000000.
         * In decimal it's 18446744073709548544.000000, very different from the
         * original. Linux on IBM Power architecture converts it to
         * 0x43f0000000000000bff0000000000000. The value is correct in decimal.
         * It uses some special algorithm.  We're going to define the macro and
         * skip the test until we can figure out how they do it.
         */
        if (s[0]==0x43 && s[1]==0xf0 && s[2]==0x00 && s[3]==0x00 &&
            s[4]==0x00 && s[5]==0x00 && s[6]==0x00 && s[7]==0x00 &&
            s[8]==0xbf && s[9]==0xf0 && s[10]==0x00 && s[11]==0x00 &&
            s[12]==0x00 && s[13]==0x00 && s[14]==0x00 && s[15]==0x00)

            ret = 0;
        }
done:
    exit(ret);
}

#endif

#ifdef H5_LDOUBLE_TO_LLONG_ACCURATE_TEST

#include <string.h>
#include <stdlib.h>

int HDF_NO_UBSAN main(void)
{
    long double         ld = 20041683600089727.779961L;
    long long           ll;
    unsigned long long  ull;
    unsigned char       s[16];
    int                 ret = 0;

    if (sizeof(long double) == 16) {
        /* Make sure the long double type is the same as the failing type
         * which has 16 bytes in size and 11 bits of exponent.  If it is,
         * the bit sequence should be like below.  It's not
         * a decent way to check but this info isn't available.
         */
        memcpy(s, &ld, 16);

        if (s[0]==0x43 && s[1]==0x51 && s[2]==0xcc && s[3]==0xf3 &&
            s[4]==0x85 && s[5]==0xeb && s[6]==0xc8 && s[7]==0xa0 &&
            s[8]==0xbf && s[9]==0xcc && s[10]==0x2a && s[11]==0x3c) {

            /* Slightly adjust the bit sequence (s[8]=0xdf).  The converted
             * values will go wild on Mac OS 10.4 and IRIX64 6.5.
             */
            s[0]=0x43; s[1]=0x51; s[2]=0xcc; s[3]=0xf3;
            s[4]=0x85; s[5]=0xeb; s[6]=0xc8; s[7]=0xa0;
            s[8]=0xdf; s[9]=0xcc; s[10]=0x2a; s[11]=0x3c;
            s[12]=0x3d; s[13]=0x85; s[14]=0x56; s[15]=0x20;

            memcpy(&ld, s, 16);
            ll = (long long)ld;
            ull = (unsigned long long)ld;

            if (ll != 20041683600089728 || ull != 20041683600089728)
                ret = 1;
        }
    }
done:
    exit(ret);
}
#endif

#ifdef H5_LLONG_TO_LDOUBLE_CORRECT_TEST

#include <string.h>
#include <stdlib.h>

int HDF_NO_UBSAN main(void)
{
    long double         ld;
    long long           ll;
    unsigned long long  ull;
    unsigned char       s[16];
    int                 flag=0, ret=0;

    /* Determine if long double has 16 byte in size, 11 bit exponent, and
     * the bias is 0x3ff
     */
    if (sizeof(long double) == 16) {
        ld = 1.0L;
        memcpy(s, &ld, 16);
        if (s[0]==0x3f && s[1]==0xf0 && s[2]==0x00 && s[3]==0x00 &&
            s[4]==0x00 && s[5]==0x00 && s[6]==0x00 && s[7]==0x00)

            flag = 1;
    }

    if (flag==1 && sizeof(long long)==8) {
        ll = 0x01ffffffffffffffLL;
        ld = (long double)ll;
        memcpy(s, &ld, 16);

        /* Check if the bit sequence is as expected*/
        if (s[0]!=0x43 || s[1]!=0x7f || s[2]!=0xff || s[3]!=0xff ||
            s[4]!=0xff || s[5]!=0xff || s[6]!=0xff || s[7]!=0xff ||
            s[8]!=0xf0 || s[9]!=0x00 || s[10]!=0x00 || s[11]!=0x00)

            ret = 1;
    }
    if (flag==1 && sizeof(unsigned long long)==8) {
        ull = 0x01ffffffffffffffULL;
        ld = (long double)ull;
        memcpy(s, &ld, 16);

        if (s[0]!=0x43 || s[1]!=0x7f || s[2]!=0xff || s[3]!=0xff ||
            s[4]!=0xff || s[5]!=0xff || s[6]!=0xff || s[7]!=0xff ||
            s[8]!=0xf0 || s[9]!=0x00 || s[10]!=0x00 || s[11]!=0x00)

            ret = 1;
    }

done:
    exit(ret);
}
#endif

#ifdef H5_DISABLE_SOME_LDOUBLE_CONV_TEST

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int HDF_NO_UBSAN main(void)
{
    FILE *fp;
    char cpu[64];

    fp = popen("uname -m", "r");

    fgets(cpu, sizeof(cpu)-1, fp);

    pclose(fp);

    if (strncmp(cpu, "ppc64le", 7) == 0)
        return 0;

    return 1;
}

#endif

#if defined(H5_FLOAT16_CONVERSION_FUNCS_LINK_TEST) || defined(H5_FLOAT16_CONVERSION_FUNCS_LINK_NO_FLAGS_TEST)

#define __STDC_WANT_IEC_60559_TYPES_EXT__

#include <stdlib.h>
#include <float.h>

int HDF_NO_UBSAN main(void)
{
    _Float16           fl16_var;
    signed char        sc;
    unsigned char      usc;
    short              s;
    unsigned short     us;
    int                i;
    unsigned int       ui;
    long               l;
    unsigned long      ul;
    long long          ll;
    unsigned long long ull;
    float              f;
    double             d;
    long double        ld;
    int                ret = 0;

    /*
     * Cast the _Float16 type between all the different C datatypes
     * we support conversions for in H5Tconv.c to check if the compiler
     * properly links any software conversion functions it may generate
     * for the casts, such as __extendhfsf2 or __truncdfhf2.
     */

    fl16_var = 3.0f16;

    sc  = (signed char)fl16_var;
    usc = (unsigned char)fl16_var;
    s   = (short)fl16_var;
    us  = (unsigned short)fl16_var;
    i   = (int)fl16_var;
    ui  = (unsigned int)fl16_var;
    l   = (long)fl16_var;
    ul  = (unsigned long)fl16_var;
    ll  = (long long)fl16_var;
    ull = (unsigned long long)fl16_var;
    f   = (float)fl16_var;
    d   = (double)fl16_var;
    ld  = (long double)fl16_var;

    sc       = (signed char)3;
    fl16_var = (_Float16)sc;

    usc      = (unsigned char)3;
    fl16_var = (_Float16)usc;

    s        = (short)3;
    fl16_var = (_Float16)s;

    us       = (unsigned short)3;
    fl16_var = (_Float16)us;

    i        = (int)3;
    fl16_var = (_Float16)i;

    ui       = (unsigned int)3;
    fl16_var = (_Float16)ui;

    l        = (long)3;
    fl16_var = (_Float16)l;

    ul       = (unsigned long)3;
    fl16_var = (_Float16)ul;

    ll       = (long long)3;
    fl16_var = (_Float16)ll;

    ull      = (unsigned long long)3;
    fl16_var = (_Float16)ull;

    f        = (float)3.0f;
    fl16_var = (_Float16)f;

    d        = (double)3.0;
    fl16_var = (_Float16)d;

    ld       = (long double)3.0l;
    fl16_var = (_Float16)ld;

done:
    exit(ret);
}

#endif

#if defined(H5_LDOUBLE_TO_FLOAT16_CORRECT_TEST) || defined(H5_LDOUBLE_TO_FLOAT16_CORRECT_NO_FLAGS_TEST)

#define __STDC_WANT_IEC_60559_TYPES_EXT__

#include <stdlib.h>
#include <float.h>
#include <math.h>
#include <limits.h>

int HDF_NO_UBSAN main(void)
{
    long double ld;
    _Float16    half;
    int         ret = 1;

    ld   = 32.0L;
    half = 64.0f16;

    half = (_Float16)ld;

    if (fabsl(ld - (long double)half) < LDBL_EPSILON)
        ret = 0;

done:
    exit(ret);
}

#endif
