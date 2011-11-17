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
* expmp - ex_put_attr_param
*
* entry conditions - 
*   input parameters:
*       int     exoid           exodus file id
*       int     obj_type        block/set type (node, edge, face, elem)
*       int     obj_id          block/set id (ignored for NODAL)       
*       int     num_attrs       number of attributes
*
* exit conditions - 
*
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"

/*!
 * defines the number of attributes.
 * \param   exoid           exodus file id
 * \param   obj_type        block/set type (node, edge, face, elem)
 * \param   obj_id          block/set id (ignored for NODAL)       
 * \param   num_attrs       number of attributes
 */

int ex_put_attr_param (int   exoid,
           ex_entity_type obj_type,
           int   obj_id,
           int   num_attrs)
{
  int status;
  int dims[2];
  int strdim, varid;
  
  char errmsg[MAX_ERR_LENGTH];
  const char *dnumobjent;
  const char *dnumobjatt;
  const char *vobjatt;
  const char *vattnam;
  int numobjentdim;
  int obj_id_ndx;
  int numattrdim;
  
  /* Determine index of obj_id in obj_type id array */
  if (obj_type == EX_NODAL)
    obj_id_ndx = 0;
  else {
    obj_id_ndx = ex_id_lkup(exoid,obj_type,obj_id);
    
    if (exerrval != 0) {
      if (exerrval == EX_NULLENTITY) {
  sprintf(errmsg,
    "Warning: no attributes found for NULL %s %d in file id %d",
    ex_name_of_object(obj_type),obj_id,exoid);
  ex_err("ex_put_attr_param",errmsg,EX_MSG);
  return (EX_WARN);              /* no attributes for this object */
      } else {
  sprintf(errmsg,
    "Warning: failed to locate %s id %d in id array in file id %d",
    ex_name_of_object(obj_type),obj_id, exoid);
  ex_err("ex_put_attr_param",errmsg,exerrval);
  return (EX_WARN);
      }
    }
  }

  switch (obj_type) {
  case EX_SIDE_SET:
    dnumobjent = DIM_NUM_SIDE_SS(obj_id_ndx);
    dnumobjatt = DIM_NUM_ATT_IN_SS(obj_id_ndx);
    vobjatt = VAR_SSATTRIB(obj_id_ndx);
    vattnam = VAR_NAME_SSATTRIB(obj_id_ndx);
    break;
  case EX_NODE_SET:
    dnumobjent = DIM_NUM_NOD_NS(obj_id_ndx);
    dnumobjatt = DIM_NUM_ATT_IN_NS(obj_id_ndx);
    vobjatt = VAR_NSATTRIB(obj_id_ndx);
    vattnam = VAR_NAME_NSATTRIB(obj_id_ndx);
    break;
  case EX_EDGE_SET:
    dnumobjent = DIM_NUM_EDGE_ES(obj_id_ndx);
    dnumobjatt = DIM_NUM_ATT_IN_ES(obj_id_ndx);
    vobjatt = VAR_ESATTRIB(obj_id_ndx);
    vattnam = VAR_NAME_ESATTRIB(obj_id_ndx);
    break;
  case EX_FACE_SET:
    dnumobjent = DIM_NUM_FACE_FS(obj_id_ndx);
    dnumobjatt = DIM_NUM_ATT_IN_FS(obj_id_ndx);
    vobjatt = VAR_FSATTRIB(obj_id_ndx);
    vattnam = VAR_NAME_FSATTRIB(obj_id_ndx);
    break;
  case EX_ELEM_SET:
    dnumobjent = DIM_NUM_ELE_ELS(obj_id_ndx);
    dnumobjatt = DIM_NUM_ATT_IN_ELS(obj_id_ndx);
    vobjatt = VAR_ELSATTRIB(obj_id_ndx);
    vattnam = VAR_NAME_ELSATTRIB(obj_id_ndx);
    break;
  case EX_NODAL:
    dnumobjent = DIM_NUM_NODES;
    dnumobjatt = DIM_NUM_ATT_IN_NBLK;
    vobjatt = VAR_NATTRIB;
    vattnam = VAR_NAME_NATTRIB;
    break;
  case EX_EDGE_BLOCK:
    dnumobjent = DIM_NUM_ED_IN_EBLK(obj_id_ndx);
    dnumobjatt = DIM_NUM_ATT_IN_EBLK(obj_id_ndx);
    vobjatt = VAR_EATTRIB(obj_id_ndx);
    vattnam = VAR_NAME_EATTRIB(obj_id_ndx);
    break;
  case EX_FACE_BLOCK:
    dnumobjent = DIM_NUM_FA_IN_FBLK(obj_id_ndx);
    dnumobjatt = DIM_NUM_ATT_IN_FBLK(obj_id_ndx);
    vobjatt = VAR_FATTRIB(obj_id_ndx);
    vattnam = VAR_NAME_FATTRIB(obj_id_ndx);
    break;
  case EX_ELEM_BLOCK:
    dnumobjent = DIM_NUM_EL_IN_BLK(obj_id_ndx);
    dnumobjatt = DIM_NUM_ATT_IN_BLK(obj_id_ndx);
    vobjatt = VAR_ATTRIB(obj_id_ndx);
    vattnam = VAR_NAME_ATTRIB(obj_id_ndx);
    break;
  default:
    exerrval = EX_BADPARAM;
    sprintf(errmsg, "Error: Bad block type (%d) specified for file id %d",
      obj_type, exoid );
    ex_err("ex_put_attr_param",errmsg,exerrval);
    return (EX_FATAL);
  }

  exerrval = 0; /* clear error code */

  if ((status = nc_inq_dimid(exoid, dnumobjent, &numobjentdim)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
      "Error: failed to locate number of entries for %s %d in file id %d",
      ex_name_of_object(obj_type), obj_id, exoid);
    ex_err("ex_put_attr_param",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* put netcdf file into define mode  */
  if ((status = nc_redef (exoid)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,"Error: failed to place file id %d into define mode",exoid);
    ex_err("ex_put_attr_param",errmsg,exerrval);
    return (EX_FATAL);
  }
  
  if ((status = nc_def_dim(exoid, dnumobjatt, num_attrs, &numattrdim)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
      "Error: failed to define number of attributes in %s %d in file id %d",
      ex_name_of_object(obj_type), obj_id,exoid);
    ex_err("ex_put_attr_param",errmsg,exerrval);
    goto error_ret;         /* exit define mode and return */
  }

  dims[0] = numobjentdim;
  dims[1] = numattrdim;
  
  if ((status = nc_def_var(exoid, vobjatt, nc_flt_code(exoid), 2, dims, &varid)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
      "Error:  failed to define attributes for %s %d in file id %d",
      ex_name_of_object(obj_type), obj_id,exoid);
    ex_err("ex_put_attr_param",errmsg,exerrval);
    goto error_ret;         /* exit define mode and return */
  }
  
  /* inquire previously defined dimensions  */
  if ((status = nc_inq_dimid(exoid, DIM_STR_NAME, &strdim)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
      "Error: failed to get string length in file id %d",exoid);
    ex_err("ex_put_attr_param",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* Attribute names... */
  dims[0] = numattrdim;
  dims[1] = strdim;

  if ((status = nc_def_var(exoid, vattnam, NC_CHAR, 2, dims, &varid)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
      "Error: failed to define %s attribute name array in file id %d",
      ex_name_of_object(obj_type), exoid);
    ex_err("ex_put_attr_param",errmsg,exerrval);
    goto error_ret;         /* exit define mode and return */
  }

  /* leave define mode  */
  if ((status = nc_enddef(exoid)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
      "Error: failed to complete %s attribute parameter definition in file id %d",
      ex_name_of_object(obj_type), exoid);
    ex_err("ex_put_attr_param",errmsg,exerrval);
    return (EX_FATAL);
  }

  return (EX_NOERR);

  /* Fatal error: exit definition mode and return */
 error_ret:
  if (nc_enddef (exoid) != NC_NOERR) {     /* exit define mode */       
    sprintf(errmsg,
      "Error: failed to complete definition for file id %d",
      exoid);
    ex_err("ex_put_attr_param",errmsg,exerrval);
  }
  return (EX_FATAL);
}
