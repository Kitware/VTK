/*
 * Copyright(C) 1999-2020, 2022 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/* Function(s) contained in this file:
 *      ex_put_loadbal_param_cc()
 *****************************************************************************
 * This function outputs the concatenated list of load-balance parameters.
 *****************************************************************************
 *  Variable Index:
 *      exoid             - The NetCDF ID of an already open NemesisI file.
 *      num_int_nodes    - Vector of number of internal FEM nodes for
 *                         "num_proc_in_f" processors.
 *      num_bor_nodes    - Vector of number of border FEM nodes for
 *                         "num_proc_in_f" processors.
 *      num_ext_nodes    - Vector of number of external FEM nodes for
 *                         "num_proc_in_f" processors.
 *      num_int_elems    - Vector of number of internal FEM elems for
 *                         "num_proc_in_f" processors.
 *      num_bor_elems    - Vector of number of border FEM elems for
 *                         "num_proc_in_f" processors.
 *      num_node_cmaps   - Vector of number of nodal communication maps
 *                         for "num_proc_in_f" processors.
 *      num_elem_cmaps   - Vector of number of elemental communication maps
 *                         for "num_proc_in_f" processors.
 */
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

#include <exodusII.h>     // for ex_err, etc
#include <exodusII_int.h> // for exi_leavedef, EX_FATAL, etc

#ifndef NC_INT64
#define NC_INT64 NC_INT
#endif

/*!
 * \ingroup ModelDescription
 * \undoc
 */
int ex_put_loadbal_param_cc(int exoid, const void_int *num_int_nodes, const void_int *num_bor_nodes,
                            const void_int *num_ext_nodes, const void_int *num_int_elems,
                            const void_int *num_bor_elems, const void_int *num_node_cmaps,
                            const void_int *num_elem_cmaps)
{
  int    status;
  int    iproc, varid, dimid_npf, dimid[3];
  int    num_proc, num_proc_in_f;
  int    varid_nm[3], varid_em[2];
  int    varid_idx[7] = {0, 0, 0, 0, 0, 0, 0};
  size_t start[1];
  char   ftype[2];
  int    oldfill;

#if NC_HAS_HDF5
  int64_t num_int_elem = 0, num_int_node = 0, num_bor_elem = 0;
  int64_t num_bor_node = 0, num_ext_node = 0;
  int64_t num_n_cmaps = 0, num_e_cmaps = 0;
#else
  int num_int_elem = 0, num_int_node = 0, num_bor_elem = 0;
  int num_bor_node = 0, num_ext_node = 0;
  int num_n_cmaps = 0, num_e_cmaps = 0;
#endif

  int nmstat;

  char errmsg[MAX_ERR_LENGTH];

  int format;

  int index_type = NC_INT;
  int map_type   = NC_INT;
  int id_type    = NC_INT;

  /*-----------------------------Execution begins-----------------------------*/
  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (ex_int64_status(exoid) & EX_MAPS_INT64_DB) {
    map_type = NC_INT64;
  }
  if (ex_int64_status(exoid) & EX_IDS_INT64_DB) {
    id_type = NC_INT64;
  }

  /* See if using NC_FORMAT_NETCDF4 format... */
  nc_inq_format(exoid, &format);
  if ((ex_int64_status(exoid) & EX_BULK_INT64_DB) || (format == NC_FORMAT_NETCDF4)) {
    index_type = NC_INT64;
  }

  /* Get the processor information from the file */
  if (ex_get_init_info(exoid, &num_proc, &num_proc_in_f, ftype) != EX_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: Unable to get processor info from file ID %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_MSG);

    EX_FUNC_LEAVE(EX_FATAL);
  }

  /*
   * Get the dimension ID for the number of processors storing
   * information in this file.
   */
  if ((status = nc_inq_dimid(exoid, DIM_NUM_PROCS_F, &dimid_npf)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find dimension ID for \"%s\" in file ID %d",
             DIM_NUM_PROCS_F, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);

    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Put NetCDF file into define mode */
  if ((status = exi_redef(exoid, __func__)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to put file id %d into define mode", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Set the fill mode */
  if ((status = nc_set_fill(exoid, NC_NOFILL, &oldfill)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to put file ID %d into no-fill mode", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Output the file version */
  if ((status = exi_put_nemesis_version(exoid)) < 0) {
    EX_FUNC_LEAVE(status);
  }

  /* Output the file type */
  if (nc_inq_varid(exoid, VAR_FILE_TYPE, &varid) != NC_NOERR) {
    if ((status = nc_def_var(exoid, VAR_FILE_TYPE, NC_INT, 0, NULL, &varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define file type in file ID %d", exoid);
      ex_err_fn(exoid, __func__, errmsg, status);

      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }
  }

  /* Define the status variables for the nodal vectors */
  if (nc_inq_varid(exoid, VAR_INT_N_STAT, &varid) != NC_NOERR) {
    if ((status = nc_def_var(exoid, VAR_INT_N_STAT, NC_INT, 1, &dimid_npf, &varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define variable \"%s\" in file ID %d",
               VAR_INT_N_STAT, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }
  }

  /* Set the dimension for status vectors */
  if (nc_inq_varid(exoid, VAR_BOR_N_STAT, &varid) != NC_NOERR) {
    if ((status = nc_def_var(exoid, VAR_BOR_N_STAT, NC_INT, 1, &dimid_npf, &varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define variable \"%s\" in file ID %d",
               VAR_BOR_N_STAT, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }
  }

  if (nc_inq_varid(exoid, VAR_EXT_N_STAT, &varid) != NC_NOERR) {
    if ((status = nc_def_var(exoid, VAR_EXT_N_STAT, NC_INT, 1, &dimid_npf, &varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define variable \"%s\" in file ID %d",
               VAR_EXT_N_STAT, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }
  }

  /* Define the variable IDs for the elemental status vectors */
  if (nc_inq_varid(exoid, VAR_INT_E_STAT, &varid) != NC_NOERR) {
    if ((status = nc_def_var(exoid, VAR_INT_E_STAT, NC_INT, 1, &dimid_npf, &varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define variable \"%s\" in file ID %d",
               VAR_INT_E_STAT, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }
  }

  if (nc_inq_varid(exoid, VAR_BOR_E_STAT, &varid) != NC_NOERR) {
    if ((status = nc_def_var(exoid, VAR_BOR_E_STAT, NC_INT, 1, &dimid_npf, &varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: Failed to define variable \"%s\" in file ID %d",
               VAR_BOR_E_STAT, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }
  }

  /* Get the variable ID for the nodal status vectors */
  if ((status = nc_inq_varid(exoid, VAR_INT_N_STAT, &varid_nm[0])) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find variable ID for \"%s\" in file ID %d",
             VAR_INT_N_STAT, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    /* Leave define mode before returning */
    exi_leavedef(exoid, __func__);

    EX_FUNC_LEAVE(EX_FATAL);
  }

  if ((status = nc_inq_varid(exoid, VAR_BOR_N_STAT, &varid_nm[1])) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find variable ID for \"%s\" in file ID %d",
             VAR_BOR_N_STAT, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    /* Leave define mode before returning */
    exi_leavedef(exoid, __func__);

    EX_FUNC_LEAVE(EX_FATAL);
  }

  if ((status = nc_inq_varid(exoid, VAR_EXT_N_STAT, &varid_nm[2])) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find variable ID for \"%s\" in file ID %d",
             VAR_EXT_N_STAT, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    /* Leave define mode before returning */
    exi_leavedef(exoid, __func__);

    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Get the variable IDs for the elemental status vectors */
  if ((status = nc_inq_varid(exoid, VAR_INT_E_STAT, &varid_em[0])) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find variable ID for \"%s\" in file ID %d",
             VAR_INT_E_STAT, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    /* Leave define mode before returning */
    exi_leavedef(exoid, __func__);

    EX_FUNC_LEAVE(EX_FATAL);
  }

  if ((status = nc_inq_varid(exoid, VAR_BOR_E_STAT, &varid_em[1])) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find variable ID for \"%s\" in file ID %d",
             VAR_BOR_E_STAT, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    /* Leave define mode before returning */
    exi_leavedef(exoid, __func__);

    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* mms: NEW STUFF HERE */
  /*
  ** first need to loop through the processors in this
  ** file and get the counts of the element and cmap lists
  */
  if (ex_int64_status(exoid) & EX_BULK_INT64_API) {
    for (iproc = 0; iproc < num_proc_in_f; iproc++) {
      num_int_elem += ((int64_t *)num_int_elems)[iproc];
      num_int_node += ((int64_t *)num_int_nodes)[iproc];
      num_bor_elem += ((int64_t *)num_bor_elems)[iproc];
      num_bor_node += ((int64_t *)num_bor_nodes)[iproc];
      num_ext_node += ((int64_t *)num_ext_nodes)[iproc];
      num_e_cmaps += ((int64_t *)num_elem_cmaps)[iproc];
      num_n_cmaps += ((int64_t *)num_node_cmaps)[iproc];
    }
  }
  else {
    for (iproc = 0; iproc < num_proc_in_f; iproc++) {
      num_int_elem += ((int *)num_int_elems)[iproc];
      num_int_node += ((int *)num_int_nodes)[iproc];
      num_bor_elem += ((int *)num_bor_elems)[iproc];
      num_bor_node += ((int *)num_bor_nodes)[iproc];
      num_ext_node += ((int *)num_ext_nodes)[iproc];
      num_e_cmaps += ((int *)num_elem_cmaps)[iproc];
      num_n_cmaps += ((int *)num_node_cmaps)[iproc];
    }
  }

  /* Define variable for the internal element information */
  if (num_int_elem > 0) {
    if ((status = nc_def_dim(exoid, DIM_NUM_INT_ELEMS, num_int_elem, &dimid[0])) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to dimension \"%s\" in file id %d",
               DIM_NUM_INT_ELEMS, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }

    if ((status = nc_def_var(exoid, VAR_ELEM_MAP_INT, map_type, 1, dimid, &varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define variable \"%s\" in file ID %d",
               VAR_ELEM_MAP_INT, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }
    exi_compress_variable(exoid, varid, 1);

    /* and the index variable */
    if ((status = nc_def_var(exoid, VAR_ELEM_MAP_INT_IDX, index_type, 1, &dimid_npf,
                             &varid_idx[0])) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define variable \"%s\" in file ID %d",
               VAR_ELEM_MAP_INT_IDX, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }
  } /* End "if (num_int_elem > 0)" */

  /* Define variable for the border element information */
  if (num_bor_elem > 0) {
    if ((status = nc_def_dim(exoid, DIM_NUM_BOR_ELEMS, num_bor_elem, &dimid[0])) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to dimension \"%s\" in file id %d",
               DIM_NUM_BOR_ELEMS, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }

    if ((status = nc_def_var(exoid, VAR_ELEM_MAP_BOR, map_type, 1, dimid, &varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define variable \"%s\" in file ID %d",
               VAR_ELEM_MAP_BOR, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }
    exi_compress_variable(exoid, varid, 1);

    /* and the index variable */
    if ((status = nc_def_var(exoid, VAR_ELEM_MAP_BOR_IDX, index_type, 1, &dimid_npf,
                             &varid_idx[1])) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define variable \"%s\" in file ID %d",
               VAR_ELEM_MAP_BOR_IDX, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }

  } /* End "if (num_bor_elem > 0)" */

  if (num_int_node > 0) {
    /* Define variable for vector of internal FEM node IDs */
    if ((status = nc_def_dim(exoid, DIM_NUM_INT_NODES, num_int_node, &dimid[0])) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to dimension \"%s\" in file id %d",
               DIM_NUM_INT_NODES, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }

    if ((status = nc_def_var(exoid, VAR_NODE_MAP_INT, map_type, 1, &dimid[0], &varid)) !=
        NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define variable \"%s\" in file ID %d",
               VAR_NODE_MAP_INT, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }
    exi_compress_variable(exoid, varid, 1);

    /* and the index variable */
    if ((status = nc_def_var(exoid, VAR_NODE_MAP_INT_IDX, index_type, 1, &dimid_npf,
                             &varid_idx[2])) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define variable \"%s\" in file ID %d",
               VAR_NODE_MAP_INT_IDX, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }

  } /* End "if (num_int_node > 0)" */

  if (num_bor_node > 0) {
    /* Define variable for vector of border FEM node IDs */
    if ((status = nc_def_dim(exoid, DIM_NUM_BOR_NODES, num_bor_node, &dimid[1])) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to dimension \"%s\" in file id %d",
               DIM_NUM_BOR_NODES, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }

    if ((status = nc_def_var(exoid, VAR_NODE_MAP_BOR, map_type, 1, &dimid[1], &varid)) !=
        NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define variable \"%s\" in file ID %d",
               VAR_NODE_MAP_BOR, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }
    exi_compress_variable(exoid, varid, 1);

    /* and the index variable */
    if ((status = nc_def_var(exoid, VAR_NODE_MAP_BOR_IDX, index_type, 1, &dimid_npf,
                             &varid_idx[3])) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define variable \"%s\" in file ID %d",
               VAR_NODE_MAP_BOR_IDX, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }

  } /* End "if (num_bor_node > 0)" */

  if (num_ext_node > 0) {
    /* Define dimension for vector of external FEM node IDs */
    if ((status = nc_def_dim(exoid, DIM_NUM_EXT_NODES, num_ext_node, &dimid[2])) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to dimension \"%s\" in file id %d",
               DIM_NUM_EXT_NODES, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }

    if ((status = nc_def_var(exoid, VAR_NODE_MAP_EXT, map_type, 1, &dimid[2], &varid)) !=
        NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define variable \"%s\" in file ID %d",
               VAR_NODE_MAP_EXT, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }
    exi_compress_variable(exoid, varid, 1);

    /* and the index variable */
    if ((status = nc_def_var(exoid, VAR_NODE_MAP_EXT_IDX, index_type, 1, &dimid_npf,
                             &varid_idx[4])) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define variable \"%s\" in file ID %d",
               VAR_NODE_MAP_EXT_IDX, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }

  } /* End "if (num_ext_node > 0)" */

  /* Output the communication map dimensions */
  if (num_n_cmaps > 0) {
    if ((status = nc_def_dim(exoid, DIM_NUM_N_CMAPS, num_n_cmaps, &dimid[0])) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to add dimension \"%s\" in file ID %d",
               DIM_NUM_N_CMAPS, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* Add variables for communication maps */
    if ((status = nc_def_var(exoid, VAR_N_COMM_IDS, id_type, 1, dimid, &varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define variable \"%s\" in file ID %d",
               VAR_N_COMM_IDS, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }

    if ((status = nc_def_var(exoid, VAR_N_COMM_STAT, NC_INT, 1, dimid, &varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define variable \"%s\" in file ID %d",
               VAR_N_COMM_STAT, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* and the index variable */
    if ((status = nc_def_var(exoid, VAR_N_COMM_INFO_IDX, index_type, 1, &dimid_npf,
                             &varid_idx[5])) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define variable \"%s\" in file ID %d",
               VAR_N_COMM_INFO_IDX, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }

  } /* End "if (num_n_cmaps > 0)" */

  if (num_e_cmaps > 0) {
    if ((status = nc_def_dim(exoid, DIM_NUM_E_CMAPS, num_e_cmaps, &dimid[0])) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to add dimension \"%s\" in file ID %d",
               DIM_NUM_E_CMAPS, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* Add variables for elemental communication maps */
    if ((status = nc_def_var(exoid, VAR_E_COMM_IDS, id_type, 1, dimid, &varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define variable \"%s\" in file ID %d",
               VAR_E_COMM_IDS, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }

    if ((status = nc_def_var(exoid, VAR_E_COMM_STAT, NC_INT, 1, dimid, &varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define variable \"%s\" in file ID %d",
               VAR_E_COMM_STAT, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* and the index variable */
    if ((status = nc_def_var(exoid, VAR_E_COMM_INFO_IDX, index_type, 1, &dimid_npf,
                             &varid_idx[6])) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define variable \"%s\" in file ID %d",
               VAR_E_COMM_INFO_IDX, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }

  } /* End "if (num_e_cmaps > 0)" */

  /* Leave define mode */
  if (exi_leavedef(exoid, __func__) != EX_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* need to reset these counters */
  num_int_elem = 0;
  num_int_node = 0;
  num_bor_elem = 0;
  num_bor_node = 0;
  num_ext_node = 0;
  num_n_cmaps  = 0;
  num_e_cmaps  = 0;

  /* Update the status vectors */
  for (iproc = 0; iproc < num_proc_in_f; iproc++) {
    size_t nin;
    size_t nbn;
    size_t nen;
    size_t nie;
    size_t nbe;
    size_t nnc;
    size_t nec;
    if (ex_int64_status(exoid) & EX_BULK_INT64_API) {
      nie = ((int64_t *)num_int_elems)[iproc];
      nin = ((int64_t *)num_int_nodes)[iproc];
      nbe = ((int64_t *)num_bor_elems)[iproc];
      nbn = ((int64_t *)num_bor_nodes)[iproc];
      nen = ((int64_t *)num_ext_nodes)[iproc];
      nec = ((int64_t *)num_elem_cmaps)[iproc];
      nnc = ((int64_t *)num_node_cmaps)[iproc];
    }
    else {
      nie = ((int *)num_int_elems)[iproc];
      nin = ((int *)num_int_nodes)[iproc];
      nbe = ((int *)num_bor_elems)[iproc];
      nbn = ((int *)num_bor_nodes)[iproc];
      nen = ((int *)num_ext_nodes)[iproc];
      nec = ((int *)num_elem_cmaps)[iproc];
      nnc = ((int *)num_node_cmaps)[iproc];
    }

    start[0] = iproc;

    if (nin > 0) {
      nmstat = 1;
    }
    else {
      nmstat = 0;
    }

    if ((status = nc_put_var1_int(exoid, varid_nm[0], start, &nmstat)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to output status int node map in file ID %d",
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    if (nbn > 0) {
      nmstat = 1;
    }
    else {
      nmstat = 0;
    }

    if ((status = nc_put_var1_int(exoid, varid_nm[1], start, &nmstat)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to output status bor node map in file ID %d",
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    if (nen > 0) {
      nmstat = 1;
    }
    else {
      nmstat = 0;
    }

    if ((status = nc_put_var1_int(exoid, varid_nm[2], start, &nmstat)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to output status ext node map in file ID %d",
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    if (nie > 0) {
      nmstat = 1;
    }
    else {
      nmstat = 0;
    }

    if ((status = nc_put_var1_int(exoid, varid_em[0], start, &nmstat)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to output status int elem map in file ID %d",
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    if (nbe > 0) {
      nmstat = 1;
    }
    else {
      nmstat = 0;
    }

    if ((status = nc_put_var1_int(exoid, varid_em[1], start, &nmstat)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to output status bor elem map in file ID %d",
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* now fill the index variables */
    if (varid_idx[0] > 0) {
      /* increment to the next starting position */
      num_int_elem += nie;
#if NC_HAS_HDF5
      status = nc_put_var1_longlong(exoid, varid_idx[0], start, (long long *)&num_int_elem);
#else
      status = nc_put_var1_int(exoid, varid_idx[0], start, &num_int_elem);
#endif
      if (status != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to output internal element map index in file ID %d", exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }
    }

    if (varid_idx[1] > 0) {
      /* increment to the next starting position */
      num_bor_elem += nbe;
#if NC_HAS_HDF5
      status = nc_put_var1_longlong(exoid, varid_idx[1], start, (long long *)&num_bor_elem);
#else
      status = nc_put_var1_int(exoid, varid_idx[1], start, &num_bor_elem);
#endif
      if (status != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to output border element map index in file ID %d", exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }
    }

    if (varid_idx[2] > 0) {
      /* increment to the next starting position */
      num_int_node += nin;
#if NC_HAS_HDF5
      status = nc_put_var1_longlong(exoid, varid_idx[2], start, (long long *)&num_int_node);
#else
      status = nc_put_var1_int(exoid, varid_idx[2], start, &num_int_node);
#endif
      if (status != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to output internal node map index in file ID %d", exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }
    }

    if (varid_idx[3] > 0) {
      /* increment to the next starting position */
      num_bor_node += nbn;
#if NC_HAS_HDF5
      status = nc_put_var1_longlong(exoid, varid_idx[3], start, (long long *)&num_bor_node);
#else
      status = nc_put_var1_int(exoid, varid_idx[3], start, &num_bor_node);
#endif
      if (status != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to output border node map index in file ID %d", exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }
    }

    if (varid_idx[4] > 0) {
      /* increment to the next starting position */
      num_ext_node += nen;
#if NC_HAS_HDF5
      status = nc_put_var1_longlong(exoid, varid_idx[4], start, (long long *)&num_ext_node);
#else
      status = nc_put_var1_int(exoid, varid_idx[4], start, &num_ext_node);
#endif
      if (status != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to output external node map index in file ID %d", exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }
    }

    if (varid_idx[5] > 0) {
      /* increment to the next starting position */
      num_n_cmaps += nnc;
#if NC_HAS_HDF5
      status = nc_put_var1_longlong(exoid, varid_idx[5], start, (long long *)&num_n_cmaps);
#else
      status = nc_put_var1_int(exoid, varid_idx[5], start, &num_n_cmaps);
#endif
      if (status != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to output node communication map index "
                 "in file ID %d",
                 exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }
    }

    if (varid_idx[6] > 0) {
      /* increment to the next starting position */
      num_e_cmaps += nec;
#if NC_HAS_HDF5
      status = nc_put_var1_longlong(exoid, varid_idx[6], start, (long long *)&num_e_cmaps);
#else
      status = nc_put_var1_int(exoid, varid_idx[6], start, &num_e_cmaps);
#endif
      if (status != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to output elem communication map index "
                 "in file ID %d",
                 exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }
    }

  } /* End "for(iproc=0; iproc < num_proc_in_f; iproc++)" */
  EX_FUNC_LEAVE(EX_NOERR);
}
