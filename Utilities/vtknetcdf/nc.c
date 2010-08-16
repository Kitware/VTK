/*
 *  Copyright 1996, University Corporation for Atmospheric Research
 *      See netcdf/COPYRIGHT file for copying and redistribution conditions.
 */
/* $Id: nc.c,v 2.159 2010/04/11 04:15:39 dmh Exp $ */

#include "nc.h"
#include "rename.h"
#include "rnd.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "ncx.h"
#if defined(LOCKNUMREC) /* && _CRAYMPP */
#  include <mpp/shmem.h>
#  include <intrinsics.h>
#endif
#ifdef HAVE_UNISTD
#include <unistd.h>
#endif

#ifdef __BORLANDC__
#  pragma warn -8004 /* "assigned a value that is never used" */
#  pragma warn -8065 /* "Call to function 'XXX' with no prototype" */
#endif

#if defined(_MSC_VER) && (_MSC_VER >= 1300)
#  pragma warning ( disable : 4127 ) /* conditional expression is constant */
#  pragma warning ( disable : 4130 ) /* logical operation on address of string constant */
#endif /* MSVC 7.1 */

/* list of open netcdf's */
static NC *NClist = NULL;

/* This is the default create format for nc_create and nc__create. */
int default_create_format = NC_FORMAT_CLASSIC;

/* These have to do with version numbers. */
#define MAGIC_NUM_LEN 4
#define VER_CLASSIC 1
#define VER_64BIT_OFFSET 2
#define VER_HDF5 3

static void
add_to_NCList(NC *ncp)
{
  assert(ncp != NULL);

  ncp->prev = NULL;
  if(NClist != NULL)
    NClist->prev = ncp;
  ncp->next = NClist;
  NClist = ncp;
}

static void
del_from_NCList(NC *ncp)
{
  assert(ncp != NULL);

  if(NClist == ncp)
  {
    assert(ncp->prev == NULL);
    NClist = ncp->next;
  }
  else
  {
    assert(ncp->prev != NULL);
    ncp->prev->next = ncp->next;
  }

  if(ncp->next != NULL)
    ncp->next->prev = ncp->prev;

  ncp->next = NULL;
  ncp->prev = NULL;
}


int
NC_check_id(int ncid, NC **ncpp)
{
  NC *ncp;

  if(ncid >= 0)
  {
    for(ncp = NClist; ncp != NULL; ncp = ncp->next)
    {
      if(ncp->nciop->fd == ncid)
      {
        *ncpp = ncp;
        return NC_NOERR; /* normal return */
      }
    }
  }

  /* else, not found */
  return NC_EBADID;
}


static void
free_NC(NC *ncp)
{
  if(ncp == NULL)
    return;
  free_NC_dimarrayV(&ncp->dims);
  free_NC_attrarrayV(&ncp->attrs);
  free_NC_vararrayV(&ncp->vars);
#if _CRAYMPP && defined(LOCKNUMREC)
  shfree(ncp);
#else
  free(ncp);
#endif /* _CRAYMPP && LOCKNUMREC */
}


static NC *
new_NC(const size_t *chunkp)
{
  NC *ncp;

#if _CRAYMPP && defined(LOCKNUMREC)
  ncp = (NC *) shmalloc(sizeof(NC));
#else
  ncp = (NC *) malloc(sizeof(NC));
#endif /* _CRAYMPP && LOCKNUMREC */
  if(ncp == NULL)
    return NULL;
  (void) memset(ncp, 0, sizeof(NC));

  ncp->xsz = MIN_NC_XSZ;
  assert(ncp->xsz == ncx_len_NC(ncp,0));

  ncp->chunk = chunkp != NULL ? *chunkp : NC_SIZEHINT_DEFAULT;

  return ncp;
}


static NC *
dup_NC(const NC *ref)
{
  NC *ncp;

#if _CRAYMPP && defined(LOCKNUMREC)
  ncp = (NC *) shmalloc(sizeof(NC));
#else
  ncp = (NC *) malloc(sizeof(NC));
#endif /* _CRAYMPP && LOCKNUMREC */
  if(ncp == NULL)
    return NULL;
  (void) memset(ncp, 0, sizeof(NC));

  if(dup_NC_dimarrayV(&ncp->dims, &ref->dims) != NC_NOERR)
    goto err;
  if(dup_NC_attrarrayV(&ncp->attrs, &ref->attrs) != NC_NOERR)
    goto err;
  if(dup_NC_vararrayV(&ncp->vars, &ref->vars) != NC_NOERR)
    goto err;

  ncp->xsz = ref->xsz;
  ncp->begin_var = ref->begin_var;
  ncp->begin_rec = ref->begin_rec;
  ncp->recsize = ref->recsize;
  NC_set_numrecs(ncp, NC_get_numrecs(ref));
  return ncp;
err:
  free_NC(ncp);
  return NULL;
}


/*
 *  Verify that this is a user nc_type
 * Formerly
NCcktype()
 * Sense of the return is changed.
 */
int
nc_cktype(nc_type type)
{
  switch((int)type){
  case NC_BYTE:
  case NC_CHAR:
  case NC_SHORT:
  case NC_INT:
  case NC_FLOAT:
  case NC_DOUBLE:
    return(NC_NOERR);
  }
  return(NC_EBADTYPE);
}


/*
 * How many objects of 'type'
 * will fit into xbufsize?
 */
size_t
ncx_howmany(nc_type type, size_t xbufsize)
{
  switch(type){
  case NC_BYTE:
  case NC_CHAR:
    return xbufsize;
  case NC_SHORT:
    return xbufsize/X_SIZEOF_SHORT;
  case NC_INT:
    return xbufsize/X_SIZEOF_INT;
  case NC_FLOAT:
    return xbufsize/X_SIZEOF_FLOAT;
  case NC_DOUBLE:
    return xbufsize/X_SIZEOF_DOUBLE;
  default:
          assert("ncx_howmany: Bad type" == 0);
    return(0);
  }
}

#define  D_RNDUP(x, align) _RNDUP(x, (off_t)(align))

/*
 * Compute each variable's 'begin' offset,
 * update 'begin_rec' as well.
 */
static int
NC_begins(NC *ncp,
  size_t h_minfree, size_t v_align,
  size_t v_minfree, size_t r_align)
{
  size_t ii;
  int sizeof_off_t;
  off_t index = 0;
  NC_var **vpp;
  NC_var *last = NULL;

  if(v_align == NC_ALIGN_CHUNK)
    v_align = ncp->chunk;
  if(r_align == NC_ALIGN_CHUNK)
    r_align = ncp->chunk;

  if (fIsSet(ncp->flags, NC_64BIT_OFFSET)) {
    sizeof_off_t = 8;
  } else {
    sizeof_off_t = 4;
  }

  ncp->xsz = ncx_len_NC(ncp,sizeof_off_t);

  if(ncp->vars.nelems == 0)
    return NC_NOERR;

  /* only (re)calculate begin_var if there is not sufficient space in header
     or start of non-record variables is not aligned as requested by valign */
  if (ncp->begin_var < ncp->xsz + h_minfree ||
      ncp->begin_var != D_RNDUP(ncp->begin_var, v_align) )
  {
    index = (off_t) ncp->xsz;
    ncp->begin_var = D_RNDUP(index, v_align);
    if(ncp->begin_var < index + h_minfree)
    {
      ncp->begin_var = D_RNDUP(index + (off_t)h_minfree, v_align);
    }
  }
  index = ncp->begin_var;

  /* loop thru vars, first pass is for the 'non-record' vars */
  vpp = ncp->vars.value;
  for(ii = 0; ii < ncp->vars.nelems ; ii++, vpp++)
  {
    if( IS_RECVAR(*vpp) )
    {
      /* skip record variables on this pass */
      continue;
    }
#if 0
fprintf(stderr, "    VAR %d %s: %ld\n", ii, (*vpp)->name->cp, (long)index);
#endif
                if( sizeof_off_t == 4 && (index > X_OFF_MAX || index < 0) )
    {
        return NC_EVARSIZE;
                }
    (*vpp)->begin = index;
    index += (*vpp)->len;
  }

  /* only (re)calculate begin_rec if there is not sufficient
     space at end of non-record variables or if start of record
     variables is not aligned as requested by r_align */
  if (ncp->begin_rec < index + v_minfree ||
      ncp->begin_rec != D_RNDUP(ncp->begin_rec, r_align) )
  {
    ncp->begin_rec = D_RNDUP(index, r_align);
    if(ncp->begin_rec < index + v_minfree)
    {
      ncp->begin_rec = D_RNDUP(index + (off_t)v_minfree, r_align);
    }
  }
  index = ncp->begin_rec;

  ncp->recsize = 0;

  /* loop thru vars, second pass is for the 'record' vars */
  vpp = (NC_var **)ncp->vars.value;
  for(ii = 0; ii < ncp->vars.nelems; ii++, vpp++)
  {
    if( !IS_RECVAR(*vpp) )
    {
      /* skip non-record variables on this pass */
      continue;
    }

#if 0
fprintf(stderr, "    REC %d %s: %ld\n", ii, (*vpp)->name->cp, (long)index);
#endif
                if( sizeof_off_t == 4 && (index > X_OFF_MAX || index < 0) )
    {
        return NC_EVARSIZE;
                }
    (*vpp)->begin = index;
    index += (*vpp)->len;
    /* check if record size must fit in 32-bits */
#if SIZEOF_OFF_T == SIZEOF_SIZE_T && SIZEOF_SIZE_T == 4
    if( ncp->recsize > X_UINT_MAX - (*vpp)->len )
    {
        return NC_EVARSIZE;
    }
#endif
    ncp->recsize += (*vpp)->len;
    last = (*vpp);
  }

  /*
   * for special case of exactly one record variable, pack value
   */
  if(last != NULL && ncp->recsize == last->len)
    ncp->recsize = *last->dsizes * last->xsz;

  if(NC_IsNew(ncp))
    NC_set_numrecs(ncp, 0);
  return NC_NOERR;
}


/*
 * Read just the numrecs member.
 * (A relatively expensive way to do things.)
 */
int
read_numrecs(NC *ncp)
{
  int status = NC_NOERR;
  const void *xp = NULL;
  size_t nrecs = NC_get_numrecs(ncp);

  assert(!NC_indef(ncp));

#define NC_NUMRECS_OFFSET 4
#define NC_NUMRECS_EXTENT 4
  status = ncp->nciop->get(ncp->nciop,
     NC_NUMRECS_OFFSET, NC_NUMRECS_EXTENT, 0, (void **)&xp);
          /* cast away const */
  if(status != NC_NOERR)
    return status;

  status = ncx_get_size_t(&xp, &nrecs);

  (void) ncp->nciop->rel(ncp->nciop, NC_NUMRECS_OFFSET, 0);

  if(status == NC_NOERR)
  {
    NC_set_numrecs(ncp, nrecs);
    fClr(ncp->flags, NC_NDIRTY);
  }

  return status;
}


/*
 * Write out just the numrecs member.
 * (A relatively expensive way to do things.)
 */
int
write_numrecs(NC *ncp)
{
  int status = NC_NOERR;
  void *xp = NULL;

  assert(!NC_readonly(ncp));
  assert(!NC_indef(ncp));

  status = ncp->nciop->get(ncp->nciop,
     NC_NUMRECS_OFFSET, NC_NUMRECS_EXTENT, RGN_WRITE, &xp);
  if(status != NC_NOERR)
    return status;

  {
    const size_t nrecs = NC_get_numrecs(ncp);
    status = ncx_put_size_t(&xp, &nrecs);
  }

  (void) ncp->nciop->rel(ncp->nciop, NC_NUMRECS_OFFSET, RGN_MODIFIED);

  if(status == NC_NOERR)
    fClr(ncp->flags, NC_NDIRTY);

  return status;
}


/*
 * Read in the header
 * It is expensive.
 */
static int
read_NC(NC *ncp)
{
  int status = NC_NOERR;

  free_NC_dimarrayV(&ncp->dims);
  free_NC_attrarrayV(&ncp->attrs);
  free_NC_vararrayV(&ncp->vars);

  status = nc_get_NC(ncp);

  if(status == NC_NOERR)
    fClr(ncp->flags, NC_NDIRTY | NC_HDIRTY);

  return status;
}


/*
 * Write out the header
 */
static int
write_NC(NC *ncp)
{
  int status = NC_NOERR;

  assert(!NC_readonly(ncp));

  status = ncx_put_NC(ncp, NULL, 0, 0);

  if(status == NC_NOERR)
    fClr(ncp->flags, NC_NDIRTY | NC_HDIRTY);

  return status;
}


/*
 * Write the header or the numrecs if necessary.
 */
int
NC_sync(NC *ncp)
{
  assert(!NC_readonly(ncp));

  if(NC_hdirty(ncp))
  {
    return write_NC(ncp);
  }
  /* else */

  if(NC_ndirty(ncp))
  {
    return write_numrecs(ncp);
  }
  /* else */

  return NC_NOERR;
}


/*
 * Initialize the 'non-record' variables.
 */
static int
fillerup(NC *ncp)
{
  int status = NC_NOERR;
  size_t ii;
  NC_var **varpp;

  assert(!NC_readonly(ncp));
  assert(NC_dofill(ncp));

  /* loop thru vars */
  varpp = ncp->vars.value;
  for(ii = 0; ii < ncp->vars.nelems; ii++, varpp++)
  {
    if(IS_RECVAR(*varpp))
    {
      /* skip record variables */
      continue;
    }

    status = fill_NC_var(ncp, *varpp, (*varpp)->len, 0);
    if(status != NC_NOERR)
      break;
  }
  return status;
}

/* Begin endef */

/*
 */
static int
fill_added_recs(NC *gnu, NC *old)
{
  NC_var ** const gnu_varpp = (NC_var **)gnu->vars.value;

  const int old_nrecs = (int) NC_get_numrecs(old);
  int recno = 0;
  NC_var **vpp = gnu_varpp;
  NC_var *const *const end = &vpp[gnu->vars.nelems];
  int numrecvars = 0;

  /* Determine if there is only one record variable.  If so, we
     must treat as a special case because there's no record padding */
  for(; vpp < end; vpp++) {
      if(IS_RECVAR(*vpp)) {
    numrecvars++;
      }
  }

  for(; recno < old_nrecs; recno++)
      {
    int varid = (int)old->vars.nelems;
    for(; varid < (int)gnu->vars.nelems; varid++)
        {
      const NC_var *const gnu_varp = *(gnu_varpp + varid);
      if(!IS_RECVAR(gnu_varp))
          {
        /* skip non-record variables */
        continue;
          }
      /* else */
      {
          size_t varsize = numrecvars == 1 ? gnu->recsize :  gnu_varp->len;
          const int status = fill_NC_var(gnu, gnu_varp, varsize, recno);
          if(status != NC_NOERR)
        return status;
      }
        }
      }
  return NC_NOERR;
}

/*
 */
static int
fill_added(NC *gnu, NC *old)
{
  NC_var ** const gnu_varpp = (NC_var **)gnu->vars.value;
  int varid = (int)old->vars.nelems;

  for(; varid < (int)gnu->vars.nelems; varid++)
  {
    const NC_var *const gnu_varp = *(gnu_varpp + varid);
    if(IS_RECVAR(gnu_varp))
    {
      /* skip record variables */
      continue;
    }
    /* else */
    {
    const int status = fill_NC_var(gnu, gnu_varp, gnu_varp->len, 0);
    if(status != NC_NOERR)
      return status;
    }
  }

  return NC_NOERR;
}


/*
 * Move the records "out".
 * Fill as needed.
 */
static int
move_recs_r(NC *gnu, NC *old)
{
  int status;
  int recno;
  int varid;
  NC_var **gnu_varpp = (NC_var **)gnu->vars.value;
  NC_var **old_varpp = (NC_var **)old->vars.value;
  NC_var *gnu_varp;
  NC_var *old_varp;
  off_t gnu_off;
  off_t old_off;
  const size_t old_nrecs = NC_get_numrecs(old);

  /* Don't parallelize this loop */
  for(recno = (int)old_nrecs -1; recno >= 0; recno--)
  {
  /* Don't parallelize this loop */
  for(varid = (int)old->vars.nelems -1; varid >= 0; varid--)
  {
    gnu_varp = *(gnu_varpp + varid);
    if(!IS_RECVAR(gnu_varp))
    {
      /* skip non-record variables on this pass */
      continue;
    }
    /* else */

    /* else, a pre-existing variable */
    old_varp = *(old_varpp + varid);
    gnu_off = gnu_varp->begin + (off_t)(gnu->recsize * recno);
    old_off = old_varp->begin + (off_t)(old->recsize * recno);

    if(gnu_off == old_off)
      continue;   /* nothing to do */

    assert(gnu_off > old_off);

    status = gnu->nciop->move(gnu->nciop, gnu_off, old_off,
       old_varp->len, 0);

    if(status != NC_NOERR)
      return status;

  }
  }

  NC_set_numrecs(gnu, old_nrecs);

  return NC_NOERR;
}


/*
 * Move the "non record" variables "out".
 * Fill as needed.
 */
static int
move_vars_r(NC *gnu, NC *old)
{
  int status;
  int varid;
  NC_var **gnu_varpp = (NC_var **)gnu->vars.value;
  NC_var **old_varpp = (NC_var **)old->vars.value;
  NC_var *gnu_varp;
  NC_var *old_varp;
  off_t gnu_off;
  off_t old_off;

  /* Don't parallelize this loop */
  for(varid = (int)old->vars.nelems -1;
     varid >= 0; varid--)
  {
    gnu_varp = *(gnu_varpp + varid);
    if(IS_RECVAR(gnu_varp))
    {
      /* skip record variables on this pass */
      continue;
    }
    /* else */

    old_varp = *(old_varpp + varid);
    gnu_off = gnu_varp->begin;
    old_off = old_varp->begin;

    if(gnu_off == old_off)
      continue;   /* nothing to do */

    assert(gnu_off > old_off);

    status = gnu->nciop->move(gnu->nciop, gnu_off, old_off,
       old_varp->len, 0);

    if(status != NC_NOERR)
      return status;

  }

  return NC_NOERR;
}


/*
 * Given a valid ncp, return NC_EVARSIZE if any variable has a bad len
 * (product of non-rec dim sizes too large), else return NC_NOERR.
 */
static int
NC_check_vlens(NC *ncp)
{
    NC_var **vpp;
    /* maximum permitted variable size (or size of one record's worth
       of a record variable) in bytes.  This is different for format 1
       and format 2. */
    size_t vlen_max;
    size_t ii;
    size_t large_vars_count;
    size_t rec_vars_count;
    int last = 0;

    if(ncp->vars.nelems == 0)
  return NC_NOERR;

    if ((ncp->flags & NC_64BIT_OFFSET) && sizeof(off_t) > 4) {
  /* CDF2 format and LFS */
  vlen_max = X_UINT_MAX - 3; /* "- 3" handles rounded-up size */
    } else {
  /* CDF1 format */
  vlen_max = X_INT_MAX - 3;
    }
    /* Loop through vars, first pass is for non-record variables.   */
    large_vars_count = 0;
    rec_vars_count = 0;
    vpp = ncp->vars.value;
    for (ii = 0; ii < ncp->vars.nelems; ii++, vpp++) {
  if( !IS_RECVAR(*vpp) ) {
      last = 0;
      if( NC_check_vlen(*vpp, vlen_max) == 0 ) {
    large_vars_count++;
    last = 1;
      }
  } else {
    rec_vars_count++;
  }
    }
    /* OK if last non-record variable size too large, since not used to
       compute an offset */
    if( large_vars_count > 1) { /* only one "too-large" variable allowed */
      return NC_EVARSIZE;
    }
    /* and it has to be the last one */
    if( large_vars_count == 1 && last == 0) {
      return NC_EVARSIZE;
    }
    if( rec_vars_count > 0 ) {
  /* and if it's the last one, there can't be any record variables */
  if( large_vars_count == 1 && last == 1) {
      return NC_EVARSIZE;
  }
  /* Loop through vars, second pass is for record variables.   */
  large_vars_count = 0;
  vpp = ncp->vars.value;
  for (ii = 0; ii < ncp->vars.nelems; ii++, vpp++) {
      if( IS_RECVAR(*vpp) ) {
    last = 0;
    if( NC_check_vlen(*vpp, vlen_max) == 0 ) {
        large_vars_count++;
        last = 1;
    }
      }
  }
  /* OK if last record variable size too large, since not used to
     compute an offset */
  if( large_vars_count > 1) { /* only one "too-large" variable allowed */
      return NC_EVARSIZE;
  }
  /* and it has to be the last one */
  if( large_vars_count == 1 && last == 0) {
      return NC_EVARSIZE;
  }
    }
    return NC_NOERR;
}


/*
 *  End define mode.
 *  Common code for ncendef, ncclose(endef)
 *  Flushes I/O buffers.
 */
static int
NC_endef(NC *ncp,
  size_t h_minfree, size_t v_align,
  size_t v_minfree, size_t r_align)
{
  int status = NC_NOERR;

  assert(!NC_readonly(ncp));
  assert(NC_indef(ncp));

  status = NC_check_vlens(ncp);
  if(status != NC_NOERR)
      return status;
  status = NC_begins(ncp, h_minfree, v_align, v_minfree, r_align);
  if(status != NC_NOERR)
      return status;

  if(ncp->old != NULL)
  {
    /* a plain redef, not a create */
    assert(!NC_IsNew(ncp));
    assert(fIsSet(ncp->flags, NC_INDEF));
    assert(ncp->begin_rec >= ncp->old->begin_rec);
    assert(ncp->begin_var >= ncp->old->begin_var);

    if(ncp->vars.nelems != 0)
    {
    if(ncp->begin_rec > ncp->old->begin_rec)
    {
      status = move_recs_r(ncp, ncp->old);
      if(status != NC_NOERR)
        return status;
      if(ncp->begin_var > ncp->old->begin_var)
      {
        status = move_vars_r(ncp, ncp->old);
        if(status != NC_NOERR)
          return status;
      }
      /* else if (ncp->begin_var == ncp->old->begin_var) { NOOP } */
    }
    else
    {  /* Even if (ncp->begin_rec == ncp->old->begin_rec)
         and     (ncp->begin_var == ncp->old->begin_var)
         might still have added a new record variable */
            if(ncp->recsize > ncp->old->recsize)
      {
              status = move_recs_r(ncp, ncp->old);
        if(status != NC_NOERR)
              return status;
      }
    }
    }
  }

  status = write_NC(ncp);
  if(status != NC_NOERR)
    return status;

  if(NC_dofill(ncp))
  {
    if(NC_IsNew(ncp))
    {
      status = fillerup(ncp);
      if(status != NC_NOERR)
        return status;

    }
    else if(ncp->vars.nelems > ncp->old->vars.nelems)
    {
      status = fill_added(ncp, ncp->old);
      if(status != NC_NOERR)
        return status;
      status = fill_added_recs(ncp, ncp->old);
      if(status != NC_NOERR)
        return status;
    }
  }

  if(ncp->old != NULL)
  {
    free_NC(ncp->old);
    ncp->old = NULL;
  }

  fClr(ncp->flags, NC_CREAT | NC_INDEF);

  return ncp->nciop->sync(ncp->nciop);
}

#ifdef LOCKNUMREC
static int
NC_init_pe(NC *ncp, int basepe) {
  if (basepe < 0 || basepe >= _num_pes()) {
    return NC_EINVAL; /* invalid base pe */
  }
  /* initialize common values */
  ncp->lock[LOCKNUMREC_VALUE] = 0;
  ncp->lock[LOCKNUMREC_LOCK] = 0;
  ncp->lock[LOCKNUMREC_SERVING] = 0;
  ncp->lock[LOCKNUMREC_BASEPE] =  basepe;
  return NC_NOERR;
}
#endif


/*
 * Compute the expected size of the file.
 */
int
NC_calcsize(NC *ncp, off_t *calcsizep)
{
  NC_var **vpp = (NC_var **)ncp->vars.value;
  NC_var *const *const end = &vpp[ncp->vars.nelems];
  NC_var *last_fix = NULL;  /* last "non-record" var */
  int status;
  int numrecvars = 0;  /* number of record variables */

  if(ncp->vars.nelems == 0) { /* no non-record variables and
               no record variables */
      *calcsizep = ncp->xsz; /* size of header */
      return NC_NOERR;
  }

  for( /*NADA*/; vpp < end; vpp++) {
      status = NC_var_shape(*vpp, &ncp->dims);
      if(status != NC_NOERR)
    return status;
      if(IS_RECVAR(*vpp)) {
    numrecvars++;
      } else {
    last_fix = *vpp;
      }
  }

  if(numrecvars == 0) {
      assert(last_fix != NULL);
      *calcsizep = last_fix->begin + last_fix->len;
      /*last_var = last_fix;*/
  } else {       /* we have at least one record variable */
      *calcsizep = ncp->begin_rec + ncp->numrecs * ncp->recsize;
  }

  return NC_NOERR;
}

/* Public */

int
nc__create(const char * path, int ioflags, size_t initialsz,
  size_t *chunksizehintp, int *ncid_ptr)
{
  return nc__create_mp(path, ioflags, initialsz, 0,
    chunksizehintp, ncid_ptr);
}

int
RENAME(_create_mp)(const char * path, int ioflags, size_t initialsz, int basepe,
  size_t *chunksizehintp, int *ncid_ptr)
{
  NC *ncp;
  int status;
  void *xp = NULL;
  int sizeof_off_t = 0;

#if ALWAYS_NC_SHARE /* DEBUG */
  fSet(ioflags, NC_SHARE);
#endif

  ncp = new_NC(chunksizehintp);
  if(ncp == NULL)
    return NC_ENOMEM;

#if defined(LOCKNUMREC) /* && _CRAYMPP */
  if (status = NC_init_pe(ncp, basepe)) {
    return status;
  }
#else
  /*
   * !_CRAYMPP, only pe 0 is valid
   */
  if(basepe != 0)
    return NC_EINVAL;
#endif

  assert(ncp->flags == 0);

  /* Apply default create format. */
  if (default_create_format == NC_FORMAT_64BIT)
    ioflags |= NC_64BIT_OFFSET;

  if (fIsSet(ioflags, NC_64BIT_OFFSET)) {
    fSet(ncp->flags, NC_64BIT_OFFSET);
    sizeof_off_t = 8;
  } else {
    sizeof_off_t = 4;
  }

  assert(ncp->xsz == ncx_len_NC(ncp,sizeof_off_t));

  status = ncio_create(path, ioflags,
    initialsz,
    0, ncp->xsz, &ncp->chunk,
    &ncp->nciop, &xp);
  if(status != NC_NOERR)
  {
    /* translate error status */
    if(status == EEXIST)
      status = NC_EEXIST;
    goto unwind_alloc;
  }

  fSet(ncp->flags, NC_CREAT);

  if(fIsSet(ncp->nciop->ioflags, NC_SHARE))
  {
    /*
     * NC_SHARE implies sync up the number of records as well.
     * (File format version one.)
     * Note that other header changes are not shared
     * automatically.  Some sort of IPC (external to this package)
     * would be used to trigger a call to nc_sync().
     */
    fSet(ncp->flags, NC_NSYNC);
  }

  status = ncx_put_NC(ncp, &xp, sizeof_off_t, ncp->xsz);
  if(status != NC_NOERR)
    goto unwind_ioc;

  add_to_NCList(ncp);

  if(chunksizehintp != NULL)
    *chunksizehintp = ncp->chunk;
  *ncid_ptr = ncp->nciop->fd;
  return NC_NOERR;

unwind_ioc:
  (void) ncio_close(ncp->nciop, 1); /* N.B.: unlink */
  ncp->nciop = NULL;
  /*FALLTHRU*/
unwind_alloc:
  free_NC(ncp);
  return status;
}

/* This function sets a default create flag that will be logically
   or'd to whatever flags are passed into nc_create for all future
   calls to nc_create.
   Valid default create flags are NC_64BIT_OFFSET, NC_CLOBBER,
   NC_LOCK, NC_SHARE. */
int
nc_set_default_format(int format, int *old_formatp)
{
    /* Return existing format if desired. */
    if (old_formatp)
      *old_formatp = default_create_format;

    /* Make sure only valid format is set. */
#ifdef USE_NETCDF4
    if (format != NC_FORMAT_CLASSIC && format != NC_FORMAT_64BIT &&
  format != NC_FORMAT_NETCDF4 && format != NC_FORMAT_NETCDF4_CLASSIC)
      return NC_EINVAL;
#else
    if (format != NC_FORMAT_CLASSIC && format != NC_FORMAT_64BIT)
      return NC_EINVAL;
#endif
    default_create_format = format;
    return NC_NOERR;
}

int
nc_create(const char * path, int ioflags, int *ncid_ptr)
{
  return nc__create(path, ioflags, 0, NULL, ncid_ptr);
}

int
nc__open(const char * path, int ioflags,
  size_t *chunksizehintp, int *ncid_ptr)
{
  return nc__open_mp(path, ioflags, 0,
    chunksizehintp, ncid_ptr);
}

int
RENAME(_open_mp)(const char * path, int ioflags, int basepe,
  size_t *chunksizehintp, int *ncid_ptr)
{
  NC *ncp;
  int status;

#if ALWAYS_NC_SHARE /* DEBUG */
  fSet(ioflags, NC_SHARE);
#endif

  ncp = new_NC(chunksizehintp);
  if(ncp == NULL)
    return NC_ENOMEM;

#if defined(LOCKNUMREC) /* && _CRAYMPP */
  if (status = NC_init_pe(ncp, basepe)) {
    return status;
  }
#else
  /*
   * !_CRAYMPP, only pe 0 is valid
   */
  if(basepe != 0)
    return NC_EINVAL;
#endif

  status = ncio_open(path, ioflags,
    0, 0, &ncp->chunk,
    &ncp->nciop, 0);
  if(status)
    goto unwind_alloc;

  assert(ncp->flags == 0);

  if(fIsSet(ncp->nciop->ioflags, NC_SHARE))
  {
    /*
     * NC_SHARE implies sync up the number of records as well.
     * (File format version one.)
     * Note that other header changes are not shared
     * automatically.  Some sort of IPC (external to this package)
     * would be used to trigger a call to nc_sync().
     */
    fSet(ncp->flags, NC_NSYNC);
  }

  status = nc_get_NC(ncp);
  if(status != NC_NOERR)
    goto unwind_ioc;

  add_to_NCList(ncp);

  if(chunksizehintp != NULL)
    *chunksizehintp = ncp->chunk;
  *ncid_ptr = ncp->nciop->fd;
  return NC_NOERR;

unwind_ioc:
  (void) ncio_close(ncp->nciop, 0);
  ncp->nciop = NULL;
  /*FALLTHRU*/
unwind_alloc:
  free_NC(ncp);
  return status;
}

int
nc_open(const char * path, int ioflags, int *ncid_ptr)
{
  return nc__open(path, ioflags, NULL, ncid_ptr);
}


int
RENAME(_enddef)(int ncid,
  size_t h_minfree, size_t v_align,
  size_t v_minfree, size_t r_align)
{
  int status;
  NC *ncp;

  status = NC_check_id(ncid, &ncp);
  if(status != NC_NOERR)
    return status;

  if(!NC_indef(ncp))
    return(NC_ENOTINDEFINE);

  return (NC_endef(ncp, h_minfree, v_align, v_minfree, r_align));
}

int
RENAME(enddef)(int ncid)
{
  int status;
  NC *ncp;

  status = NC_check_id(ncid, &ncp);
  if(status != NC_NOERR)
    return status;

  if(!NC_indef(ncp))
    return(NC_ENOTINDEFINE);

  /* return(NC_endef(ncp, 0, 4096, 0, 4096)); */
  return (NC_endef(ncp, 0, 1, 0, 1));
}


int
RENAME(close)(int ncid)
{
  int status = NC_NOERR;
  NC *ncp;

  status = NC_check_id(ncid, &ncp);
  if(status != NC_NOERR)
    return status;

  if(NC_indef(ncp))
  {
    status = NC_endef(ncp, 0, 1, 0, 1); /* TODO: defaults */
    if(status != NC_NOERR )
    {
      (void) nc_abort(ncid);
      return status;
    }
  }
  else if(!NC_readonly(ncp))
  {
    status = NC_sync(ncp);
    /* flush buffers before any filesize comparisons */
    (void) ncp->nciop->sync(ncp->nciop);
  }

  /*
   * If file opened for writing and filesize is less than
   * what it should be (due to previous use of NOFILL mode),
   * pad it to correct size, as reported by NC_calcsize().
   */
  if (status == ENOERR) {
      off_t filesize;   /* current size of open file */
      off_t calcsize;  /* calculated file size, from header */
      status = ncio_filesize(ncp->nciop, &filesize);
      if(status != ENOERR)
    return status;
      status = NC_calcsize(ncp, &calcsize);
      if(status != NC_NOERR)
    return status;
      if(filesize < calcsize && !NC_readonly(ncp)) {
    status = ncio_pad_length(ncp->nciop, calcsize);
    if(status != ENOERR)
        return status;
      }
  }

  (void) ncio_close(ncp->nciop, 0);
  ncp->nciop = NULL;

  del_from_NCList(ncp);

  free_NC(ncp);

  return status;
}


int
nc_delete(const char * path)
{
  return nc_delete_mp(path, 0);
}

int
nc_delete_mp(const char * path, int basepe)
{
  NC *ncp;
  int status;
  size_t chunk = 512;

  ncp = new_NC(&chunk);
  if(ncp == NULL)
    return NC_ENOMEM;

#if defined(LOCKNUMREC) /* && _CRAYMPP */
  if (status = NC_init_pe(ncp, basepe)) {
    return status;
  }
#else
  /*
   * !_CRAYMPP, only pe 0 is valid
   */
  if(basepe != 0)
    return NC_EINVAL;
#endif
  status = ncio_open(path, NC_NOWRITE,
    0, 0, &ncp->chunk,
    &ncp->nciop, 0);
  if(status)
    goto unwind_alloc;

  assert(ncp->flags == 0);

  status = nc_get_NC(ncp);
  if(status != NC_NOERR)
  {
    /* Not a netcdf file, don't delete */
    /* ??? is this the right semantic? what if it was just too big? */
    (void) ncio_close(ncp->nciop, 0);
  }
  else
  {
    /* ncio_close does the unlink */
    status = ncio_close(ncp->nciop, 1); /* ncio_close does the unlink */
  }

  ncp->nciop = NULL;
unwind_alloc:
  free_NC(ncp);
  return status;
}


/*
 * In data mode, same as ncclose.
 * In define mode, restore previous definition.
 * In create, remove the file.
 */
int
RENAME(abort)(int ncid)
{
  int status;
  NC *ncp;
  int doUnlink = 0;

  status = NC_check_id(ncid, &ncp);
  if(status != NC_NOERR)
    return status;

  doUnlink = NC_IsNew(ncp);

  if(ncp->old != NULL)
  {
    /* a plain redef, not a create */
    assert(!NC_IsNew(ncp));
    assert(fIsSet(ncp->flags, NC_INDEF));
    free_NC(ncp->old);
    ncp->old = NULL;
    fClr(ncp->flags, NC_INDEF);
  }
  else if(!NC_readonly(ncp))
  {
    status = NC_sync(ncp);
    if(status != NC_NOERR)
      return status;
  }


  (void) ncio_close(ncp->nciop, doUnlink);
  ncp->nciop = NULL;

  del_from_NCList(ncp);

  free_NC(ncp);

  return NC_NOERR;
}


int
RENAME(redef)(int ncid)
{
  int status;
  NC *ncp;

  status = NC_check_id(ncid, &ncp);
  if(status != NC_NOERR)
    return status;

  if(NC_readonly(ncp))
    return NC_EPERM;

  if(NC_indef(ncp))
    return NC_EINDEFINE;


  if(fIsSet(ncp->nciop->ioflags, NC_SHARE))
  {
    /* read in from disk */
    status = read_NC(ncp);
    if(status != NC_NOERR)
      return status;
  }

  ncp->old = dup_NC(ncp);
  if(ncp->old == NULL)
    return NC_ENOMEM;

  fSet(ncp->flags, NC_INDEF);

  return NC_NOERR;
}


int
DISPNAME(inq)(int ncid,
  int *ndimsp,
  int *nvarsp,
  int *nattsp,
  int *xtendimp)
{
  int status;
  NC *ncp;

  status = NC_check_id(ncid, &ncp);
  if(status != NC_NOERR)
    return status;

  if(ndimsp != NULL)
    *ndimsp = (int) ncp->dims.nelems;
  if(nvarsp != NULL)
    *nvarsp = (int) ncp->vars.nelems;
  if(nattsp != NULL)
    *nattsp = (int) ncp->attrs.nelems;
  if(xtendimp != NULL)
    *xtendimp = find_NC_Udim(&ncp->dims, NULL);

  return NC_NOERR;
}

int
nc_inq_ndims(int ncid, int *ndimsp)
{
  int status;
  NC *ncp;

  status = NC_check_id(ncid, &ncp);
  if(status != NC_NOERR)
    return status;

  if(ndimsp != NULL)
    *ndimsp = (int) ncp->dims.nelems;

  return NC_NOERR;
}

int
nc_inq_nvars(int ncid, int *nvarsp)
{
  int status;
  NC *ncp;

  status = NC_check_id(ncid, &ncp);
  if(status != NC_NOERR)
    return status;

  if(nvarsp != NULL)
    *nvarsp = (int) ncp->vars.nelems;

  return NC_NOERR;
}

int
nc_inq_natts(int ncid, int *nattsp)
{
  int status;
  NC *ncp;

  status = NC_check_id(ncid, &ncp);
  if(status != NC_NOERR)
    return status;

  if(nattsp != NULL)
    *nattsp = (int) ncp->attrs.nelems;

  return NC_NOERR;
}

int
nc_inq_unlimdim(int ncid, int *xtendimp)
{
  int status;
  NC *ncp;

  status = NC_check_id(ncid, &ncp);
  if(status != NC_NOERR)
    return status;

  if(xtendimp != NULL)
    *xtendimp = find_NC_Udim(&ncp->dims, NULL);

  return NC_NOERR;
}


int
RENAME(sync)(int ncid)
{
  int status;
  NC *ncp;

  status = NC_check_id(ncid, &ncp);
  if(status != NC_NOERR)
    return status;

  if(NC_indef(ncp))
    return NC_EINDEFINE;

  if(NC_readonly(ncp))
  {
    return read_NC(ncp);
  }
  /* else, read/write */

  status = NC_sync(ncp);
  if(status != NC_NOERR)
    return status;

  status = ncp->nciop->sync(ncp->nciop);
  if(status != NC_NOERR)
    return status;

#ifdef USE_FSYNC
  /* may improve concurrent access, but slows performance if
   * called frequently */
#ifndef WIN32
  status = fsync(ncp->nciop->fd);
#else
  status = _commit(ncp->nciop->fd);
#endif  /* WIN32 */
#endif  /* USE_FSYNC */

  return status;
}


int
DISPNAME(set_fill)(int ncid,
  int fillmode, int *old_mode_ptr)
{
  int status;
  NC *ncp;
  int oldmode;

  status = NC_check_id(ncid, &ncp);
  if(status != NC_NOERR)
    return status;

  if(NC_readonly(ncp))
    return NC_EPERM;

  oldmode = fIsSet(ncp->flags, NC_NOFILL) ? NC_NOFILL : NC_FILL;

  if(fillmode == NC_NOFILL)
  {
    fSet(ncp->flags, NC_NOFILL);
  }
  else if(fillmode == NC_FILL)
  {
    if(fIsSet(ncp->flags, NC_NOFILL))
    {
      /*
       * We are changing back to fill mode
       * so do a sync
       */
      status = NC_sync(ncp);
      if(status != NC_NOERR)
        return status;
    }
    fClr(ncp->flags, NC_NOFILL);
  }
  else
  {
    return NC_EINVAL; /* Invalid fillmode */
  }

  if(old_mode_ptr != NULL)
    *old_mode_ptr = oldmode;

  return NC_NOERR;
}

#ifdef LOCKNUMREC

/* create function versions of the NC_*_numrecs macros */
size_t NC_get_numrecs(const NC *ncp) {
  shmem_t numrec;
  shmem_short_get(&numrec, (shmem_t *) ncp->lock + LOCKNUMREC_VALUE, 1,
    ncp->lock[LOCKNUMREC_BASEPE]);
  return (size_t) numrec;
}

void NC_set_numrecs(NC *ncp, size_t nrecs) {
  shmem_t numrec = (shmem_t) nrecs;
  /* update local value too */
  ncp->lock[LOCKNUMREC_VALUE] = (ushmem_t) numrec;
  shmem_short_put((shmem_t *) ncp->lock + LOCKNUMREC_VALUE, &numrec, 1,
    ncp->lock[LOCKNUMREC_BASEPE]);
}

void NC_increase_numrecs(NC *ncp, size_t nrecs) {
  /* this is only called in one place that's already protected
   * by a lock ... so don't worry about it */
  if (nrecs > NC_get_numrecs(ncp))
    NC_set_numrecs(ncp, nrecs);
}

#endif /* LOCKNUMREC */

/* everyone in communicator group will be executing this */
/*ARGSUSED*/
int
DISPNAME(set_base_pe)(int ncid, int pe)
{
#if _CRAYMPP && defined(LOCKNUMREC)
  int status;
  NC *ncp;
  shmem_t numrecs;

  if ((status = NC_check_id(ncid, &ncp)) != NC_NOERR) {
    return status;
  }
  if (pe < 0 || pe >= _num_pes()) {
    return NC_EINVAL; /* invalid base pe */
  }

  numrecs = (shmem_t) NC_get_numrecs(ncp);

  ncp->lock[LOCKNUMREC_VALUE] = (ushmem_t) numrecs;

  /* update serving & lock values for a "smooth" transition */
  /* note that the "real" server will being doing this as well */
  /* as all the rest in the group */
  /* must have syncronization before & after this step */
  shmem_short_get(
    (shmem_t *) ncp->lock + LOCKNUMREC_SERVING,
    (shmem_t *) ncp->lock + LOCKNUMREC_SERVING,
    1, ncp->lock[LOCKNUMREC_BASEPE]);

  shmem_short_get(
    (shmem_t *) ncp->lock + LOCKNUMREC_LOCK,
    (shmem_t *) ncp->lock + LOCKNUMREC_LOCK,
    1, ncp->lock[LOCKNUMREC_BASEPE]);

  /* complete transition */
  ncp->lock[LOCKNUMREC_BASEPE] = (ushmem_t) pe;

#endif /* _CRAYMPP && LOCKNUMREC */
  return NC_NOERR;
}

/*ARGSUSED*/
int
DISPNAME(inq_base_pe)(int ncid, int *pe)
{
#if _CRAYMPP && defined(LOCKNUMREC)
  int status;
  NC *ncp;

  if ((status = NC_check_id(ncid, &ncp)) != NC_NOERR) {
    return status;
  }

  *pe = (int) ncp->lock[LOCKNUMREC_BASEPE];
#else
  /*
   * !_CRAYMPP, only pe 0 is valid
   */
  *pe = 0;
#endif /* _CRAYMPP && LOCKNUMREC */
  return NC_NOERR;
}

int
DISPNAME(inq_format)(int ncid, int *formatp)
{
  int status;
  NC *ncp;

  status = NC_check_id(ncid, &ncp);
  if(status != NC_NOERR)
    return status;

  /* only need to check for netCDF-3 variants, since this is never called for netCDF-4
     files */
  *formatp = fIsSet(ncp->flags, NC_64BIT_OFFSET) ? NC_FORMAT_64BIT
      : NC_FORMAT_CLASSIC;
  return NC_NOERR;
}


#ifdef USE_DAP
/*
Following routines need to be externally accessible by drno code
1. add_to_NCList
2. del_from_NCList
3. free_NC
4. new_NC
Also need to be able to set the size of unlimited.
=> extra function
*/

void
drno_add_to_NCList(NC *ncp)
{
    add_to_NCList(ncp);
}

void
drno_del_from_NCList(NC *ncp)
{
    del_from_NCList(ncp);
}

void
drno_free_NC(NC *ncp)
{
    free_NC(ncp);
}

NC *
drno_new_NC(const size_t *chunkp)
{
    return new_NC(chunkp);
}

void
drno_set_numrecs(NC* ncp, size_t size)
{
    NC_set_numrecs(ncp,size);
}

size_t
drno_get_numrecs(NC* ncp)
{
    return NC_get_numrecs(ncp);
}

int
drno_ncio_open(NC* ncp, const char* path, int mode)
{
    int ncstat;
    ncio* nciop = NULL;
    size_t sizehint = NC_SIZEHINT_DEFAULT;
    off_t igeto = 0;
    size_t igetsz = 0;
    void* ignore = (void*)17;
    ncstat = ncio_open(path,mode,igeto,igetsz,&sizehint,&nciop,&ignore);
    if(ncstat != NC_NOERR) nciop = NULL;
    ncp->nciop = nciop;
    return ncstat;
}

#endif

#ifndef USE_NETCDF4

/* The sizes of types may vary from platform to platform, but within
 * netCDF files, type sizes are fixed. */
#define NC_BYTE_LEN 1
#define NC_CHAR_LEN 1
#define NC_SHORT_LEN 2
#define NC_INT_LEN 4
#define NC_FLOAT_LEN 4
#define NC_DOUBLE_LEN 8
#define NUM_ATOMIC_TYPES 6

/* This netCDF-4 function proved so popular that a netCDF-classic
 * version is provided. You're welcome. */
int
DISPNAME(inq_type)(int ncid, nc_type typeid, char *name, size_t *size)
{
   int atomic_size[NUM_ATOMIC_TYPES] = {NC_BYTE_LEN, NC_CHAR_LEN, NC_SHORT_LEN,
          NC_INT_LEN, NC_FLOAT_LEN, NC_DOUBLE_LEN};
   char atomic_name[NUM_ATOMIC_TYPES][NC_MAX_NAME + 1] = {"byte", "char", "short",
                "int", "float", "double"};

   /* Only netCDF classic model needs to be handled. */
   if (typeid < NC_BYTE || typeid > NC_DOUBLE)
      return NC_EBADTYPE;

   /* Give the user the values they want. Subtract one because types
    * are numbered starting at 1, not 0. */
   if (name)
      strcpy(name, atomic_name[typeid - 1]);
   if (size)
      *size = atomic_size[typeid - 1];

   return NC_NOERR;
}

#endif /* USE_NETCDF4 */
