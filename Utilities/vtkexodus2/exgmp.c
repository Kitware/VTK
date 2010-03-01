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
* exgmp - ex_get_map_param
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*
* exit conditions - 
*       int*    num_node_maps           number of node maps
*       int*    num_elem_maps           number of element maps
*
* revision history - 
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"

/*
 * reads the number of node and element maps
 */

int ex_get_map_param (int   exoid,
                      int  *num_node_maps,
                      int  *num_elem_maps)
{
  int status;
  int dimid;
  size_t lnum_node_maps, lnum_elem_maps;
  char errmsg[MAX_ERR_LENGTH];

  exerrval = 0; /* clear error code */


  /* node maps are optional */
  if (nc_inq_dimid(exoid, DIM_NUM_NM, &dimid) != NC_NOERR) {
    *num_node_maps = 0;
  } else {
    if ((status = nc_inq_dimlen(exoid, dimid, &lnum_node_maps)) != NC_NOERR) {
          exerrval = status;
          sprintf(errmsg,
                  "Error: failed to get number of node maps in file id %d",
                  exoid);
          ex_err("ex_get_map_param",errmsg,exerrval);
          return (EX_FATAL);
        }
        *num_node_maps = lnum_node_maps;
        }

    /* element maps are optional */
    if (nc_inq_dimid(exoid, DIM_NUM_EM, &dimid) != NC_NOERR) {
    *num_elem_maps = 0;
  } else {
    if ((status = nc_inq_dimlen(exoid, dimid, &lnum_elem_maps)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
              "Error: failed to get number of element maps in file id %d",
              exoid);
      ex_err("ex_get_map_param",errmsg,exerrval);
      return (EX_FATAL);
    }
    *num_elem_maps = lnum_elem_maps;
  }
  return (EX_NOERR);
}
