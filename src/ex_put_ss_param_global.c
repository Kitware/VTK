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
 *     ex_put_ss_param_global()
 *****************************************************************************
 * This function outputs the global side-set parameters.
 *****************************************************************************
 *  Variable Index:
 *      exoid            - The NetCDF ID of an already open NemesisI file.
 *      global_ids      - Pointer to a vector of global side-set IDs.
 *      side_cnts       - Pointer to a vector of global side counts in
 *                        each global side set.
 *      df_cnts         - Pointer to a vector of global distribution
 *                        factors in each global side set.
 */
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

#include <exodusII.h>
#include <exodusII_int.h>

int ex_put_ss_param_global(int exoid, const void_int *global_ids, const void_int *side_cnts,
                           const void_int *df_cnts)
{
  int varid;

  int  status;
  char errmsg[MAX_ERR_LENGTH];
  /*-----------------------------Execution begins-----------------------------*/

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Get the variable ID for the vector of global side set IDs */
  if ((status = nc_inq_varid(exoid, VAR_SS_IDS_GLOBAL, &varid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find variable ID for \"%s\" in file ID %d",
             VAR_SS_IDS_GLOBAL, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);

    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Output the vector of global side set IDs */
  if (ex_int64_status(exoid) & EX_IDS_INT64_API) {
    status = nc_put_var_longlong(exoid, varid, global_ids);
  }
  else {
    status = nc_put_var_int(exoid, varid, global_ids);
  }
  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to output variable \"%s\" to file ID %d",
             VAR_SS_IDS_GLOBAL, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);

    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Get the variable ID for the vector of global side-set side counts */
  if ((status = nc_inq_varid(exoid, VAR_SS_SIDE_CNT_GLOBAL, &varid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find variable ID for \"%s\" in file ID %d",
             VAR_SS_SIDE_CNT_GLOBAL, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);

    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Output the vector of global side counts in each global side set */
  if (ex_int64_status(exoid) & EX_BULK_INT64_API) {
    status = nc_put_var_longlong(exoid, varid, side_cnts);
  }
  else {
    status = nc_put_var_int(exoid, varid, side_cnts);
  }
  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to put variable \"%s\" in file ID %d",
             VAR_SS_SIDE_CNT_GLOBAL, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);

    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Get the variable ID for the number of dist. factors in each side set */
  if ((status = nc_inq_varid(exoid, VAR_SS_DF_CNT_GLOBAL, &varid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find variable ID for \"%s\" in file ID %d",
             VAR_SS_DF_CNT_GLOBAL, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);

    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Output the vector of dist. factor counts */
  if (ex_int64_status(exoid) & EX_BULK_INT64_API) {
    status = nc_put_var_longlong(exoid, varid, df_cnts);
  }
  else {
    status = nc_put_var_int(exoid, varid, df_cnts);
  }
  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to output variable \"%s\" in file ID %d",
             VAR_SS_DF_CNT_GLOBAL, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);

    EX_FUNC_LEAVE(EX_FATAL);
  }

  EX_FUNC_LEAVE(EX_NOERR);
}
