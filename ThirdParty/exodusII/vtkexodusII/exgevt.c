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
* exgevt - ex_get_elem_var_time
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*       int     elem_var_index          element variable index
*       int     elem_number             element number
*       int     beg_time_step           time step number
*       int     end_time_step           time step number
*
* exit conditions - 
*       float*  elem_var_vals           array of element variable values
*
* revision history - 
*   20061002 - David Thompson - Moved to ex_get_var_time.
*
*****************************************************************************/

#include <stdlib.h>
#include "exodusII.h"
#include "exodusII_int.h"

/*!
\deprecated Use ex_get_var_time()(exoid, EX_ELEM_BLOCK, elem_var_index, elem_number, beg_time_step, end_time_step, elem_var_vals)

The function ex_get_elem_var_time() reads the values of an element
variable for a single element through a specified number of time
steps. Memory must be allocated for the element variable values array
before this function is invoked.

Because element variables are floating point values, the application
code must declare the array passed to be the appropriate type (\c
float or \c double) to match the compute word size passed in
ex_create() or ex_open().

\return In case of an error, ex_get_elem_var_time() returns a negative
number; a warning will return a positive number. Possible causes of
errors include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  data file not initialized properly with call to ex_put_init().
  -  ex_put_elem_block() not called previously to specify
     parameters for all element blocks.
  -  variable does not exist for the desired element or results
      haven't been written.

\param[in] exoid             exodus file ID returned from a previous call to ex_create()
                             or ex_open().
\param[in] elem_var_index    The index of the desired element variable. The first variable has an
                             index of 1.
\param[in] elem_number       The internal ID (see Section LocalElementIds) of the desired
                             element. The first element is 1.
\param[in] beg_time_step     The beginning time step for which an element variable value is
                             desired. This is not a time value but rather a time step number, as
           described under ex_put_time(). The first time step is 1.
\param[in] end_time_step     The last time step for which an element variable value is desired. If
                             negative, the last time step in the database will be used. The first
           time step is 1.
\param[out]  elem_var_vals   returned array of(\c end_time_step {-} \c beg_time_step + 
                             1) values of the \c elem_number-th element for the
           \c elem_var_index-th element variable.


For example, the following coding will read the values of the \c
var_index-th element variable for element number 2 from the first
time step to the last time step:

\code
#include "exodusII.h"

int error, exoid, num_time_steps, var_index, elem_num, 
    beg_time, end_time;

float *var_values;

\comment{determine how many time steps are stored}
num_time_steps = ex_inquire_int(exoid, EX_INQ_TIME);

\comment{read an element variable through time}
var_values = (float *) calloc (num_time_steps, sizeof(float));
var_index = 2;

elem_num = 2;

beg_time =  1;
end_time = -1;

error = ex_get_elem_var_time (exoid, var_index, elem_num, 
                              beg_time, end_time, var_values);

\comment{Using non-deprecated function:}
error = ex_get_elem_var_time (exoid, EX_ELEM_BLOCK, var_index, elem_num, 
                              beg_time, end_time, var_values);
\endcode

 */

int ex_get_elem_var_time (int   exoid,
                          int   elem_var_index,
                          int   elem_number,
                          int   beg_time_step, 
                          int   end_time_step,
                          void *elem_var_vals)
{
  return ex_get_var_time( exoid, EX_ELEM_BLOCK, elem_var_index, elem_number, beg_time_step, end_time_step, elem_var_vals );
}
