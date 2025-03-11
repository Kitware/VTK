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
 *     ex_put_cmap_params_cc()
 *****************************************************************************
 * This function outputs the concatenated list of communication map
 * parameters.
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
 *      proc_ids            - The processor the file being read was written
 *                            for.
 */
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
#include <exodusII.h>     // for ex_err, etc
#include <exodusII_int.h> // for EX_FATAL, exi_leavedef, etc

int ex_put_cmap_params_cc(int exoid, const void_int *node_cmap_ids,
                          const void_int *node_cmap_node_cnts, const void_int *node_proc_ptrs,
                          const void_int *elem_cmap_ids, const void_int *elem_cmap_elem_cnts,
                          const void_int *elem_proc_ptrs)
{
  size_t num_n_comm_maps, num_e_comm_maps;
  int    status, n_varid[2], e_varid[2];
  int    varid, n_dimid[1], e_dimid[1];
  int    n_varid_idx, e_varid_idx;
  int    num_icm;
  size_t start[1], count[1];
  size_t iproc;

  long long nl_ecnt_cmap, nl_ncnt_cmap;
  void_int *n_var_idx = NULL;
  void_int *e_var_idx = NULL;

  int nmstat;

  char errmsg[MAX_ERR_LENGTH];
  int  index_type, bulk_type;
  /*-----------------------------Execution begins-----------------------------*/

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* See if using NC_FORMAT_NETCDF4 format... */
  int format;
  nc_inq_format(exoid, &format);
  if ((ex_int64_status(exoid) & EX_BULK_INT64_DB) || (format == NC_FORMAT_NETCDF4)) {
    index_type = NC_INT64;
  }
  else {
    index_type = NC_INT;
  }
  if (ex_int64_status(exoid) & EX_BULK_INT64_DB) {
    bulk_type = NC_INT64;
  }
  else {
    bulk_type = NC_INT;
  }

  /* Get the number of processors in the file */
  if ((status = nc_inq_dimid(exoid, DIM_NUM_PROCS_F, &n_dimid[0])) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get dimension ID for \"%s\" in file ID %d",
             DIM_NUM_PROCS_F, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  size_t num_procs_in_file;
  if ((status = nc_inq_dimlen(exoid, n_dimid[0], &num_procs_in_file)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to find length of dimension \"%s\" in file ID %d", DIM_NUM_PROCS_F,
             exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /*
   * since I cannot get variables while in define mode, I need to
   * get the cmap information index variables before I go into
   * define mode
   */

  /* Check to see if there are nodal communications maps in the file */
  if (nc_inq_dimid(exoid, DIM_NUM_N_CMAPS, &n_dimid[0]) != NC_NOERR) {
    num_n_comm_maps = 0;
  }
  else {
    if ((status = nc_inq_dimlen(exoid, n_dimid[0], &num_n_comm_maps)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find length of dimension \"%s\" in \
file ID %d",
               DIM_NUM_N_CMAPS, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }

  if (num_n_comm_maps > 0) {
    /* Get the variable ID for the comm map index vector */
    if ((status = nc_inq_varid(exoid, VAR_N_COMM_INFO_IDX, &n_varid_idx)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find variable ID for \"%s\" in file ID %d",
               VAR_N_COMM_INFO_IDX, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* allocate space for the index variable */
    if (index_type == NC_INT64) {
      n_var_idx = malloc((num_procs_in_file + 1) * sizeof(long long));
    }
    else {
      n_var_idx = malloc((num_procs_in_file + 1) * sizeof(int));
    }
    if (!n_var_idx) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: insufficient memory to read index variable from file ID %d", exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_MSG);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* and set the last value of the index */

    /* get the communication map info index */
    if (index_type == NC_INT64) {
      ((long long *)n_var_idx)[0] = 0;
      status = nc_get_var_longlong(exoid, n_varid_idx, &((long long *)n_var_idx)[1]);
    }
    else {
      ((int *)n_var_idx)[0] = 0;
      status                = nc_get_var_int(exoid, n_varid_idx, &((int *)n_var_idx)[1]);
    }
    if (status != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get variable \"%s\" from file ID %d",
               VAR_N_COMM_INFO_IDX, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      free(n_var_idx);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  } /* "if (num_n_comm_maps > 0)" */

  /* Check to see if there are elemental communications maps in the file */
  if (nc_inq_dimid(exoid, DIM_NUM_E_CMAPS, &e_dimid[0]) != NC_NOERR) {
    num_e_comm_maps = 0;
  }
  else {
    if ((status = nc_inq_dimlen(exoid, e_dimid[0], &num_e_comm_maps)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find length of dimension \"%s\" in \
file ID %d",
               DIM_NUM_E_CMAPS, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }

  if (num_e_comm_maps > 0) {
    /* Get the variable ID for the comm map index vector */
    if ((status = nc_inq_varid(exoid, VAR_E_COMM_INFO_IDX, &e_varid_idx)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find variable ID for \"%s\" in file ID %d",
               VAR_E_COMM_INFO_IDX, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* allocate space for the index variable */
    if (index_type == NC_INT64) {
      e_var_idx = malloc((num_procs_in_file + 1) * sizeof(long long));
    }
    else {
      e_var_idx = malloc((num_procs_in_file + 1) * sizeof(int));
    }
    if (!e_var_idx) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: insufficient memory to read index variable from file ID %d", exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_MEMFAIL);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* get the communication map info index */
    if (index_type == NC_INT64) {
      ((long long *)e_var_idx)[0] = 0;
      status = nc_get_var_longlong(exoid, e_varid_idx, &((long long *)e_var_idx)[1]);
    }
    else {
      ((int *)e_var_idx)[0] = 0;
      status                = nc_get_var_int(exoid, e_varid_idx, &((int *)e_var_idx)[1]);
    }
    if (status != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get variable \"%s\" from file ID %d",
               VAR_E_COMM_INFO_IDX, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      free(e_var_idx);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  } /* "if (num_e_comm_maps >0)" */

  /* Put NetCDF file into define mode */
  if ((status = exi_redef(exoid, __func__)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to put file ID %d into define mode", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /*
   * Add dimensions for the size of the number of nodal
   * communication maps.
   */
  if (num_n_comm_maps > 0) {
    /* add the communications data index variable */
    if ((status = nc_def_var(exoid, VAR_N_COMM_DATA_IDX, index_type, 1, n_dimid, &n_varid_idx)) !=
        NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to add variable \"%s\" in file ID %d",
               VAR_N_COMM_DATA_IDX, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* now add up all of the nodal communications maps */
    size_t ncnt_cmap = 0;
    for (iproc = 0; iproc < num_procs_in_file; iproc++) {
      if (index_type == NC_INT64) {
        num_icm = ((int64_t *)n_var_idx)[iproc + 1] - ((int64_t *)n_var_idx)[iproc];
      }
      else {
        num_icm = ((int *)n_var_idx)[iproc + 1] - ((int *)n_var_idx)[iproc];
      }
      for (int icm = 0; icm < num_icm; icm++) {
        if (ex_int64_status(exoid) & EX_BULK_INT64_API) {
          ncnt_cmap += ((int64_t *)node_cmap_node_cnts)[((int64_t *)node_proc_ptrs)[iproc] + icm];
        }
        else {
          ncnt_cmap += ((int *)node_cmap_node_cnts)[((int *)node_proc_ptrs)[iproc] + icm];
        }
      }
    }

    if ((status = nc_def_dim(exoid, DIM_NCNT_CMAP, ncnt_cmap, &n_dimid[0])) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to add dimension for \"%s\" of size %zu in file ID %d", DIM_NCNT_CMAP,
               ncnt_cmap, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* Define variables for the nodal IDS and processor vectors */
    if ((status = nc_def_var(exoid, VAR_N_COMM_NIDS, bulk_type, 1, n_dimid, &varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to add variable \"%s\" in file ID %d",
               VAR_N_COMM_NIDS, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }
    exi_compress_variable(exoid, varid, 1);

    if ((status = nc_def_var(exoid, VAR_N_COMM_PROC, NC_INT, 1, n_dimid, &varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to add variable \"%s\" in file ID %d",
               VAR_N_COMM_PROC, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }
    exi_compress_variable(exoid, varid, 1);

  } /* End "if (num_n_comm_maps > 0)" */

  /*
   * Add dimensions for the size of the number of elemental
   * communication maps.
   */
  if (num_e_comm_maps > 0) {
    /* add the communications data index variable */
    if ((status = nc_def_var(exoid, VAR_E_COMM_DATA_IDX, index_type, 1, e_dimid, &e_varid_idx)) !=
        NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to add variable \"%s\" in file ID %d",
               VAR_E_COMM_DATA_IDX, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* now add up all of the nodal communications maps */
    size_t ecnt_cmap = 0;
    for (iproc = 0; iproc < num_procs_in_file; iproc++) {
      if (index_type == NC_INT64) {
        num_icm = ((int64_t *)e_var_idx)[iproc + 1] - ((int64_t *)e_var_idx)[iproc];
      }
      else {
        num_icm = ((int *)e_var_idx)[iproc + 1] - ((int *)e_var_idx)[iproc];
      }
      for (int icm = 0; icm < num_icm; icm++) {
        if (ex_int64_status(exoid) & EX_BULK_INT64_API) {
          ecnt_cmap += ((int64_t *)elem_cmap_elem_cnts)[((int64_t *)elem_proc_ptrs)[iproc] + icm];
        }
        else {
          ecnt_cmap += ((int *)elem_cmap_elem_cnts)[((int *)elem_proc_ptrs)[iproc] + icm];
        }
      }
    }

    /* Add dimensions for elemental communications maps */
    if ((status = nc_def_dim(exoid, DIM_ECNT_CMAP, ecnt_cmap, &e_dimid[0])) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to add dimension for \"%s\" in file ID %d",
               DIM_ECNT_CMAP, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* Define variables for the element IDS and processor vectors */
    if ((status = nc_def_var(exoid, VAR_E_COMM_EIDS, bulk_type, 1, e_dimid, &varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to add variable \"%s\" in file ID %d",
               VAR_E_COMM_EIDS, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }
    exi_compress_variable(exoid, varid, 1);

    if ((status = nc_def_var(exoid, VAR_E_COMM_PROC, NC_INT, 1, e_dimid, &varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to add variable \"%s\" in file ID %d",
               VAR_E_COMM_PROC, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }
    exi_compress_variable(exoid, varid, 1);

    if ((status = nc_def_var(exoid, VAR_E_COMM_SIDS, bulk_type, 1, e_dimid, &varid)) != NC_NOERR) {
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

    /* need to get the two "n_comm_*" variable ids */

    if ((status = nc_inq_varid(exoid, VAR_N_COMM_STAT, &n_varid[0])) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find variable ID for \"%s\" in file ID %d",
               VAR_N_COMM_STAT, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* Get the variable ID for the comm map IDs vector */
    if ((status = nc_inq_varid(exoid, VAR_N_COMM_IDS, &n_varid[1])) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find variable ID for \"%s\" in file ID %d",
               VAR_N_COMM_IDS, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* reset the index variable */
    nl_ncnt_cmap = 0;

    for (iproc = 0; iproc < num_procs_in_file; iproc++) {
      size_t proc_ptr;
      if (ex_int64_status(exoid) & EX_BULK_INT64_API) {
        proc_ptr = ((int64_t *)node_proc_ptrs)[iproc];
      }
      else {
        proc_ptr = ((int *)node_proc_ptrs)[iproc];
      }

      if (index_type == NC_INT64) {
        num_icm = ((int64_t *)n_var_idx)[iproc + 1] - ((int64_t *)n_var_idx)[iproc];
      }
      else {
        num_icm = ((int *)n_var_idx)[iproc + 1] - ((int *)n_var_idx)[iproc];
      }
      for (int icm = 0; icm < num_icm; icm++) {
        size_t cnt;
        if (ex_int64_status(exoid) & EX_BULK_INT64_API) {
          cnt = ((int64_t *)node_cmap_node_cnts)[proc_ptr + icm];
        }
        else {
          cnt = ((int *)node_cmap_node_cnts)[proc_ptr + icm];
        }

        if (index_type == NC_INT64) {
          start[0] = ((int64_t *)n_var_idx)[iproc] + icm;
        }
        else {
          start[0] = ((int *)n_var_idx)[iproc] + icm;
        }
        if (cnt > 0) {
          nmstat = 1;
        }
        else {
          nmstat = 0;
        }

        if ((status = nc_put_var1_int(exoid, n_varid[0], start, &nmstat)) != NC_NOERR) {
          snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: unable to output variable in file ID %d", exoid);
          ex_err_fn(exoid, __func__, errmsg, status);
          EX_FUNC_LEAVE(EX_FATAL);
        }

        /* increment to the next starting position */
        nl_ncnt_cmap += cnt;

        /* fill the data index variable */
        status = nc_put_var1_longlong(exoid, n_varid_idx, start, &nl_ncnt_cmap);

        if (status != NC_NOERR) {
          snprintf(errmsg, MAX_ERR_LENGTH,
                   "ERROR: failed to output int elem map index in file ID %d", exoid);
          ex_err_fn(exoid, __func__, errmsg, status);
          EX_FUNC_LEAVE(EX_FATAL);
        }
      } /* End "for(icm=0; icm < num_icm; icm++)" */

      if (num_icm > 0) {
        /* Output the nodal comm map IDs */
        if (index_type == NC_INT64) {
          start[0] = ((int64_t *)n_var_idx)[iproc];
        }
        else {
          start[0] = ((int *)n_var_idx)[iproc];
        }
        count[0] = num_icm;
        if (ex_int64_status(exoid) & EX_IDS_INT64_API) {
          status = nc_put_vara_longlong(exoid, n_varid[1], start, count,
                                        &((long long *)node_cmap_ids)[proc_ptr]);
        }
        else {
          status =
              nc_put_vara_int(exoid, n_varid[1], start, count, &((int *)node_cmap_ids)[proc_ptr]);
        }
        if (status != NC_NOERR) {
          snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to output variable in file ID %d", exoid);
          ex_err_fn(exoid, __func__, errmsg, status);

          EX_FUNC_LEAVE(EX_FATAL);
        }
      }
    } /* End "for(iproc=0; iproc < num_procs_in_file; iproc++)" */

    /* free up memory for index */
    free(n_var_idx);

  } /* End "if (num_n_comm_maps > 0)" */

  /* Set the status of the elemental communication maps */
  if (num_e_comm_maps > 0) {

    /* need to get the two "e_comm_*" variables" */

    /* Get variable ID for elemental status vector */
    if ((status = nc_inq_varid(exoid, VAR_E_COMM_STAT, &e_varid[0])) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find variable ID for \"%s\" in file ID %d",
               VAR_E_COMM_STAT, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* Get the variable ID for the elemental comm map IDs vector */
    if ((status = nc_inq_varid(exoid, VAR_E_COMM_IDS, &e_varid[1])) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find variable ID for \"%s\" in file ID %d",
               VAR_E_COMM_IDS, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* reset the index variable */
    nl_ecnt_cmap = 0;

    for (iproc = 0; iproc < num_procs_in_file; iproc++) {
      size_t proc_ptr;
      if (ex_int64_status(exoid) & EX_BULK_INT64_API) {
        proc_ptr = ((int64_t *)elem_proc_ptrs)[iproc];
      }
      else {
        proc_ptr = ((int *)elem_proc_ptrs)[iproc];
      }
      if (index_type == NC_INT64) {
        num_icm = ((int64_t *)e_var_idx)[iproc + 1] - ((int64_t *)e_var_idx)[iproc];
      }
      else {
        num_icm = ((int *)e_var_idx)[iproc + 1] - ((int *)e_var_idx)[iproc];
      }
      for (int icm = 0; icm < num_icm; icm++) {

        size_t cnt;
        if (ex_int64_status(exoid) & EX_BULK_INT64_API) {
          cnt = ((int64_t *)elem_cmap_elem_cnts)[proc_ptr + icm];
        }
        else {
          cnt = ((int *)elem_cmap_elem_cnts)[proc_ptr + icm];
        }

        if (index_type == NC_INT64) {
          start[0] = ((int64_t *)e_var_idx)[iproc] + icm;
        }
        else {
          start[0] = ((int *)e_var_idx)[iproc] + icm;
        }
        if (cnt > 0) {
          nmstat = 1;
        }
        else {
          nmstat = 0;
        }

        if ((status = nc_put_var1_int(exoid, e_varid[0], start, &nmstat)) != NC_NOERR) {
          snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: unable to output variable in file ID %d", exoid);
          ex_err_fn(exoid, __func__, errmsg, status);
          EX_FUNC_LEAVE(EX_FATAL);
        }

        /* increment to the next starting position */
        nl_ecnt_cmap += cnt;

        /* fill the data index variable */
        status = nc_put_var1_longlong(exoid, e_varid_idx, start, &nl_ecnt_cmap);

        if (status != NC_NOERR) {
          snprintf(errmsg, MAX_ERR_LENGTH,
                   "ERROR: failed to output int elem map index in file ID %d", exoid);
          ex_err_fn(exoid, __func__, errmsg, status);
          EX_FUNC_LEAVE(EX_FATAL);
        }
      } /* End "for(icm=0; icm < num_icm; icm++)" */

      if (num_icm > 0) {
        /* Output the elemental comm map IDs */
        if (index_type == NC_INT64) {
          start[0] = ((int64_t *)e_var_idx)[iproc];
        }
        else {
          start[0] = ((int *)e_var_idx)[iproc];
        }
        count[0] = num_icm;
        if (ex_int64_status(exoid) & EX_IDS_INT64_API) {
          status = nc_put_vara_longlong(exoid, e_varid[1], start, count,
                                        &((long long *)elem_cmap_ids)[proc_ptr]);
        }
        else {
          status =
              nc_put_vara_int(exoid, e_varid[1], start, count, &((int *)elem_cmap_ids)[proc_ptr]);
        }
        if (status != NC_NOERR) {
          snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to output variable in file ID %d", exoid);
          ex_err_fn(exoid, __func__, errmsg, status);
          EX_FUNC_LEAVE(EX_FATAL);
        }
      }
    } /* End "for(iproc=0; iproc < num_procs_in_file; iproc++)" */

    free(e_var_idx);

  } /* End "if (num_e_comm_maps > 0)" */

  EX_FUNC_LEAVE(EX_NOERR);
}
