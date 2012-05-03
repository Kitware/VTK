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
* exgvtt - ex_get_elem_var_tab
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*       int     num_elem_blk            number of element blocks
*       int     num_elem_var            number of element variables
*
* exit conditions - 
*       int*    elem_var_tab            element variable truth table array
*
* revision history - 
*
*
*****************************************************************************/

#include <stdlib.h>
#include "exodusII.h"
#include "exodusII_int.h"

/*!
\deprecated Use ex_get_truth_table()(exoid, EX_ELEM_BLOCK, num_elem_blk, num_elem_var, elem_var_tab)

The function ex_get_elem_var_tab() reads the exodus element variable
truth table from the database. For a description of the truth table,
see the usage of the function ex_put_elem_var_tab(). Memory must be
allocated for the truth table(\c num_elem_blk \b x \c
num_elem_var in length) before this function is invoked. If the truth
table is not stored in the file, it will be created based on
information in the file and then returned.

\return In case of an error, ex_get_elem_var_tab() returns a negative
number; a warning will return a positive number.  Possible causes of
errors include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  data file not initialized properly with call to ex_put_init().
  -  the specified number of element blocks is different than the
     number specified in a call to ex_put_init().
  -  there are no element variables stored in the file or the
     specified number of element variables doesn't match the number
     specified in a call to ex_put_variable_param().

\param[in]   exoid         exodus file ID returned from a previous call to ex_create() or ex_open().
\param[in]   num_elem_blk  The number of element blocks.
\param[in]   num_elem_var  The number of element variables.
\param[out]  elem_var_tab  Size [num_elem_blk,num_elem_var]. 
                           Returned 2-dimensional array (with the \c num_elem_var index cycling
         faster) containing the element variable truth table.

As an example, the following coding will read the element 
variable truth table from an opened exodus file :

\code
int *truth_tab, num_elem_blk, num_ele_vars, error, exoid;

truth_tab = (int *) calloc ((num_elem_blk*num_ele_vars), 
                            sizeof(int));

error = ex_get_elem_var_tab (exoid, num_elem_blk, num_ele_vars, 
                             truth_tab);
\endcode

 */

int ex_get_elem_var_tab (int  exoid,
                         int  num_elem_blk,
                         int  num_elem_var,
                         int *elem_var_tab)
{
  return ex_get_truth_table(exoid, EX_ELEM_BLOCK, num_elem_blk, num_elem_var, elem_var_tab);
}
