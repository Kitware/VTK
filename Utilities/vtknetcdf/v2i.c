/*
 *  Copyright 1996, University Corporation for Atmospheric Research
 *      See netcdf/COPYRIGHT file for copying and redistribution conditions.
 */
/* Id */

#include "nc.h"

#if SIZEOF_LONG == SIZEOF_SIZE_T
/*
 * We don't have to copy the arguments to switch from 'long'
 * to 'size_t' or 'ptrdiff_t'. Use dummy macros.
 */

# define NDIMS_DECL
# define A_DECL(name, type, ndims, rhs) \
  const type *const name = ((const type *)(rhs))

# define A_FREE(name)

# define A_INIT(lhs, type, ndims, rhs)
  
#else 
/*
 * We do have to copy the arguments to switch from 'long'
 * to 'size_t' or 'ptrdiff_t'. In my tests on an SGI,
 * any additional cost was lost in measurement variation.
 */

# include "onstack.h"

static size_t
nvdims(int ncid, int varid)
{
  NC *ncp;
  if(NC_check_id(ncid, &ncp) != NC_NOERR)
    return 0;
  {
    const NC_var *const varp = NC_lookupvar(ncp, varid);
    if(varp == NULL)
      return 0;
    return varp->ndims;
  }
}

#define NDIMS_DECL  const size_t ndims = nvdims(ncid, varid);

# define A_DECL(name, type, ndims, rhs) \
  ALLOC_ONSTACK(name, type, ndims)

# define A_FREE(name) \
  FREE_ONSTACK(name)

# define A_INIT(lhs, type, ndims, rhs) \
  { \
    const long *lp = rhs; \
    type *tp = lhs; \
    type *const end = lhs + ndims; \
    while(tp < end) \
    { \
      *tp++ = (type) *lp++; \
    } \
  }


#endif


/* Begin globals */

/*
 * Error code
 */
int ncerr = NC_NOERR ;


/*
 * The subroutines in error.c emit no messages unless NC_VERBOSE bit is on.
 * They call exit() when NC_FATAL bit is on.
 */
int ncopts = (NC_FATAL | NC_VERBOSE) ;


/*
 * Backward compatibility for the version 2 fortran jackets
 */
const char *cdf_routine_name;


/* End globals */

/* Begin error handling */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>


/*
 */
void
nc_advise(const char *routine_name, int err, const char *fmt,...)
{
  va_list args;

  if(NC_ISSYSERR(err))
    ncerr = NC_SYSERR;
  else
    ncerr = err;

  if( ncopts & NC_VERBOSE )
  {
    (void) fprintf(stderr,"%s: ", routine_name);
    va_start(args ,fmt);
    (void) vfprintf(stderr,fmt,args);
    va_end(args);
    if(err != NC_NOERR)
    {
      (void) fprintf(stderr,": %s",
        nc_strerror(err));
    }
    (void) fputc('\n',stderr);
    (void) fflush(stderr);  /* to ensure log files are current */
  }

  if( (ncopts & NC_FATAL) && err != NC_NOERR )
  {
    exit(ncopts);
  }
}


/*
 * Backward compatibility for the version 2 fortran jackets
 */
void
NCadvise(int err, char *fmt,...)
{
  va_list args;

  va_start(args ,fmt);
  nc_advise(cdf_routine_name, err, fmt, args);
  va_end(args);
}

/* End error handling */


int
nccreate(const char* path, int cmode)
{
  int ncid;
  const int status = nc_create(path, cmode, &ncid);
  if(status != NC_NOERR)
  {
    nc_advise("nccreate", status, "filename \"%s\"", path);
    return -1;
  }
  return ncid;
}


int
ncopen(const char *path, int mode)
{
  int ncid;
  const int status = nc_open(path, mode, &ncid);
  if(status != NC_NOERR)
  {
    nc_advise("ncopen", status, "filename \"%s\"", path);
    return -1;
  }
  return ncid;
}


int
ncredef(int ncid)
{
  const int status =  nc_redef(ncid);
  if(status != NC_NOERR)
  {
    nc_advise("ncredef", status, "ncid %d", ncid);
    return -1;
  }
  return 0;
}


int
ncendef(int ncid)
{
  const int status = nc_enddef(ncid);
  if(status != NC_NOERR)
  {
    nc_advise("ncendef", status, "ncid %d", ncid);
    return -1;
  }
  return 0;
}


int
ncclose(int ncid)
{
  const int status = nc_close(ncid);
  if(status != NC_NOERR)
  {
    nc_advise("ncclose", status, "ncid %d", ncid);
    return -1;
    
  }
  return 0;
}


int
ncinquire(
    int    ncid,
    int*  ndims,
    int*  nvars,
    int*  natts, 
    int*  recdim
)
{
  int nd, nv, na;
  const int status = nc_inq(ncid, &nd, &nv, &na, recdim);

  if(status != NC_NOERR)
  {
    nc_advise("ncinquire", status, "ncid %d", ncid);
    return -1;
  }
  /* else */

  if(ndims != NULL)
    *ndims = (int) nd;

  if(nvars != NULL)
    *nvars = (int) nv;

  if(natts != NULL)
    *natts = (int) na;

  return ncid;
}


int
ncsync(int ncid)
{
  const int status = nc_sync(ncid);
  if(status != NC_NOERR)
  {
    nc_advise("ncsync", status, "ncid %d", ncid);
    return -1;
    
  }
  return 0;
}


int
ncabort(int ncid)
{
  const int status = nc_abort(ncid);
  if(status != NC_NOERR)
  {
    nc_advise("ncabort", status, "ncid %d", ncid);
    return -1;
  }
  return 0;
}


int
ncdimdef(
    int    ncid,
    const char*  name,
    long  length
)
{
  int dimid;
  const int status =  nc_def_dim(ncid, name, (size_t)length, &dimid);
  if(status != NC_NOERR)
  {
    nc_advise("ncdimdef", status, "ncid %d", ncid);
    return -1;
  }
  return dimid;
}


int
ncdimid(int ncid, const char*  name)
{
  int dimid;
  const int status =  nc_inq_dimid(ncid, name, &dimid);
  if(status != NC_NOERR)
  {
    nc_advise("ncdimid", status, "ncid %d", ncid);
    return -1;
  }
  return dimid;
}


int
ncdiminq(
    int    ncid,
    int    dimid,
    char*  name,
    long*  length
)
{
  size_t ll;
  const int status = nc_inq_dim(ncid, dimid, name, &ll);

  if(status != NC_NOERR)
  {
    nc_advise("ncdiminq", status, "ncid %d", ncid);
    return -1;
  }
  /* else */
  
  if(length != NULL)
    *length = (int) ll;

  return dimid;
}


int
ncdimrename(
    int    ncid,
    int    dimid,
    const char*  name
)
{
  const int status = nc_rename_dim(ncid, dimid, name);
  if(status != NC_NOERR)
  {
    nc_advise("ncdimrename", status, "ncid %d", ncid);
    return -1;
  }
  return dimid;
}


int
ncvardef(
    int    ncid,
    const char*  name,
    nc_type  datatype, 
    int    ndims,
    const int*  dim
)
{
  int varid = -1;
  const int status = nc_def_var(ncid, name, datatype, ndims, dim, &varid);
  if(status != NC_NOERR)
  {
    nc_advise("ncvardef", status, "ncid %d", ncid);
    return -1;
  }
  return varid;
}


int
ncvarid(
    int    ncid,
    const char*  name
)
{
  int varid = -1;
  const int status = nc_inq_varid(ncid, name, &varid);
  if(status != NC_NOERR)
  {
    nc_advise("ncvarid", status, "ncid %d", ncid);
    return -1;
  }
  return varid;
}


int
ncvarinq(
    int    ncid,
    int    varid,
    char*  name,
    nc_type*  datatype,
    int*  ndims,
    int*  dim,
    int*  natts
)
{
  int nd, na;
  const int status = nc_inq_var(ncid, varid, name, datatype,
     &nd, dim, &na);

  if(status != NC_NOERR)
  {
    nc_advise("ncvarinq", status, "ncid %d", ncid);
    return -1;
  }
  /* else */
  
  if(ndims != NULL)
    *ndims = (int) nd;

  if(natts != NULL)
    *natts = (int) na;

  return varid;
}


EXTERNL int
ncvarput1(
    int    ncid,
    int    varid,
    const long*  index,
    const void*  value
)
{
  NDIMS_DECL
  A_DECL(coordp, size_t, ndims, index);
  A_INIT(coordp, size_t, ndims, index);
  {
  const int status = nc_put_var1(ncid, varid, coordp, value);
  A_FREE(coordp);
  if(status != NC_NOERR)
  {
    nc_advise("ncvarput1", status, "ncid %d", ncid);
    return -1;
  }
  }
  return 0;
}


int
ncvarget1(
    int    ncid,
    int    varid,
    const long*  index,
    void*  value
)
{
  NDIMS_DECL
  A_DECL(coordp, size_t, ndims, index);
  A_INIT(coordp, size_t, ndims, index);
  {
  const int status = nc_get_var1(ncid, varid, coordp, value);
  A_FREE(coordp);
  if(status != NC_NOERR)
  {
    nc_advise("ncdimid", status, "ncid %d", ncid);
    return -1;
  }
  }
  return 0;
}


int
ncvarput(
    int    ncid,
    int    varid,
    const long*  start,
    const long*  count, 
    const void*  value
)
{
  NDIMS_DECL
  A_DECL(stp, size_t, ndims, start);
  A_DECL(cntp, size_t, ndims, count);
  A_INIT(stp, size_t, ndims, start);
  A_INIT(cntp, size_t, ndims, count);
  {
  const int status = nc_put_vara(ncid, varid, stp, cntp, value);
  A_FREE(cntp);
  A_FREE(stp);
  if(status != NC_NOERR)
  {
    nc_advise("ncvarput", status, "ncid %d", ncid);
    return -1;
  }
  }
  return 0;
}


int
ncvarget(
    int    ncid,
    int    varid,
    const long*  start,
    const long*  count, 
    void*  value
)
{
  NDIMS_DECL
  A_DECL(stp, size_t, ndims, start);
  A_DECL(cntp, size_t, ndims, count);
  A_INIT(stp, size_t, ndims, start);
  A_INIT(cntp, size_t, ndims, count);
  {
  const int status = nc_get_vara(ncid, varid, stp, cntp, value);
  A_FREE(cntp);
  A_FREE(stp);
  if(status != NC_NOERR)
  {
    nc_advise("ncvarget", status, "ncid %d", ncid);
    return -1;
  }
  }
  return 0;
}


int
ncvarputs(
    int    ncid,
    int    varid,
    const long*  start,
    const long*  count,
    const long*  stride,
    const void*  value
)
{
  if(stride == NULL)
    return ncvarput(ncid, varid, start, count, value);
  /* else */
  {
  NDIMS_DECL
  A_DECL(stp, size_t, ndims, start);
  A_DECL(cntp, size_t, ndims, count);
  A_DECL(strdp, ptrdiff_t, ndims, stride);
  A_INIT(stp, size_t, ndims, start);
  A_INIT(cntp, size_t, ndims, count);
  A_INIT(strdp, ptrdiff_t, ndims, stride);
  {
  const int status = nc_put_vars(ncid, varid, stp, cntp, strdp, value);
  A_FREE(strdp);
  A_FREE(cntp);
  A_FREE(stp);
  if(status != NC_NOERR)
  {
    nc_advise("ncvarputs", status, "ncid %d", ncid);
    return -1;
  }
  }
  return 0;
  }
}


int
ncvargets(
    int    ncid,
    int    varid,
    const long*  start,
    const long*  count,
    const long*  stride,
    void*  value
)
{
  if(stride == NULL)
    return ncvarget(ncid, varid, start, count, value);
  /* else */
  {
  NDIMS_DECL
  A_DECL(stp, size_t, ndims, start);
  A_DECL(cntp, size_t, ndims, count);
  A_DECL(strdp, ptrdiff_t, ndims, stride);
  A_INIT(stp, size_t, ndims, start);
  A_INIT(cntp, size_t, ndims, count);
  A_INIT(strdp, ptrdiff_t, ndims, stride);
  {
  const int status = nc_get_vars(ncid, varid, stp, cntp, strdp, value);
  A_FREE(strdp);
  A_FREE(cntp);
  A_FREE(stp);
  if(status != NC_NOERR)
  {
    nc_advise("ncvargets", status, "ncid %d", ncid);
    return -1;
  }
  }
  return 0;
  }
}


int
ncvarputg(
    int    ncid,
    int    varid,
    const long*  start,
    const long*  count,
    const long*  stride,
    const long*  map,
    const void* value
)
{
  if(map == NULL)
    return ncvarputs(ncid, varid, start, count, stride, value);
  /* else */
  {
  NDIMS_DECL
  A_DECL(stp, size_t, ndims, start);
  A_DECL(cntp, size_t, ndims, count);
  A_DECL(strdp, ptrdiff_t, ndims, stride);
  A_DECL(imp, ptrdiff_t, ndims, map);
  A_INIT(stp, size_t, ndims, start);
  A_INIT(cntp, size_t, ndims, count);
  A_INIT(strdp, ptrdiff_t, ndims, stride);
  A_INIT(imp, ptrdiff_t, ndims, map);
  {
  const int status = nc_put_varm(ncid, varid,
       stp, cntp, strdp, imp, value);
  A_FREE(imp);
  A_FREE(strdp);
  A_FREE(cntp);
  A_FREE(stp);
  if(status != NC_NOERR)
  {
    nc_advise("ncvarputg", status, "ncid %d", ncid);
    return -1;
  }
  }
  return 0;
  }
}


int
ncvargetg(
    int    ncid,
    int    varid,
    const long*  start,
    const long*  count,
    const long*  stride,
    const long*  map,
    void*  value
)
{
  if(map == NULL)
    return ncvargets(ncid, varid, start, count, stride, value);
  /* else */
  {
  NDIMS_DECL
  A_DECL(stp, size_t, ndims, start);
  A_DECL(cntp, size_t, ndims, count);
  A_DECL(strdp, ptrdiff_t, ndims, stride);
  A_DECL(imp, ptrdiff_t, ndims, map);
  A_INIT(stp, size_t, ndims, start);
  A_INIT(cntp, size_t, ndims, count);
  A_INIT(strdp, ptrdiff_t, ndims, stride);
  A_INIT(imp, ptrdiff_t, ndims, map);
  {
  const int status = nc_get_varm(ncid, varid,
      stp, cntp, strdp, imp, value);
  A_FREE(imp);
  A_FREE(strdp);
  A_FREE(cntp);
  A_FREE(stp);
  if(status != NC_NOERR)
  {
    nc_advise("ncvargetg", status, "ncid %d", ncid);
    return -1;
  }
  }
  return 0;
  }
}


int
ncvarrename(
    int    ncid,
    int    varid,
    const char*  name
)
{
  const int status = nc_rename_var(ncid, varid, name);
  if(status != NC_NOERR)
  {
    nc_advise("ncvarrename", status, "ncid %d", ncid);
    return -1;
  }
  return varid;
}


int
ncattput(
    int    ncid,
    int    varid,
    const char*  name, 
    nc_type  datatype,
    int    len,
    const void*  value
)
{
  const int status = nc_put_att(ncid, varid, name, datatype, len, value);
  if(status != NC_NOERR)
  {
    nc_advise("ncattput", status, "ncid %d", ncid);
    return -1;
  }
  return 0;
}


int
ncattinq(
    int    ncid,
    int    varid,
    const char*  name, 
    nc_type*  datatype,
    int*  len
)
{
  size_t ll;
  const int status = nc_inq_att(ncid, varid, name, datatype, &ll);
  if(status != NC_NOERR)
  {
    nc_advise("ncattinq", status, "ncid %d", ncid);
    return -1;
  }
  
  if(len != NULL)
    *len = (int) ll;

  return 1;

}


int
ncattget(
    int    ncid,
    int    varid,
    const char*  name, 
    void*  value
)
{
  const int status = nc_get_att(ncid, varid, name, value);
  if(status != NC_NOERR)
  {
    nc_advise("ncattget", status, "ncid %d", ncid);
    return -1;
  }
  return 1;
}


int
ncattcopy(
    int    ncid_in,
    int    varid_in,
    const char*  name, 
    int    ncid_out,
    int    varid_out
)
{
  const int status = nc_copy_att(ncid_in, varid_in, name, ncid_out, varid_out);
  if(status != NC_NOERR)
  {
    nc_advise("ncattcopy", status, "%s", name);
    return -1;
  }
  return 0;
}


int
ncattname(
    int    ncid,
    int    varid,
    int    attnum,
    char*  name
)
{
  const int status = nc_inq_attname(ncid, varid, attnum, name);
  if(status != NC_NOERR)
  {
    nc_advise("ncattname", status, "ncid %d", ncid);
    return -1;
  }
  return attnum;
}


int
ncattrename(
    int    ncid,
    int    varid,
    const char*  name, 
    const char*  newname
)
{
  const int status = nc_rename_att(ncid, varid, name, newname);
  if(status != NC_NOERR)
  {
    nc_advise("ncattrename", status, "ncid %d", ncid);
    return -1;
  }
  return 1;
}


int
ncattdel(
    int    ncid,
    int    varid,
    const char*  name
)
{
   const int status = nc_del_att(ncid, varid, name);
  if(status != NC_NOERR)
  {
    nc_advise("ncattdel", status, "ncid %d", ncid);
    return -1;
  }
  return 1;
}


/*
 *  This is how much space is required by the user, as in
 *
 *   vals = malloc(nel * nctypelen(var.type));
 *   ncvarget(cdfid, varid, cor, edg, vals);
 */
int
nctypelen(nc_type type) 
{
  switch(type){
  case NC_BYTE :
  case NC_CHAR :
    return((int)sizeof(char));
  case NC_SHORT :
    return(int)(sizeof(short));
  case NC_INT :
    return((int)sizeof(nclong));
  case NC_FLOAT :
    return((int)sizeof(float));
  case NC_DOUBLE : 
    return((int)sizeof(double));
  }
  /* else */
  nc_advise("nctypelen", NC_EBADTYPE, "Unknown type %d",
    (int)type);
  return -1;
}


int
ncsetfill(
    int    ncid,
    int    fillmode
)
{
  int oldmode = -1;
  const int status = nc_set_fill(ncid, fillmode, &oldmode);
  if(status != NC_NOERR)
  {
    nc_advise("ncsetfill", status, "ncid %d", ncid);
    return -1;
  }
  return oldmode;
}


int
ncrecinq(
    int    ncid,
    int*  nrecvars,
    int*  recvarids,
    long*  recsizes
)
{
  size_t nrv = 0;
  size_t rs[NC_MAX_VARS]; /* TODO */
  const int status = nc_inq_rec(ncid, &nrv, recvarids, rs);
  if(status != NC_NOERR)
  {
    nc_advise("ncrecinq", status, "ncid %d", ncid);
    return -1;
  }

  if(nrecvars != NULL)
    *nrecvars = (int) nrv;

  if(recsizes != NULL)
  {
    size_t ii;
    for(ii = 0; ii < nrv; ii++)
    {
      recsizes[ii] = (long) rs[ii];
    }
  }

  return (int) nrv;
}


int
ncrecget(
    int    ncid,
    long  recnum,
    void**  datap
)
{
  const int status = nc_get_rec(ncid, (size_t)recnum, datap);
  if(status != NC_NOERR)
  {
    nc_advise("ncrecget", status, "ncid %d", ncid);
    return -1;
  }
  return 0;
}


int
ncrecput(
    int    ncid,
    long  recnum,
    void* const* datap
)
{
  const int status = nc_put_rec(ncid, (size_t)recnum, datap);
  if(status != NC_NOERR)
  {
    nc_advise("ncrecput", status, "ncid %d", ncid);
    return -1;
  }
  return 0;
}
