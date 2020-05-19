/*
 * Copyright (c) 2005-2017 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of NTESS nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, ex__get_dimension, etc

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
  int    varid, dimid, time_dim, numobjdim, dims[2], obj_id_ndx;
  size_t num_obj;
  size_t num_obj_var;
  size_t num_entity;
  size_t start[2], count[2];
  int *  obj_var_truth_tab;
  int    status;
  char   errmsg[MAX_ERR_LENGTH];

  EX_FUNC_ENTER();

  ex__check_valid_file_id(exoid, __func__);

#define EX_LOOK_UP_VAR(VOBJID, VVAR, VOBJTAB, DNUMOBJ, DNUMOBJVAR)                                 \
  /* Determine index of obj_id in VOBJID array */                                                  \
  obj_id_ndx = ex__id_lkup(exoid, var_type, obj_id);                                               \
  if (obj_id_ndx <= 0) {                                                                           \
    ex_get_err(NULL, NULL, &status);                                                               \
                                                                                                   \
    if (status != 0) {                                                                             \
      if (status == EX_NULLENTITY) {                                                               \
        snprintf(errmsg, MAX_ERR_LENGTH,                                                           \
                 "Warning: no variables allowed for NULL block %" PRId64 " in file id %d", obj_id, \
                 exoid);                                                                           \
        ex_err_fn(exoid, __func__, errmsg, EX_NULLENTITY);                                         \
        EX_FUNC_LEAVE(EX_WARN);                                                                    \
      }                                                                                            \
      else {                                                                                       \
        snprintf(errmsg, MAX_ERR_LENGTH,                                                           \
                 "ERROR: failed to locate %s id %" PRId64 " in %s array in file id %d",            \
                 ex_name_of_object(var_type), obj_id, VOBJID, exoid);                              \
        ex_err_fn(exoid, __func__, errmsg, status);                                                \
        EX_FUNC_LEAVE(EX_FATAL);                                                                   \
      }                                                                                            \
    }                                                                                              \
  }                                                                                                \
                                                                                                   \
  if ((status = nc_inq_varid(exoid, VVAR(var_index, obj_id_ndx), &varid)) != NC_NOERR) {           \
    if (status == NC_ENOTVAR) /* variable doesn't exist, create it! */                             \
    {                                                                                              \
      /* check for the existence of an TNAME variable truth table */                               \
      if (nc_inq_varid(exoid, VOBJTAB, &varid) == NC_NOERR) {                                      \
        /* find out number of TNAMEs and TNAME variables */                                        \
        status = ex__get_dimension(exoid, DNUMOBJ, ex_name_of_object(var_type), &num_obj, &dimid,  \
                                   __func__);                                                      \
        if (status != NC_NOERR)                                                                    \
          EX_FUNC_LEAVE(status);                                                                   \
                                                                                                   \
        status = ex__get_dimension(exoid, DNUMOBJVAR, ex_name_of_object(var_type), &num_obj_var,   \
                                   &dimid, __func__);                                              \
        if (status != NC_NOERR)                                                                    \
          EX_FUNC_LEAVE(status);                                                                   \
                                                                                                   \
        if (!(obj_var_truth_tab = malloc(num_obj * num_obj_var * sizeof(int)))) {                  \
          snprintf(errmsg, MAX_ERR_LENGTH,                                                         \
                   "ERROR: failed to allocate memory for %s variable "                             \
                   "truth table in file id %d",                                                    \
                   ex_name_of_object(var_type), exoid);                                            \
          ex_err_fn(exoid, __func__, errmsg, EX_MEMFAIL);                                          \
          EX_FUNC_LEAVE(EX_FATAL);                                                                 \
        }                                                                                          \
                                                                                                   \
        /*   read in the TNAME variable truth table */                                             \
        if ((status = nc_get_var_int(exoid, varid, obj_var_truth_tab)) != NC_NOERR) {              \
          snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get truth table from file id %d",     \
                   exoid);                                                                         \
          ex_err_fn(exoid, __func__, errmsg, status);                                              \
          EX_FUNC_LEAVE(EX_FATAL);                                                                 \
        }                                                                                          \
                                                                                                   \
        if (obj_var_truth_tab[num_obj_var * (obj_id_ndx - 1) + var_index - 1] == 0L) {             \
          free(obj_var_truth_tab);                                                                 \
          snprintf(errmsg, MAX_ERR_LENGTH,                                                         \
                   "ERROR: Invalid %s variable %d, %s %" PRId64 " in file id %d",                  \
                   ex_name_of_object(var_type), var_index, ex_name_of_object(var_type), obj_id,    \
                   exoid);                                                                         \
          ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);                                         \
          EX_FUNC_LEAVE(EX_FATAL);                                                                 \
        }                                                                                          \
        free(obj_var_truth_tab);                                                                   \
      }                                                                                            \
                                                                                                   \
      if ((status = nc_inq_dimid(exoid, DIM_TIME, &time_dim)) != NC_NOERR) {                       \
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate time dimension in file id %d",   \
                 exoid);                                                                           \
        ex_err_fn(exoid, __func__, errmsg, status);                                                \
        goto error_ret; /* exit define mode and return */                                          \
      }                                                                                            \
                                                                                                   \
      ex__get_dimension(exoid, ex__dim_num_entries_in_object(var_type, obj_id_ndx),                \
                        ex_name_of_object(var_type), &num_entity, &numobjdim, __func__);           \
                                                                                                   \
      /*    variable doesn't exist so put file into define mode  */                                \
      if ((status = nc_redef(exoid)) != NC_NOERR) {                                                \
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to put file id %d into define mode",       \
                 exoid);                                                                           \
        ex_err_fn(exoid, __func__, errmsg, status);                                                \
        EX_FUNC_LEAVE(EX_FATAL);                                                                   \
      }                                                                                            \
                                                                                                   \
      /* define netCDF variable to store TNAME variable values */                                  \
      dims[0] = time_dim;                                                                          \
      dims[1] = numobjdim;                                                                         \
      if ((status = nc_def_var(exoid, VVAR(var_index, obj_id_ndx), nc_flt_code(exoid), 2, dims,    \
                               &varid)) != NC_NOERR) {                                             \
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define %s variable %d in file id %d",   \
                 ex_name_of_object(var_type), var_index, exoid);                                   \
        ex_err_fn(exoid, __func__, errmsg, status);                                                \
        goto error_ret;                                                                            \
      }                                                                                            \
      ex__compress_variable(exoid, varid, 2);                                                      \
                                                                                                   \
      /*    leave define mode  */                                                                  \
                                                                                                   \
      if ((status = ex__leavedef(exoid, __func__)) != NC_NOERR) {                                  \
        EX_FUNC_LEAVE(EX_FATAL);                                                                   \
      }                                                                                            \
    }                                                                                              \
    else {                                                                                         \
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate %s variable %s in file id %d",     \
               ex_name_of_object(var_type), VVAR(var_index, obj_id_ndx), exoid);                   \
      ex_err_fn(exoid, __func__, errmsg, status);                                                  \
      EX_FUNC_LEAVE(EX_FATAL);                                                                     \
    }                                                                                              \
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
  case EX_EDGE_BLOCK:
    EX_LOOK_UP_VAR(VAR_ID_ED_BLK, VAR_EDGE_VAR, VAR_EBLK_TAB, DIM_NUM_ED_BLK, DIM_NUM_EDG_VAR);
    break;
  case EX_FACE_BLOCK:
    EX_LOOK_UP_VAR(VAR_ID_FA_BLK, VAR_FACE_VAR, VAR_FBLK_TAB, DIM_NUM_FA_BLK, DIM_NUM_FAC_VAR);
    break;
  case EX_ELEM_BLOCK:
    EX_LOOK_UP_VAR(VAR_ID_EL_BLK, VAR_ELEM_VAR, VAR_ELEM_TAB, DIM_NUM_EL_BLK, DIM_NUM_ELE_VAR);
    break;
  case EX_NODE_SET:
    EX_LOOK_UP_VAR(VAR_NS_IDS, VAR_NS_VAR, VAR_NSET_TAB, DIM_NUM_NS, DIM_NUM_NSET_VAR);
    break;
  case EX_EDGE_SET:
    EX_LOOK_UP_VAR(VAR_ES_IDS, VAR_ES_VAR, VAR_ESET_TAB, DIM_NUM_ES, DIM_NUM_ESET_VAR);
    break;
  case EX_FACE_SET:
    EX_LOOK_UP_VAR(VAR_FS_IDS, VAR_FS_VAR, VAR_FSET_TAB, DIM_NUM_FS, DIM_NUM_FSET_VAR);
    break;
  case EX_SIDE_SET:
    EX_LOOK_UP_VAR(VAR_SS_IDS, VAR_SS_VAR, VAR_SSET_TAB, DIM_NUM_SS, DIM_NUM_SSET_VAR);
    break;
  case EX_ELEM_SET:
    EX_LOOK_UP_VAR(VAR_ELS_IDS, VAR_ELS_VAR, VAR_ELSET_TAB, DIM_NUM_ELS, DIM_NUM_ELSET_VAR);
    break;
  default:
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: invalid variable type (%d) specified for file id %d",
             var_type, exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
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

/* Fatal error: exit definition mode and return */
error_ret:
  ex__leavedef(exoid, __func__);
  EX_FUNC_LEAVE(EX_FATAL);
}
