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
* author - Sandia National Laboratories
*          Larry A. Schoof - Original
*          James A. Schutt - 8 byte float and standard C definitions
*          Vic Yarberry    - Added headers and error logging
*          David Thompson  - Added edge/face blocks/sets
*
*          
* environment - UNIX
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

/*
 * reads the initialization parameters from an opened EXODUS II file
 */

int ex_get_init_ext (int   exoid,
                     ex_init_params *info)
{
  int dimid;
  long lnum_dim, lnum_nodes, lnum_elem, lnum_elem_blk, lnum_node_sets; 
  long lnum_side_sets, lnum_edge_sets, lnum_face_sets, lnum_elem_sets;
  long lnum_node_maps, lnum_edge_maps, lnum_face_maps, lnum_elem_maps;
  long lnum_edge, lnum_face, lnum_edge_blk, lnum_face_blk;
  char errmsg[MAX_ERR_LENGTH];
  int title_len;
  nc_type title_type;

  exerrval = 0; /* clear error code */

  if (ncattinq (exoid, NC_GLOBAL, ATT_TITLE, &title_type, &title_len) == -1)
    {
      exerrval = ncerr;
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
            title_len-1, exoid);
    exerrval = -1;
    ex_err("ex_get_init",errmsg,exerrval);
    return (EX_FATAL);
  }
  /* printf("[ex_get_init] title length: %d\n",title_len); */

  if (ncattget (exoid, NC_GLOBAL, ATT_TITLE, info->title) == -1)
    {
      exerrval = ncerr;
      sprintf(errmsg,
              "Error: failed to get title in file id %d", exoid);
      ex_err("ex_get_init",errmsg,exerrval);
      return (EX_FATAL);
    }

    
  /* printf("[ex_get_init] title: %s\n",info->title); */


  if ((dimid = ncdimid (exoid, DIM_NUM_DIM)) == -1)
    {
      exerrval = ncerr;
      sprintf(errmsg,
              "Error: failed to locate number of dimensions in file id %d",
              exoid);
      ex_err("ex_get_init",errmsg,exerrval);
      return (EX_FATAL);
    }

  if (ncdiminq (exoid, dimid, (char *) 0, &lnum_dim) == -1)
    {
      exerrval = ncerr;
      sprintf(errmsg,
              "Error: failed to get number of dimensions in file id %d",
              exoid);
      ex_err("ex_get_init",errmsg,exerrval);
      return (EX_FATAL);
    }
  info->num_dim = lnum_dim;


  /* Handle case with zero-nodes */
#define EX_GET_DIM_VALUE(TNAME,DNAME,DIMVAR,LDIMVAL,SDIMVAL) \
  if ((DIMVAR = ncdimid (exoid, DNAME)) == -1) { \
    /* TNAME are optional and default to zero. */ \
    SDIMVAL = 0; \
  } else { \
      \
    if (ncdiminq (exoid, DIMVAR, (char *) 0, &LDIMVAL) == -1) \
      { \
        exerrval = ncerr; \
        sprintf(errmsg, \
                "Error: failed to get number of " TNAME " in file id %d", \
                exoid); \
        ex_err("ex_get_init",errmsg,exerrval); \
        return (EX_FATAL); \
      } \
    SDIMVAL = LDIMVAL; \
  }
  EX_GET_DIM_VALUE(   "nodes",DIM_NUM_NODES,dimid,lnum_nodes,info->num_nodes);
  EX_GET_DIM_VALUE(   "edges", DIM_NUM_EDGE,dimid, lnum_edge, info->num_edge);
  EX_GET_DIM_VALUE(   "faces", DIM_NUM_FACE,dimid, lnum_face, info->num_face);
  EX_GET_DIM_VALUE("elements", DIM_NUM_ELEM,dimid, lnum_elem, info->num_elem);
   
  if (info->num_elem > 0) {
    if ((dimid = ncdimid (exoid, DIM_NUM_EL_BLK)) == -1)
      {
        exerrval = ncerr;
        sprintf(errmsg,
                "Error: failed to locate number of element blocks in file id %d",
                exoid);
        ex_err("ex_get_init",errmsg,exerrval);
        return (EX_FATAL);
      }

    if (ncdiminq (exoid, dimid, (char *) 0, &lnum_elem_blk) == -1)
      {
        exerrval = ncerr;
        sprintf(errmsg,
                "Error: failed to get number of element blocks in file id %d",
                exoid);
        ex_err("ex_get_init",errmsg,exerrval);
        return (EX_FATAL);
      }
    info->num_elem_blk = lnum_elem_blk;
  } else {
    info->num_elem_blk = 0;
  }


  EX_GET_DIM_VALUE("node sets", DIM_NUM_NS,dimid,lnum_node_sets,info->num_node_sets);
  EX_GET_DIM_VALUE("edge sets", DIM_NUM_ES,dimid,lnum_edge_sets,info->num_edge_sets);
  EX_GET_DIM_VALUE("face sets", DIM_NUM_FS,dimid,lnum_face_sets,info->num_face_sets);
  EX_GET_DIM_VALUE("side sets", DIM_NUM_SS,dimid,lnum_side_sets,info->num_side_sets);
  EX_GET_DIM_VALUE("elem sets",DIM_NUM_ELS,dimid,lnum_elem_sets,info->num_elem_sets);

  EX_GET_DIM_VALUE("node maps", DIM_NUM_NM,dimid,lnum_node_maps,info->num_node_maps);
  EX_GET_DIM_VALUE("edge maps",DIM_NUM_EDM,dimid,lnum_edge_maps,info->num_edge_maps);
  EX_GET_DIM_VALUE("face maps",DIM_NUM_FAM,dimid,lnum_face_maps,info->num_face_maps);
  EX_GET_DIM_VALUE("elem maps", DIM_NUM_EM,dimid,lnum_elem_maps,info->num_elem_maps);

  /* Edge and face blocks are also optional (for backwards compatability) */
  EX_GET_DIM_VALUE("edge blocks",DIM_NUM_ED_BLK,dimid,lnum_edge_blk,info->num_edge_blk);
  EX_GET_DIM_VALUE("face blocks",DIM_NUM_FA_BLK,dimid,lnum_face_blk,info->num_face_blk);

  return (EX_NOERR);
}
