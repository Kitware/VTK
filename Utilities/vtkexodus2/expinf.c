/*
 * Copyright (c) 2005 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000 with Sandia Corporation, the U.S. Governement
 * retains certain rights in this software.
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
 *     * Neither the name of Sandia Corporation nor the names of its
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

#include "exodusII.h"
#include "exodusII_int.h"
#include <string.h>

/*!
The function ex_put_info() writes information records to the
database. The records are \c MAX_LINE_LENGTH-character strings.

In case of an error, ex_put_info() returns a negative number;
a warning will return a positive number. Possible causes of errors
include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  data file opened for read only.
  -  information records already exist in file.

\param[in] exoid       exodus file ID returned from a previous call to ex_create() or ex_open().
\param[in] num_info    The number of information records.
\param[in] info        Array containing the information records.

The following code will write out three information records 
to an open exodus file -

\code
#include "exodusII.h"
int error, exoid, num_info;
char *info[3];

\comment{write information records}
num_info = 3;

info[0] = "This is the first information record.";
info[1] = "This is the second information record.";
info[2] = "This is the third information record.";

error = ex_put_info(exoid, num_info, info);
\endcode

 */

int ex_put_info (int   exoid, 
                 int   num_info,
                 char *info[])
{
  int status;
  int i, lindim, num_info_dim, dims[2], varid;
  size_t start[2], count[2];
  char errmsg[MAX_ERR_LENGTH];

  exerrval = 0; /* clear error code */

  /* only do this if there are records */
  if (num_info > 0) {

    /*   inquire previously defined dimensions  */
    if ((status = nc_inq_dimid(exoid, DIM_LIN, &lindim)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
              "Error: failed to get line string length in file id %d", exoid);
      ex_err("ex_put_info",errmsg,exerrval);
      return (EX_FATAL);
    }

    /* put file into define mode  */
    if ((status = nc_redef (exoid)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
              "Error: failed put file id %d into define mode", exoid);
      ex_err("ex_put_info",errmsg,exerrval);
      return (EX_FATAL);
    }

    /* define dimensions */
    if ((status = nc_def_dim(exoid, DIM_NUM_INFO, num_info, &num_info_dim)) != NC_NOERR) {
      if (status == NC_ENAMEINUSE) {     /* duplicate entry? */
  exerrval = status;
  sprintf(errmsg,
    "Error: info records already exist in file id %d", 
    exoid);
  ex_err("ex_put_info",errmsg,exerrval);
      } else {
  exerrval = status;
  sprintf(errmsg,
    "Error: failed to define number of info records in file id %d",
    exoid);
  ex_err("ex_put_info",errmsg,exerrval);
      }

      goto error_ret;         /* exit define mode and return */
    }

    /* define variable  */
    dims[0] = num_info_dim;
    dims[1] = lindim;

    if ((status = nc_def_var(exoid, VAR_INFO, NC_CHAR, 2, dims, &varid)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
        "Error: failed to define info record in file id %d",
        exoid);
      ex_err("ex_put_info",errmsg,exerrval);
      goto error_ret;         /* exit define mode and return */
    }

    /*   leave define mode  */
    if ((status = nc_enddef (exoid)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
        "Error: failed to complete info record definition in file id %d",
        exoid);
      ex_err("ex_put_info",errmsg,exerrval);
      return (EX_FATAL);
    }


    /* write out information records */
    for (i=0; i<num_info; i++) {
      int length = strlen(info[i]);
      start[0] = i;
      start[1] = 0;

      count[0] = 1;
      count[1] = length < MAX_LINE_LENGTH ? length : MAX_LINE_LENGTH;

      if ((status = nc_put_vara_text(exoid, varid, start, count, info[i])) != NC_NOERR) {
  exerrval = status;
  sprintf(errmsg,
    "Error: failed to store info record in file id %d",
    exoid);
  ex_err("ex_put_info",errmsg,exerrval);
  return (EX_FATAL);
      }
    }
  }

  return (EX_NOERR);

  /* Fatal error: exit definition mode and return */
 error_ret:
  if (nc_enddef (exoid) != NC_NOERR) {     /* exit define mode */
    sprintf(errmsg,
      "Error: failed to complete definition for file id %d",
      exoid);
    ex_err("ex_put_info",errmsg,exerrval);
  }
  return (EX_FATAL);
}
