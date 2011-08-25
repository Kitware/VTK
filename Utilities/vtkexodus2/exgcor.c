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

#include "exodusII.h"
#include "exodusII_int.h"

/*!
The function ex_get_coord() reads the nodal coordinates of the
nodes. Memory must be allocated for the coordinate arrays (\c x_coor,
\c y_coor, and \c z_coor) before this call is made. The length of each
of these arrays is the number of nodes in the mesh.

Because the coordinates are floating point values, the application
code must declare the arrays passed to be the appropriate type
(\c float or \c double) to match the compute word size passed in
ex_create() or ex_open().

\return
In case of an error, ex_get_coord() returns a negative number;
a warning will return a positive number. Possible causes of errors
include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  a warning value is returned if nodal coordinates were not stored.

\param[in]   exoid    exodus file ID returned from a previous call to ex_create() or ex_open().
\param[out]  x_coor   Returned X coordinates of the nodes. If this is \c NULL, the
                      X-coordinates will not be read.
\param[out]  y_coor   Returned Y coordinates of the nodes. These are returned only if
                      \c num_dim > 1; otherwise, pass in \c NULL. If this
          is \c NULL, the Y-coordinates will not be read.
\param[out]  z_coor   Returned Z coordinates of the nodes. These are returned only if
                      \c num_dim > 2; otherwise, pass in \c NULL. If this
                      is \c NULL, the Z-coordinates will not be read.

The following code segment will read the nodal coordinates 
from an open exodus file :

\code
int error, exoid;

double *x, *y, *z;

\comment{read nodal coordinates values from database}
x = (double *)calloc(num_nodes, sizeof(double));
y = (double *)calloc(num_nodes, sizeof(double));
if (num_dim >= 3)
   z = (double *)calloc(num_nodes, sizeof(double));
else
   z = 0;

error = ex_get_coord(exoid, x, y, z);

\comment{Do the same as the previous call in three separate calls}
error = ex_get_coord(exoid, x,    NULL, NULL);
error = ex_get_coord(exoid, NULL, y,    NULL);
if (num_dim >= 3)
   error = ex_get_coord(exoid, NULL, NULL, z);
\endcode

*/

int ex_get_coord (int exoid,
                  void *x_coor,
                  void *y_coor,
                  void *z_coor)
{
  int status;
  int coordid;
  int coordidx, coordidy, coordidz;

  int numnoddim, ndimdim;
  size_t num_nod, num_dim, start[2], count[2], i;
  char errmsg[MAX_ERR_LENGTH];

  exerrval = 0;

  /* inquire id's of previously defined dimensions  */

  if (ex_get_dimension(exoid, DIM_NUM_DIM, "dimensions",
           &num_dim, &ndimdim, "ex_get_coord") != NC_NOERR) {
    return(EX_FATAL);
  }
      
  if (nc_inq_dimid (exoid, DIM_NUM_NODES, &numnoddim) != NC_NOERR)
    {
      /* If not found, then this file is storing 0 nodes.
         Return immediately */
      return (EX_NOERR);
    }

  if ((status = nc_inq_dimlen(exoid, numnoddim, &num_nod)) != NC_NOERR)
    {
      exerrval = status;
      sprintf(errmsg,
              "Error: failed to get number of nodes in file id %d",
              exoid);
      ex_err("ex_get_coord",errmsg,exerrval);
      return (EX_FATAL);
    }

  /* read in the coordinates  */
  if (ex_large_model(exoid) == 0) {
    if ((status = nc_inq_varid (exoid, VAR_COORD, &coordid)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
              "Error: failed to locate nodal coordinates in file id %d", exoid);
      ex_err("ex_get_coord",errmsg,exerrval);
      return (EX_FATAL);
    } 

    for (i=0; i<num_dim; i++) {
      char *which;
      start[0] = i;
      start[1] = 0;

      count[0] = 1;
      count[1] = num_nod;

      if (i == 0 && x_coor != NULL) {
  which = "X";
  if (ex_comp_ws(exoid) == 4) {
    status = nc_get_vara_float(exoid, coordid, start, count, x_coor);
  } else {
    status = nc_get_vara_double(exoid, coordid, start, count, x_coor);
  }
      } 
      else if (i == 1 && y_coor != NULL) {
  which = "Y";
  if (ex_comp_ws(exoid) == 4) {
    status = nc_get_vara_float(exoid, coordid, start, count, y_coor);
  } else {
    status = nc_get_vara_double(exoid, coordid, start, count, y_coor);
  }
      } 
      else if (i == 2 && z_coor != NULL) {
  which = "Z";
  if (ex_comp_ws(exoid) == 4) {
    status = nc_get_vara_float(exoid, coordid, start, count, z_coor);
  } else {
    status = nc_get_vara_double(exoid, coordid, start, count, z_coor);
  }
      }

      if (status != NC_NOERR) {
  exerrval = status;
  sprintf(errmsg,
    "Error: failed to get %s coord array in file id %d", which, exoid);
  ex_err("ex_get_coord",errmsg,exerrval);
  return (EX_FATAL);
      }
    }

  } else {
    if ((status = nc_inq_varid (exoid, VAR_COORD_X, &coordidx)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
        "Error: failed to locate x nodal coordinates in file id %d", exoid);
      ex_err("ex_get_coord",errmsg,exerrval);
      return (EX_FATAL);
    }

    if (num_dim > 1) {
      if ((status = nc_inq_varid (exoid, VAR_COORD_Y, &coordidy)) != NC_NOERR) {
  exerrval = status;
  sprintf(errmsg,
    "Error: failed to locate y nodal coordinates in file id %d", exoid);
  ex_err("ex_get_coord",errmsg,exerrval);
  return (EX_FATAL);
      }
    } else {
      coordidy = 0;
    }

    if (num_dim > 2) {
      if ((status = nc_inq_varid (exoid, VAR_COORD_Z, &coordidz)) != NC_NOERR) {
  exerrval = status;
  sprintf(errmsg,
    "Error: failed to locate z nodal coordinates in file id %d", exoid);
  ex_err("ex_get_coord",errmsg,exerrval);
  return (EX_FATAL);
      }
    } else {
      coordidz = 0;
    }

    /* write out the coordinates  */
    for (i=0; i<num_dim; i++)
      {
        void *coor = NULL;
        char *which = NULL;
       
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
            status = nc_get_var_float(exoid, coordid, coor); 
          } else {
            status = nc_get_var_double(exoid, coordid, coor);
          }
          
          if (status != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
        "Error: failed to get %s coord array in file id %d", which, exoid);
      ex_err("ex_put_coord",errmsg,exerrval);
      return (EX_FATAL);
    }
        }
      }
  }
  return (EX_NOERR);
}
