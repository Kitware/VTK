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
*  Id
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"

/* Used to reduce repeated code below */
#define EX_GET_DIM_VALUE(TNAME,DNAME,DIMVAR,SDIMVAL) \
  if ((status = nc_inq_dimid (exoid, DNAME, &DIMVAR)) != NC_NOERR) {    \
    /* TNAME are optional and default to zero. */ \
    SDIMVAL = 0; \
  } else { \
    if ((status = nc_inq_dimlen(exoid, DIMVAR, &lnum)) != NC_NOERR) {	\
        exerrval = status; \
        sprintf(errmsg, "Error: failed to get number of " TNAME " in file id %d", \
                exoid); \
        ex_err("ex_get_init",errmsg,exerrval); \
        return (EX_FATAL); \
      } \
    SDIMVAL = lnum; \
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

  /* Check title length to avoid overrunning clients memory space;
     include trailing null */
  if (title_len > MAX_LINE_LENGTH+1) {
    sprintf(errmsg,
            "Error: Title is too long (%d characters) in file id %d",
            (int)title_len-1, exoid);
    exerrval = -1;
    ex_err("ex_get_init",errmsg,exerrval);
    return (EX_FATAL);
  }

  if ((status = nc_get_att_text(exoid, NC_GLOBAL, ATT_TITLE, info->title)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
              "Error: failed to get title in file id %d", exoid);
      ex_err("ex_get_init",errmsg,exerrval);
      return (EX_FATAL);
    }
  info->title[title_len] = '\0';

  status = ex_get_dimension(exoid, DIM_NUM_DIM, "dimensions", &lnum, &dimid, "ex_get_init");
  if (status != NC_NOERR) return status;
  info->num_dim = lnum;


  /* Handle case with zero-nodes */
  EX_GET_DIM_VALUE(   "nodes",DIM_NUM_NODES,dimid,info->num_nodes);
  EX_GET_DIM_VALUE(   "edges", DIM_NUM_EDGE,dimid,info->num_edge);
  EX_GET_DIM_VALUE(   "faces", DIM_NUM_FACE,dimid,info->num_face);
  EX_GET_DIM_VALUE("elements", DIM_NUM_ELEM,dimid,info->num_elem);
   
  EX_GET_DIM_VALUE("element blocks", DIM_NUM_EL_BLK,dimid,info->num_elem_blk);
  if (info->num_elem_blk == 0 && info->num_elem > 0) {
    exerrval = status;
    sprintf(errmsg,
	    "Error: failed to locate number of element blocks in file id %d",
	    exoid);
    ex_err("ex_get_init",errmsg,exerrval);
    return (EX_FATAL);
  }

  EX_GET_DIM_VALUE("node sets", DIM_NUM_NS,dimid,info->num_node_sets);
  EX_GET_DIM_VALUE("edge sets", DIM_NUM_ES,dimid,info->num_edge_sets);
  EX_GET_DIM_VALUE("face sets", DIM_NUM_FS,dimid,info->num_face_sets);
  EX_GET_DIM_VALUE("side sets", DIM_NUM_SS,dimid,info->num_side_sets);
  EX_GET_DIM_VALUE("elem sets",DIM_NUM_ELS,dimid,info->num_elem_sets);

  EX_GET_DIM_VALUE("node maps", DIM_NUM_NM,dimid,info->num_node_maps);
  EX_GET_DIM_VALUE("edge maps",DIM_NUM_EDM,dimid,info->num_edge_maps);
  EX_GET_DIM_VALUE("face maps",DIM_NUM_FAM,dimid,info->num_face_maps);
  EX_GET_DIM_VALUE("elem maps", DIM_NUM_EM,dimid,info->num_elem_maps);

  /* Edge and face blocks are also optional (for backwards compatability) */
  EX_GET_DIM_VALUE("edge blocks",DIM_NUM_ED_BLK,dimid,info->num_edge_blk);
  EX_GET_DIM_VALUE("face blocks",DIM_NUM_FA_BLK,dimid,info->num_face_blk);

  return (EX_NOERR);
}
