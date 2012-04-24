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
* expatn - ex_put_attr_names
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*       int     blk_type                block type (edge, face, elem)
*       int     blk_id                  block id
*       char*   names                   ptr to array of attribute names

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

/*!
 * writes the attribute names for a block
 * \param   exoid                   exodus file id
 * \param   blk_type                block type (edge, face, elem)
 * \param   blk_id                  block id
 * \param   names                   ptr to array of attribute names
 */
int ex_put_attr_names(int   exoid,
          ex_entity_type blk_type,
          int   blk_id,
          char* names[])
{
  int varid, numattrdim, blk_id_ndx;
  size_t num_attr;

  int status;
  char errmsg[MAX_ERR_LENGTH];
   
  exerrval = 0; /* clear error code */

  blk_id_ndx = ex_id_lkup(exoid, blk_type, blk_id);

  /* Determine index of blk_id in blk_id_ndx array */
  if (exerrval != 0) {
    if (exerrval == EX_NULLENTITY) {
      sprintf(errmsg,
        "Warning: no attributes allowed for NULL %s %d in file id %d",
        ex_name_of_object(blk_type),blk_id,exoid);
      ex_err("ex_put_attr_names",errmsg,EX_MSG);
      return (EX_WARN);              /* no attributes for this block */
    } else {
      sprintf(errmsg,
        "Error: no %s id %d in %s array in file id %d",
        ex_name_of_object(blk_type), blk_id, VAR_ID_EL_BLK, exoid);
      ex_err("ex_put_attr_names",errmsg,exerrval);
      return (EX_FATAL);
    }
  }

  /* inquire id's of previously defined dimensions  */
  switch (blk_type) {
  case EX_SIDE_SET:
    status = nc_inq_dimid(exoid, DIM_NUM_ATT_IN_SS(blk_id_ndx), &numattrdim);
    break;
  case EX_NODE_SET:
    status = nc_inq_dimid(exoid, DIM_NUM_ATT_IN_NS(blk_id_ndx), &numattrdim);
    break;
  case EX_EDGE_SET:
    status = nc_inq_dimid(exoid, DIM_NUM_ATT_IN_ES(blk_id_ndx), &numattrdim);
    break;
  case EX_FACE_SET:
    status = nc_inq_dimid(exoid, DIM_NUM_ATT_IN_FS(blk_id_ndx), &numattrdim);
    break;
  case EX_ELEM_SET:
    status = nc_inq_dimid(exoid, DIM_NUM_ATT_IN_ELS(blk_id_ndx), &numattrdim);
    break;
  case EX_NODAL:
    status = nc_inq_dimid(exoid, DIM_NUM_ATT_IN_NBLK, &numattrdim);
    break;
  case EX_EDGE_BLOCK:
    status = nc_inq_dimid(exoid, DIM_NUM_ATT_IN_EBLK(blk_id_ndx), &numattrdim);
    break;
  case EX_FACE_BLOCK:
    status = nc_inq_dimid(exoid, DIM_NUM_ATT_IN_FBLK(blk_id_ndx), &numattrdim);
    break;
  case EX_ELEM_BLOCK:
    status = nc_inq_dimid(exoid, DIM_NUM_ATT_IN_BLK(blk_id_ndx), &numattrdim);
    break;
  default:
    exerrval = 1005;
    sprintf(errmsg,
      "Internal Error: unrecognized object type in switch: %d in file id %d",
      blk_type,exoid);
    ex_err("ex_put_attr_names",errmsg,EX_MSG);
    return (EX_FATAL);              /* number of attributes not defined */
  }

  if (status != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
      "Error: number of attributes not defined for %s %d in file id %d",
      ex_name_of_object(blk_type),blk_id,exoid);
    ex_err("ex_put_attr_names",errmsg,EX_MSG);
    return (EX_FATAL);              /* number of attributes not defined */
  }

  if ((status = nc_inq_dimlen(exoid, numattrdim, &num_attr)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
      "Error: failed to get number of attributes for %s %d in file id %d",
      ex_name_of_object(blk_type),blk_id,exoid);
    ex_err("ex_put_attr_names",errmsg,exerrval);
    return (EX_FATAL);
  }

  switch (blk_type) {
  case EX_SIDE_SET:
    status = nc_inq_varid (exoid, VAR_NAME_SSATTRIB(blk_id_ndx), &varid);
    break;
  case EX_NODE_SET:
    status = nc_inq_varid (exoid, VAR_NAME_NSATTRIB(blk_id_ndx), &varid);
    break;
  case EX_EDGE_SET:
    status = nc_inq_varid (exoid, VAR_NAME_ESATTRIB(blk_id_ndx), &varid);
    break;
  case EX_FACE_SET:
    status = nc_inq_varid (exoid, VAR_NAME_FSATTRIB(blk_id_ndx), &varid);
    break;
  case EX_ELEM_SET:
    status = nc_inq_varid (exoid, VAR_NAME_ELSATTRIB(blk_id_ndx), &varid);
    break;
  case EX_NODAL:
    status = nc_inq_varid (exoid, VAR_NAME_NATTRIB, &varid);
    break;
  case EX_EDGE_BLOCK:
    status = nc_inq_varid (exoid, VAR_NAME_EATTRIB(blk_id_ndx), &varid);
    break;
  case EX_FACE_BLOCK:
    status = nc_inq_varid (exoid, VAR_NAME_FATTRIB(blk_id_ndx), &varid);
    break;
  case EX_ELEM_BLOCK:
    status = nc_inq_varid (exoid, VAR_NAME_ATTRIB(blk_id_ndx), &varid);
    break;
  default:
    exerrval = 1005;
    sprintf(errmsg,
      "Internal Error: unrecognized object type in switch: %d in file id %d",
      blk_type,exoid);
    ex_err("ex_put_attr_names",errmsg,EX_MSG);
    return (EX_FATAL);              /* number of attributes not defined */
  }

  if (status != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
      "Error: failed to locate %s attribute names for %s %d in file id %d",
      ex_name_of_object(blk_type),ex_name_of_object(blk_type),blk_id, exoid);
    ex_err("ex_put_attr_names",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* write out the attributes  */
  status = ex_put_names_internal(exoid, varid, num_attr, names, blk_type,
         "attribute", "ex_put_attr_names");

  return(status);
}
