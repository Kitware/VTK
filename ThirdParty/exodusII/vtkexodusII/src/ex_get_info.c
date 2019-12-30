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
#include "exodusII_int.h" // for EX_FATAL, ex__trim, etc

/*!
  \ingroup Utilities

The function ex_get_info() reads information records from the
database. The records are #MAX_LINE_LENGTH character
strings. Memory must be allocated for the information records before
this call is made. The number of records can be determined by invoking
ex_inquire() or ex_inquire_int().

\returns In case of an error, ex_get_info() returns a negative number;
         a warning will return a positive number. Possible causes of errors
         include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  a warning value is returned if no information records were stored.

\param[in]  exoid   exodus file ID returned from a previous call to ex_create()
or ex_open().
\param[out] info    Returned array containing the information records.

The following code segment will determine the number of information
records and read them from an open exodus file :

~~~{.c}
int error, exoid, num_info;
char *info[MAXINFO];

\comment{read information records}
num_info = ex_inquire_int (exoid,EX_INQ_INFO);
for (i=0; i < num_info; i++) {
   info[i] = (char *) calloc ((EX_MAX_LINE_LENGTH+1), sizeof(char));
}
error = ex_get_info (exoid, info);
~~~

 */

int ex_get_info(int exoid, char **info)
{
  int    status;
  size_t i;
  int    dimid, varid;
  size_t num_info, start[2], count[2];
  char   errmsg[MAX_ERR_LENGTH];

  int rootid = exoid & EX_FILE_ID_MASK;

  EX_FUNC_ENTER();
  ex__check_valid_file_id(exoid, __func__);

  /* inquire previously defined dimensions and variables  */
  if ((status = nc_inq_dimid(rootid, DIM_NUM_INFO, &dimid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "Warning: failed to locate number of info records in file id %d", rootid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_WARN);
  }

  if ((status = nc_inq_dimlen(rootid, dimid, &num_info)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get number of info records in file id %d",
             rootid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* do this only if there are any information records */
  if (num_info > 0) {
    if ((status = nc_inq_varid(rootid, VAR_INFO, &varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate info record data in file id %d",
               rootid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* read the information records */
    for (i = 0; i < num_info; i++) {
      start[0] = i;
      count[0] = 1;
      start[1] = 0;
      count[1] = MAX_LINE_LENGTH + 1;

      if ((status = nc_get_vara_text(rootid, varid, start, count, info[i])) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get info record data in file id %d",
                 rootid);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }
      info[i][MAX_LINE_LENGTH] = '\0';
      ex__trim(info[i]);
    }
  }
  EX_FUNC_LEAVE(EX_NOERR);
}
