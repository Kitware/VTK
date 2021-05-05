/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * expev - ex_put_elem_var
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       int     time_step               time step number
 *       int     elem_var_index          element variable index
 *       int     elem_blk_id             element block id
 *       int     num_elem_this_blk       number of elements in this block
 *
 * exit conditions -
 *
 *
 * exit conditions -
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h" // for ex_put_var, ex_entity_id, etc

/*!
\deprecated Use ex_put_var()(exoid, time_step, EX_ELEM_BLOCK, elem_var_index,
elem_blk_id, num_elem_this_blk, elem_var_vals)

The function ex_put_elem_var() writes the values of a single element
variable for one element block at one time step. It is recommended,
but not required, to write the element variable truth table (with
ex_put_elem_var_tab() before this function is invoked for better
efficiency.

Because element variables are floating point values, the application
code must declare the array passed to be the appropriate type
(float or double) to match the compute word size passed in
ex_create() or ex_open().

\return In case of an error, ex_put_elem_var() returns a negative number; a
warning will return a positive number.  Possible causes of errors
include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  data file opened for read only.
  -  data file not initialized properly with call to ex_put_init().
  -  invalid element block ID.
  -  ex_put_elem_block() not called previously to specify parameters for this
element block.
  -  ex_put_variable_param() not called previously specifying the number of
element variables.
  - an element variable truth table was stored in the file but
    contains a zero (indicating no valid element variable) for the
    specified element block and element variable.

\param[in] exoid           exodus file ID returned from a previous call to
ex_create() or ex_open().
\param[in] time_step       The time step number, as described under
ex_put_time().
                           This is essentially a counter that is incremented
only when results
                           variables are output. The first time step is 1.
\param[in] elem_var_index  The index of the element variable. The first variable
has
                           an index of 1.
\param[in] elem_blk_id     The element block ID.
\param[in] num_elem_this_blk  The number of elements in the given element block.
\param[in]  elem_var_vals  Array of num_elem_this_blk values of the
elem_var_index-th
                           element variable for the element block with ID of
elem_blk_id
                           at the time_step-th time step.

The following coding will write out all of the element variables for a
single time step n to an open exodus file :

~~~{.c}
int num_ele_vars, num_elem_blk, *num_elem_in_block,error,
    exoid, n, *ebids;

float *elem_var_vals;

\comment{write element variables}
for (k=1; k <= num_ele_vars; k++) {
   for (j=0; j < num_elem_blk; j++) {
      elem_var_vals = (float *)
         calloc(num_elem_in_block[j], sizeof(float));

         for (m=0; m < num_elem_in_block[j]; m++) {
            \comment{simulation code fills this in}
            elem_var_vals[m] = 10.0;
         }

      error = ex_put_elem_var (exoid, n, k, ebids[j],
                               num_elem_in_block[j], elem_var_vals);

      \comment{Using non-deprecated function:}
      error = ex_put_var (exoid, n, EX_ELEM_BLOCK, k, ebids[j],
                               num_elem_in_block[j], elem_var_vals);

      free (elem_var_vals);
   }
}
~~~

 */

int ex_put_elem_var(int exoid, int time_step, int elem_var_index, ex_entity_id elem_blk_id,
                    int64_t num_elem_this_blk, const void *elem_var_vals)
{
  return ex_put_var(exoid, time_step, EX_ELEM_BLOCK, elem_var_index, elem_blk_id, num_elem_this_blk,
                    elem_var_vals);
}
