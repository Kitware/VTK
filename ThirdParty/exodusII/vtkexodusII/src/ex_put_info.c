/*
 * Copyright (c) 2005-2017 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of NTESS nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
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

int ex_put_info(int exoid, int num_info, char *info[])
{
  int    status;
  int    i, lindim, num_info_dim, dims[2], varid;
  size_t start[2], count[2];
  char   errmsg[MAX_ERR_LENGTH];

  int rootid = exoid & EX_FILE_ID_MASK;

  EX_FUNC_ENTER();
  ex__check_valid_file_id(exoid, __func__);

  /* only do this if there are records */
  if (num_info > 0) {

    /* See if the number of info records has already been defined.
       Assume that if the DIM_NUM_INFO dimension exists, then the
       VAR_INFO variable also exists...
     */
    status = nc_inq_dimid(rootid, DIM_NUM_INFO, &num_info_dim);
    if (status != NC_NOERR) {

      /* put file into define mode  */
      if ((status = nc_redef(rootid)) != NC_NOERR) {
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
      ex__compress_variable(rootid, varid, 3);

      /*   leave define mode  */
      if ((status = ex__leavedef(rootid, __func__)) != NC_NOERR) {
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
    else if (ex__is_parallel(rootid)) {
      /* All processors need to call nc_put_vara_text in case in a global
       * collective mode */
      char dummy[] = " ";
      for (i = 0; i < num_info; i++) {
        start[0] = start[1] = 0;
        count[0] = count[1] = 0;
        nc_put_vara_text(rootid, varid, start, count, dummy);
      }
    }
  }
  EX_FUNC_LEAVE(EX_NOERR);

/* Fatal error: exit definition mode and return */
error_ret:
  ex__leavedef(rootid, __func__);
  EX_FUNC_LEAVE(EX_FATAL);
}
