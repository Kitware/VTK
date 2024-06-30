/*
 * Copyright(C) 1999-2020, 2024 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, EX_NOERR

/**
 * \ingroup Utilities
 * Given a file or group 'parent' id, return the
 * number of child groups and the ids of the child groups below the
 * parent.  If num_groups is NULL, do not return count; if group_ids
 * is NULL, do not return ids.
 */
int ex_get_group_ids(int parent_id, int *num_groups, int *group_ids)
{
  char errmsg[MAX_ERR_LENGTH];

#if NC_HAS_HDF5
  int status;

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(parent_id, __func__) != EX_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  status = nc_inq_grps(parent_id, num_groups, group_ids);
  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: Failed to get child group ids in file id %d",
             parent_id);
    ex_err_fn(parent_id, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }
  EX_FUNC_LEAVE(EX_NOERR);
#else
  EX_FUNC_ENTER();
  snprintf(errmsg, MAX_ERR_LENGTH,
           "ERROR: Group capabilities are not available in this netcdf "
           "version--not netcdf4");
  ex_err_fn(parent_id, __func__, errmsg, NC_ENOTNC4);
  EX_FUNC_LEAVE(EX_FATAL);
#endif
}
