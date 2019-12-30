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

/*****************************************************************************
 * This function outputs the nodal map.
 *****************************************************************************
 * Variable Index:
 *      exoid            - The NetCDF ID of an already open NemesisI file.
 *      node_mapi       - Pointer to vector containing the internal FEM
 *                        nodal IDs.
 *      node_mapb       - Pointer to vector containing the border FEM
 *                        nodal IDs.
 *      node_mape       - Pointer to vector containing the external FEM
 *                        nodal IDs.
 *      proc_id         - The processor the file is being written for.
 */
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

#include <exodusII.h>     // for ex_err, etc
#include <exodusII_int.h> // for EX_FATAL, DIM_NUM_BOR_NODES, etc

int ex_put_processor_node_maps(int exoid, void_int *node_mapi, void_int *node_mapb,
                               void_int *node_mape, int proc_id)
{
  int     status, varid, dimid;
  char    ftype[2];
  size_t  start[1], count[1];
  int64_t varidx[2];
  int     nmstat;

  char errmsg[MAX_ERR_LENGTH];
  /*-----------------------------Execution begins-----------------------------*/

  EX_FUNC_ENTER();
  ex__check_valid_file_id(exoid, __func__);

  /* Get the file type */
  if (ex__get_file_type(exoid, ftype) != EX_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: unable to find file type for file ID %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);

    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Get the status of this node map */
  if ((status = nc_inq_varid(exoid, VAR_INT_N_STAT, &varid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find variable ID for \"%s\" from file ID %d",
             VAR_INT_N_STAT, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (ftype[0] == 'p') {
    start[0] = 0;
  }
  else {
    start[0] = proc_id;
  }

  if ((status = nc_get_var1_int(exoid, varid, start, &nmstat)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get status for \"%s\" from file %d",
             VAR_INT_N_STAT, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Write out the internal node-number map */
  if (nmstat == 1) {
    /* get the index */
    if (ex_get_idx(exoid, VAR_NODE_MAP_INT_IDX, varidx, proc_id) == -1) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to find index variable, \"%s\", in file ID %d", VAR_NODE_MAP_INT_IDX,
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);

      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* check if I need to get the dimension */
    if (varidx[1] == -1) {
      if ((status = nc_inq_dimid(exoid, DIM_NUM_INT_NODES, &dimid)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to find dimension ID for \"%s\" in file ID %d", DIM_NUM_INT_NODES,
                 exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }

      if ((status = nc_inq_dimlen(exoid, dimid, count)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to find length of dimension \"%s\" in file ID %d",
                 DIM_NUM_INT_NODES, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }

      varidx[1] = count[0];
    }

    if ((status = nc_inq_varid(exoid, VAR_NODE_MAP_INT, &varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find variable ID for \"%s\" in file ID %d",
               VAR_NODE_MAP_INT, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);

      EX_FUNC_LEAVE(EX_FATAL);
    }

    start[0] = varidx[0];
    count[0] = varidx[1] - varidx[0];
    if (ex_int64_status(exoid) & EX_MAPS_INT64_API) {
      status = nc_put_vara_longlong(exoid, varid, start, count, node_mapi);
    }
    else {
      status = nc_put_vara_int(exoid, varid, start, count, node_mapi);
    }
    if (status != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to output variable \"%s\" in file ID %d",
               VAR_NODE_MAP_INT, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  } /* End "if (nmstat == 1)" */

  /* Get the status of this node map */
  if ((status = nc_inq_varid(exoid, VAR_BOR_N_STAT, &varid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find variable ID for \"%s\" from file ID %d",
             VAR_BOR_N_STAT, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (ftype[0] == 'p') {
    start[0] = 0;
  }
  else {
    start[0] = proc_id;
  }

  if ((status = nc_get_var1_int(exoid, varid, start, &nmstat)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get status for \"%s\" from file %d",
             VAR_BOR_N_STAT, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (nmstat == 1) {
    /* Write out the border node-number map */
    /* get the index */
    if (ex_get_idx(exoid, VAR_NODE_MAP_BOR_IDX, varidx, proc_id) == -1) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to find index variable, \"%s\", in file ID %d", VAR_NODE_MAP_BOR_IDX,
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);

      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* check if I need to get the dimension */
    if (varidx[1] == -1) {
      if ((status = nc_inq_dimid(exoid, DIM_NUM_BOR_NODES, &dimid)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to find dimension ID for \"%s\" in file ID %d", DIM_NUM_BOR_NODES,
                 exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }

      if ((status = nc_inq_dimlen(exoid, dimid, count)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to find length of dimension \"%s\" in file ID %d",
                 DIM_NUM_BOR_NODES, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }

      varidx[1] = count[0];
    }

    if ((status = nc_inq_varid(exoid, VAR_NODE_MAP_BOR, &varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find variable ID for \"%s\" in file ID %d",
               VAR_NODE_MAP_BOR, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);

      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* Output the map */
    start[0] = varidx[0];
    count[0] = varidx[1] - varidx[0];
    if (ex_int64_status(exoid) & EX_MAPS_INT64_API) {
      status = nc_put_vara_longlong(exoid, varid, start, count, node_mapb);
    }
    else {
      status = nc_put_vara_int(exoid, varid, start, count, node_mapb);
    }
    if (status != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to output variable \"%s\" in file ID %d",
               VAR_NODE_MAP_BOR, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  } /* End "if (nmstat == 1)" */

  /* Get the status of this node map */
  if ((status = nc_inq_varid(exoid, VAR_EXT_N_STAT, &varid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find variable ID for \"%s\" from file ID %d",
             VAR_EXT_N_STAT, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (ftype[0] == 'p') {
    start[0] = 0;
  }
  else {
    start[0] = proc_id;
  }

  if ((status = nc_get_var1_int(exoid, varid, start, &nmstat)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get status for \"%s\" from file %d",
             VAR_EXT_N_STAT, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (nmstat == 1) {
    /* Write out the external node-number map */
    if (ex_get_idx(exoid, VAR_NODE_MAP_EXT_IDX, varidx, proc_id) == -1) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to find index variable, \"%s\", in file ID %d", VAR_NODE_MAP_EXT_IDX,
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);

      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* check if I need to get the dimension */
    if (varidx[1] == -1) {
      if ((status = nc_inq_dimid(exoid, DIM_NUM_EXT_NODES, &dimid)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to find dimension ID for \"%s\" in file ID %d", DIM_NUM_EXT_NODES,
                 exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }

      if ((status = nc_inq_dimlen(exoid, dimid, count)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to find length of dimension \"%s\" in file ID %d",
                 DIM_NUM_EXT_NODES, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }
      varidx[1] = count[0];
    }

    if ((status = nc_inq_varid(exoid, VAR_NODE_MAP_EXT, &varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find variable ID for \"%s\" in file ID %d",
               VAR_NODE_MAP_EXT, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);

      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* Output the map */
    start[0] = varidx[0];
    count[0] = varidx[1] - varidx[0];
    if (ex_int64_status(exoid) & EX_MAPS_INT64_API) {
      status = nc_put_vara_longlong(exoid, varid, start, count, node_mape);
    }
    else {
      status = nc_put_vara_int(exoid, varid, start, count, node_mape);
    }
    if (status != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to output variable \"%s\" in file ID %d",
               VAR_NODE_MAP_EXT, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  } /* End "if (nmstat == 1)" */
  EX_FUNC_LEAVE(EX_NOERR);
}
