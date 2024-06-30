/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
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

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, DIM_NUM_EM, etc

/*
 * reads the number of node and element maps
 */

int ex_get_map_param(int exoid, int *num_node_maps, int *num_elem_maps)
{
  int    status;
  int    dimid;
  size_t lnum_node_maps, lnum_elem_maps;
  char   errmsg[MAX_ERR_LENGTH];

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* node maps are optional */
  if (nc_inq_dimid(exoid, DIM_NUM_NM, &dimid) != NC_NOERR) {
    *num_node_maps = 0;
  }
  else {
    if ((status = nc_inq_dimlen(exoid, dimid, &lnum_node_maps)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get number of node maps in file id %d",
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
    *num_node_maps = lnum_node_maps;
  }

  /* element maps are optional */
  if (nc_inq_dimid(exoid, DIM_NUM_EM, &dimid) != NC_NOERR) {
    *num_elem_maps = 0;
  }
  else {
    if ((status = nc_inq_dimlen(exoid, dimid, &lnum_elem_maps)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get number of element maps in file id %d",
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
    *num_elem_maps = lnum_elem_maps;
  }
  EX_FUNC_LEAVE(EX_NOERR);
}
