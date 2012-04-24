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
* exgatt - ex_get_attr
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*       int     obj_type                object type (edge/face/element block)
*       int     obj_id                  object id (edge id, face id, elem id)
*
* exit conditions - 
*       float*  attrib                  array of attributes
*
* revision history - 
*
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"

/*!
 * \undoc reads the attributes for an edge, face, or element block
 */

int ex_get_attr( int   exoid,
                 ex_entity_type obj_type,
                 int   obj_id,
                 void* attrib )

{
  int status;
  int attrid, obj_id_ndx;
  char errmsg[MAX_ERR_LENGTH];
  const char* vattrbname;

  exerrval = 0; /* clear error code */

  /* Determine index of obj_id in vobjids array */
  if (obj_type == EX_NODAL)
    obj_id_ndx = 0;
  else {
    obj_id_ndx = ex_id_lkup(exoid,obj_type,obj_id);
    
    if (exerrval != 0) {
      if (exerrval == EX_NULLENTITY) {
  sprintf(errmsg,
    "Warning: no attributes found for NULL %s %d in file id %d",
    ex_name_of_object(obj_type),obj_id,exoid);
  ex_err("ex_get_attr",errmsg,EX_MSG);
  return (EX_WARN);              /* no attributes for this object */
      } else {
  sprintf(errmsg,
    "Warning: failed to locate %s id %d in id array in file id %d",
    ex_name_of_object(obj_type),obj_id, exoid);
  ex_err("ex_get_attr",errmsg,exerrval);
  return (EX_WARN);
      }
    }
  }

  switch (obj_type) {
  case EX_SIDE_SET:
    vattrbname = VAR_SSATTRIB(obj_id_ndx);
    break;
  case EX_NODE_SET:
    vattrbname = VAR_NSATTRIB(obj_id_ndx);
    break;
  case EX_EDGE_SET:
    vattrbname = VAR_ESATTRIB(obj_id_ndx);
    break;
  case EX_FACE_SET:
    vattrbname = VAR_FSATTRIB(obj_id_ndx);
    break;
  case EX_ELEM_SET:
    vattrbname = VAR_ELSATTRIB(obj_id_ndx);
    break;
  case EX_NODAL:
    vattrbname = VAR_NATTRIB;
    break;
  case EX_EDGE_BLOCK:
    vattrbname = VAR_EATTRIB(obj_id_ndx);
    break;
  case EX_FACE_BLOCK:
    vattrbname = VAR_FATTRIB(obj_id_ndx);
    break;
  case EX_ELEM_BLOCK:
    vattrbname = VAR_ATTRIB(obj_id_ndx);
    break;
  default:
    exerrval = 1005;
    sprintf(errmsg,
      "Internal Error: unrecognized object type in switch: %d in file id %d",
      obj_type,exoid);
    ex_err("ex_get_attr",errmsg,EX_MSG);
    return (EX_FATAL);              /* number of attributes not defined */
  }

  /* inquire id's of previously defined dimensions  */
  if ((status = nc_inq_varid(exoid, vattrbname, &attrid)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
            "Error: failed to locate attributes for %s %d in file id %d",
            ex_name_of_object(obj_type), obj_id,exoid);
    ex_err("ex_get_attr",errmsg,exerrval);
    return (EX_FATAL);
  }


  /* read in the attributes */
  if (ex_comp_ws(exoid) == 4) {
    status = nc_get_var_float(exoid, attrid, attrib);
  } else {
    status = nc_get_var_double(exoid, attrid, attrib);
  }

  if (status != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
            "Error: failed to get attributes for %s %d in file id %d",
            ex_name_of_object(obj_type),obj_id,exoid);
    ex_err("ex_get_attr",errmsg,exerrval);
    return (EX_FATAL);
  }
  return(EX_NOERR);
}
