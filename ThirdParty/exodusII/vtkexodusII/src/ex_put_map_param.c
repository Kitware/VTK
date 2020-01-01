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
 *
 * expmp - ex_put_map_param
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       int     num_node_maps           number of node maps
 *       int     num_elem_maps           number of element maps
 *
 * exit conditions -
 *
 *
 *****************************************************************************/

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, etc

/*!
 * defines the number of node and element maps. It is more efficient
 * to define both of these at the same time; however, they can be
 * defined in separate calls by setting only one of the counts to a
 * non-zero value. It is an error to redefine the number of node or
 * element maps.
 * \param exoid                   exodus file id
 * \param num_node_maps           number of node maps
 * \param num_elem_maps           number of element maps
 */

int ex_put_map_param(int exoid, int num_node_maps, int num_elem_maps)
{
  int  dim[2], dimid, strdim, varid, status;
  int  var_nm_id, var_em_id;
  int  i;
  char errmsg[MAX_ERR_LENGTH];
  int  id_type  = NC_INT;
  int  int_type = NC_INT;
#if NC_HAS_HDF5
  int fill = NC_FILL_CHAR;
#endif

  EX_FUNC_ENTER();
  ex__check_valid_file_id(exoid, __func__);

  if (ex_int64_status(exoid) & EX_IDS_INT64_DB) {
    id_type = NC_INT64;
  }
  if (ex_int64_status(exoid) & EX_BULK_INT64_DB) {
    int_type = NC_INT64;
  }

  /* return if these have been defined before */
  if ((num_node_maps > 0 && ((nc_inq_dimid(exoid, DIM_NUM_NM, &dimid)) == NC_NOERR)) ||
      (num_elem_maps > 0 && ((nc_inq_dimid(exoid, DIM_NUM_EM, &dimid)) == NC_NOERR))) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: number of maps already defined for file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if ((num_node_maps > 0) || (num_elem_maps > 0)) {

    /* inquire previously defined dimensions  */
    if ((status = nc_inq_dimid(exoid, DIM_STR_NAME, &strdim)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get string length in file id %d", exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* put file into define mode */
    if ((status = nc_redef(exoid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to put file id %d into define mode", exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* node maps: */
    if (num_node_maps > 0) {

      if ((status = nc_def_dim(exoid, DIM_NUM_NM, num_node_maps, &dimid)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to define number of node maps in file id %d", exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        goto error_ret; /* exit define mode and return */
      }

      /* node maps id array: */
      dim[0] = dimid;
      if ((status = nc_def_var(exoid, VAR_NM_PROP(1), id_type, 1, dim, &var_nm_id)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to create node maps property array in file id %d", exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        goto error_ret; /* exit define mode and return */
      }

      /*   store property name as attribute of property array variable */
      if ((status = nc_put_att_text(exoid, var_nm_id, ATT_PROP_NAME, 3, "ID")) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to store node map property name %s in file id %d", "ID", exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        goto error_ret; /* exit define mode and return */
      }

      /* Node map names... */
      dim[0] = dimid;
      dim[1] = strdim;

      if (nc_def_var(exoid, VAR_NAME_NM, NC_CHAR, 2, dim, &varid) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to define node map name array in file id %d", exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        goto error_ret; /* exit define mode and return */
      }
#if NC_HAS_HDF5
      nc_def_var_fill(exoid, varid, 0, &fill);
#endif

      /* determine number of nodes */
      if ((status = nc_inq_dimid(exoid, DIM_NUM_NODES, &dimid)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: couldn't determine number of nodes in file id %d",
                 exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        goto error_ret; /* exit define mode and return */
      }

      dim[0] = dimid;

      /* create variable array in which to store the node maps */
      for (i = 0; i < num_node_maps; i++) {
        if ((status = nc_def_var(exoid, VAR_NODE_MAP(i + 1), int_type, 1, dim, &varid)) !=
            NC_NOERR) {
          if (status == NC_ENAMEINUSE) {
            snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: node map %d already defined in file id %d", i,
                     exoid);
            ex_err_fn(exoid, __func__, errmsg, status);
          }
          else {
            snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to create node map %d in file id %d", i,
                     exoid);
            ex_err_fn(exoid, __func__, errmsg, status);
          }
          goto error_ret; /* exit define mode and return */
        }
        ex__compress_variable(exoid, varid, 1);
      }
    }

    /* element maps: */
    if (num_elem_maps > 0) {
      if ((status = nc_def_dim(exoid, DIM_NUM_EM, num_elem_maps, &dimid)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to define number of element maps in file id %d", exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        goto error_ret; /* exit define mode and return */
      }

      /* element maps id array: */
      dim[0] = dimid;
      if ((status = nc_def_var(exoid, VAR_EM_PROP(1), id_type, 1, dim, &var_em_id)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to create element maps property array in file id %d", exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        goto error_ret; /* exit define mode and return */
      }

      /*   store property name as attribute of property array variable */
      if ((status = nc_put_att_text(exoid, var_em_id, ATT_PROP_NAME, 3, "ID")) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to store element map property name %s in file id %d", "ID", exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        goto error_ret; /* exit define mode and return */
      }

      /* Element map names... */
      dim[0] = dimid;
      dim[1] = strdim;

      if ((status = nc_def_var(exoid, VAR_NAME_EM, NC_CHAR, 2, dim, &varid)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to define element map name array in file id %d", exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        goto error_ret; /* exit define mode and return */
      }
#if NC_HAS_HDF5
      nc_def_var_fill(exoid, varid, 0, &fill);
#endif

      /* determine number of elements */
      if ((status = nc_inq_dimid(exoid, DIM_NUM_ELEM, &dimid)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: couldn't determine number of elements in file id %d", exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        goto error_ret; /* exit define mode and return */
      }

      /* create variable array in which to store the element maps */
      dim[0] = dimid;
      for (i = 0; i < num_elem_maps; i++) {
        if ((status = nc_def_var(exoid, VAR_ELEM_MAP(i + 1), int_type, 1, dim, &varid)) !=
            NC_NOERR) {
          if (status == NC_ENAMEINUSE) {
            snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: element map %d already defined in file id %d",
                     i, exoid);
            ex_err_fn(exoid, __func__, errmsg, status);
          }
          else {
            snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to create element map %d in file id %d",
                     i, exoid);
            ex_err_fn(exoid, __func__, errmsg, status);
          }
          goto error_ret; /* exit define mode and return */
        }
        ex__compress_variable(exoid, varid, 1);
      }
    }

    /* leave define mode */
    if ((status = ex__leavedef(exoid, __func__)) != NC_NOERR) {
      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* Fill the id arrays with EX_INVALID_ID */
    {
      int  maxset      = num_node_maps > num_elem_maps ? num_node_maps : num_elem_maps;
      int *invalid_ids = malloc(maxset * sizeof(int));
      if (invalid_ids == NULL) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to allocate memory for invalid id "
                 "storage in file id %d",
                 exoid);
        ex_err_fn(exoid, __func__, errmsg, EX_MEMFAIL);
        EX_FUNC_LEAVE(EX_FATAL);
      }
      for (i = 0; i < maxset; i++) {
        invalid_ids[i] = EX_INVALID_ID;
      }
      if (num_node_maps > 0) {
        if ((status = nc_put_var_int(exoid, var_nm_id, invalid_ids)) != NC_NOERR) {
          snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to output node map ids in file id %d",
                   exoid);
          ex_err_fn(exoid, __func__, errmsg, status);
          free(invalid_ids);
          EX_FUNC_LEAVE(EX_FATAL);
        }
      }
      if (num_elem_maps > 0) {
        if ((status = nc_put_var_int(exoid, var_em_id, invalid_ids)) != NC_NOERR) {
          snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to output element map ids in file id %d",
                   exoid);
          ex_err_fn(exoid, __func__, errmsg, status);
          free(invalid_ids);
          EX_FUNC_LEAVE(EX_FATAL);
        }
      }
      free(invalid_ids);
    }
  }

  EX_FUNC_LEAVE(EX_NOERR);

/* Fatal error: exit definition mode and return */
error_ret:
  ex__leavedef(exoid, __func__);
  EX_FUNC_LEAVE(EX_FATAL);
}
