/*
 * Copyright(C) 1999-2021 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

/*!
 *
 * expfrm - ex_put_coordinate_frames: write coordinate frames
 *
 * \param exoid          exodus file id
 * \param nframes        number of coordinate frames in model
 * \param cf_ids         coordinate ids
 * \param pt_coordinates pointer to coordinates. 9 values per coordinate frame
 * \param tags           character tag for each frame. 'r' - rectangular, 'c' -
 *cylindrical, 's' - spherical
 *
 * returns -
 *      EX_NOERR         for no error
 *      EX_FATAL         for fatal errors
 *      1                number frames < 0
 *
 *****************************************************************************/

#include "exodusII.h"     // for ex_err, EXERRVAL, etc
#include "exodusII_int.h" // for EX_FATAL, EX_NOERR, etc

int ex_put_coordinate_frames(int exoid, int nframes, const void_int *cf_ids,
                             const void *pt_coordinates, const char *tags)
{
  char errmsg[MAX_ERR_LENGTH]; /* buffer for error messages      */

  EX_FUNC_ENTER();

  if (exoid < 0) {
    EX_FUNC_LEAVE(exoid);
  }

  if (nframes == 0) { /* write nothing */
    EX_FUNC_LEAVE(EX_NOERR);
  }

  if (nframes < 0) {
    EX_FUNC_LEAVE(1);
  }

  assert(cf_ids != NULL);
  assert(pt_coordinates != NULL);
  assert(tags != NULL);

  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* make the definitions */
  /* go into define mode. define num_frames, num_frames9 */
  int status;
  if ((status = exi_redef(exoid, __func__)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to place file id %d into define mode", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  int dim, dim9; /* dimension id for nframes, nframes*9 */
  if ((status = nc_def_dim(exoid, DIM_NUM_CFRAMES, nframes, &dim)) != NC_NOERR ||
      (nc_def_dim(exoid, DIM_NUM_CFRAME9, nframes * 9, &dim9) != NC_NOERR)) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to define number of coordinate frames in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    goto error_ret;
  }

  int int_type = NC_INT;
  if (ex_int64_status(exoid) & EX_IDS_INT64_DB) {
    int_type = NC_INT64;
  }

  /* define the variables. coordinates, tags and ids */
  int varcoords; /* variable id for the coordinates */
  int varids;    /* variable id for the frame ids  */
  int vartags;   /* variable id for the frame tags */
  if (nc_def_var(exoid, VAR_FRAME_COORDS, nc_flt_code(exoid), 1, &dim9, &varcoords) != NC_NOERR ||
      (nc_def_var(exoid, VAR_FRAME_IDS, int_type, 1, &dim, &varids) != NC_NOERR) ||
      (nc_def_var(exoid, VAR_FRAME_TAGS, NC_CHAR, 1, &dim, &vartags) != NC_NOERR)) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR:  failed to define coordinate frames in file id %d",
             exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_FATAL);
    goto error_ret; /* exit define mode and return */
  }

  /* leave define mode */
  if ((status = exi_leavedef(exoid, __func__)) != NC_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* check variables consistency */
  for (int i = 0; i < nframes; i++) {
    if (strchr("RrCcSs", tags[i]) == NULL) {
      snprintf(errmsg, MAX_ERR_LENGTH, "Warning: Unrecognized coordinate frame tag: '%c'.",
               tags[i]);
      ex_err_fn(exoid, __func__, errmsg, 2);
    }
  }
  /* could also check vectors. Leave this up to the application */

  /* put the variables into the file */
  if (nc_put_var_text(exoid, vartags, tags) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed writing frame data in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (ex_int64_status(exoid) & EX_IDS_INT64_API) {
    status = nc_put_var_longlong(exoid, varids, cf_ids);
  }
  else {
    status = nc_put_var_int(exoid, varids, cf_ids);
  }

  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed writing frame data in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (exi_comp_ws(exoid) == 4) {
    status = nc_put_var_float(exoid, varcoords, pt_coordinates);
  }
  else {
    status = nc_put_var_double(exoid, varcoords, pt_coordinates);
  }

  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed writing frame data in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }
  EX_FUNC_LEAVE(EX_NOERR);

error_ret:
  exi_leavedef(exoid, __func__);
  EX_FUNC_LEAVE(EX_FATAL);
}
