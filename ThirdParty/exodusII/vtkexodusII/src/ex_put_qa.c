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
#include "exodusII_int.h" // for EX_FATAL, DIM_NUM_QA, etc

/*!
\ingroup Utilities

The function ex_put_qa() writes the QA records to the database. Each
QA record contains four #MAX_STR_LENGTH byte character
strings. The character strings are:

 -  the analysis code name
 -  the analysis code QA descriptor
 -  the analysis date
 -  the analysis time

\return In case of an error, ex_put_qa() returns a negative number; a
        warning will return a positive number. Possible causes of errors
        include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  data file opened for read only.
  -  QA records already exist in file.

\param[in] exoid            exodus file ID returned from a previous call to
ex_create() or ex_open().

\param[in] num_qa_records   The number of QA records.
\param[in]  qa_record       Array containing the QA records. To only
                            define the number of qa records instead of
                            defining and outputting, pass NULL for
                            qa_record argument.

The following code segment will write out two QA records:

~~~{.c}
int num_qa_rec, error, exoid;
char *qa_record[2][4];

\comment{write QA records}
num_qa_rec = 2;

qa_record[0][0] = "TESTWT1";
qa_record[0][1] = "testwt1";
qa_record[0][2] = "07/07/93";
qa_record[0][3] = "15:41:33";
qa_record[1][0] = "FASTQ";
qa_record[1][1] = "fastq";
qa_record[1][2] = "07/07/93";
qa_record[1][3] = "16:41:33";

error = ex_put_qa (exoid, num_qa_rec, qa_record);
~~~

 */

int ex_put_qa(int exoid, int num_qa_records, char *qa_record[][4])
{
  int    status;
  int    i, j, strdim, num_qa_dim, varid, n4dim;
  int    dims[3];
  size_t start[3], count[3];
  char   errmsg[MAX_ERR_LENGTH];

  EX_FUNC_ENTER();
  int rootid = exoid & EX_FILE_ID_MASK;

  ex__check_valid_file_id(exoid, __func__);

  /* only do this if there are records */

  if (num_qa_records > 0) {
    /* See if the number of qa records has already been defined.
       Assume that if the DIM_NUM_QA dimension exists, then the
       VAR_QA_TITLE variable also exists...
     */
    status = nc_inq_dimid(rootid, DIM_NUM_QA, &num_qa_dim);
    if (status != NC_NOERR) {

      /*   put file into define mode  */
      if ((status = nc_redef(rootid)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to put file id %d into define mode",
                 rootid);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }

      /*   define dimensions */
      if ((status = nc_def_dim(rootid, DIM_NUM_QA, num_qa_records, &num_qa_dim)) != NC_NOERR) {
        if (status == NC_ENAMEINUSE) { /* duplicate entry? */
          snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: qa records already exist in file id %d", rootid);
          ex_err_fn(exoid, __func__, errmsg, status);
        }
        else {
          snprintf(errmsg, MAX_ERR_LENGTH,
                   "ERROR: failed to define qa record array size in file id %d", rootid);
          ex_err_fn(exoid, __func__, errmsg, status);
        }

        goto error_ret; /* exit define mode and return */
      }

      /* create number "4" dimension; must be of type long */
      if ((status = nc_def_dim(rootid, DIM_N4, 4L, &n4dim)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to define number \"4\" dimension in file id %d", rootid);
        ex_err_fn(exoid, __func__, errmsg, status);
        goto error_ret; /* exit define mode and return */
      }

      /* create string length dimension -- only used for QA records */
      if ((status = nc_def_dim(rootid, DIM_STR, (MAX_STR_LENGTH + 1), &strdim)) != NC_NOERR) {
        if (status == NC_ENAMEINUSE) { /* already defined */
          nc_inq_dimid(rootid, DIM_STR, &strdim);
        }
        else {
          snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define string length in file id %d",
                   rootid);
          ex_err_fn(exoid, __func__, errmsg, status);
          goto error_ret; /* exit define mode and return */
        }
      }

      /*   define variable  */
      dims[0] = num_qa_dim;
      dims[1] = n4dim;
      dims[2] = strdim;

      if ((status = nc_def_var(rootid, VAR_QA_TITLE, NC_CHAR, 3, dims, &varid)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define qa record array in file id %d",
                 rootid);
        ex_err_fn(exoid, __func__, errmsg, status);
        goto error_ret; /* exit define mode and return */
      }

      /*   leave define mode  */
      if ((status = ex__leavedef(rootid, __func__)) != NC_NOERR) {
        EX_FUNC_LEAVE(EX_FATAL);
      }
    }
    else {
      if ((status = nc_inq_varid(rootid, VAR_QA_TITLE, &varid)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find qa records variable in file id %d",
                 rootid);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }
    }

    if (qa_record != NULL) {
      /*   write out QA records */

      for (i = 0; i < num_qa_records; i++) {
        for (j = 0; j < 4; j++) {
          start[0] = i;
          start[1] = j;
          start[2] = 0;

          count[0] = 1;
          count[1] = 1;
          count[2] = strlen(qa_record[i][j]) + 1;

          if ((status = nc_put_vara_text(rootid, varid, start, count, qa_record[i][j])) !=
              NC_NOERR) {
            snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to store qa record in file id %d",
                     rootid);
            ex_err_fn(exoid, __func__, errmsg, status);
            EX_FUNC_LEAVE(EX_FATAL);
          }
        }
      }
    }
    else if (ex__is_parallel(rootid)) {
      /* In case we are in a collective mode, all processors need to call */
      const char dummy[] = " ";
      for (i = 0; i < num_qa_records; i++) {
        for (j = 0; j < 4; j++) {
          start[0] = start[1] = start[2] = 0;
          count[0] = count[1] = count[2] = 0;
          nc_put_vara_text(rootid, varid, start, count, dummy);
        }
      }
    }
  }
  EX_FUNC_LEAVE(EX_NOERR);

/* Fatal error: exit definition mode and return */
error_ret:
  ex__leavedef(rootid, __func__);
  EX_FUNC_LEAVE(EX_FATAL);
}
