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

#include "exodusII.h"
#include "exodusII_int.h"

#include <ctype.h>

/*!
The function ex_get_variable_names() reads the names of the results
variables from the database. Memory must be allocated for the name
array before this function is invoked. The names are \p
MAX_STR_LENGTH-characters in length.

\return In case of an error, ex_get_variable_names() returns a
negative number; a warning will return a positive number.  Possible
causes of errors include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  invalid variable type specified.
  -  a warning value is returned if no variables of the specified
     type are stored in the file.

\param[in]  exoid      exodus file ID returned from a previous call to ex_create() or ex_open(). 
\param[in]  obj_type   Variable indicating the type of variable which is described. Use one
                       of the options in the table below.
\param[in]  num_vars   The number of \c var_type variables that will be read 
                       from the database.
\param[out] var_names  Returned array of pointers to \c num_vars variable names.

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

As an example, the following code segment will read the names of the
nodal variables stored in the data file:

\code
#include "exodusII.h"
int error, exoid, num_nod_vars;
char *var_names[10];

\comment{read nodal variables parameters and names}
error = ex_get_variable_param(exoid, EX_NODAL, &num_nod_vars);
for (i=0; i < num_nod_vars; i++) {
   var_names[i] = (char *) calloc ((MAX_STR_LENGTH+1), sizeof(char));
}
error = ex_get_variable_names(exoid, EX_NODAL, num_nod_vars, var_names);
\endcode

*/

int ex_get_variable_names (int   exoid,
         ex_entity_type obj_type,
         int   num_vars,
         char *var_names[])
{
  int varid, status;
  char errmsg[MAX_ERR_LENGTH];
  const char* vvarname;

  exerrval = 0; /* clear error code */

  switch (obj_type) {
  case EX_NODAL:
    vvarname = VAR_NAME_NOD_VAR;
    break;
  case EX_EDGE_BLOCK:
    vvarname = VAR_NAME_EDG_VAR;
    break;
  case EX_FACE_BLOCK:
    vvarname = VAR_NAME_FAC_VAR;
    break;
  case EX_ELEM_BLOCK:
    vvarname = VAR_NAME_ELE_VAR;
    break;
  case EX_NODE_SET:
    vvarname = VAR_NAME_NSET_VAR;
    break;
  case EX_EDGE_SET:
    vvarname = VAR_NAME_ESET_VAR;
    break;
  case EX_FACE_SET:
    vvarname = VAR_NAME_FSET_VAR;
    break;
  case EX_SIDE_SET:
    vvarname = VAR_NAME_SSET_VAR;
    break;
  case EX_ELEM_SET:
    vvarname = VAR_NAME_ELSET_VAR;
    break;
  case EX_GLOBAL:
    vvarname = VAR_NAME_GLO_VAR;
    break;
  default:
    exerrval = EX_BADPARAM;
    sprintf(errmsg,
      "Warning: invalid variable type %d requested from file id %d",
      obj_type, exoid);
    ex_err("ex_get_variable_names",errmsg,exerrval);
    return (EX_WARN);
  }

  /* inquire previously defined variables  */
  if ((status = nc_inq_varid(exoid, vvarname, &varid)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg, "Warning: no %s variables names stored in file id %d",
      ex_name_of_object(obj_type),exoid);
    ex_err("ex_get_variable_names",errmsg,exerrval);
    return (EX_WARN);
  }

  /* read the variable names */
  status = ex_get_names_internal(exoid, varid, num_vars, var_names, obj_type, "ex_get_variable_names");
  if (status != NC_NOERR) {
    return (EX_FATAL);
  }
  return (EX_NOERR);
}
