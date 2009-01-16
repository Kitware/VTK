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
* expnam - ex_put_name
*
* environment - UNIX
*
* entry conditions - 
*   input parameters:
*       int     exoid          exodus file id
*       int     obj_type       object type
*       int     entity_id      id of entity name to write
*       char*   name           ptr to entity name
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
 * writes the name of the specified entity to the database.
 * \param  exoid          exodus file id
 * \param  obj_type       object type
 * \param  entity_id      id of entity name to write
 * \param  name           ptr to entity name
 */

int ex_put_name (int   exoid,
		 ex_entity_type obj_type,
		 int   entity_id,
		 const char *name)
{
  int status;
  int varid, ent_ndx; 
  size_t  start[2], count[2];
  char errmsg[MAX_ERR_LENGTH];
  const char *routine = "ex_put_name";
  const char *vobj;
   
  exerrval = 0; /* clear error code */

  switch(obj_type) {
  case EX_EDGE_BLOCK:
    vobj = VAR_NAME_ED_BLK;
    break;
  case EX_FACE_BLOCK:
    vobj = VAR_NAME_FA_BLK;
    break;
  case EX_ELEM_BLOCK:
    vobj = VAR_NAME_EL_BLK;
    break;
  case EX_NODE_SET:
    vobj = VAR_NAME_NS;
    break;
  case EX_SIDE_SET:
    vobj = VAR_NAME_SS;
    break;
  case EX_EDGE_SET:
    vobj = VAR_NAME_ES;
    break;
  case EX_FACE_SET:
    vobj = VAR_NAME_FS;
    break;
  case EX_ELEM_SET:
    vobj = VAR_NAME_ELS;
    break;
  case EX_NODE_MAP:
    vobj = VAR_NAME_NM;
    break;
  case EX_EDGE_MAP:
    vobj = VAR_NAME_EDM;
    break;
  case EX_FACE_MAP:
    vobj = VAR_NAME_FAM;
    break;
  case EX_ELEM_MAP:
    vobj = VAR_NAME_EM;
    break;
  default:
    exerrval = EX_BADPARAM;
    sprintf(errmsg,
	    "Error: Invalid type specified in file id %d", exoid);
    ex_err(routine,errmsg,exerrval);
    return(EX_FATAL);
  }
   
  if ((status = nc_inq_varid(exoid, vobj, &varid)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
	    "Error: failed to locate %s names in file id %d",
	    ex_name_of_object(obj_type), exoid);
    ex_err(routine,errmsg,exerrval);
    return (EX_FATAL);
  }

  ent_ndx = ex_id_lkup(exoid, obj_type, entity_id);

  /* If this is a null entity, then 'ent_ndx' will be negative.
   * We don't care in this routine, so make it positive and continue...
   */
  if (ent_ndx < 0) ent_ndx = -ent_ndx;
   
  /* write EXODUS entityname */
  start[0] = ent_ndx-1;
  start[1] = 0;
   
  count[0] = 1;
  count[1] = strlen(name) + 1;
   
  if ((status = nc_put_vara_text(exoid, varid, start, count, name)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
	    "Error: failed to store %s name for id %d in file id %d",
	    ex_name_of_object(obj_type), entity_id, exoid);
    ex_err(routine,errmsg,exerrval);
    return (EX_FATAL);
  }
  return(EX_NOERR);
}
