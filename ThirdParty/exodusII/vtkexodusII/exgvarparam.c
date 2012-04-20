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
* exgvp - ex_get_variable_param
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*       int     obj_type                variable type
*
* exit conditions - 
*       int*    num_vars                number of variables in database
*
* revision history - 
*
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"

#include <ctype.h>

/*!

The function ex_get_variable_param() reads the number of global,
nodal, or element variables stored in the database.

\return In case of an error, ex_get_variable_param() returns a negative
number; a warning will return a positive number. Possible causes of
errors include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  invalid variable type specified.

\param[in]  exoid     exodus file ID returned from a previous call to ex_create() or ex_open().
\param[in]  obj_type  Variable indicating the type of variable which is described. Use one
                      of the options in the table below.
\param[out] num_vars  Returned number of \c var_type variables that are stored in the database.

<table>
<tr><td> \c EX_GLOBAL}    </td><td>  Global entity type       </td></tr>
<tr><td> \c EX_NODAL}     </td><td>  Nodal entity type        </td></tr>
<tr><td> \c EX_NODE_SET   </td><td>  Node Set entity type     </td></tr>
<tr><td> \c EX_EDGE_BLOCK </td><td>  Edge Block entity type   </td></tr>
<tr><td> \c EX_EDGE_SET   </td><td>  Edge Set entity type     </td></tr>
<tr><td> \c EX_FACE_BLOCK </td><td>  Face Block entity type   </td></tr>
<tr><td> \c EX_FACE_SET   </td><td>  Face Set entity type     </td></tr>
<tr><td> \c EX_ELEM_BLOCK </td><td>  Element Block entity type</td></tr>
<tr><td> \c EX_ELEM_SET   </td><td>  Element Set entity type  </td></tr>
<tr><td> \c EX_SIDE_SET   </td><td>  Side Set entity type     </td></tr>
</table>

As an example, the following coding will determine the number of
global variables stored in the data file:

\code
int num_glo_vars, error, exoid;

\comment{read global variables parameters}
error = ex_get_variable_param(exoid, EX_GLOBAL, &num_glo_vars);
\endcode

*/

int ex_get_variable_param (int   exoid,
         ex_entity_type obj_type,
         int  *num_vars)
{
  int dimid;
  size_t dimlen;
  char errmsg[MAX_ERR_LENGTH];
  const char* dnumvar;
  int status;
  
  exerrval = 0; /* clear error code */
  *num_vars = 0;
  
  switch (obj_type) {
  case EX_GLOBAL:
    dnumvar = DIM_NUM_GLO_VAR;
    break;
  case EX_NODAL:
    dnumvar = DIM_NUM_NOD_VAR;
    break;
  case EX_EDGE_BLOCK:
    dnumvar = DIM_NUM_EDG_VAR;
    break;
  case EX_FACE_BLOCK:
    dnumvar = DIM_NUM_FAC_VAR;
    break;
  case EX_ELEM_BLOCK:
    dnumvar = DIM_NUM_ELE_VAR;
    break;
  case EX_NODE_SET:
    dnumvar = DIM_NUM_NSET_VAR;
    break;
  case EX_EDGE_SET:
    dnumvar = DIM_NUM_ESET_VAR;
    break;
  case EX_FACE_SET:
    dnumvar = DIM_NUM_FSET_VAR;
    break;
  case EX_SIDE_SET:
    dnumvar = DIM_NUM_SSET_VAR;
    break;
  case EX_ELEM_SET:
    dnumvar = DIM_NUM_ELSET_VAR;
    break;
  default:
    exerrval = EX_BADPARAM;
    sprintf(errmsg,
            "Warning: invalid variable type %d requested from file id %d",
            obj_type, exoid);
    ex_err("ex_get_var_param",errmsg,exerrval);
    return (EX_WARN);
  }

  if ((status = nc_inq_dimid (exoid, dnumvar, &dimid)) != NC_NOERR) {
    *num_vars = 0;
    if (status == NC_EBADDIM)
      return(EX_NOERR);      /* no global variables defined */
    else {
      exerrval = status;
      sprintf(errmsg,
        "Error: failed to locate %s variable names in file id %d",
        ex_name_of_object(obj_type),exoid);
      ex_err("ex_get_var_param",errmsg,exerrval);
      return (EX_FATAL);
    }
  }

  if ((status = nc_inq_dimlen(exoid, dimid, &dimlen)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
      "Error: failed to get number of %s variables in file id %d",
      ex_name_of_object(obj_type),exoid);
    ex_err("ex_get_var_param",errmsg,exerrval);
    return (EX_FATAL);
  }
  *num_vars = dimlen;
  
  return(EX_NOERR);
}
