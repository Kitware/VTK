/*
 * Copyright (c) 2006 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000 with Sandia Corporation, the U.S. Governement
 * retains certain rights in this software.
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
 *     * Neither the name of Sandia Corporation nor the names of its
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

#include "exodusII.h"
#include "exodusII_int.h"

/*!
\deprecated Use ex_get_var()(exoid, time_step, EX_ELEM_BLOCK, elem_var_index, elem_blk_id, num_elem_this_blk, elem_var_vals) instead

The function ex_get_elem_var() reads the values of a single element
variable for one element block at one time step. Memory must be
allocated for the element variable values array before this function
is invoked.

Because element variables are floating point values, the application
code must declare the array passed to be the appropriate type (\c
float or \c double) to match the compute word size passed in
ex_create() or ex_open().

\return In case of an error, ex_get_elem_var() returns a negative
number; a warning will return a positive number. Possible causes of
errors include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  variable does not exist for the desired element block.
  -  invalid element block.


\param[in] exoid            exodus file ID returned from a previous call to ex_create() or ex_open().
\param[in] time_step        The time step number, as described under ex_put_time(), at
                            which the element variable values are desired. This is essentially an
          index (in the time dimension) into the element variable values array
          stored in the database. The first time step is 1.

\param[in] elem_var_index   The index of the desired element variable. The first variable 
                            has an index of 1.

\param[in] elem_blk_id      The desired element block ID.

\param[in] num_elem_this_blk  The number of elements in this element block.

\param[out] elem_var_vals   Returned array of \c num_elem_this_blk values of the \c elem_var_index 
                            element variable for the element block with ID of \c elem_blk_id 
          at the \c time_step time step.


As an example, the following code segment will read the \c
var_index-th element variable at one time step stored in an exodus
file :

\code
int *ids, num_elem_blk, error, exoid, *num_elem_in_block, 
    step, var_ind;

float *var_vals;

ids = (int *) calloc(num_elem_blk, sizeof(int));
error = ex_get_elem_blk_ids (exoid, ids);

step = 1; \comment{read at the first time step}
for (i=0; i < num_elem_blk; i++) {
   var_vals = (float *) calloc (num_elem_in_block[i], sizeof(float));
   error = ex_get_elem_var (exoid, step, var_ind, ids[i], 
                            num_elem_in_block[i], var_vals);

\comment{Using non-deprecated function:}
   error = ex_get_var (exoid, step, EX_ELEM_BLOCK, var_ind, ids[i], 
                            num_elem_in_block[i], var_vals);

   free (var_values); 
}
\endcode

*/

int ex_get_elem_var (int   exoid,
                     int   time_step,
                     int   elem_var_index,
                     int   elem_blk_id, 
                     int   num_elem_this_blk,
                     void *elem_var_vals)
{
  return ex_get_var( exoid, time_step, EX_ELEM_BLOCK, elem_var_index, elem_blk_id, num_elem_this_blk, elem_var_vals );
}
