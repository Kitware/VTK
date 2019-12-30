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
 * expmp - ex_get_attr_param
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

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, EX_NOERR, etc

/*!
 * \undoc retrieves the number of attributes.
 */

int ex_get_attr_param(int exoid, ex_entity_type obj_type, ex_entity_id obj_id, int *num_attrs)
{
  int status;
  int dimid;

  char        errmsg[MAX_ERR_LENGTH];
  const char *dnumobjatt;

  int    obj_id_ndx;
  size_t lnum_attr_per_entry;

  EX_FUNC_ENTER();
  ex__check_valid_file_id(exoid, __func__);

  /* Determine index of obj_id in vobjids array */
  if (obj_type == EX_NODAL) {
    obj_id_ndx = 0;
  }
  else {
    obj_id_ndx = ex__id_lkup(exoid, obj_type, obj_id);
    if (obj_id_ndx <= 0) {
      ex_get_err(NULL, NULL, &status);

      if (status != 0) {
        if (status == EX_NULLENTITY) {
          *num_attrs = 0;
          EX_FUNC_LEAVE(EX_NOERR);
        }
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "Warning: failed to locate %s id %" PRId64 " in id array in file id %d",
                 ex_name_of_object(obj_type), obj_id, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_WARN);
      }
    }
  }

  switch (obj_type) {
  case EX_SIDE_SET: dnumobjatt = DIM_NUM_ATT_IN_SS(obj_id_ndx); break;
  case EX_NODE_SET: dnumobjatt = DIM_NUM_ATT_IN_NS(obj_id_ndx); break;
  case EX_EDGE_SET: dnumobjatt = DIM_NUM_ATT_IN_ES(obj_id_ndx); break;
  case EX_FACE_SET: dnumobjatt = DIM_NUM_ATT_IN_FS(obj_id_ndx); break;
  case EX_ELEM_SET: dnumobjatt = DIM_NUM_ATT_IN_ELS(obj_id_ndx); break;
  case EX_NODAL: dnumobjatt = DIM_NUM_ATT_IN_NBLK; break;
  case EX_EDGE_BLOCK: dnumobjatt = DIM_NUM_ATT_IN_EBLK(obj_id_ndx); break;
  case EX_FACE_BLOCK: dnumobjatt = DIM_NUM_ATT_IN_FBLK(obj_id_ndx); break;
  case EX_ELEM_BLOCK: dnumobjatt = DIM_NUM_ATT_IN_BLK(obj_id_ndx); break;
  default:
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: Bad block type (%d) specified for file id %d",
             obj_type, exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if ((status = nc_inq_dimid(exoid, dnumobjatt, &dimid)) != NC_NOERR) {
    /* dimension is undefined */
    *num_attrs = 0;
  }
  else {
    if ((status = nc_inq_dimlen(exoid, dimid, &lnum_attr_per_entry)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to get number of attributes in %s %" PRId64 " in file id %d",
               ex_name_of_object(obj_type), obj_id, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
    *num_attrs = lnum_attr_per_entry;
  }
  EX_FUNC_LEAVE(EX_NOERR);
}
