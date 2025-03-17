/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/* Function(s) contained in this file:
 *     ex_put_init_global()
 *****************************************************************************
 * This function outputs the initial global information.
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

#include <exodusII.h>     // for ex_err, etc
#include <exodusII_int.h> // for exi_leavedef, EX_FATAL, etc

/*!
 * \ingroup ModelDescription
 * \undoc
 */
int ex_put_init_global(int exoid, int64_t num_nodes_g, int64_t num_elems_g, int64_t num_elem_blks_g,
                       int64_t num_node_sets_g, int64_t num_side_sets_g)
{
  int  varid, dimid, status;
  char errmsg[MAX_ERR_LENGTH];

  int int_type = NC_INT;
  int id_type  = NC_INT;

  /*-----------------------------Execution begins-----------------------------*/
  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (ex_int64_status(exoid) & EX_BULK_INT64_DB) {
    int_type = NC_INT64;
  }
  if (ex_int64_status(exoid) & EX_IDS_INT64_DB) {
    id_type = NC_INT64;
  }
  /* Put NetCDF file into define mode */
  if ((status = exi_redef(exoid, __func__)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to put file ID %d into define mode", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Output the file version */
  if ((status = exi_put_nemesis_version(exoid)) < 0) {
    EX_FUNC_LEAVE(status);
  }

  /* Define dimension for number of global nodes */
  if ((status = nc_def_dim(exoid, DIM_NUM_NODES_GLOBAL, num_nodes_g, &dimid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to dimension \"%s\" in file ID %d",
             DIM_NUM_NODES_GLOBAL, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    /* Leave define mode before returning */
    exi_leavedef(exoid, __func__);

    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Define dimension for number of global elements */
  if ((status = nc_def_dim(exoid, DIM_NUM_ELEMS_GLOBAL, num_elems_g, &dimid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to dimension \"%s\" in file ID %d",
             DIM_NUM_ELEMS_GLOBAL, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    /* Leave define mode before returning */
    exi_leavedef(exoid, __func__);

    EX_FUNC_LEAVE(EX_FATAL);
  }

  /*
   * Output the number of global element blocks. This is output as a
   * dimension since the vector of global element block IDs is sized
   * by this quantity.
   */
  if ((status = nc_def_dim(exoid, DIM_NUM_ELBLK_GLOBAL, num_elem_blks_g, &dimid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to dimension \"%s\" in file ID %d",
             DIM_NUM_ELBLK_GLOBAL, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    /* Leave define mode before returning */
    exi_leavedef(exoid, __func__);

    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Define the element block IDs variable. */
  if ((status = nc_def_var(exoid, VAR_ELBLK_IDS_GLOBAL, id_type, 1, &dimid, &varid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to put variable definition for \"%s\" into file ID %d",
             VAR_ELBLK_IDS_GLOBAL, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    /* Leave define mode before returning */
    exi_leavedef(exoid, __func__);

    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Define the element block counts variable. */
  if ((status = nc_def_var(exoid, VAR_ELBLK_CNT_GLOBAL, int_type, 1, &dimid, &varid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to put variable definition for \"%s\" into file ID %d",
             VAR_ELBLK_CNT_GLOBAL, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    /* Leave define mode before returning */
    exi_leavedef(exoid, __func__);

    EX_FUNC_LEAVE(EX_FATAL);
  }

  /*
   * Output the number of global node sets. This is output as a
   * dimension since the vector of global element block IDs is sized
   * by this quantity.
   */
  if (num_node_sets_g > 0) {
    if ((status = nc_def_dim(exoid, DIM_NUM_NS_GLOBAL, num_node_sets_g, &dimid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to dimension \"%s\" in file ID %d",
               DIM_NUM_NS_GLOBAL, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* Define the variable for output of global node set IDs */
    if ((status = nc_def_var(exoid, VAR_NS_IDS_GLOBAL, id_type, 1, &dimid, &varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to put variable definition for \"%s\" into file ID %d",
               VAR_NS_IDS_GLOBAL, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);

      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* Define variable for global node counts in each global node set */
    if ((status = nc_def_var(exoid, VAR_NS_NODE_CNT_GLOBAL, int_type, 1, &dimid, &varid)) !=
        NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to put variable definition for \"%s\" into file ID %d",
               VAR_NS_NODE_CNT_GLOBAL, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }

    /*
     * Define variable for global dist. factor count in each global
     * node set
     */
    if ((status = nc_def_var(exoid, VAR_NS_DF_CNT_GLOBAL, int_type, 1, &dimid, &varid)) !=
        NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to put variable definition for \"%s\" into file ID %d",
               VAR_NS_DF_CNT_GLOBAL, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }

  } /* End "if (num_node_sets_g > 0)" */

  /*
   * Output the number of global side sets. This is output as a
   * dimension since the vector of global element block IDs is sized
   * by this quantity.
   */
  if (num_side_sets_g > 0) {
    if ((status = nc_def_dim(exoid, DIM_NUM_SS_GLOBAL, num_side_sets_g, &dimid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to dimension \"%s\" in file id %d",
               DIM_NUM_SS_GLOBAL, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* Define the variable for output of global side set IDs */
    if ((status = nc_def_var(exoid, VAR_SS_IDS_GLOBAL, id_type, 1, &dimid, &varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to put variable definition for \"%s\" into file id %d",
               VAR_SS_IDS_GLOBAL, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);

      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }

    /*
     * Define the variable for count of global number of sides in the
     * global side sets.
     */
    if ((status = nc_def_var(exoid, VAR_SS_SIDE_CNT_GLOBAL, int_type, 1, &dimid, &varid)) !=
        NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to put variable definition for \"%s\" into file id %d",
               VAR_SS_SIDE_CNT_GLOBAL, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);

      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }

    /*
     * Define the variable for count of global dist. factors in the
     * global side sets.
     */
    if ((status = nc_def_var(exoid, VAR_SS_DF_CNT_GLOBAL, int_type, 1, &dimid, &varid)) !=
        NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to put variable definition for \"%s\" into file id %d",
               VAR_SS_DF_CNT_GLOBAL, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);

      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }

  } /* End "if (num_side_sets_g > 0)" */

  /* End define mode */
  if (exi_leavedef(exoid, __func__) != EX_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  EX_FUNC_LEAVE(EX_NOERR);
}
