/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************/
/*****************************************************************************/
/* Function(s) contained in this file:
 *     ex_get_eb_info_global()
 *****************************************************************************
 * This function retrieves the global parameters of all the element blocks
 * associated with a geometry.
 *****************************************************************************
 *  Variable Index:
 *      exoid            - The NetCDF ID of an already open NemesisI file.
 *      el_blk_ids      - Pointer to vector of global element block IDs.
 *      el_blk_cnts     - Pointer to vector of global element block counts.
 */
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

#include <exodusII.h>     // for ex_err, etc
#include <exodusII_int.h> // for EX_FATAL, etc

int ex_get_eb_info_global(int exoid, void_int *el_blk_ids, void_int *el_blk_cnts)
{
  int varid, status;

  char errmsg[MAX_ERR_LENGTH];
  /*-----------------------------Execution begins-----------------------------*/

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Find the variable ID for the element block IDs */
  if ((status = nc_inq_varid(exoid, VAR_ELBLK_IDS_GLOBAL, &varid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find variable ID for \"%s\" in file ID %d",
             VAR_ELBLK_IDS_GLOBAL, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);

    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Get the global element block IDs */
  if (ex_int64_status(exoid) & EX_IDS_INT64_API) {
    status = nc_get_var_longlong(exoid, varid, el_blk_ids);
  }
  else {
    status = nc_get_var_int(exoid, varid, el_blk_ids);
  }
  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get variable \"%s\" from file ID %d",
             VAR_ELBLK_IDS_GLOBAL, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);

    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Find the variable ID for the element block counts */
  if ((status = nc_inq_varid(exoid, VAR_ELBLK_CNT_GLOBAL, &varid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find variable ID for \"%s\" in file ID %d",
             VAR_ELBLK_CNT_GLOBAL, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);

    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Get the global element block counts */
  if (ex_int64_status(exoid) & EX_BULK_INT64_API) {
    status = nc_get_var_longlong(exoid, varid, el_blk_cnts);
  }
  else {
    status = nc_get_var_int(exoid, varid, el_blk_cnts);
  }
  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get variable \"%s\" from file ID %d",
             VAR_ELBLK_CNT_GLOBAL, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);

    EX_FUNC_LEAVE(EX_FATAL);
  }
  EX_FUNC_LEAVE(EX_NOERR);
}
