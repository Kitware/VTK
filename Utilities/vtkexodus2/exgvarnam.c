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
* exgvnm - ex_get_variable_name
*
* entry conditions - 
*   input parameters:
*       int   exoid                   exodus file id
*       int   obj_type                variable type
*       int   var_num                 variable index to read 1..num_var
*
* exit conditions - 
*       char*   var_name                ptr to variable name
*
* revision history - 
*
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"

/*!
 * reads the name of a particular results variable from the database
 */

int ex_get_variable_name (int   exoid,
        ex_entity_type obj_type,
        int   var_num,
        char *var_name)
{
  int status;
  int varid;
  char errmsg[MAX_ERR_LENGTH];
  const char *vname = NULL;
   
  exerrval = 0; /* clear error code */

  /* inquire previously defined variables  */

  switch (obj_type) {
  case EX_GLOBAL:
    vname = VAR_NAME_GLO_VAR;
    break;
  case EX_NODAL:
    vname = VAR_NAME_NOD_VAR;
    break;
  case EX_EDGE_BLOCK:
    vname = VAR_NAME_EDG_VAR;
    break;
  case EX_FACE_BLOCK:
    vname = VAR_NAME_FAC_VAR;
    break;
  case EX_ELEM_BLOCK:
    vname = VAR_NAME_ELE_VAR;
    break;
  case EX_NODE_SET:
    vname = VAR_NAME_NSET_VAR;
    break;
  case EX_EDGE_SET:
    vname = VAR_NAME_ESET_VAR;
    break;
  case EX_FACE_SET:
    vname = VAR_NAME_FSET_VAR;
    break;
  case EX_SIDE_SET:
    vname = VAR_NAME_SSET_VAR;
    break;
  case EX_ELEM_SET:
    vname = VAR_NAME_ELSET_VAR;
    break;
  default:
    exerrval = EX_BADPARAM;
    sprintf( errmsg, "Error: Invalid variable type (%d) given for file id %d", obj_type, exoid );
    ex_err( "ex_get_variable_name", errmsg, exerrval );
    return (EX_FATAL);
  }

  if ((status = nc_inq_varid(exoid, vname, &varid)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
      "Warning: no %s variable names stored in file id %d",
      ex_name_of_object(obj_type), exoid);
    ex_err("ex_get_variable_name",errmsg,exerrval);
    return (EX_WARN);
  }

  /* read the variable name */
  status = ex_get_name_internal(exoid, varid, var_num-1, var_name, obj_type, "ex_get_variable_name");
  if (status != NC_NOERR) {
    return (EX_FATAL);
  }
  return (EX_NOERR);
}
