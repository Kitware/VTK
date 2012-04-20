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

#include "exodusII.h"
#include "exodusII_int.h"
#include <stdlib.h>
#include <string.h>

/* Used to reduce repeated code below */
static int ex_get_dim_value(int exoid, const char *name, const char *dimension_name,
                            int dimension, int *value)
{
  char errmsg[MAX_ERR_LENGTH];
  int status;
  size_t lnum;

  if ((status = nc_inq_dimid (exoid, dimension_name, &dimension)) != NC_NOERR) {
    /* optional and default to zero. */
    *value = 0;
  } else {
    if ((status = nc_inq_dimlen(exoid, dimension, &lnum)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg, "Error: failed to get number of %s in file id %d",
        name, exoid);
      ex_err("ex_get_init",errmsg,exerrval);
      return (EX_FATAL);
    }
    *value = lnum;
  }
  return EX_NOERR;
}

/*!
 * reads the initialization parameters from an opened EXODUS II file
 * \param exoid exodus file id
 * \param[out] info #ex_init_params structure containing metadata for mesh.
 * \sa ex_get_init()
 */

int ex_get_init_ext (int   exoid,
                     ex_init_params *info)
{
  int dimid;
  size_t lnum;
  char errmsg[MAX_ERR_LENGTH];
  int status;
  size_t title_len;
  nc_type title_type;

  exerrval = 0; /* clear error code */

  if ((status = nc_inq_att(exoid, NC_GLOBAL, ATT_TITLE, &title_type, &title_len)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
      "Error: failed to inquire title in file id %d", exoid);
    ex_err("ex_get_init",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* Check title length to avoid overrunning clients memory space; include trailing null */
  if (title_len > MAX_LINE_LENGTH+1) {
    char *title = malloc(title_len+1);
    if ((status = nc_get_att_text(exoid, NC_GLOBAL, ATT_TITLE, title)) == NC_NOERR) {
      strncpy(info->title, title, MAX_LINE_LENGTH+1);
      info->title[MAX_LINE_LENGTH] = '\0';
    }
    free(title);
  } else {
    status = nc_get_att_text(exoid, NC_GLOBAL, ATT_TITLE, info->title);
    info->title[title_len] = '\0';
  }
  if (status != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
            "Error: failed to get title in file id %d", exoid);
    ex_err("ex_get_init",errmsg,exerrval);
    return (EX_FATAL);
  }

  status = ex_get_dimension(exoid, DIM_NUM_DIM, "dimensions", &lnum, &dimid, "ex_get_init");
  if (status != NC_NOERR) return status;
  info->num_dim = lnum;


  /* Handle case with zero-nodes */
  if (ex_get_dim_value(exoid,   "nodes",DIM_NUM_NODES,dimid,&info->num_nodes) != EX_NOERR) return EX_FATAL;
  if (ex_get_dim_value(exoid,   "edges", DIM_NUM_EDGE,dimid,&info->num_edge) != EX_NOERR) return EX_FATAL;
  if (ex_get_dim_value(exoid,   "faces", DIM_NUM_FACE,dimid,&info->num_face) != EX_NOERR) return EX_FATAL;
  if (ex_get_dim_value(exoid,"elements", DIM_NUM_ELEM,dimid,&info->num_elem) != EX_NOERR) return EX_FATAL;

  if (ex_get_dim_value(exoid,"element blocks", DIM_NUM_EL_BLK,dimid,&info->num_elem_blk) != EX_NOERR) return EX_FATAL;
  if (info->num_elem_blk == 0 && info->num_elem > 0) {
    exerrval = status;
    sprintf(errmsg,
            "Error: failed to locate number of element blocks in file id %d",
            exoid);
    ex_err("ex_get_init",errmsg,exerrval);
    return (EX_FATAL);
  }

  if (ex_get_dim_value(exoid,"node sets", DIM_NUM_NS,dimid,&info->num_node_sets) != EX_NOERR) return EX_FATAL;
  if (ex_get_dim_value(exoid,"edge sets", DIM_NUM_ES,dimid,&info->num_edge_sets) != EX_NOERR) return EX_FATAL;
  if (ex_get_dim_value(exoid,"face sets", DIM_NUM_FS,dimid,&info->num_face_sets) != EX_NOERR) return EX_FATAL;
  if (ex_get_dim_value(exoid,"side sets", DIM_NUM_SS,dimid,&info->num_side_sets) != EX_NOERR) return EX_FATAL;
  if (ex_get_dim_value(exoid,"elem sets",DIM_NUM_ELS,dimid,&info->num_elem_sets) != EX_NOERR) return EX_FATAL;

  if (ex_get_dim_value(exoid,"node maps", DIM_NUM_NM,dimid,&info->num_node_maps) != EX_NOERR) return EX_FATAL;
  if (ex_get_dim_value(exoid,"edge maps",DIM_NUM_EDM,dimid,&info->num_edge_maps) != EX_NOERR) return EX_FATAL;
  if (ex_get_dim_value(exoid,"face maps",DIM_NUM_FAM,dimid,&info->num_face_maps) != EX_NOERR) return EX_FATAL;
  if (ex_get_dim_value(exoid,"elem maps", DIM_NUM_EM,dimid,&info->num_elem_maps) != EX_NOERR) return EX_FATAL;

  /* Edge and face blocks are also optional (for backwards compatability) */
  if (ex_get_dim_value(exoid,"edge blocks",DIM_NUM_ED_BLK,dimid,&info->num_edge_blk) != EX_NOERR) return EX_FATAL;
  if (ex_get_dim_value(exoid,"face blocks",DIM_NUM_FA_BLK,dimid,&info->num_face_blk) != EX_NOERR) return EX_FATAL;

  return (EX_NOERR);
}
