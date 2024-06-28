/*
 * Copyright(C) 1999-2021 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************/
/*****************************************************************************/
/* Function(s) contained in this file:
 *     ex_get_init_global()
 *****************************************************************************
 * This function reads the global initial information.
 *****************************************************************************
 *  Variable Index:
 *      exoid            - The NetCDF ID of an already open NemesisI file.
 *      num_nodes_g     - The number of global FEM nodes. This is output as
 *                        a NetCDF variable.
 *      num_elems_g     - The number of global FEM elements. This is output
 *                        as a NetCDF variable.
 *      num_elem_blks_g - The number of global element blocks. This is output
 *                        as a NetCDF dimension.
 *      num_node_sets_g - The number of global node sets. This is output as
 *                        a NetCDF dimension.
 *      num_side_sets_g - The number of global side sets. This is output as
 *                        a NetCDF dimension.
 */
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

#include "exodusII.h"     // for ex_err, void_int, etc
#include "exodusII_int.h" // for EX_FATAL, etc

/* Used to reduce repeated code below */
static int ex_get_dim_value(int exoid, const char *name, const char *dimension_name, int dimension,
                            size_t *value)
{

  if (nc_inq_dimid(exoid, dimension_name, &dimension) != NC_NOERR) {
    /* optional and default to zero. */
    *value = 0;
  }
  else {
    size_t tmp;
    int    status;
    if ((status = nc_inq_dimlen(exoid, dimension, &tmp)) != NC_NOERR) {
      char errmsg[MAX_ERR_LENGTH];
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get number of %s in file id %d", name,
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      return (EX_FATAL);
    }
    *value = tmp;
  }
  return (EX_NOERR);
}

/*!
 * \ingroup ModelDescription
 * \undoc
 */
int ex_get_init_global(int exoid, void_int *num_nodes_g, void_int *num_elems_g,
                       void_int *num_elem_blks_g, void_int *num_node_sets_g,
                       void_int *num_side_sets_g)
{
  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Check the file version information */
  int dimid;
  if ((dimid = nei_check_file_version(exoid)) != EX_NOERR) {
    EX_FUNC_LEAVE(dimid);
  }

  size_t nng;
  if (ex_get_dim_value(exoid, "global nodes", DIM_NUM_NODES_GLOBAL, dimid, &nng) != EX_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  size_t neg;
  if (ex_get_dim_value(exoid, "global elements", DIM_NUM_ELEMS_GLOBAL, dimid, &neg) != EX_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  size_t nebg;
  if (ex_get_dim_value(exoid, "global element blocks", DIM_NUM_ELBLK_GLOBAL, dimid, &nebg) !=
      EX_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  size_t nnsg;
  if (ex_get_dim_value(exoid, "global node sets", DIM_NUM_NS_GLOBAL, dimid, &nnsg) != EX_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  size_t nssg;
  if (ex_get_dim_value(exoid, "global side sets", DIM_NUM_SS_GLOBAL, dimid, &nssg) != EX_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (ex_int64_status(exoid) & EX_BULK_INT64_API) {
    *(int64_t *)num_nodes_g     = nng;
    *(int64_t *)num_elems_g     = neg;
    *(int64_t *)num_elem_blks_g = nebg;
    *(int64_t *)num_node_sets_g = nnsg;
    *(int64_t *)num_side_sets_g = nssg;
  }
  else {
    *(int *)num_nodes_g     = nng;
    *(int *)num_elems_g     = neg;
    *(int *)num_elem_blks_g = nebg;
    *(int *)num_node_sets_g = nnsg;
    *(int *)num_side_sets_g = nssg;
  }

  EX_FUNC_LEAVE(EX_NOERR);
}
