/*
 *  Copyright 1996, University Corporation for Atmospheric Research
 *      See netcdf/COPYRIGHT file for copying and redistribution conditions.
 */
/* $Id: v2i.c,v 1.52 2009/02/20 22:00:46 dmh Exp $ */

#ifndef NO_NETCDF_2

#include <ncconfig.h>
#include <stdlib.h>
#ifndef NO_SYS_TYPES_H
#  include <sys/types.h> /* Keep before netcdf.h or Win64 gets confused. */
#endif /* NO_SYS_TYPES_H */
#include "netcdf.h"

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
# include "nc.h"

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

typedef signed char schar;

/*
 * Computes number of record variables in an open netCDF file, and an array of
 * the record variable ids, if the array parameter is non-null.
 */
static int
numrecvars(int ncid, int *nrecvarsp, int *recvarids)
{
    int status;
    int nvars = 0;
    int ndims = 0;
    int nrecvars = 0;
    int varid;
    int recdimid;
    int dimids[MAX_NC_DIMS];

    status = nc_inq_nvars(ncid, &nvars);
    if(status != NC_NOERR)
  return status;

    status = nc_inq_unlimdim(ncid, &recdimid);
    if(status != NC_NOERR)
  return status;

    if (recdimid == -1) {
  *nrecvarsp = 0;
  return NC_NOERR;
    }
    nrecvars = 0;
    for (varid = 0; varid < nvars; varid++) {
  status = nc_inq_varndims(ncid, varid, &ndims);
  if(status != NC_NOERR)
      return status;
  status = nc_inq_vardimid(ncid, varid, dimids);
  if(status != NC_NOERR)
      return status;
  if (ndims > 0 && dimids[0] == recdimid) {
      if (recvarids != NULL)
        recvarids[nrecvars] = varid;
      nrecvars++;
  }
    }
    *nrecvarsp = nrecvars;
    return NC_NOERR;
}


/*
 * Computes record size (in bytes) of the record variable with a specified
 * variable id.  Returns size as 0 if not a record variable.
 */
static int
ncrecsize(int ncid, int varid, size_t *recsizep)
{
    int status;
    int recdimid;
    nc_type type;
    int ndims;
    int dimids[MAX_NC_DIMS];
    int id;
    size_t size;

    *recsizep = 0;
    status = nc_inq_unlimdim(ncid, &recdimid);
    if(status != NC_NOERR)
  return status;
    status = nc_inq_vartype(ncid, varid, &type);
    if(status != NC_NOERR)
  return status;
    status = nc_inq_varndims(ncid, varid, &ndims);
    if(status != NC_NOERR)
  return status;
    status = nc_inq_vardimid(ncid, varid, dimids);
    if(status != NC_NOERR)
  return status;
    if (ndims == 0 || dimids[0] != recdimid) {
  return NC_NOERR;
    }
    size = nctypelen(type);
    for (id = 1; id < ndims; id++) {
  size_t len;
  status = nc_inq_dimlen(ncid, dimids[id], &len);
  if(status != NC_NOERR)
    return status;
  size *= len;
    }
    *recsizep = size;
    return NC_NOERR;
}


/*
 * Retrieves the dimension sizes of a variable with a specified variable id in
 * an open netCDF file.  Returns -1 on error.
 */
static int
dimsizes(int ncid, int varid, size_t *sizes)
{
    int status;
    int ndims;
    int id;
    int dimids[MAX_NC_DIMS];

    status = nc_inq_varndims(ncid, varid, &ndims);
    if(status != NC_NOERR)
  return status;
    status = nc_inq_vardimid(ncid, varid, dimids);
    if(status != NC_NOERR)
  return status;
    if (ndims == 0 || sizes == NULL)
      return NC_NOERR;
    for (id = 0; id < ndims; id++) {
  size_t len;
  status = nc_inq_dimlen(ncid, dimids[id], &len);
  if(status != NC_NOERR)
    return status;
  sizes[id] = len;
    }
    return NC_NOERR;
}


/*
 * Retrieves the number of record variables, the record variable ids, and the
 * record size of each record variable.  If any pointer to info to be returned
 * is null, the associated information is not returned.  Returns -1 on error.
 */
int
nc_inq_rec(
  int ncid,
  size_t *nrecvarsp,
  int *recvarids,
  size_t *recsizes)
{
    int status;
    int nvars = 0;
    int recdimid;
    int varid;
    int rvarids[MAX_NC_VARS];
    int nrvars = 0;

    status = nc_inq_nvars(ncid, &nvars);
    if(status != NC_NOERR)
  return status;

    status = nc_inq_unlimdim(ncid, &recdimid);
    if(status != NC_NOERR)
  return status;

    *nrecvarsp = 0;
    if (recdimid == -1)
  return NC_NOERR;

    status = numrecvars(ncid, &nrvars, rvarids);
    if(status != NC_NOERR)
  return status;

    if (nrecvarsp != NULL)
  *nrecvarsp = nrvars;
    if (recvarids != NULL)
  for (varid = 0; varid < nrvars; varid++)
      recvarids[varid] = rvarids[varid];

    if (recsizes != NULL)
  for (varid = 0; varid < nrvars; varid++) {
      size_t rsize;
      status = ncrecsize(ncid, rvarids[varid], &rsize);
      if (status != NC_NOERR)
    return status;
      recsizes[varid] = rsize;
  }
  return NC_NOERR;
}


/*
 * Write one record's worth of data, except don't write to variables for which
 * the address of the data to be written is NULL.  Return -1 on error.  This is
 * the same as the ncrecput() in the library, except that can handle errors
 * better.
 */
int
nc_put_rec(
  int ncid,
  size_t recnum,
  void* const* datap)
{
    int status;
    int varid;
    int rvarids[MAX_NC_VARS];
    int nrvars;
    size_t start[MAX_NC_DIMS];
    size_t edges[MAX_NC_DIMS];

    status = numrecvars(ncid, &nrvars, rvarids);
    if(status != NC_NOERR)
  return status;

    if (nrvars == 0)
      return NC_NOERR;

    start[0] = recnum;
    for (varid = 1; varid < nrvars; varid++)
  start[varid] = 0;

    for (varid = 0; varid < nrvars; varid++) {
  if (datap[varid] != NULL) {
      status = dimsizes(ncid, rvarids[varid], edges);
      if(status != NC_NOERR)
    return status;

      edges[0] = 1;    /* only 1 record's worth */
      status = nc_put_vara(ncid, rvarids[varid], start, edges, datap[varid]);
      if(status != NC_NOERR)
    return status;
  }
    }
    return 0;
}


/*
 * Read one record's worth of data, except don't read from variables for which
 * the address of the data to be read is null.  Return -1 on error.  This is
 * the same as the ncrecget() in the library, except that can handle errors
 * better.
 */
int
nc_get_rec(
  int ncid,
  size_t recnum,
  void **datap)
{
    int status;
    int varid;
    int rvarids[MAX_NC_VARS];
    int nrvars;
    size_t start[MAX_NC_DIMS];
    size_t edges[MAX_NC_DIMS];

    status = numrecvars(ncid, &nrvars, rvarids);
    if(status != NC_NOERR)
  return status;

    if (nrvars == 0)
      return NC_NOERR;

    start[0] = recnum;
    for (varid = 1; varid < nrvars; varid++)
  start[varid] = 0;

    for (varid = 0; varid < nrvars; varid++) {
  if (datap[varid] != NULL) {
      status = dimsizes(ncid, rvarids[varid], edges);
      if(status != NC_NOERR)
    return status;
      edges[0] = 1;    /* only 1 record's worth */
      status = nc_get_vara(ncid, rvarids[varid], start, edges, datap[varid]);
      if(status != NC_NOERR)
    return status;
  }
    }
    return 0;
}


/* Begin globals */

/*
 * Error code
 */
#ifndef DLL_NETCDF /* define when library is not a DLL */
int ncerr = NC_NOERR ;


/*
 * The subroutines in error.c emit no messages unless NC_VERBOSE bit is on.
 * They call exit() when NC_FATAL bit is on.
 */
int ncopts = (NC_FATAL | NC_VERBOSE) ;
#endif

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
  int status;
  if(length < 0) {
      status = NC_EDIMSIZE;
      nc_advise("ncdimdef", status, "ncid %d", ncid);
      return -1;
  }
  status =  nc_def_dim(ncid, name, (size_t)length, &dimid);
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


int
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
    nc_advise("ncvarget", status, "ncid %d; varid %d", ncid, varid);
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
    nc_advise("ncattinq", status,
        "ncid %d; varid %d; attname \"%s\"",
        ncid, varid, name);
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

#endif /* NO_NETCDF_2 */

#ifndef NO_NETCDF_2

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

#endif /* NO_NETCDF_2 */
