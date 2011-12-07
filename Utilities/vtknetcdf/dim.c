/*
 *  Copyright 1996, University Corporation for Atmospheric Research
 *      See netcdf/COPYRIGHT file for copying and redistribution conditions.
 */
/* $Id: dim.c,v 1.79 2010/04/11 04:15:38 dmh Exp $ */

#include "nc.h"
#include "rename.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "ncx.h"
#include "fbits.h"
#include "utf8proc.h"

#if defined(_MSC_VER) && (_MSC_VER >= 1300)
#  pragma warning ( disable : 4127 ) /* conditional expression is constant */
#endif /* MSVC 7.1 */

/*
 * Free dim
 * Formerly
NC_free_dim(dim)
 */
void
free_NC_dim(NC_dim *dimp)
{
  if(dimp == NULL)
    return;
  free_NC_string(dimp->name);
  free(dimp);
}


NC_dim *
new_x_NC_dim(NC_string *name)
{
  NC_dim *dimp;

  dimp = (NC_dim *) malloc(sizeof(NC_dim));
  if(dimp == NULL)
    return NULL;

  dimp->name = name;
  dimp->size = 0;

  return(dimp);
}


/*
 * Formerly
NC_new_dim(const char *uname, long size)
 */
static NC_dim *
new_NC_dim(const char *uname, size_t size)
{
  NC_string *strp;
  NC_dim *dimp;

  char *name = (char *)utf8proc_NFC((const unsigned char *)uname);
  if(name == NULL)
      return NULL;
  strp = new_NC_string(strlen(name), name);
  free(name);
  if(strp == NULL)
    return NULL;

  dimp = new_x_NC_dim(strp);
  if(dimp == NULL)
  {
    free_NC_string(strp);
    return NULL;
  }

  dimp->size = size;

  return(dimp);
}


static NC_dim *
dup_NC_dim(const NC_dim *dimp)
{
  return new_NC_dim(dimp->name->cp, dimp->size);
}

/*
 * Step thru NC_DIMENSION array, seeking the UNLIMITED dimension.
 * Return dimid or -1 on not found.
 * *dimpp is set to the appropriate NC_dim.
 * The loop structure is odd. In order to parallelize,
 * we moved a clearer 'break' inside the loop body to the loop test.
 */
int
find_NC_Udim(const NC_dimarray *ncap, NC_dim **dimpp)
{
  assert(ncap != NULL);

  if(ncap->nelems == 0)
    return -1;

  {
  size_t dimid = 0;
  NC_dim **loc = ncap->value;

  for(; (size_t) dimid < ncap->nelems
       && (*loc)->size != NC_UNLIMITED; dimid++, loc++)
  {
    /*EMPTY*/
  }
  if(dimid >= ncap->nelems)
    return(-1); /* not found */
  /* else, normal return */
  if(dimpp != NULL)
    *dimpp = *loc;
  return dimid;
  }
}


/*
 * Step thru NC_DIMENSION array, seeking match on uname.
 * Return dimid or -1 on not found.
 * *dimpp is set to the appropriate NC_dim.
 * The loop structure is odd. In order to parallelize,
 * we moved a clearer 'break' inside the loop body to the loop test.
 */
static int
NC_finddim(const NC_dimarray *ncap, const char *uname, NC_dim **dimpp)
{

   size_t dimid;
   size_t slen;
   NC_dim ** loc;
   char *name;

   assert(ncap != NULL);

   if(ncap->nelems == 0)
      return -1;

   {
      dimid = 0;
      loc = (NC_dim **) ncap->value;
      /* normalized version of uname */
      name = (char *)utf8proc_NFC((const unsigned char *)uname);
      if(name == NULL)
   return NC_ENOMEM;
      slen = strlen(name);

      for(; (size_t) dimid < ncap->nelems
       && (strlen((*loc)->name->cp) != slen
     || strncmp((*loc)->name->cp, name, slen) != 0);
    dimid++, loc++)
      {
   /*EMPTY*/
      }
      free(name);
      if(dimid >= ncap->nelems)
   return(-1); /* not found */
      /* else, normal return */
      if(dimpp != NULL)
   *dimpp = *loc;
      return(dimid);
   }
}


/* dimarray */


/*
 * Free the stuff "in" (referred to by) an NC_dimarray.
 * Leaves the array itself allocated.
 */
void
free_NC_dimarrayV0(NC_dimarray *ncap)
{
  assert(ncap != NULL);

  if(ncap->nelems == 0)
    return;

  assert(ncap->value != NULL);

  {
    NC_dim **dpp = ncap->value;
    NC_dim *const *const end = &dpp[ncap->nelems];
    for( /*NADA*/; dpp < end; dpp++)
    {
      free_NC_dim(*dpp);
      *dpp = NULL;
    }
  }
  ncap->nelems = 0;
}


/*
 * Free NC_dimarray values.
 * formerly
NC_free_array()
 */
void
free_NC_dimarrayV(NC_dimarray *ncap)
{
  assert(ncap != NULL);

  if(ncap->nalloc == 0)
    return;

  assert(ncap->value != NULL);

  free_NC_dimarrayV0(ncap);

  free(ncap->value);
  ncap->value = NULL;
  ncap->nalloc = 0;
}


int
dup_NC_dimarrayV(NC_dimarray *ncap, const NC_dimarray *ref)
{
  int status = NC_NOERR;

  assert(ref != NULL);
  assert(ncap != NULL);

  if(ref->nelems != 0)
  {
    const size_t sz = ref->nelems * sizeof(NC_dim *);
    ncap->value = (NC_dim **) malloc(sz);
    if(ncap->value == NULL)
      return NC_ENOMEM;
    (void) memset(ncap->value, 0, sz);
    ncap->nalloc = ref->nelems;
  }

  ncap->nelems = 0;
  {
    NC_dim **dpp = ncap->value;
    const NC_dim **drpp = (const NC_dim **)ref->value;
    NC_dim *const *const end = &dpp[ref->nelems];
    for( /*NADA*/; dpp < end; drpp++, dpp++, ncap->nelems++)
    {
      *dpp = dup_NC_dim(*drpp);
      if(*dpp == NULL)
      {
        status = NC_ENOMEM;
        break;
      }
    }
  }

  if(status != NC_NOERR)
  {
    free_NC_dimarrayV(ncap);
    return status;
  }

  assert(ncap->nelems == ref->nelems);

  return NC_NOERR;
}


/*
 * Add a new handle on the end of an array of handles
 * Formerly
NC_incr_array(array, tail)
 */
static int
incr_NC_dimarray(NC_dimarray *ncap, NC_dim *newelemp)
{
  NC_dim **vp;

  assert(ncap != NULL);

  if(ncap->nalloc == 0)
  {
    assert(ncap->nelems == 0);
    vp = (NC_dim **) malloc(NC_ARRAY_GROWBY * sizeof(NC_dim *));
    if(vp == NULL)
      return NC_ENOMEM;
    ncap->value = vp;
    ncap->nalloc = NC_ARRAY_GROWBY;
  }
  else if(ncap->nelems +1 > ncap->nalloc)
  {
    vp = (NC_dim **) realloc(ncap->value,
      (ncap->nalloc + NC_ARRAY_GROWBY) * sizeof(NC_dim *));
    if(vp == NULL)
      return NC_ENOMEM;
    ncap->value = vp;
    ncap->nalloc += NC_ARRAY_GROWBY;
  }

  if(newelemp != NULL)
  {
    ncap->value[ncap->nelems] = newelemp;
    ncap->nelems++;
  }
  return NC_NOERR;
}


NC_dim *
elem_NC_dimarray(const NC_dimarray *ncap, size_t elem)
{
  assert(ncap != NULL);
    /* cast needed for braindead systems with signed size_t */
  if(ncap->nelems == 0 || (unsigned long) elem >= ncap->nelems)
    return NULL;

  assert(ncap->value != NULL);

  return ncap->value[elem];
}


/* Public */

int
DISPNAME(def_dim)(int ncid, const char *name, size_t size, int *dimidp)
{
  int status;
  NC *ncp;
  int dimid;
  NC_dim *dimp;

  status = NC_check_id(ncid, &ncp);
  if(status != NC_NOERR)
    return status;

  if(!NC_indef(ncp))
    return NC_ENOTINDEFINE;

  status = NC_check_name(name);
  if(status != NC_NOERR)
    return status;

  if ((ncp->flags & NC_64BIT_OFFSET) && sizeof(off_t) > 4) {
      /* CDF2 format and LFS */
      if(size > X_UINT_MAX - 3) /* "- 3" handles rounded-up size */
    return NC_EDIMSIZE;
  } else {
      /* CDF1 format */
      if(size > X_INT_MAX - 3)
    return NC_EDIMSIZE;
  }

  if(size == NC_UNLIMITED)
  {
    dimid = find_NC_Udim(&ncp->dims, &dimp);
    if(dimid != -1)
    {
      assert(dimid != -1);
      return NC_EUNLIMIT;
    }
  }

  if(ncp->dims.nelems >= NC_MAX_DIMS)
    return NC_EMAXDIMS;

  dimid = NC_finddim(&ncp->dims, name, &dimp);
  if(dimid != -1)
    return NC_ENAMEINUSE;

  dimp = new_NC_dim(name, size);
  if(dimp == NULL)
    return NC_ENOMEM;
  status = incr_NC_dimarray(&ncp->dims, dimp);
  if(status != NC_NOERR)
  {
    free_NC_dim(dimp);
    return status;
  }

  if(dimidp != NULL)
    *dimidp = (int)ncp->dims.nelems -1;
  return NC_NOERR;
}


int
DISPNAME(inq_dimid)(int ncid, const char *name, int *dimid_ptr)
{
  int status;
  NC *ncp;
  int dimid;

  status = NC_check_id(ncid, &ncp);
  if(status != NC_NOERR)
    return status;

  dimid = NC_finddim(&ncp->dims, name, NULL);

  if(dimid == -1)
    return NC_EBADDIM;

  *dimid_ptr = dimid;
  return NC_NOERR;
}


int
nc_inq_dim(int ncid, int dimid, char *name, size_t *sizep)
{
  int status;
  NC *ncp;
  NC_dim *dimp;

  status = NC_check_id(ncid, &ncp);
  if(status != NC_NOERR)
    return status;

  dimp = elem_NC_dimarray(&ncp->dims, (size_t)dimid);
  if(dimp == NULL)
    return NC_EBADDIM;

  if(name != NULL)
  {
    (void)strncpy(name, dimp->name->cp,
      dimp->name->nchars);
    name[dimp->name->nchars] = 0;
  }
  if(sizep != 0)
  {
    if(dimp->size == NC_UNLIMITED)
      *sizep = NC_get_numrecs(ncp);
    else
      *sizep = dimp->size;
  }
  return NC_NOERR;
}


int
nc_inq_dimname(int ncid, int dimid, char *name)
{
  int status;
  NC *ncp;
  NC_dim *dimp;

  status = NC_check_id(ncid, &ncp);
  if(status != NC_NOERR)
    return status;

  dimp = elem_NC_dimarray(&ncp->dims, (size_t)dimid);
  if(dimp == NULL)
    return NC_EBADDIM;

  if(name != NULL)
  {
    (void)strncpy(name, dimp->name->cp,
      dimp->name->nchars);
    name[dimp->name->nchars] = 0;
  }

  return NC_NOERR;
}


int
nc_inq_dimlen(int ncid, int dimid, size_t *lenp)
{
  int status;
  NC *ncp;
  NC_dim *dimp;

  status = NC_check_id(ncid, &ncp);
  if(status != NC_NOERR)
    return status;

  dimp = elem_NC_dimarray(&ncp->dims, (size_t)dimid);
  if(dimp == NULL)
    return NC_EBADDIM;

  if(lenp != 0)
  {
    if(dimp->size == NC_UNLIMITED)
      *lenp = NC_get_numrecs(ncp);
    else
      *lenp = dimp->size;
  }
  return NC_NOERR;
}


int
DISPNAME(rename_dim)( int ncid, int dimid, const char *unewname)
{
  int status;
  NC *ncp;
  int existid;
  NC_dim *dimp;
  char *newname;    /* normalized */

  status = NC_check_id(ncid, &ncp);
  if(status != NC_NOERR)
    return status;

  if(NC_readonly(ncp))
    return NC_EPERM;

  status = NC_check_name(unewname);
  if(status != NC_NOERR)
    return status;

  existid = NC_finddim(&ncp->dims, unewname, &dimp);
  if(existid != -1)
    return NC_ENAMEINUSE;

  dimp = elem_NC_dimarray(&ncp->dims, (size_t)dimid);
  if(dimp == NULL)
    return NC_EBADDIM;

  newname = (char *)utf8proc_NFC((const unsigned char *)unewname);
  if(newname == NULL)
      return NC_ENOMEM;
  if(NC_indef(ncp))
  {
    NC_string *old = dimp->name;
    NC_string *newStr = new_NC_string(strlen(newname), newname);
    free(newname);
    if(newStr == NULL)
      return NC_ENOMEM;
    dimp->name = newStr;
    free_NC_string(old);
    return NC_NOERR;
  }

  /* else, not in define mode */

  status = set_NC_string(dimp->name, newname);
  free(newname);
  if(status != NC_NOERR)
    return status;

  set_NC_hdirty(ncp);

  if(NC_doHsync(ncp))
  {
    status = NC_sync(ncp);
    if(status != NC_NOERR)
      return status;
  }

  return NC_NOERR;
}
