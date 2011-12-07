#ifndef _NC3TOLNC3_CONV_
#define _NC3TOLNC3_CONV_

#include <stddef.h> /* size_t, ptrdiff_t */
#include <errno.h>  /* netcdf functions sometimes return system errors */

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * The Interface
 */
#define nc__create l3nc__create
#define nc_create l3nc_create
#define nc__open l3nc__open
#define nc_open l3nc_open
#define nc_redef l3nc_redef
#define nc__enddef l3nc__enddef
#define nc_enddef l3nc_enddef
#define nc_sync l3nc_sync
#define nc_close l3nc_close
#ifndef PROTECT
#define nc_abort l3nc_abort
#endif

/* Begin {get}_var1 */
#define nc_get_var1 l3nc_get_var1
#define nc_get_var1_text l3nc_get_var1_text
#define nc_get_var1_uchar l3nc_get_var1_uchar
#define nc_get_var1_schar l3nc_get_var1_schar
#define nc_get_var1_short l3nc_get_var1_short
#define nc_get_var1_int l3nc_get_var1_int
#define nc_get_var1_long l3nc_get_var1_long
#define nc_get_var1_float l3nc_get_var1_float
#define nc_get_var1_double l3nc_get_var1_double
/* End {get}_var1 */
/* Begin {get}_vara */
#define nc_get_vara l3nc_get_vara
#define nc_get_vara_text l3nc_get_vara_text
#define nc_get_vara_uchar l3nc_get_vara_uchar
#define nc_get_vara_schar l3nc_get_vara_schar
#define nc_get_vara_short l3nc_get_vara_short
#define nc_get_vara_int l3nc_get_vara_int
#define nc_get_vara_long l3nc_get_vara_long
#define nc_get_vara_float l3nc_get_vara_float
#define nc_get_vara_double l3nc_get_vara_double

/* End {get}_vara */
/* Begin {get}_vars */

#define nc_get_vars l3nc_get_vars
#define nc_get_vars_text l3nc_get_vars_text
#define nc_get_vars_uchar l3nc_get_vars_uchar
#define nc_get_vars_schar l3nc_get_vars_schar
#define nc_get_vars_short l3nc_get_vars_short
#define nc_get_vars_int l3nc_get_vars_int
#define nc_get_vars_long l3nc_get_vars_long
#define nc_get_vars_float l3nc_get_vars_float
#define nc_get_vars_double l3nc_get_vars_double

/* End {get}_vars */
/* Begin {get}_varm */
#ifndef PROTECT
#define nc_get_varm l3nc_get_varm
#define nc_get_varm_text l3nc_get_varm_text
#define nc_get_varm_uchar l3nc_get_varm_uchar
#define nc_get_varm_schar l3nc_get_varm_schar
#define nc_get_varm_short l3nc_get_varm_short
#define nc_get_varm_int l3nc_get_varm_int
#define nc_get_varm_long l3nc_get_varm_long
#define nc_get_varm_float l3nc_get_varm_float
#define nc_get_varm_double l3nc_get_varm_double
#endif

/* End {get}_varm */
/* Begin {get}_var */

#define nc_get_var_text l3nc_get_var_text
#define nc_get_var_uchar l3nc_get_var_uchar
#define nc_get_var_schar l3nc_get_var_schar
#define nc_get_var_short l3nc_get_var_short
#define nc_get_var_int l3nc_get_var_int
#define nc_get_var_long l3nc_get_var_long
#define nc_get_var_float l3nc_get_var_float
#define nc_get_var_double l3nc_get_var_double
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

#ifndef PROTECT
#define nc__create_mp l3nc__create_mp
#define nc__open_mp l3nc__open_mp
/*#define nc_delete_mp l3nc_delete_mp*/
#endif

/* #endif _CRAYMPP */

#if defined(__cplusplus)
}
#endif

#endif /* _NETCDF_ */
