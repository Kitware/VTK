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
/*
 *  Id
 *
 *****************************************************************************/

#include <stdlib.h>
#include "exodusII.h"
#include "exodusII_int.h"

/*
 *  reads the element block ids from the database
 */

int ex_get_ids (int  exoid,
                ex_entity_type obj_type, 
                int *ids)
{
  int varid, status;
  char errmsg[MAX_ERR_LENGTH];

  const char* varidobj;

  exerrval = 0; /* clear error code */

  switch (obj_type) {
  case EX_EDGE_BLOCK:
    varidobj = VAR_ID_ED_BLK;
    break;
  case EX_FACE_BLOCK:
    varidobj = VAR_ID_FA_BLK;
    break;
  case EX_ELEM_BLOCK:
    varidobj = VAR_ID_EL_BLK;
    break;
  case EX_NODE_SET:
    varidobj = VAR_NS_IDS;
    break;
  case EX_EDGE_SET:
    varidobj = VAR_ES_IDS;
    break;
  case EX_FACE_SET:
    varidobj = VAR_FS_IDS;
    break;
  case EX_SIDE_SET:
    varidobj = VAR_SS_IDS;
    break;
  case EX_ELEM_SET:
    varidobj = VAR_ELS_IDS;
    break;
  case EX_NODE_MAP:
    varidobj = VAR_NM_PROP(1);
    break;
  case EX_EDGE_MAP:
    varidobj = VAR_EDM_PROP(1);
    break;
  case EX_FACE_MAP:
    varidobj = VAR_FAM_PROP(1);
    break;
  case EX_ELEM_MAP:
    varidobj = VAR_EM_PROP(1);
    break;
  default:/* invalid variable type */
    exerrval = EX_BADPARAM;
    sprintf(errmsg, "Error: Invalid type specified in file id %d", exoid);
    ex_err("ex_get_ids",errmsg,exerrval);
    return(EX_FATAL);
  }

  /* Determine if there are any 'obj-type' objects */
  if ((status = nc_inq_dimid (exoid, ex_dim_num_objects(obj_type), &varid)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
            "Warning: no %s defined in file id %d",
            ex_name_of_object(obj_type), exoid);
    ex_err("ex_get_ids",errmsg,exerrval);
    return (EX_WARN);
  }


  /* inquire id's of previously defined dimensions and variables  */
  if ((status = nc_inq_varid(exoid, varidobj, &varid)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
            "Error: failed to locate %s ids variable in file id %d",
            ex_name_of_object(obj_type),exoid);
    ex_err("ex_get_ids",errmsg,exerrval);
    return (EX_FATAL);
  }
  
  /* read in the element block ids  */
  status = nc_get_var_int(exoid, varid, ids);
  
  if (status != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
            "Error: failed to return %s ids in file id %d",
            ex_name_of_object(obj_type),exoid);
    ex_err("ex_get_ids",errmsg,exerrval);
    return (EX_FATAL);
  }
  return(EX_NOERR);
}
