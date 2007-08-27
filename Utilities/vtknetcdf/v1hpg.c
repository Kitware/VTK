/*
 *  Copyright 1996, University Corporation for Atmospheric Research
 *      See netcdf/COPYRIGHT file for copying and redistribution conditions.
 */
/* Id */

#include "nc.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "rnd.h"
#include "ncx.h"

/*
 * This module defines the external representation
 * of the "header" of a netcdf version one file and
 * the version two variant that uses 64-bit file 
 * offsets instead of the 32-bit file offsets in version 
 * one files.
 * For each of the components of the NC structure,
 * There are (static) ncx_len_XXX(), v1h_put_XXX()
 * and v1h_get_XXX() functions. These define the
 * external representation of the components.
 * The exported entry points for the whole NC structure
 * are built up from these.
 */


/*
 * "magic number" at beginning of file: 0x43444601 (big endian)
 * assert(sizeof(ncmagic) % X_ALIGN == 0);
 */
static const schar ncmagic[] = {'C', 'D', 'F', 0x02};
static const schar ncmagic1[] = {'C', 'D', 'F', 0x01};


/*
 * v1hs == "Version 1 Header Stream"
 *
 * The netcdf file version 1 header is
 * of unknown and potentially unlimited size.
 * So, we don't know how much to get() on
 * the initial read. We build a stream, 'v1hs'
 * on top of ncio to do the header get.
 */
typedef struct v1hs {
  ncio *nciop;
  off_t offset; /* argument to nciop->get() */
  size_t extent;  /* argument to nciop->get() */
  int flags;  /* set to RGN_WRITE for write */
        int version;    /* format variant: NC_FORMAT_CLASSIC or NC_FORMAT_64BIT */
  void *base; /* beginning of current buffer */
  void *pos;  /* current position in buffer */
  void *end;  /* end of current buffer = base + extent */
} v1hs;


/*
 * Release the stream, invalidate buffer
 */
static int
rel_v1hs(v1hs *gsp)
{
  int status;
  if(gsp->offset == OFF_NONE || gsp->base == NULL)
    return ENOERR;
  status = gsp->nciop->rel(gsp->nciop, gsp->offset,
       gsp->flags == RGN_WRITE ? RGN_MODIFIED : 0);
  gsp->end = NULL;
  gsp->pos = NULL;
  gsp->base = NULL;
  return status;
}


/*
 * Release the current chunk and get the next one.
 * Also used for initialization when gsp->base == NULL.
 */
static int
fault_v1hs(v1hs *gsp, size_t extent)
{
  int status;

  if(gsp->base != NULL)
  {
    const ptrdiff_t incr = (char *)gsp->pos - (char *)gsp->base;
    status = rel_v1hs(gsp);
    if(status)
      return status;
    gsp->offset += incr;
  }
  
  if(extent > gsp->extent)
    gsp->extent = extent; 

  status = gsp->nciop->get(gsp->nciop,
      gsp->offset, gsp->extent,
      gsp->flags, &gsp->base);
  if(status)
    return status;

  gsp->pos = gsp->base;
  gsp->end = (char *)gsp->base + gsp->extent;

  return ENOERR;
}


/*
 * Ensure that 'nextread' bytes are available.
 */
static int
check_v1hs(v1hs *gsp, size_t nextread)
{

#if 0 /* DEBUG */
fprintf(stderr, "nextread %lu, remaining %lu\n",
  (unsigned long)nextread,
  (unsigned long)((char *)gsp->end - (char *)gsp->pos));
#endif

  if((char *)gsp->pos + nextread <= (char *)gsp->end)
    return ENOERR;
  return fault_v1hs(gsp, nextread);
}

/* End v1hs */

/* Write a size_t to the header */
static int
v1h_put_size_t(v1hs *psp, const size_t *sp)
{
  int status = check_v1hs(psp, X_SIZEOF_SIZE_T);
  if(status != ENOERR)
    return status;
  return ncx_put_size_t(&psp->pos, sp);
}


/* Read a size_t from the header */
static int
v1h_get_size_t(v1hs *gsp, size_t *sp)
{
  int status = check_v1hs(gsp, X_SIZEOF_SIZE_T);
  if(status != ENOERR)
    return status;
  return ncx_get_size_t((const void **)(&gsp->pos), sp);
}


/* Begin nc_type */

#define X_SIZEOF_NC_TYPE X_SIZEOF_INT

/* Write a nc_type to the header */
static int
v1h_put_nc_type(v1hs *psp, const nc_type *typep)
{
  const int itype = (int) *typep;
  int status = check_v1hs(psp, X_SIZEOF_INT);
  if(status != ENOERR)
    return status;
  status =  ncx_put_int_int(psp->pos, &itype);
  psp->pos = (void *)((char *)psp->pos + X_SIZEOF_INT);
  return status;
}


/* Read a nc_type from the header */
static int
v1h_get_nc_type(v1hs *gsp, nc_type *typep)
{
  int type = 0;
  int status = check_v1hs(gsp, X_SIZEOF_INT);
  if(status != ENOERR)
    return status;
  status =  ncx_get_int_int(gsp->pos, &type);
  gsp->pos = (void *)((char *)gsp->pos + X_SIZEOF_INT);
  if(status != ENOERR)
    return status;

  assert(type == NC_BYTE
    || type == NC_CHAR
    || type == NC_SHORT
    || type == NC_INT
    || type == NC_FLOAT
    || type == NC_DOUBLE);

  /* else */
  *typep = (nc_type) type;

  return ENOERR;
}

/* End nc_type */
/* Begin NCtype (internal tags) */

#define X_SIZEOF_NCTYPE X_SIZEOF_INT

/* Write a NCtype to the header */
static int
v1h_put_NCtype(v1hs *psp, NCtype type)
{
  const int itype = (int) type;
  int status = check_v1hs(psp, X_SIZEOF_INT);
  if(status != ENOERR)
    return status;
  status = ncx_put_int_int(psp->pos, &itype);
  psp->pos = (void *)((char *)psp->pos + X_SIZEOF_INT);
  return status;
}

/* Read a NCtype from the header */
static int
v1h_get_NCtype(v1hs *gsp, NCtype *typep)
{
  int type = 0;
  int status = check_v1hs(gsp, X_SIZEOF_INT);
  if(status != ENOERR)
    return status;
  status =  ncx_get_int_int(gsp->pos, &type);
  gsp->pos = (void *)((char *)gsp->pos + X_SIZEOF_INT);
  if(status != ENOERR)
    return status;
  /* else */
  *typep = (NCtype) type;
  return ENOERR;
}

/* End NCtype */
/* Begin NC_string */

/*
 * How much space will the xdr'd string take.
 * Formerly
NC_xlen_string(cdfstr)
 */
static size_t
ncx_len_NC_string(const NC_string *ncstrp)
{
  size_t sz = X_SIZEOF_SIZE_T; /* nchars */

  assert(ncstrp != NULL);

  if(ncstrp->nchars != 0) 
  {
#if 0
    assert(ncstrp->nchars % X_ALIGN == 0);
    sz += ncstrp->nchars;
#else
    sz += _RNDUP(ncstrp->nchars, X_ALIGN);
#endif
  }
  return sz;
}


/* Write a NC_string to the header */
static int
v1h_put_NC_string(v1hs *psp, const NC_string *ncstrp)
{
  int status;

#if 0
  assert(ncstrp->nchars % X_ALIGN == 0);
#endif

  status = v1h_put_size_t(psp, &ncstrp->nchars);
  if(status != ENOERR)
    return status;
  status = check_v1hs(psp, _RNDUP(ncstrp->nchars, X_ALIGN));
  if(status != ENOERR)
    return status;
  status = ncx_pad_putn_text(&psp->pos, ncstrp->nchars, ncstrp->cp);
  if(status != ENOERR)
    return status;

  return ENOERR;
}


/* Read a NC_string from the header */
static int
v1h_get_NC_string(v1hs *gsp, NC_string **ncstrpp)
{
  int status;
  size_t nchars = 0;
  NC_string *ncstrp;

  status = v1h_get_size_t(gsp, &nchars);
  if(status != ENOERR)
    return status;

  ncstrp = new_NC_string(nchars, NULL);
  if(ncstrp == NULL)
  {
    return NC_ENOMEM;
  }


#if 0
/* assert(ncstrp->nchars == nchars || ncstrp->nchars - nchars < X_ALIGN); */
  assert(ncstrp->nchars % X_ALIGN == 0);
  status = check_v1hs(gsp, ncstrp->nchars);
#else
  
  status = check_v1hs(gsp, _RNDUP(ncstrp->nchars, X_ALIGN));
#endif
  if(status != ENOERR)
    goto unwind_alloc;

  status = ncx_pad_getn_text((const void **)(&gsp->pos),
     nchars, ncstrp->cp);
  if(status != ENOERR)
    goto unwind_alloc;

  *ncstrpp = ncstrp;

  return ENOERR;

unwind_alloc:
  free_NC_string(ncstrp);
  return status;
  
}

/* End NC_string */
/* Begin NC_dim */

/*
 * How much space will the xdr'd dim take.
 * Formerly
NC_xlen_dim(dpp)
 */
static size_t
ncx_len_NC_dim(const NC_dim *dimp)
{
  size_t sz;

  assert(dimp != NULL);

  sz = ncx_len_NC_string(dimp->name);
  sz += X_SIZEOF_SIZE_T;

  return(sz);
}


/* Write a NC_dim to the header */
static int
v1h_put_NC_dim(v1hs *psp, const NC_dim *dimp)
{
  int status;

  status = v1h_put_NC_string(psp, dimp->name);
  if(status != ENOERR)
    return status;

  status = v1h_put_size_t(psp, &dimp->size);
  if(status != ENOERR)
    return status;

  return ENOERR;
}

/* Read a NC_dim from the header */
static int
v1h_get_NC_dim(v1hs *gsp, NC_dim **dimpp)
{
  int status;
  NC_string *ncstrp;
  NC_dim *dimp;

  status = v1h_get_NC_string(gsp, &ncstrp);
  if(status != ENOERR)
    return status;

  dimp = new_x_NC_dim(ncstrp);
  if(dimp == NULL)
  {
    status = NC_ENOMEM;
    goto unwind_name;
  }

  status = v1h_get_size_t(gsp, &dimp->size);
  if(status != ENOERR)
  {
    free_NC_dim(dimp); /* frees name */
    return status;
  }

  *dimpp = dimp;

  return ENOERR;

unwind_name:
  free_NC_string(ncstrp);
  return status;
}


/* How much space in the header is required for this NC_dimarray? */
static size_t
ncx_len_NC_dimarray(const NC_dimarray *ncap)
{
  size_t xlen = X_SIZEOF_NCTYPE;  /* type */
  xlen += X_SIZEOF_SIZE_T;  /* count */
  if(ncap == NULL)
    return xlen;
  /* else */
  {
    const NC_dim **dpp = (const NC_dim **)ncap->value;
    const NC_dim *const *const end = &dpp[ncap->nelems];
    for(  /*NADA*/; dpp < end; dpp++)
    {
      xlen += ncx_len_NC_dim(*dpp);
    }
  }
  return xlen;
}


/* Write a NC_dimarray to the header */
static int
v1h_put_NC_dimarray(v1hs *psp, const NC_dimarray *ncap)
{
  int status;

  assert(psp != NULL);

  if(ncap == NULL
#if 1
    /* Backward:
     * This clause is for 'byte for byte'
     * backward compatibility.
     * Strickly speaking, it is 'bug for bug'.
     */
    || ncap->nelems == 0
#endif
    )
  {
    /*
     * Handle empty netcdf
     */
    const size_t nosz = 0;

    status = v1h_put_NCtype(psp, NC_UNSPECIFIED);
    if(status != ENOERR)
      return status;
    status = v1h_put_size_t(psp, &nosz);
    if(status != ENOERR)
      return status;
    return ENOERR;
  }
  /* else */

  status = v1h_put_NCtype(psp, NC_DIMENSION);
  if(status != ENOERR)
    return status;
  status = v1h_put_size_t(psp, &ncap->nelems);
  if(status != ENOERR)
    return status;

  {
    const NC_dim **dpp = (const NC_dim **)ncap->value;
    const NC_dim *const *const end = &dpp[ncap->nelems];
    for( /*NADA*/; dpp < end; dpp++)
    {
      status = v1h_put_NC_dim(psp, *dpp);
      if(status)
        return status;
    }
  }
  return ENOERR;
}


/* Read a NC_dimarray from the header */
static int
v1h_get_NC_dimarray(v1hs *gsp, NC_dimarray *ncap)
{
  int status;
  NCtype type = NC_UNSPECIFIED;

  assert(gsp != NULL && gsp->pos != NULL);
  assert(ncap != NULL);
  assert(ncap->value == NULL);

  status = v1h_get_NCtype(gsp, &type);
  if(status != ENOERR)
    return status;

  status = v1h_get_size_t(gsp, &ncap->nelems);
  if(status != ENOERR)
    return status;
  
  if(ncap->nelems == 0)
    return ENOERR;
  /* else */
  if(type != NC_DIMENSION)
    return EINVAL;

  ncap->value = (NC_dim **) malloc(ncap->nelems * sizeof(NC_dim *));
  if(ncap->value == NULL)
    return NC_ENOMEM;
  ncap->nalloc = ncap->nelems;

  {
    NC_dim **dpp = ncap->value;
    NC_dim *const *const end = &dpp[ncap->nelems];
    for( /*NADA*/; dpp < end; dpp++)
    {
      status = v1h_get_NC_dim(gsp, dpp);
      if(status)
      {
        ncap->nelems = (size_t)(dpp - ncap->value);
        free_NC_dimarrayV(ncap);
        return status;
      }
    }
  }

  return ENOERR;
}


/* End NC_dim */
/* Begin NC_attr */


/*
 * How much space will 'attrp' take in external representation?
 * Formerly
NC_xlen_attr(app)
 */
static size_t
ncx_len_NC_attr(const NC_attr *attrp)
{
  size_t sz;

  assert(attrp != NULL);

  sz = ncx_len_NC_string(attrp->name);
  sz += X_SIZEOF_NC_TYPE; /* type */
  sz += X_SIZEOF_SIZE_T; /* nelems */
  sz += attrp->xsz;

  return(sz);
}


#undef MIN
#define MIN(mm,nn) (((mm) < (nn)) ? (mm) : (nn))

/*
 * Put the values of an attribute
 * The loop is necessary since attrp->nelems
 * could potentially be quite large.
 */
static int
v1h_put_NC_attrV(v1hs *psp, const NC_attr *attrp)
{
  int status;
  const size_t perchunk =  psp->extent;
  size_t remaining = attrp->xsz;
  void *value = attrp->xvalue;
  size_t nbytes; 

  assert(psp->extent % X_ALIGN == 0);
  
  do {
    nbytes = MIN(perchunk, remaining);
  
    status = check_v1hs(psp, nbytes);
    if(status != ENOERR)
      return status;
  
    (void) memcpy(psp->pos, value, nbytes);

    psp->pos = (void *)((char *)psp->pos + nbytes);
    value = (void *)((char *)value + nbytes);
    remaining -= nbytes;

  } while(remaining != 0); 

  return ENOERR;
}

/* Write a NC_attr to the header */
static int
v1h_put_NC_attr(v1hs *psp, const NC_attr *attrp)
{
  int status;

  status = v1h_put_NC_string(psp, attrp->name);
  if(status != ENOERR)
    return status;

  status = v1h_put_nc_type(psp, &attrp->type);
  if(status != ENOERR)
    return status;

  status = v1h_put_size_t(psp, &attrp->nelems);
  if(status != ENOERR)
    return status;

  status = v1h_put_NC_attrV(psp, attrp);
  if(status != ENOERR)
    return status;

  return ENOERR;
}


/*
 * Get the values of an attribute
 * The loop is necessary since attrp->nelems
 * could potentially be quite large.
 */
static int
v1h_get_NC_attrV(v1hs *gsp, NC_attr *attrp)
{
  int status;
  const size_t perchunk =  gsp->extent;
  size_t remaining = attrp->xsz;
  void *value = attrp->xvalue;
  size_t nget; 

  do {
    nget = MIN(perchunk, remaining);
  
    status = check_v1hs(gsp, nget);
    if(status != ENOERR)
      return status;
  
    (void) memcpy(value, gsp->pos, nget);

    gsp->pos = (void *)((char *)gsp->pos + nget);
    value = (void *)((char *)value + nget);
    remaining -= nget;

  } while(remaining != 0); 

  return ENOERR;
}


/* Read a NC_attr from the header */
static int
v1h_get_NC_attr(v1hs *gsp, NC_attr **attrpp)
{
  NC_string *strp;
  int status;
  nc_type type;
  size_t nelems;
  NC_attr *attrp;

  status = v1h_get_NC_string(gsp, &strp);
  if(status != ENOERR)
    return status;

  status = v1h_get_nc_type(gsp, &type);
  if(status != ENOERR)
    goto unwind_name;

  status = v1h_get_size_t(gsp, &nelems);
  if(status != ENOERR)
    goto unwind_name;

  attrp = new_x_NC_attr(strp, type, nelems);
  if(attrp == NULL)
  {
    status = NC_ENOMEM;
    goto unwind_name;
  }
  
  status = v1h_get_NC_attrV(gsp, attrp);
  if(status != ENOERR)
  {
    free_NC_attr(attrp); /* frees strp */
    return status;
  }

  *attrpp = attrp;

  return ENOERR;

unwind_name:
  free_NC_string(strp);
  return status;
}


/* How much space in the header is required for this NC_attrarray? */
static size_t
ncx_len_NC_attrarray(const NC_attrarray *ncap)
{
  size_t xlen = X_SIZEOF_NCTYPE;  /* type */
  xlen += X_SIZEOF_SIZE_T;  /* count */
  if(ncap == NULL)
    return xlen;
  /* else */
  {
    const NC_attr **app = (const NC_attr **)ncap->value;
    const NC_attr *const *const end = &app[ncap->nelems];
    for( /*NADA*/; app < end; app++)
    {
      xlen += ncx_len_NC_attr(*app);
    }
  }
  return xlen;
}


/* Write a NC_attrarray to the header */
static int
v1h_put_NC_attrarray(v1hs *psp, const NC_attrarray *ncap)
{
  int status;

  assert(psp != NULL);

  if(ncap == NULL
#if 1
    /* Backward:
     * This clause is for 'byte for byte'
     * backward compatibility.
     * Strickly speaking, it is 'bug for bug'.
     */
    || ncap->nelems == 0
#endif
    )
  {
    /*
     * Handle empty netcdf
     */
    const size_t nosz = 0;

    status = v1h_put_NCtype(psp, NC_UNSPECIFIED);
    if(status != ENOERR)
      return status;
    status = v1h_put_size_t(psp, &nosz);
    if(status != ENOERR)
      return status;
    return ENOERR;
  }
  /* else */

  status = v1h_put_NCtype(psp, NC_ATTRIBUTE);
  if(status != ENOERR)
    return status;
  status = v1h_put_size_t(psp, &ncap->nelems);
  if(status != ENOERR)
    return status;

  {
    const NC_attr **app = (const NC_attr **)ncap->value;
    const NC_attr *const *const end = &app[ncap->nelems];
    for( /*NADA*/; app < end; app++)
    {
      status = v1h_put_NC_attr(psp, *app);
      if(status)
        return status;
    }
  }
  return ENOERR;
}


/* Read a NC_attrarray from the header */
static int
v1h_get_NC_attrarray(v1hs *gsp, NC_attrarray *ncap)
{
  int status;
  NCtype type = NC_UNSPECIFIED;

  assert(gsp != NULL && gsp->pos != NULL);
  assert(ncap != NULL);
  assert(ncap->value == NULL);

  status = v1h_get_NCtype(gsp, &type);
  if(status != ENOERR)
    return status;
  status = v1h_get_size_t(gsp, &ncap->nelems);
  if(status != ENOERR)
    return status;
  
  if(ncap->nelems == 0)
    return ENOERR;
  /* else */
  if(type != NC_ATTRIBUTE)
    return EINVAL;

  ncap->value = (NC_attr **) malloc(ncap->nelems * sizeof(NC_attr *));
  if(ncap->value == NULL)
    return NC_ENOMEM;
  ncap->nalloc = ncap->nelems;

  {
    NC_attr **app = ncap->value;
    NC_attr *const *const end = &app[ncap->nelems];
    for( /*NADA*/; app < end; app++)
    {
      status = v1h_get_NC_attr(gsp, app);
      if(status)
      {
        ncap->nelems = (size_t)(app - ncap->value);
        free_NC_attrarrayV(ncap);
        return status;
      }
    }
  }

  return ENOERR;
}

/* End NC_attr */
/* Begin NC_var */

/*
 * How much space will the xdr'd var take.
 * Formerly
NC_xlen_var(vpp)
 */
static size_t
ncx_len_NC_var(const NC_var *varp, size_t sizeof_off_t)
{
  size_t sz;

  assert(varp != NULL);
  assert(sizeof_off_t != 0);

  sz = ncx_len_NC_string(varp->name);
  sz += X_SIZEOF_SIZE_T; /* ndims */
  sz += ncx_len_int(varp->ndims); /* dimids */
  sz += ncx_len_NC_attrarray(&varp->attrs);
  sz += X_SIZEOF_NC_TYPE; /* type */
  sz += X_SIZEOF_SIZE_T; /* len */
  sz += sizeof_off_t; /* begin */

  return(sz);
}


/* Write a NC_var to the header */
static int
v1h_put_NC_var(v1hs *psp, const NC_var *varp)
{
  int status;

  status = v1h_put_NC_string(psp, varp->name);
  if(status != ENOERR)
    return status;

  status = v1h_put_size_t(psp, &varp->ndims);
  if(status != ENOERR)
    return status;

  status = check_v1hs(psp, ncx_len_int(varp->ndims));
  if(status != ENOERR)
    return status;
  status = ncx_putn_int_int(&psp->pos,
      varp->ndims, varp->dimids);
  if(status != ENOERR)
    return status;

  status = v1h_put_NC_attrarray(psp, &varp->attrs);
  if(status != ENOERR)
    return status;

  status = v1h_put_nc_type(psp, &varp->type);
  if(status != ENOERR)
    return status;

  status = v1h_put_size_t(psp, &varp->len);
  if(status != ENOERR)
    return status;

  status = check_v1hs(psp, psp->version == 1 ? 4 : 8);
  if(status != ENOERR)
     return status;
  status = ncx_put_off_t(&psp->pos, &varp->begin, psp->version == 1 ? 4 : 8);
  if(status != ENOERR)
    return status;

  return ENOERR;
}


/* Read a NC_var from the header */
static int
v1h_get_NC_var(v1hs *gsp, NC_var **varpp)
{
  NC_string *strp;
  int status;
  size_t ndims;
  NC_var *varp;

  status = v1h_get_NC_string(gsp, &strp);
  if(status != ENOERR)
    return status;

  status = v1h_get_size_t(gsp, &ndims);
  if(status != ENOERR)
    goto unwind_name;

  varp = new_x_NC_var(strp, ndims);
  if(varp == NULL)
  {
    status = NC_ENOMEM;
    goto unwind_name;
  }

  status = check_v1hs(gsp, ncx_len_int(ndims));
  if(status != ENOERR)
    goto unwind_alloc;
  status = ncx_getn_int_int((const void **)(&gsp->pos),
      ndims, varp->dimids);
  if(status != ENOERR)
    goto unwind_alloc;

  status = v1h_get_NC_attrarray(gsp, &varp->attrs);
  if(status != ENOERR)
    goto unwind_alloc;

  status = v1h_get_nc_type(gsp, &varp->type);
  if(status != ENOERR)
     goto unwind_alloc;

  status = v1h_get_size_t(gsp, &varp->len);
  if(status != ENOERR)
     goto unwind_alloc;

  status = check_v1hs(gsp, gsp->version == 1 ? 4 : 8);
  if(status != ENOERR)
     goto unwind_alloc;
  status = ncx_get_off_t((const void **)&gsp->pos,
             &varp->begin, gsp->version == 1 ? 4 : 8);
  if(status != ENOERR)
     goto unwind_alloc;
  
  *varpp = varp;
  return ENOERR;

unwind_alloc:
  free_NC_var(varp); /* frees name */
  return status;

unwind_name:
  free_NC_string(strp);
  return status;
}


/* How much space in the header is required for this NC_vararray? */
static size_t
ncx_len_NC_vararray(const NC_vararray *ncap, size_t sizeof_off_t)
{
  size_t xlen = X_SIZEOF_NCTYPE;  /* type */
  xlen += X_SIZEOF_SIZE_T;  /* count */
  if(ncap == NULL)
    return xlen;
  /* else */
  {
    const NC_var **vpp = (const NC_var **)ncap->value;
    const NC_var *const *const end = &vpp[ncap->nelems];
    for( /*NADA*/; vpp < end; vpp++)
    {
      xlen += ncx_len_NC_var(*vpp, sizeof_off_t);
    }
  }
  return xlen;
}


/* Write a NC_vararray to the header */
static int
v1h_put_NC_vararray(v1hs *psp, const NC_vararray *ncap)
{
  int status;

  assert(psp != NULL);

  if(ncap == NULL
#if 1
    /* Backward:
     * This clause is for 'byte for byte'
     * backward compatibility.
     * Strickly speaking, it is 'bug for bug'.
     */
    || ncap->nelems == 0
#endif
    )
  {
    /*
     * Handle empty netcdf
     */
    const size_t nosz = 0;

    status = v1h_put_NCtype(psp, NC_UNSPECIFIED);
    if(status != ENOERR)
      return status;
    status = v1h_put_size_t(psp, &nosz);
    if(status != ENOERR)
      return status;
    return ENOERR;
  }
  /* else */

  status = v1h_put_NCtype(psp, NC_VARIABLE);
  if(status != ENOERR)
    return status;
  status = v1h_put_size_t(psp, &ncap->nelems);
  if(status != ENOERR)
    return status;

  {
    const NC_var **vpp = (const NC_var **)ncap->value;
    const NC_var *const *const end = &vpp[ncap->nelems];
    for( /*NADA*/; vpp < end; vpp++)
    {
      status = v1h_put_NC_var(psp, *vpp);
      if(status)
        return status;
    }
  }
  return ENOERR;
}


/* Read a NC_vararray from the header */
static int
v1h_get_NC_vararray(v1hs *gsp, NC_vararray *ncap)
{
  int status;
  NCtype type = NC_UNSPECIFIED;

  assert(gsp != NULL && gsp->pos != NULL);
  assert(ncap != NULL);
  assert(ncap->value == NULL);

  status = v1h_get_NCtype(gsp, &type);
  if(status != ENOERR)
    return status;
  
  status = v1h_get_size_t(gsp, &ncap->nelems);
  if(status != ENOERR)
    return status;
  
  if(ncap->nelems == 0)
    return ENOERR;
  /* else */
  if(type != NC_VARIABLE)
    return EINVAL;

  ncap->value = (NC_var **) malloc(ncap->nelems * sizeof(NC_var *));
  if(ncap->value == NULL)
    return NC_ENOMEM;
  ncap->nalloc = ncap->nelems;

  {
    NC_var **vpp = ncap->value;
    NC_var *const *const end = &vpp[ncap->nelems];
    for( /*NADA*/; vpp < end; vpp++)
    {
      status = v1h_get_NC_var(gsp, vpp);
      if(status)
      {
        ncap->nelems = (size_t)(vpp - ncap->value);
        free_NC_vararrayV(ncap);
        return status;
      }
    }
  }

  return ENOERR;
}


/* End NC_var */
/* Begin NC */


/*
 * Recompute the shapes of all variables
 * Sets ncp->begin_var to start of first variable.
 * Sets ncp->begin_rec to start of first record variable.
 * Returns -1 on error. The only possible error is a reference
 * to a non existent dimension, which could occur for a corrupted
 * netcdf file.
 */
static int
NC_computeshapes(NC *ncp)
{
  NC_var **vpp = (NC_var **)ncp->vars.value;
  NC_var *const *const end = &vpp[ncp->vars.nelems];
  NC_var *first_var = NULL; /* first "non-record" var */
  NC_var *first_rec = NULL; /* first "record" var */
  int status;

  ncp->begin_var = (off_t) ncp->xsz;
  ncp->begin_rec = (off_t) ncp->xsz;
  ncp->recsize = 0;

  if(ncp->vars.nelems == 0)
    return(0);
  
  for( /*NADA*/; vpp < end; vpp++)
  {
    status = NC_var_shape(*vpp, &ncp->dims);
    if(status != ENOERR)
      return(status);

      if(IS_RECVAR(*vpp)) 
    {
        if(first_rec == NULL) 
        first_rec = *vpp;
      ncp->recsize += (*vpp)->len;
    }
    else
    {
            if(first_var == NULL)
              first_var = *vpp;
      /*
       * Overwritten each time thru.
       * Usually overwritten in first_rec != NULL clause below.
       */
      ncp->begin_rec = (*vpp)->begin + (off_t)(*vpp)->len;
    }
  }

  if(first_rec != NULL)
  {
    assert(ncp->begin_rec <= first_rec->begin);
    ncp->begin_rec = first_rec->begin;
    /*
     * for special case of exactly one record variable, pack value
     */
    if(ncp->recsize == first_rec->len)
      ncp->recsize = *first_rec->dsizes * first_rec->xsz;
  }

  if(first_var != NULL)
  {
    ncp->begin_var = first_var->begin;
  }
  else
  {
    ncp->begin_var = ncp->begin_rec;
  }

  assert(ncp->begin_var > 0);
  assert(ncp->xsz <= (size_t)ncp->begin_var);
  assert(ncp->begin_rec > 0);
  assert(ncp->begin_var <= ncp->begin_rec);
  
  return(ENOERR);
}


/* How much space in the header is required for the NC data structure? */
size_t
ncx_len_NC(const NC *ncp, size_t sizeof_off_t)
{
  size_t xlen = sizeof(ncmagic);

  assert(ncp != NULL);
  
  xlen += X_SIZEOF_SIZE_T; /* numrecs */
  xlen += ncx_len_NC_dimarray(&ncp->dims);
  xlen += ncx_len_NC_attrarray(&ncp->attrs);
  xlen += ncx_len_NC_vararray(&ncp->vars, sizeof_off_t);

  return xlen;
}


/* Write the file header */
int
ncx_put_NC(const NC *ncp, void **xpp, off_t offset, size_t extent)
{
  int status;
  v1hs ps; /* the get stream */

  assert(ncp != NULL);

  /* Initialize stream ps */

  ps.nciop = ncp->nciop;
  ps.flags = RGN_WRITE;

  if (ncp->flags & NC_64BIT_OFFSET)
    ps.version = 2;
  else 
    ps.version = 1;

  if(xpp == NULL)
  {
    /*
     * Come up with a reasonable stream read size.
     */
    extent = ncp->xsz;
    if(extent <= MIN_NC_XSZ)
    {
      /* first time read */
      extent = ncp->chunk;
      /* Protection for when ncp->chunk is huge;
       * no need to read hugely. */
            if(extent > 4096)
        extent = 4096;
    }
    else if(extent > ncp->chunk)
    {
      extent = ncp->chunk;
    }
    
    ps.offset = 0;
    ps.extent = extent;
    ps.base = NULL;
    ps.pos = ps.base;

    status = fault_v1hs(&ps, extent);
    if(status)
      return status;
  }
  else
  {
    ps.offset = offset;
    ps.extent = extent;
    ps.base = *xpp;
    ps.pos = ps.base;
    ps.end = (char *)ps.base + ps.extent;
  }

  if (ps.version == 2)
    status = ncx_putn_schar_schar(&ps.pos, sizeof(ncmagic), ncmagic);
  else
    status = ncx_putn_schar_schar(&ps.pos, sizeof(ncmagic1), ncmagic1);
  if(status != ENOERR)
    goto release;

  {
  const size_t nrecs = NC_get_numrecs(ncp);
  status = ncx_put_size_t(&ps.pos, &nrecs);
  if(status != ENOERR)
    goto release;
  }

  assert((char *)ps.pos < (char *)ps.end);

  status = v1h_put_NC_dimarray(&ps, &ncp->dims);
  if(status != ENOERR)
    goto release;

  status = v1h_put_NC_attrarray(&ps, &ncp->attrs);
  if(status != ENOERR)
    goto release;

  status = v1h_put_NC_vararray(&ps, &ncp->vars);
  if(status != ENOERR)
    goto release;

release:
  (void) rel_v1hs(&ps);

  return status;
}


/* Make the in-memory NC structure from reading the file header */
int
nc_get_NC(NC *ncp)
{
  int status;
  v1hs gs; /* the get stream */

  assert(ncp != NULL);

  /* Initialize stream gs */

  gs.nciop = ncp->nciop;
  gs.offset = 0; /* beginning of file */
  gs.extent = 0;
  gs.flags = 0;
  gs.version = 0;
  gs.base = NULL;
  gs.pos = gs.base;

  {
    /*
     * Come up with a reasonable stream read size.
     */
    off_t filesize;
    size_t extent;
    
    extent = ncp->xsz;
    if(extent <= MIN_NC_XSZ)
    {
      status = ncio_filesize(ncp->nciop, &filesize);
      if(status)
          return status;
      /* first time read */
      extent = ncp->chunk;
      /* Protection for when ncp->chunk is huge;
       * no need to read hugely. */
      if(extent > 4096)
        extent = 4096;
      if(extent > (size_t)filesize)
        extent = filesize;
    }
    else if(extent > ncp->chunk)
    {
      extent = ncp->chunk;
    }

    /*
     * Invalidate the I/O buffers to force a read of the header
     * region.
     */
    status = gs.nciop->sync(gs.nciop);
    if(status)
      return status;

    status = fault_v1hs(&gs, extent);
    if(status)
      return status;
  }

  /* get the header from the stream gs */

  {
    /* Get & check magic number */
    schar magic[sizeof(ncmagic)];
    (void) memset(magic, 0, sizeof(magic));

    status = ncx_getn_schar_schar(
      (const void **)(&gs.pos), sizeof(magic), magic);
    if(status != ENOERR)
      goto unwind_get;
  
    if(memcmp(magic, ncmagic, sizeof(ncmagic)-1) != 0)
    {
      status = NC_ENOTNC;
      goto unwind_get;
    }
    /* Check version number in last byte of magic */
    if (magic[sizeof(ncmagic)-1] == 0x1) {
      gs.version = 1;
    } else if (magic[sizeof(ncmagic)-1] == 0x2) {
      gs.version = 2;
      fSet(ncp->flags, NC_64BIT_OFFSET);
      /* Now we support version 2 file access on non-LFS systems -- rkr */
#if 0
      if (sizeof(off_t) != 8) {
        fprintf(stderr, "NETCDF WARNING: Version 2 file on 32-bit system.\n");
      }
#endif
    } else {
      status = NC_ENOTNC;
      goto unwind_get;
    }
  }
  
  {
  size_t nrecs = 0;
  status = ncx_get_size_t((const void **)(&gs.pos), &nrecs);
  if(status != ENOERR)
    goto unwind_get;
  NC_set_numrecs(ncp, nrecs);
  }

  assert((char *)gs.pos < (char *)gs.end);

  status = v1h_get_NC_dimarray(&gs, &ncp->dims);
  if(status != ENOERR)
    goto unwind_get;

  status = v1h_get_NC_attrarray(&gs, &ncp->attrs);
  if(status != ENOERR)
    goto unwind_get;

  status = v1h_get_NC_vararray(&gs, &ncp->vars);
  if(status != ENOERR)
    goto unwind_get;
    
  ncp->xsz = ncx_len_NC(ncp, (gs.version == 1) ? 4 : 8);

  status = NC_computeshapes(ncp);
  if(status != ENOERR)
    goto unwind_get;

unwind_get:
  (void) rel_v1hs(&gs);
  return status;
}
