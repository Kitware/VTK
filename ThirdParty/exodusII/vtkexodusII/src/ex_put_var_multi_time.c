/*
 * Copyright(C) 1999-2020, 2023, 2024 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, etc

static int exi_look_up_var(int exoid, ex_entity_type var_type, int var_index, ex_entity_id obj_id,
                           const char *VOBJID, const char *VOBJTAB, const char *DNUMOBJ,
                           const char *DNUMOBJVAR, int *varid)
{
  int status;
  int dimid, time_dim, numobjdim, dims[2];

  int obj_id_ndx = 0;

  int *obj_var_truth_tab;
  char errmsg[MAX_ERR_LENGTH];

  if (var_type == EX_ASSEMBLY) {
    status = nc_inq_varid(exoid, VAR_ENTITY_ASSEMBLY(obj_id), varid);
    if (status != 0) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to locate %s id %" PRId64 " in %s array in file id %d",
               ex_name_of_object(var_type), obj_id, VOBJID, exoid);
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
               ex_name_of_object(var_type), obj_id, VOBJID, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      return EX_FATAL;
    }
    obj_id_ndx = obj_id;
  }
  else {
    /* Determine index of obj_id in VOBJID array */
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
                 ex_name_of_object(var_type), obj_id, VOBJID, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        return EX_FATAL;
      }
    }
  }

  if ((status = nc_inq_varid(exoid, exi_name_var_of_object(var_type, var_index, obj_id_ndx),
                             varid)) != NC_NOERR) {
    if (status == NC_ENOTVAR) { /* variable doesn't exist, create it! */
      /* check for the existence of an TNAME variable truth table */
      if (nc_inq_varid(exoid, VOBJTAB, varid) == NC_NOERR) {
        /* find out number of TNAMEs and TNAME variables */
        size_t num_obj     = 0;
        size_t num_obj_var = 0;

        status = exi_get_dimension(exoid, DNUMOBJ, ex_name_of_object(var_type), &num_obj, &dimid,
                                   __func__);
        if (status != NC_NOERR) {
          return status;
        }

        status = exi_get_dimension(exoid, DNUMOBJVAR, ex_name_of_object(var_type), &num_obj_var,
                                   &dimid, __func__);
        if (status != NC_NOERR) {
          return status;
        }

        if (!(obj_var_truth_tab = malloc(num_obj * num_obj_var * sizeof(int)))) {
          snprintf(errmsg, MAX_ERR_LENGTH,
                   "ERROR: failed to allocate memory for %s variable "
                   "truth table in file id %d",
                   ex_name_of_object(var_type), exoid);
          ex_err_fn(exoid, __func__, errmsg, EX_MEMFAIL);
          return EX_FATAL;
        }

        /*   read in the TNAME variable truth table */
        if ((status = nc_get_var_int(exoid, *varid, obj_var_truth_tab)) != NC_NOERR) {
          snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get truth table from file id %d",
                   exoid);
          ex_err_fn(exoid, __func__, errmsg, status);
          return EX_FATAL;
        }

        if (obj_var_truth_tab[num_obj_var * (obj_id_ndx - 1) + var_index - 1] == 0L) {
          free(obj_var_truth_tab);
          snprintf(
              errmsg, MAX_ERR_LENGTH, "ERROR: Invalid %s variable %d, %s %" PRId64 " in file id %d",
              ex_name_of_object(var_type), var_index, ex_name_of_object(var_type), obj_id, exoid);
          ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
          return EX_FATAL;
        }
        free(obj_var_truth_tab);
      }

      if ((status = nc_inq_dimid(exoid, DIM_TIME, &time_dim)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate time dimension in file id %d",
                 exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        goto error_ret; /* exit define mode and return */
      }

      size_t num_entity = 0;
      exi_get_dimension(exoid, exi_dim_num_entries_in_object(var_type, obj_id_ndx),
                        ex_name_of_object(var_type), &num_entity, &numobjdim, __func__);

      /*    variable doesn't exist so put file into define mode  */
      if ((status = exi_redef(exoid, __func__)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to put file id %d into define mode", exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        return EX_FATAL;
      }

      /* define netCDF variable to store TNAME variable values */
      dims[0] = time_dim;
      dims[1] = numobjdim;
      if ((status = nc_def_var(exoid, exi_name_var_of_object(var_type, var_index, obj_id_ndx),
                               nc_flt_code(exoid), 2, dims, varid)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define %s variable %d in file id %d",
                 ex_name_of_object(var_type), var_index, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        goto error_ret;
      }
      exi_compress_variable(exoid, *varid, 2);

      /*    leave define mode  */
      if ((status = exi_leavedef(exoid, __func__)) != NC_NOERR) {
        return EX_FATAL;
      }
    }
    else {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate %s variable %s in file id %d",
               ex_name_of_object(var_type), exi_name_var_of_object(var_type, var_index, obj_id_ndx),
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      return EX_FATAL;
    }
  }
  return EX_NOERR;

/* Fatal error: exit definition mode and return */
error_ret:
  exi_leavedef(exoid, __func__);
  return EX_FATAL;
}

/*!
\ingroup ResultsData
writes the values of a single variable of the specified type for a
single time step. The function ex_put_variable_param() must be invoked
before this call is made.

Because variables are floating point values, the application
code must declare the array passed to be the appropriate type
(float or double) to match the compute word size passed in
ex_create() or ex_open().

\return In case of an error, ex_put_var() returns a negative number; a
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

\param[in] var_type
type (edge block, face block, edge set, ... )

\param[in] var_index
The index of the variable. The first variable has an index of 1.

\param[in]  obj_id
entity block/set id (ignored for global and nodal variables)

\param[in] num_entries_this_obj
The number of items in this block/set

\param[in] beg_time_step
The beginning time step number, as described under ex_put_time(). This
is essentially a counter that is incremented when results variables
are output. The first time step is 1.

\param[in] end_time_step
The last step number to put values. The first time step is 1.

\param[in]  var_vals
Array of `num_entries_this_obj` values of the `var_index`-th variable for the requested time
step(s).

As an example, the following code segment writes all the nodal
variables for a single time step:

~~~{.c}
int num_nod_vars, num_nodes, error, exoid, beg_time_step, end_time_step;
float *nodal_var_vals;

\comment{write nodal variables}
nodal_var_vals = (float *) calloc(num_nodes*num_steps, sizeof(float));
for (k=1; k <= num_nod_vars; k++) {
   for (t=1, t < num_steps; t++) {
      for (j=0; j < num_nodes; j++) {
         \comment{application code fills in this array}
         nodal_var_vals[j] = 10.0;
      }
   }
   error = ex_put_var(exoid, EX_NODAL, 0, k, num_nodes,
                      1, 1+num_steps, nodal_var_vals);
}
~~~
 */

int ex_put_var_multi_time(int exoid, ex_entity_type var_type, int var_index, ex_entity_id obj_id,
                          int64_t num_entries_this_obj, int beg_time_step, int end_time_step,
                          const void *var_vals)
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
  case EX_GLOBAL:
    if (num_entries_this_obj <= 0) {
      snprintf(errmsg, MAX_ERR_LENGTH, "Warning: no global variables specified for file id %d",
               exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);

      EX_FUNC_LEAVE(EX_WARN);
    }

    /* inquire previously defined variable */
    if ((status = nc_inq_varid(exoid, VAR_GLO_VAR, &varid)) != NC_NOERR) {
      if (status == NC_ENOTVAR) {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: no global variables defined in file id %d", exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
      }
      else {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to get global variables parameters in file id %d", exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
      }
      EX_FUNC_LEAVE(EX_FATAL);
    }
    break;
  case EX_NODAL:
    status = exi_put_nodal_var_time(exoid, var_index, num_entries_this_obj, beg_time_step,
                                    end_time_step, var_vals);
    EX_FUNC_LEAVE(status);
  case EX_ASSEMBLY:
    status = exi_look_up_var(exoid, var_type, var_index, obj_id, "", VAR_ASSEMBLY_TAB,
                             DIM_NUM_ASSEMBLY, DIM_NUM_ASSEMBLY_VAR, &varid);
    break;
  case EX_BLOB:
    status = exi_look_up_var(exoid, var_type, var_index, obj_id, "", VAR_BLOB_TAB, DIM_NUM_BLOB,
                             DIM_NUM_BLOB_VAR, &varid);
    break;
  case EX_EDGE_BLOCK:
    status = exi_look_up_var(exoid, var_type, var_index, obj_id, VAR_ID_ED_BLK, VAR_EBLK_TAB,
                             DIM_NUM_ED_BLK, DIM_NUM_EDG_VAR, &varid);
    break;
  case EX_FACE_BLOCK:
    status = exi_look_up_var(exoid, var_type, var_index, obj_id, VAR_ID_FA_BLK, VAR_FBLK_TAB,
                             DIM_NUM_FA_BLK, DIM_NUM_FAC_VAR, &varid);
    break;
  case EX_ELEM_BLOCK:
    status = exi_look_up_var(exoid, var_type, var_index, obj_id, VAR_ID_EL_BLK, VAR_ELEM_TAB,
                             DIM_NUM_EL_BLK, DIM_NUM_ELE_VAR, &varid);
    break;
  case EX_NODE_SET:
    status = exi_look_up_var(exoid, var_type, var_index, obj_id, VAR_NS_IDS, VAR_NSET_TAB,
                             DIM_NUM_NS, DIM_NUM_NSET_VAR, &varid);
    break;
  case EX_EDGE_SET:
    status = exi_look_up_var(exoid, var_type, var_index, obj_id, VAR_ES_IDS, VAR_ESET_TAB,
                             DIM_NUM_ES, DIM_NUM_ESET_VAR, &varid);
    break;
  case EX_FACE_SET:
    status = exi_look_up_var(exoid, var_type, var_index, obj_id, VAR_FS_IDS, VAR_FSET_TAB,
                             DIM_NUM_FS, DIM_NUM_FSET_VAR, &varid);
    break;
  case EX_SIDE_SET:
    status = exi_look_up_var(exoid, var_type, var_index, obj_id, VAR_SS_IDS, VAR_SSET_TAB,
                             DIM_NUM_SS, DIM_NUM_SSET_VAR, &varid);
    break;
  case EX_ELEM_SET:
    status = exi_look_up_var(exoid, var_type, var_index, obj_id, VAR_ELS_IDS, VAR_ELSET_TAB,
                             DIM_NUM_ELS, DIM_NUM_ELSET_VAR, &varid);
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
  start[0] = beg_time_step - 1;
  start[1] = 0;

  if (var_type == EX_GLOBAL) {
    /* global variables may be written
     * - all at once (by setting var_index to 1 and num_entries_this_obj to
     * num_glob, or
     * - one at a time (by setting var_index to the desired index and
     * num_entries_this_obj to 1.
     */
    start[1] = var_index - 1;
  }
  count[0] = end_time_step - beg_time_step + 1;
  count[1] = num_entries_this_obj;

  if (exi_comp_ws(exoid) == 4) {
    status = nc_put_vara_float(exoid, varid, start, count, var_vals);
  }
  else {
    status = nc_put_vara_double(exoid, varid, start, count, var_vals);
  }

  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to store %s %" PRId64 " variable %d at steps %d to %d in file id %d",
             ex_name_of_object(var_type), obj_id, var_index, beg_time_step, end_time_step, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  EX_FUNC_LEAVE(EX_NOERR);
}
