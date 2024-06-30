/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h"     // for ex_err, ex_name_of_object, etc
#include "exodusII_int.h" // for exi_is_parallel, etc

/*!
 * reads a portion of the set entry list and set extra list for a single set
 * \param   exoid                   exodus file id
 * \param   set_type                set type
 * \param   set_id                  set id
 * \param   offset                  index of first entry in set to access (1-based)
 * \param   num_to_get              number of entries to output.
 * \param   set_entry_list          array of entries in set. Set to NULL to not
 * read.
 * \param   set_extra_list          array of extras in set. Set to NULL to not
 * read.
 */

int ex_get_partial_set(int exoid, ex_entity_type set_type, ex_entity_id set_id, int64_t offset,
                       int64_t num_to_get, void_int *set_entry_list, void_int *set_extra_list)
{
  int    dimid, status;
  int    entry_list_id, extra_list_id, set_id_ndx;
  size_t start[1], count[1];
  char   errmsg[MAX_ERR_LENGTH];
  char  *entryptr = NULL;
  char  *extraptr = NULL;

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* first check if any sets are specified */
  if ((status = nc_inq_dimid(exoid, exi_dim_num_objects(set_type), &dimid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: no %ss defined in file id %d",
             ex_name_of_object(set_type), exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Lookup index of set id in VAR_*S_IDS array */
  set_id_ndx = exi_id_lkup(exoid, set_type, set_id);
  if (set_id_ndx <= 0) {
    ex_get_err(NULL, NULL, &status);

    if (status != 0) {
      if (status == EX_NULLENTITY) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "Warning: no data allowed for NULL %s %" PRId64 " in file id %d",
                 ex_name_of_object(set_type), set_id, exoid);
        ex_err_fn(exoid, __func__, errmsg, EX_NULLENTITY);
        EX_FUNC_LEAVE(EX_WARN);
      }
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to locate %s id %" PRId64 " in VAR_*S_IDS array in file id %d",
               ex_name_of_object(set_type), set_id, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }

  /* setup more pointers based on set_type */
  if (set_type == EX_NODE_SET) {
    entryptr = VAR_NODE_NS(set_id_ndx);
    extraptr = NULL;
  }
  else if (set_type == EX_EDGE_SET) {
    entryptr = VAR_EDGE_ES(set_id_ndx);
    extraptr = VAR_ORNT_ES(set_id_ndx);
  }
  else if (set_type == EX_FACE_SET) {
    entryptr = VAR_FACE_FS(set_id_ndx);
    extraptr = VAR_ORNT_FS(set_id_ndx);
  }
  else if (set_type == EX_SIDE_SET) {
    entryptr = VAR_ELEM_SS(set_id_ndx);
    extraptr = VAR_SIDE_SS(set_id_ndx);
  }
  else if (set_type == EX_ELEM_SET) {
    entryptr = VAR_ELEM_ELS(set_id_ndx);
    extraptr = NULL;
  }

  /* inquire id's of previously defined variables  */
  if ((status = nc_inq_varid(exoid, entryptr, &entry_list_id)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to locate entry list for %s %" PRId64 " in file id %d",
             ex_name_of_object(set_type), set_id, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* read the entry list and extra list arrays */
  if (set_entry_list != NULL || exi_is_parallel(exoid)) {
    start[0] = offset - 1;
    count[0] = num_to_get;
    if (count[0] == 0) {
      start[0] = 0;
    }

    if (ex_int64_status(exoid) & EX_BULK_INT64_API) {
      status = nc_get_vara_longlong(exoid, entry_list_id, start, count, set_entry_list);
    }
    else {
      status = nc_get_vara_int(exoid, entry_list_id, start, count, set_entry_list);
    }

    if (status != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to get entry list for %s %" PRId64 " in file id %d",
               ex_name_of_object(set_type), set_id, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }

  /* only do for edge, face and side sets */
  if (extraptr && (set_extra_list != NULL || exi_is_parallel(exoid))) {
    if ((status = nc_inq_varid(exoid, extraptr, &extra_list_id)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to locate extra list for %s %" PRId64 " in file id %d",
               ex_name_of_object(set_type), set_id, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    start[0] = offset - 1;
    count[0] = num_to_get;
    if (count[0] == 0) {
      start[0] = 0;
    }
    if (ex_int64_status(exoid) & EX_BULK_INT64_API) {
      status = nc_get_vara_longlong(exoid, extra_list_id, start, count, set_extra_list);
    }
    else {
      status = nc_get_vara_int(exoid, extra_list_id, start, count, set_extra_list);
    }

    if (status != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to get extra list for %s %" PRId64 " in file id %d",
               ex_name_of_object(set_type), set_id, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }

  /* warn if extra data was sent in for node sets and elem sets */
  if ((set_type == EX_NODE_SET || set_type == EX_ELEM_SET) && set_extra_list != NULL) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "Warning: extra list was ignored for %s %" PRId64 " in file id %d",
             ex_name_of_object(set_type), set_id, exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_MSG);
    EX_FUNC_LEAVE(EX_WARN);
  }

  EX_FUNC_LEAVE(EX_NOERR);
}
