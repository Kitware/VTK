/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://support.hdfgroup.org/ftp/HDF5/releases.  *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#if defined(__has_attribute)
#if __has_attribute(no_sanitize)
#define HDF_NO_UBSAN __attribute__((no_sanitize("undefined")))
#else
#define HDF_NO_UBSAN
#endif
#else
#define HDF_NO_UBSAN
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

    if(sizeof(long double) == 16 && sizeof(long) == 8) {
    /*make sure the long double type has 16 bytes in size and
    * 11 bits of exponent.  If it is,
    *the bit sequence should be like below.  It's not
    *a decent way to check but this info isn't available. */
    memcpy(s, &ld, 16);
    if(s[0]==0x43 && s[1]==0x51 && s[2]==0xcc && s[3]==0xf3 &&
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

        /* The library's algorithm converts it to 0x 00 47 33 ce 17 af 22 82
        * and gets wrong value 20041683600089730 on the IBM Power6 Linux.
        * But the IBM Power6 Linux converts it to 0x00 47 33 ce 17 af 22 7f
        * and gets the correct value 20041683600089727.  It uses some special
        * algorithm.  We're going to define the macro and skip the test until
        * we can figure out how they do it. */
        if(s2[0]==0x00 && s2[1]==0x47 && s2[2]==0x33 && s2[3]==0xce &&
            s2[4]==0x17 && s2[5]==0xaf && s2[6]==0x22 && s2[7]==0x7f)
        ret = 0;

        ull = (unsigned long)ld;
        memcpy(s2, &ull, 8);

        /* The unsigned long is the same as signed long. */
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

    /*Determine if long double has 16 byte in size, 11 bit exponent, and
     *the bias is 0x3ff */
    if(sizeof(long double) == 16) {
    ld = 1.0L;
    memcpy(s, &ld, 16);
    if(s[0]==0x3f && s[1]==0xf0 && s[2]==0x00 && s[3]==0x00 &&
        s[4]==0x00 && s[5]==0x00 && s[6]==0x00 && s[7]==0x00)
        flag = 1;
    }

    if(flag==1 && sizeof(long)==8) {
    ll = 0x003fffffffffffffL;
    ld = (long double)ll;
    memcpy(s, &ld, 16);
    /* The library converts the value to 0x434fffffffffffff8000000000000000.
    * In decimal it is 18014398509481982.000000, one value short of the original.
    * The IBM Power6 Linux converts it to 0x4350000000000000bff0000000000000.
    * The value is correct in decimal. It uses some special
    * algorithm.  We're going to define the macro and skip the test until
    * we can figure out how they do it. */
    if(s[0]==0x43 && s[1]==0x50 && s[2]==0x00 && s[3]==0x00 &&
        s[4]==0x00 && s[5]==0x00 && s[6]==0x00 && s[7]==0x00 &&
        s[8]==0xbf && s[9]==0xf0 && s[10]==0x00 && s[11]==0x00 &&
        s[12]==0x00 && s[13]==0x00 && s[14]==0x00 && s[15]==0x00)
        ret = 0;
    }
    if(flag==1 && sizeof(unsigned long)==8) {
    ull = 0xffffffffffffffffUL;
    ld = (long double)ull;
    memcpy(s, &ld, 16);
    /* Use a different value from signed long to test. The problem is the same
    * for both long and unsigned long. The value is 18446744073709551615.
    * The library converts the value to 0x43effffffffffffffe000000000000000.
    * In decimal it's 18446744073709548544.000000, very different from the original.
    * The IBM Power6 Linux converts it to 0x43f0000000000000bff0000000000000.
    * The value is correct in decimal. It uses some special
    * algorithm.  We're going to define the macro and skip the test until
    * we can figure out how they do it. */
    if(s[0]==0x43 && s[1]==0xf0 && s[2]==0x00 && s[3]==0x00 &&
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

    if(sizeof(long double) == 16) {
        /*make sure the long double type is the same as the failing type
         *which has 16 bytes in size and 11 bits of exponent.  If it is,
         *the bit sequence should be like below.  It's not
         *a decent way to check but this info isn't available. */
        memcpy(s, &ld, 16);
        if(s[0]==0x43 && s[1]==0x51 && s[2]==0xcc && s[3]==0xf3 &&
            s[4]==0x85 && s[5]==0xeb && s[6]==0xc8 && s[7]==0xa0 &&
            s[8]==0xbf && s[9]==0xcc && s[10]==0x2a && s[11]==0x3c) {

            /*slightly adjust the bit sequence (s[8]=0xdf).  The converted
             *values will go wild on Mac OS 10.4 and IRIX64 6.5.*/
            s[0]=0x43; s[1]=0x51; s[2]=0xcc; s[3]=0xf3;
            s[4]=0x85; s[5]=0xeb; s[6]=0xc8; s[7]=0xa0;
            s[8]=0xdf; s[9]=0xcc; s[10]=0x2a; s[11]=0x3c;
            s[12]=0x3d; s[13]=0x85; s[14]=0x56; s[15]=0x20;

            memcpy(&ld, s, 16);
            ll = (long long)ld;
            ull = (unsigned long long)ld;

            if(ll != 20041683600089728 || ull != 20041683600089728)
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

    /*Determine if long double has 16 byte in size, 11 bit exponent, and
     *the bias is 0x3ff */
    if(sizeof(long double) == 16) {
        ld = 1.0L;
        memcpy(s, &ld, 16);
        if(s[0]==0x3f && s[1]==0xf0 && s[2]==0x00 && s[3]==0x00 &&
            s[4]==0x00 && s[5]==0x00 && s[6]==0x00 && s[7]==0x00)
            flag = 1;
    }

    if(flag==1 && sizeof(long long)==8) {
        ll = 0x01ffffffffffffffLL;
        ld = (long double)ll;
        memcpy(s, &ld, 16);
        /*Check if the bit sequence is as supposed to be*/
        if(s[0]!=0x43 || s[1]!=0x7f || s[2]!=0xff || s[3]!=0xff ||
            s[4]!=0xff || s[5]!=0xff || s[6]!=0xff || s[7]!=0xff ||
            s[8]!=0xf0 || s[9]!=0x00 || s[10]!=0x00 || s[11]!=0x00)
            ret = 1;
    }
    if(flag==1 && sizeof(unsigned long long)==8) {
        ull = 0x01ffffffffffffffULL;
        ld = (long double)ull;
        memcpy(s, &ld, 16);
        if(s[0]!=0x43 || s[1]!=0x7f || s[2]!=0xff || s[3]!=0xff ||
            s[4]!=0xff || s[5]!=0xff || s[6]!=0xff || s[7]!=0xff ||
            s[8]!=0xf0 || s[9]!=0x00 || s[10]!=0x00 || s[11]!=0x00)
            ret = 1;
    }
done:
    exit(ret);
}
#endif

#ifdef H5_NO_ALIGNMENT_RESTRICTIONS_TEST

#include <stdlib.h>
#include <string.h>

typedef struct {
    size_t len;
    void *p;
} hvl_t;

#ifdef FC_DUMMY_MAIN
#ifndef FC_DUMMY_MAIN_EQ_F77
#  ifdef __cplusplus
extern "C"
#  endif
int FC_DUMMY_MAIN()
{ return 1;}
#endif
#endif
int HDF_NO_UBSAN
main ()
{

    char *chp = "beefs";
    char **chpp = malloc (2 * sizeof (char *));
    char **chpp2;
    hvl_t vl = { 12345, (void *) chp };
    hvl_t *vlp;
    hvl_t *vlp2;

    memcpy ((void *) ((char *) chpp + 1), &chp, sizeof (char *));
    chpp2 = (char **) ((char *) chpp + 1);
    if (strcmp (*chpp2, chp)) {
        free (chpp);
        return 1;
    }
    free (chpp);

    vlp = malloc (2 * sizeof (hvl_t));
    memcpy ((void *) ((char *) vlp + 1), &vl, sizeof (hvl_t));
    vlp2 = (hvl_t *) ((char *) vlp + 1);
    if (vlp2->len != vl.len || vlp2->p != vl.p) {
        free (vlp);
        return 1;
    }
    free (vlp);

  ;
  return 0;
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

    if(strncmp(cpu, "ppc64le", 7) == 0)
        return 0;

    return 1;
}

#endif
