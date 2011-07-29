/*
 * Copyright (c) 2006 Sandia Corporation. Under the terms of Contract
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
* exgeat - ex_get_attr_names
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*       int     obj_type                object type (edge/face/elem block)
*       int     obj_id                  object id (edge/face/elem block id)
*
* exit conditions - 
*       char*   names[]                 ptr array of attribute names
*
* revision history - 
*
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"

/*! \undoc */
/*
 * reads the attribute names for an element block
 */
int ex_get_attr_names( int   exoid,
                       ex_entity_type obj_type,
                       int   obj_id,
                       char **names)
{
  int status;
  int varid, numattrdim, obj_id_ndx;
  size_t num_attr, i;
  char errmsg[MAX_ERR_LENGTH];
  const char* dnumobjatt;
  const char* vattrbname;

  exerrval = 0; /* clear error code */

  /* Determine index of obj_id in vobjids array */
  if (obj_type != EX_NODAL) {
    obj_id_ndx = ex_id_lkup(exoid,obj_type,obj_id);
    if (exerrval != 0) {
      if (exerrval == EX_NULLENTITY) {
  sprintf(errmsg,
    "Warning: no attributes found for NULL %s %d in file id %d",
    ex_name_of_object(obj_type), obj_id, exoid);
  ex_err("ex_get_attr_names",errmsg,EX_MSG);
  return (EX_WARN);              /* no attributes for this object */
      } else {
  sprintf(errmsg,
    "Warning: failed to locate %s id %d in id array in file id %d",
    ex_name_of_object(obj_type), obj_id, exoid);
  ex_err("ex_get_attr_names",errmsg,exerrval);
  return (EX_WARN);
      }
    }
  }

  switch (obj_type) {
  case EX_NODE_SET:
    dnumobjatt = DIM_NUM_ATT_IN_NS(obj_id_ndx);
    vattrbname = VAR_NAME_NSATTRIB(obj_id_ndx);
    break;
  case EX_SIDE_SET:
    dnumobjatt = DIM_NUM_ATT_IN_SS(obj_id_ndx);
    vattrbname = VAR_NAME_SSATTRIB(obj_id_ndx);
    break;
  case EX_EDGE_SET:
    dnumobjatt = DIM_NUM_ATT_IN_ES(obj_id_ndx);
    vattrbname = VAR_NAME_ESATTRIB(obj_id_ndx);
    break;
  case EX_FACE_SET:
    dnumobjatt = DIM_NUM_ATT_IN_FS(obj_id_ndx);
    vattrbname = VAR_NAME_FSATTRIB(obj_id_ndx);
    break;
  case EX_ELEM_SET:
    dnumobjatt = DIM_NUM_ATT_IN_ELS(obj_id_ndx);
    vattrbname = VAR_NAME_ELSATTRIB(obj_id_ndx);
    break;
  case EX_NODAL:
    dnumobjatt = DIM_NUM_ATT_IN_NBLK;
    vattrbname = VAR_NAME_NATTRIB;
    break;
  case EX_EDGE_BLOCK:
    dnumobjatt = DIM_NUM_ATT_IN_EBLK(obj_id_ndx);
    vattrbname = VAR_NAME_EATTRIB(obj_id_ndx);
    break;
  case EX_FACE_BLOCK:
    dnumobjatt = DIM_NUM_ATT_IN_FBLK(obj_id_ndx);
    vattrbname = VAR_NAME_FATTRIB(obj_id_ndx);
    break;
  case EX_ELEM_BLOCK:
    dnumobjatt = DIM_NUM_ATT_IN_BLK(obj_id_ndx);
    vattrbname = VAR_NAME_ATTRIB(obj_id_ndx);
    break;
  default:
    exerrval = 1005;
    sprintf(errmsg,
      "Internal Error: unrecognized object type in switch: %d in file id %d",
      obj_type,exoid);
    ex_err("ex_get_attr_names",errmsg,EX_MSG);
    return (EX_FATAL);              /* number of attributes not defined */
  }
  /* inquire id's of previously defined dimensions  */

  if ((status = nc_inq_dimid(exoid, dnumobjatt, &numattrdim)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
      "Warning: no attributes found for %s %d in file id %d",
      ex_name_of_object(obj_type),obj_id,exoid);
    ex_err("ex_get_attr_names",errmsg,EX_MSG);
    return (EX_WARN);              /* no attributes for this object */
  }

  if ((status = nc_inq_dimlen(exoid, numattrdim, &num_attr)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
      "Error: failed to get number of attributes for %s %d in file id %d",
      ex_name_of_object(obj_type),obj_id,exoid);
    ex_err("ex_get_attr_names",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* It is OK if we don't find the attribute names since they were
     added at version 4.26; earlier databases won't have the names.
  */
  status = nc_inq_varid(exoid, vattrbname, &varid);

  /* read in the attributes */

  if (status == NC_NOERR) {
    /* read the names */
    status = ex_get_names_internal(exoid, varid, num_attr, names, obj_type, "ex_get_attr_names");
    if (status != NC_NOERR) {
      return (EX_FATAL);
    }
  } else {
    /* Names variable does not exist on the database; probably since this is an
     * older version of the database.  Return an empty array...
     */
    for (i=0; i<num_attr; i++) {
      names[i][0] = '\0';
    }
  }
  return(EX_NOERR);
}
