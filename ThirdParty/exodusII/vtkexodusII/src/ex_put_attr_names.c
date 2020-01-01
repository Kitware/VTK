/*
 * Copyright (c) 2005-2017 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
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
 *     * Neither the name of NTESS nor the names of its
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

#include "exodusII.h"     // for ex_err, ex_name_of_object, etc
#include "exodusII_int.h" // for EX_FATAL, ex__id_lkup, etc

/*!
 * writes the attribute names for a block
 * \param   exoid                   exodus file id
 * \param   blk_type                block type (edge, face, elem)
 * \param   blk_id                  block id
 * \param   names                   ptr to array of attribute names
 */
int ex_put_attr_names(int exoid, ex_entity_type blk_type, ex_entity_id blk_id, char *names[])
{
  int    varid, numattrdim, blk_id_ndx;
  size_t num_attr;

  int  status;
  char errmsg[MAX_ERR_LENGTH];

  EX_FUNC_ENTER();
  ex__check_valid_file_id(exoid, __func__);

  blk_id_ndx = ex__id_lkup(exoid, blk_type, blk_id);
  if (blk_id_ndx <= 0) {
    ex_get_err(NULL, NULL, &status);

    if (status != 0) {
      if (status == EX_NULLENTITY) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "Warning: no attributes allowed for NULL %s %" PRId64 " in file id %d",
                 ex_name_of_object(blk_type), blk_id, exoid);
        ex_err_fn(exoid, __func__, errmsg, EX_NULLENTITY);
        EX_FUNC_LEAVE(EX_WARN); /* no attributes for this block */
      }
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: no %s id %" PRId64 " in %s array in file id %d",
               ex_name_of_object(blk_type), blk_id, VAR_ID_EL_BLK, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }

  /* inquire id's of previously defined dimensions  */
  switch (blk_type) {
  case EX_SIDE_SET: status = nc_inq_dimid(exoid, DIM_NUM_ATT_IN_SS(blk_id_ndx), &numattrdim); break;
  case EX_NODE_SET: status = nc_inq_dimid(exoid, DIM_NUM_ATT_IN_NS(blk_id_ndx), &numattrdim); break;
  case EX_EDGE_SET: status = nc_inq_dimid(exoid, DIM_NUM_ATT_IN_ES(blk_id_ndx), &numattrdim); break;
  case EX_FACE_SET: status = nc_inq_dimid(exoid, DIM_NUM_ATT_IN_FS(blk_id_ndx), &numattrdim); break;
  case EX_ELEM_SET:
    status = nc_inq_dimid(exoid, DIM_NUM_ATT_IN_ELS(blk_id_ndx), &numattrdim);
    break;
  case EX_NODAL: status = nc_inq_dimid(exoid, DIM_NUM_ATT_IN_NBLK, &numattrdim); break;
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
    snprintf(errmsg, MAX_ERR_LENGTH,
             "Internal ERROR: unrecognized object type in switch: %d in file id %d", blk_type,
             exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL); /* number of attributes not defined */
  }

  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: number of attributes not defined for %s %" PRId64 " in file id %d",
             ex_name_of_object(blk_type), blk_id, exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_MSG);
    EX_FUNC_LEAVE(EX_FATAL); /* number of attributes not defined */
  }

  if ((status = nc_inq_dimlen(exoid, numattrdim, &num_attr)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to get number of attributes for %s %" PRId64 " in file id %d",
             ex_name_of_object(blk_type), blk_id, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  switch (blk_type) {
  case EX_SIDE_SET: status = nc_inq_varid(exoid, VAR_NAME_SSATTRIB(blk_id_ndx), &varid); break;
  case EX_NODE_SET: status = nc_inq_varid(exoid, VAR_NAME_NSATTRIB(blk_id_ndx), &varid); break;
  case EX_EDGE_SET: status = nc_inq_varid(exoid, VAR_NAME_ESATTRIB(blk_id_ndx), &varid); break;
  case EX_FACE_SET: status = nc_inq_varid(exoid, VAR_NAME_FSATTRIB(blk_id_ndx), &varid); break;
  case EX_ELEM_SET: status = nc_inq_varid(exoid, VAR_NAME_ELSATTRIB(blk_id_ndx), &varid); break;
  case EX_NODAL: status = nc_inq_varid(exoid, VAR_NAME_NATTRIB, &varid); break;
  case EX_EDGE_BLOCK: status = nc_inq_varid(exoid, VAR_NAME_EATTRIB(blk_id_ndx), &varid); break;
  case EX_FACE_BLOCK: status = nc_inq_varid(exoid, VAR_NAME_FATTRIB(blk_id_ndx), &varid); break;
  case EX_ELEM_BLOCK: status = nc_inq_varid(exoid, VAR_NAME_ATTRIB(blk_id_ndx), &varid); break;
  default:
    snprintf(errmsg, MAX_ERR_LENGTH,
             "Internal ERROR: unrecognized object type in switch: %d in file id %d", blk_type,
             exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL); /* number of attributes not defined */
  }

  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to locate %s attribute names for %s %" PRId64 " in file id %d",
             ex_name_of_object(blk_type), ex_name_of_object(blk_type), blk_id, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* write out the attributes  */
  status = ex__put_names(exoid, varid, num_attr, names, blk_type, "attribute", __func__);

  EX_FUNC_LEAVE(status);
}
