/*
 * Copyright(C) 1999-2021, 2024 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h"
#include "exodusII_int.h"

static int exi_look_up_var(int exoid, ex_entity_type var_type, ex_entity_id obj_id,
                           const char *var_obj_id, const char *dim_num_obj_var, int *varid)
{
  int  status;
  int  obj_id_ndx;
  char errmsg[MAX_ERR_LENGTH];

  if (var_type == EX_ASSEMBLY) {
    status = nc_inq_varid(exoid, VAR_ENTITY_ASSEMBLY(obj_id), varid);
    if (status != 0) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to locate %s id %" PRId64 " in %s array in file id %d",
               ex_name_of_object(var_type), obj_id, var_obj_id, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      return EX_FATAL;
    }
    obj_id_ndx = obj_id;
  }
  else if (var_type == EX_BLOB) {
    status = nc_inq_varid(exoid, VAR_ENTITY_BLOB(obj_id), varid);
    if (status != 0) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to locate %s id %" PRId64 " in %s array in file id %d",
               ex_name_of_object(var_type), obj_id, var_obj_id, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      return EX_FATAL;
    }
    obj_id_ndx = obj_id;
  }
  else {
    /* Determine index of obj_id in var_obj_id array */
    obj_id_ndx = exi_id_lkup(exoid, var_type, obj_id);
    if (obj_id_ndx <= 0) {
      ex_get_err(NULL, NULL, &status);

      if (status != 0) {
        if (status == EX_NULLENTITY) {
          snprintf(errmsg, MAX_ERR_LENGTH,
                   "Warning: no variables allowed for NULL block %" PRId64 " in file id %d", obj_id,
                   exoid);
          ex_err_fn(exoid, __func__, errmsg, EX_NULLENTITY);
          return EX_WARN;
        }

        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to locate %s id %" PRId64 " in %s array in file id %d",
                 ex_name_of_object(var_type), obj_id, var_obj_id, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        return EX_FATAL;
      }
    }
  }
  if ((status = nc_inq_varid(exoid, exi_name_red_var_of_object(var_type, obj_id_ndx), varid)) !=
      NC_NOERR) {
    if (status == NC_ENOTVAR) { /* variable doesn't exist, create it! */
      int time_dim;
      if ((status = nc_inq_dimid(exoid, DIM_TIME, &time_dim)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate time dimension in file id %d",
                 exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        return EX_FATAL;
      }

      size_t num_obj_var;
      int    numvardim;
      if ((status = exi_get_dimension(exoid, dim_num_obj_var, ex_name_of_object(var_type),
                                      &num_obj_var, &numvardim, __func__)) != EX_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to inquire number of %s reduction variables in file id %d",
                 ex_name_of_object(var_type), exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        return EX_FATAL;
      }

      /*    variable doesn't exist so put file into define mode  */
      if ((status = exi_redef(exoid, __func__)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to put file id %d into define mode", exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        return EX_FATAL;
      }

      /* define NetCDF variable to store reduction variable values */
      int dims[] = {time_dim, numvardim};
      if ((status = nc_def_var(exoid, exi_name_red_var_of_object(var_type, obj_id_ndx),
                               nc_flt_code(exoid), 2, dims, varid)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define %s in file id %d",
                 ex_name_of_object(var_type), exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        exi_leavedef(exoid, __func__);
        return EX_FATAL;
      }
      exi_compress_variable(exoid, *varid, 2);

      /*    leave define mode  */
      if ((status = exi_leavedef(exoid, __func__)) != NC_NOERR) {
        return EX_FATAL;
      }
    }
    else {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate %s %s in file id %d",
               ex_name_of_object(var_type), exi_name_red_var_of_object(var_type, obj_id_ndx),
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      return EX_FATAL;
    }
  }
  return EX_NOERR;
}

/*!
\ingroup ResultsData
writes the values of a single variable of the specified type for a
single time step. The function ex_put_reduction_variable_param() must be invoked
before this call is made.

Because variables are floating point values, the application
code must declare the array passed to be the appropriate type
(float or double) to match the compute word size passed in
ex_create() or ex_open().

\return In case of an error, ex_put_reduction_vars() returns a negative number; a
warning will return a positive number. Possible causes of errors
include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  data file opened for read only.
  -  data file not initialized properly with call to ex_put_init().
  -  ex_put_variable_param() not called previously specifying the number of
variables.

\param[in] exoid
exodus file ID returned from a previous call to
ex_create() or ex_open().

\param[in] time_step
The time step number, as described under
ex_put_time(). This is essentially a counter that is incremented when
results variables are output. The first time step is 1.

\param[in] var_type
type (edge block, face block, edge set, ... )

\param[in]  obj_id
entity block/set id (ignored for global and nodal variables)

\param[in] num_variables
The number of reduction variables in this block/set

\param[in]  var_vals
Array of `num_variables` values for all reduction variables for the `time_step`-th time
step.

As an example, the following code segment writes all the assembly
variables for assembly 100 for a single time step:

~~~{.c}
int num_assem_vars, num_assembly, error, exoid, time_step;
float *assembly_var_vals;

\comment{write assembly variables}
assembly_var_vals = (float *) calloc(num_assem_vars, sizeof(float));
for (int k=1; k <= num_assem_vars; k++) {
   \comment{application code fills in this array}
   assembly_var_vals[j] = 10.0;
}
error = ex_put_reduction_vars(exoid, time_step, EX_ASSEMBLY, 100, num_assem_vars,
assembly_var_vals);
}
~~~
 */

int ex_put_reduction_vars(int exoid, int time_step, ex_entity_type var_type, ex_entity_id obj_id,
                          int64_t num_variables, const void *var_vals)
{
  int    varid;
  size_t start[2], count[2];
  int    status;
  char   errmsg[MAX_ERR_LENGTH];

  EX_FUNC_ENTER();

  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  switch (var_type) {
    /* NOTE: Global variables are always reduction variables, so use the ex_put_var function. */
  case EX_GLOBAL:
    EX_FUNC_UNLOCK();
    return ex_put_var(exoid, time_step, var_type, 1, 1, num_variables, var_vals);
    break;
  case EX_ASSEMBLY:
    status = exi_look_up_var(exoid, var_type, obj_id, "assembly", DIM_NUM_ASSEMBLY_RED_VAR, &varid);
    break;
  case EX_BLOB:
    status = exi_look_up_var(exoid, var_type, obj_id, "blob", DIM_NUM_BLOB_RED_VAR, &varid);
    break;
  case EX_EDGE_BLOCK:
    status = exi_look_up_var(exoid, var_type, obj_id, VAR_ID_ED_BLK, DIM_NUM_EDG_RED_VAR, &varid);
    break;
  case EX_FACE_BLOCK:
    status = exi_look_up_var(exoid, var_type, obj_id, VAR_ID_FA_BLK, DIM_NUM_FAC_RED_VAR, &varid);
    break;
  case EX_ELEM_BLOCK:
    status = exi_look_up_var(exoid, var_type, obj_id, VAR_ID_EL_BLK, DIM_NUM_ELE_RED_VAR, &varid);
    break;
  case EX_NODE_SET:
    status = exi_look_up_var(exoid, var_type, obj_id, VAR_NS_IDS, DIM_NUM_NSET_RED_VAR, &varid);
    break;
  case EX_EDGE_SET:
    status = exi_look_up_var(exoid, var_type, obj_id, VAR_ES_IDS, DIM_NUM_ESET_RED_VAR, &varid);
    break;
  case EX_FACE_SET:
    status = exi_look_up_var(exoid, var_type, obj_id, VAR_FS_IDS, DIM_NUM_FSET_RED_VAR, &varid);
    break;
  case EX_SIDE_SET:
    status = exi_look_up_var(exoid, var_type, obj_id, VAR_SS_IDS, DIM_NUM_SSET_RED_VAR, &varid);
    break;
  case EX_ELEM_SET:
    status = exi_look_up_var(exoid, var_type, obj_id, VAR_ELS_IDS, DIM_NUM_ELSET_RED_VAR, &varid);
    break;
  default:
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: invalid variable type (%d) specified for file id %d",
             var_type, exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (status != EX_NOERR) {
    EX_FUNC_LEAVE(status);
  }

  /* store element variable values */
  start[0] = time_step - 1;
  count[0] = 1;

  start[1] = 0;
  count[1] = num_variables;

  if (exi_comp_ws(exoid) == 4) {
    status = nc_put_vara_float(exoid, varid, start, count, var_vals);
  }
  else {
    status = nc_put_vara_double(exoid, varid, start, count, var_vals);
  }

  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to store %s %" PRId64 " at step %d in file id %d",
             ex_name_of_object(var_type), obj_id, time_step, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  EX_FUNC_LEAVE(EX_NOERR);
}
