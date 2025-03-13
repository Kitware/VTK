/*
 * Copyright(C) 1999-2021 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/* Function(s) contained in this file:
 *      ex_put_loadbal_param()
 *****************************************************************************
 * This function outputs the load balance parameters.
 *****************************************************************************
 *  Variable Index:
 *      exoid             - The NetCDF ID of an already open NemesisI file.
 *      num_int_nodes    - The number of internal FEM nodes.
 *      num_bor_nodes    - The number of border FEM nodes.
 *      num_ext_nodes    - The number of external FEM nodes.
 *      num_int_elems    - The number of internal FEM elements.
 *      num_bor_elems    - The number of border FEM elements.
 *      num_node_cmaps   - The number of nodal communication maps.
 *      num_elem_cmaps   - The number of elemental communication maps.
 *      processor        - The processor the file being read was written for.
 */
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

#include <exodusII.h>     // for ex_err, etc
#include <exodusII_int.h> // for exi_leavedef, EX_FATAL, etc

/*!
 * \ingroup ModelDescription
 * \undoc
 */
int ex_put_loadbal_param(int exoid, int64_t num_int_nodes, int64_t num_bor_nodes,
                         int64_t num_ext_nodes, int64_t num_int_elems, int64_t num_bor_elems,
                         int64_t num_node_cmaps, int64_t num_elem_cmaps, int processor)
{
  int  status, varid;
  int  dimid_npf, dimid[3];
  int  varid_nm[3], varid_em[2];
  char ftype[2];

  int nmstat, ltempsv;

  char errmsg[MAX_ERR_LENGTH];

  int id_type  = NC_INT;
  int map_type = NC_INT;
  if (ex_int64_status(exoid) & EX_IDS_INT64_DB) {
    id_type = NC_INT64;
  }
  if (ex_int64_status(exoid) & EX_MAPS_INT64_DB) {
    map_type = NC_INT64;
  }
  EX_UNUSED(processor);
  /*-----------------------------Execution begins-----------------------------*/

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Get the file type */
  if (exi_get_file_type(exoid, ftype) != EX_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get file type from file ID %d\n", exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* make sure that this is a parallel file */
  if (ftype[0] != 'p') {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: function for use with parallel files only, file ID %d\n", exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
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

  /* Output the file version */
  if ((status = exi_put_nemesis_version(exoid)) < 0) {
    exi_leavedef(exoid, __func__);
    EX_FUNC_LEAVE(status);
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

  /* Define variable for the internal element information */
  if (num_int_elems > 0) {
    ltempsv = num_int_elems;
    if ((status = nc_def_dim(exoid, DIM_NUM_INT_ELEMS, ltempsv, &dimid[0])) != NC_NOERR) {
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

  } /* End "if (num_int_elems > 0)" */

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

  /* Define variable for the border element information */
  if (num_bor_elems > 0) {
    ltempsv = num_bor_elems;
    if ((status = nc_def_dim(exoid, DIM_NUM_BOR_ELEMS, ltempsv, &dimid[0])) != NC_NOERR) {
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

  } /* End "if (num_bor_elems > 0)" */

  if (num_int_nodes > 0) {
    /* Define variable for vector of internal FEM node IDs */
    ltempsv = num_int_nodes;
    if ((status = nc_def_dim(exoid, DIM_NUM_INT_NODES, ltempsv, &dimid[0])) != NC_NOERR) {
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

  } /* End "if (num_int_nodes > 0)" */

  if (num_bor_nodes > 0) {
    /* Define variable for vector of border FEM node IDs */
    ltempsv = num_bor_nodes;
    if ((status = nc_def_dim(exoid, DIM_NUM_BOR_NODES, ltempsv, &dimid[1])) != NC_NOERR) {
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

  } /* End "if (num_bor_nodes > 0)" */

  if (num_ext_nodes > 0) {
    /* Define dimension for vector of external FEM node IDs */
    ltempsv = num_ext_nodes;
    if ((status = nc_def_dim(exoid, DIM_NUM_EXT_NODES, ltempsv, &dimid[2])) != NC_NOERR) {
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

  } /* End "if (num_ext_nodes > 0)" */

  /* Add the nodal communication map count */
  if (num_node_cmaps > 0) {
    ltempsv = num_node_cmaps;
    if ((status = nc_def_dim(exoid, DIM_NUM_N_CMAPS, ltempsv, &dimid[0])) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to add dimension \"%s\" in file ID %d",
               DIM_NUM_N_CMAPS, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* Add the ID vector */
    if ((status = nc_def_var(exoid, VAR_N_COMM_IDS, id_type, 1, dimid, &varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define variable \"%s\" in file ID %d",
               VAR_N_COMM_IDS, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }
    exi_compress_variable(exoid, varid, 1);

    /* Add the status vector */
    if ((status = nc_def_var(exoid, VAR_N_COMM_STAT, NC_INT, 1, dimid, &varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define variable \"%s\" in file ID %d",
               VAR_N_COMM_STAT, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }

  } /* End "if (num_node_cmaps > 0)" */

  /* Add the nodal communication map count */
  if (num_elem_cmaps > 0) {
    ltempsv = num_elem_cmaps;
    if ((status = nc_def_dim(exoid, DIM_NUM_E_CMAPS, ltempsv, &dimid[0])) != NC_NOERR) {
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
    exi_compress_variable(exoid, varid, 1);

    if ((status = nc_def_var(exoid, VAR_E_COMM_STAT, NC_INT, 1, dimid, &varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define variable \"%s\" in file ID %d",
               VAR_E_COMM_STAT, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }

  } /* End "if (num_elem_cmaps > 0)" */

  /* Leave define mode */
  if (exi_leavedef(exoid, __func__) != EX_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /*
  ** Set up status vector for internal node map
  ** NOTE(9/26/96): this function is no longer valid
  ** for scaler files, so no need to check for file type
  */
  if (num_int_nodes == 0) {
    /* NULL set for internal nodes */
    nmstat = 0;
    if ((status = nc_put_var_int(exoid, varid_nm[0], &nmstat)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to output status for int node map in file ID %d", exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }
  else {
    nmstat = 1;
    if ((status = nc_put_var_int(exoid, varid_nm[0], &nmstat)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to output status for int node map in file ID %d", exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

  } /* End "if (num_int_nodes == 0)" */

  /* Set up status vector for border node map */
  if (num_bor_nodes == 0) {
    /* NULL set for border nodes */
    nmstat = 0;
    if ((status = nc_put_var_int(exoid, varid_nm[1], &nmstat)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to output status for bor node map in file ID %d", exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }
  else {
    /* Set the status indicating non-NULL size */
    nmstat = 1;
    if ((status = nc_put_var_int(exoid, varid_nm[1], &nmstat)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to output status for bor node map in file ID %d", exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

  } /* End "if (num_bor_nodes == 0)" */

  /* Set up status vector for external node map */
  if (num_ext_nodes == 0) {
    /* NULL set for external nodes */
    nmstat = 0;
    if ((status = nc_put_var_int(exoid, varid_nm[2], &nmstat)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to output status for ext node map in file ID %d", exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }
  else {
    /* Set the status indicating non-NULL size */
    nmstat = 1;
    if ((status = nc_put_var_int(exoid, varid_nm[2], &nmstat)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to output status for ext node map in file ID %d", exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

  } /* End "if (num_ext_nodes == 0)" */

  /* Set up status vector for internal element map */
  if (num_int_elems == 0) {
    /* NULL set for internal elements */
    nmstat = 0;
    if ((status = nc_put_var_int(exoid, varid_em[0], &nmstat)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to output status for int elem map in file ID %d", exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }
  else {
    /* Set the status indicating non-NULL size */
    nmstat = 1;
    if ((status = nc_put_var_int(exoid, varid_em[0], &nmstat)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to output status for int elem map in file ID %d", exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

  } /* End "if (num_int_elems == 0)" */

  /* Set up status vector for border element map */
  if (num_bor_elems == 0) {
    /* NULL set for internal elements */
    nmstat = 0;
    if ((status = nc_put_var_int(exoid, varid_em[1], &nmstat)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to output status for bor elem map in file ID %d", exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }
  else {
    /* Set the status indicating non-NULL size */
    nmstat = 1;
    if ((status = nc_put_var_int(exoid, varid_em[1], &nmstat)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to output status for bor elem map in file ID %d", exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

  } /* End "if (num_bor_elems == 0)" */

  EX_FUNC_LEAVE(EX_NOERR);
}
