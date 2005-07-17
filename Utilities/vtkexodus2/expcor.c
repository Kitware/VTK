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
* expcor - ex_put_coord
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
*       float*  x_coord                 X coord array
*       float*  y_coord                 y coord array
*       float*  z_coord                 z coord array
*
* exit conditions - 
*
* revision history - 
*
*  Id
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"

/*
 * writes the coordinates of all the nodes in the model
 * Only writes the 'non-null' arrays.
 */

int ex_put_coord (int   exoid,
                  const void *x_coor,
                  const void *y_coor,
                  const void *z_coor)
{
  int coordid;
  int coordidx, coordidy, coordidz;

  int numnoddim, ndimdim, i;
  long num_nod, num_dim, start[2], count[2];
  char errmsg[MAX_ERR_LENGTH];

  exerrval = 0; /* clear error code */

  /* inquire id's of previously defined dimensions  */

  if ((numnoddim = ncdimid (exoid, DIM_NUM_NODES)) == -1)
    {
      /* If not found, then this file is storing 0 nodes.
         Return immediately */
      return (EX_NOERR);
    }

  if (ncdiminq (exoid, numnoddim, NULL, &num_nod) == -1)
    {
      exerrval = ncerr;
      sprintf(errmsg,
              "Error: inquire failed to return number of nodes in file id %d",
              exoid);
      ex_err("ex_put_coord",errmsg,exerrval);
      return (EX_FATAL);
    }

  if ((ndimdim = ncdimid (exoid, DIM_NUM_DIM)) == -1)
    {
      exerrval = ncerr;
      sprintf(errmsg,
              "Error: failed to locate number of dimensions in file id %d",
              exoid);
      ex_err("ex_put_coord",errmsg,exerrval);
      return (EX_FATAL);
    }

  if (ncdiminq (exoid, ndimdim, NULL, &num_dim) == -1)
    {
      exerrval = ncerr;
      sprintf(errmsg,
              "Error: failed to get number of dimensions in file id %d",
              exoid);
      ex_err("ex_put_coord",errmsg,exerrval);
      return (EX_FATAL);
    }

  /* write out the coordinates  */
  if (ex_large_model(exoid) == 0) {
    if ((coordid = ncvarid (exoid, VAR_COORD)) == -1) {
      exerrval = ncerr;
      sprintf(errmsg,
              "Error: failed to locate nodal coordinates in file id %d", exoid);
      ex_err("ex_put_coord",errmsg,exerrval);
      return (EX_FATAL);
    } 

    for (i=0; i<num_dim; i++)
      {
        start[0] = i;
        start[1] = 0;

        count[0] = 1;
        count[1] = num_nod;

        if (i == 0 && x_coor != NULL)
          {
            if (ncvarput (exoid, coordid, start, count,
                          ex_conv_array(exoid,WRITE_CONVERT,x_coor,(int)num_nod)) == -1)
              {
                exerrval = ncerr;
                sprintf(errmsg,
                        "Error: failed to put X coord array in file id %d", exoid);
                ex_err("ex_put_coord",errmsg,exerrval);
                return (EX_FATAL);
              }
          }

        else if (i == 1 && y_coor != NULL)
          {
            if (ncvarput (exoid, coordid, start, count,
                          ex_conv_array(exoid,WRITE_CONVERT,y_coor,(int)num_nod)) == -1)
              {
                exerrval = ncerr;
                sprintf(errmsg,
                        "Error: failed to put Y coord array in file id %d", exoid);
                ex_err("ex_put_coord",errmsg,exerrval);
                return (EX_FATAL);
              }
          }

        else if (i == 2 && z_coor != NULL)
          {
            if (ncvarput (exoid, coordid, start, count,
                          ex_conv_array(exoid,WRITE_CONVERT,z_coor,(int)num_nod)) == -1)
              {
                exerrval = ncerr;
                sprintf(errmsg,
                        "Error: failed to put Z coord array in file id %d", exoid);
                ex_err("ex_put_coord",errmsg,exerrval);
                return (EX_FATAL);
              }
          }
      }
  } else {
    if ((coordidx = ncvarid (exoid, VAR_COORD_X)) == -1)
      {
        exerrval = ncerr;
        sprintf(errmsg,
                "Error: failed to locate x nodal coordinates in file id %d", exoid);
        ex_err("ex_put_coord",errmsg,exerrval);
        return (EX_FATAL);
      }

    if (num_dim > 1) {
      if ((coordidy = ncvarid (exoid, VAR_COORD_Y)) == -1)
        {
          exerrval = ncerr;
          sprintf(errmsg,
                  "Error: failed to locate y nodal coordinates in file id %d", exoid);
          ex_err("ex_put_coord",errmsg,exerrval);
          return (EX_FATAL);
        }
    } else {
      coordidy = 0;
    }
    if (num_dim > 2) {
      if ((coordidz = ncvarid (exoid, VAR_COORD_Z)) == -1)
        {
          exerrval = ncerr;
          sprintf(errmsg,
                  "Error: failed to locate z nodal coordinates in file id %d", exoid);
          ex_err("ex_put_coord",errmsg,exerrval);
          return (EX_FATAL);
        }
    } else {
      coordidz = 0;
    }
    /* write out the coordinates  */
    for (i=0; i<num_dim; i++)
      {
        const void *coor;
        char *which;
        int status;
       
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

        if (coor != NULL) {
        if (nc_flt_code(exoid) == NC_FLOAT) {
          status = nc_put_var_float(exoid, coordid, 
                                    ex_conv_array(exoid,WRITE_CONVERT,
                                                  coor,(int)num_nod));
        } else {
          status = nc_put_var_double(exoid, coordid, 
                                     ex_conv_array(exoid,WRITE_CONVERT,
                                                   coor,(int)num_nod));
        }

        if (status == -1)
          {
            exerrval = ncerr;
            sprintf(errmsg,
                    "Error: failed to put %s coord array in file id %d", which, exoid);
            ex_err("ex_put_coord",errmsg,exerrval);
            return (EX_FATAL);
          }
        }
      }
  }
  return (EX_NOERR);
}
