/*
 *  Copyright 1996, University Corporation for Atmospheric Research
 *  See netcdf/COPYRIGHT file for copying and redistribution conditions.
 */
/* Id */

#include <stdio.h>
#include <limits.h>
/* alias poorly named limits.h macros */
#define  SHORT_MAX  SHRT_MAX
#define  SHORT_MIN  SHRT_MIN
#define USHORT_MAX USHRT_MAX
#include <rpc/types.h>
#include <rpc/xdr.h>
#include <string.h>
#include "ncx.h"

#define NO_UNSIGNED
#define NO_UNSIGNED_LONG

/*
 * This program tests the xdr_mem implementation and
 * the ncx_ implementation, and compares the two.
 * Link like this: 
 * cc t_ncx.c ncx.o [-lsome_xdr_lib] -o t_nxc
 * Successful output is:
  xdr_encode ends at byte 640
  xdr_check  ends at byte 640
  ncx_encode ends at byte 640
  ncx_check  ends at byte 640
  xdr_check  ends at byte 640
  ncx_check  ends at byte 640
 * with exit status 0;
 */

#define XBSZ 1024

char xdrb[XBSZ];
char ncxb[XBSZ];

#define ArraySize(thang) (sizeof(thang)/sizeof(thang[0]))
#define uiArraySize(thang) ((u_int)ArraySize(thang))
#define eSizeOf(thang) ((u_int)(sizeof(thang[0])))

/*
 * Some test data
 */

static char text[] = { "Hiya sailor. New in town?" };

/*
 * Some test data
 * The ideas is that ncx_putn_type_type(...., types)
 * should not return NC_ERANGE.
 */

#if SCHAR_MAX == X_SCHAR_MAX && SCHAR_MIN == X_SCHAR_MIN
static schar schars[] = {
  SCHAR_MIN, SCHAR_MIN +1,
  -1, 0, 1,
  SCHAR_MAX - 1, SCHAR_MAX
};
#else
/* The implementation and this test assume 8 bit bytes. */
#error "Not 8 bit bytes ??"
#endif

static short shorts[] = {
#if SHORT_MAX <= X_SHORT_MAX
  SHORT_MIN, SHORT_MIN + 1,
#  if SCHAR_MAX < X_SHORT_MAX
  SCHAR_MIN - 1, SCHAR_MIN, SCHAR_MIN + 1,
#  endif
  -1, 0, 1,
#  if SCHAR_MAX < X_SHORT_MAX
  SCHAR_MAX - 1, SCHAR_MAX, SCHAR_MAX + 1,
#  endif
  SHORT_MAX - 1, SHORT_MAX
#else
  X_SHORT_MIN, X_SHORT_MIN + 1,
#  if SCHAR_MAX < X_SHORT_MAX
  SCHAR_MIN - 1, SCHAR_MIN, SCHAR_MIN + 1,
#  endif
  -1, 0, 1,
#  if SCHAR_MAX < X_SHORT_MAX
  SCHAR_MAX - 1, SCHAR_MAX, SCHAR_MAX + 1,
#  endif
  X_SHORT_MAX - 1, X_SHORT_MAX
#endif
};

static int ints[] = {
#if INT_MAX <= X_INT_MAX
  INT_MIN, INT_MIN +1,
#  if SHORT_MAX < X_INT_MAX
  SHORT_MIN -1, SHORT_MIN, SHORT_MIN + 1,
#  endif
#  if SCHAR_MAX < X_INT_MAX
  SCHAR_MIN - 1, SCHAR_MIN, SCHAR_MIN + 1,
#  endif
  -1, 0, 1,
#  if SCHAR_MAX < X_INT_MAX
  SCHAR_MAX - 1, SCHAR_MAX, SCHAR_MAX + 1,
#  endif
#  if SHORT_MAX < X_INT_MAX
  SHORT_MAX - 1, SHORT_MAX, SHORT_MAX +1,
#  endif
  INT_MAX - 1, INT_MAX
#else
  X_INT_MIN, X_INT_MIN +1,
#  if SHORT_MAX < X_INT_MAX
  SHORT_MIN -1, SHORT_MIN, SHORT_MIN + 1,
#  endif
#  if SCHAR_MAX < X_INT_MAX
  SCHAR_MIN - 1, SCHAR_MIN, SCHAR_MIN + 1,
#  endif
  -1, 0, 1,
#  if SCHAR_MAX < X_INT_MAX
  SCHAR_MAX - 1, SCHAR_MAX, SCHAR_MAX + 1,
#  endif
#  if SHORT_MAX < X_INT_MAX
  SHORT_MAX - 1, SHORT_MAX, SHORT_MAX +1,
#  endif
  X_INT_MAX - 1, X_INT_MAX
#endif /* INT */
};


/* N.B. only testing longs over X_INT range for now */
static long longs[] = {
#if LONG_MAX <= X_INT_MAX
  LONG_MIN, LONG_MIN +1,
#  if INT_MAX < X_INT_MAX
  INT_MIN -1, INT_MIN, INT_MIN + 1,
#  endif
#  if SHORT_MAX < X_INT_MAX
  SHORT_MIN -1, SHORT_MIN, SHORT_MIN + 1,
#  endif
#  if SCHAR_MAX < X_INT_MAX
  SCHAR_MIN - 1, SCHAR_MIN, SCHAR_MIN + 1,
#  endif
  -1, 0, 1,
#  if SCHAR_MAX < X_INT_MAX
  SCHAR_MAX - 1, SCHAR_MAX, SCHAR_MAX + 1,
#  endif
#  if SHORT_MAX < X_INT_MAX
  SHORT_MAX - 1, SHORT_MAX, SHORT_MAX +1,
#  endif
#  if INT_MAX < X_INT_MAX
  INT_MAX -1, INT_MAX, INT_MAX + 1,
#  endif
  LONG_MAX - 1, LONG_MAX
#else
  X_INT_MIN, X_INT_MIN +1,
#  if SHORT_MAX < X_INT_MAX
  SHORT_MIN -1, SHORT_MIN, SHORT_MIN + 1,
#  endif
#  if SCHAR_MAX < X_INT_MAX
  SCHAR_MIN - 1, SCHAR_MIN, SCHAR_MIN + 1,
#  endif
  -1, 0, 1,
#  if SCHAR_MAX < X_INT_MAX
  SCHAR_MAX - 1, SCHAR_MAX, SCHAR_MAX + 1,
#  endif
#  if SHORT_MAX < X_INT_MAX
  SHORT_MAX - 1, SHORT_MAX, SHORT_MAX +1,
#  endif
  X_INT_MAX - 1, X_INT_MAX
#endif
};

static float floats[] = {
   -100.625, -100.5, -100.375, -100.25, -100.125,
  -1.0, -.125, 0., .125, 1.,
   100.125, 100.25, 100.375, 100.5, 100.625
};

/* The big numbers require 25 bits: 2^(25-i)+1/2^i, i = 2, 3, ..., 6 */
static double doubles[] = {
  -8388608.25, -4194304.125, -2097152.0625, -1048576.03125, -524288.015625
  -100.625, -100.5, -100.375, -100.25, -100.125,
  -1.0, -.125, 0., .125, 1.,
  100.125, 100.25, 100.375, 100.5, 100.625,
  524288.015625, 1048576.03125, 2097152.0625, 4194304.125, 8388608.25
};

/* End of test data */

/*
 *  Copyright 1993, University Corporation for Atmospheric Research
 *      See netcdf/COPYRIGHT file for copying and redistribution conditions.
 */
/*  Id */

/* putget.c */
/*
 * xdr 1 - 3 bytes, leaving adjoining bytes within the word ok.
 * (minimum unit of io is 4 bytes)
 */
static bool_t
xdr_NCvbyte(XDR *xdrs, unsigned rem, unsigned count, char *value) 
{
  char buf[4] ;
  u_int origin ;
  enum xdr_op  x_op = xdrs->x_op ; /* save state */

  if(x_op == XDR_ENCODE)
  {
  /*
   * Since we only read/write multiples of four bytes,
   * We will read in the word to change one byte in it.
   */
    origin = xdr_getpos( xdrs ) ;
    /* next op is a get */
    xdrs->x_op = XDR_DECODE  ;
  }

  if(!xdr_opaque(xdrs, buf, 4))
  {
    /* get failed, assume we are trying to read off the end */
    (void)memset(buf, 0, sizeof(buf)) ;
  }

  if(x_op == XDR_ENCODE) /* back to encode */
    xdrs->x_op = x_op ;

  while(count-- != 0)
  {
    if(x_op == XDR_ENCODE)
      buf[rem] = *value ;
    else
      *value = buf[rem] ;
  
    rem++ ;
    value++ ;
  }

  if(x_op == XDR_ENCODE)
  {
    if( !xdr_setpos(xdrs, origin) )
      return(FALSE) ;
    if( !xdr_opaque(xdrs, buf, 4))
      return(FALSE) ;
  }

  return(TRUE) ;
}


/* xdrshorts.c */
/* you may wish to tune this: big on a cray, small on a PC? */
#define NC_SHRT_BUFSIZ 8192
#define NC_NSHRTS_PER (NC_SHRT_BUFSIZ/2) /* number of netshorts the buffer holds */

/*
 * xdr a short leaving adjoining short within the word ok.
 * (minimum unit of io is 4 bytes)
 */
static bool_t
xdr_NCvshort(XDR *xdrs, unsigned which, short *value)
{
  unsigned char buf[4] ; /* unsigned is important here */
  u_int origin ;
  enum xdr_op  x_op = xdrs->x_op ; /* save state */

  if(x_op == XDR_ENCODE)
  {
    origin = xdr_getpos( xdrs ) ;
    /* next op is a get */
    xdrs->x_op = XDR_DECODE  ;
  }

  if(!xdr_opaque(xdrs, (caddr_t)buf, 4))
  {
    /* get failed, assume we are trying to read off the end */
    (void)memset(buf, 0, sizeof(buf)) ;
  }

  if(x_op == XDR_ENCODE) /* back to encode */
    xdrs->x_op = x_op ;

  if(which != 0) which = 2 ;

  if(xdrs->x_op == XDR_ENCODE)
  {
    buf[which +1] = *value % 256 ;
    buf[which] = (*value >> 8) ;

    if( !xdr_setpos(xdrs, origin) )
      return(FALSE) ;
    if( !xdr_opaque(xdrs, (caddr_t)buf, 4))
      return(FALSE) ;
  }
  else
  {
    *value = (((unsigned)buf[which] & 0x7f) << 8) +
       (unsigned)buf[which + 1] ;
    if((unsigned)buf[which] & 0x80)
    {
      /* extern is neg */
      *value -= 0x8000 ;
    }
  }
  return(TRUE) ;
}

/*
 * internal function, bulk xdr of an even number of shorts, less than NC_NSHRTS_PER
 */
static
bool_t
NCxdr_shortsb(XDR *xdrs, short *sp, u_int nshorts)
{
  unsigned char buf[NC_SHRT_BUFSIZ] ;
  unsigned char *cp ;
  unsigned int nbytes = nshorts * 2;

  /* assert(nshorts <= NC_NSHRTS_PER) ; */
  /* assert(nshorts > 0) ; */

  if(xdrs->x_op == XDR_ENCODE)
  {
    for(cp = buf ; cp < &buf[nbytes] ; sp++, cp += 2 )
    {
      *(cp +1) = *sp % 256 ;
      *cp = (*sp >> 8) ;
    }
  }

  if(!xdr_opaque(xdrs, (caddr_t)buf, nbytes))
    return FALSE ;
  
  if(xdrs->x_op == XDR_DECODE)
  {
    for(cp = buf ; cp < &buf[nbytes] ; sp++, cp += 2 )
    {
      *sp = (((unsigned)*cp & 0x7f) << 8) +
         (unsigned)*(cp +1) ;
      if((unsigned)*cp & 0x80)
      {
        /* extern is neg */
        *sp -= 0x8000 ;
      }
    }
  }

  return TRUE ;
}


/*
 * Translate an array of cnt short integers at sp.
 */
bool_t
xdr_shorts(XDR *xdrs, short *sp, u_int cnt)
{
  int odd ; /* 1 if cnt is odd, 0 otherwise */

  if(cnt == 0)
    return TRUE ;  /* ? */

  odd = cnt % 2 ;
  if(odd) 
    cnt-- ;
  /* cnt is even, odd is set if apropos */

  while(cnt > NC_NSHRTS_PER)
  {
    if(!NCxdr_shortsb(xdrs, sp, NC_NSHRTS_PER))
      return FALSE ;
    /* else */
    sp += NC_NSHRTS_PER ;
    cnt -= NC_NSHRTS_PER ;
  }

  /* we know cnt <= NC_NSHRTS_PER at this point */

  if(cnt != 0)
  {
    if(!NCxdr_shortsb(xdrs, sp, cnt))
      return FALSE ;
    /* else */
    sp += cnt ;
    cnt = 0 ;
  }

  if(odd)
    if(!xdr_NCvshort(xdrs, 0, sp))
      return FALSE ;

  return TRUE ;
}

/*
 * Use standard xdr interface (plus the netcdf xdr_shorts())
 * to encode data to 'buf'
 * Returns 0 on success.
 */
static int
xdr_encode(char *buf, u_int sz)
{
  XDR xdrs[1];
  u_int pos;
  int ii;

  xdrmem_create(xdrs, buf, sz, XDR_ENCODE);

  if(!xdr_opaque(xdrs, (caddr_t)text, (u_int)sizeof(text)))
    return 1;

  if(!xdr_opaque(xdrs, (caddr_t)schars, (u_int)sizeof(schars)))
    return 2;

  if(!xdr_shorts(xdrs, shorts, uiArraySize(shorts)))
    return 3;

  if(!xdr_vector(xdrs, (char *)ints,
      uiArraySize(ints), eSizeOf(ints),
      (xdrproc_t)xdr_int))
    return 4;

  /* double the ints to check both ncx_ interfaces */
  if(!xdr_vector(xdrs, (char *)ints,
      uiArraySize(ints), eSizeOf(ints),
      (xdrproc_t)xdr_int))
    return 5;

#ifndef NO_UNSIGNED
  if(!xdr_vector(xdrs, (char *)u_ints,
      uiArraySize(u_ints), eSizeOf(u_ints),
      (xdrproc_t)xdr_u_int))
    return 6;
#endif

  if(!xdr_vector(xdrs, (char *)longs,
      uiArraySize(longs), eSizeOf(longs),
      (xdrproc_t)xdr_long))
    return 7;

#ifndef NO_UNSIGNED_LONG
  if(!xdr_vector(xdrs, (char *)u_longs,
      uiArraySize(u_longs), eSizeOf(u_longs),
      (xdrproc_t)xdr_u_long))
    return 9;
#endif

  if(!xdr_vector(xdrs, (char *)floats,
      uiArraySize(floats), eSizeOf(floats),
      (xdrproc_t)xdr_float))
    return 10;

  if(!xdr_vector(xdrs, (char *)doubles,
      uiArraySize(doubles), eSizeOf(doubles),
      (xdrproc_t)xdr_double))
    return 11;

  /* mix it up */
  for(ii = 1; ii < 5; ii++)
  {
    if(
        !xdr_opaque(xdrs, (caddr_t)text, ii)
        || !xdr_shorts(xdrs, shorts, ii)
        || !xdr_opaque(xdrs, (caddr_t)schars, ii)
    )
      return (11 + ii);
  }

  /*
   * Test non-aligned unit ops used by netcdf.
   */

  for(ii = 1; ii < 5; ii++)
  {
    pos = xdr_getpos(xdrs);
    if(!xdr_NCvbyte(xdrs, ii, BYTES_PER_XDR_UNIT -ii, &text[ii]))
      return (15 + ii);
    if(!xdr_setpos(xdrs, pos + BYTES_PER_XDR_UNIT))
      return (15 + ii);
  }

  for(ii = 1; ii < 5; ii++)
  {
    pos = xdr_getpos(xdrs);
    if(!xdr_NCvbyte(xdrs, ii, BYTES_PER_XDR_UNIT -ii,
        (char *)&schars[ii]))
      return (19 + ii);
    if(!xdr_setpos(xdrs, pos + BYTES_PER_XDR_UNIT))
      return (18 + ii);
  }

  for(ii = 1; ii < 3; ii++)
  {
    pos = xdr_getpos(xdrs);
    if(!xdr_NCvshort(xdrs, ii%2, &shorts[ii]))
      return (23 + ii);
    if(!xdr_setpos(xdrs, pos + BYTES_PER_XDR_UNIT))
      return (23 + ii);
  }

  pos = xdr_getpos(xdrs);
  (void) printf("xdr_encode ends at byte %u\n", pos);

  return 0;
}


static int
cmp_chars(const char *c1, const char *c2, size_t nchars)
{
  int status = 0;
  const char *const end = c1 + nchars;

  while(c1 < end)
  {
    if(*c1 != *c2)
    {
      (void) fprintf(stderr,
          "%c != %c char\n",
          *c1,
          *c2);
      if(status == 0)
        status = *c2 < *c1 ? -1 : 1;
    }
    c1++, c2++;
  }

  return status;
}

static int
cmp_schars(const schar *b1, const schar *b2, size_t nbytes)
{
  int status = 0;
  const schar *const end = b1 + nbytes;

  while(b1 < end)
  {
    if(*b1 != *b2)
    {
      (void) fprintf(stderr,
          "0x%02x != 0x%02x byte\n",
          (unsigned)(*b1),
          (unsigned)(*b2));
        
      if(status == 0)
        status = *b2 < *b1 ? -1 : 1;
    }
    b1++, b2++;
  }

  return status;
}

static int
cmp_shorts(const short *s1, const short *s2, size_t nshorts)
{
  int status = 0;
  const short *const end = s1 + nshorts;

  while(s1 < end)
  {
    if(*s1 != *s2)
    {
      (void) fprintf(stderr,
          "0x%04x != 0x%04x (%hd) short\n",
          (unsigned)(*s1),
          (unsigned)(*s2), *s2);
      if(status == 0)
        status = *s2 < *s1 ? -1 : 1;
    }
    s1++, s2++;
  }

  return status;
}

static int
cmp_ints(const int *i1, const int *i2, size_t nints)
{
  int status = 0;
  const int *const end = i1 + nints;

  while(i1 < end)
  {
    if(*i1 != *i2)
    {
      (void) fprintf(stderr,
          "0x%08x != 0x%08x int\n",
          (unsigned)(*i1),
          (unsigned)(*i2));
      if(status == 0)
        status = *i2 < *i1 ? -1 : 1;
    }
    i1++, i2++;
  }

  return status;
}

#ifndef NO_UNSIGNED
static int
cmp_u_ints(const unsigned int *i1, const unsigned int *i2, size_t nints)
{
  int status = 0;
  const unsigned int *const end = i1 + nints;

  while(i1 < end)
  {
    if(*i1 != *i2)
    {
      (void) fprintf(stderr,
          "(%u) 0x%08x != 0x%08x (%u) uint\n",
          *i1, *i1,
          *i2, *i2);
      if(status == 0)
        status = *i2 < *i1 ? -1 : 1;
    }
    i1++, i2++;
  }

  return status;
}
#endif

static int
cmp_longs(const long *l1, const long *l2, size_t nlongs)
{
  int status = 0;
  const long *const end = l1 + nlongs;

  while(l1 < end)
  {
    if(*l1 != *l2)
    {
      (void) fprintf(stderr,
          "0x%016lx != 0x%016lx long\n",
          (unsigned long)(*l1),
          (unsigned long)(*l2));
      if(status == 0)
        status = *l2 < *l1 ? -1 : 1;
    }
    l1++, l2++;
  }

  return status;
}

#ifndef NO_UNSIGNED_LONG
static int
cmp_u_longs(const unsigned long *l1, const unsigned long *l2, size_t nlongs)
{
  int status = 0;
  const unsigned long *const end = l1 + nlongs;

  while(l1 < end)
  {
    if(*l1 != *l2)
    {
      (void) fprintf(stderr,
          "0x%016lx != 0x%016lx ulong\n",
          *l1,
          *l2);
      if(status == 0)
        status = *l2 < *l1 ? -1 : 1;
    }
    l1++, l2++;
  }

  return status;
}
#endif

static int
cmp_floats(const float *f1, const float *f2, size_t nfloats)
{
#define F_EPS 1.0e-6

  int status = 0;
  const float *const end = f1 + nfloats;

  while(f1 < end)
  {
    if(*f1 < *f2 && *f2 - *f1 > F_EPS)
    {
      (void) fprintf(stderr,
          "%.9e != %.9e float (diff %.9e)\n",
          *f1, *f2, *f1 - *f2);
      if(status == 0)
        status = 1;
    }
    else if( *f2 < *f1 && *f1 - *f2 > F_EPS)
    {
      (void) fprintf(stderr,
          "%.9e != %.9e float (diff %.9e)\n",
          *f1, *f2, *f1 - *f2);
      if(status == 0)
        status = -1;
    }
    f1++, f2++;
  }

  return status;
}

static int
cmp_doubles(const double *d1, const double *d2, size_t ndoubles)
{
#define D_EPS 1.0e-15

  int status = 0;
  const double *const end = d1 + ndoubles;

  while(d1 < end)
  {
    if(*d1 < *d2 && *d2 - *d1 > D_EPS)
    {
      (void) fprintf(stderr,
          "%.17e != %.17e double (diff %.17e)\n",
          *d1, *d2, *d1 - *d2);
      if(status == 0)
        status = 1;
    }
    else if( *d2 < *d1 && *d1 - *d2 > D_EPS)
    {
      (void) fprintf(stderr,
          "%.17e != %.17e double (diff %.17e)\n",
          *d1, *d2, *d1 - *d2);
      if(status == 0)
        status = -1;
    }
    d1++, d2++;
  }

  return status;
}

/*
 * Verify that data in buf is as encoded
 * by xdr_encode() above.
 * Returns zero on sucess.
 */
static int
xdr_check(char *buf, u_int sz)
{
  XDR xdrs[1];
  char tbuf[XBSZ];
  u_int pos;
  int ii;
  int jj;

  xdrmem_create(xdrs, buf, sz, XDR_DECODE);

  (void) memset(tbuf, 0, sizeof(text)+4);
  if(!xdr_opaque(xdrs, (caddr_t)tbuf, (u_int)sizeof(text))
      || cmp_chars(tbuf, text,
         sizeof(text)) != 0)
    return 1;

  (void) memset(tbuf, 0, sizeof(schars)+4);
  if(!xdr_opaque(xdrs, (caddr_t)tbuf, (u_int)sizeof(schars))
      || cmp_schars((schar *)tbuf, schars,
        sizeof(schars)) != 0)
    return 2;

  (void) memset(tbuf, 0, sizeof(shorts)+4);
  if(!xdr_shorts(xdrs, (short *)tbuf, uiArraySize(shorts))
      || cmp_shorts((short *)tbuf, shorts,
         ArraySize(shorts)) != 0)
    return 3;

  (void) memset(tbuf, 0, sizeof(ints)+4);
  if(!xdr_vector(xdrs, tbuf,
    uiArraySize(ints), eSizeOf(ints),
    (xdrproc_t)xdr_int)
      || cmp_ints((int *)tbuf, ints,
         ArraySize(ints)) != 0)
    return 4;

  /* double the ints to check both ncx_ interfaces */
  (void) memset(tbuf, 0, sizeof(ints)+4);
  if(!xdr_vector(xdrs, tbuf,
    uiArraySize(ints), eSizeOf(ints),
    (xdrproc_t)xdr_int)
      || cmp_ints((int *)tbuf, ints,
         ArraySize(ints)) != 0)
    return 5;

#ifndef NO_UNSIGNED
  (void) memset(tbuf, 0, sizeof(u_ints)+4);
  if(!xdr_vector(xdrs, tbuf,
    uiArraySize(u_ints), eSizeOf(u_ints),
    (xdrproc_t)xdr_u_int)
      || cmp_u_ints((unsigned int *)tbuf, u_ints,
        ArraySize(u_ints)) != 0)
    return 6;
#endif

  (void) memset(tbuf, 0, sizeof(longs)+4);
  if(!xdr_vector(xdrs, tbuf,
    uiArraySize(longs), eSizeOf(longs), (xdrproc_t)xdr_long)
      || cmp_longs((long *)tbuf, longs,
         ArraySize(longs)) != 0)
    return 7;

#ifndef NO_UNSIGNED_LONG
  (void) memset(tbuf, 0, sizeof(u_longs)+4);
  if(!xdr_vector(xdrs, tbuf,
    uiArraySize(u_longs), eSizeOf(u_longs), (xdrproc_t)xdr_u_long)
      || cmp_u_longs((unsigned long *)tbuf, u_longs,
        ArraySize(u_longs)) != 0)
    return 9;
#endif

  (void) memset(tbuf, 0, sizeof(floats)+4);
  if(!xdr_vector(xdrs, tbuf,
    uiArraySize(floats), eSizeOf(floats), (xdrproc_t)xdr_float)
      || cmp_floats((float *)tbuf, floats,
         ArraySize(floats)) != 0)
    return 10;

  (void) memset(tbuf, 0, sizeof(doubles)+4);
  if(!xdr_vector(xdrs, tbuf,
    uiArraySize(doubles), eSizeOf(doubles), (xdrproc_t)xdr_double)
      || cmp_doubles((double *)tbuf, doubles,
         ArraySize(doubles)) != 0)
    return 11;

  for(ii = 1; ii < 5; ii++)
  {
    char tx[4];
    short sh[4];
    schar by[4];
    if(
        !xdr_opaque(xdrs, (caddr_t)tx, ii)
        || !xdr_shorts(xdrs, sh, ii)
        || !xdr_opaque(xdrs, (caddr_t)by, ii)
    )
      return (11 + ii);
    for(jj = 0; jj < ii; jj++)
    {
      if(tx[jj] != text[jj])
      {
        (void) fprintf(stderr, "\txdr %c != %c text[%d]\n",
            tx[jj], text[jj], jj);
        return (11 + ii);
      }
      /* else */
      if(sh[jj] != shorts[jj])
      {
        (void) fprintf(stderr, "\txdr %hd != %hd shorts[%d]\n",
            sh[jj], shorts[jj], jj);
        return (11 + ii);
      }
      /* else */
      if(by[jj] != schars[jj])
      {
        (void) fprintf(stderr,
          "\txdr 0x%02x != 0x%02x schars[%d]\n",
            (unsigned) by[jj],
            (unsigned) schars[jj], jj);
        return (11 + ii);
      }
      /* else */
    }
  }

  /*
   * Test non-aligned unit ops used by netcdf.
   */

  for(ii = 1; ii < 5; ii++)
  {
    pos = xdr_getpos(xdrs);
    (void) memset(tbuf, 0, BYTES_PER_XDR_UNIT);
    if(!xdr_NCvbyte(xdrs, ii, BYTES_PER_XDR_UNIT -ii, tbuf)
        || cmp_chars(&text[ii], tbuf,
          BYTES_PER_XDR_UNIT -ii) != 0)
      return (15 + ii);
    if(!xdr_setpos(xdrs, pos + BYTES_PER_XDR_UNIT))
      return (15 + ii);
  }

  for(ii = 1; ii < 5; ii++)
  {
    pos = xdr_getpos(xdrs);
    (void) memset(tbuf, 0, BYTES_PER_XDR_UNIT);
    if(!xdr_NCvbyte(xdrs, ii, BYTES_PER_XDR_UNIT -ii, tbuf)
        || cmp_schars((schar *)tbuf, &schars[ii],
          BYTES_PER_XDR_UNIT -ii) != 0)
      return (19 + ii);
    if(!xdr_setpos(xdrs, pos + BYTES_PER_XDR_UNIT))
      return (19 + ii);
  }

  for(ii = 1; ii < 3; ii++)
  {
    pos = xdr_getpos(xdrs);
    (void) memset(tbuf, 0, BYTES_PER_XDR_UNIT);
    if(!xdr_NCvshort(xdrs, ii%2, (short *)tbuf)
        || cmp_shorts((short *)tbuf, &shorts[ii], 1))
      return (23 + ii);
    if(!xdr_setpos(xdrs, pos + BYTES_PER_XDR_UNIT))
      return (23 + ii);
  }

  pos = xdr_getpos(xdrs);
  (void) printf("xdr_check  ends at byte %u\n", pos);

  return 0;
}


/* Poor man's template */
#define NCX_VEC(xpp, nn, vecp, TYPE, proc, step) \
{ \
\
  size_t nelems = (nn); \
  TYPE *elemp = (vecp); \
\
  while(nelems != 0) \
  { \
    status = (proc)((*(xpp)), elemp); \
    if(status != ENOERR) \
      break; \
    (*(xpp)) = (void *)((char *)(*(xpp)) + (step)); \
    elemp ++; \
    nelems--; \
  } \
}


/*
 * Use ncx interface
 * to encode data to 'buf'
 * Returns zero on success.
 */
static int
ncx_encode(char *buf)
{
  int status = ENOERR;

  void *vp = buf;
  int ii;

  if(ncx_pad_putn_text(&vp, sizeof(text), text))
    return 1;

  if(ncx_pad_putn_schar_schar(&vp, sizeof(schars), schars))
    return 2;

  if(ncx_pad_putn_short_short(&vp, ArraySize(shorts), shorts))
    return 3;

  if(ncx_putn_int_int(&vp, ArraySize(ints), ints))
    return 4;

  NCX_VEC(&vp, ArraySize(ints), ints,
       int, ncx_put_int_int, X_SIZEOF_INT);
  if(status != ENOERR)
    return 5;

#ifndef NO_UNSIGNED
  NCX_VEC(&vp, ArraySize(u_ints), u_ints,
      unsigned int, ncx_put_uint_uint, X_SIZEOF_INT);
  if(status != ENOERR)
    return 6;
#endif

  if(ncx_putn_int_long(&vp, ArraySize(longs), longs))
    return 7;

#ifndef NO_UNSIGNED_LONG
  NCX_VEC(&vp, ArraySize(u_longs), u_longs,
      unsigned long, ncx_put_ulong_ulong, X_SIZEOF_LONG);
  if(status != ENOERR)
    return 9;
#endif

  if(ncx_putn_float_float(&vp, ArraySize(floats), floats))
    return 10;

  if(ncx_putn_double_double(&vp, ArraySize(doubles), doubles))
    return 11;

  /* mix it up */
  for(ii = 1; ii < 5; ii++)
  {
    if(
        ncx_pad_putn_text(&vp, ii, text)
        || ncx_pad_putn_short_short(&vp, ii, shorts)
        || ncx_pad_putn_schar_schar(&vp, ii, schars)
    )
      return (11 + ii);
  }

  /*
   * Test non-aligned unit ops used by netcdf.
   */

  for(ii = 1; ii < 5; ii++)
  {
    vp = (char *)vp + ii;
    if(ncx_putn_text(&vp, X_ALIGN - ii, &text[ii]))
      return (15 + ii);
  }

  for(ii = 1; ii < 5; ii++)
  {
    vp = (char *)vp + ii;
    if(ncx_putn_schar_schar(&vp, X_ALIGN - ii, &schars[ii]))
      return (19 + ii);
  }

  for(ii = 1; ii < 3; ii++)
  {
    char *pos = vp;
    vp = (char *)vp + (ii%2) * 2;
    if(ncx_putn_short_short(&vp, 1, &shorts[ii]))
      return (23 + ii);
    vp = pos + X_ALIGN;
  }

  (void) printf("ncx_encode ends at byte %u\n",
     (unsigned)(((char *)vp) - buf));

  return 0;
}

/* 
 * Verify the ncx_getn_xxx() routines.
 * Returns zero on success.
 */
static int
ncx_check(char *buf)
{
  int status = ENOERR;
  const void *vp = buf;
  char tbuf[XBSZ];
  int ii;
  int jj;

  (void) memset(tbuf, 0, sizeof(text)+4);
  if(ncx_pad_getn_text(&vp, sizeof(text), tbuf)
      || cmp_chars(tbuf, text,
         sizeof(text)) != 0)
    return 1;

  (void) memset(tbuf, 0, sizeof(schars)+4);
  if(ncx_pad_getn_schar_schar(&vp, sizeof(schars), (schar *)tbuf)
      || cmp_schars((schar *)tbuf, schars,
         sizeof(schars)) != 0)
    return 2;

  (void) memset(tbuf, 0, sizeof(shorts)+4);
  if(ncx_pad_getn_short_short(&vp, ArraySize(shorts), (short *)tbuf)
      || cmp_shorts((short *)tbuf, shorts,
         ArraySize(shorts)) != 0)
    return 3;

  (void) memset(tbuf, 0, sizeof(ints)+4);
  if(ncx_getn_int_int(&vp, ArraySize(ints), (int *)tbuf)
      || cmp_ints((int *)tbuf, ints,
         ArraySize(ints)) != 0)
    return 4;

  (void) memset(tbuf, 0, sizeof(ints)+4);
  NCX_VEC(&vp, ArraySize(ints), (int *)tbuf,
        int, ncx_get_int_int, X_SIZEOF_INT);
  if(status != ENOERR
      || cmp_ints((int *)tbuf, ints,
         ArraySize(ints)) != 0)
    return 5;

#ifndef NO_UNSIGNED
  (void) memset(tbuf, 0, sizeof(u_ints)+4);
  NCX_VEC(&vp, ArraySize(u_ints), (unsigned int *)tbuf,
        unsigned, ncx_get_uint_uint, X_SIZEOF_INT);
  if(status != ENOERR
      || cmp_u_ints((unsigned int *)tbuf, u_ints,
         ArraySize(u_ints)) != 0)
    return 6;
#endif

  (void) memset(tbuf, 0, sizeof(longs)+4);
  if(ncx_getn_int_long(&vp, ArraySize(longs), (long *)tbuf)
      || cmp_longs((long *)tbuf, longs,
         ArraySize(longs)) != 0)
    return 7;

#ifndef NO_UNSIGNED_LONG
  (void) memset(tbuf, 0, sizeof(u_longs)+4);
  NCX_VEC(&vp, ArraySize(u_longs), (unsigned long *)tbuf,
      unsigned long, ncx_get_ulong_ulong, X_SIZEOF_LONG);
  if(status != ENOERR
      || cmp_u_longs((unsigned long *)tbuf, u_longs,
        ArraySize(u_longs)) != 0)
    return 9;
#endif

  (void) memset(tbuf, 0, sizeof(floats)+4);
  if(ncx_getn_float_float(&vp, ArraySize(floats), (float *)tbuf)
      || cmp_floats((float *)tbuf, floats,
         ArraySize(floats)) != 0)
    return 10;

  (void) memset(tbuf, 0, sizeof(doubles)+4);
  if(ncx_getn_double_double(&vp, ArraySize(doubles), (double *)tbuf)
      || cmp_doubles((double *)tbuf, doubles,
         ArraySize(doubles)) != 0)
    return 11;

  for(ii = 1; ii < 5; ii++)
  {
    char tx[4];
    short sh[4];
    schar by[4];
    if(
        ncx_pad_getn_text(&vp, ii, tx)
        || ncx_pad_getn_short_short(&vp, ii, sh)
        || ncx_pad_getn_schar_schar(&vp, ii, by)
    )
      return (11 + ii);
    for(jj = 0; jj < ii; jj++)
    {
      if(tx[jj] != text[jj])
      {
        (void) fprintf(stderr,
          "\tncx %c != %c text[%d]\n",
            tx[jj], text[jj], jj);
        return (11 + ii);
      }
      /* else */
      if(sh[jj] != shorts[jj])
      {
        (void) fprintf(stderr,
           "\tncx %hd != %hd shorts[%d]\n",
            sh[jj], shorts[jj], jj);
        return (11 + ii);
      }
      /* else */
      if((unsigned)by[jj] != (unsigned)schars[jj])
      {
        (void) fprintf(stderr,
          "\tncx 0x%02x != 0x%02x schars[%d] %d\n",
            by[jj], schars[jj], jj, ii);
        return (11 + ii);
      }
    }
  }

  /*
   * Test non-aligned unit ops used by netcdf.
   */

  for(ii = 1; ii < 5; ii++)
  {
    (void) memset(tbuf, 0, X_ALIGN);
    vp = (char *)vp + ii;
    if(ncx_getn_text(&vp, X_ALIGN -ii, tbuf)
        || cmp_chars(tbuf, &text[ii],
           X_ALIGN -ii) != 0)
      return (15 + ii);
  }

  for(ii = 1; ii < 5; ii++)
  {
    (void) memset(tbuf, 0, X_ALIGN);
    vp = (char *)vp + ii;
    if(ncx_getn_schar_schar(&vp, X_ALIGN -ii, (schar *)tbuf)
        || cmp_schars((schar *)tbuf, &schars[ii],
           X_ALIGN -ii) != 0)
      return (19 + ii);
  }

  for(ii = 1; ii < 3; ii++)
  {
    const char *pos = vp;
    (void) memset(tbuf, 0, X_ALIGN);
    vp = (char *)vp + (ii%2) *2;
    if(ncx_getn_short_short(&vp, 1, (short *)tbuf)
        || cmp_shorts((short *)tbuf, &shorts[ii],
           1) != 0)
      return (23 + ii);
    vp = pos + X_ALIGN;
  }

  (void) printf("ncx_check  ends at byte %u\n",
     (unsigned)(((char *)vp) - buf));


  return 0;
}


int
main(int ac, char *av[])
{
  int status;

  status = xdr_encode(xdrb, sizeof(xdrb));
  if(status)
  {
    (void) fprintf(stderr,
       "xdr_encode failed %d\n", status);
    return 1;
  }

  status = xdr_check(xdrb, sizeof(xdrb));
  if(status)
  {
    (void) fprintf(stderr,
       "xdr_check of xdrb failed %d\n", status);
    return 1;
  }

  status = ncx_encode(ncxb);
  if(status)
  {
    (void) fprintf(stderr,
       "ncx_encode failed %d\n", status);
    return 1;
  }

  /* cross check */
  status = xdr_check(ncxb, sizeof(ncxb));
  if(status)
  {
    (void) fprintf(stderr,
       "xdr_check of ncxb failed %d\n", status);
    return 1;
  }

  status = ncx_check(xdrb);
  if(status)
  {
    (void) fprintf(stderr,
       "ncx_check of xdrb failed %d\n", status);
    return 1;
  }

  status = ncx_check(ncxb);
  if(status)
  {
    (void) fprintf(stderr,
       "ncx_check of ncxb failed %d\n", status);
    return 1;
  }

  return 0;
}
