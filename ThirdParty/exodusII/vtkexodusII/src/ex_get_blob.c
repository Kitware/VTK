/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h"     // for ex_blob, ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, etc

/*!
 * reads the blob parameters and optionally blob data for one blob
 * \param   exoid                   exodus file id
 * \param  *blob                ex_blob structure
 */

int ex_get_blob(int exoid, ex_blob *blob)
{
  struct exi_file_item *file   = NULL;
  int                   status = 0;
  int                   dimid  = 0;
  size_t                len    = 0;
  char                  errmsg[MAX_ERR_LENGTH];

  EX_FUNC_ENTER();

  file = exi_find_file_item(exoid);
  if (!file) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: unknown file id %d.", exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADFILEID);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* First, locate varid of blob  */
  int entlst_id = 0;
  if ((status = nc_inq_varid(exoid, VAR_ENTITY_BLOB(blob->id), &entlst_id)) != NC_NOERR) {
    ex_get_err(NULL, NULL, &status);
    if (status != 0) {
      if (blob->name != NULL) {
        ex_copy_string(blob->name, "NULL", MAX_STR_LENGTH + 1); /* NULL element type name */
      }
      blob->num_entry = 0;
      if (status == EX_NULLENTITY) { /* NULL element blob?    */
        EX_FUNC_LEAVE(EX_NOERR);
      }
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to locate blob id  %" PRId64 " in id array in file id %d", blob->id,
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }

  char *numentryptr = DIM_NUM_VALUES_BLOB(blob->id);
  if ((status = nc_inq_dimid(exoid, numentryptr, &dimid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to locate number of entities in blob %" PRId64 " in file id %d",
             blob->id, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if ((status = nc_inq_dimlen(exoid, dimid, &len)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to get number of entities in blob %" PRId64 " in file id %d", blob->id,
             exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  blob->num_entry = len;

  /* look up entity list array for this blob id */
  if ((status = nc_inq_varid(exoid, VAR_ENTITY_BLOB(blob->id), &entlst_id)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to locate entity list array for blob %" PRId64 " in file id %d",
             blob->id, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* read the name */
  if (blob->name != NULL) {
    int  name_size             = ex_inquire_int(exoid, EX_INQ_MAX_READ_NAME_LENGTH);
    char tmp_name[EX_MAX_NAME] = "";
    if ((status = nc_get_att_text(exoid, entlst_id, EX_ATTRIBUTE_NAME, tmp_name)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to read blob name for blob %" PRId64 " in file id %d", blob->id,
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
    ex_copy_string(blob->name, tmp_name, name_size + 1);
  }

  EX_FUNC_LEAVE(EX_NOERR);
}
