/*
 * Copyright(C) 1999-2020, 2022 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h"
#include "exodusII_int.h"
#include <stdbool.h>
/*!
 * writes the blob parameters for 1 or more blobs
 * \param   exoid                exodus file id
 * \param   count                size of `blobs` array
 * \param  *blobs                array of ex_blob structures
 */

int ex_put_blobs(int exoid, size_t count, const struct ex_blob *blobs)
{
  int  dimid, status, n1dim, dims[1];
  char errmsg[MAX_ERR_LENGTH];

  EX_FUNC_ENTER();

  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  int *entlst_id = (int *)calloc(count, sizeof(int));

  if ((status = exi_redef(exoid, __func__)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to put file id %d into define mode", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    free(entlst_id);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  status = nc_inq_dimid(exoid, DIM_N1, &n1dim);
  if (status != NC_NOERR) {
    if ((status = nc_def_dim(exoid, DIM_N1, 1L, &n1dim)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to define number \"1\" dimension in file id %d", exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      goto error_ret; /* exit define mode and return */
    }
  }

  for (size_t i = 0; i < count; i++) {
    char *numentryptr = DIM_NUM_VALUES_BLOB(blobs[i].id);

    /* define dimensions and variables */
    if ((status = nc_def_dim(exoid, numentryptr, blobs[i].num_entry, &dimid)) != NC_NOERR) {
      if (status == NC_ENAMEINUSE) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: blob %" PRId64 " -- size already defined in file id %d", blobs[i].id,
                 exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
      }
      else {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to define number of entries in blob %" PRId64 " in file id %d",
                 blobs[i].id, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
      }
      goto error_ret;
    }

    /* create a variable just as a way to have a blob and its attributes; values not used for
     * anything */
    dims[0] = n1dim;
    if ((status = nc_def_var(exoid, VAR_ENTITY_BLOB(blobs[i].id), NC_INT, 1, dims,
                             &entlst_id[i])) != NC_NOERR) {
      if (status == NC_ENAMEINUSE) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: entity already exists for blob %" PRId64 " in file id %d", blobs[i].id,
                 exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
      }
      else {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to create entity for blob %" PRId64 " in file id %d", blobs[i].id,
                 exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
      }
      goto error_ret; /* exit define mode and return */
    }
    exi_compress_variable(exoid, entlst_id[i], 1);

    if (ex_int64_status(exoid) & EX_IDS_INT64_DB) {
      long long id = blobs[i].id;
      status       = nc_put_att_longlong(exoid, entlst_id[i], EX_ATTRIBUTE_ID, NC_INT64, 1, &id);
    }
    else {
      int id = blobs[i].id;
      status = nc_put_att_int(exoid, entlst_id[i], EX_ATTRIBUTE_ID, NC_INT, 1, &id);
    }
    if (status != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to store blob id %" PRId64 " in file id %d",
               blobs[i].id, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      goto error_ret; /* exit define mode and return */
    }

    if ((status = nc_put_att_text(exoid, entlst_id[i], EX_ATTRIBUTE_NAME, strlen(blobs[i].name) + 1,
                                  blobs[i].name)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to store blob name %s in file id %d",
               blobs[i].name, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      goto error_ret; /* exit define mode and return */
    }

    /* Increment blob count */
    struct exi_file_item *file = exi_find_file_item(exoid);
    if (file) {
      file->blob_count++;
    }
  }
  /* leave define mode  */
  if ((status = exi_leavedef(exoid, __func__)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to exit define mode in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    free(entlst_id);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Output dummy data for the blob var; */
  long dummy = 0;
  for (size_t i = 0; i < count; i++) {
    if ((status = nc_put_var_long(exoid, entlst_id[i], &dummy)) != EX_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to output dummy value for blob %" PRId64 " in file id %d",
               blobs[i].id, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      free(entlst_id);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }
  free(entlst_id);
  EX_FUNC_LEAVE(EX_NOERR);

/* Fatal error: exit definition mode and return */
error_ret:
  exi_leavedef(exoid, __func__);
  free(entlst_id);
  EX_FUNC_LEAVE(EX_FATAL);
}
