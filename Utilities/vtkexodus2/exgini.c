/*
 * Copyright (c) 1994 Sandia Corporation. Under the terms of Contract
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
*
*          
* environment - UNIX
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*
* exit conditions - 
*       char*   title                   title of file
*       int*    num_dim                 number of dimensions (per node)
*       int*    num_nodes               number of nodes
*       int*    num_elem                number of elements
*       int*    num_elem_blk            number of element blocks
*       int*    num_node_sets           number of node sets
*       int*    num_side_sets           numver of side sets
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

int ex_get_init (int   exoid,
                 char *title,
                 int  *num_dim,
                 int  *num_nodes,
                 int  *num_elem, 
                 int  *num_elem_blk,
                 int  *num_node_sets,
                 int  *num_side_sets)
{
  int dimid;
  long lnum_dim, lnum_nodes, lnum_elem, lnum_elem_blk, lnum_node_sets; 
  long lnum_side_sets;
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

  if (ncattget (exoid, NC_GLOBAL, ATT_TITLE, title) == -1)
    {
      exerrval = ncerr;
      sprintf(errmsg,
              "Error: failed to get title in file id %d", exoid);
      ex_err("ex_get_init",errmsg,exerrval);
      return (EX_FATAL);
    }

    
  /* printf("[ex_get_init] title: %s\n",title); */


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
  *num_dim = lnum_dim;


  /* Handle case with zero-nodes */
  if ((dimid = ncdimid (exoid, DIM_NUM_NODES)) == -1) {
    *num_nodes = 0;
  } else {
     
    if (ncdiminq (exoid, dimid, (char *) 0, &lnum_nodes) == -1)
      {
        exerrval = ncerr;
        sprintf(errmsg,
                "Error: failed to get number of nodes in file id %d",
                exoid);
        ex_err("ex_get_init",errmsg,exerrval);
        return (EX_FATAL);
      }
    *num_nodes = lnum_nodes;
  }
   
  if ((dimid = ncdimid (exoid, DIM_NUM_ELEM)) == -1) {
    *num_elem = 0;
  } else {
    if (ncdiminq (exoid, dimid, (char *) 0, &lnum_elem) == -1)
      {
        exerrval = ncerr;
        sprintf(errmsg,
                "Error: failed to get number of elements in file id %d",
                exoid);
        ex_err("ex_get_init",errmsg,exerrval);
        return (EX_FATAL);
      }
    *num_elem = lnum_elem;
  }


  if (*num_elem > 0) {
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
    *num_elem_blk = lnum_elem_blk;
  } else {
    *num_elem_blk = 0;
  }


  /* node sets are optional */
  if ((dimid = ncdimid (exoid, DIM_NUM_NS)) == -1)
    *num_node_sets = 0;
  else
    {
      if (ncdiminq (exoid, dimid, (char *) 0, &lnum_node_sets) == -1)
        {
          exerrval = ncerr;
          sprintf(errmsg,
                  "Error: failed to get number of node sets in file id %d",
                  exoid);
          ex_err("ex_get_init",errmsg,exerrval);
          return (EX_FATAL);
        }
      *num_node_sets = lnum_node_sets;
    }

  /* side sets are optional */
  if ((dimid = ncdimid (exoid, DIM_NUM_SS))  == -1)
    *num_side_sets = 0;
  else
    {
      if (ncdiminq (exoid, dimid, (char *) 0, &lnum_side_sets) == -1)
        {
          exerrval = ncerr;
          sprintf(errmsg,
                  "Error: failed to get number of side sets in file id %d",
                  exoid);
          ex_err("ex_get_init",errmsg,exerrval);
          return (EX_FATAL);
        }
      *num_side_sets = lnum_side_sets;
    }

  return (EX_NOERR);
}
