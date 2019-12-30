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
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE P!OSSIBILITY OF SUCH DAMAGE.
 *
 */
/*****************************************************************************
 *
 * expatt - ex_put_partial_attr
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

#include "exodusII.h"     // for ex_err, ex_name_of_object, etc
#include "exodusII_int.h" // for ex__check_valid_file_id, etc

/*!
 * writes the attributes for an edge/face/element block
 * \param   exoid                   exodus file id
 * \param   blk_type                block type
 * \param   blk_id                  block id
 * \param   start_entity            the starting index (1-based) of the attribute to be written
 * \param   num_entity              the number of entities to write attributes
 * \param   attrib                  array of attributes
 */

int ex_put_partial_attr(int exoid, ex_entity_type blk_type, ex_entity_id blk_id,
                        int64_t start_entity, int64_t num_entity, const void *attrib)
{
  int    status;
  int    attrid;
  int    blk_id_ndx = 0;
  int    numattrdim;
  size_t start[2], count[2];
  size_t num_attr;
  char   errmsg[MAX_ERR_LENGTH];

  EX_FUNC_ENTER();
  ex__check_valid_file_id(exoid, __func__);

  if (blk_type != EX_NODAL) {
    /* Determine index of blk_id in VAR_ID_EL_BLK array */
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
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: no %s id %" PRId64 " in in file id %d",
                 ex_name_of_object(blk_type), blk_id, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }
    }
  }

  switch (blk_type) {
  case EX_SIDE_SET: status = nc_inq_varid(exoid, VAR_SSATTRIB(blk_id_ndx), &attrid); break;
  case EX_NODE_SET: status = nc_inq_varid(exoid, VAR_NSATTRIB(blk_id_ndx), &attrid); break;
  case EX_EDGE_SET: status = nc_inq_varid(exoid, VAR_ESATTRIB(blk_id_ndx), &attrid); break;
  case EX_FACE_SET: status = nc_inq_varid(exoid, VAR_FSATTRIB(blk_id_ndx), &attrid); break;
  case EX_ELEM_SET: status = nc_inq_varid(exoid, VAR_ELSATTRIB(blk_id_ndx), &attrid); break;
  case EX_NODAL: status = nc_inq_varid(exoid, VAR_NATTRIB, &attrid); break;
  case EX_EDGE_BLOCK: status = nc_inq_varid(exoid, VAR_EATTRIB(blk_id_ndx), &attrid); break;
  case EX_FACE_BLOCK: status = nc_inq_varid(exoid, VAR_FATTRIB(blk_id_ndx), &attrid); break;
  case EX_ELEM_BLOCK: status = nc_inq_varid(exoid, VAR_ATTRIB(blk_id_ndx), &attrid); break;
  default:
    snprintf(errmsg, MAX_ERR_LENGTH,
             "Internal ERROR: unrecognized object type in switch: %d in file id %d", blk_type,
             exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL); /* number of attributes not defined */
  }

  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to locate attribute variable for %s %" PRId64 " in file id %d",
             ex_name_of_object(blk_type), blk_id, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Determine number of attributes */
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
    /* No need for error message, handled in previous switch; just to quiet
     * compiler. */
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: number of attributes not defined for %s %" PRId64 " in file id %d",
             ex_name_of_object(blk_type), blk_id, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL); /* number of attributes not defined */
  }

  if ((status = nc_inq_dimlen(exoid, numattrdim, &num_attr)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to get number of attributes for %s %" PRId64 " in file id %d",
             ex_name_of_object(blk_type), blk_id, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* write out the attributes  */
  start[0] = --start_entity;
  start[1] = 0;

  count[0] = num_entity;
  count[1] = num_attr;
  if (count[0] == 0) {
    start[0] = 0;
  }

  if (ex__comp_ws(exoid) == 4) {
    status = nc_put_vara_float(exoid, attrid, start, count, attrib);
  }
  else {
    status = nc_put_vara_double(exoid, attrid, start, count, attrib);
  }

  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to put attributes for %s %" PRId64 " in file id %d",
             ex_name_of_object(blk_type), blk_id, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }
  EX_FUNC_LEAVE(EX_NOERR);
}
