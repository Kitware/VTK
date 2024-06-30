/*
 * Copyright(C) 1999-2020, 2022 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * expcor - ex_put_partial_coord
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       int     start_node_num          starting index (1-based) of coordinates
 *to be written.
 *       int     num_nodes               number of nodes to write coordinates
 *for.
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

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, exi_comp_ws, etc

/*!
 * writes the coordinates of some of the nodes in the model
 * Only writes the 'non-null' arrays.
 * \param   exoid           exodus file id
 * \param   start_node_num  the starting index (1-based) of the coordinates to
 * be written
 * \param   num_nodes       the number of nodes to write coordinates for.
 * \param   x_coor          x coord array
 * \param   y_coor          y coord array
 * \param   z_coor          z coord array
 */

int ex_put_partial_coord(int exoid, int64_t start_node_num, int64_t num_nodes, const void *x_coor,
                         const void *y_coor, const void *z_coor)
{
  int status;
  int coordid = -1;
  int coordidx, coordidy, coordidz;

  int     numnoddim, ndimdim;
  int64_t num_nod;
  size_t  i, num_dim, start[2], count[2];
  char    errmsg[MAX_ERR_LENGTH];

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* inquire id's of previously defined dimensions  */

  if (nc_inq_dimid(exoid, DIM_NUM_NODES, &numnoddim) != NC_NOERR) {
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

  /* write out the coordinates  */
  if (num_dim > 0) {
    if ((status = nc_inq_varid(exoid, VAR_COORD_X, &coordidx)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate x nodal coordinates in file id %d",
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }
  else {
    coordidx = -1;
  }
  if (num_dim > 1) {
    if ((status = nc_inq_varid(exoid, VAR_COORD_Y, &coordidy)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate y nodal coordinates in file id %d",
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }
  else {
    coordidy = -1;
  }
  if (num_dim > 2) {
    if ((status = nc_inq_varid(exoid, VAR_COORD_Z, &coordidz)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate z nodal coordinates in file id %d",
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }
  else {
    coordidz = -1;
  }

  /* write out the coordinates  */
  for (i = 0; i < num_dim; i++) {
    const void *coor  = NULL;
    char       *which = NULL;

    start[0] = start_node_num;
    count[0] = num_nodes;
    if (count[0] == 0) {
      start[0] = 0;
    }

    if (i == 0) {
      coor    = x_coor;
      which   = "X";
      coordid = coordidx;
    }
    else if (i == 1) {
      coor    = y_coor;
      which   = "Y";
      coordid = coordidy;
    }
    else if (i == 2) {
      coor    = z_coor;
      which   = "Z";
      coordid = coordidz;
    }

    if (coor != NULL && coordid != -1) {
      if (exi_comp_ws(exoid) == 4) {
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
  }
  EX_FUNC_LEAVE(EX_NOERR);
}
