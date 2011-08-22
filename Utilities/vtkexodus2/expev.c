/*
 * Copyright (c) 2005 Sandia Corporation. Under the terms of Contract
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

#include <stdlib.h>
#include "exodusII.h"
#include "exodusII_int.h"

/*!
\deprecated Use ex_put_var()(exoid, time_step, EX_ELEM_BLOCK, elem_var_index, elem_blk_id, num_elem_this_blk, elem_var_vals)

The function ex_put_elem_var() writes the values of a single element
variable for one element block at one time step. It is recommended,
but not required, to write the element variable truth table (with
ex_put_elem_var_tab() before this function is invoked for better
efficiency. See #Efficiency for a discussion of
efficiency issues.

Because element variables are floating point values, the application
code must declare the array passed to be the appropriate type (\c
float or \c double) to match the compute word size passed in
ex_create() or ex_open().

\return In case of an error, ex_put_elem_var() returns a negative number; a
warning will return a positive number.  Possible causes of errors
include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  data file opened for read only.
  -  data file not initialized properly with call to ex_put_init().
  -  invalid element block ID.
  -  ex_put_elem_block() not called previously to specify parameters for this element block.
  -  ex_put_variable_param() not called previously specifying the number of element variables.
  - an element variable truth table was stored in the file but
    contains a zero (indicating no valid element variable) for the
    specified element block and element variable.

\param[in] exoid           exodus file ID returned from a previous call to ex_create() or ex_open().
\param[in] time_step       The time step number, as described under ex_put_time(). 
                           This is essentially a counter that is incremented only when results 
         variables are output. The first time step is 1.
\param[in] elem_var_index  The index of the element variable. The first variable has 
                           an index of 1.
\param[in] elem_blk_id     The element block ID.
\param[in] num_elem_this_blk  The number of elements in the given element block.
\param[in]  elem_var_vals  Array of \c num_elem_this_blk values of the \c elem_var_index-th 
                           element variable for the element block with ID of \c elem_blk_id 
         at the \c time_step-th time step.

The following coding will write out all of the element variables for a
single time step \c n to an open exodus file :

\code
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

\comment {Using non-deprecated function:}
      error = ex_put_var (exoid, n, EX_ELEM_BLOCK, k, ebids[j],
                               num_elem_in_block[j], elem_var_vals);

      free (elem_var_vals);
   }
}
\endcode

 */

int ex_put_elem_var (int   exoid,
                     int   time_step,
                     int   elem_var_index,
                     int   elem_blk_id,
                     int   num_elem_this_blk,
                     const void *elem_var_vals)
{
  return ex_put_var(exoid, time_step, EX_ELEM_BLOCK, elem_var_index,
        elem_blk_id, num_elem_this_blk, elem_var_vals);
}
