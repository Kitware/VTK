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
/****************************************************************************/
/****************************************************************************/
/* Function(s) in this file:
 *     ex_get_node_cmap()
 *
 ****************************************************************************
 * Variable Index:
 *      exoid            - The NetCDF ID of an already open NemesisI file.
 *      map_id          - The ID of the nodal communication map to retrieve.
 *      node_ids        - Pointer to vector for retrieval of FEM node IDs
 *                        that make up this communication map.
 *      proc_ids        - Pointer to vector for retrieval of the processors
 *                        associated with each of the nodes in this nodal
 *                        communication map.
 *      processor         - The processor the file being read was written for.
 */
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, DIM_NCNT_CMAP, etc

int ex_get_node_cmap(int exoid, ex_entity_id map_id, void_int *node_ids, void_int *proc_ids,
                     int processor)
{
  int     map_idx, dimid, varid[2], status;
  size_t  start[1], count[1];
  int64_t varidx[2];

  char errmsg[MAX_ERR_LENGTH];

  EX_FUNC_ENTER();
  ex__check_valid_file_id(exoid, __func__);

  /* get the cmap information variables index */
  if (ex_get_idx(exoid, VAR_N_COMM_INFO_IDX, varidx, processor) == -1) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find index variable, \"%s\", in file ID %d",
             VAR_N_COMM_INFO_IDX, exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_LASTERR);

    EX_FUNC_LEAVE(EX_FATAL);
  }

  /*
   * no need to check if the second index is -1 that is handled
   * in ne__id_lkup, where the dimension must be looked up anyways
   */

  /* Get the index of the nodal comm map with the given ID */
  if ((map_idx = ne__id_lkup(exoid, VAR_N_COMM_IDS, varidx, map_id)) < 0) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to find nodal comm map with ID %" PRId64 " in file ID %d", map_id,
             exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_LASTERR);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* get the cmap data variables index for this map */
  if (ex_get_idx(exoid, VAR_N_COMM_DATA_IDX, varidx, map_idx) == -1) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find index variable, \"%s\", in file ID %d",
             VAR_N_COMM_DATA_IDX, exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_LASTERR);

    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (varidx[1] == -1) {
    /* Get the dimension of this nodal communication map */
    if ((status = nc_inq_dimid(exoid, DIM_NCNT_CMAP, &dimid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to find dimension ID for \"%s\" in file ID %d", DIM_NCNT_CMAP, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    if ((status = nc_inq_dimlen(exoid, dimid, count)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to find length of dimension \"%s\" in file ID %d", DIM_NCNT_CMAP,
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
    varidx[1] = count[0];
  }

  /* Get the variable ID for the nodal comm map node IDs */
  if ((status = nc_inq_varid(exoid, VAR_N_COMM_NIDS, &varid[0])) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find variable ID for \"%s\" in file ID %d",
             VAR_N_COMM_NIDS, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Get the variable ID for the nodal comm map processor IDs */
  if ((status = nc_inq_varid(exoid, VAR_N_COMM_PROC, &varid[1])) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find variable ID for \"%s\" in file ID %d",
             VAR_N_COMM_PROC, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Get the nodal comm map node IDs */
  start[0] = varidx[0];
  count[0] = varidx[1] - varidx[0];
  if (ex_int64_status(exoid) & EX_BULK_INT64_API) {
    status = nc_get_vara_longlong(exoid, varid[0], start, count, node_ids);
  }
  else {
    status = nc_get_vara_int(exoid, varid[0], start, count, node_ids);
  }

  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get variable \"%s\" from file ID %d",
             VAR_N_COMM_NIDS, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Get the nodal comm map processor IDs */
  if (ex_int64_status(exoid) & EX_BULK_INT64_API) {
    status = nc_get_vara_longlong(exoid, varid[1], start, count, proc_ids);
  }
  else {
    status = nc_get_vara_int(exoid, varid[1], start, count, proc_ids);
  }

  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get variable \"%s\" from file ID %d",
             VAR_N_COMM_PROC, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  EX_FUNC_LEAVE(EX_NOERR);
}
