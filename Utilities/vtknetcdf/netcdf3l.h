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
/* "$Id: netcdf3l.h,v 2.18 2009/05/31 18:53:38 dmh Exp $" */

#ifndef _NETCDF3L_
#define _NETCDF3L_

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

EXTERNL int
l3nc_redef(int ncid);

EXTERNL int
l3nc__enddef(int ncid, size_t h_minfree, size_t v_align,
  size_t v_minfree, size_t r_align);

EXTERNL int
l3nc_enddef(int ncid);

EXTERNL int
l3nc_sync(int ncid);

EXTERNL int
l3nc_abort(int ncid);

EXTERNL int
l3nc_close(int ncid);

/* Begin {get}_var1 */

EXTERNL int
l3nc_get_var1(int ncid, int varid, const size_t *indexp, void *value);

EXTERNL int
l3nc_get_var1_text(int ncid, int varid, const size_t *indexp, char *ip);

EXTERNL int
l3nc_get_var1_uchar(int ncid, int varid, const size_t *indexp,
  unsigned char *ip);

EXTERNL int
l3nc_get_var1_schar(int ncid, int varid, const size_t *indexp,
  signed char *ip);

EXTERNL int
l3nc_get_var1_short(int ncid, int varid, const size_t *indexp,
  short *ip);

EXTERNL int
l3nc_get_var1_int(int ncid, int varid, const size_t *indexp, int *ip);

EXTERNL int
l3nc_get_var1_long(int ncid, int varid, const size_t *indexp, long *ip);

EXTERNL int
l3nc_get_var1_float(int ncid, int varid, const size_t *indexp, float *ip);

EXTERNL int
l3nc_get_var1_double(int ncid, int varid, const size_t *indexp, double *ip);

/* End {get}_var1 */
/* Begin {get}_vara */

EXTERNL int
l3nc_get_vara(int ncid, int varid,
   const size_t *start, const size_t *count, void *value);

EXTERNL int
l3nc_get_vara_text(int ncid, int varid,
  const size_t *startp, const size_t *countp, char *ip);

EXTERNL int
l3nc_get_vara_uchar(int ncid, int varid,
  const size_t *startp, const size_t *countp, unsigned char *ip);

EXTERNL int
l3nc_get_vara_schar(int ncid, int varid,
  const size_t *startp, const size_t *countp, signed char *ip);

EXTERNL int
l3nc_get_vara_short(int ncid, int varid,
  const size_t *startp, const size_t *countp, short *ip);

EXTERNL int
l3nc_get_vara_int(int ncid, int varid,
  const size_t *startp, const size_t *countp, int *ip);

EXTERNL int
l3nc_get_vara_long(int ncid, int varid,
  const size_t *startp, const size_t *countp, long *ip);

EXTERNL int
l3nc_get_vara_float(int ncid, int varid,
  const size_t *startp, const size_t *countp, float *ip);

EXTERNL int
l3nc_get_vara_double(int ncid, int varid,
  const size_t *startp, const size_t *countp, double *ip);

/* End {get}_vara */
/* Begin {get}_vars */

EXTERNL int
l3nc_get_vars(int ncid, int varid,
   const size_t *start, const size_t *count, const ptrdiff_t *stride,
   void * value);

EXTERNL int
l3nc_get_vars_text(int ncid, int varid,
  const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
  char *ip);

EXTERNL int
l3nc_get_vars_uchar(int ncid, int varid,
  const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
  unsigned char *ip);

EXTERNL int
l3nc_get_vars_schar(int ncid, int varid,
  const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
  signed char *ip);

EXTERNL int
l3nc_get_vars_short(int ncid, int varid,
  const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
  short *ip);

EXTERNL int
l3nc_get_vars_int(int ncid, int varid,
  const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
  int *ip);

EXTERNL int
l3nc_get_vars_long(int ncid, int varid,
  const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
  long *ip);

EXTERNL int
l3nc_get_vars_float(int ncid, int varid,
  const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
  float *ip);

EXTERNL int
l3nc_get_vars_double(int ncid, int varid,
  const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
  double *ip);

/* End {get}_vars */
/* Begin {get}_varm */

EXTERNL int
l3nc_get_varm(int ncid, int varid, const size_t *start, const size_t *count,
      const ptrdiff_t *stride, const ptrdiff_t *imapp, void *value);

EXTERNL int
l3nc_get_varm_text(int ncid, int varid,
  const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
  const ptrdiff_t *imapp,
  char *ip);

EXTERNL int
l3nc_get_varm_uchar(int ncid, int varid,
  const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
  const ptrdiff_t *imapp,
  unsigned char *ip);

EXTERNL int
l3nc_get_varm_schar(int ncid, int varid,
  const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
  const ptrdiff_t *imapp,
  signed char *ip);

EXTERNL int
l3nc_get_varm_short(int ncid, int varid,
  const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
  const ptrdiff_t *imapp,
  short *ip);

EXTERNL int
l3nc_get_varm_int(int ncid, int varid,
  const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
  const ptrdiff_t *imapp,
  int *ip);

EXTERNL int
l3nc_get_varm_long(int ncid, int varid,
  const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
  const ptrdiff_t *imapp,
  long *ip);

EXTERNL int
l3nc_get_varm_float(int ncid, int varid,
  const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
  const ptrdiff_t *imapp,
  float *ip);

EXTERNL int
l3nc_get_varm_double(int ncid, int varid,
  const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
  const ptrdiff_t * imap,
  double *ip);

/* End {get}_varm */
/* Begin {get}_var */

EXTERNL int
l3nc_get_var_text(int ncid, int varid, char *ip);

EXTERNL int
l3nc_get_var_uchar(int ncid, int varid, unsigned char *ip);

EXTERNL int
l3nc_get_var_schar(int ncid, int varid, signed char *ip);

EXTERNL int
l3nc_get_var_short(int ncid, int varid, short *ip);

EXTERNL int
l3nc_get_var_int(int ncid, int varid, int *ip);

EXTERNL int
l3nc_get_var_long(int ncid, int varid, long *ip);

EXTERNL int
l3nc_get_var_float(int ncid, int varid, float *ip);

EXTERNL int
l3nc_get_var_double(int ncid, int varid, double *ip);

/* End {get}_var */

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
l3nc__create_mp(const char *path, int cmode, size_t initialsz, int basepe,
   size_t *chunksizehintp, int *ncidp);

EXTERNL int
l3nc__open_mp(const char *path, int mode, int basepe,
  size_t *chunksizehintp, int *ncidp);

/*
EXTERNL int
l3nc_delete_mp(const char * path, int basepe);
*/

/* #endif _CRAYMPP */

#if defined(__cplusplus)
}
#endif
#endif
