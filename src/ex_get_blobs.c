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
 * writes the blob parameters and optionally blob data for all blobs
 * assumes that `blob` is large enough to contain all blobs.
 * \param   exoid                   exodus file id
 * \param  *blob                array of ex_blob structures
 */
int ex_get_blobs(int exoid, ex_blob *blob)
{
  /* Determine number of blobs on database */
  int num_blob = ex_inquire_int(exoid, EX_INQ_BLOB);
  if (num_blob < 0) {
    char errmsg[MAX_ERR_LENGTH];
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to inquire BLOB count in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, num_blob);
    return (EX_FATAL);
  }

  if (ex_int64_status(exoid) & EX_IDS_INT64_API) {
    int64_t *ids = calloc(num_blob, sizeof(int64_t));
    ex_get_ids(exoid, EX_BLOB, ids);
    for (int i = 0; i < num_blob; i++) {
      blob[i].id = ids[i];
    }
    free(ids);
  }
  else {
    int *ids = calloc(num_blob, sizeof(int));
    ex_get_ids(exoid, EX_BLOB, ids);
    for (int i = 0; i < num_blob; i++) {
      blob[i].id = ids[i];
    }
    free(ids);
  }

  for (int i = 0; i < num_blob; i++) {
    int status = ex_get_blob(exoid, &blob[i]);
    if (status != EX_NOERR) {
      return status;
    }
  }
  return EX_NOERR;
}
