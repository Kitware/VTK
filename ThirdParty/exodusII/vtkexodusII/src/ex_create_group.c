/*
 * Copyright(C) 1999-2020, 2022, 2024 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL

/*!
  \ingroup ModelDescription
  \undoc
*/

int ex_create_group(int parent_id, const char *group_name)
{
  char errmsg[MAX_ERR_LENGTH];
#if NC_HAS_HDF5
  int exoid = -1;
  int status;

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(parent_id, __func__) != EX_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if ((status = exi_redef(parent_id, __func__)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to put file id %d into define mode", parent_id);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if ((status = nc_def_grp(parent_id, group_name, &exoid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: group create failed for %s in file id %d", group_name,
             parent_id);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if ((status = exi_leavedef(parent_id, __func__)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to exit define mode");
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }
  EX_FUNC_LEAVE(exoid);
#else
  EX_FUNC_ENTER();
  snprintf(errmsg, MAX_ERR_LENGTH,
           "ERROR: Group capabilities are not available in this netcdf "
           "version--not netcdf4");
  ex_err(__func__, errmsg, NC_ENOTNC4);
  EX_FUNC_LEAVE(EX_FATAL);
#endif
}
