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
 * exgini - ex_get_init
 *
 * entry conditions -
 *   input parameters:
 *       int             exoid           exodus file id
 *
 * exit conditions -
 *       ex_init_params* info            parameter information
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h"     // for ex_init_params, etc
#include "exodusII_int.h" // for EX_FATAL, EX_NOERR, etc

/* Used to reduce repeated code below */
static int64_t ex_get_dim_value(int exoid, const char *name, const char *dimension_name,
                                int dimension, int64_t *value)
{
  char errmsg[MAX_ERR_LENGTH];
  int  status;

  if ((status = nc_inq_dimid(exoid, dimension_name, &dimension)) != NC_NOERR) {
    /* optional and default to zero. */
    *value = 0;
  }
  else {
    size_t tmp;
    if ((status = nc_inq_dimlen(exoid, dimension, &tmp)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get number of %s in file id %d", name,
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      return (EX_FATAL);
    }
    *value = tmp;
  }
  return (EX_NOERR);
}

/*!
 * \ingroup ModelDescription
 * reads the initialization parameters from an opened EXODUS file
 * \param exoid exodus file id
 * \param[out] info #ex_init_params structure containing metadata for mesh.
 * \sa ex_get_init()
 */

int ex_get_init_ext(int exoid, ex_init_params *info)
{
  int     dimid = 0;
  char    errmsg[MAX_ERR_LENGTH];
  int     status     = 0;
  size_t  title_len  = 0;
  nc_type title_type = 0;

  int rootid = exoid & EX_FILE_ID_MASK;

  EX_FUNC_ENTER();
  ex__check_valid_file_id(exoid, __func__);

  info->num_dim       = 0;
  info->num_nodes     = 0;
  info->num_edge      = 0;
  info->num_edge_blk  = 0;
  info->num_face      = 0;
  info->num_face_blk  = 0;
  info->num_elem      = 0;
  info->num_elem_blk  = 0;
  info->num_node_sets = 0;
  info->num_edge_sets = 0;
  info->num_face_sets = 0;
  info->num_side_sets = 0;
  info->num_elem_sets = 0;
  info->num_node_maps = 0;
  info->num_edge_maps = 0;
  info->num_face_maps = 0;
  info->num_elem_maps = 0;

  dimid = 0;
  if (ex_get_dim_value(exoid, "dimension count", DIM_NUM_DIM, dimid, &info->num_dim) != EX_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }
  if (ex_get_dim_value(exoid, "nodes", DIM_NUM_NODES, dimid, &info->num_nodes) != EX_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }
  if (ex_get_dim_value(exoid, "edges", DIM_NUM_EDGE, dimid, &info->num_edge) != EX_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }
  if (ex_get_dim_value(exoid, "faces", DIM_NUM_FACE, dimid, &info->num_face) != EX_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }
  if (ex_get_dim_value(exoid, "elements", DIM_NUM_ELEM, dimid, &info->num_elem) != EX_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (ex_get_dim_value(exoid, "element blocks", DIM_NUM_EL_BLK, dimid, &info->num_elem_blk) !=
      EX_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }
  if (info->num_elem_blk == 0 && info->num_elem > 0) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to locate number of element blocks in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (ex_get_dim_value(exoid, "node sets", DIM_NUM_NS, dimid, &info->num_node_sets) != EX_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }
  if (ex_get_dim_value(exoid, "edge sets", DIM_NUM_ES, dimid, &info->num_edge_sets) != EX_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }
  if (ex_get_dim_value(exoid, "face sets", DIM_NUM_FS, dimid, &info->num_face_sets) != EX_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }
  if (ex_get_dim_value(exoid, "side sets", DIM_NUM_SS, dimid, &info->num_side_sets) != EX_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }
  if (ex_get_dim_value(exoid, "elem sets", DIM_NUM_ELS, dimid, &info->num_elem_sets) != EX_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (ex_get_dim_value(exoid, "node maps", DIM_NUM_NM, dimid, &info->num_node_maps) != EX_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }
  if (ex_get_dim_value(exoid, "edge maps", DIM_NUM_EDM, dimid, &info->num_edge_maps) != EX_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }
  if (ex_get_dim_value(exoid, "face maps", DIM_NUM_FAM, dimid, &info->num_face_maps) != EX_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }
  if (ex_get_dim_value(exoid, "elem maps", DIM_NUM_EM, dimid, &info->num_elem_maps) != EX_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Edge and face blocks are also optional (for backwards compatibility) */
  if (ex_get_dim_value(exoid, "edge blocks", DIM_NUM_ED_BLK, dimid, &info->num_edge_blk) !=
      EX_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }
  if (ex_get_dim_value(exoid, "face blocks", DIM_NUM_FA_BLK, dimid, &info->num_face_blk) !=
      EX_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if ((status = nc_inq_att(rootid, NC_GLOBAL, ATT_TITLE, &title_type, &title_len)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "Warning: no title in file id %d", rootid);
    ex_err_fn(exoid, __func__, errmsg, status);
  }

  /* Check title length to avoid overrunning clients memory space; include
   * trailing null */
  if (title_len > 0) {
    if (title_len > MAX_LINE_LENGTH) {
      char *title = malloc(title_len + 1);
      if ((status = nc_get_att_text(rootid, NC_GLOBAL, ATT_TITLE, title)) == NC_NOERR) {
        ex_copy_string(info->title, title, MAX_LINE_LENGTH + 1);
        info->title[MAX_LINE_LENGTH] = '\0';
      }
      free(title);
    }
    else {
      status                 = nc_get_att_text(rootid, NC_GLOBAL, ATT_TITLE, info->title);
      info->title[title_len] = '\0';
    }
    if (status != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get title in file id %d", rootid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }
  else {
    info->title[0] = '\0';
  }

  /* Update settings in ex__file_item struct */
  {
    struct ex__file_item *file = ex__find_file_item(exoid);
    if (file) {
      file->has_nodes = info->num_nodes > 0;
      file->has_edges = info->num_edge > 0;
      file->has_faces = info->num_face > 0;
      file->has_elems = info->num_elem > 0;
    }
  }
  EX_FUNC_LEAVE(EX_NOERR);
}
