/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, exi_comp_ws, etc

/*!
 * reads the coordinates of some of the nodes in the model for the specified component
 * Memory must be allocated for the coordinate array (coor)
 * before this call is made. The length of the array is at least `num_nodes`
 * Because the coordinates
 * are floating point values, the application code must declare the
 * array passed to be the appropriate type "float" or "double"
 * to match the compute word size passed in ex_create() or ex_open()
 * \param      exoid  exodus file id
 * \param      start_node_num  the starting index of the coordinates to be
 * returned.
 * \param      num_nodes  the number of nodes to read coordinates for.
 * \param      component  which component (1=X, 2=Y, 3=Z)
 * \param[out] coor Returned coordinate of the nodes for the specified component.
 */

int ex_get_partial_coord_component(int exoid, int64_t start_node_num, int64_t num_nodes,
                                   int component, void *coor)
{
  int status;
  int coordid;

  int         numnoddim, ndimdim;
  size_t      num_nod;
  size_t      num_dim, start[2], count[2];
  char        errmsg[MAX_ERR_LENGTH];
  const char *which = "XYZ";

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

  if ((status = nc_inq_dimlen(exoid, numnoddim, &num_nod)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get number of nodes in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  --start_node_num;
  if (start_node_num + num_nodes > num_nod) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: start index (%" PRId64 ") + node count (%" PRId64
             ") is larger than total number of nodes (%zu) in file id %d",
             start_node_num, num_nodes, num_nod, exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (exi_get_dimension(exoid, DIM_NUM_DIM, "dimensions", &num_dim, &ndimdim, __func__) !=
      NC_NOERR) {
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

  /* read in the coordinates  */
  if (ex_large_model(exoid) == 0) {
    if ((status = nc_inq_varid(exoid, VAR_COORD, &coordid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate nodal coordinates in file id %d",
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    {
      start[0] = component;
      start[1] = start_node_num;

      count[0] = 1;
      count[1] = num_nodes;
      if (count[1] == 0) {
        start[1] = 0;
      }

      if (exi_comp_ws(exoid) == 4) {
        status = nc_get_vara_float(exoid, coordid, start, count, coor);
      }
      else {
        status = nc_get_vara_double(exoid, coordid, start, count, coor);
      }

      if (status != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get %c coord array in file id %d",
                 which[component], exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }
    }
  }
  else {
    /* read the coordinates  */
    {
      char *comp[] = {VAR_COORD_X, VAR_COORD_Y, VAR_COORD_Z};
      start[0]     = start_node_num;
      count[0]     = num_nodes;
      if (count[0] == 0) {
        start[0] = 0;
      }

      if ((status = nc_inq_varid(exoid, comp[component], &coordid)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to locate %c nodal coordinates in file id %d", which[component],
                 exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }

      if (exi_comp_ws(exoid) == 4) {
        status = nc_get_vara_float(exoid, coordid, start, count, coor);
      }
      else {
        status = nc_get_vara_double(exoid, coordid, start, count, coor);
      }

      if (status != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get %c coord array in file id %d",
                 which[component], exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }
    }
  }
  EX_FUNC_LEAVE(EX_NOERR);
}
