/*
 *   Copyright 1988 University Corporation for Atmospheric Research
 *      See netcdf/COPYRIGHT file for copying and redistribution conditions.
 */
/* $Id: t_nc.c,v 1.91 2009/04/08 17:16:42 ed Exp $ */

/*
 *   Program to create a cdf, exercise all cdf functions.
 *  Creates cdf, stuff it full of numbers, closes it. Then
 *  reopens it, and checks for consistency.
 *  Leaves the file around afterwards.
 *
 *  Based on a program to test the nasa look-alike program,
 * so not the most appropropriate test. See ../nctest for a
 * complete spec test.
 */


#define REDEF
/* #define SYNCDEBUG */

#undef NDEBUG  /* always active assert() in this file */

#include <config.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "rename.h"

#define MAXSHORT  32767
#define MAXINT    2147483647
#define MAXBYTE    127


#define FNAME    "test.nc"
#define  NUM_DIMS   3
#define DONT_CARE  -1
/* make these numbers big when you want to give this a real workout */
#define NUM_RECS  8
#define SIZE_1    7
#define SIZE_2    8

static struct {
  int num_dims;
  int num_vars;
  int num_attrs;
  int xtendim;
} cdesc[1];

static struct {
  char mnem[NC_MAX_NAME];
  nc_type type;
  int ndims;
  int dims[NC_MAX_DIMS];
  int num_attrs;
} vdesc[1];

static struct {
  char mnem[NC_MAX_NAME];
  nc_type type;
  size_t len;
} adesc[1];

union getret
{
    char            by[8];
    short           sh[4];
    int          in[2];
    float           fl[2];
    double          dbl;
};


static void
chkgot(nc_type type, union getret got, double check)
{
  switch(type){
  case NC_BYTE :
    assert( (char)check == got.by[0] );
    break;
  case NC_CHAR :  /* TODO */
    assert( (char)check == got.by[0] );
    break;
  case NC_SHORT :
    assert( (short)check == got.sh[0] );
    break;
  case NC_INT :
    assert( (int)check == got.in[0] );
    break;
  case NC_FLOAT :
    assert( (float)check == got.fl[0] );
    break;
  case NC_DOUBLE :
    assert( check == got.dbl );
    break;
  default:
    break;
  }
}

static const char *fname = FNAME;


static size_t num_dims = NUM_DIMS;
static size_t sizes[] = { NC_UNLIMITED, SIZE_1 , SIZE_2 };
static const char * const dim_names[] = { "record", "ixx", "iyy"};

static void
createtestdims(int cdfid, size_t num_dims, const size_t *sizes, const char * const dim_names[])
{
  int dimid;
  while(num_dims-- != 0)
  {
    assert( nc_def_dim(cdfid, *dim_names++, *sizes, &dimid)
       == NC_NOERR);
    sizes++;
  }

}


static void
testdims(int cdfid, size_t num_dims, size_t *sizes, const char * const dim_names[])
{
  int ii;
  size_t size;
  char cp[NC_MAX_NAME];
  for(ii=0; (size_t) ii < num_dims; ii++, sizes++)
  {
    assert( nc_inq_dim(cdfid, ii, cp, &size) == NC_NOERR);
    if( size != *sizes)
      (void) fprintf(stderr, "%d: %lu != %lu\n",
        ii, (unsigned long)size, (unsigned long)*sizes);
    assert( size == *sizes);
    assert( strcmp(cp, *dim_names++) == 0);
  }

}



static const char * const reqattr[] = {
  "UNITS",
  "VALIDMIN",
  "VALIDMAX",
  "SCALEMIN",
  "SCALEMAX",
  "FIELDNAM",
  _FillValue
};
#define NUM_RATTRS  6

static struct tcdfvar {
  const char *mnem;
  nc_type type;
  const char *fieldnam;
  double validmin;
  double validmax;
  double scalemin;
  double scalemax;
  const char *units;
  int ndims;
  int dims[NUM_DIMS];
} const testvars[]  = {
#define Byte_id 0
  { "Byte", NC_BYTE, "Byte sized integer variable",
    -MAXBYTE, MAXBYTE, -MAXBYTE, MAXBYTE , "ones",
      2, {0,1,DONT_CARE} },
#define Char_id 1
  { "Char", NC_CHAR, "char (string) variable",
    DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE, "(unitless)",
      2, {0,2,DONT_CARE} },
#define Short_id 2
  { "Short", NC_SHORT, "Short variable",
    -MAXSHORT, MAXSHORT, -MAXSHORT, MAXSHORT , "ones",
      2, {0, 2, DONT_CARE }},
#define Long_id 3
  { "Long", NC_INT, "Long Integer variable", /* 2.x backward strings */
    -MAXINT, MAXINT, -MAXINT, MAXINT, "ones",
      2, {1, 2, DONT_CARE}},
#define Float_id 4
  { "Float", NC_FLOAT, "Single Precision Floating Point variable",
    -MAXINT, MAXINT, -MAXINT, MAXINT, "flots",
      3, {0, 1, 2 }},
#define Double_id 5
  { "Double", NC_DOUBLE, "Double Precision Floating Point variable",
    -MAXINT, MAXINT, -MAXINT, MAXINT, "dflots",
      3, {0, 1, 2 }},
};
#define  NUM_TESTVARS  6

static void
createtestvars(int id, const struct tcdfvar *testvars, size_t count)
{
  int ii;
  int varid;
  const struct tcdfvar *vp = testvars;

  for(ii = 0; (size_t) ii < count; ii++, vp++ )
  {
    assert(nc_def_var(id, vp->mnem, vp->type, vp->ndims, vp->dims,
         &varid)
       == NC_NOERR );

     assert(
      nc_put_att_text(id,ii,reqattr[0],strlen(vp->units),
        vp->units)
      == NC_NOERR);
     assert(
      nc_put_att_double(id,ii,reqattr[1],NC_DOUBLE,1,
        &vp->validmin)
      == NC_NOERR);
     assert(
      nc_put_att_double(id,ii,reqattr[2],NC_DOUBLE,1,
        &vp->validmax)
      == NC_NOERR);
     assert(
      nc_put_att_double(id,ii,reqattr[3],NC_DOUBLE,1,
        &vp->scalemin)
      == NC_NOERR);
     assert(
      nc_put_att_double(id,ii,reqattr[4],NC_DOUBLE,1,
        &vp->scalemax)
      == NC_NOERR);
     assert(
      nc_put_att_text(id,ii,reqattr[5],strlen(vp->fieldnam),
        vp->fieldnam)
      == NC_NOERR);
  }
}

static void
parray(const char *label, size_t count, const size_t array[])
{
  (void) fprintf(stdout, "%s", label);
  (void) fputc('\t',stdout);
  for(; count != 0; count--, array++)
    (void) fprintf(stdout," %lu", (unsigned long) *array);
}


static void
fill_seq(int id)
{
  float values[NUM_RECS * SIZE_1 * SIZE_2];
  size_t vindices[NUM_DIMS];

  {
    size_t ii = 0;
    for(; ii < sizeof(values)/sizeof(values[0]); ii++)
    {
      values[ii] = (float) ii;
    }
  }

  /* zero the vindices */
  {
    size_t *cc = vindices;
    while (cc < &vindices[num_dims])
      *cc++ = 0;
  }

  sizes[0] = NUM_RECS;

  assert( nc_put_vara_float(id, Float_id, vindices, sizes, values)== NC_NOERR);

}

static void
check_fill_seq(int id)
{
  size_t vindices[NUM_DIMS];
  size_t *cc, *mm;
  union getret got;
  int ii = 0;
  float val;

  sizes[0] = NUM_RECS;
  cc = vindices;
  while (cc < &vindices[num_dims])
    *cc++ = 0;

  /* ripple counter */
  cc = vindices;
  mm = sizes;
  while (*vindices < *sizes)
  {
      while (*cc < *mm)
      {
    if (mm == &sizes[num_dims - 1])
    {
  if(nc_get_var1_float(id, Float_id, vindices, &got.fl[0]) == -1)
    goto bad_ret;
  val = (float) ii;
  if(val != got.fl[0])
  {
    parray("indices", NUM_DIMS, vindices);
    (void) printf("\t%f != %f\n", val, got.fl[0]);
  }
        (*cc)++; ii++;
        continue;
    }
    cc++;
    mm++;
      }
    if(cc == vindices)
      break;
      *cc = 0;
      cc--;
      mm--;
      (*cc)++;
  }
  return;
bad_ret :
  (void) printf("couldn't get a var in check_fill_seq() %d\n",
    ii);
  return;
}

static const size_t  indices[][3] = {
  {0, 1, 3},
  {0, 3, 0},
  {1, 2, 3},
  {3, 2, 1},
  {2, 1, 3},
  {1, 0, 0},
  {0, 0, 0},
};

static const char chs[] = {'A','B', ((char)0xff) };
static const size_t s_start[] = {0,1};
static const size_t s_edges[] = {NUM_RECS, SIZE_1 - 1};
static char sentence[NUM_RECS* SIZE_1 -1] =
  "The red death had long devastated the country.";
static const short shs[] = {97, 99};
static const int birthday = 82555;
#define M_E  2.7182818284590452354
static const float e = (float) M_E;
static const double pinot = 3.25;
static const double zed = 0.0;


/*ARGSUSED*/
int
main(int ac, char *av[])
{
  int ret;
  int   id;
  char buf[256];
#ifdef SYNCDEBUG
  char *str = "one";
#endif
  int ii;
  size_t ui;
  const struct tcdfvar *tvp = testvars;
  union getret got;
  const size_t initialsz = 8192;
  size_t chunksz = 8192;
  size_t align = 8192/32;

  ret = nc__create(fname,NC_NOCLOBBER, initialsz, &chunksz, &id);
  if(ret != NC_NOERR) {
    (void) fprintf(stderr, "trying again\n");
    ret = nc__create(fname,NC_CLOBBER, initialsz, &chunksz, &id);
  }
  if(ret != NC_NOERR)
    exit(ret);

  assert( nc_put_att_text(id, NC_GLOBAL,
    "TITLE", 12, "another name") == NC_NOERR);
  assert( nc_get_att_text(id, NC_GLOBAL,
    "TITLE", buf) == NC_NOERR);
/*  (void) printf("title 1 \"%s\"\n", buf); */
  assert( nc_put_att_text(id, NC_GLOBAL,
    "TITLE", strlen(fname), fname) == NC_NOERR);
  assert( nc_get_att_text(id, NC_GLOBAL,
    "TITLE", buf) == NC_NOERR);
  buf[strlen(fname)] = 0;
/*  (void) printf("title 2 \"%s\"\n", buf); */
  assert( strcmp(fname, buf) == 0);

  createtestdims(id, NUM_DIMS, sizes, dim_names);
  testdims(id, NUM_DIMS, sizes, dim_names);

  createtestvars(id, testvars, NUM_TESTVARS);

   {
   int ifill = -1; double dfill = -9999;
   assert( nc_put_att_int(id, Long_id,
     _FillValue, NC_INT, 1, &ifill) == NC_NOERR);
   assert( nc_put_att_double(id, Double_id,
     _FillValue, NC_DOUBLE, 1, &dfill) == NC_NOERR);
   }

#ifdef REDEF
  assert( nc__enddef(id, 0, align, 0, 2*align) == NC_NOERR );
  assert( nc_put_var1_int(id, Long_id, indices[3], &birthday)
    == NC_NOERR );
  fill_seq(id);
  assert( nc_redef(id) == NC_NOERR );
/*  assert( nc_rename_dim(id,2, "a long dim name") == NC_NOERR); */
#endif

  assert( nc_rename_dim(id,1, "IXX") == NC_NOERR);
  assert( nc_inq_dim(id, 1, buf, &ui) == NC_NOERR);
  (void) printf("dimrename: %s\n", buf);
  assert( nc_rename_dim(id,1, dim_names[1]) == NC_NOERR);

#ifdef ATTRX
  assert( nc_rename_att(id, 1, "UNITS", "units") == NC_NOERR);
  assert( nc_del_att(id, 4, "FIELDNAM")== NC_NOERR);
  assert( nc_del_att(id, 2, "SCALEMIN")== NC_NOERR);
  assert( nc_del_att(id, 2, "SCALEMAX")== NC_NOERR);
#endif /* ATTRX */

  assert( nc__enddef(id, 0, align, 0, 2*align) == NC_NOERR );

#ifndef REDEF
  fill_seq(id);
  assert( nc_put_var1_int(id, Long_id, indices[3], &birthday)== NC_NOERR );
#endif

  assert( nc_put_vara_schar(id, Byte_id, s_start, s_edges,
    (signed char *)sentence)
    == NC_NOERR);
  assert( nc_put_var1_schar(id, Byte_id, indices[6], (signed char *)(chs+1))
    == NC_NOERR);
  assert( nc_put_var1_schar(id, Byte_id, indices[5], (signed char *)chs)
    == NC_NOERR);

  assert( nc_put_vara_text(id, Char_id, s_start, s_edges, sentence)
    == NC_NOERR);
  assert( nc_put_var1_text(id, Char_id, indices[6], (chs+1))
    == NC_NOERR) ;
  assert( nc_put_var1_text(id, Char_id, indices[5], chs)
    == NC_NOERR);

  assert( nc_put_var1_short(id, Short_id, indices[4], shs)
    == NC_NOERR);

  assert( nc_put_var1_float(id, Float_id, indices[2], &e)
    == NC_NOERR);

  assert( nc_put_var1_double(id, Double_id, indices[1], &zed)
    == NC_NOERR);
  assert( nc_put_var1_double(id, Double_id, indices[0], &pinot)
    == NC_NOERR);


#ifdef SYNCDEBUG
  (void) printf("Hit Return to sync\n");
  gets(str);
  nc_sync(id,0);
  (void) printf("Sync done. Hit Return to continue\n");
  gets(str);
#endif /* SYNCDEBUG */

  ret = nc_close(id);
  (void) printf("nc_close ret = %d\n\n", ret);


/*
 *  read it
 */
  ret = nc__open(fname,NC_NOWRITE, &chunksz, &id);
  if(ret != NC_NOERR)
  {
    (void) printf("Could not open %s: %s\n", fname,
      nc_strerror(ret));
    exit(1);
  }
  (void) printf("reopen id = %d for filename %s\n",
    id, fname);

  /*  NC  */
  (void) printf("NC ");
  assert( nc_inq(id, &(cdesc->num_dims), &(cdesc->num_vars),
    &(cdesc->num_attrs), &(cdesc->xtendim) ) == NC_NOERR);
  assert((size_t) cdesc->num_dims == num_dims);
  assert(cdesc->num_attrs == 1);
  assert(cdesc->num_vars == NUM_TESTVARS);
  (void) printf("done\n");

  /*  GATTR  */
  (void) printf("GATTR ");

  assert( nc_inq_attname(id, NC_GLOBAL, 0, adesc->mnem) == 0);
  assert(strcmp("TITLE",adesc->mnem) == 0);
  assert( nc_inq_att(id, NC_GLOBAL, adesc->mnem, &(adesc->type), &(adesc->len))== NC_NOERR);
  assert( adesc->type == NC_CHAR );
  assert( adesc->len == strlen(fname) );
  assert( nc_get_att_text(id, NC_GLOBAL, "TITLE", buf)== NC_NOERR);
  buf[adesc->len] = 0;
  assert( strcmp(fname, buf) == 0);

  /*  VAR  */
  (void) printf("VAR ");
  assert( cdesc->num_vars == NUM_TESTVARS );

  for(ii = 0; ii < cdesc->num_vars; ii++, tvp++ )
  {
    int jj;
    assert( nc_inq_var(id, ii,
      vdesc->mnem,
      &(vdesc->type),
      &(vdesc->ndims),
      vdesc->dims,
      &(vdesc->num_attrs)) == NC_NOERR);
    if(strcmp(tvp->mnem , vdesc->mnem) != 0)
    {
      (void) printf("attr %d mnem mismatch %s, %s\n",
        ii, tvp->mnem, vdesc->mnem);
      continue;
    }
    if(tvp->type != vdesc->type)
    {
      (void) printf("attr %d type mismatch %d, %d\n",
        ii, (int)tvp->type, (int)vdesc->type);
      continue;
    }
    for(jj = 0; jj < vdesc->ndims; jj++ )
    {
      if(tvp->dims[jj] != vdesc->dims[jj] )
      {
    (void) printf(
    "inconsistent dim[%d] for variable %d: %d != %d\n",
    jj, ii, tvp->dims[jj], vdesc->dims[jj] );
      continue;
      }
    }

    /* VATTR */
    (void) printf("VATTR\n");
    for(jj=0; jj<vdesc->num_attrs; jj++ )
    {
      assert( nc_inq_attname(id, ii, jj, adesc->mnem) == NC_NOERR);
      if( strcmp(adesc->mnem, reqattr[jj]) != 0 )
      {
        (void) printf("var %d attr %d mismatch %s != %s\n",
          ii, jj, adesc->mnem, reqattr[jj] );
        break;
      }
    }

    if( nc_inq_att(id, ii, reqattr[0], &(adesc->type), &(adesc->len))
      != -1) {
    assert( adesc->type == NC_CHAR );
    assert( adesc->len == strlen(tvp->units) );
     assert( nc_get_att_text(id,ii,reqattr[0],buf)== NC_NOERR);
    buf[adesc->len] = 0;
    assert( strcmp(tvp->units, buf) == 0);
    }

    if(
      nc_inq_att(id, ii, reqattr[1], &(adesc->type), &(adesc->len))
      != -1)
    {
    assert( adesc->type == NC_DOUBLE );
    assert( adesc->len == 1 );
     assert( nc_get_att_double(id, ii, reqattr[1], &got.dbl)== NC_NOERR);
    chkgot(adesc->type, got, tvp->validmin);
    }

    if(
      nc_inq_att(id, ii, reqattr[2], &(adesc->type), &(adesc->len))
      != -1)
    {
    assert( adesc->type == NC_DOUBLE );
    assert( adesc->len == 1 );
     assert( nc_get_att_double(id, ii, reqattr[2], &got.dbl)== NC_NOERR);
    chkgot(adesc->type, got, tvp->validmax);
    }

    if(
      nc_inq_att(id, ii, reqattr[3], &(adesc->type), &(adesc->len))
      != -1)
    {
    assert( adesc->type == NC_DOUBLE );
    assert( adesc->len ==1 );
     assert( nc_get_att_double(id, ii, reqattr[3], &got.dbl)== NC_NOERR);
    chkgot(adesc->type, got, tvp->scalemin);
    }

    if(
      nc_inq_att(id, ii, reqattr[4], &(adesc->type), &(adesc->len))
      != -1)
    {
    assert( adesc->type == NC_DOUBLE );
    assert( adesc->len == 1 );
     assert( nc_get_att_double(id, ii, reqattr[4], &got.dbl)== NC_NOERR);
    chkgot(adesc->type, got, tvp->scalemax);
    }

    if( nc_inq_att(id, ii, reqattr[5], &(adesc->type), &(adesc->len))== NC_NOERR)
    {
    assert( adesc->type == NC_CHAR );
    assert( adesc->len == strlen(tvp->fieldnam) );
     assert( nc_get_att_text(id,ii,reqattr[5],buf)== NC_NOERR);
    buf[adesc->len] = 0;
    assert( strcmp(tvp->fieldnam, buf) == 0);
    }
  }

  (void) printf("fill_seq ");
  check_fill_seq(id);
  (void) printf("Done\n");

  assert( nc_get_var1_double(id, Double_id, indices[0], &got.dbl)== NC_NOERR);
  (void) printf("got val = %f\n", got.dbl );

  assert( nc_get_var1_double(id, Double_id, indices[1], &got.dbl)== NC_NOERR);
  (void) printf("got val = %f\n", got.dbl );

  assert( nc_get_var1_float(id, Float_id, indices[2], &got.fl[0])== NC_NOERR);
  (void) printf("got val = %f\n", got.fl[0] );

  assert( nc_get_var1_int(id, Long_id, indices[3], &got.in[0])== NC_NOERR);
  (void) printf("got val = %d\n", got.in[0] );

  assert( nc_get_var1_short(id, Short_id, indices[4], &got.sh[0])== NC_NOERR);
  (void) printf("got val = %d\n", got.sh[0] );

  assert( nc_get_var1_text(id, Char_id, indices[5], &got.by[0]) == NC_NOERR);
  (void) printf("got NC_CHAR val = %c (0x%02x) \n",
     got.by[0] , got.by[0]);

  assert( nc_get_var1_text(id, Char_id, indices[6], &got.by[0]) == NC_NOERR);
  (void) printf("got NC_CHAR val = %c (0x%02x) \n",
     got.by[0], got.by[0] );

  (void) memset(buf,0,sizeof(buf));
  assert( nc_get_vara_text(id, Char_id, s_start, s_edges, buf) == NC_NOERR);
  (void) printf("got NC_CHAR val = \"%s\"\n", buf);

  assert( nc_get_var1_schar(id, Byte_id, indices[5],
      (signed char *)&got.by[0])== NC_NOERR);
  (void) printf("got val = %c (0x%02x) \n", got.by[0] , got.by[0]);

  assert( nc_get_var1_schar(id, Byte_id, indices[6],
      (signed char *)&got.by[0])== NC_NOERR);
  (void) printf("got val = %c (0x%02x) \n", got.by[0], got.by[0] );

  (void) memset(buf,0,sizeof(buf));
  assert( nc_get_vara_schar(id, Byte_id, s_start, s_edges,
      (signed char *)buf)== NC_NOERR );
  (void) printf("got val = \"%s\"\n", buf);

  {
    double dbuf[NUM_RECS * SIZE_1 * SIZE_2];
    assert(nc_get_var_double(id, Float_id, dbuf) == NC_NOERR);
    (void) printf("got vals = %f ... %f\n", dbuf[0],
       dbuf[NUM_RECS * SIZE_1 * SIZE_2 -1] );
  }

  ret = nc_close(id);
  (void) printf("re nc_close ret = %d\n", ret);

  return 0;
}
