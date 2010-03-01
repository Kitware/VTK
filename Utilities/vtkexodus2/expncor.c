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
* expcor - ex_put_n_coord
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*       int     start_node_num          starting index (1-based) of coordinates to be written.
*       int     num_nodes               number of nodes to write coordinates for.
*       float*  x_coord                 X coord array
*       float*  y_coord                 y coord array
*       float*  z_coord                 z coord array
*
* exit conditions - 
*
* revision history - 
*
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"

/*!
 * writes the coordinates of some of the nodes in the model
 * Only writes the 'non-null' arrays.
 * \param   exoid           exodus file id
 * \param   start_node_num  the starting index (1-based) of the coordinates to be written
 * \param   num_nodes       the number of nodes to write coordinates for.
 * \param   x_coor          x coord array
 * \param   y_coor          y coord array
 * \param   z_coor          z coord array
 */

int ex_put_n_coord (int   exoid,
                    int start_node_num,
                    int num_nodes,
                    const void *x_coor,
                    const void *y_coor,
                    const void *z_coor)
{
  int status;
  int coordid;
  int coordidx, coordidy, coordidz;

  int numnoddim, ndimdim;
  size_t i, num_nod, num_dim, start[2], count[2];
  char errmsg[MAX_ERR_LENGTH];

  exerrval = 0; /* clear error code */

  /* inquire id's of previously defined dimensions  */

  if ((status = nc_inq_dimid(exoid, DIM_NUM_NODES, &numnoddim)) != NC_NOERR) {
    /* If not found, then this file is storing 0 nodes.
       Return immediately */
    return (EX_NOERR);
  }

  if ((status = nc_inq_dimlen(exoid, numnoddim, &num_nod)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
            "Error: inquire failed to return number of nodes in file id %d",
            exoid);
    ex_err("ex_put_n_coord",errmsg,exerrval);
    return (EX_FATAL);
  }

  if ((status = nc_inq_dimid(exoid, DIM_NUM_DIM, &ndimdim)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
            "Error: failed to locate number of dimensions in file id %d",
            exoid);
    ex_err("ex_put_n_coord",errmsg,exerrval);
    return (EX_FATAL);
  }

  if ((status = nc_inq_dimlen(exoid, ndimdim, &num_dim)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
            "Error: failed to get number of dimensions in file id %d",
            exoid);
    ex_err("ex_put_n_coord",errmsg,exerrval);
    return (EX_FATAL);
  }

  if (start_node_num + num_nodes -1 > (int)num_nod) {
    exerrval = EX_BADPARAM;
      sprintf(errmsg,
              "Error: start index (%d) + node count (%d) is larger than total number of nodes (%d) in file id %d",
              start_node_num, num_nodes, (int)num_nod, exoid);
      ex_err("ex_put_n_coord",errmsg,exerrval);
      return (EX_FATAL);
  }

  --start_node_num;

  /* write out the coordinates  */
  if (ex_large_model(exoid) == 0) {
    if ((status = nc_inq_varid(exoid, VAR_COORD, &coordid)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
              "Error: failed to locate nodal coordinates in file id %d", exoid);
      ex_err("ex_put_n_coord",errmsg,exerrval);
      return (EX_FATAL);
    } 

    for (i=0; i<num_dim; i++) {
      char *which = 0;
      start[0] = i;
      start[1] = start_node_num;

      count[0] = 1;
      count[1] = num_nodes;

      if (i == 0 && x_coor != NULL) {
        which = "X";
        if (ex_comp_ws(exoid) == 4) {
          status = nc_put_vara_float(exoid, coordid, start, count, x_coor);
        } else {
          status = nc_put_vara_double(exoid, coordid, start, count, x_coor);
        }
      }   
      else if (i == 1 && y_coor != NULL) {
        which = "Y";
        if (ex_comp_ws(exoid) == 4) {
          status = nc_put_vara_float(exoid, coordid, start, count, y_coor);
        } else {
          status = nc_put_vara_double(exoid, coordid, start, count, y_coor);
        }
      }
      else if (i == 2 && z_coor != NULL) {
        which = "Z";
        if (ex_comp_ws(exoid) == 4) {
          status = nc_put_vara_float(exoid, coordid, start, count, z_coor);
        } else {
          status = nc_put_vara_double(exoid, coordid, start, count, z_coor);
        }
      }
          
      if (status != NC_NOERR) {
        exerrval = status;
        sprintf(errmsg,
                "Error: failed to put %s coord array in file id %d", which, exoid);
        ex_err("ex_put_n_coord",errmsg,exerrval);
        return (EX_FATAL);
      }
    }
  } else {
    if ((status = nc_inq_varid(exoid, VAR_COORD_X, &coordidx)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
              "Error: failed to locate x nodal coordinates in file id %d", exoid);
      ex_err("ex_put_n_coord",errmsg,exerrval);
      return (EX_FATAL);
    }

    if (num_dim > 1) {
      if ((status = nc_inq_varid(exoid, VAR_COORD_Y, &coordidy)) != NC_NOERR) {
        exerrval = status;
        sprintf(errmsg,
                "Error: failed to locate y nodal coordinates in file id %d", exoid);
        ex_err("ex_put_n_coord",errmsg,exerrval);
        return (EX_FATAL);
      }
    } else {
      coordidy = 0;
    }
    if (num_dim > 2) {
      if ((status = nc_inq_varid(exoid, VAR_COORD_Z, &coordidz)) != NC_NOERR) {
        exerrval = status;
        sprintf(errmsg,
                "Error: failed to locate z nodal coordinates in file id %d", exoid);
        ex_err("ex_put_n_coord",errmsg,exerrval);
        return (EX_FATAL);
      }
    } else {
      coordidz = 0;
    }

    /* write out the coordinates  */
    for (i=0; i<num_dim; i++) {
      const void *coor;
      char *which;
       
      start[0] = start_node_num;
      count[0] = num_nodes;

      if (i == 0) {
        coor = x_coor;
        which = "X";
        coordid = coordidx;
      } else if (i == 1) {
        coor = y_coor;
        which = "Y";
        coordid = coordidy;
      } else if (i == 2) {
        coor = z_coor;
        which = "Z";
        coordid = coordidz;
      }

      if (coor != NULL && coordid != 0) {
        if (ex_comp_ws(exoid) == 4) {
          status = nc_put_vara_float(exoid, coordid, start, count, coor);
        } else {
          status = nc_put_vara_double(exoid, coordid, start, count, coor);
        }

        if (status != NC_NOERR) {
          exerrval = status;
          sprintf(errmsg,
                  "Error: failed to put %s coord array in file id %d", which, exoid);
          ex_err("ex_put_n_coord",errmsg,exerrval);
          return (EX_FATAL);
        }
      }
    }
  }
  return (EX_NOERR);
}
