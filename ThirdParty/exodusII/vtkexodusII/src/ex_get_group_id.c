/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, EX_FILE_ID_MASK, etc

/**
 * \ingroup Utilities
 * Given an exoid and group name (NULL gets root group), return id of that
 * group.
 * If the name is NULL, return the root group.
 * If the name contains "/", then the name is assumed to be a full path name
 * and all groups in the file are searched.
 * Otherwise, the name is assumed to be the name of a child group of exoid
 */
int ex_get_group_id(int parent_id, const char *group_name, int *group_id)
{
  char errmsg[MAX_ERR_LENGTH];
#if NC_HAS_HDF5
  EX_FUNC_ENTER();
  /* See if name contains "/" indicating it is a full path name... */
  if (group_name == NULL) {
    /* Return root */
    *group_id = (unsigned)parent_id & EX_FILE_ID_MASK;
  }
  else if (strchr(group_name, '/') == NULL) {
    /* Local child */
    int status = nc_inq_grp_ncid(parent_id, group_name, group_id);
    if (status != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: Failed to locate group with name %s as child "
               "group in file id %d",
               group_name, parent_id);
      ex_err_fn(parent_id, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }
  else {
    /* Full path name */
    int status = nc_inq_grp_full_ncid(parent_id, group_name, group_id);
    if (status != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: Failed to locate group with full path name %s in file id %d", group_name,
               parent_id);
      ex_err_fn(parent_id, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }
  EX_FUNC_LEAVE(EX_NOERR);
#else
  EX_UNUSED(parent_id);
  EX_UNUSED(group_name);
  EX_UNUSED(group_id);
  EX_FUNC_ENTER();
  snprintf(errmsg, MAX_ERR_LENGTH,
           "ERROR: Group capabilities are not available in this netcdf "
           "version--not netcdf4");
  ex_err_fn(parent_id, __func__, errmsg, NC_ENOTNC4);
  EX_FUNC_LEAVE(EX_FATAL);
#endif
}
