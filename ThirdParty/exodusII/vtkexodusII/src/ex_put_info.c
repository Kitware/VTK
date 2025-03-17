/*
 * Copyright(C) 1999-2020, 2022, 2024 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, DIM_NUM_INFO, etc

/*!
\ingroup Utilities

The function ex_put_info() writes information records to the
database. The records are #MAX_LINE_LENGTH character strings.

In case of an error, ex_put_info() returns a negative number;
a warning will return a positive number. Possible causes of errors
include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  data file opened for read only.
  -  information records already exist in file.

\param[in] exoid       exodus file ID returned from a previous call to
ex_create() or ex_open().
\param[in] num_info    The number of information records.
\param[in] info        Array containing the information records. To only
                       define the number of info records instead of
                       defining and outputting, pass NULL for
                       info argument.

The following code will write out three information records
to an open exodus file -

~~~{.c}
int error, exoid, num_info;
char *info[3];

\comment{write information records}
num_info = 3;

info[0] = "This is the first information record.";
info[1] = "This is the second information record.";
info[2] = "This is the third information record.";

error = ex_put_info(exoid, num_info, info);
~~~

The following code will first tell the database that there are three
information records, and then later actually output those records.

~~~{.c}
int error, exoid, num_info;
char *info[3];

\comment{Define the number of information records that will be written
later.}
num_info = 3;

error = ex_put_info(exoid, num_info, NULL);

\comment{Now, actually write the information records}
info[0] = "This is the first information record.";
info[1] = "This is the second information record.";
info[2] = "This is the third information record.";
error = ex_put_info(exoid, num_info, info);

~~~

 */

int ex_put_info(int exoid, int num_info, char *const info[])
{
  int    status;
  int    i, lindim, num_info_dim, dims[2], varid;
  size_t start[2], count[2];
  char   errmsg[MAX_ERR_LENGTH];

  int rootid = exoid & EX_FILE_ID_MASK;

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* only do this if there are records */
  if (num_info > 0) {

    /* See if the number of info records has already been defined.
       Assume that if the DIM_NUM_INFO dimension exists, then the
       VAR_INFO variable also exists...
     */
    status = nc_inq_dimid(rootid, DIM_NUM_INFO, &num_info_dim);
    if (status != NC_NOERR) {

      /* put file into define mode  */
      if ((status = exi_redef(rootid, __func__)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed put file id %d into define mode", rootid);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }

      /* define dimensions */
      if ((status = nc_def_dim(rootid, DIM_NUM_INFO, num_info, &num_info_dim)) != NC_NOERR) {
        if (status == NC_ENAMEINUSE) { /* duplicate entry? */
          snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: info records already exist in file id %d",
                   rootid);
          ex_err_fn(exoid, __func__, errmsg, status);
        }
        else {
          snprintf(errmsg, MAX_ERR_LENGTH,
                   "ERROR: failed to define number of info records in file id %d", rootid);
          ex_err_fn(exoid, __func__, errmsg, status);
        }

        goto error_ret; /* exit define mode and return */
      }

      /* create line length dimension */
      if ((status = nc_def_dim(rootid, DIM_LIN, (MAX_LINE_LENGTH + 1), &lindim)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define line length in file id %d",
                 rootid);
        ex_err_fn(exoid, __func__, errmsg, status);
        goto error_ret; /* exit define mode and return */
      }

      /* define variable  */
      dims[0] = num_info_dim;
      dims[1] = lindim;

      if ((status = nc_def_var(rootid, VAR_INFO, NC_CHAR, 2, dims, &varid)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define info record in file id %d",
                 rootid);
        ex_err_fn(exoid, __func__, errmsg, status);
        goto error_ret; /* exit define mode and return */
      }
      /* In parallel, only rank=0 will write the info records.
       * Should be able to take advantage of HDF5 handling identical data on all ranks
       * or use the compact storage, but we had issues on some NFS filesystems and some
       * compilers/mpi so are doing it this way...
       */
#if defined(PARALLEL_AWARE_EXODUS)
      if (exi_is_parallel(rootid)) {
        nc_var_par_access(rootid, varid, NC_INDEPENDENT);
      }
#endif

      /*   leave define mode  */
      if ((status = exi_leavedef(rootid, __func__)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to exit define mode");
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }
    }
    else {
      if ((status = nc_inq_varid(rootid, VAR_INFO, &varid)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find info record variable in file id %d",
                 rootid);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }
    }

    if (info != NULL) {
      /* write out information records */
      for (i = 0; i < num_info; i++) {
        int length = strlen(info[i]) + 1;
        start[0]   = i;
        start[1]   = 0;

        count[0] = 1;
        count[1] = length < MAX_LINE_LENGTH ? length : MAX_LINE_LENGTH;

        if ((status = nc_put_vara_text(rootid, varid, start, count, info[i])) != NC_NOERR) {
          snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to store info record in file id %d",
                   rootid);
          ex_err_fn(exoid, __func__, errmsg, status);
          EX_FUNC_LEAVE(EX_FATAL);
        }
      }
    }
    /* PnetCDF applies setting to entire file, so put back to collective... */
#if defined(PARALLEL_AWARE_EXODUS)
    if (exi_is_parallel(rootid)) {
      nc_var_par_access(rootid, varid, NC_COLLECTIVE);
    }
#endif
  }
  EX_FUNC_LEAVE(EX_NOERR);

/* Fatal error: exit definition mode and return */
error_ret:
  exi_leavedef(rootid, __func__);
  EX_FUNC_LEAVE(EX_FATAL);
}
