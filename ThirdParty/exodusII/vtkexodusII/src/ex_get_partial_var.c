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

#include "exodusII.h"     // for ex_err, ex_name_of_object, etc
#include "exodusII_int.h" // for ex__check_valid_file_id, etc

/*
 * reads the values of a single element variable for one element block at
 * one time step in the database; assume the first time step and
 * element variable index is 1
 */

/*!
\ingroup ResultsData

 * reads the values of a single variable for a partial block at one time
 * step from the database; assume the first time step and variable index
 * and start_index are 1
 * \param      exoid           exodus file id
 * \param      time_step       time step number
 * \param      var_type        type (edge block, face block, edge set, ... )
 * \param      var_index       element variable index
 * \param      obj_id          element block id
 * \param      start_index     index of first entity in block to read (1-based)
 * \param      num_entities    number of entries to read in this block/set
 * \param      var_vals        the values to read
 */

int ex_get_partial_var(int exoid, int time_step, ex_entity_type var_type, int var_index,
                       ex_entity_id obj_id, int64_t start_index, int64_t num_entities,
                       void *var_vals)
{
  int    status = 0;
  int    varid, obj_id_ndx;
  size_t start[2], count[2];
  char   errmsg[MAX_ERR_LENGTH];

  EX_FUNC_ENTER();

#if !defined(PARALLEL_AWARE_EXODUS)
  if (num_entities == 0) {
    EX_FUNC_LEAVE(status);
  }
#endif

  if (var_type == EX_NODAL) {
    /* FIXME: Special case: ignore obj_id, possible large_file complications,
     * etc. */
    status =
        ex__get_partial_nodal_var(exoid, time_step, var_index, start_index, num_entities, var_vals);
    EX_FUNC_LEAVE(status);
  }
  if (var_type == EX_GLOBAL) {
    /* FIXME: Special case: all vars stored in 2-D single array. */
    status = ex__get_glob_vars(exoid, time_step, num_entities, var_vals);
    EX_FUNC_LEAVE(status);
  }

  ex__check_valid_file_id(exoid, __func__);

  /* Determine index of obj_id in VAR_ID_EL_BLK array */
  obj_id_ndx = ex__id_lkup(exoid, var_type, obj_id);
  if (obj_id_ndx <= 0) {
    ex_get_err(NULL, NULL, &status);

    if (status != 0) {
      if (status == EX_NULLENTITY) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "Warning: no %s variables for NULL block %" PRId64 " in file id %d",
                 ex_name_of_object(var_type), obj_id, exoid);
        ex_err_fn(exoid, __func__, errmsg, EX_NULLENTITY);
        EX_FUNC_LEAVE(EX_WARN);
      }
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to locate %s id %" PRId64 " in id variable in file id %d",
               ex_name_of_object(var_type), obj_id, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }

  /* inquire previously defined variable */

  if ((status = nc_inq_varid(exoid, ex__name_var_of_object(var_type, var_index, obj_id_ndx),
                             &varid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate %s %" PRId64 " var %d in file id %d",
             ex_name_of_object(var_type), obj_id, var_index, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* read values of element variable */
  start[0] = --time_step;
  start[1] = start_index - 1;

  count[0] = 1;
  count[1] = num_entities;
  if (count[1] == 0) {
    start[1] = 0;
  }

  if (ex__comp_ws(exoid) == 4) {
    status = nc_get_vara_float(exoid, varid, start, count, var_vals);
  }
  else {
    status = nc_get_vara_double(exoid, varid, start, count, var_vals);
  }

  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to get %s %" PRId64 " variable %d in file id %d",
             ex_name_of_object(var_type), obj_id, var_index, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }
  EX_FUNC_LEAVE(EX_NOERR);
}
