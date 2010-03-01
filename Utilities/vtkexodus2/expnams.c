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
* expnam - ex_put_names
*
* environment - UNIX
*
* entry conditions - 
*   input parameters:
*       int     exoid       exodus file id
*       int     obj_type    object type
*       char*   names       ptr array of entity names
*
* exit conditions - 
*
* revision history - 
*
*  Id
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"
#include <string.h>

/*!
 * writes the names of the results variables to the database
 * \param exoid       exodus file id
 * \param obj_type    object type
 * \param names       ptr array of entity names
 */

int ex_put_names (int   exoid,
                  ex_entity_type obj_type,
                  char* names[])
{
  int status;
  int varid; 
  size_t num_entity, i;
  size_t start[2], count[2];
  char errmsg[MAX_ERR_LENGTH];
  const char *vname = NULL;
   
  const char *routine = "ex_put_names";
   
  exerrval = 0; /* clear error code */

  switch (obj_type) {
    /*  ======== BLOCKS ========= */
  case EX_EDGE_BLOCK:
    vname = VAR_NAME_ED_BLK;
    break;
  case EX_FACE_BLOCK:
    vname = VAR_NAME_FA_BLK;
    break;
  case EX_ELEM_BLOCK:
    vname = VAR_NAME_EL_BLK;
    break;

    /*  ======== SETS ========= */
  case EX_NODE_SET:
    vname = VAR_NAME_NS;
    break;
  case EX_EDGE_SET:
    vname = VAR_NAME_ES;
    break;
  case EX_FACE_SET:
    vname = VAR_NAME_FS;
    break;
  case EX_SIDE_SET:
    vname = VAR_NAME_SS;
    break;
  case EX_ELEM_SET:
    vname = VAR_NAME_ELS;
    break;

    /*  ======== MAPS ========= */
  case EX_NODE_MAP:
    vname = VAR_NAME_NM;
    break;
  case EX_EDGE_MAP:
    vname = VAR_NAME_EDM;
    break;
  case EX_FACE_MAP:
    vname = VAR_NAME_FAM;
    break;
  case EX_ELEM_MAP:
    vname = VAR_NAME_EM;
    break;

    /*  ======== ERROR (Invalid type) ========= */
  default:
    exerrval = EX_BADPARAM;
    sprintf(errmsg,
            "Error: Invalid type specified in file id %d", exoid);
    ex_err(routine,errmsg,exerrval);
    return(EX_FATAL);
  }
   
  ex_get_dimension(exoid, ex_dim_num_objects(obj_type), ex_name_of_object(obj_type),
                   &num_entity, &varid, routine);

  if ((status = nc_inq_varid(exoid, vname, &varid)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
            "Error: failed to locate %s names in file id %d",
            ex_name_of_object(obj_type), exoid);
    ex_err(routine,errmsg,exerrval);
    return (EX_FATAL);
  }

  /* write EXODUS entitynames */
  for (i=0; i<num_entity; i++) {
    if (names[i] != '\0') {
      start[0] = i;
      start[1] = 0;
       
      count[0] = 1;
      count[1] = strlen(names[i]) + 1;
       
      if ((status = nc_put_vara_text(exoid, varid, start, count, names[i])) != NC_NOERR) {
        exerrval = status;
        sprintf(errmsg,
                "Error: failed to store %s names in file id %d",
                ex_name_of_object(obj_type), exoid);
        ex_err(routine,errmsg,exerrval);
        return (EX_FATAL);
      }
    }
  }
  return(EX_NOERR);
}
