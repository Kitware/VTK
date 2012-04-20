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

/*!
The function ex_get_qa() reads the QA records from the database. Each
QA record contains four \c MAX_STR_LENGTH-byte character
strings. The character strings are:
 -  the analysis code name
 -  the analysis code QA descriptor
 -  the analysis date
 -  the analysis time

Memory must be allocated for the QA records before this call is
made. The number of QA records can be determined by invoking
ex_inquire().

\return In case of an error, ex_get_qa() returns a negative number; a
        warning will return a positive number.  Possible causes of errors
        include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  a warning value is returned if no QA records were stored.

\param[in] exoid          exodus file ID returned from a previous call to ex_create() or ex_open().
\param[out]  qa_record    Returned array containing the QA records.

The following will determine the number of QA records and 
read them from the open exodus file:

\code
#include "exodusII.h"
int num_qa_rec, error, exoid
char *qa_record[MAX_QA_REC][4];

\comment{read QA records}
num_qa_rec = ex_inquire_int(exoid, EX_INQ_QA);

for (i=0; i<num_qa_rec; i++) {
    for (j=0; j<4; j++)
    qa_record[i][j] = (char *) calloc ((MAX_STR_LENGTH+1), sizeof(char));
}
error = ex_get_qa (exoid, qa_record);
\endcode

 */

int ex_get_qa (int exoid,
               char *qa_record[][4])
{
  int status;
  int dimid, varid;
  size_t i, j;
  size_t num_qa_records, start[3], count[3];

  char errmsg[MAX_ERR_LENGTH];

  exerrval = 0; /* clear error code */

  /* inquire previously defined dimensions and variables  */
  if ((status = nc_inq_dimid(exoid, DIM_NUM_QA, &dimid)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
            "Warning: no qa records stored in file id %d", 
            exoid);
    ex_err("ex_get_qa",errmsg,exerrval);
    return (EX_WARN);
  }

  if ((status = nc_inq_dimlen(exoid, dimid, &num_qa_records)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
            "Error: failed to get number of qa records in file id %d",
      exoid);
    ex_err("ex_get_qa",errmsg,exerrval);
    return (EX_FATAL);
  }


  /* do this only if there are any QA records */
  if (num_qa_records > 0) {
    if ((status = nc_inq_varid(exoid, VAR_QA_TITLE, &varid)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
        "Error: failed to locate qa record data in file id %d", exoid);
      ex_err("ex_get_qa",errmsg,exerrval);
      return (EX_FATAL);
    }


    /* read the QA records */
    for (i=0; i<num_qa_records; i++) {
      for (j=0; j<4; j++) {
  start[0] = i; count[0] = 1;
  start[1] = j; count[1] = 1;
  start[2] = 0; count[2] = MAX_STR_LENGTH+1;
  if ((status = nc_get_vara_text(exoid, varid, start, count, qa_record[i][j])) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
      "Error: failed to get qa record data in file id %d", exoid);
    ex_err("ex_get_qa",errmsg,exerrval);
    return (EX_FATAL);
  }
  qa_record[i][j][MAX_STR_LENGTH] = '\0';
  ex_trim_internal(qa_record[i][j]);
      }
    }
  }
  return (EX_NOERR);
}
