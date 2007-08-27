/*
 * Copyright 1993-1996 University Corporation for Atmospheric Research/Unidata
 * 
 * Portions of this software were developed by the Unidata Program at the 
 * University Corporation for Atmospheric Research.
 * 
 * Access and use of this software shall impose the following obligations
 * and understandings on the user. The user is granted the right, without
 * any fee or cost, to use, copy, modify, alter, enhance and distribute
 * this software, and any derivative works thereof, and its supporting
 * documentation for any purpose whatsoever, provided that this entire
 * notice appears in all copies of the software, derivative works and
 * supporting documentation.  Further, UCAR requests that the user credit
 * UCAR/Unidata in any publications that result from the use of this
 * software or in any product that includes this software. The names UCAR
 * and/or Unidata, however, may not be used in any advertising or publicity
 * to endorse or promote any products or commercial entity unless specific
 * written permission is obtained from UCAR/Unidata. The user also
 * understands that UCAR/Unidata is not obligated to provide the user with
 * any support, consulting, training or assistance of any kind with regard
 * to the use, operation and performance of this software nor to provide
 * the user with any updates, revisions, new versions or "bug fixes."
 * 
 * THIS SOFTWARE IS PROVIDED BY UCAR/UNIDATA "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL UCAR/UNIDATA BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
 * FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE ACCESS, USE OR PERFORMANCE OF THIS SOFTWARE.
 */
/* "Id" */

#ifndef _NETCDF3_
#define _NETCDF3_

#include <stddef.h> /* size_t, ptrdiff_t */
#include <errno.h>  /* netcdf functions sometimes return system errors */
#include "netcdf.h"

#if defined(__cplusplus)
extern "C" {
#endif
/*
 * The Interface
 */

/* Declaration modifiers for DLL support (MSC et al) */

#if defined(DLL_NETCDF) /* define when library is a DLL */
#  if defined(DLL_EXPORT) /* define when building the library */
#   define MSC_EXTRA __declspec(dllexport)
#  else
#   define MSC_EXTRA __declspec(dllimport)
#  endif
#else
#define MSC_EXTRA
#endif  /* defined(DLL_NETCDF) */

# define EXTERNL extern MSC_EXTRA

EXTERNL const char *
nc3_inq_libvers(void);

EXTERNL const char *
nc3_strerror(int ncerr1);

EXTERNL int
nc3__create(const char *path, int cmode, size_t initialsz,
   size_t *chunksizehintp, int *ncidp);

EXTERNL int
nc3_create(const char *path, int cmode, int *ncidp);

EXTERNL int
nc3__open(const char *path, int mode, 
  size_t *chunksizehintp, int *ncidp);

EXTERNL int
nc3_open(const char *path, int mode, int *ncidp);

EXTERNL int
nc3_set_fill(int ncid, int fillmode, int *old_modep);

EXTERNL int
nc3_set_default_format(int format, int *old_formatp);

EXTERNL int
nc3_inq_format(int ncid, int *formatp);

EXTERNL int
nc3_redef(int ncid);

EXTERNL int
nc3__enddef(int ncid, size_t h_minfree, size_t v_align,
  size_t v_minfree, size_t r_align);

EXTERNL int
nc3_enddef(int ncid);

EXTERNL int
nc3_sync(int ncid);

EXTERNL int
nc3_abort(int ncid);

EXTERNL int
nc3_close(int ncid);

EXTERNL int
nc3_inq(int ncid, int *ndimsp, int *nvarsp, int *nattsp, int *unlimdimidp);

EXTERNL int 
nc3_inq_ndims(int ncid, int *ndimsp);

EXTERNL int 
nc3_inq_nvars(int ncid, int *nvarsp);

EXTERNL int 
nc3_inq_natts(int ncid, int *nattsp);

EXTERNL int 
nc3_inq_unlimdim(int ncid, int *unlimdimidp);

/* Begin _dim */

EXTERNL int
nc3_def_dim(int ncid, const char *name, size_t len, int *idp);

EXTERNL int
nc3_inq_dimid(int ncid, const char *name, int *idp);

EXTERNL int
nc3_inq_dim(int ncid, int dimid, char *name, size_t *lenp);

EXTERNL int 
nc3_inq_dimname(int ncid, int dimid, char *name);

EXTERNL int 
nc3_inq_dimlen(int ncid, int dimid, size_t *lenp);

EXTERNL int
nc3_rename_dim(int ncid, int dimid, const char *name);

/* End _dim */
/* Begin _att */

EXTERNL int
nc3_inq_att(int ncid, int varid, const char *name,
   nc_type *xtypep, size_t *lenp);

EXTERNL int 
nc3_inq_attid(int ncid, int varid, const char *name, int *idp);

EXTERNL int 
nc3_inq_atttype(int ncid, int varid, const char *name, nc_type *xtypep);

EXTERNL int 
nc3_inq_attlen(int ncid, int varid, const char *name, size_t *lenp);

EXTERNL int
nc3_inq_attname(int ncid, int varid, int attnum, char *name);

EXTERNL int
nc3_copy_att(int ncid_in, int varid_in, const char *name, int ncid_out, int varid_out);

EXTERNL int
nc3_rename_att(int ncid, int varid, const char *name, const char *newname);

EXTERNL int
nc3_del_att(int ncid, int varid, const char *name);

/* End _att */
/* Begin {put,get}_att */

EXTERNL int
nc3_put_att_text(int ncid, int varid, const char *name,
  size_t len, const char *op);

EXTERNL int
nc3_get_att_text(int ncid, int varid, const char *name, char *ip);

EXTERNL int
nc3_put_att_uchar(int ncid, int varid, const char *name, nc_type xtype,
  size_t len, const unsigned char *op);

EXTERNL int
nc3_get_att_uchar(int ncid, int varid, const char *name, unsigned char *ip);

EXTERNL int
nc3_put_att_schar(int ncid, int varid, const char *name, nc_type xtype,
  size_t len, const signed char *op);

EXTERNL int
nc3_get_att_schar(int ncid, int varid, const char *name, signed char *ip);

EXTERNL int
nc3_put_att_short(int ncid, int varid, const char *name, nc_type xtype,
  size_t len, const short *op);

EXTERNL int
nc3_get_att_short(int ncid, int varid, const char *name, short *ip);

EXTERNL int
nc3_put_att_int(int ncid, int varid, const char *name, nc_type xtype,
  size_t len, const int *op);

EXTERNL int
nc3_get_att_int(int ncid, int varid, const char *name, int *ip);

EXTERNL int
nc3_put_att_long(int ncid, int varid, const char *name, nc_type xtype,
  size_t len, const long *op);

EXTERNL int
nc3_get_att_long(int ncid, int varid, const char *name, long *ip);

EXTERNL int
nc3_put_att_float(int ncid, int varid, const char *name, nc_type xtype,
  size_t len, const float *op);

EXTERNL int
nc3_get_att_float(int ncid, int varid, const char *name, float *ip);

EXTERNL int
nc3_put_att_double(int ncid, int varid, const char *name, nc_type xtype,
  size_t len, const double *op);

EXTERNL int
nc3_get_att_double(int ncid, int varid, const char *name, double *ip);

/* End {put,get}_att */
/* Begin _var */

EXTERNL int
nc3_def_var(int ncid, const char *name,
   nc_type xtype, int ndims, const int *dimidsp, int *varidp);

EXTERNL int
nc3_inq_var(int ncid, int varid, char *name,
   nc_type *xtypep, int *ndimsp, int *dimidsp, int *nattsp);

EXTERNL int
nc3_inq_varid(int ncid, const char *name, int *varidp);

EXTERNL int 
nc3_inq_varname(int ncid, int varid, char *name);

EXTERNL int 
nc3_inq_vartype(int ncid, int varid, nc_type *xtypep);

EXTERNL int 
nc3_inq_varndims(int ncid, int varid, int *ndimsp);

EXTERNL int 
nc3_inq_vardimid(int ncid, int varid, int *dimidsp);

EXTERNL int 
nc3_inq_varnatts(int ncid, int varid, int *nattsp);

EXTERNL int
nc3_rename_var(int ncid, int varid, const char *name);

EXTERNL int
nc3_copy_var(int ncid_in, int varid, int ncid_out);
#ifndef ncvarcpy
/* support the old name for now */
#define ncvarcpy(ncid_in, varid, ncid_out) ncvarcopy((ncid_in), (varid), (ncid_out))
#endif

/* End _var */
/* Begin {put,get}_var1 */

EXTERNL int
nc3_put_var1_text(int ncid, int varid, const size_t *indexp, const char *op);

EXTERNL int
nc3_get_var1_text(int ncid, int varid, const size_t *indexp, char *ip);

EXTERNL int
nc3_put_var1_uchar(int ncid, int varid, const size_t *indexp,
  const unsigned char *op);

EXTERNL int
nc3_get_var1_uchar(int ncid, int varid, const size_t *indexp,
  unsigned char *ip);

EXTERNL int
nc3_put_var1_schar(int ncid, int varid, const size_t *indexp,
  const signed char *op);

EXTERNL int
nc3_get_var1_schar(int ncid, int varid, const size_t *indexp,
  signed char *ip);

EXTERNL int
nc3_put_var1_short(int ncid, int varid, const size_t *indexp,
  const short *op);

EXTERNL int
nc3_get_var1_short(int ncid, int varid, const size_t *indexp,
  short *ip);

EXTERNL int
nc3_put_var1_int(int ncid, int varid, const size_t *indexp, const int *op);

EXTERNL int
nc3_get_var1_int(int ncid, int varid, const size_t *indexp, int *ip);

EXTERNL int
nc3_put_var1_long(int ncid, int varid, const size_t *indexp, const long *op);

EXTERNL int
nc3_get_var1_long(int ncid, int varid, const size_t *indexp, long *ip);

EXTERNL int
nc3_put_var1_float(int ncid, int varid, const size_t *indexp, const float *op);

EXTERNL int
nc3_get_var1_float(int ncid, int varid, const size_t *indexp, float *ip);

EXTERNL int
nc3_put_var1_double(int ncid, int varid, const size_t *indexp, const double *op);

EXTERNL int
nc3_get_var1_double(int ncid, int varid, const size_t *indexp, double *ip);

/* End {put,get}_var1 */
/* Begin {put,get}_vara */

EXTERNL int
nc3_put_vara_text(int ncid, int varid,
  const size_t *startp, const size_t *countp, const char *op);

EXTERNL int
nc3_get_vara_text(int ncid, int varid,
  const size_t *startp, const size_t *countp, char *ip);

EXTERNL int
nc3_put_vara_uchar(int ncid, int varid,
  const size_t *startp, const size_t *countp, const unsigned char *op);

EXTERNL int
nc3_get_vara_uchar(int ncid, int varid,
  const size_t *startp, const size_t *countp, unsigned char *ip);

EXTERNL int
nc3_put_vara_schar(int ncid, int varid,
  const size_t *startp, const size_t *countp, const signed char *op);

EXTERNL int
nc3_get_vara_schar(int ncid, int varid,
  const size_t *startp, const size_t *countp, signed char *ip);

EXTERNL int
nc3_put_vara_short(int ncid, int varid,
  const size_t *startp, const size_t *countp, const short *op);

EXTERNL int
nc3_get_vara_short(int ncid, int varid,
  const size_t *startp, const size_t *countp, short *ip);

EXTERNL int
nc3_put_vara_int(int ncid, int varid,
  const size_t *startp, const size_t *countp, const int *op);

EXTERNL int
nc3_get_vara_int(int ncid, int varid,
  const size_t *startp, const size_t *countp, int *ip);

EXTERNL int
nc3_put_vara_long(int ncid, int varid,
  const size_t *startp, const size_t *countp, const long *op);

EXTERNL int
nc3_get_vara_long(int ncid, int varid,
  const size_t *startp, const size_t *countp, long *ip);

EXTERNL int
nc3_put_vara_float(int ncid, int varid,
  const size_t *startp, const size_t *countp, const float *op);

EXTERNL int
nc3_get_vara_float(int ncid, int varid,
  const size_t *startp, const size_t *countp, float *ip);

EXTERNL int
nc3_put_vara_double(int ncid, int varid,
  const size_t *startp, const size_t *countp, const double *op);

EXTERNL int
nc3_get_vara_double(int ncid, int varid,
  const size_t *startp, const size_t *countp, double *ip);

/* End {put,get}_vara */
/* Begin {put,get}_vars */

EXTERNL int
nc3_put_vars_text(int ncid, int varid,
  const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
  const char *op);

EXTERNL int
nc3_get_vars_text(int ncid, int varid,
  const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
  char *ip);

EXTERNL int
nc3_put_vars_uchar(int ncid, int varid,
  const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
  const unsigned char *op);

EXTERNL int
nc3_get_vars_uchar(int ncid, int varid,
  const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
  unsigned char *ip);

EXTERNL int
nc3_put_vars_schar(int ncid, int varid,
  const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
  const signed char *op);

EXTERNL int
nc3_get_vars_schar(int ncid, int varid,
  const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
  signed char *ip);

EXTERNL int
nc3_put_vars_short(int ncid, int varid,
  const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
  const short *op);

EXTERNL int
nc3_get_vars_short(int ncid, int varid,
  const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
  short *ip);

EXTERNL int
nc3_put_vars_int(int ncid, int varid,
  const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
  const int *op);

EXTERNL int
nc3_get_vars_int(int ncid, int varid,
  const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
  int *ip);

EXTERNL int
nc3_put_vars_long(int ncid, int varid,
  const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
  const long *op);

EXTERNL int
nc3_get_vars_long(int ncid, int varid,
  const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
  long *ip);

EXTERNL int
nc3_put_vars_float(int ncid, int varid,
  const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
  const float *op);

EXTERNL int
nc3_get_vars_float(int ncid, int varid,
  const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
  float *ip);

EXTERNL int
nc3_put_vars_double(int ncid, int varid,
  const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
  const double *op);

EXTERNL int
nc3_get_vars_double(int ncid, int varid,
  const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
  double *ip);

/* End {put,get}_vars */
/* Begin {put,get}_varm */

EXTERNL int
nc3_put_varm_text(int ncid, int varid,
  const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
  const ptrdiff_t *imapp, 
  const char *op);

EXTERNL int
nc3_get_varm_text(int ncid, int varid,
  const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
  const ptrdiff_t *imapp, 
  char *ip);

EXTERNL int
nc3_put_varm_uchar(int ncid, int varid,
  const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
  const ptrdiff_t *imapp, 
  const unsigned char *op);

EXTERNL int
nc3_get_varm_uchar(int ncid, int varid,
  const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
  const ptrdiff_t *imapp, 
  unsigned char *ip);

EXTERNL int
nc3_put_varm_schar(int ncid, int varid,
  const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
  const ptrdiff_t *imapp, 
  const signed char *op);

EXTERNL int
nc3_get_varm_schar(int ncid, int varid,
  const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
  const ptrdiff_t *imapp, 
  signed char *ip);

EXTERNL int
nc3_put_varm_short(int ncid, int varid,
  const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
  const ptrdiff_t *imapp, 
  const short *op);

EXTERNL int
nc3_get_varm_short(int ncid, int varid,
  const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
  const ptrdiff_t *imapp, 
  short *ip);

EXTERNL int
nc3_put_varm_int(int ncid, int varid,
  const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
  const ptrdiff_t *imapp, 
  const int *op);

EXTERNL int
nc3_get_varm_int(int ncid, int varid,
  const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
  const ptrdiff_t *imapp, 
  int *ip);

EXTERNL int
nc3_put_varm_long(int ncid, int varid,
  const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
  const ptrdiff_t *imapp, 
  const long *op);

EXTERNL int
nc3_get_varm_long(int ncid, int varid,
  const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
  const ptrdiff_t *imapp, 
  long *ip);

EXTERNL int
nc3_put_varm_float(int ncid, int varid,
  const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
  const ptrdiff_t *imapp, 
  const float *op);

EXTERNL int
nc3_get_varm_float(int ncid, int varid,
  const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
  const ptrdiff_t *imapp, 
  float *ip);

EXTERNL int
nc3_put_varm_double(int ncid, int varid,
  const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
  const ptrdiff_t *imapp, 
  const double *op);

EXTERNL int
nc3_get_varm_double(int ncid, int varid,
  const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
  const ptrdiff_t * imap, 
  double *ip);

/* End {put,get}_varm */
/* Begin {put,get}_var */

EXTERNL int
nc3_put_var_text(int ncid, int varid, const char *op);

EXTERNL int
nc3_get_var_text(int ncid, int varid, char *ip);

EXTERNL int
nc3_put_var_uchar(int ncid, int varid, const unsigned char *op);

EXTERNL int
nc3_get_var_uchar(int ncid, int varid, unsigned char *ip);

EXTERNL int
nc3_put_var_schar(int ncid, int varid, const signed char *op);

EXTERNL int
nc3_get_var_schar(int ncid, int varid, signed char *ip);

EXTERNL int
nc3_put_var_short(int ncid, int varid, const short *op);

EXTERNL int
nc3_get_var_short(int ncid, int varid, short *ip);

EXTERNL int
nc3_put_var_int(int ncid, int varid, const int *op);

EXTERNL int
nc3_get_var_int(int ncid, int varid, int *ip);

EXTERNL int
nc3_put_var_long(int ncid, int varid, const long *op);

EXTERNL int
nc3_get_var_long(int ncid, int varid, long *ip);

EXTERNL int
nc3_put_var_float(int ncid, int varid, const float *op);

EXTERNL int
nc3_get_var_float(int ncid, int varid, float *ip);

EXTERNL int
nc3_put_var_double(int ncid, int varid, const double *op);

EXTERNL int
nc3_get_var_double(int ncid, int varid, double *ip);

/* End {put,get}_var */

/* #ifdef _CRAYMPP */
/*
 * Public interfaces to better support
 * CRAY multi-processor systems like T3E.
 * A tip of the hat to NERSC.
 */
/*
 * It turns out we need to declare and define
 * these public interfaces on all platforms
 * or things get ugly working out the
 * FORTRAN interface. On !_CRAYMPP platforms,
 * these functions work as advertised, but you
 * can only use "processor element" 0.
 */

EXTERNL int
nc3__create_mp(const char *path, int cmode, size_t initialsz, int basepe,
   size_t *chunksizehintp, int *ncidp);

EXTERNL int
nc3__open_mp(const char *path, int mode, int basepe,
  size_t *chunksizehintp, int *ncidp);

EXTERNL int
nc3_delete_mp(const char * path, int basepe);

EXTERNL int
nc3_set_base_pe(int ncid, int pe);

EXTERNL int
nc3_inq_base_pe(int ncid, int *pe);

/* #endif _CRAYMPP */


/* Begin v2.4 backward compatiblity */
EXTERNL void
nc3_advise(const char *cdf_routine_name, int err, const char *fmt,...);

/*
 * C data type corresponding to a netCDF NC_LONG argument,
 * a signed 32 bit object.
 * 
 * This is the only thing in this file which architecture dependent.
 */

EXTERNL int
nc3typelen(nc_type datatype);

EXTERNL int
nc3create(const char* path, int cmode);

EXTERNL int
nc3open(const char* path, int mode);

EXTERNL int
nc3setfill(int ncid, int fillmode);

EXTERNL int
nc3redef(int ncid);

EXTERNL int
nc3endef(int ncid);

EXTERNL int
nc3sync(int ncid);

EXTERNL int
nc3abort(int ncid);

EXTERNL int
nc3close(int ncid);

EXTERNL int
nc3inquire(int ncid, int *ndimsp, int *nvarsp, int *nattsp, int *unlimdimp);

EXTERNL int
nc3dimdef(int ncid, const char *name, long len);

EXTERNL int
nc3dimid(int ncid, const char *name);

EXTERNL int
nc3diminq(int ncid, int dimid, char *name, long *lenp);

EXTERNL int
nc3dimrename(int ncid, int dimid, const char *name);

EXTERNL int
nc3attput(int ncid, int varid, const char *name, nc_type xtype,
  int len, const void *op);

EXTERNL int
nc3attinq(int ncid, int varid, const char *name, nc_type *xtypep, int *lenp);

EXTERNL int
nc3attget(int ncid, int varid, const char *name, void *ip);

EXTERNL int
nc3attcopy(int ncid_in, int varid_in, const char *name, int ncid_out,
  int varid_out);

EXTERNL int
nc3attname(int ncid, int varid, int attnum, char *name);

EXTERNL int
nc3attrename(int ncid, int varid, const char *name, const char *newname);

EXTERNL int
nc3attdel(int ncid, int varid, const char *name);

EXTERNL int
nc3vardef(int ncid, const char *name, nc_type xtype,
  int ndims, const int *dimidsp);

EXTERNL int
nc3varid(int ncid, const char *name);

EXTERNL int
nc3varinq(int ncid, int varid, char *name, nc_type *xtypep,
  int *ndimsp, int *dimidsp, int *nattsp);

EXTERNL int
nc3varput1(int ncid, int varid, const long *indexp, const void *op);

EXTERNL int
nc3varget1(int ncid, int varid, const long *indexp, void *ip);

EXTERNL int
nc3varput(int ncid, int varid, const long *startp, const long *countp,
  const void *op);

EXTERNL int
nc3varget(int ncid, int varid, const long *startp, const long *countp, 
  void *ip);

EXTERNL int
nc3varputs(int ncid, int varid, const long *startp, const long *countp,
  const long *stridep, const void *op);

EXTERNL int
nc3vargets(int ncid, int varid, const long *startp, const long *countp,
  const long *stridep, void *ip);

EXTERNL int
nc3varputg(int ncid, int varid, const long *startp, const long *countp,
  const long *stridep, const long *imapp, const void *op);

EXTERNL int
nc3vargetg(int ncid, int varid, const long *startp, const long *countp,
  const long *stridep, const long *imapp, void *ip);

EXTERNL int
nc3varrename(int ncid, int varid, const char *name);

EXTERNL int
nc3recinq(int ncid, int *nrecvarsp, int *recvaridsp, long *recsizesp);

EXTERNL int
nc3recget(int ncid, long recnum, void **datap);

EXTERNL int
nc3recput(int ncid, long recnum, void *const *datap);

/* End v2.4 backward compatiblity */
#endif /*!NO_NETCDF_2*/

#if defined(__cplusplus)
}
#endif

