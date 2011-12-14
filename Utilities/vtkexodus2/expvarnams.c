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
* expvan - ex_put_variable_names
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*       int     obj_type                variable type: G,N, or E
*       int     num_vars                # of variables to read
*       char*   var_names               ptr array of variable names
*
* exit conditions - 
*
* revision history - 
*
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"
#include <string.h>
#include <ctype.h>

#define EX_PUT_NAMES(TNAME,DNUMVAR,VNAMES)        \
  if ((status = nc_inq_dimid(exoid, DNUMVAR, &dimid)) != NC_NOERR) {  \
    exerrval = status;              \
    if (status == NC_EBADDIM) {           \
      sprintf(errmsg,             \
        "Error: no " TNAME " variables defined in file id %d",  \
        exoid);             \
      ex_err("ex_put_var_names",errmsg,exerrval);     \
    } else  {               \
      sprintf(errmsg,             \
        "Error: failed to locate number of " TNAME " variables in file id %d", \
        exoid);             \
      ex_err("ex_put_var_names",errmsg,exerrval);     \
    }                 \
    return(EX_FATAL);             \
  }                 \
                  \
  if ((status = nc_inq_varid(exoid, VNAMES, &varid)) != NC_NOERR) { \
    exerrval = status;              \
    if (status == NC_ENOTVAR)           \
      {                 \
  sprintf(errmsg,             \
    "Error: no " TNAME " variable names defined in file id %d", \
    exoid);             \
  ex_err("ex_put_var_names",errmsg,exerrval);     \
      }  else {               \
      sprintf(errmsg,             \
        "Error: " TNAME " name variable names not found in file id %d", \
        exoid);             \
      ex_err("ex_put_var_names",errmsg,exerrval);     \
    }                 \
    return(EX_FATAL);             \
  }

/*!

The function ex_put_variable_names() writes the names of the results
variables to the database. The names are \p MAX_STR_LENGTH -characters
in length. The function ex_put_variable_param() must be called before
this function is invoked.

\return In case of an error, ex_put_variable_names() returns a negative
number; a warning will return a positive number.  Possible causes of
errors include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  data file not initialized properly with call to ex_put_init().
  -  invalid variable type specified.
  -  ex_put_variable_param() was not called previously or was
     called with zero variables of the specified type.
  -  ex_put_variable_names() has been called previously for the
     specified variable type.

\param[in]  exoid      exodus file ID returned from a previous call to ex_create() or ex_open(). 
\param[in]  obj_type   Variable indicating the type of variable which is described. 
                       Use one of the options in the table below.
\param[in]  num_vars   The number of \c var_type variables that will be written 
                       to the database.
\param[in]  var_names  Array of pointers to \c num_vars variable names.

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

The following coding will write out the names associated with the
nodal variables:
\code
int num_nod_vars, error, exoid;
char *var_names[2];

\comment{write results variables parameters and names}
num_nod_vars = 2;

var_names[0] = "disx";
var_names[1] = "disy";

error = ex_put_variable_param (exoid, EX_NODAL, num_nod_vars);
error = ex_put_variable_names (exoid, EX_NODAL, num_nod_vars, var_names);
\endcode

*/

int ex_put_variable_names (int   exoid,
         ex_entity_type obj_type,
         int   num_vars,
         char* var_names[])
{
  int varid, dimid, status; 
  char errmsg[MAX_ERR_LENGTH];

  exerrval = 0; /* clear error code */

  switch (obj_type) {
  case EX_GLOBAL:
    EX_PUT_NAMES(     "global",DIM_NUM_GLO_VAR,  VAR_NAME_GLO_VAR);
    break;
  case EX_NODAL:
    EX_PUT_NAMES(      "nodal",DIM_NUM_NOD_VAR,  VAR_NAME_NOD_VAR);
    break;
  case EX_EDGE_BLOCK:
    EX_PUT_NAMES(       "edge",DIM_NUM_EDG_VAR,  VAR_NAME_EDG_VAR);
    break;
  case EX_FACE_BLOCK:
    EX_PUT_NAMES(       "face",DIM_NUM_FAC_VAR,  VAR_NAME_FAC_VAR);
    break;
  case EX_ELEM_BLOCK:
    EX_PUT_NAMES(    "element",DIM_NUM_ELE_VAR,  VAR_NAME_ELE_VAR);
    break;
  case EX_NODE_SET:
    EX_PUT_NAMES(   "node set",DIM_NUM_NSET_VAR, VAR_NAME_NSET_VAR);
    break;
  case EX_EDGE_SET:
    EX_PUT_NAMES(   "edge set",DIM_NUM_ESET_VAR, VAR_NAME_ESET_VAR);
    break;
  case EX_FACE_SET:
    EX_PUT_NAMES(   "face set",DIM_NUM_FSET_VAR, VAR_NAME_FSET_VAR);
    break;
  case EX_SIDE_SET:
    EX_PUT_NAMES(   "side set",DIM_NUM_SSET_VAR, VAR_NAME_SSET_VAR);
    break;
  case EX_ELEM_SET:
    EX_PUT_NAMES("element set",DIM_NUM_ELSET_VAR,VAR_NAME_ELSET_VAR);
    break;
  default:
    exerrval = EX_BADPARAM;
    sprintf(errmsg,
            "Error: Invalid variable type %d specified in file id %d",
      obj_type, exoid);
    ex_err("ex_put_var_names",errmsg,exerrval);
    return(EX_FATAL);
  }

  /* write EXODUS variable names */
  status = ex_put_names_internal(exoid, varid, num_vars, var_names, obj_type,
         "variable", "ex_put_var_names");
  
  return(status);
}
