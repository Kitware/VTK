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

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, ex__comp_ws, etc

/*!
 * writes the coordinates of some of the nodes in the model for the specified component
 * \param   exoid           exodus file id
 * \param   start_node_num  the starting index (1-based) of the coordinates to
 * be written
 * \param   num_nodes       the number of nodes to write coordinates for.
 * \param   component       which component (1=X, 2=Y, 3=Z)
 * \param   coor            coord array
 */

int ex_put_partial_coord_component(int exoid, int64_t start_node_num, int64_t num_nodes,
                                   int component, const void *coor)
{
  int status;
  int coordid;

  int     numnoddim, ndimdim;
  int64_t num_nod;
  size_t  num_dim, start[2], count[2];
  char    errmsg[MAX_ERR_LENGTH];

  EX_FUNC_ENTER();
  ex__check_valid_file_id(exoid, __func__);

  /* inquire id's of previously defined dimensions  */

  if ((status = nc_inq_dimid(exoid, DIM_NUM_NODES, &numnoddim)) != NC_NOERR) {
    /* If not found, then this file is storing 0 nodes.
       Return immediately */
    EX_FUNC_LEAVE(EX_NOERR);
  }

  {
    size_t tmp;
    if ((status = nc_inq_dimlen(exoid, numnoddim, &tmp)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: inquire failed to return number of nodes in file id %d", exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
    num_nod = tmp;
  }

  if ((status = nc_inq_dimid(exoid, DIM_NUM_DIM, &ndimdim)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate number of dimensions in file id %d",
             exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if ((status = nc_inq_dimlen(exoid, ndimdim, &num_dim)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get number of dimensions in file id %d",
             exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  --start_node_num;
  if (start_node_num + num_nodes > num_nod) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: start index (%" PRId64 ") + node count (%" PRId64
             ") is larger than total number of nodes (%" PRId64 ") in file id %d",
             start_node_num, num_nodes, num_nod, exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (component > num_dim) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: Component (%d) is larger than number of dimensions (%zu) in file id %d",
             component, num_dim, exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
  }
  --component;

  /* write out the coordinates  */
  {
    char *which = NULL;
    char *comp  = NULL;

    start[0] = start_node_num;
    count[0] = num_nodes;
    if (count[0] == 0) {
      start[0] = 0;
    }

    if (component == 0) {
      which = "X";
      comp  = VAR_COORD_X;
    }
    else if (component == 1) {
      which = "Y";
      comp  = VAR_COORD_Y;
    }
    else if (component == 2) {
      which = "Z";
      comp  = VAR_COORD_Z;
    }

    if ((status = nc_inq_varid(exoid, comp, &coordid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate %s nodal coordinates in file id %d",
               which, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    if (ex__comp_ws(exoid) == 4) {
      status = nc_put_vara_float(exoid, coordid, start, count, coor);
    }
    else {
      status = nc_put_vara_double(exoid, coordid, start, count, coor);
    }

    if (status != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to put %s coord array in file id %d", which,
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }
  EX_FUNC_LEAVE(EX_NOERR);
}
