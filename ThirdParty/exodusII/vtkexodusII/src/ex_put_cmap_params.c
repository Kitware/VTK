/*
 * Copyright(C) 1999-2022 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/* Function(s) contained in this file:
 *     ex_put_cmap_params()
 *****************************************************************************
 * This function outputs the communication map parameters.
 *****************************************************************************
 *  Variable Index:
 *      exoid                - The NetCDF ID of an already open NemesisI file.
 *      node_cmap_ids       - Pointer to vector of nodal communication
 *                            set IDs.
 *      node_cmap_node_cnts - Pointer to a vector which contains a count of
 *                            the number of FEM nodes for each nodal
 *                            communication map.
 *      elem_cmap_ids       - Pointer to vector for retrieval of elemental
 *                            communication set IDs.
 *      elem_cmap_elem_cnts - Pointer to a vector which contains a count of
 *                            the number of FEM elements for each elemental
 *                            communication map.
 *      processor           - The processor the file being read was written
 *                            for.
 */
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

#include <exodusII.h>     // for ex_err, etc
#include <exodusII_int.h> // for EX_FATAL, exi_leavedef, etc

int ex_put_cmap_params(int exoid, const void_int *node_cmap_ids,
                       const void_int *node_cmap_node_cnts, const void_int *elem_cmap_ids,
                       const void_int *elem_cmap_elem_cnts, int64_t processor)
{
  int    dimid[1];
  size_t start[1];
  char   ftype[2];

  char errmsg[MAX_ERR_LENGTH];

  EX_UNUSED(processor);

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  int format;
  nc_inq_format(exoid, &format);

  int index_type = NC_INT;
  if ((ex_int64_status(exoid) & EX_BULK_INT64_DB) || (format == NC_FORMAT_NETCDF4)) {
    index_type = NC_INT64;
  }

  int id_type = NC_INT;
  if (ex_int64_status(exoid) & EX_IDS_INT64_DB) {
    id_type = NC_INT64;
  }
  /*-----------------------------Execution begins-----------------------------*/

  /*
  ** with the new database format, this function should only
  ** be used for writing a parallel file
  */
  /* Get the file type */
  if (exi_get_file_type(exoid, ftype) != EX_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get file type from file ID %d\n", exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_MSG);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* make sure that this is a parallel file */
  if (ftype[0] != 'p') {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: function for use with parallel files only, file ID %d\n", exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_MSG);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Put NetCDF file into define mode */
  int status;
  if ((status = exi_redef(exoid, __func__)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to file ID %d into define mode", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Check to see if there are nodal communications maps in the file */
  size_t num_n_comm_maps;
  if (nc_inq_dimid(exoid, DIM_NUM_N_CMAPS, &dimid[0]) != NC_NOERR) {
    num_n_comm_maps = 0;
  }
  else {
    if ((status = nc_inq_dimlen(exoid, dimid[0], &num_n_comm_maps)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to find length of dimension \"%s\" in file ID %d", DIM_NUM_N_CMAPS,
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }

  /*
   * Add dimensions for the size of the number of nodal
   * communication maps.
   */
  int n_varid_idx;
  if (num_n_comm_maps > 0) {

    /* add the communications data index variable */
    if ((status = nc_def_var(exoid, VAR_N_COMM_DATA_IDX, index_type, 1, dimid, &n_varid_idx)) !=
        NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to add variable \"%s\" in file ID %d",
               VAR_N_COMM_DATA_IDX, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* Add dimensions for all of the nodal communication maps */
    int ncnt_cmap = 0;
    if (ex_int64_status(exoid) & EX_BULK_INT64_API) {
      for (size_t icm = 0; icm < num_n_comm_maps; icm++) {
        ncnt_cmap += ((int64_t *)node_cmap_node_cnts)[icm];
      }
    }
    else {
      for (size_t icm = 0; icm < num_n_comm_maps; icm++) {
        ncnt_cmap += ((int *)node_cmap_node_cnts)[icm];
      }
    }

    if ((status = nc_def_dim(exoid, DIM_NCNT_CMAP, ncnt_cmap, &dimid[0])) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to add dimension for \"%s\" in file ID %d",
               DIM_NCNT_CMAP, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* Define variables for the nodal IDS and processor vectors */
    int varid;
    if ((status = nc_def_var(exoid, VAR_N_COMM_NIDS, id_type, 1, dimid, &varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to add variable \"%s\" in file ID %d",
               VAR_N_COMM_NIDS, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }
    exi_compress_variable(exoid, varid, 1);

    if ((status = nc_def_var(exoid, VAR_N_COMM_PROC, NC_INT, 1, dimid, &varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to add variable \"%s\" in file ID %d",
               VAR_N_COMM_PROC, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }
    exi_compress_variable(exoid, varid, 1);

  } /* End "if (num_n_comm_maps > 0)" */

  /* Check to see if there are elemental communications maps in the file */
  size_t num_e_comm_maps;
  if (nc_inq_dimid(exoid, DIM_NUM_E_CMAPS, &dimid[0]) != NC_NOERR) {
    num_e_comm_maps = 0;
  }
  else {
    if ((status = nc_inq_dimlen(exoid, dimid[0], &num_e_comm_maps)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to find length of dimension \"%s\" in file ID %d", DIM_NUM_E_CMAPS,
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }

  /*
   * Add dimensions for the size of the number of elemental
   * communication maps.
   */
  int e_varid_idx;
  if (num_e_comm_maps > 0) {

    /* add the communications data index variable */
    if ((status = nc_def_var(exoid, VAR_E_COMM_DATA_IDX, index_type, 1, dimid, &e_varid_idx)) !=
        NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to add variable \"%s\" in file ID %d",
               VAR_E_COMM_DATA_IDX, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* Add dimensions for each of the nodal communication maps */
    int ecnt_cmap = 0;
    if (ex_int64_status(exoid) & EX_BULK_INT64_API) {
      for (size_t icm = 0; icm < num_e_comm_maps; icm++) {
        ecnt_cmap += ((int64_t *)elem_cmap_elem_cnts)[icm];
      }
    }
    else {
      for (size_t icm = 0; icm < num_e_comm_maps; icm++) {
        ecnt_cmap += ((int *)elem_cmap_elem_cnts)[icm];
      }
    }
    if ((status = nc_def_dim(exoid, DIM_ECNT_CMAP, ecnt_cmap, &dimid[0])) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to add dimension for \"%s\" in file ID %d",
               DIM_ECNT_CMAP, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* Define variables for the element IDS and processor vectors */
    int varid;
    if ((status = nc_def_var(exoid, VAR_E_COMM_EIDS, id_type, 1, dimid, &varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to add variable \"%s\" in file ID %d",
               VAR_E_COMM_EIDS, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }
    exi_compress_variable(exoid, varid, 1);

    if ((status = nc_def_var(exoid, VAR_E_COMM_PROC, NC_INT, 1, dimid, &varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to add variable \"%s\" in file ID %d",
               VAR_E_COMM_PROC, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }
    exi_compress_variable(exoid, varid, 1);

    if ((status = nc_def_var(exoid, VAR_E_COMM_SIDS, NC_INT, 1, dimid, &varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to add variable \"%s\" in file ID %d",
               VAR_E_COMM_SIDS, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }
    exi_compress_variable(exoid, varid, 1);

  } /* End "if (num_e_comm_maps > 0)" */

  /* Exit define mode */
  exi_leavedef(exoid, __func__);

  /* Set the status of the nodal communication maps */
  if (num_n_comm_maps > 0) {

    int n_varid;
    if ((status = nc_inq_varid(exoid, VAR_N_COMM_STAT, &n_varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find variable ID for \"%s\" in file ID %d",
               VAR_N_COMM_STAT, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    int64_t nl_ncnt_cmap = 0; /* reset this for index */
    for (size_t icm = 0; icm < num_n_comm_maps; icm++) {

      size_t ncnc;
      if (ex_int64_status(exoid) & EX_BULK_INT64_API) {
        ncnc = ((int64_t *)node_cmap_node_cnts)[icm];
      }
      else {
        ncnc = ((int *)node_cmap_node_cnts)[icm];
      }

      int nmstat = ncnc > 0 ? 1 : 0;
      start[0]   = icm;

      if ((status = nc_put_var1_int(exoid, n_varid, start, &nmstat)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: unable to output variable in file ID %d", exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }

      /* increment to the next starting position */
      nl_ncnt_cmap += ncnc;

      /* fill the cmap data index */
      if ((status = nc_put_var1_longlong(exoid, n_varid_idx, start, (long long *)&nl_ncnt_cmap)) !=
          NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to output int elem map index in file ID %d",
                 exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }
    } /* End "for(icm=0; icm < num_n_comm_maps; icm++)" */

    /* Get the variable ID for the comm map IDs vector */
    if ((status = nc_inq_varid(exoid, VAR_N_COMM_IDS, &n_varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find variable ID for \"%s\" in file ID %d",
               VAR_N_COMM_IDS, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* Output the nodal comm map IDs */
    if (ex_int64_status(exoid) & EX_IDS_INT64_API) {
      status = nc_put_var_longlong(exoid, n_varid, node_cmap_ids);
    }
    else {
      status = nc_put_var_int(exoid, n_varid, node_cmap_ids);
    }
    if (status != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to output variable in file ID %d", exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

  } /* End "if (num_n_comm_maps > 0)" */

  /* Set the status of the elemental communication maps */
  if (num_e_comm_maps > 0) {

    /* Get variable ID for elemental status vector */
    int e_varid;
    if ((status = nc_inq_varid(exoid, VAR_E_COMM_STAT, &e_varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find variable ID for \"%s\" in file ID %d",
               VAR_E_COMM_STAT, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    int64_t nl_ecnt_cmap = 0; /* reset this for index */
    for (size_t icm = 0; icm < num_e_comm_maps; icm++) {
      size_t ecec;
      if (ex_int64_status(exoid) & EX_BULK_INT64_API) {
        ecec = ((int64_t *)elem_cmap_elem_cnts)[icm];
      }
      else {
        ecec = ((int *)elem_cmap_elem_cnts)[icm];
      }
      start[0]   = icm;
      int nmstat = ecec > 0 ? 1 : 0;

      if ((status = nc_put_var1_int(exoid, e_varid, start, &nmstat)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: unable to output variable in file ID %d", exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }

      /* increment to the next starting position */
      nl_ecnt_cmap += ecec;

      /* fill the cmap data index */
      if ((status = nc_put_var1_longlong(exoid, e_varid_idx, start, (long long *)&nl_ecnt_cmap)) !=
          NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to output int elem map index in file ID %d",
                 exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }
    } /* End "for(icm=0; icm < num_e_comm_maps; icm++)" */

    /* Get the variable ID for the elemental comm map IDs vector */
    if ((status = nc_inq_varid(exoid, VAR_E_COMM_IDS, &e_varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find variable ID for \"%s\" in file ID %d",
               VAR_E_COMM_IDS, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* Output the elemental comm map IDs */
    if (ex_int64_status(exoid) & EX_IDS_INT64_API) {
      status = nc_put_var_longlong(exoid, e_varid, elem_cmap_ids);
    }
    else {
      status = nc_put_var_int(exoid, e_varid, elem_cmap_ids);
    }
    if (status != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to output variable in file ID %d", exoid);
      ex_err_fn(exoid, __func__, errmsg, status);

      EX_FUNC_LEAVE(EX_FATAL);
    }

  } /* End "if (num_e_comm_maps > 0)" */

  EX_FUNC_LEAVE(EX_NOERR);
}
