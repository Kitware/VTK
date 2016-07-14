/* This file performs several undefined behaviors, suppress Undefined
 * Behavior Sanitizer (UBSan) from warning.
 */
#if defined(__clang__)
    #if __has_attribute(no_sanitize)
        #define HDF_NO_UBSAN __attribute__((no_sanitize("undefined")))
    #else
        #define HDF_NO_UBSAN
    #endif
#else
    #define HDF_NO_UBSAN
#endif

#ifdef H5_FP_TO_INTEGER_OVERFLOW_WORKS_TEST

int main(void) HDF_NO_UBSAN
{
  float f = 2147483648.0f;
  int i;

  i = (int)f;

  done:
  exit(0);
}

#endif

#ifdef H5_FP_TO_ULLONG_ACCURATE_TEST

int main(void) HDF_NO_UBSAN
{
  float f = 111.60f;
  double d = 222.55L;
  unsigned long long l1 = (unsigned long long)f;
  unsigned long long l2 = (unsigned long long)d;
  int ret = 0;

  if(l1 == 112)
  ret = 1;
  if(l2 == 223)
  ret = 1;

  done:
  exit(ret);
}

#endif

#ifdef H5_FP_TO_ULLONG_RIGHT_MAXIMUM_TEST
int main(void) HDF_NO_UBSAN
{
  float f =        9701917572145405952.00f;
  double d1 =      9701917572145405952.00L;
  long double d2 = 9701917572145405952.00L;
  double d3 = 2e40L;
  unsigned long long l1 = (unsigned long long)f;
  unsigned long long l2 = (unsigned long long)d1;
  unsigned long long l3 = (unsigned long long)d2;
  unsigned long long l4;
  unsigned long long l5 = 0x7fffffffffffffffULL;
  int ret = 0;

  if(l1 <= l5 || l2 <= l5 || l3 <= l5)
  ret = 1;

  l4 = (unsigned long long)d3;
  if(l4 <= l5)
  ret = 1;

  done:
  exit(ret);
}

#endif

#ifdef H5_LDOUBLE_TO_INTEGER_WORKS_TEST

#include <stdlib.h>
#include <string.h>

int main(void) HDF_NO_UBSAN
{
  void *align;
  long double ld = 9701917572145405952.00L;
  unsigned char v1;
  short v2;
  unsigned int v3;
  int ret = 0;

  align = (void*) malloc(sizeof(long double));
  memcpy(align, &ld, sizeof(long double));

  /*For HU-UX11.00, there's floating exception(core dump) when doing some of casting
   *from 'long double' to integers*/
  v1 = (unsigned char) (*((long double*) align));
  v2 = (short) (*((long double*) align));
  v3 = (unsigned int) (*((long double*) align));

  exit(ret);
}

#endif

#ifdef H5_LDOUBLE_TO_LLONG_ACCURATE_TEST
int main(void) HDF_NO_UBSAN
{
  long double ld = 20041683600089727.779961L;
  long long ll;
  unsigned long long ull;
  unsigned char s[16];
  int ret = 0;

  if(sizeof(long double) == 16)
  {
    /*make sure the long double type is the same as the failing type
     *which has 16 bytes in size and 11 bits of exponent.  If it is,
     *the bit sequence should be like below.  It's not
     *a decent way to check but this info isn't available. */
    memcpy(s, &ld, 16);
    if(s[0]==0x43 && s[1]==0x51 && s[2]==0xcc && s[3]==0xf3 &&
        s[4]==0x85 && s[5]==0xeb && s[6]==0xc8 && s[7]==0xa0 &&
        s[8]==0xbf && s[9]==0xcc && s[10]==0x2a && s[11]==0x3c)
    {

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

#ifdef H5_LDOUBLE_TO_UINT_ACCURATE_TEST
int main(void) HDF_NO_UBSAN
{
  long double ld = 2733248032.9183987530L;
  unsigned int i;
  int ret = 0;

  i = (unsigned int)ld;
  if(i!=2733248032 && i!=2733248031 && i!=2733248033)
  ret = 1;

  done:
  exit(ret);
}
#endif

#ifdef H5_LLONG_TO_LDOUBLE_CORRECT_TEST
int main(void) HDF_NO_UBSAN
{
  long double ld;
  long long ll;
  unsigned long long ull;
  unsigned char s[16];
  int flag=0, ret=0;

  /*Determine if long double has 16 byte in size, 11 bit exponent, and
   *the bias is 0x3ff */
  if(sizeof(long double) == 16)
  {
    ld = 1.0L;
    memcpy(s, &ld, 16);
    if(s[0]==0x3f && s[1]==0xf0 && s[2]==0x00 && s[3]==0x00 &&
        s[4]==0x00 && s[5]==0x00 && s[6]==0x00 && s[7]==0x00)
    flag = 1;
  }

  if(flag==1 && sizeof(long long)==8)
  {
    ll = 0x01ffffffffffffffLL;
    ld = (long double)ll;
    memcpy(s, &ld, 16);
    /*Check if the bit sequence is as supposed to be*/
    if(s[0]!=0x43 || s[1]!=0x7f || s[2]!=0xff || s[3]!=0xff ||
        s[4]!=0xff || s[5]!=0xff || s[6]!=0xff || s[7]!=0xff ||
        s[8]!=0xf0 || s[9]!=0x00 || s[10]!=0x00 || s[11]!=0x00)
    ret = 1;
  }
  if(flag==1 && sizeof(unsigned long long)==8)
  {
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

typedef struct
{
  size_t len;
  void *p;
}hvl_t;

#ifdef FC_DUMMY_MAIN
#ifndef FC_DUMMY_MAIN_EQ_F77
#  ifdef __cplusplus
extern "C"
#  endif
int FC_DUMMY_MAIN()
{ return 1;}
#endif
#endif
int
main () HDF_NO_UBSAN
{

  char *chp = "beefs";
  char **chpp = malloc (2 * sizeof (char *));
  char **chpp2;
  hvl_t vl =
  { 12345, (void *) chp};
  hvl_t *vlp;
  hvl_t *vlp2;

  memcpy ((void *) ((char *) chpp + 1), &chp, sizeof (char *));
  chpp2 = (char **) ((char *) chpp + 1);
  if (strcmp (*chpp2, chp))
  {
    free (chpp);
    return 1;
  }
  free (chpp);

  vlp = malloc (2 * sizeof (hvl_t));
  memcpy ((void *) ((char *) vlp + 1), &vl, sizeof (hvl_t));
  vlp2 = (hvl_t *) ((char *) vlp + 1);
  if (vlp2->len != vl.len || vlp2->p != vl.p)
  {
    free (vlp);
    return 1;
  }
  free (vlp);

  ;
  return 0;
}

#endif

#ifdef H5_ULLONG_TO_LDOUBLE_PRECISION_TEST

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

int main(void) HDF_NO_UBSAN
{
  /* General variables */
  int endian;
  int tst_value = 1;
  int ret = 0;

  /* For FreeBSD */
  unsigned long long l = 0xa601e80bda85fcefULL;
  long double ld;
  unsigned char *c1, *c2;
  size_t size;

  /* For Cygwin */
  unsigned long long l_cyg = 0xfffffffffffffff0ULL;
  long double ld_cyg;
  unsigned char *c2_cyg;
  size_t size_cyg;

  /* Determine this system's endianess */
  c1 = (unsigned char*)calloc(1, sizeof(int));
  memcpy((void*)c1, &tst_value, sizeof(int));
  if(c1[0]==1)
  endian = 0; /* little endian */
  else
  endian = 1; /* big endian */

  /* For FreeBSD */
  size = sizeof(long double);
  memset(&ld, 0, size);
  ld = (long double)l;

  c2 = (unsigned char*)calloc(1, size);
  memcpy((void*)c2, &ld, size);

  /* Test if the last 2 bytes of mantissa are lost.  Mainly for FreeBSD on Intel
   * architecture(sleipnir) where it happens. */
  /*if(endian==0 && c2[0]==0 && c2[1]==0)*//*little endian*/
  if(endian==0 && c2[0]==0)
  { /*little endian*/
    ret = 1;
    goto done;
  }

  /* For Cygwin */
  size_cyg = sizeof(long double);
  memset(&ld_cyg, 0, size);
  ld_cyg = (long double)l_cyg;

  c2_cyg = (unsigned char*)calloc(1, size_cyg);
  memcpy((void*)c2_cyg, &ld_cyg, size_cyg);

  /* Test if the last 4 bytes(roughly) of mantissa are rounded up.  Mainly for Cygwin
   * where the values like 0xffffffffffffffff,  0xfffffffffffffffe, ...,
   * 0xfffffffffffff000 ... are rounded up as 0x0000403f8000000000000000
   * instead of 0x0000403effffffffffffffff, 0x0000403efffffffffffffffe, ...,
   * 0x0000403efffffffffffff000 ...
   */
  if(endian==0 && c2_cyg[0]==0 && c2_cyg[1]==0 && c2_cyg[2]==0 && c2_cyg[3]==0)
  ret = 1;

  done:
  if(c1)
  free(c1);
  if(c2)
  free(c2);
  if(c2_cyg)
  free(c2_cyg);
  exit(ret);
}

#endif


#ifdef H5_ULONG_TO_FLOAT_ACCURATE_TEST

int main(void) HDF_NO_UBSAN
{
    int           ret = 0;
    unsigned long l1;
    unsigned long l2;
    unsigned long l3;
    float f1;
    float f2;
    float f3;


    if(sizeof(unsigned long)==8) {
        l1 = 0xffffffffffffffffUL;
        l2 = 0xffffffffffff0000UL;
        l3 = 0xf000000000000000UL;

        f1 = (float)l1;
        f2 = (float)l2;
        f3 = (float)l3;

        if((f1 < 0) || (f2 < 0) || (f3 < 0))
            ret = 1;
    }

done:
    exit(ret);
}

#endif

#ifdef H5_ULONG_TO_FP_BOTTOM_BIT_ACCURATE_TEST

#include <string.h>


int main(void) HDF_NO_UBSAN
{
    unsigned long l1;
    unsigned long l2;
    unsigned long l3;
    unsigned long l4;
    unsigned long long ld1;
    unsigned long long ld2;
    unsigned long long ld3;
    unsigned long long ld4;
    double        d1, d2, d3, d4;
    unsigned char s[8];
    int           ret = 0;

    if(sizeof(unsigned long)==8) {
        l1 = 0xf000000000000b00UL; /*Round-down case*/
        l2 = 0xf000000000000401UL; /*Round-up case*/
        l3 = 0xf000000000000400UL; /*Round-down case*/
        l4 = 0xf000000000000c00UL; /*Round-up case*/

        d1 = (double)l1;
        d2 = (double)l2;
        d3 = (double)l3;
        d4 = (double)l4;
    } else if(sizeof(unsigned long long)==8) {
        ld1 = 0xf000000000000b00ULL; /*Round-down case*/
        ld2 = 0xf000000000000401ULL; /*Round-up case*/
        ld3 = 0xf000000000000400ULL; /*Round-down case*/
        ld4 = 0xf000000000000c00ULL; /*Round-up case*/

        d1 = (double)ld1;
        d2 = (double)ld2;
        d3 = (double)ld3;
        d4 = (double)ld4;
    } else {
        ret = 1;
        goto done;
    }

    memcpy(s, &d1, 8);
    if(s[7]!=1)
        ret = 1;

    memcpy(s, &d2, 8);
    if(s[7]!=1)
        ret = 1;

    memcpy(s, &d3, 8);
    if(s[7]!=0)
        ret = 1;

    memcpy(s, &d4, 8);
    if(s[7]!=2)
        ret = 1;

done:
    exit(ret);
}
#endif
