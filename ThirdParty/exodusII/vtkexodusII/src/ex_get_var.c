/*
 * Copyright(C) 1999-2020, 2023 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h"     // for ex_err, ex_name_of_object, etc
#include "exodusII_int.h" // for exi_check_valid_file_id, etc

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
  return ex_get_var_multi_time(exoid, var_type, var_index, obj_id, num_entry_this_obj, time_step,
                               time_step, var_vals);
}
