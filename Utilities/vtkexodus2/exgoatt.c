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
* exgeat - ex_get_one_attr
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*       int     obj_type                object type (edge, face, elem block)
*       int     obj_id                  object id (edge face, elem block ID)
*
* exit conditions - 
*       float*  attrib                  array of attributes
*
* revision history - 
*
*  Id
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"

/*
 * reads the attributes for an edge, face, or element block
 */
int ex_get_one_attr( int   exoid,
                     ex_entity_type obj_type,
                     int   obj_id,
                     int   attrib_index,
                     void* attrib )

{
  int status;
  int attrid, obj_id_ndx = 0;
  int temp;
  size_t num_entries_this_obj, num_attr;
  size_t start[2], count[2];
  ptrdiff_t stride[2];
  char errmsg[MAX_ERR_LENGTH];
  const char* dnumobjent;
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
		ex_name_of_object(obj_type),obj_id,exoid);
	ex_err("ex_get_one_attr",errmsg,EX_MSG);
	return (EX_WARN);              /* no attributes for this object */
      } else {
	sprintf(errmsg,
		"Warning: failed to locate %s id %d in id array in file id %d",
		ex_name_of_object(obj_type),obj_id, exoid);
	ex_err("ex_get_one_attr",errmsg,exerrval);
	return (EX_WARN);
      }
    }
  }

  switch (obj_type) {
  case EX_SIDE_SET:
    dnumobjent = DIM_NUM_SIDE_SS(obj_id_ndx);
    dnumobjatt = DIM_NUM_ATT_IN_SS(obj_id_ndx);
    vattrbname = VAR_SSATTRIB(obj_id_ndx);
    break;
  case EX_NODE_SET:
    dnumobjent = DIM_NUM_NOD_NS(obj_id_ndx);
    dnumobjatt = DIM_NUM_ATT_IN_NS(obj_id_ndx);
    vattrbname = VAR_NSATTRIB(obj_id_ndx);
    break;
  case EX_EDGE_SET:
    dnumobjent = DIM_NUM_EDGE_ES(obj_id_ndx);
    dnumobjatt = DIM_NUM_ATT_IN_ES(obj_id_ndx);
    vattrbname = VAR_ESATTRIB(obj_id_ndx);
    break;
  case EX_FACE_SET:
    dnumobjent = DIM_NUM_FACE_FS(obj_id_ndx);
    dnumobjatt = DIM_NUM_ATT_IN_FS(obj_id_ndx);
    vattrbname = VAR_FSATTRIB(obj_id_ndx);
    break;
  case EX_ELEM_SET:
    dnumobjent = DIM_NUM_ELE_ELS(obj_id_ndx);
    dnumobjatt = DIM_NUM_ATT_IN_ELS(obj_id_ndx);
    vattrbname = VAR_ELSATTRIB(obj_id_ndx);
    break;
  case EX_NODAL:
    dnumobjent = DIM_NUM_NODES;
    dnumobjatt = DIM_NUM_ATT_IN_NBLK;
    vattrbname = VAR_NATTRIB;
    break;
  case EX_EDGE_BLOCK:
    dnumobjent = DIM_NUM_ED_IN_EBLK(obj_id_ndx);
    dnumobjatt = DIM_NUM_ATT_IN_EBLK(obj_id_ndx);
    vattrbname = VAR_EATTRIB(obj_id_ndx);
    break;
  case EX_FACE_BLOCK:
    dnumobjent = DIM_NUM_FA_IN_FBLK(obj_id_ndx);
    dnumobjatt = DIM_NUM_ATT_IN_FBLK(obj_id_ndx);
    vattrbname = VAR_FATTRIB(obj_id_ndx);
    break;
  case EX_ELEM_BLOCK:
    dnumobjent = DIM_NUM_EL_IN_BLK(obj_id_ndx);
    dnumobjatt = DIM_NUM_ATT_IN_BLK(obj_id_ndx);
    vattrbname = VAR_ATTRIB(obj_id_ndx);
    break;
  default:
    sprintf(errmsg,
      "Error: Called with invalid obj_type %d", obj_type);
	ex_err("ex_get_one_attr",errmsg,exerrval);
	return (EX_FATAL);
  }

  /* inquire id's of previously defined dimensions  */
  if (ex_get_dimension(exoid, dnumobjent,"entries", &num_entries_this_obj, &temp, "ex_get_one_attr") != NC_NOERR)
    return EX_FATAL;
  
  if (ex_get_dimension(exoid, dnumobjatt,"attributes", &num_attr, &temp, "ex_get_one_attr") != NC_NOERR)
    return EX_FATAL;

  if (attrib_index < 1 || attrib_index > (int)num_attr) {
    exerrval = EX_FATAL;
    sprintf(errmsg,
            "Error: Invalid attribute index specified: %d.  Valid range is 1 to %d for %s %d in file id %d",
            attrib_index, (int)num_attr, ex_name_of_object(obj_type), obj_id, exoid);
    ex_err("ex_get_one_attr",errmsg,exerrval);
    return (EX_FATAL);
  }

  if ((status = nc_inq_varid(exoid, vattrbname, &attrid)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
	    "Error: failed to locate attributes for %s %d in file id %d",
	    ex_name_of_object(obj_type),obj_id,exoid);
    ex_err("ex_get_one_attr",errmsg,exerrval);
    return (EX_FATAL);
  }


  /* read in the attributes */
  start[0] = 0;
  start[1] = attrib_index-1;

  count[0] = num_entries_this_obj;
  count[1] = 1;

  stride[0] = 1;
  stride[1] = num_attr;
  
  if (ex_comp_ws(exoid) == 4) {
    status = nc_get_vars_float(exoid, attrid, start, count, stride, attrib);
  } else {
    status = nc_get_vars_double(exoid, attrid, start, count, stride, attrib);
  }

  if (status != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
            "Error: failed to get attribute %d for %s %d in file id %d",
            attrib_index, ex_name_of_object(obj_type), obj_id,exoid);
    ex_err("ex_get_one_attr",errmsg,exerrval);
    return (EX_FATAL);
  }
  return(EX_NOERR);
}
