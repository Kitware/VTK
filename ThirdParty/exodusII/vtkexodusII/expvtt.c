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
* expvtt - ex_put_elem_var_tab
*
* environment - UNIX
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*       int     num_elem_blk            number of element blocks
*       int     num_elem_var            number of element variables
*       int*    elem_var_tab            element variable truth table array
*
* exit conditions - 
*
* revision history - 
*
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"

/*!
\deprecated Use ex_put_truth_table()(exoid, EX_ELEM_BLOCK, num_elem_blk, num_elem_var, elem_var_tab)

The function ex_put_elem_var_tab() writes the exodus element variable
truth table to the database. The element variable truth table
indicates whether a particular element result is written for the
elements in a particular element block. A 0 (zero) entry indicates
that no results will be output for that element variable for that
element block. A non-zero entry indicates that the appropriate results
will be output.

Although writing the element variable truth table is optional, it is
encouraged because it creates at one time all the necessary
\code{NetCDF} variables in which to hold the exodus element variable
values. This results in significant time savings. See
Section #Efficiency for a discussion of efficiency issues.

The function ex_put_variable_param() must be called before this
routine in order to define the number of element variables.

\return In case of an error, ex_put_elem_var_tab() returns a negative
number; a warning will return a positive number. Possible causes of
errors include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  data file opened for read only.
  -  data file not initialized properly with call to ex_put_init().
  -  the specified number of element blocks is different than the
     number specified in a call to ex_put_init().
  -  ex_put_elem_block() not called previously to specify
     element block parameters.
  -  ex_put_variable_param() not called previously to specify
     the number of element variables or was called but with a different
     number of element variables.
  -  ex_put_elem_var() previously called.

\param[in]  exoid          exodus file ID returned from a previous call to ex_create() or ex_open().
\param[in]  num_elem_blk   The number of element blocks.
\param[in]  num_elem_var   The number of element variables.
\param[in]  elem_var_tab   Size [num_elem_blk,num_elem_var]. A 2-dimensional array
                           (with the \c num_elem_var index cycling faster)
         containing the element variable truth table.

The following coding will create, populate, and write an element
variable truth table to an opened exodus file (NOTE: all element
variables are valid for all element blocks in this example.):

\code
int *truth_tab, num_elem_blk, num_ele_vars, error, exoid;

\comment{write element variable truth table}
truth_tab = (int *)calloc((num_elem_blk*num_ele_vars), sizeof(int));

for (i=0, k=0; i < num_elem_blk; i++) {
   for (j=0; j < num_ele_vars; j++) {
      truth_tab[k++] = 1;
   }
}
error = ex_put_elem_var_tab(exoid, num_elem_blk, num_ele_vars, 
                            truth_tab);
\endcode

*/

int ex_put_elem_var_tab (int  exoid,
                         int  num_elem_blk,
                         int  num_elem_var,
                         int *elem_var_tab)
{
  return ex_put_truth_table(exoid, EX_ELEM_BLOCK, num_elem_blk, num_elem_var, elem_var_tab);
}
