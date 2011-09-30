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
* expatt - ex_put_attr
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*       int     blk_type                block type
*       int     blk_id                  block id
*       float*  attrib                  array of attributes
*
* exit conditions - 
*
* revision history - 
*
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"

/*!
 * writes the attributes for an edge/face/element block
 * \param   exoid                   exodus file id
 * \param   blk_type                block type
 * \param   blk_id                  block id
 * \param   attrib                  array of attributes
 */

int ex_put_attr (int   exoid,
     ex_entity_type blk_type,
     int   blk_id,
     const void *attrib)
{
  int status;
  int attrid, blk_id_ndx;
  char errmsg[MAX_ERR_LENGTH];

  exerrval = 0; /* clear error code */

  if ( blk_type != EX_NODAL ) {
    /* Determine index of blk_id in VAR_ID_EL_BLK array */
    blk_id_ndx = ex_id_lkup(exoid,blk_type,blk_id);
    if (exerrval != 0) {
      if (exerrval == EX_NULLENTITY) {
        sprintf(errmsg,
    "Warning: no attributes allowed for NULL %s %d in file id %d",
                ex_name_of_object(blk_type),blk_id,exoid);
        ex_err("ex_put_attr",errmsg,EX_MSG);
        return (EX_WARN);              /* no attributes for this block */
      } else {
        sprintf(errmsg,
    "Error: no %s id %d in in file id %d",
                ex_name_of_object(blk_type), blk_id, exoid);
        ex_err("ex_put_attr",errmsg,exerrval);
        return (EX_FATAL);
      }
    }
  }

  switch (blk_type) {
  case EX_SIDE_SET:
    status = nc_inq_varid (exoid, VAR_SSATTRIB(blk_id_ndx), &attrid);
    break;
  case EX_NODE_SET:
    status = nc_inq_varid (exoid, VAR_NSATTRIB(blk_id_ndx), &attrid);
    break;
  case EX_EDGE_SET:
    status = nc_inq_varid (exoid, VAR_ESATTRIB(blk_id_ndx), &attrid);
    break;
  case EX_FACE_SET:
    status = nc_inq_varid (exoid, VAR_FSATTRIB(blk_id_ndx), &attrid);
    break;
  case EX_ELEM_SET:
    status = nc_inq_varid (exoid, VAR_ELSATTRIB(blk_id_ndx), &attrid);
    break;
  case EX_NODAL:
    status = nc_inq_varid (exoid, VAR_NATTRIB, &attrid);
    break;
  case EX_EDGE_BLOCK:
    status = nc_inq_varid (exoid, VAR_EATTRIB(blk_id_ndx), &attrid);
    break;
  case EX_FACE_BLOCK:
    status = nc_inq_varid (exoid, VAR_FATTRIB(blk_id_ndx), &attrid);
    break;
  case EX_ELEM_BLOCK:
    status = nc_inq_varid (exoid, VAR_ATTRIB(blk_id_ndx), &attrid);
    break;
  default:
    exerrval = 1005;
    sprintf(errmsg,
      "Internal Error: unrecognized object type in switch: %d in file id %d",
      blk_type,exoid);
    ex_err("ex_put_attr",errmsg,EX_MSG);
    return (EX_FATAL);              /* number of attributes not defined */
  }

  if (status != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
      "Error: failed to locate attribute variable for %s %d in file id %d",
            ex_name_of_object(blk_type),blk_id,exoid);
    ex_err("ex_put_attr",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* write out the attributes  */
  if (ex_comp_ws(exoid) == 4) {
    status = nc_put_var_float(exoid, attrid, attrib);
  } else {
    status = nc_put_var_double(exoid, attrid, attrib);
  }

  if (status != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
            "Error: failed to put attributes for %s %d in file id %d",
            ex_name_of_object(blk_type),blk_id,exoid);
    ex_err("ex_put_attr",errmsg,exerrval);
    return (EX_FATAL);
  }
  return(EX_NOERR);
}
