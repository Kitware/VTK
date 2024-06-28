/*
 * Copyright(C) 1999-2022 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, exi_comp_ws, etc

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
  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* inquire id's of previously defined dimensions  */

  int status;
  int numnoddim;
  if (nc_inq_dimid(exoid, DIM_NUM_NODES, &numnoddim) != NC_NOERR) {
    /* If not found, then this file is storing 0 nodes.
       Return immediately */
    EX_FUNC_LEAVE(EX_NOERR);
  }

  int64_t num_nod;
  {
    size_t tmp;
    if ((status = nc_inq_dimlen(exoid, numnoddim, &tmp)) != NC_NOERR) {
      char errmsg[MAX_ERR_LENGTH];
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: inquire failed to return number of nodes in file id %d", exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
    num_nod = tmp;
  }

  int ndimdim;
  if ((status = nc_inq_dimid(exoid, DIM_NUM_DIM, &ndimdim)) != NC_NOERR) {
    char errmsg[MAX_ERR_LENGTH];
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate number of dimensions in file id %d",
             exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  size_t num_dim;
  if ((status = nc_inq_dimlen(exoid, ndimdim, &num_dim)) != NC_NOERR) {
    char errmsg[MAX_ERR_LENGTH];
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get number of dimensions in file id %d",
             exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  --start_node_num;
  if (start_node_num + num_nodes > num_nod) {
    char errmsg[MAX_ERR_LENGTH];
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: start index (%" PRId64 ") + node count (%" PRId64
             ") is larger than total number of nodes (%" PRId64 ") in file id %d",
             start_node_num, num_nodes, num_nod, exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (component > num_dim) {
    char errmsg[MAX_ERR_LENGTH];
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

    size_t start[] = {start_node_num};
    size_t count[] = {num_nodes};
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

    int coordid;
    if ((status = nc_inq_varid(exoid, comp, &coordid)) != NC_NOERR) {
      char errmsg[MAX_ERR_LENGTH];
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate %s nodal coordinates in file id %d",
               which, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    if (exi_comp_ws(exoid) == 4) {
      status = nc_put_vara_float(exoid, coordid, start, count, coor);
    }
    else {
      status = nc_put_vara_double(exoid, coordid, start, count, coor);
    }

    if (status != NC_NOERR) {
      char errmsg[MAX_ERR_LENGTH];
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to put %s coord array in file id %d", which,
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }
  EX_FUNC_LEAVE(EX_NOERR);
}
