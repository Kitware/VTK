/*
 *  Copyright 1996, University Corporation for Atmospheric Research
 *  See netcdf/COPYRIGHT file for copying and redistribution conditions.
 *   
 */
/* Id */
#ifndef _CRAY
#error "ncx_cray.c is a cray specific implementation"
#endif

/*
 * An external data representation interface.
 */
/*
 * TODO: Fix "off diagonal" functions (ncx_{put,get}[n]_t1_t2() s.t. t1 != t2)
 * to be use IEG functions when diagonals are.
 *
 * Whine to cray about IEG function limiting behavior.
 */

#include <string.h>
#include <limits.h>
/* alias poorly named limits.h macros */
#define  SHORT_MAX  SHRT_MAX
#define  SHORT_MIN  SHRT_MIN
#define USHORT_MAX USHRT_MAX
#include <float.h>
#include <assert.h>
#include "ncx.h"

/**/
#if USE_IEG
#define C_SIZE_T size_t

extern int
CRAY2IEG(
  const int *typep,
  const C_SIZE_T *nump,
  word *foreignp,
  const int *bitoffp,
  const void *local,
  const int *stride
);

extern int
IEG2CRAY(
  const int *typep,
  const C_SIZE_T *nump,
  const word *foreignp,
  const int *bitoffp,
  void *local,
  const int *stride
);


static const int Zero = 0;
static const C_SIZE_T One = 1;
static const int ThirtyTwo = 32;
static const int UnitStride = 1;
static const int Cray2_I32 = 1;  /* 32 bit two complement */
static const int Cray2_F32 = 2;  /* IEEE single precision */
static const int Cray2_I16 = 7;  /* 16 bit twos complement */
static const int Cray2_F64 = 8;  /* CRAY float to IEEE double */

#define SHORT_USE_IEG 1
#define INT_USE_IEG 1
#define FLOAT_USE_IEG 1
#define DOUBLE_USE_IEG 1

#if _CRAY1
/*
 * Return the number of bits "into" a word that (void *) is.
 * N.B. This is based on the CRAY1 (PVP) address structure,
 * which puts the address within a word in the leftmost 3 bits
 * of the address.
 */
static size_t
bitoff(const void *vp)
{
  const size_t bitoffset = ((size_t)vp >> (64 - 6)) & 0x3f;
  return bitoffset;
}
# else
#error "Don't use IEG2CRAY, CRAY2IEG except on CRAY1 (MPP) platforms"
#define bitoff(vp) ((size_t)(vp) % 64) /* N.B. Assumes 64 bit word */
# endif /* _CRAY1 */

#endif /* USE_IEG */

#if _CRAY1
/*
 * Return the number of bytes "into" a word that (void *) is.
 * N.B. This is based on the CRAY1 (PVP) address structure,
 * which puts the address within a word in the leftmost 3 bits
 * of the address.
 */
static size_t
byteoff(const void *vp)
{
  const size_t byteoffset = ((size_t)vp >> (64 - 3)) & 0x7;
  return byteoffset;
}
#else
#define byteoff(vp) ((size_t)(vp) % 8) /* N.B. Assumes 64 bit word */
#endif /* _CRAY1 */

/*
 * Return the number of bytes until the next "word" boundary
 */
static size_t
word_align(const void *vp)
{
  const size_t rem = byteoff(vp);
  if(rem == 0)
    return 0;
  return sizeof(word) - rem;
}


static const char nada[X_ALIGN] = {0, 0, 0, 0};


/*
 * Primitive numeric conversion functions.
 */

/* x_schar */

 /* We don't implement and x_schar primitives. */


/* x_short */

typedef short ix_short;
#define SIZEOF_IX_SHORT SIZEOF_SHORT
#define IX_SHORT_MAX SHORT_MAX

static void
cget_short_short(const void *xp, short *ip, int which)
{
  const long *wp = xp;

  switch(which) {
  case 0:
    *ip = (short)(*wp >> 48);
    break;
  case 1:
    *ip = (short)((*wp >> 32) & 0xffff);
    break;
  case 2:
    *ip = (short)((*wp >> 16) & 0xffff);
    break;
  case 3:
    *ip = (short)(*wp & 0xffff);
    break;
  }

  if(*ip & 0x8000)
  {
    /* extern is negative */
    *ip |= (~(0xffff));
  }
}
#define get_ix_short(xp, ip) cget_short_short((xp), (ip), byteoff(xp)/X_SIZEOF_SHORT)

static int
cput_short_short(void *xp, const short *ip, int which)
{
  word *wp = xp;

  switch(which) {
  case 0:
    *wp = (*ip << 48)
          | (*wp & 0x0000ffffffffffff);
    break;
  case 1:
    *wp = ((*ip << 32) & 0x0000ffff00000000)
          | (*wp & 0xffff0000ffffffff);
    break;
  case 2:
    *wp = ((*ip << 16) & 0x00000000ffff0000)
          | (*wp & 0xffffffff0000ffff);
    break;
  case 3:
    *wp = (*ip         & 0x000000000000ffff)   
          | (*wp & 0xffffffffffff0000);
    break;
  }

  if(*ip > X_SHORT_MAX || *ip < X_SHORT_MIN)
    return NC_ERANGE;
  return ENOERR;
}

int
ncx_get_short_schar(const void *xp, schar *ip)
{
  ix_short xx;
  get_ix_short(xp, &xx);
  *ip = xx;
  if(xx > SCHAR_MAX || xx < SCHAR_MIN)
    return NC_ERANGE;
  return ENOERR;
}

int
ncx_get_short_uchar(const void *xp, uchar *ip)
{
  ix_short xx;
  get_ix_short(xp, &xx);
  *ip = xx;
  if(xx > UCHAR_MAX || xx < 0)
    return NC_ERANGE;
  return ENOERR;
}

int
ncx_get_short_short(const void *xp, short *ip)
{
  get_ix_short(xp, ip);
  return ENOERR;
}

int
ncx_get_short_int(const void *xp, int *ip)
{
  ix_short xx;
  get_ix_short(xp, &xx);
  *ip = xx;
  return ENOERR;
}

int
ncx_get_short_long(const void *xp, long *ip)
{
  ix_short xx;
  get_ix_short(xp, &xx);
  *ip = xx;
  return ENOERR;
}

int
ncx_get_short_float(const void *xp, float *ip)
{
  ix_short xx;
  get_ix_short(xp, &xx);
  *ip = xx;
  return ENOERR;
}

int
ncx_get_short_double(const void *xp, double *ip)
{
  ix_short xx;
  get_ix_short(xp, &xx);
  *ip = xx;
  return ENOERR;
}

int
ncx_put_short_schar(void *xp, const schar *ip)
{
  uchar *cp = xp;
  if(*ip & 0x80)
    *cp++ = 0xff;
  else
    *cp++ = 0;
  *cp = (uchar)*ip;
  return ENOERR;
}

int
ncx_put_short_uchar(void *xp, const uchar *ip)
{
  uchar *cp = xp;
  *cp++ = 0;
  *cp = *ip;
  return ENOERR;
}

int
ncx_put_short_short(void *xp, const short *ip)
{
  return cput_short_short(xp, ip, byteoff(xp)/X_SIZEOF_SHORT);
}

int
ncx_put_short_int(void *xp, const int *ip)
{
  ix_short xx = (ix_short)*ip;
  return cput_short_short(xp, &xx, byteoff(xp)/X_SIZEOF_SHORT);
}

int
ncx_put_short_long(void *xp, const long *ip)
{
  ix_short xx = (ix_short)*ip;
  return cput_short_short(xp, &xx, byteoff(xp)/X_SIZEOF_SHORT);
}

int
ncx_put_short_float(void *xp, const float *ip)
{
  ix_short xx = (ix_short)*ip;
  const int status = cput_short_short(xp, &xx, byteoff(xp)/X_SIZEOF_SHORT);
  if(status != ENOERR)
    return status;
  if(*ip > X_SHORT_MAX || *ip < X_SHORT_MIN)
    return NC_ERANGE;
  return ENOERR;
}

int
ncx_put_short_double(void *xp, const double *ip)
{
  ix_short xx = (ix_short)*ip;
  const int status = cput_short_short(xp, &xx, byteoff(xp)/X_SIZEOF_SHORT);
  if(status != ENOERR)
    return status;
  if(*ip > X_SHORT_MAX || *ip < X_SHORT_MIN)
    return NC_ERANGE;
  return ENOERR;
}

/* x_int */

typedef int ix_int;
#define SIZEOF_IX_INT SIZEOF_INT
#define IX_INT_MAX INT_MAX

static void
cget_int_int(const void *xp, int *ip, int which)
{
  const long *wp = xp;

  if(which == 0)
  {
    *ip = (int)(*wp >> 32);
  }
  else
  {
    *ip = (int)(*wp & 0xffffffff);
  }

  if(*ip & 0x80000000)
  {
    /* extern is negative */
    *ip |= (~(0xffffffff));
  }
}
#define get_ix_int(xp, ip) cget_int_int((xp), (ip), byteoff(xp))

static int
cput_int_int(void *xp, const int *ip, int which)
{
  word *wp = xp;

  if(which == 0)
  {
    *wp = (*ip << 32) | (*wp & 0xffffffff);
  }
  else
  {
    *wp = (*wp & ~0xffffffff) | (*ip & 0xffffffff);
  }
  if(*ip > X_INT_MAX || *ip < X_INT_MIN)
    return NC_ERANGE;
  return ENOERR;
}
#define put_ix_int(xp, ip) cput_int_int((xp), (ip), byteoff(xp))

int
ncx_get_int_schar(const void *xp, schar *ip)
{
  ix_int xx;
  get_ix_int(xp, &xx);
  *ip = xx;
  if(xx > SCHAR_MAX || xx < SCHAR_MIN)
    return NC_ERANGE;
  return ENOERR;
}

int
ncx_get_int_uchar(const void *xp, uchar *ip)
{
  ix_int xx;
  get_ix_int(xp, &xx);
  *ip = xx;
  if(xx > UCHAR_MAX || xx < 0)
    return NC_ERANGE;
  return ENOERR;
}

int
ncx_get_int_short(const void *xp, short *ip)
{
  ix_int xx;
  get_ix_int(xp, &xx);
  *ip = xx;
  if(xx > SHORT_MAX || xx < SHORT_MIN)
    return NC_ERANGE;
  return ENOERR;
}

int
ncx_get_int_int(const void *xp, int *ip)
{
  ix_int xx;
  get_ix_int(xp, &xx);
  *ip = xx;
  return ENOERR;
}

static void
cget_int_long(const void *xp, long *ip, int which)
{
  const long *wp = xp;

  if(which == 0)
  {
    *ip = (int)(*wp >> 32);
  }
  else
  {
    *ip = (int)(*wp & 0xffffffff);
  }

  if(*ip & 0x80000000)
  {
    /* extern is negative */
    *ip |= (~(0xffffffff));
  }
}

int
ncx_get_int_long(const void *xp, long *ip)
{
  cget_int_long(xp, ip, byteoff(xp));
  return ENOERR;
}

int
ncx_get_int_float(const void *xp, float *ip)
{
  ix_int xx;
  get_ix_int(xp, &xx);
  *ip = xx;
  if(xx > FLT_MAX || xx < (-FLT_MAX))
    return NC_ERANGE;
  return ENOERR;
}

int
ncx_get_int_double(const void *xp, double *ip)
{
  ix_int xx;
  get_ix_int(xp, &xx);
  *ip = xx;
  return ENOERR;
}

int
ncx_put_int_schar(void *xp, const schar *ip)
{
  uchar *cp = xp;
  if(*ip & 0x80)
  {
    *cp++ = 0xff;
    *cp++ = 0xff;
    *cp++ = 0xff;
  }
  else
  {
    *cp++ = 0x00;
    *cp++ = 0x00;
    *cp++ = 0x00;
  }
  *cp = (uchar)*ip;
  return ENOERR;
}

int
ncx_put_int_uchar(void *xp, const uchar *ip)
{
  uchar *cp = xp;
  *cp++ = 0x00;
  *cp++ = 0x00;
  *cp++ = 0x00;
  *cp   = *ip;
  return ENOERR;
}

int
ncx_put_int_short(void *xp, const short *ip)
{
  ix_int xx = (ix_int)(*ip);
  return put_ix_int(xp, &xx);
}

int
ncx_put_int_int(void *xp, const int *ip)
{
  return put_ix_int(xp, ip);
}

static int
cput_int_long(void *xp, const long *ip, int which)
{
  long *wp = xp;

  if(which == 0)
  {
    *wp = (*ip << 32) | (*wp & 0xffffffff);
  }
  else
  {
    *wp = (*wp & ~0xffffffff) | (*ip & 0xffffffff);
  }
  if(*ip > X_INT_MAX || *ip < X_INT_MIN)
    return NC_ERANGE;
  return ENOERR;
}

int
ncx_put_int_long(void *xp, const long *ip)
{
  return cput_int_long(xp, ip, byteoff(xp));
}

int
ncx_put_int_float(void *xp, const float *ip)
{
  ix_int xx = (ix_int)(*ip);
  const int status = put_ix_int(xp, &xx);
  if(status != ENOERR)
    return status;
  if(*ip > (double)X_INT_MAX || *ip < (double)X_INT_MIN)
    return NC_ERANGE;
  return ENOERR;
}

int
ncx_put_int_double(void *xp, const double *ip)
{
  ix_int xx = (ix_int)(*ip);
  const int status = put_ix_int(xp, &xx);
  if(status != ENOERR)
    return status;
  if(*ip > X_INT_MAX || *ip < X_INT_MIN)
    return NC_ERANGE;
  return ENOERR;
}
 

/* x_float */

#if defined(NO_IEEE_FLOAT)

struct cray_single {
  unsigned int  sign  : 1;
  unsigned int   exp  :15;
  unsigned int  mant  :48;
};
typedef struct cray_single cray_single;

static const int cs_ieis_bias = 0x4000 - 0x7f;

static const int cs_id_bias = 0x4000 - 0x3ff;

#endif

struct ieee_single_hi {
  unsigned int  sign  : 1;
  unsigned int   exp  : 8;
  unsigned int  mant  :23;
  unsigned int  pad  :32;
};
typedef struct ieee_single_hi ieee_single_hi;

struct ieee_single_lo {
  unsigned int  pad  :32;
  unsigned int  sign  : 1;
  unsigned int   exp  : 8;
  unsigned int  mant  :23;
};
typedef struct ieee_single_lo ieee_single_lo;

static const int ieee_single_bias = 0x7f;


struct ieee_double {
  unsigned int  sign  : 1;
  unsigned int   exp  :11;
  unsigned int  mant  :52;
};
typedef struct ieee_double ieee_double;

static const int ieee_double_bias = 0x3ff;

#if FLOAT_USE_IEG

static void
get_ix_float(const void *xp, float *ip)
{
  const int bo = bitoff(xp);
  (void) IEG2CRAY(&Cray2_F32, &One, (word *)xp, &bo, ip, &UnitStride);
}

static int
put_ix_float(void *xp, const float *ip)
{
  const int bo = bitoff(xp);
  int status = CRAY2IEG(&Cray2_F32, &One, (word *)xp, &bo, ip, &UnitStride);
  if(status != 0)
    status = NC_ERANGE;
  /* else, status == 0 == ENOERR */
  return status;
}

#else
  /* !FLOAT_USE_IEG */

#if defined(NO_IEEE_FLOAT)

static void
cget_float_float(const void *xp, float *ip, int which)
{

  if(which == 0)
  {
    const ieee_single_hi *isp = (const ieee_single_hi *) xp;
    cray_single *csp = (cray_single *) ip;

    if(isp->exp == 0)
    {
      /* ieee subnormal */
      *ip = (double)isp->mant;
      if(isp->mant != 0)
      {
        csp->exp -= (ieee_single_bias + 22);
      }
    }
    else
    {
      csp->exp  = isp->exp + cs_ieis_bias + 1;
      csp->mant = isp->mant << (48 - 1 - 23);
      csp->mant |= (1 << (48 - 1));
    }
    csp->sign = isp->sign;


  }
  else
  {
    const ieee_single_lo *isp = (const ieee_single_lo *) xp;
    cray_single *csp = (cray_single *) ip;

    if(isp->exp == 0)
    {
      /* ieee subnormal */
      *ip = (double)isp->mant;
      if(isp->mant != 0)
      {
        csp->exp -= (ieee_single_bias + 22);
      }
    }
    else
    {
      csp->exp  = isp->exp + cs_ieis_bias + 1;
      csp->mant = isp->mant << (48 - 1 - 23);
      csp->mant |= (1 << (48 - 1));
    }
    csp->sign = isp->sign;


  }
}

static int
cput_float_float(void *xp, const float *ip, int which)
{
  int status = ENOERR;
  if(which == 0)
  {
    ieee_single_hi *isp = (ieee_single_hi*)xp;
  const cray_single *csp = (const cray_single *) ip;
  int ieee_exp = csp->exp - cs_ieis_bias -1;

  isp->sign = csp->sign;

  if(ieee_exp >= 0xff
      || *ip > X_FLOAT_MAX || *ip < X_FLOAT_MIN)
  {
    /* NC_ERANGE => ieee Inf */
    isp->exp = 0xff;
    isp->mant = 0x0;
    return NC_ERANGE;
  }
  /* else */

  if(ieee_exp > 0)
  {
    /* normal ieee representation */
    isp->exp  = ieee_exp;
    /* assumes cray rep is in normal form */
    /* assert(csp->mant & 0x800000000000); */
    isp->mant = (((csp->mant << 1) &
        0xffffffffffff) >> (48 - 23));
  }
  else if(ieee_exp > -23)
  {
    /* ieee subnormal, right  */
    const int rshift = (48 - 23 - ieee_exp);

    isp->mant = csp->mant >> rshift;
    isp->exp  = 0;
  }
  else
  {
    /* smaller than ieee can represent */
    isp->exp = 0;
    isp->mant = 0;
  }

  }
  else
  {
    ieee_single_lo *isp = (ieee_single_lo*)xp;
  const cray_single *csp = (const cray_single *) ip;
  int ieee_exp = csp->exp - cs_ieis_bias -1;

  isp->sign = csp->sign;

  if(ieee_exp >= 0xff
      || *ip > X_FLOAT_MAX || *ip < X_FLOAT_MIN)
  {
    /* NC_ERANGE => ieee Inf */
    isp->exp = 0xff;
    isp->mant = 0x0;
    return NC_ERANGE;
  }
  /* else */

  if(ieee_exp > 0)
  {
    /* normal ieee representation */
    isp->exp  = ieee_exp;
    /* assumes cray rep is in normal form */
    /* assert(csp->mant & 0x800000000000); */
    isp->mant = (((csp->mant << 1) &
        0xffffffffffff) >> (48 - 23));
  }
  else if(ieee_exp > -23)
  {
    /* ieee subnormal, right  */
    const int rshift = (48 - 23 - ieee_exp);

    isp->mant = csp->mant >> rshift;
    isp->exp  = 0;
  }
  else
  {
    /* smaller than ieee can represent */
    isp->exp = 0;
    isp->mant = 0;
  }

  }
  return ENOERR;
}

#define get_ix_float(xp, ip) cget_float_float((xp), (ip), byteoff(xp))
#define put_ix_float(xp, ip) cput_float_float((xp), (ip), byteoff(xp))

#else
  /* IEEE Cray with only doubles */
static void
cget_float_float(const void *xp, float *ip, int which)
{

  ieee_double *idp = (ieee_double *) ip;

  if(which == 0)
  {
    const ieee_single_hi *isp = (const ieee_single_hi *) xp;
    if(isp->exp == 0 && isp->mant == 0)
    {
      idp->exp = 0;
      idp->mant = 0;
    }
    else
    {
      idp->exp = isp->exp + (ieee_double_bias - ieee_single_bias);
      idp->mant = isp->mant << (52 - 23);
    }
    idp->sign = isp->sign;
  }
  else
  {
    const ieee_single_lo *isp = (const ieee_single_lo *) xp;
    if(isp->exp == 0 && isp->mant == 0)
    {
      idp->exp = 0;
      idp->mant = 0;
    }
    else
    {
      idp->exp = isp->exp + (ieee_double_bias - ieee_single_bias);
      idp->mant = isp->mant << (52 - 23);
    }
    idp->sign = isp->sign;
  }
}

static int
cput_float_float(void *xp, const float *ip, int which)
{
  const ieee_double *idp = (const ieee_double *) ip;
  if(which == 0)
  {
    ieee_single_hi *isp = (ieee_single_hi*)xp;
    if(idp->exp > (ieee_double_bias - ieee_single_bias))
      isp->exp = idp->exp - (ieee_double_bias - ieee_single_bias);
    else
      isp->exp = 0;
    isp->mant = idp->mant >> (52 - 23);
    isp->sign = idp->sign;
  }
  else
  {
    ieee_single_lo *isp = (ieee_single_lo*)xp;
    if(idp->exp > (ieee_double_bias - ieee_single_bias))
      isp->exp = idp->exp - (ieee_double_bias - ieee_single_bias);
    else
      isp->exp = 0;
    isp->mant = idp->mant >> (52 - 23);
    isp->sign = idp->sign;
  }
  if(*ip > X_FLOAT_MAX || *ip < X_FLOAT_MIN)
    return NC_ERANGE;
  return ENOERR;
}

#define get_ix_float(xp, ip) cget_float_float((xp), (ip), byteoff(xp))
#define put_ix_float(xp, ip) cput_float_float((xp), (ip), byteoff(xp))

#endif /* NO_IEEE_FLOAT */

#endif /* FLOAT_USE_IEG */


int
ncx_get_float_schar(const void *xp, schar *ip)
{
  float xx;
  get_ix_float(xp, &xx);
  *ip = (schar) xx;
  if(xx > SCHAR_MAX || xx < SCHAR_MIN)
    return NC_ERANGE;
  return ENOERR;
}

int
ncx_get_float_uchar(const void *xp, uchar *ip)
{
  float xx;
  get_ix_float(xp, &xx);
  *ip = (uchar) xx;
  if(xx > UCHAR_MAX || xx < 0)
    return NC_ERANGE;
  return ENOERR;
}

int
ncx_get_float_short(const void *xp, short *ip)
{
  float xx;
  get_ix_float(xp, &xx);
  *ip = (short) xx;
  if(xx > SHORT_MAX || xx < SHORT_MIN)
    return NC_ERANGE;
  return ENOERR;
}

int
ncx_get_float_int(const void *xp, int *ip)
{
  float xx;
  get_ix_float(xp, &xx);
  *ip = (int) xx;
  if(xx > (double)INT_MAX || xx < (double)INT_MIN)
    return NC_ERANGE;
  return ENOERR;
}

int
ncx_get_float_long(const void *xp, long *ip)
{
  float xx;
  get_ix_float(xp, &xx);
  *ip = (long) xx;
  if(xx > LONG_MAX || xx < LONG_MIN)
    return NC_ERANGE;
  return ENOERR;
}

int
ncx_get_float_float(const void *xp, float *ip)
{
  get_ix_float(xp, ip);
  return ENOERR;
}

int
ncx_get_float_double(const void *xp, double *ip)
{
#if SIZEOF_FLOAT == SIZEOF_DOUBLE && FLT_MANT_DIG == DBL_MANT_DIG
  return ncx_get_float_float(xp, (float *)ip);
#else
  float xx;
  get_ix_float(xp, &xx);
  *ip = xx;
  return ENOERR;
#endif
}


int
ncx_put_float_schar(void *xp, const schar *ip)
{
  float xx = (float) *ip;
  return put_ix_float(xp, &xx);
}

int
ncx_put_float_uchar(void *xp, const uchar *ip)
{
  float xx = (float) *ip;
  return put_ix_float(xp, &xx);
}

int
ncx_put_float_short(void *xp, const short *ip)
{
  float xx = (float) *ip;
  return put_ix_float(xp, &xx);
}

int
ncx_put_float_int(void *xp, const int *ip)
{
  float xx = (float) *ip;
  return put_ix_float(xp, &xx);
}

int
ncx_put_float_long(void *xp, const long *ip)
{
  float xx = (float) *ip;
  return put_ix_float(xp, &xx);
}

int
ncx_put_float_float(void *xp, const float *ip)
{
  return put_ix_float(xp, ip);
}

int
ncx_put_float_double(void *xp, const double *ip)
{
#if SIZEOF_FLOAT == SIZEOF_DOUBLE && FLT_MANT_DIG == DBL_MANT_DIG
  return put_ix_float(xp, (float *)ip);
#else
  float xx = (float) *ip;
  int status = put_ix_float(xp, &xx);
  if(*ip > X_FLOAT_MAX || *ip < X_FLOAT_MIN)
    return NC_ERANGE;
  return status;
#endif
}

/* x_double */


#if X_SIZEOF_DOUBLE == SIZEOF_DOUBLE  && !defined(NO_IEEE_FLOAT)

static void
get_ix_double(const void *xp, double *ip)
{
  (void) memcpy(ip, xp, sizeof(double));
}

static int
put_ix_double(void *xp, const double *ip)
{
  (void) memcpy(xp, ip, X_SIZEOF_DOUBLE);
  return ENOERR;
}

#else

static void
cget_double_double(const void *xp, double *ip)
{
  const ieee_double *idp = (const ieee_double *) xp;
  cray_single *csp = (cray_single *) ip;

  if(idp->exp == 0)
  {
    /* ieee subnormal */
    *ip = (double)idp->mant;
    if(idp->mant != 0)
    {
      csp->exp -= (ieee_double_bias + 51);
    }
  }
  else
  {
    csp->exp  = idp->exp + cs_id_bias + 1;
    csp->mant = idp->mant >> (52 - 48 + 1);
    csp->mant |= (1 << (48 - 1));
  }
  csp->sign = idp->sign;
}

static int
cput_double_double(void *xp, const double *ip)
{
  ieee_double *idp = (ieee_double *) xp;
  const cray_single *csp = (const cray_single *) ip;

  int ieee_exp = csp->exp - cs_id_bias -1;

  idp->sign = csp->sign;

  if(ieee_exp >= 0x7ff)
  {
    /* NC_ERANGE => ieee Inf */
    idp->exp = 0x7ff;
    idp->mant = 0x0;
    return NC_ERANGE;
  }
  /* else */

  if(ieee_exp > 0)
  {
    /* normal ieee representation */
    idp->exp  = ieee_exp;
    /* assumes cray rep is in normal form */
    assert(csp->mant & 0x800000000000);
    idp->mant = (((csp->mant << 1) &
        0xffffffffffff) << (52 - 48));
  }
  else if(ieee_exp >= (-(52 -48)))
  {
    /* ieee subnormal, left  */
    const int lshift = (52 - 48) + ieee_exp;
    idp->mant = csp->mant << lshift;
    idp->exp  = 0;
  }
  else if(ieee_exp >= -52)
  {
    /* ieee subnormal, right  */
    const int rshift = (- (52 - 48) - ieee_exp);

    idp->mant = csp->mant >> rshift;
    idp->exp  = 0;
  }
  else
  {
    /* smaller than ieee can represent */
    idp->exp = 0;
    idp->mant = 0;
  }
  return ENOERR;
}

#define get_ix_double(xp, ip) cget_double_double((xp), (ip))
#define put_ix_double(xp, ip) cput_double_double((xp), (ip))

#endif /* NO_IEEE_FLOAT */

int
ncx_get_double_schar(const void *xp, schar *ip)
{
  double xx;
  get_ix_double(xp, &xx);
  *ip = (schar) xx;
  if(xx > SCHAR_MAX || xx < SCHAR_MIN)
    return NC_ERANGE;
  return ENOERR;
}

int
ncx_get_double_uchar(const void *xp, uchar *ip)
{
  double xx;
  get_ix_double(xp, &xx);
  *ip = (uchar) xx;
  if(xx > UCHAR_MAX || xx < 0)
    return NC_ERANGE;
  return ENOERR;
}

int
ncx_get_double_short(const void *xp, short *ip)
{
  double xx;
  get_ix_double(xp, &xx);
  *ip = (short) xx;
  if(xx > SHORT_MAX || xx < SHORT_MIN)
    return NC_ERANGE;
  return ENOERR;
}

int
ncx_get_double_int(const void *xp, int *ip)
{
  double xx;
  get_ix_double(xp, &xx);
  *ip = (int) xx;
  if(xx > INT_MAX || xx < INT_MIN)
    return NC_ERANGE;
  return ENOERR;
}

int
ncx_get_double_long(const void *xp, long *ip)
{
  double xx;
  get_ix_double(xp, &xx);
  *ip = (long) xx;
  if(xx > LONG_MAX || xx < LONG_MIN)
    return NC_ERANGE;
  return ENOERR;
}

int
ncx_get_double_float(const void *xp, float *ip)
{
#if SIZEOF_FLOAT == SIZEOF_DOUBLE && FLT_MANT_DIG == DBL_MANT_DIG
  get_ix_double(xp, (double *)ip);
  return ENOERR;
#else
  double xx;
  get_ix_double(xp, &xx);
  if(xx > FLT_MAX || xx < (-FLT_MAX))
  {
    *ip = FLT_MAX;
    return NC_ERANGE;
  }
  if(xx < (-FLT_MAX))
  {
    *ip = (-FLT_MAX);
    return NC_ERANGE;
  }
  *ip = (float) xx;
  return ENOERR;
#endif
}

int
ncx_get_double_double(const void *xp, double *ip)
{
  get_ix_double(xp, ip);
  return ENOERR;
}


int
ncx_put_double_schar(void *xp, const schar *ip)
{
  double xx = (double) *ip;
  put_ix_double(xp, &xx);
  return ENOERR;
}

int
ncx_put_double_uchar(void *xp, const uchar *ip)
{
  double xx = (double) *ip;
  put_ix_double(xp, &xx);
  return ENOERR;
}

int
ncx_put_double_short(void *xp, const short *ip)
{
  double xx = (double) *ip;
  put_ix_double(xp, &xx);
  return ENOERR;
}

int
ncx_put_double_int(void *xp, const int *ip)
{
  double xx = (double) *ip;
  put_ix_double(xp, &xx);
  return ENOERR;
}

int
ncx_put_double_long(void *xp, const long *ip)
{
  double xx = (double) *ip;
  put_ix_double(xp, &xx);
  /* TODO: Deal with big guys */
  return ENOERR;
}

int
ncx_put_double_float(void *xp, const float *ip)
{
#if SIZEOF_FLOAT == SIZEOF_DOUBLE && FLT_MANT_DIG == DBL_MANT_DIG
  put_ix_double(xp, (double *)ip);
  return ENOERR;
#else
  double xx = (double) *ip;
  return put_ix_double(xp, &xx);
#endif
}

int
ncx_put_double_double(void *xp, const double *ip)
{
#if !defined(NO_IEEE_FLOAT)
  put_ix_double(xp, ip);
  return ENOERR;
#else
  return put_ix_double(xp, ip);
#endif
}


/* x_size_t */

int
ncx_put_size_t(void **xpp, const size_t *ulp)
{
  /* similar to put_ix_int() */
  uchar *cp = *xpp;
    /* sizes limited to 2^31 -1 in netcdf */
  assert(*ulp <= X_SIZE_MAX && (long) (*ulp) >= 0);

  *cp++ = (uchar)((*ulp) >> 24);
  *cp++ = (uchar)(((*ulp) & 0x00ff0000) >> 16);
  *cp++ = (uchar)(((*ulp) & 0x0000ff00) >>  8);
  *cp   = (uchar)((*ulp) & 0x000000ff);

  *xpp = (void *)((char *)(*xpp) + X_SIZEOF_SIZE_T);
  return ENOERR;
}

int
ncx_get_size_t(const void **xpp,  size_t *ulp)
{
  /* similar to get_ix_int */
  const uchar *cp = *xpp;
  assert((*cp & 0x80) == 0); /* sizes limited to 2^31 -1 in netcdf */

  *ulp = *cp++ << 24;
  *ulp |= (*cp++ << 16);
  *ulp |= (*cp++ << 8);
  *ulp |= *cp; 

  *xpp = (const void *)((const char *)(*xpp) + X_SIZEOF_SIZE_T);
  return ENOERR;
}

/* x_off_t */

int
ncx_put_off_t(void **xpp, const off_t *lp)
{
  /* similar to put_ix_int() */
  uchar *cp = *xpp;
    /* No negative offsets stored in netcdf */
  assert(*lp >= 0 && *lp <= X_OFF_MAX);

  *cp++ = (uchar)((*lp) >> 24);
  *cp++ = (uchar)(((*lp) & 0x00ff0000) >> 16);
  *cp++ = (uchar)(((*lp) & 0x0000ff00) >>  8);
  *cp   = (uchar)((*lp) & 0x000000ff);

  *xpp = (void *)((char *)(*xpp) + X_SIZEOF_OFF_T);
  return ENOERR;
}

int
ncx_get_off_t(const void **xpp, off_t *lp)
{
  /* similar to get_ix_int() */
  const uchar *cp = *xpp;
  assert((*cp & 0x80) == 0); /* No negative offsets stored in netcdf */

  *lp = *cp++ << 24;
  *lp |= (*cp++ << 16);
  *lp |= (*cp++ << 8);
  *lp |= *cp; 

  *xpp = (const void *)((const char *)(*xpp) + X_SIZEOF_OFF_T);
  return ENOERR;
}


/*
 * Aggregate numeric conversion functions.
 */



/* schar */

int
ncx_getn_schar_schar(const void **xpp, size_t nelems, schar *tp)
{
    (void) memcpy(tp, *xpp, nelems);
  *xpp = (void *)((char *)(*xpp) + nelems);
  return ENOERR;

}
int
ncx_getn_schar_uchar(const void **xpp, size_t nelems, uchar *tp)
{
    (void) memcpy(tp, *xpp, nelems);
  *xpp = (void *)((char *)(*xpp) + nelems);
  return ENOERR;

}
int
ncx_getn_schar_short(const void **xpp, size_t nelems, short *tp)
{
  schar *xp = (schar *)(*xpp);

  while(nelems-- != 0)
  {
    *tp++ = *xp++;
  }

  *xpp = (const void *)xp;
  return ENOERR;
}

int
ncx_getn_schar_int(const void **xpp, size_t nelems, int *tp)
{
  schar *xp = (schar *)(*xpp);

  while(nelems-- != 0)
  {
    *tp++ = *xp++;
  }

  *xpp = (const void *)xp;
  return ENOERR;
}

int
ncx_getn_schar_long(const void **xpp, size_t nelems, long *tp)
{
  schar *xp = (schar *)(*xpp);

  while(nelems-- != 0)
  {
    *tp++ = *xp++;
  }

  *xpp = (const void *)xp;
  return ENOERR;
}

int
ncx_getn_schar_float(const void **xpp, size_t nelems, float *tp)
{
  schar *xp = (schar *)(*xpp);

  while(nelems-- != 0)
  {
    *tp++ = *xp++;
  }

  *xpp = (const void *)xp;
  return ENOERR;
}

int
ncx_getn_schar_double(const void **xpp, size_t nelems, double *tp)
{
  schar *xp = (schar *)(*xpp);

  while(nelems-- != 0)
  {
    *tp++ = *xp++;
  }

  *xpp = (const void *)xp;
  return ENOERR;
}


int
ncx_pad_getn_schar_schar(const void **xpp, size_t nelems, schar *tp)
{
    size_t rndup = nelems % X_ALIGN;

  if(rndup)
    rndup = X_ALIGN - rndup;

  (void) memcpy(tp, *xpp, nelems);
  *xpp = (void *)((char *)(*xpp) + nelems + rndup);

  return ENOERR;

}
int
ncx_pad_getn_schar_uchar(const void **xpp, size_t nelems, uchar *tp)
{
    size_t rndup = nelems % X_ALIGN;

  if(rndup)
    rndup = X_ALIGN - rndup;

  (void) memcpy(tp, *xpp, nelems);
  *xpp = (void *)((char *)(*xpp) + nelems + rndup);

  return ENOERR;

}
int
ncx_pad_getn_schar_short(const void **xpp, size_t nelems, short *tp)
{
  size_t rndup = nelems % X_ALIGN;
  schar *xp = (schar *)(*xpp);

  if(rndup)
    rndup = X_ALIGN - rndup;

  while(nelems-- != 0)
  {
    *tp++ = *xp++;
  }

  *xpp = (void *)(xp + rndup);
  return ENOERR;
}

int
ncx_pad_getn_schar_int(const void **xpp, size_t nelems, int *tp)
{
  size_t rndup = nelems % X_ALIGN;
  schar *xp = (schar *)(*xpp);

  if(rndup)
    rndup = X_ALIGN - rndup;

  while(nelems-- != 0)
  {
    *tp++ = *xp++;
  }

  *xpp = (void *)(xp + rndup);
  return ENOERR;
}

int
ncx_pad_getn_schar_long(const void **xpp, size_t nelems, long *tp)
{
  size_t rndup = nelems % X_ALIGN;
  schar *xp = (schar *)(*xpp);

  if(rndup)
    rndup = X_ALIGN - rndup;

  while(nelems-- != 0)
  {
    *tp++ = *xp++;
  }

  *xpp = (void *)(xp + rndup);
  return ENOERR;
}

int
ncx_pad_getn_schar_float(const void **xpp, size_t nelems, float *tp)
{
  size_t rndup = nelems % X_ALIGN;
  schar *xp = (schar *)(*xpp);

  if(rndup)
    rndup = X_ALIGN - rndup;

  while(nelems-- != 0)
  {
    *tp++ = *xp++;
  }

  *xpp = (void *)(xp + rndup);
  return ENOERR;
}

int
ncx_pad_getn_schar_double(const void **xpp, size_t nelems, double *tp)
{
  size_t rndup = nelems % X_ALIGN;
  schar *xp = (schar *)(*xpp);

  if(rndup)
    rndup = X_ALIGN - rndup;

  while(nelems-- != 0)
  {
    *tp++ = *xp++;
  }

  *xpp = (void *)(xp + rndup);
  return ENOERR;
}


int
ncx_putn_schar_schar(void **xpp, size_t nelems, const schar *tp)
{
    (void) memcpy(*xpp, tp, nelems);
  *xpp = (void *)((char *)(*xpp) + nelems);

  return ENOERR;

}
int
ncx_putn_schar_uchar(void **xpp, size_t nelems, const uchar *tp)
{
    (void) memcpy(*xpp, tp, nelems);
  *xpp = (void *)((char *)(*xpp) + nelems);

  return ENOERR;

}
int
ncx_putn_schar_short(void **xpp, size_t nelems, const short *tp)
{
  int status = ENOERR;
  schar *xp = (schar *)(*xpp);

  while(nelems-- != 0)
  {
    if(*tp > X_SCHAR_MAX || *tp < X_SCHAR_MIN)
      status = NC_ERANGE;
    *xp++ = (schar) *tp++;
  }

  *xpp = (void *)xp;
  return status;
}

int
ncx_putn_schar_int(void **xpp, size_t nelems, const int *tp)
{
  int status = ENOERR;
  schar *xp = (schar *)(*xpp);

  while(nelems-- != 0)
  {
    if(*tp > X_SCHAR_MAX || *tp < X_SCHAR_MIN)
      status = NC_ERANGE;
    *xp++ = (schar) *tp++;
  }

  *xpp = (void *)xp;
  return status;
}

int
ncx_putn_schar_long(void **xpp, size_t nelems, const long *tp)
{
  int status = ENOERR;
  schar *xp = (schar *)(*xpp);

  while(nelems-- != 0)
  {
    if(*tp > X_SCHAR_MAX || *tp < X_SCHAR_MIN)
      status = NC_ERANGE;
    *xp++ = (schar) *tp++;
  }

  *xpp = (void *)xp;
  return status;
}

int
ncx_putn_schar_float(void **xpp, size_t nelems, const float *tp)
{
  int status = ENOERR;
  schar *xp = (schar *)(*xpp);

  while(nelems-- != 0)
  {
    if(*tp > X_SCHAR_MAX || *tp < X_SCHAR_MIN)
      status = NC_ERANGE;
    *xp++ = (schar) *tp++;
  }

  *xpp = (void *)xp;
  return status;
}

int
ncx_putn_schar_double(void **xpp, size_t nelems, const double *tp)
{
  int status = ENOERR;
  schar *xp = (schar *)(*xpp);

  while(nelems-- != 0)
  {
    if(*tp > X_SCHAR_MAX || *tp < X_SCHAR_MIN)
      status = NC_ERANGE;
    *xp++ = (schar) *tp++;
  }

  *xpp = (void *)xp;
  return status;
}


int
ncx_pad_putn_schar_schar(void **xpp, size_t nelems, const schar *tp)
{
    size_t rndup = nelems % X_ALIGN;

  if(rndup)
    rndup = X_ALIGN - rndup;

  (void) memcpy(*xpp, tp, nelems);
  *xpp = (void *)((char *)(*xpp) + nelems);

  if(rndup)
  {
    (void) memcpy(*xpp, nada, rndup);
    *xpp = (void *)((char *)(*xpp) + rndup);
  }
  
  return ENOERR;

}
int
ncx_pad_putn_schar_uchar(void **xpp, size_t nelems, const uchar *tp)
{
    size_t rndup = nelems % X_ALIGN;

  if(rndup)
    rndup = X_ALIGN - rndup;

  (void) memcpy(*xpp, tp, nelems);
  *xpp = (void *)((char *)(*xpp) + nelems);

  if(rndup)
  {
    (void) memcpy(*xpp, nada, rndup);
    *xpp = (void *)((char *)(*xpp) + rndup);
  }
  
  return ENOERR;

}
int
ncx_pad_putn_schar_short(void **xpp, size_t nelems, const short *tp)
{
  int status = ENOERR;
  size_t rndup = nelems % X_ALIGN;
  schar *xp = (schar *)(*xpp);

  if(rndup)
    rndup = X_ALIGN - rndup;

  while(nelems-- != 0)
  {
    /* N.B. schar as signed */
    if(*tp > X_SCHAR_MAX || *tp < X_SCHAR_MIN)
      status = NC_ERANGE;
    *xp++ = (schar) *tp++;
  }


  if(rndup)
  {
    (void) memcpy(xp, nada, rndup);
    xp += rndup;
  }

  *xpp = (void *)xp;
  return status;
}

int
ncx_pad_putn_schar_int(void **xpp, size_t nelems, const int *tp)
{
  int status = ENOERR;
  size_t rndup = nelems % X_ALIGN;
  schar *xp = (schar *)(*xpp);

  if(rndup)
    rndup = X_ALIGN - rndup;

  while(nelems-- != 0)
  {
    /* N.B. schar as signed */
    if(*tp > X_SCHAR_MAX || *tp < X_SCHAR_MIN)
      status = NC_ERANGE;
    *xp++ = (schar) *tp++;
  }


  if(rndup)
  {
    (void) memcpy(xp, nada, rndup);
    xp += rndup;
  }

  *xpp = (void *)xp;
  return status;
}

int
ncx_pad_putn_schar_long(void **xpp, size_t nelems, const long *tp)
{
  int status = ENOERR;
  size_t rndup = nelems % X_ALIGN;
  schar *xp = (schar *)(*xpp);

  if(rndup)
    rndup = X_ALIGN - rndup;

  while(nelems-- != 0)
  {
    /* N.B. schar as signed */
    if(*tp > X_SCHAR_MAX || *tp < X_SCHAR_MIN)
      status = NC_ERANGE;
    *xp++ = (schar) *tp++;
  }


  if(rndup)
  {
    (void) memcpy(xp, nada, rndup);
    xp += rndup;
  }

  *xpp = (void *)xp;
  return status;
}

int
ncx_pad_putn_schar_float(void **xpp, size_t nelems, const float *tp)
{
  int status = ENOERR;
  size_t rndup = nelems % X_ALIGN;
  schar *xp = (schar *)(*xpp);

  if(rndup)
    rndup = X_ALIGN - rndup;

  while(nelems-- != 0)
  {
    /* N.B. schar as signed */
    if(*tp > X_SCHAR_MAX || *tp < X_SCHAR_MIN)
      status = NC_ERANGE;
    *xp++ = (schar) *tp++;
  }


  if(rndup)
  {
    (void) memcpy(xp, nada, rndup);
    xp += rndup;
  }

  *xpp = (void *)xp;
  return status;
}

int
ncx_pad_putn_schar_double(void **xpp, size_t nelems, const double *tp)
{
  int status = ENOERR;
  size_t rndup = nelems % X_ALIGN;
  schar *xp = (schar *)(*xpp);

  if(rndup)
    rndup = X_ALIGN - rndup;

  while(nelems-- != 0)
  {
    /* N.B. schar as signed */
    if(*tp > X_SCHAR_MAX || *tp < X_SCHAR_MIN)
      status = NC_ERANGE;
    *xp++ = (schar) *tp++;
  }


  if(rndup)
  {
    (void) memcpy(xp, nada, rndup);
    xp += rndup;
  }

  *xpp = (void *)xp;
  return status;
}



/* short */

int
ncx_getn_short_schar(const void **xpp, size_t nelems, schar *tp)
{
  const char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_SHORT, tp++)
  {
    const int lstatus = ncx_get_short_schar(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  *xpp = (const void *)xp;
  return status;
}

int
ncx_getn_short_uchar(const void **xpp, size_t nelems, uchar *tp)
{
  const char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_SHORT, tp++)
  {
    const int lstatus = ncx_get_short_uchar(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  *xpp = (const void *)xp;
  return status;
}

#if SHORT_USE_IEG
int
ncx_getn_short_short(const void **xpp, size_t nelems, short *tp)
{
  if(nelems > 0)
  {
    const int bo = bitoff(*xpp);
    const word *wp = *xpp;
    int ierr;
    *xpp = ((const char *) (*xpp) + nelems * X_SIZEOF_SHORT);
    ierr = IEG2CRAY(&Cray2_I16, &nelems, wp,
        &bo, tp, &UnitStride);
    assert(ierr >= 0);
    if(ierr > 0)
      return NC_ERANGE;
  }
  return ENOERR;
}
#else
int
ncx_getn_short_short(const void **xpp, const size_t nelems, short *tp)
{
  if(nelems > 0)
  {
  const word *wp = *xpp;
  const short *const last = &tp[nelems -1];
  const int rem = word_align(*xpp)/X_SIZEOF_SHORT;
  *xpp = ((const char *) (*xpp) + nelems * X_SIZEOF_SHORT);

  switch(rem) {
  case 3:
    *tp = (short)((*wp >> 32) & 0xffff);
    if(*tp & 0x8000)
      *tp |= (~(0xffff));
    if(tp == last)
      return ENOERR;
    tp++;
    /*FALLTHRU*/  
  case 2:
    *tp = (short)((*wp >> 16) & 0xffff);
    if(*tp & 0x8000)
      *tp |= (~(0xffff));
    if(tp == last)
      return ENOERR;
    tp++;
    /*FALLTHRU*/  
  case 1:
    *tp = (short)(*wp & 0xffff);
    if(*tp & 0x8000)
      *tp |= (~(0xffff));
    if(tp == last)
      return ENOERR;
    tp++;
    wp++; /* Note Bene */
    /*FALLTHRU*/  
  }

  assert((nelems - rem) != 0);
  {
    const int nwords = ((nelems - rem) * X_SIZEOF_SHORT)
          / sizeof(word);
    const word *const endw = &wp[nwords];

#pragma _CRI ivdep
    for( ; wp < endw; wp++)
    {

      *tp = (short)(*wp >> 48);
      if(*tp & 0x8000)
        *tp |= (~(0xffff));
      tp++;

      *tp = (short)((*wp >> 32) & 0xffff);
      if(*tp & 0x8000)
        *tp |= (~(0xffff));
      tp++;

      *tp = (short)((*wp >> 16) & 0xffff);
      if(*tp & 0x8000)
        *tp |= (~(0xffff));
      tp++;

      *tp = (short)(*wp & 0xffff);
      if(*tp & 0x8000)
        *tp |= (~(0xffff));
      tp++;
    }
  }

  if(tp <= last)
  {
    *tp = (short)(*wp >> 48);
    if(*tp & 0x8000)
      *tp |= (~(0xffff));
    tp++;
  }
  if(tp <= last)
  {
    *tp = (short)((*wp >> 32) & 0xffff);
    if(*tp & 0x8000)
      *tp |= (~(0xffff));
    tp++;
  }
  if(tp <= last)
  {
    *tp = (short)((*wp >> 16) & 0xffff);
    if(*tp & 0x8000)
      *tp |= (~(0xffff));
    tp++;
  }

  }
  return ENOERR;
}
#endif

int
ncx_getn_short_int(const void **xpp, size_t nelems, int *tp)
{
  const char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_SHORT, tp++)
  {
    const int lstatus = ncx_get_short_int(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  *xpp = (const void *)xp;
  return status;
}

int
ncx_getn_short_long(const void **xpp, size_t nelems, long *tp)
{
  const char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_SHORT, tp++)
  {
    const int lstatus = ncx_get_short_long(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  *xpp = (const void *)xp;
  return status;
}

int
ncx_getn_short_float(const void **xpp, size_t nelems, float *tp)
{
  const char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_SHORT, tp++)
  {
    const int lstatus = ncx_get_short_float(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  *xpp = (const void *)xp;
  return status;
}

int
ncx_getn_short_double(const void **xpp, size_t nelems, double *tp)
{
  const char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_SHORT, tp++)
  {
    const int lstatus = ncx_get_short_double(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  *xpp = (const void *)xp;
  return status;
}


int
ncx_pad_getn_short_schar(const void **xpp, size_t nelems, schar *tp)
{
  const size_t rndup = nelems % 2;

  const char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_SHORT, tp++)
  {
    const int lstatus = ncx_get_short_schar(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  if(rndup != 0)
    xp += X_SIZEOF_SHORT;
    
  *xpp = (void *)xp;
  return status;
}

int
ncx_pad_getn_short_uchar(const void **xpp, size_t nelems, uchar *tp)
{
  const size_t rndup = nelems % 2;

  const char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_SHORT, tp++)
  {
    const int lstatus = ncx_get_short_uchar(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  if(rndup != 0)
    xp += X_SIZEOF_SHORT;
    
  *xpp = (void *)xp;
  return status;
}

int
ncx_pad_getn_short_short(const void **xpp, size_t nelems, short *tp)
{
  const size_t rndup = nelems % 2;

  const int status = ncx_getn_short_short(xpp, nelems, tp);

  if(rndup != 0)
  {
    *xpp = ((char *) (*xpp) + X_SIZEOF_SHORT);
  }
    
  return status;
}

int
ncx_pad_getn_short_int(const void **xpp, size_t nelems, int *tp)
{
  const size_t rndup = nelems % 2;

  const char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_SHORT, tp++)
  {
    const int lstatus = ncx_get_short_int(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  if(rndup != 0)
    xp += X_SIZEOF_SHORT;
    
  *xpp = (void *)xp;
  return status;
}

int
ncx_pad_getn_short_long(const void **xpp, size_t nelems, long *tp)
{
  const size_t rndup = nelems % 2;

  const char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_SHORT, tp++)
  {
    const int lstatus = ncx_get_short_long(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  if(rndup != 0)
    xp += X_SIZEOF_SHORT;
    
  *xpp = (void *)xp;
  return status;
}

int
ncx_pad_getn_short_float(const void **xpp, size_t nelems, float *tp)
{
  const size_t rndup = nelems % 2;

  const char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_SHORT, tp++)
  {
    const int lstatus = ncx_get_short_float(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  if(rndup != 0)
    xp += X_SIZEOF_SHORT;
    
  *xpp = (void *)xp;
  return status;
}

int
ncx_pad_getn_short_double(const void **xpp, size_t nelems, double *tp)
{
  const size_t rndup = nelems % 2;

  const char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_SHORT, tp++)
  {
    const int lstatus = ncx_get_short_double(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  if(rndup != 0)
    xp += X_SIZEOF_SHORT;
    
  *xpp = (void *)xp;
  return status;
}


int
ncx_putn_short_schar(void **xpp, size_t nelems, const schar *tp)
{
  char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_SHORT, tp++)
  {
    const int lstatus = ncx_put_short_schar(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  *xpp = (void *)xp;
  return status;
}

int
ncx_putn_short_uchar(void **xpp, size_t nelems, const uchar *tp)
{
  char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_SHORT, tp++)
  {
    const int lstatus = ncx_put_short_uchar(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  *xpp = (void *)xp;
  return status;
}

#if SHORT_USE_IEG
int
ncx_putn_short_short(void **xpp, size_t nelems, const short *tp)
{
  if(nelems > 0)
  {
    word *wp = *xpp;
    const int bo = bitoff(*xpp);
    int ierr;
  
    *xpp = ((char *) (*xpp) + nelems * X_SIZEOF_SHORT);
  
    ierr = CRAY2IEG(&Cray2_I16, &nelems, wp,
        &bo, tp, &UnitStride);
    assert(ierr >= 0);
    if(ierr > 0)
      return NC_ERANGE;
  }
  return ENOERR;
}
#else
int
ncx_putn_short_short(void **xpp, const size_t nelems, const short *tp)
{
  int status = ENOERR;
  if(nelems == 0)
    return status;
{
  word *wp = *xpp;
  const short *const last = &tp[nelems -1];
  const int rem = word_align(*xpp)/X_SIZEOF_SHORT;
  *xpp = ((char *) (*xpp) + nelems * X_SIZEOF_SHORT);

  switch(rem) {
  case 3:
    *wp = ((*tp << 32) & 0x0000ffff00000000)
          | (*wp & 0xffff0000ffffffff);
    if(*tp > X_SHORT_MAX || *tp < X_SHORT_MIN)
      status = NC_ERANGE;
    if(tp == last)
      return status;
    tp++;
    /*FALLTHRU*/  
  case 2:
    *wp = ((*tp << 16) & 0x00000000ffff0000)
          | (*wp & 0xffffffff0000ffff);
    if(*tp > X_SHORT_MAX || *tp < X_SHORT_MIN)
      status = NC_ERANGE;
    if(tp == last)
      return status;
    tp++;
    /*FALLTHRU*/  
  case 1:
    *wp = (*tp         & 0x000000000000ffff)   
          | (*wp & 0xffffffffffff0000);
    if(*tp > X_SHORT_MAX || *tp < X_SHORT_MIN)
      status = NC_ERANGE;
    if(tp == last)
      return status;
    tp++;
    wp++; /* Note Bene */
    /*FALLTHRU*/  
  }

  assert((nelems - rem) != 0);
  {
    const int nwords = ((nelems - rem) * X_SIZEOF_SHORT)
          / sizeof(word);
    const word *const endw = &wp[nwords];

#pragma _CRI ivdep
    for( ; wp < endw; wp++)
    {
      *wp =     (*tp      << 48)
        | ((*(tp +1) << 32) & 0x0000ffff00000000)
        | ((*(tp +2) << 16) & 0x00000000ffff0000)
        | ((*(tp +3)      ) & 0x000000000000ffff);

      { int ii = 0;
      for(; ii < 4; ii++, tp++)
        if(*tp > X_SHORT_MAX || *tp < X_SHORT_MIN)
          status = NC_ERANGE;
      }
    }
  }

  if(tp <= last)
  {
    *wp = (*tp << 48)
          | (*wp & 0x0000ffffffffffff);
    if(*tp > X_SHORT_MAX || *tp < X_SHORT_MIN)
      status = NC_ERANGE;
    tp++;
  }
  if(tp <= last)
  {
    *wp = ((*tp << 32) & 0x0000ffff00000000)
          | (*wp & 0xffff0000ffffffff);
    if(*tp > X_SHORT_MAX || *tp < X_SHORT_MIN)
      status = NC_ERANGE;
    tp++;
  }
  if(tp <= last)
  {
    *wp = ((*tp << 16) & 0x00000000ffff0000)
          | (*wp & 0xffffffff0000ffff);
    if(*tp > X_SHORT_MAX || *tp < X_SHORT_MIN)
      status = NC_ERANGE;
  }

  return status;
}
}
#endif

int
ncx_putn_short_int(void **xpp, size_t nelems, const int *tp)
{
  char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_SHORT, tp++)
  {
    const int lstatus = ncx_put_short_int(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  *xpp = (void *)xp;
  return status;
}

int
ncx_putn_short_long(void **xpp, size_t nelems, const long *tp)
{
  char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_SHORT, tp++)
  {
    const int lstatus = ncx_put_short_long(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  *xpp = (void *)xp;
  return status;
}

int
ncx_putn_short_float(void **xpp, size_t nelems, const float *tp)
{
  char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_SHORT, tp++)
  {
    const int lstatus = ncx_put_short_float(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  *xpp = (void *)xp;
  return status;
}

int
ncx_putn_short_double(void **xpp, size_t nelems, const double *tp)
{
  char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_SHORT, tp++)
  {
    const int lstatus = ncx_put_short_double(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  *xpp = (void *)xp;
  return status;
}


int
ncx_pad_putn_short_schar(void **xpp, size_t nelems, const schar *tp)
{
  const size_t rndup = nelems % 2;

  char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_SHORT, tp++)
  {
    const int lstatus = ncx_put_short_schar(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  if(rndup != 0)
  {
    (void) memcpy(xp, nada, X_SIZEOF_SHORT);
    xp += X_SIZEOF_SHORT;  
  }
    
  *xpp = (void *)xp;
  return status;
}

int
ncx_pad_putn_short_uchar(void **xpp, size_t nelems, const uchar *tp)
{
  const size_t rndup = nelems % 2;

  char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_SHORT, tp++)
  {
    const int lstatus = ncx_put_short_uchar(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  if(rndup != 0)
  {
    (void) memcpy(xp, nada, X_SIZEOF_SHORT);
    xp += X_SIZEOF_SHORT;  
  }
    
  *xpp = (void *)xp;
  return status;
}

int
ncx_pad_putn_short_short(void **xpp, size_t nelems, const short *tp)
{
  const size_t rndup = nelems % 2;

  const int status = ncx_putn_short_short(xpp, nelems, tp);

  if(rndup != 0)
  {
    *xpp = ((char *) (*xpp) + X_SIZEOF_SHORT);
  }
    
  return status;
}

int
ncx_pad_putn_short_int(void **xpp, size_t nelems, const int *tp)
{
  const size_t rndup = nelems % 2;

  char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_SHORT, tp++)
  {
    const int lstatus = ncx_put_short_int(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  if(rndup != 0)
  {
    (void) memcpy(xp, nada, X_SIZEOF_SHORT);
    xp += X_SIZEOF_SHORT;  
  }
    
  *xpp = (void *)xp;
  return status;
}

int
ncx_pad_putn_short_long(void **xpp, size_t nelems, const long *tp)
{
  const size_t rndup = nelems % 2;

  char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_SHORT, tp++)
  {
    const int lstatus = ncx_put_short_long(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  if(rndup != 0)
  {
    (void) memcpy(xp, nada, X_SIZEOF_SHORT);
    xp += X_SIZEOF_SHORT;  
  }
    
  *xpp = (void *)xp;
  return status;
}

int
ncx_pad_putn_short_float(void **xpp, size_t nelems, const float *tp)
{
  const size_t rndup = nelems % 2;

  char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_SHORT, tp++)
  {
    const int lstatus = ncx_put_short_float(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  if(rndup != 0)
  {
    (void) memcpy(xp, nada, X_SIZEOF_SHORT);
    xp += X_SIZEOF_SHORT;  
  }
    
  *xpp = (void *)xp;
  return status;
}

int
ncx_pad_putn_short_double(void **xpp, size_t nelems, const double *tp)
{
  const size_t rndup = nelems % 2;

  char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_SHORT, tp++)
  {
    const int lstatus = ncx_put_short_double(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  if(rndup != 0)
  {
    (void) memcpy(xp, nada, X_SIZEOF_SHORT);
    xp += X_SIZEOF_SHORT;  
  }
    
  *xpp = (void *)xp;
  return status;
}



/* int */

int
ncx_getn_int_schar(const void **xpp, size_t nelems, schar *tp)
{
  const char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_INT, tp++)
  {
    const int lstatus = ncx_get_int_schar(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  *xpp = (const void *)xp;
  return status;
}

int
ncx_getn_int_uchar(const void **xpp, size_t nelems, uchar *tp)
{
  const char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_INT, tp++)
  {
    const int lstatus = ncx_get_int_uchar(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  *xpp = (const void *)xp;
  return status;
}

int
ncx_getn_int_short(const void **xpp, size_t nelems, short *tp)
{
  const char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_INT, tp++)
  {
    const int lstatus = ncx_get_int_short(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  *xpp = (const void *)xp;
  return status;
}

#if INT_USE_IEG
int
ncx_getn_int_int(const void **xpp, size_t nelems, int *tp)
{
  if(nelems > 0)
  {
    const int bo = bitoff(*xpp);
    const word *wp = *xpp;
    int ierr;
    *xpp = ((const char *) (*xpp) + nelems * X_SIZEOF_INT);
    ierr = IEG2CRAY(&Cray2_I32, &nelems, wp,
        &bo, tp, &UnitStride);
    assert(ierr >= 0);
    if(ierr > 0)
      return NC_ERANGE;
  }
  return ENOERR;
}
#else
int
ncx_getn_int_int(const void **xpp, size_t nelems, int *tp)
{
  const int bo = byteoff(*xpp);

  if(nelems == 0)
    return ENOERR;

  if(bo != 0)
  {
    cget_int_int(*xpp, tp, bo);
    *xpp = ((char *) (*xpp) + X_SIZEOF_INT);
    nelems--;
    if(nelems == 0)
      return ENOERR;
    tp++;
  }

  assert(byteoff(*xpp) == 0);

  {
    const int nwords = (nelems * X_SIZEOF_INT)/sizeof(word);
    const word *wp = *xpp;
    const word *const end = &wp[nwords];

#pragma _CRI ivdep
    for( ; wp < end; wp++, tp += 2)
    {
      cget_int_int(wp, tp, 0);
      cget_int_int(wp, tp + 1, 1);
    }

    *xpp = ((char *) (*xpp) + nwords * sizeof(word)); 
    nelems -= (nwords * sizeof(word)/X_SIZEOF_INT);
    if(nelems != 0)
    {
      cget_int_int(wp, tp, 0);
      *xpp = ((char *) (*xpp) + X_SIZEOF_INT);
    }
  }

  return ENOERR;
}
#endif

int
ncx_getn_int_long(const void **xpp, size_t nelems, long *tp)
{
  const int bo = byteoff(*xpp);

  if(nelems == 0)
    return ENOERR;

  if(bo != 0)
  {
    cget_int_long(*xpp, tp, bo);
    *xpp = ((char *) (*xpp) + X_SIZEOF_INT);
    nelems--;
    if(nelems == 0)
      return ENOERR;
    tp++;
  }

  assert(byteoff(*xpp) == 0);

  {
    const int nwords = (nelems * X_SIZEOF_INT)/sizeof(word);
    const word *wp = *xpp;
    const word *const end = &wp[nwords];

#pragma _CRI ivdep
    for( ; wp < end; wp++, tp += 2)
    {
      cget_int_long(wp, tp, 0);
      cget_int_long(wp, tp + 1, 1);
    }

    *xpp = ((char *) (*xpp) + nwords * sizeof(word)); 
    nelems -= (nwords * sizeof(word)/X_SIZEOF_INT);
    if(nelems != 0)
    {
      cget_int_long(wp, tp, 0);
      *xpp = ((char *) (*xpp) + X_SIZEOF_INT);
    }
  }

  return ENOERR;
}

int
ncx_getn_int_float(const void **xpp, size_t nelems, float *tp)
{
  const char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_INT, tp++)
  {
    const int lstatus = ncx_get_int_float(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  *xpp = (const void *)xp;
  return status;
}

int
ncx_getn_int_double(const void **xpp, size_t nelems, double *tp)
{
  const char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_INT, tp++)
  {
    const int lstatus = ncx_get_int_double(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  *xpp = (const void *)xp;
  return status;
}


int
ncx_putn_int_schar(void **xpp, size_t nelems, const schar *tp)
{
  char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_INT, tp++)
  {
    const int lstatus = ncx_put_int_schar(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  *xpp = (void *)xp;
  return status;
}

int
ncx_putn_int_uchar(void **xpp, size_t nelems, const uchar *tp)
{
  char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_INT, tp++)
  {
    const int lstatus = ncx_put_int_uchar(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  *xpp = (void *)xp;
  return status;
}

int
ncx_putn_int_short(void **xpp, size_t nelems, const short *tp)
{
  char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_INT, tp++)
  {
    const int lstatus = ncx_put_int_short(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  *xpp = (void *)xp;
  return status;
}

#if INT_USE_IEG
int
ncx_putn_int_int(void **xpp, size_t nelems, const int *tp)
{
  if(nelems > 0)
  {
    word *wp = *xpp;
    const int bo = bitoff(*xpp);
    int ierr;
  
    *xpp = ((char *) (*xpp) + nelems * X_SIZEOF_INT);
  
    ierr = CRAY2IEG(&Cray2_I32, &nelems, wp,
        &bo, tp, &UnitStride);
    assert(ierr >= 0);
    if(ierr > 0)
      return NC_ERANGE;
  }
  return ENOERR;
}
#else
int
ncx_putn_int_int(void **xpp, size_t nelems, const int *tp)
{
  int status = ENOERR;
  const int bo = byteoff(*xpp);

  if(nelems == 0)
    return ENOERR;

  if(bo != 0)
  {
    status = cput_int_int(*xpp, tp, bo);
    *xpp = ((char *) (*xpp) + X_SIZEOF_INT);
    nelems--;
    if(nelems == 0)
      return status;
    tp++;
  }

  assert(byteoff(*xpp) == 0);

  {
    const int nwords = (nelems * X_SIZEOF_INT)/sizeof(word);
    word *wp = *xpp;
    const word *const end = &wp[nwords];

#pragma _CRI ivdep
    for( ; wp < end; wp++, tp += 2)
    {
      int lstatus = cput_int_int(wp, tp, 0);
      if(lstatus != ENOERR)
        status = lstatus;
      lstatus = cput_int_int(wp, tp + 1, 1);
      if(lstatus != ENOERR)
        status = lstatus;
    }

    *xpp = ((char *) (*xpp) + nwords * sizeof(word)); 
    nelems -= (nwords * sizeof(word)/X_SIZEOF_INT);
    if(nelems != 0)
    {
      const int lstatus = cput_int_int(wp, tp, 0);
      if(lstatus != ENOERR)
        status = lstatus;
      *xpp = ((char *) (*xpp) + X_SIZEOF_INT);
    }
  }

  return status;
}
#endif

int
ncx_putn_int_long(void **xpp, size_t nelems, const long *tp)
{
  int status = ENOERR;
  const int bo = byteoff(*xpp);

  if(nelems == 0)
    return ENOERR;

  if(bo != 0)
  {
    status = cput_int_long(*xpp, tp, bo);
    *xpp = ((char *) (*xpp) + X_SIZEOF_INT);
    nelems--;
    if(nelems == 0)
      return status;
    tp++;
  }

  assert(byteoff(*xpp) == 0);

  {
    const int nwords = (nelems * X_SIZEOF_INT)/sizeof(word);
    word *wp = *xpp;
    const word *const end = &wp[nwords];

#pragma _CRI ivdep
    for( ; wp < end; wp++, tp += 2)
    {
      int lstatus = cput_int_long(wp, tp, 0);
      if(lstatus != ENOERR)
        status = lstatus;
      lstatus = cput_int_long(wp, tp + 1, 1);
      if(lstatus != ENOERR)
        status = lstatus;
    }

    *xpp = ((char *) (*xpp) + nwords * sizeof(word)); 
    nelems -= (nwords * sizeof(word)/X_SIZEOF_INT);
    if(nelems != 0)
    {
      const int lstatus = cput_int_long(wp, tp, 0);
      if(lstatus != ENOERR)
        status = lstatus;
      *xpp = ((char *) (*xpp) + X_SIZEOF_INT);
    }
  }

  return status;
}

int
ncx_putn_int_float(void **xpp, size_t nelems, const float *tp)
{
  char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_INT, tp++)
  {
    const int lstatus = ncx_put_int_float(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  *xpp = (void *)xp;
  return status;
}

int
ncx_putn_int_double(void **xpp, size_t nelems, const double *tp)
{
  char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_INT, tp++)
  {
    const int lstatus = ncx_put_int_double(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  *xpp = (void *)xp;
  return status;
}



/* float */

int
ncx_getn_float_schar(const void **xpp, size_t nelems, schar *tp)
{
  const char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_FLOAT, tp++)
  {
    const int lstatus = ncx_get_float_schar(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  *xpp = (const void *)xp;
  return status;
}

int
ncx_getn_float_uchar(const void **xpp, size_t nelems, uchar *tp)
{
  const char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_FLOAT, tp++)
  {
    const int lstatus = ncx_get_float_uchar(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  *xpp = (const void *)xp;
  return status;
}

int
ncx_getn_float_short(const void **xpp, size_t nelems, short *tp)
{
  const char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_FLOAT, tp++)
  {
    const int lstatus = ncx_get_float_short(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  *xpp = (const void *)xp;
  return status;
}

int
ncx_getn_float_int(const void **xpp, size_t nelems, int *tp)
{
  const char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_FLOAT, tp++)
  {
    const int lstatus = ncx_get_float_int(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  *xpp = (const void *)xp;
  return status;
}

int
ncx_getn_float_long(const void **xpp, size_t nelems, long *tp)
{
  const char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_FLOAT, tp++)
  {
    const int lstatus = ncx_get_float_long(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  *xpp = (const void *)xp;
  return status;
}

#if FLOAT_USE_IEG
int
ncx_getn_float_float(const void **xpp, size_t nelems, float *tp)
{
  if(nelems > 0)
  {
    const int bo = bitoff(*xpp);
    const word *wp = *xpp;
    int ierr;
    *xpp = ((const char *) (*xpp) + nelems * X_SIZEOF_FLOAT);
    ierr = IEG2CRAY(&Cray2_F32, &nelems, wp,
        &bo, tp, &UnitStride);
    assert(ierr >= 0);
    if(ierr > 0)
      return NC_ERANGE;
  }
  return ENOERR;

}
#else
int
ncx_getn_float_float(const void **xpp, size_t nelems, float *tp)
{
  const int bo = byteoff(*xpp);

  if(nelems == 0)
    return ENOERR;

  if(bo != 0)
  {
    cget_float_float(*xpp, tp, bo);
    *xpp = ((char *) (*xpp) + X_SIZEOF_FLOAT);
    nelems--;
    if(nelems == 0)
      return ENOERR;
    tp++;
  }

  assert(byteoff(*xpp) == 0);

  {
    const int nwords = (nelems * X_SIZEOF_FLOAT)/sizeof(word);
    const word *wp = *xpp;
    const word *const end = &wp[nwords];

#pragma _CRI ivdep
    for( ; wp < end; wp++, tp += 2)
    {
      cget_float_float(wp, tp, 0);
      cget_float_float(wp, tp + 1, 1);
    }

    *xpp = ((char *) (*xpp) + nwords * sizeof(word)); 
    nelems -= (nwords * sizeof(word)/X_SIZEOF_FLOAT);
    if(nelems != 0)
    {
      cget_float_float(wp, tp, 0);
      *xpp = ((char *) (*xpp) + X_SIZEOF_FLOAT);
    }
  }

  return ENOERR;
}
#endif

int
ncx_getn_float_double(const void **xpp, size_t nelems, double *tp)
{
  const char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_FLOAT, tp++)
  {
    const int lstatus = ncx_get_float_double(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  *xpp = (const void *)xp;
  return status;
}


int
ncx_putn_float_schar(void **xpp, size_t nelems, const schar *tp)
{
  char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_FLOAT, tp++)
  {
    const int lstatus = ncx_put_float_schar(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  *xpp = (void *)xp;
  return status;
}

int
ncx_putn_float_uchar(void **xpp, size_t nelems, const uchar *tp)
{
  char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_FLOAT, tp++)
  {
    const int lstatus = ncx_put_float_uchar(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  *xpp = (void *)xp;
  return status;
}

int
ncx_putn_float_short(void **xpp, size_t nelems, const short *tp)
{
  char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_FLOAT, tp++)
  {
    const int lstatus = ncx_put_float_short(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  *xpp = (void *)xp;
  return status;
}

int
ncx_putn_float_int(void **xpp, size_t nelems, const int *tp)
{
  char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_FLOAT, tp++)
  {
    const int lstatus = ncx_put_float_int(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  *xpp = (void *)xp;
  return status;
}

int
ncx_putn_float_long(void **xpp, size_t nelems, const long *tp)
{
  char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_FLOAT, tp++)
  {
    const int lstatus = ncx_put_float_long(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  *xpp = (void *)xp;
  return status;
}

#if FLOAT_USE_IEG
int
ncx_putn_float_float(void **xpp, size_t nelems, const float *tp)
{
  if(nelems > 0)
  {
    word *wp = *xpp;
    const int bo = bitoff(*xpp);
    int ierr;
  
    *xpp = ((char *) (*xpp) + nelems * X_SIZEOF_FLOAT);
  
    ierr = CRAY2IEG(&Cray2_F32, &nelems, wp,
        &bo, tp, &UnitStride);
    assert(ierr >= 0);
    if(ierr > 0)
      return NC_ERANGE;
  }
  return ENOERR;
}
#else
int
ncx_putn_float_float(void **xpp, size_t nelems, const float *tp)
{
  int status = ENOERR;
  const int bo = byteoff(*xpp);

  if(nelems == 0)
    return ENOERR;

  if(bo != 0)
  {
    status = cput_float_float(*xpp, tp, bo);
    *xpp = ((char *) (*xpp) + X_SIZEOF_FLOAT);
    nelems--;
    if(nelems == 0)
      return status;
    tp++;
  }

  assert(byteoff(*xpp) == 0);

  {
    const int nwords = (nelems * X_SIZEOF_FLOAT)/sizeof(word);
    word *wp = *xpp;
    const word *const end = &wp[nwords];

#pragma _CRI ivdep
    for( ; wp < end; wp++, tp += 2)
    {
      int lstatus = cput_float_float(wp, tp, 0);
      if(lstatus != ENOERR)
        status = lstatus;
      lstatus = cput_float_float(wp, tp + 1, 1);
      if(lstatus != ENOERR)
        status = lstatus;
    }

    *xpp = ((char *) (*xpp) + nwords * sizeof(word)); 
    nelems -= (nwords * sizeof(word)/X_SIZEOF_FLOAT);
    if(nelems != 0)
    {
      const int lstatus = cput_float_float(wp, tp, 0);
      if(lstatus != ENOERR)
        status = lstatus;
      *xpp = ((char *) (*xpp) + X_SIZEOF_FLOAT);
    }
  }

  return status;
}
#endif

int
ncx_putn_float_double(void **xpp, size_t nelems, const double *tp)
{
  char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_FLOAT, tp++)
  {
    const int lstatus = ncx_put_float_double(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  *xpp = (void *)xp;
  return status;
}



/* double */

int
ncx_getn_double_schar(const void **xpp, size_t nelems, schar *tp)
{
  const char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_DOUBLE, tp++)
  {
    const int lstatus = ncx_get_double_schar(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  *xpp = (const void *)xp;
  return status;
}

int
ncx_getn_double_uchar(const void **xpp, size_t nelems, uchar *tp)
{
  const char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_DOUBLE, tp++)
  {
    const int lstatus = ncx_get_double_uchar(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  *xpp = (const void *)xp;
  return status;
}

int
ncx_getn_double_short(const void **xpp, size_t nelems, short *tp)
{
  const char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_DOUBLE, tp++)
  {
    const int lstatus = ncx_get_double_short(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  *xpp = (const void *)xp;
  return status;
}

int
ncx_getn_double_int(const void **xpp, size_t nelems, int *tp)
{
  const char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_DOUBLE, tp++)
  {
    const int lstatus = ncx_get_double_int(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  *xpp = (const void *)xp;
  return status;
}

int
ncx_getn_double_long(const void **xpp, size_t nelems, long *tp)
{
  const char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_DOUBLE, tp++)
  {
    const int lstatus = ncx_get_double_long(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  *xpp = (const void *)xp;
  return status;
}

#if DOUBLE_USE_IEG
int
ncx_getn_double_double(const void **xpp, size_t nelems, double *tp)
{
  const size_t noff = byteoff(*xpp);
  int ierr;

  if(nelems == 0)
    return ENOERR;

  if(noff != 0)
  {
    /* (*xpp) not word aligned, forced to make a copy */
    word xbuf[nelems];
    (void) memcpy(xbuf, *xpp, nelems * X_SIZEOF_DOUBLE);
    ierr = IEG2CRAY(&Cray2_F64, &nelems, xbuf,
      &Zero, tp, &UnitStride);
  }
  else
  {
    ierr = IEG2CRAY(&Cray2_F64, &nelems, *xpp,
        &Zero, tp, &UnitStride);
  }

  *xpp = (void *)((char *)(*xpp) + nelems * X_SIZEOF_DOUBLE);

  assert(ierr >= 0);

  return ierr > 0 ? NC_ERANGE : ENOERR;
}
#elif X_SIZEOF_DOUBLE == SIZEOF_DOUBLE  && !defined(NO_IEEE_FLOAT)
int
ncx_getn_double_double(const void **xpp, size_t nelems, double *tp)
{
  (void) memcpy(tp, *xpp, nelems * X_SIZEOF_DOUBLE);
  *xpp = (void *)((char *)(*xpp) + nelems * X_SIZEOF_DOUBLE);
  return ENOERR;
}
#else
int
ncx_getn_double_double(const void **xpp, size_t nelems, double *tp)
{
  const size_t noff = byteoff(*xpp);

  if(nelems == 0)
    return ENOERR;

  if(noff != 0)
  {
    /* (*xpp) not word aligned, forced to make a copy */
    word xbuf[nelems];
    const word *wp = xbuf;
    const word *const end = &wp[nelems];

    (void) memcpy(xbuf, *xpp, nelems * X_SIZEOF_DOUBLE);

#pragma _CRI ivdep
    for( ; wp < end; wp++, tp++)
    {
      cget_double_double(wp, tp);
    }

  }
  else
  {
    const word *wp = *xpp;
    const word *const end = &wp[nelems];

#pragma _CRI ivdep
    for( ; wp < end; wp++, tp++)
    {
      cget_double_double(wp, tp);
    }

  }
  *xpp = (void *)((char *)(*xpp) + nelems * X_SIZEOF_DOUBLE);
  return ENOERR;
}
#endif

int
ncx_getn_double_float(const void **xpp, size_t nelems, float *tp)
{
#if SIZEOF_FLOAT == SIZEOF_DOUBLE && FLT_MANT_DIG == DBL_MANT_DIG
  return ncx_getn_double_double(xpp, nelems, (double *)tp);
#else
  const char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_DOUBLE, tp++)
  {
    const int lstatus = ncx_get_double_float(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  *xpp = (const void *)xp;
  return status;
#endif
}


int
ncx_putn_double_schar(void **xpp, size_t nelems, const schar *tp)
{
  char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_DOUBLE, tp++)
  {
    const int lstatus = ncx_put_double_schar(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  *xpp = (void *)xp;
  return status;
}

int
ncx_putn_double_uchar(void **xpp, size_t nelems, const uchar *tp)
{
  char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_DOUBLE, tp++)
  {
    const int lstatus = ncx_put_double_uchar(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  *xpp = (void *)xp;
  return status;
}

int
ncx_putn_double_short(void **xpp, size_t nelems, const short *tp)
{
  char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_DOUBLE, tp++)
  {
    const int lstatus = ncx_put_double_short(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  *xpp = (void *)xp;
  return status;
}

int
ncx_putn_double_int(void **xpp, size_t nelems, const int *tp)
{
  char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_DOUBLE, tp++)
  {
    const int lstatus = ncx_put_double_int(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  *xpp = (void *)xp;
  return status;
}

int
ncx_putn_double_long(void **xpp, size_t nelems, const long *tp)
{
  char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_DOUBLE, tp++)
  {
    const int lstatus = ncx_put_double_long(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  *xpp = (void *)xp;
  return status;
}

#if DOUBLE_USE_IEG
int
ncx_putn_double_double(void **xpp, size_t nelems, const double *tp)
{
  const size_t noff = byteoff(*xpp);
  int ierr;

  if(nelems == 0)
    return ENOERR;

  if(noff != 0)
  {
    /* (*xpp) not word aligned, forced to make a copy */
    word xbuf[nelems];
    ierr = CRAY2IEG(&Cray2_F64, &nelems, xbuf,
      &Zero, tp, &UnitStride);
    assert(ierr >= 0);
    (void) memcpy(*xpp, xbuf, nelems * X_SIZEOF_DOUBLE);
  }
  else
  {
    ierr = CRAY2IEG(&Cray2_F64, &nelems, *xpp,
        &Zero, tp, &UnitStride);
    assert(ierr >= 0);
  }

  *xpp = (void *)((char *)(*xpp) + nelems * X_SIZEOF_DOUBLE);

  return ierr > 0 ? NC_ERANGE : ENOERR;
}
#elif X_SIZEOF_DOUBLE == SIZEOF_DOUBLE  && !defined(NO_IEEE_FLOAT)
int
ncx_putn_double_double(void **xpp, size_t nelems, const double *tp)
{
  const size_t noff = byteoff(*xpp);
  (void) memcpy(*xpp, tp, nelems * X_SIZEOF_DOUBLE);
  *xpp = (void *)((char *)(*xpp) + nelems * X_SIZEOF_DOUBLE);
  return ENOERR;
}
#else
int
ncx_putn_double_double(void **xpp, size_t nelems, const double *tp)
{
  int status = ENOERR;
  const size_t noff = byteoff(*xpp);

  if(nelems == 0)
    return ENOERR;

  if(noff != 0)
  {
    /* (*xpp) not word aligned, forced to make a copy */
    word xbuf[nelems];
    word *wp = xbuf;
    const word *const end = &wp[nelems];

#pragma _CRI ivdep
    for( ; wp < end; wp++, tp++)
    {
      const int lstatus = cput_double_double(wp, tp);
      if(lstatus != ENOERR)
        status = lstatus;
    }

    (void) memcpy(*xpp, xbuf, nelems * X_SIZEOF_DOUBLE);
  }
  else
  {
    word *wp = *xpp;
    const word *const end = &wp[nelems];

#pragma _CRI ivdep
    for( ; wp < end; wp++, tp++)
    {
      const int lstatus = cput_double_double(wp, tp);
      if(lstatus != ENOERR)
        status = lstatus;
    }

  }

  *xpp = (void *)((char *)(*xpp) + nelems * X_SIZEOF_DOUBLE);
  return status;
}
#endif

int
ncx_putn_double_float(void **xpp, size_t nelems, const float *tp)
{
#if SIZEOF_FLOAT == SIZEOF_DOUBLE && FLT_MANT_DIG == DBL_MANT_DIG
  return ncx_putn_double_double(xpp, nelems, (double *)tp);
#else
  char *xp = *xpp;
  int status = ENOERR;

  for( ; nelems != 0; nelems--, xp += X_SIZEOF_DOUBLE, tp++)
  {
    const int lstatus = ncx_put_double_float(xp, tp);
    if(lstatus != ENOERR)
      status = lstatus;
  }

  *xpp = (void *)xp;
  return status;
#endif
}



/*
 * Other aggregate conversion functions.
 */

/* text */

int
ncx_getn_text(const void **xpp, size_t nelems, char *tp)
{
  (void) memcpy(tp, *xpp, nelems);
  *xpp = (void *)((char *)(*xpp) + nelems);
  return ENOERR;

}

int
ncx_pad_getn_text(const void **xpp, size_t nelems, char *tp)
{
  size_t rndup = nelems % X_ALIGN;

  if(rndup)
    rndup = X_ALIGN - rndup;

  (void) memcpy(tp, *xpp, nelems);
  *xpp = (void *)((char *)(*xpp) + nelems + rndup);

  return ENOERR;

}

int
ncx_putn_text(void **xpp, size_t nelems, const char *tp)
{
  (void) memcpy(*xpp, tp, nelems);
  *xpp = (void *)((char *)(*xpp) + nelems);

  return ENOERR;

}

int
ncx_pad_putn_text(void **xpp, size_t nelems, const char *tp)
{
  size_t rndup = nelems % X_ALIGN;

  if(rndup)
    rndup = X_ALIGN - rndup;

  (void) memcpy(*xpp, tp, nelems);
  *xpp = (void *)((char *)(*xpp) + nelems);

  if(rndup)
  {
    (void) memcpy(*xpp, nada, rndup);
    *xpp = (void *)((char *)(*xpp) + rndup);
  }
  
  return ENOERR;

}


/* opaque */

int
ncx_getn_void(const void **xpp, size_t nelems, void *tp)
{
  (void) memcpy(tp, *xpp, nelems);
  *xpp = (void *)((char *)(*xpp) + nelems);
  return ENOERR;

}

int
ncx_pad_getn_void(const void **xpp, size_t nelems, void *tp)
{
  size_t rndup = nelems % X_ALIGN;

  if(rndup)
    rndup = X_ALIGN - rndup;

  (void) memcpy(tp, *xpp, nelems);
  *xpp = (void *)((char *)(*xpp) + nelems + rndup);

  return ENOERR;

}

int
ncx_putn_void(void **xpp, size_t nelems, const void *tp)
{
  (void) memcpy(*xpp, tp, nelems);
  *xpp = (void *)((char *)(*xpp) + nelems);

  return ENOERR;

}

int
ncx_pad_putn_void(void **xpp, size_t nelems, const void *tp)
{
  size_t rndup = nelems % X_ALIGN;

  if(rndup)
    rndup = X_ALIGN - rndup;

  (void) memcpy(*xpp, tp, nelems);
  *xpp = (void *)((char *)(*xpp) + nelems);

  if(rndup)
  {
    (void) memcpy(*xpp, nada, rndup);
    *xpp = (void *)((char *)(*xpp) + rndup);
  }
  
  return ENOERR;

}
