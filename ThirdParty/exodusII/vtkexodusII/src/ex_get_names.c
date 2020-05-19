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
 * exgnam - ex_get_names
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid          exodus file id
 *       int    obj_type,
 *
 * exit conditions -
 *       char*   names[]           ptr array of names
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for ex__get_dimension, EX_NOERR, etc

/*
 * reads the entity names from the database
 */

int ex_get_names(int exoid, ex_entity_type obj_type, char **names)
{
  int    status;
  int    varid, temp;
  size_t num_entity, i;
  char   errmsg[MAX_ERR_LENGTH];

  EX_FUNC_ENTER();
  ex__check_valid_file_id(exoid, __func__);

  /* inquire previously defined dimensions and variables  */

  switch (obj_type) {
  /*  ======== BLOCKS ========= */
  case EX_EDGE_BLOCK:
    ex__get_dimension(exoid, DIM_NUM_ED_BLK, "edge block", &num_entity, &temp, __func__);
    status = nc_inq_varid(exoid, VAR_NAME_ED_BLK, &varid);
    break;
  case EX_FACE_BLOCK:
    ex__get_dimension(exoid, DIM_NUM_FA_BLK, "face block", &num_entity, &temp, __func__);
    status = nc_inq_varid(exoid, VAR_NAME_FA_BLK, &varid);
    break;
  case EX_ELEM_BLOCK:
    ex__get_dimension(exoid, DIM_NUM_EL_BLK, "element block", &num_entity, &temp, __func__);
    status = nc_inq_varid(exoid, VAR_NAME_EL_BLK, &varid);
    break;

  /*  ======== SETS ========= */
  case EX_NODE_SET:
    ex__get_dimension(exoid, DIM_NUM_NS, "nodeset", &num_entity, &temp, __func__);
    status = nc_inq_varid(exoid, VAR_NAME_NS, &varid);
    break;
  case EX_EDGE_SET:
    ex__get_dimension(exoid, DIM_NUM_ES, "edgeset", &num_entity, &temp, __func__);
    status = nc_inq_varid(exoid, VAR_NAME_ES, &varid);
    break;
  case EX_FACE_SET:
    ex__get_dimension(exoid, DIM_NUM_FS, "faceset", &num_entity, &temp, __func__);
    status = nc_inq_varid(exoid, VAR_NAME_FS, &varid);
    break;
  case EX_SIDE_SET:
    ex__get_dimension(exoid, DIM_NUM_SS, "sideset", &num_entity, &temp, __func__);
    status = nc_inq_varid(exoid, VAR_NAME_SS, &varid);
    break;
  case EX_ELEM_SET:
    ex__get_dimension(exoid, DIM_NUM_ELS, "elemset", &num_entity, &temp, __func__);
    status = nc_inq_varid(exoid, VAR_NAME_ELS, &varid);
    break;

  /*  ======== MAPS ========= */
  case EX_NODE_MAP:
    ex__get_dimension(exoid, DIM_NUM_NM, "node map", &num_entity, &temp, __func__);
    status = nc_inq_varid(exoid, VAR_NAME_NM, &varid);
    break;
  case EX_EDGE_MAP:
    ex__get_dimension(exoid, DIM_NUM_EDM, "edge map", &num_entity, &temp, __func__);
    status = nc_inq_varid(exoid, VAR_NAME_EDM, &varid);
    break;
  case EX_FACE_MAP:
    ex__get_dimension(exoid, DIM_NUM_FAM, "face map", &num_entity, &temp, __func__);
    status = nc_inq_varid(exoid, VAR_NAME_FAM, &varid);
    break;
  case EX_ELEM_MAP:
    ex__get_dimension(exoid, DIM_NUM_EM, "element map", &num_entity, &temp, __func__);
    status = nc_inq_varid(exoid, VAR_NAME_EM, &varid);
    break;

  /* invalid variable type */
  default:
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: Invalid type specified in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (status == NC_NOERR) {
    if ((status = ex__get_names(exoid, varid, num_entity, names, obj_type, "ex_get_names")) !=
        EX_NOERR) {
      EX_FUNC_LEAVE(status);
    }
  }
  else {
    /* Names variable does not exist on the database; probably since this is an
     * older version of the database.  Return an empty array...
     */
    for (i = 0; i < num_entity; i++) {
      names[i][0] = '\0';
    }
  }
  EX_FUNC_LEAVE(EX_NOERR);
}
