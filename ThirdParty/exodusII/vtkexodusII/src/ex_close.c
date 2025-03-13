/*
 * Copyright(C) 1999-2020, 2022, 2023, 2024 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * exclos - ex_close
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *
 * exit conditions -
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for exi_get_counter_list, etc

/*!
\ingroup Utilities

The function ex_close() updates and then closes an open exodus file.

\return In case of an error, ex_close() returns a negative number; a
        warning will return a positive number. Possible causes of errors
        include:
 - data file not properly opened with call to ex_create() or ex_open()

 \param exoid      exodus file ID returned from a previous call to ex_create()
or ex_open().

The following code segment closes an open exodus file:

~~~{.c}
int error,exoid;
error = ex_close (exoid);
~~~

 */
int ex_close(int exoid)
{
  char errmsg[MAX_ERR_LENGTH];
  int  status;
  int  status1;
  int  status2;

  EX_FUNC_ENTER();

  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

#ifndef NDEBUG
  struct exi_file_item *file = exi_find_file_item(exoid);
  assert(!file->in_define_mode && file->persist_define_mode == 0);
#endif

  /*
   * NOTE: If using netcdf-4, exoid must refer to the root group.
   * Need to determine whether there are any groups and if so,
   * call exi_rm_file_item and exi_rm_stat_ptr on each group.
   */

  /*
   * Get exoid of root group
   */

  if ((status1 = nc_sync(exoid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to update file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, status1);
  }

  int root_id = exoid & EX_FILE_ID_MASK;
  if ((status2 = nc_close(root_id)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to close file id %d", root_id);
    ex_err_fn(root_id, __func__, errmsg, status2);
  }

  /* Even if we have failures above due to nc_sync() or nc_close(), we still need to clean up our
   * internal datastructures.
   */

  exi_rm_file_item(exoid, exi_get_counter_list(EX_ELEM_BLOCK));
  exi_rm_file_item(exoid, exi_get_counter_list(EX_FACE_BLOCK));
  exi_rm_file_item(exoid, exi_get_counter_list(EX_EDGE_BLOCK));
  exi_rm_file_item(exoid, exi_get_counter_list(EX_NODE_SET));
  exi_rm_file_item(exoid, exi_get_counter_list(EX_EDGE_SET));
  exi_rm_file_item(exoid, exi_get_counter_list(EX_FACE_SET));
  exi_rm_file_item(exoid, exi_get_counter_list(EX_SIDE_SET));
  exi_rm_file_item(exoid, exi_get_counter_list(EX_ELEM_SET));
  exi_rm_file_item(exoid, exi_get_counter_list(EX_NODE_MAP));
  exi_rm_file_item(exoid, exi_get_counter_list(EX_EDGE_MAP));
  exi_rm_file_item(exoid, exi_get_counter_list(EX_FACE_MAP));
  exi_rm_file_item(exoid, exi_get_counter_list(EX_ELEM_MAP));
  exi_rm_file_item(exoid, exi_get_counter_list(EX_ASSEMBLY));
  exi_rm_file_item(exoid, exi_get_counter_list(EX_BLOB));

  exi_rm_stat_ptr(exoid, &exoII_ed);
  exi_rm_stat_ptr(exoid, &exoII_fa);
  exi_rm_stat_ptr(exoid, &exoII_eb);
  exi_rm_stat_ptr(exoid, &exoII_ns);
  exi_rm_stat_ptr(exoid, &exoII_es);
  exi_rm_stat_ptr(exoid, &exoII_fs);
  exi_rm_stat_ptr(exoid, &exoII_ss);
  exi_rm_stat_ptr(exoid, &exoII_els);
  exi_rm_stat_ptr(exoid, &exoII_nm);
  exi_rm_stat_ptr(exoid, &exoII_edm);
  exi_rm_stat_ptr(exoid, &exoII_fam);
  exi_rm_stat_ptr(exoid, &exoII_em);

  exi_conv_exit(exoid);

  status = EX_NOERR;
  if (status1 != NC_NOERR || status2 != NC_NOERR) {
    status = EX_FATAL;
  }
  EX_FUNC_LEAVE(status);
}
