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

/*!
\ingroup ResultsData

The function ex_get_var() reads the values of the
selected entity variables for a single time step. Memory must be allocated for
the variables values array before this function is invoked.

Because variables are floating point values, the application
code must declare the array passed to be the appropriate type
(float or double) to match the compute word size passed in
ex_create() or ex_open().

In case of an error, ex_get_var() returns a negative
number; a warning will return a positive number. Possible causes of
errors include:

 - data file not properly opened with call to ex_create() or ex_open()
 - variable does not exist for the desired block or set.
 - invalid block or set.
 - no variables of the selected type stored in the file.
 - a warning value is returned if no variables of the selected entity type are stored in the file.

\param[in] exoid        exodus file ID returned from a previous call to
ex_create() or ex_open().

\param[in] time_step    The time step, as described under ex_put_time(), at
                        which the variable values are desired. This is
                        essentially an index (in the time dimension) into the entity
                        variable values array stored in the database. The first time step is 1.

\param[in]  var_type    block/variable type node, edge/face/element block, or
                        node/edge/face/side/element set of type ex_entity_type.
\param[in]  var_index   variable index; 1-based
\param[in]  obj_id      object id, see ex_get_ids()
\param[in]  num_entry_this_obj The number of entities in this object stored in the database.
\param[out] var_vals  Returned array of num_entry_this_obj variable values
                          for the time_step'th time step.

The following is an example code segment that reads the 10th element variable for element block with
id 100
at time step 5.  There are 'num_elements_this_block' elements in element block 100.

~~~{.c}
int num_elements_this_block, error, time_step;
int var_index = 10;
int blk_id = 100;
int num_variables;
float *var_values;

var_values = (float *) callo(num_elements_this_block, sizeof(float));
error = ex_get_var (idexo, time_step, EX_ELEM_BLOCK, var_index, blk_id,
                    num_elements_this_block, var_values);
~~~

 */

int ex_get_var(int exoid, int time_step, ex_entity_type var_type, int var_index,
               ex_entity_id obj_id, int64_t num_entry_this_obj, void *var_vals)
{
  int    status;
  int    varid, obj_id_ndx;
  size_t start[2], count[2];
  char   errmsg[MAX_ERR_LENGTH];

  EX_FUNC_ENTER();
  ex__check_valid_file_id(exoid, __func__);

  if (var_type == EX_NODAL) {
    /* FIXME: Special case: ignore obj_id, possible large_file complications,
     * etc. */
    status = ex__get_nodal_var(exoid, time_step, var_index, num_entry_this_obj, var_vals);
    EX_FUNC_LEAVE(status);
  }
  if (var_type == EX_GLOBAL) {
    /* FIXME: Special case: all vars stored in 2-D single array. */
    status = ex__get_glob_vars(exoid, time_step, num_entry_this_obj, var_vals);
    EX_FUNC_LEAVE(status);
  }

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
  start[1] = 0;

  count[0] = 1;
  count[1] = num_entry_this_obj;

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
