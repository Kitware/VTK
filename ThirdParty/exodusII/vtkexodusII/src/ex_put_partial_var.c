/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, ex__get_dimension, etc

static int ex__look_up_var(int exoid, ex_entity_type var_type, int var_index, ex_entity_id obj_id,
                           const char *VOBJID, const char *VOBJTAB, const char *DNUMOBJ,
                           const char *DNUMOBJVAR, int *varid)
{
  int status;
  int obj_id_ndx;
  int dimid, time_dim, numobjdim, dims[2];

  size_t num_obj;
  size_t num_obj_var;
  size_t num_entity;

  int *obj_var_truth_tab;
  char errmsg[MAX_ERR_LENGTH];

  if (var_type == EX_ASSEMBLY) {
    status = nc_inq_varid(exoid, VAR_ENTITY_ASSEMBLY(obj_id), varid);
    if (status != 0) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to locate %s id %" PRId64 " in %s array in file id %d",
               ex_name_of_object(var_type), obj_id, VOBJID, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      return (EX_FATAL);
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
      return (EX_FATAL);
    }
    obj_id_ndx = obj_id;
  }
  else {
    /* Determine index of obj_id in VOBJID array */
    obj_id_ndx = ex__id_lkup(exoid, var_type, obj_id);
    if (obj_id_ndx <= 0) {
      ex_get_err(NULL, NULL, &status);

      if (status != 0) {
        if (status == EX_NULLENTITY) {
          snprintf(errmsg, MAX_ERR_LENGTH,
                   "Warning: no variables allowed for NULL block %" PRId64 " in file id %d", obj_id,
                   exoid);
          ex_err_fn(exoid, __func__, errmsg, EX_NULLENTITY);
          return (EX_WARN);
        }

        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to locate %s id %" PRId64 " in %s array in file id %d",
                 ex_name_of_object(var_type), obj_id, VOBJID, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        return (EX_FATAL);
      }
    }
  }

  if ((status = nc_inq_varid(exoid, ex__name_var_of_object(var_type, var_index, obj_id_ndx),
                             varid)) != NC_NOERR) {
    if (status == NC_ENOTVAR) { /* variable doesn't exist, create it! */
      /* check for the existence of an TNAME variable truth table */
      if (nc_inq_varid(exoid, VOBJTAB, varid) == NC_NOERR) {
        /* find out number of TNAMEs and TNAME variables */
        status = ex__get_dimension(exoid, DNUMOBJ, ex_name_of_object(var_type), &num_obj, &dimid,
                                   __func__);
        if (status != NC_NOERR) {
          return (status);
        }

        status = ex__get_dimension(exoid, DNUMOBJVAR, ex_name_of_object(var_type), &num_obj_var,
                                   &dimid, __func__);
        if (status != NC_NOERR) {
          return (status);
        }

        if (!(obj_var_truth_tab = malloc(num_obj * num_obj_var * sizeof(int)))) {
          snprintf(errmsg, MAX_ERR_LENGTH,
                   "ERROR: failed to allocate memory for %s variable "
                   "truth table in file id %d",
                   ex_name_of_object(var_type), exoid);
          ex_err_fn(exoid, __func__, errmsg, EX_MEMFAIL);
          return (EX_FATAL);
        }

        /*   read in the TNAME variable truth table */
        if ((status = nc_get_var_int(exoid, *varid, obj_var_truth_tab)) != NC_NOERR) {
          snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get truth table from file id %d",
                   exoid);
          ex_err_fn(exoid, __func__, errmsg, status);
          return (EX_FATAL);
        }

        if (obj_var_truth_tab[num_obj_var * (obj_id_ndx - 1) + var_index - 1] == 0L) {
          free(obj_var_truth_tab);
          snprintf(
              errmsg, MAX_ERR_LENGTH, "ERROR: Invalid %s variable %d, %s %" PRId64 " in file id %d",
              ex_name_of_object(var_type), var_index, ex_name_of_object(var_type), obj_id, exoid);
          ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
          return (EX_FATAL);
        }
        free(obj_var_truth_tab);
      }

      if ((status = nc_inq_dimid(exoid, DIM_TIME, &time_dim)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate time dimension in file id %d",
                 exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        goto error_ret; /* exit define mode and return */
      }

      ex__get_dimension(exoid, ex__dim_num_entries_in_object(var_type, obj_id_ndx),
                        ex_name_of_object(var_type), &num_entity, &numobjdim, __func__);

      /*    variable doesn't exist so put file into define mode  */
      if ((status = nc_redef(exoid)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to put file id %d into define mode", exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        return (EX_FATAL);
      }

      /* define netCDF variable to store TNAME variable values */
      dims[0] = time_dim;
      dims[1] = numobjdim;
      if ((status = nc_def_var(exoid, ex__name_var_of_object(var_type, var_index, obj_id_ndx),
                               nc_flt_code(exoid), 2, dims, varid)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define %s variable %d in file id %d",
                 ex_name_of_object(var_type), var_index, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        goto error_ret;
      }
      ex__compress_variable(exoid, *varid, 2);

      /*    leave define mode  */
      if ((status = ex__leavedef(exoid, __func__)) != NC_NOERR) {
        return (EX_FATAL);
      }
    }
    else {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate %s variable %s in file id %d",
               ex_name_of_object(var_type), ex__name_var_of_object(var_type, var_index, obj_id_ndx),
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      return (EX_FATAL);
    }
  }
  return (EX_NOERR);

/* Fatal error: exit definition mode and return */
error_ret:
  ex__leavedef(exoid, __func__);
  return (EX_FATAL);
}

/*!
\ingroup ResultsData
 * writes the values of a single variable for a partial block at one time
 * step to the database; assume the first time step and variable index
 * are 1
 * \param      exoid           exodus file id
 * \param      time_step       time step number (1-based)
 * \param      var_type        type (edge block, face block, edge set, ... )
 * \param      var_index       entity variable index (1-based)
 * \param      obj_id          entity id
 * \param      start_index     index of first entity in block to write (1-based)
 * \param      num_entities    number of entries in this block/set
 * \param      var_vals        the values to be written
 */

int ex_put_partial_var(int exoid, int time_step, ex_entity_type var_type, int var_index,
                       ex_entity_id obj_id, int64_t start_index, int64_t num_entities,
                       const void *var_vals)
{
  int    varid;
  size_t start[2], count[2];
  int    status;
  char   errmsg[MAX_ERR_LENGTH];

  EX_FUNC_ENTER();

  if (ex__check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  switch (var_type) {
  case EX_GLOBAL:
    if (num_entities <= 0) {
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
    status =
        ex__put_partial_nodal_var(exoid, time_step, var_index, start_index, num_entities, var_vals);
    EX_FUNC_LEAVE(status);
    break;
  case EX_ASSEMBLY:
    status = ex__look_up_var(exoid, var_type, var_index, obj_id, "", VAR_ASSEMBLY_TAB,
                             DIM_NUM_ASSEMBLY, DIM_NUM_ASSEMBLY_VAR, &varid);
    break;
  case EX_BLOB:
    status = ex__look_up_var(exoid, var_type, var_index, obj_id, "", VAR_BLOB_TAB, DIM_NUM_BLOB,
                             DIM_NUM_BLOB_VAR, &varid);
    break;
  case EX_EDGE_BLOCK:
    status = ex__look_up_var(exoid, var_type, var_index, obj_id, VAR_ID_ED_BLK, VAR_EBLK_TAB,
                             DIM_NUM_ED_BLK, DIM_NUM_EDG_VAR, &varid);
    break;
  case EX_FACE_BLOCK:
    status = ex__look_up_var(exoid, var_type, var_index, obj_id, VAR_ID_FA_BLK, VAR_FBLK_TAB,
                             DIM_NUM_FA_BLK, DIM_NUM_FAC_VAR, &varid);
    break;
  case EX_ELEM_BLOCK:
    status = ex__look_up_var(exoid, var_type, var_index, obj_id, VAR_ID_EL_BLK, VAR_ELEM_TAB,
                             DIM_NUM_EL_BLK, DIM_NUM_ELE_VAR, &varid);
    break;
  case EX_NODE_SET:
    status = ex__look_up_var(exoid, var_type, var_index, obj_id, VAR_NS_IDS, VAR_NSET_TAB,
                             DIM_NUM_NS, DIM_NUM_NSET_VAR, &varid);
    break;
  case EX_EDGE_SET:
    status = ex__look_up_var(exoid, var_type, var_index, obj_id, VAR_ES_IDS, VAR_ESET_TAB,
                             DIM_NUM_ES, DIM_NUM_ESET_VAR, &varid);
    break;
  case EX_FACE_SET:
    status = ex__look_up_var(exoid, var_type, var_index, obj_id, VAR_FS_IDS, VAR_FSET_TAB,
                             DIM_NUM_FS, DIM_NUM_FSET_VAR, &varid);
    break;
  case EX_SIDE_SET:
    status = ex__look_up_var(exoid, var_type, var_index, obj_id, VAR_SS_IDS, VAR_SSET_TAB,
                             DIM_NUM_SS, DIM_NUM_SSET_VAR, &varid);
    break;
  case EX_ELEM_SET:
    status = ex__look_up_var(exoid, var_type, var_index, obj_id, VAR_ELS_IDS, VAR_ELSET_TAB,
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
  start[0] = time_step - 1;
  start[1] = start_index - 1;
  if (var_type == EX_GLOBAL) {
    /* global variables may be written
     * - all at once (by setting var_index to 1 and num_entries_this_obj to
     * num_glob, or
     * - one at a time (by setting var_index to the desired index and
     * num_entries_this_obj to 1.
     */
    count[0] = var_index;
  }
  else {
    count[0] = 1;
  }
  count[1] = num_entities;
  if (count[1] == 0) {
    start[1] = 0;
  }

  if (ex__comp_ws(exoid) == 4) {
    status = nc_put_vara_float(exoid, varid, start, count, var_vals);
  }
  else {
    status = nc_put_vara_double(exoid, varid, start, count, var_vals);
  }

  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to store %s %" PRId64 " variable %d at step %d in file id %d",
             ex_name_of_object(var_type), obj_id, var_index, time_step, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  EX_FUNC_LEAVE(EX_NOERR);
}
