/*
 * Copyright(C) 1999-, 20212021 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/****************************************************************************
 * This function outputs a portion of the nodal communication map.
 ****************************************************************************
 * Variable Index:
 *      exoid           - The NetCDF ID of an already open NemesisI file.
 *      map_id          - The ID of the nodal communication map to retrieve.
 *      node_ids        - Pointer to vector containing the FEM node IDs
 *                        that make up this communication map.
 *      proc_ids        - Pointer to vector containing the processors IDs
 *                        associated with each of the nodes in this nodal
 *                        communication map.
 *      processor       - The processor the file is being written for.
 */
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

#include <exodusII.h>     // for ex_err, etc
#include <exodusII_int.h> // for EX_FATAL, DIM_NCNT_CMAP, etc

/*!
 * writes out a portion of the nodal communication map to the
 * database; This map specifies the nodes on a processor boundary and
 * the processors that those nodes are shared with.
 *
 * \param    exoid                   exodus file id
 * \param    map_id
 * \param    start_entity_num
 * \param    num_entities
 * \param    node_ids               the file-local node ids on the processor boundary
 * \param    proc_ids               the processor ids associated with each of the node_ids
 * \param    processor              the processor the file is being written for.
 */

int ex_put_partial_node_cmap(int exoid, ex_entity_id map_id, int64_t start_entity_num,
                             int64_t num_entities, const void_int *node_ids,
                             const void_int *proc_ids, int processor)
{
  int     map_idx, varid, dimid, status;
  size_t  start[1], count[1], ret_val;
  int64_t varidx[2];
  int     value;

  char errmsg[MAX_ERR_LENGTH];
  /*-----------------------------Execution begins-----------------------------*/

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* get the index for the comm map information variables */
  if (ex_get_idx(exoid, VAR_N_COMM_INFO_IDX, varidx, processor) == -1) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find index variable, \"%s\", in file ID %d",
             VAR_N_COMM_INFO_IDX, exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_LASTERR);

    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Get the index for this map_id */
  if ((map_idx = nei_id_lkup(exoid, VAR_N_COMM_IDS, varidx, map_id)) == -1) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to find index for variable \"%s\" in file ID %d", VAR_N_COMM_IDS,
             exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_LASTERR);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /*
   * Find out if this is a NULL comm map by checking it's entry in
   * the status vector.
   */
  if ((status = nc_inq_varid(exoid, VAR_N_COMM_STAT, &varid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find variable ID for \"%s\" in file ID %d",
             VAR_N_COMM_STAT, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  start[0] = map_idx;
  if ((status = nc_get_var1_int(exoid, varid, start, &value)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: unable to get variable \"%s\" from file ID %d",
             VAR_N_COMM_STAT, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (value == 0) {
    EX_FUNC_LEAVE(EX_NOERR); /* NULL set */
  }

  /* now I need to get the comm map data index */
  if (ex_get_idx(exoid, VAR_N_COMM_DATA_IDX, varidx, map_idx) == -1) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find index variable, \"%s\", in file ID %d",
             VAR_N_COMM_DATA_IDX, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);

    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* check if I need to get the dimension of the cmap data */
  if (varidx[1] == -1) {
    /* Get the size of the comm maps */
    if ((status = nc_inq_dimid(exoid, DIM_NCNT_CMAP, &dimid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get dimension ID for \"%s\" in file ID %d",
               DIM_NCNT_CMAP, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    if ((status = nc_inq_dimlen(exoid, dimid, &ret_val)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to find length of dimension \"%s\" in file ID %d", DIM_NCNT_CMAP,
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    varidx[1] = ret_val;
  } /* "if (varidx[1]==-1)" */

  start[0] = varidx[0] + start_entity_num - 1;
  count[0] = num_entities; /* varidx[1] - varidx[0]; */
  if (num_entities == 0) {
    start[0] = 0;
  }

  /* Output the node IDs for this comm map */
  if ((status = nc_inq_varid(exoid, VAR_N_COMM_NIDS, &varid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find variable ID for \"%s\" in file ID %d",
             VAR_N_COMM_NIDS, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (ex_int64_status(exoid) & EX_BULK_INT64_API) {
    status = nc_put_vara_longlong(exoid, varid, start, count, node_ids);
  }
  else {
    status = nc_put_vara_int(exoid, varid, start, count, node_ids);
  }
  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to output vector \"%s\" in file ID %d",
             VAR_N_COMM_NIDS, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Output the processor IDs for this map */
  if ((status = nc_inq_varid(exoid, VAR_N_COMM_PROC, &varid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find variable ID for \"%s\" in file ID %d",
             VAR_N_COMM_PROC, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (ex_int64_status(exoid) & EX_BULK_INT64_API) {
    status = nc_put_vara_longlong(exoid, varid, start, count, proc_ids);
  }
  else {
    status = nc_put_vara_int(exoid, varid, start, count, proc_ids);
  }
  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to output variable \"%s\" in file ID %d",
             VAR_N_COMM_PROC, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  EX_FUNC_LEAVE(EX_NOERR);
}
