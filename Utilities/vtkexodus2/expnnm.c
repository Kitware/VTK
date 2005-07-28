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
* expnnm - ex_put_node_num_map
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
*       int*    node_map                node numbering map
*
* exit conditions - 
*
* revision history - 
*
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"

#include <stdlib.h>

/*
 * writes out the node numbering map to the database; allows node numbers
 * to be non-contiguous
 */

int ex_put_node_num_map (int  exoid,
                         const int *node_map)
{
  int numnodedim, dims[1], mapid, iresult;
  long num_nodes, start[1], count[1]; 
  nclong *lptr;
  char errmsg[MAX_ERR_LENGTH];

  exerrval = 0; /* clear error code */

  /* inquire id's of previously defined dimensions  */

  /* determine number of nodes, return immediately if 0 */
  if ((numnodedim = ncdimid (exoid, DIM_NUM_NODES)) == -1)
    {
      return (EX_NOERR);
    }
  
  if (ncdiminq (exoid, numnodedim, (char *) 0, &num_nodes) == -1)
    {
      exerrval = ncerr;
      sprintf(errmsg,
              "Error: failed to get number of nodes in file id %d",
              exoid);
      ex_err("ex_put_node_num_map",errmsg,exerrval);
      return (EX_FATAL);
    }


  /* put netcdf file into define mode  */

  if ((mapid = ncvarid (exoid, VAR_NODE_NUM_MAP)) == -1) {
    if (ncredef (exoid) == -1)
      {
        exerrval = ncerr;
        sprintf(errmsg,
                "Error: failed to put file id %d into define mode",
                exoid);
        ex_err("ex_put_node_num_map",errmsg,exerrval);
        return (EX_FATAL);
      }


    /* create a variable array in which to store the node numbering map  */

    dims[0] = numnodedim;

    if ((mapid = ncvardef (exoid, VAR_NODE_NUM_MAP, NC_LONG, 1, dims)) == -1)
      {
        if (ncerr == NC_ENAMEINUSE)
          {
            exerrval = ncerr;
            sprintf(errmsg,
                    "Error: node numbering map already exists in file id %d",
                    exoid);
            ex_err("ex_put_node_num_map",errmsg,exerrval);
          }
        else
          {
            exerrval = ncerr;
            sprintf(errmsg,
                    "Error: failed to create node numbering map array in file id %d",
                    exoid);
            ex_err("ex_put_node_num_map",errmsg,exerrval);
          }
        goto error_ret;         /* exit define mode and return */
      }


    /* leave define mode  */

    if (ncendef (exoid) == -1)
      {
        exerrval = ncerr;
        sprintf(errmsg,
                "Error: failed to complete definition in file id %d",
                exoid);
        ex_err("ex_put_node_num_map",errmsg,exerrval);
        return (EX_FATAL);
      }
  }

  /* write out the node numbering map  */

  /* this contortion is necessary because netCDF is expecting nclongs; fortunately
     it's necessary only when ints and nclongs aren't the same size */

  start[0] = 0;
  count[0] = num_nodes;

  if (sizeof(int) == sizeof(nclong)) {
    iresult = ncvarput (exoid, mapid, start, count, node_map);
  } else {
    lptr = itol (node_map, (int)num_nodes);
    iresult = ncvarput (exoid, mapid, start, count, lptr);
    free(lptr);
  }

  if (iresult == -1)
    {
      exerrval = ncerr;
      sprintf(errmsg,
              "Error: failed to store node numbering map in file id %d",
              exoid);
      ex_err("ex_put_node_num_map",errmsg,exerrval);
      return (EX_FATAL);
    }


  return (EX_NOERR);

  /* Fatal error: exit definition mode and return */
 error_ret:
  if (ncendef (exoid) == -1)     /* exit define mode */
    {
      sprintf(errmsg,
              "Error: failed to complete definition for file id %d",
              exoid);
      ex_err("ex_put_node_num_map",errmsg,exerrval);
    }
  return (EX_FATAL);
}

