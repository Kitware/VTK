#ifndef _NETCDF3_CONV
#define _NETCDF3_CONV_

#include <stddef.h> /* size_t, ptrdiff_t */
#include <errno.h>  /* netcdf functions sometimes return system errors */

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * The Interface
 */
#define nc_inq_format nc3_inq_format
#define nc_inq_libvers nc3_inq_libvers
#define nc_strerror nc3_strerror
#define nc__create nc3__create
#define nc_create nc3_create
#define nc__open nc3__open
#define nc_open nc3_open
#define nc_set_fill nc3_set_fill
#define nc_redef nc3_redef
#define nc__enddef nc3__enddef
#define nc_enddef nc3_enddef
#define nc_sync nc3_sync
#define nc_abort nc3_abort
#define nc_close nc3_close
#define nc_inq nc3_inq
#define nc_inq_ndims nc3_inq_ndims
#define nc_inq_nvars nc3_inq_nvars
#define nc_inq_natts nc3_inq_natts
#define nc_inq_unlimdim nc3_inq_unlimdim
#define nc_inq_format nc3_inq_format


/* Begin _dim */

#define nc_def_dim nc3_def_dim
#define nc_inq_dimid nc3_inq_dimid
#define nc_inq_dim nc3_inq_dim
#define nc_inq_dimname nc3_inq_dimname
#define nc_inq_dimlen nc3_inq_dimlen
#define nc_rename_dim nc3_rename_dim
/* End _dim */
/* Begin _att */
#define nc_inq_att nc3_inq_att
#define nc_inq_attid nc3_inq_attid
#define nc_inq_atttype nc3_inq_atttype
#define nc_inq_attlen nc3_inq_attlen
#define nc_inq_attname nc3_inq_attname
#define nc_copy_att nc3_copy_att
#define nc_rename_att nc3_rename_att
#define nc_del_att nc3_del_att
/* End _att */
/* Begin {put,get}_att */
#define nc_put_att nc3_put_att
#define nc_get_att nc3_get_att
#define nc_put_att_text nc3_put_att_text
#define nc_get_att_text nc3_get_att_text
#define nc_put_att_uchar nc3_put_att_uchar
#define nc_get_att_uchar nc3_get_att_uchar
#define nc_put_att_schar nc3_put_att_schar
#define nc_get_att_schar nc3_get_att_schar
#define nc_put_att_short nc3_put_att_short
#define nc_get_att_short nc3_get_att_short
#define nc_put_att_int nc3_put_att_int
#define nc_get_att_int nc3_get_att_int
#define nc_put_att_long nc3_put_att_long
#define nc_get_att_long nc3_get_att_long
#define nc_put_att_float nc3_put_att_float
#define nc_get_att_float nc3_get_att_float
#define nc_put_att_double nc3_put_att_double
#define nc_get_att_double nc3_get_att_double
  /* End {put,get}_att */
/* Begin _var */
#define nc_def_var nc3_def_var
#define nc_inq_var nc3_inq_var
#define nc_inq_varid nc3_inq_varid
#define nc_inq_varname nc3_inq_varname
#define nc_inq_vartype nc3_inq_vartype
#define nc_inq_varndims nc3_inq_varndims
#define nc_inq_vardimid nc3_inq_vardimid
#define nc_inq_varnatts nc3_inq_varnatts
#define nc_rename_var nc3_rename_var
#define nc_copy_var nc3_copy_var

/* support the old name for now */
#define ncvarcpy(ncid_in, varid, ncid_out) ncvarcopy((ncid_in), (varid), (ncid_out))

/* End _var */
/* Begin {put,get}_var1 */
#define nc_put_var1 nc3_put_var1
#define nc_get_var1 nc3_get_var1
#define nc_put_var1_text nc3_put_var1_text
#define nc_get_var1_text nc3_get_var1_text
#define nc_put_var1_uchar nc3_put_var1_uchar
#define nc_get_var1_uchar nc3_get_var1_uchar
#define nc_put_var1_schar nc3_put_var1_schar
#define nc_get_var1_schar nc3_get_var1_schar
#define nc_put_var1_short nc3_put_var1_short
#define nc_get_var1_short nc3_get_var1_short
#define nc_put_var1_int nc3_put_var1_int
#define nc_get_var1_int nc3_get_var1_int
#define nc_put_var1_long nc3_put_var1_long
#define nc_get_var1_long nc3_get_var1_long
#define nc_put_var1_float nc3_put_var1_float
#define nc_get_var1_float nc3_get_var1_float
#define nc_put_var1_double nc3_put_var1_double
#define nc_get_var1_double nc3_get_var1_double
/* End {put,get}_var1 */
/* Begin {put,get}_vara */
#define nc_put_vara nc3_put_vara
#define nc_get_vara nc3_get_vara
#define nc_put_vara_text nc3_put_vara_text
#define nc_put_vara_text nc3_put_vara_text
#define nc_get_vara_text nc3_get_vara_text
#define nc_put_vara_uchar nc3_put_vara_uchar
#define nc_get_vara_uchar nc3_get_vara_uchar
#define nc_put_vara_schar nc3_put_vara_schar
#define nc_get_vara_schar nc3_get_vara_schar
#define nc_put_vara_short nc3_put_vara_short
#define nc_get_vara_short nc3_get_vara_short
#define nc_put_vara_int nc3_put_vara_int
#define nc_get_vara_int nc3_get_vara_int
#define nc_put_vara_long nc3_put_vara_long
#define nc_get_vara_long nc3_get_vara_long
#define nc_put_vara_float nc3_put_vara_float
#define nc_get_vara_float nc3_get_vara_float
#define nc_put_vara_double nc3_put_vara_double
#define nc_get_vara_double nc3_get_vara_double

/* End {put,get}_vara */
/* Begin {put,get}_vars */

#define nc_put_vars nc3_put_vars
#define nc_get_vars nc3_get_vars
#define nc_put_vars_text nc3_put_vars_text
#define nc_get_vars_text nc3_get_vars_text
#define nc_put_vars_uchar nc3_put_vars_uchar
#define nc_get_vars_uchar nc3_get_vars_uchar
#define nc_put_vars_schar nc3_put_vars_schar
#define nc_get_vars_schar nc3_get_vars_schar
#define nc_put_vars_short nc3_put_vars_short
#define nc_get_vars_short nc3_get_vars_short
#define nc_put_vars_int nc3_put_vars_int
#define nc_get_vars_int nc3_get_vars_int
#define nc_put_vars_long nc3_put_vars_long
#define nc_get_vars_long nc3_get_vars_long
#define nc_put_vars_float nc3_put_vars_float
#define nc_get_vars_float nc3_get_vars_float
#define nc_put_vars_double nc3_put_vars_double
#define nc_get_vars_double nc3_get_vars_double

/* End {put,get}_vars */
/* Begin {put,get}_varm */
#define nc_put_varm nc3_put_varm
#define nc_get_varm nc3_get_varm
#define nc_put_varm_text nc3_put_varm_text
#define nc_get_varm_text nc3_get_varm_text
#define nc_put_varm_uchar nc3_put_varm_uchar
#define nc_get_varm_uchar nc3_get_varm_uchar
#define nc_put_varm_schar nc3_put_varm_schar
#define nc_get_varm_schar nc3_get_varm_schar
#define nc_put_varm_short nc3_put_varm_short
#define nc_get_varm_short nc3_get_varm_short
#define nc_put_varm_int nc3_put_varm_int
#define nc_get_varm_int nc3_get_varm_int
#define nc_put_varm_long nc3_put_varm_long
#define nc_get_varm_long nc3_get_varm_long
#define nc_put_varm_float nc3_put_varm_float
#define nc_get_varm_float nc3_get_varm_float
#define nc_put_varm_double nc3_put_varm_double
#define nc_get_varm_double nc3_get_varm_double

/* End {put,get}_varm */
/* Begin {put,get}_var */

#define nc_put_var_text nc3_put_var_text
#define nc_get_var_text nc3_get_var_text
#define nc_put_var_uchar nc3_put_var_uchar
#define nc_get_var_uchar nc3_get_var_uchar
#define nc_put_var_schar nc3_put_var_schar
#define nc_get_var_schar nc3_get_var_schar
#define nc_put_var_short nc3_put_var_short
#define nc_get_var_short nc3_get_var_short
#define nc_put_var_int nc3_put_var_int
#define nc_get_var_int nc3_get_var_int
#define nc_put_var_long nc3_put_var_long
#define nc_get_var_long nc3_get_var_long
#define nc_put_var_float nc3_put_var_float
#define nc_get_var_float nc3_get_var_float
#define nc_put_var_double nc3_put_var_double
#define nc_get_var_double nc3_get_var_double
/* End {put,get}_var */

#define nc_put_att nc3_put_att
#define nc_get_att nc3_get_att

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

#define nc__create_mp nc3__create_mp


#define nc__open_mp nc3__open_mp


#define nc_delete_mp nc3_delete_mp
#define nc_set_base_pe nc3_set_base_pe
#define nc_inq_base_pe nc3_inq_base_pe
/* #endif _CRAYMPP */

#if defined(__cplusplus)
}
#endif

#endif /* _NETCDF_ */
