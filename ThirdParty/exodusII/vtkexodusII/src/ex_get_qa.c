/*
 * Copyright(C) 1999-2021 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, exi_trim, etc

/*!
\ingroup Utilities

The function ex_get_qa() reads the QA records from the database. Each
QA record contains four #MAX_STR_LENGTH byte character
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

\param[in] exoid          exodus file ID returned from a previous call to
ex_create() or ex_open().
\param[out]  qa_record    Returned array containing the QA records.

The following will determine the number of QA records and
read them from the open exodus file:

~~~{.c}
int num_qa_rec, error, exoid
char *qa_record[MAX_QA_REC][4];

\comment{read QA records}
num_qa_rec = ex_inquire_int(exoid, EX_INQ_QA);

for (i=0; i<num_qa_rec; i++) {
    for (j=0; j<4; j++)
    qa_record[i][j] = (char *) calloc ((MAX_STR_LENGTH+1), sizeof(char));
}
error = ex_get_qa (exoid, qa_record);
~~~

 */

int ex_get_qa(int exoid, char *qa_record[][4])
{
  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* inquire previously defined dimensions and variables  */
  int status;
  int rootid = exoid & EX_FILE_ID_MASK;
  int dimid;
  if ((status = nc_inq_dimid(rootid, DIM_NUM_QA, &dimid)) != NC_NOERR) {
    char errmsg[MAX_ERR_LENGTH];
    snprintf(errmsg, MAX_ERR_LENGTH, "Warning: no qa records stored in file id %d", rootid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_WARN);
  }

  size_t num_qa_records;
  if ((status = nc_inq_dimlen(rootid, dimid, &num_qa_records)) != NC_NOERR) {
    char errmsg[MAX_ERR_LENGTH];
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get number of qa records in file id %d",
             rootid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* do this only if there are any QA records */
  if (num_qa_records > 0) {
    int varid;
    if ((status = nc_inq_varid(rootid, VAR_QA_TITLE, &varid)) != NC_NOERR) {
      char errmsg[MAX_ERR_LENGTH];
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate qa record data in file id %d",
               rootid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* read the QA records */
    for (size_t i = 0; i < num_qa_records; i++) {
      for (size_t j = 0; j < 4; j++) {
        size_t start[] = {i, j, 0};
        size_t count[] = {1, 1, MAX_STR_LENGTH + 1};
        if ((status = nc_get_vara_text(rootid, varid, start, count, qa_record[i][j])) != NC_NOERR) {
          char errmsg[MAX_ERR_LENGTH];
          snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get qa record data in file id %d",
                   rootid);
          ex_err_fn(exoid, __func__, errmsg, status);
          EX_FUNC_LEAVE(EX_FATAL);
        }
        qa_record[i][j][MAX_STR_LENGTH] = '\0';
        exi_trim(qa_record[i][j]);
      }
    }
  }
  EX_FUNC_LEAVE(EX_NOERR);
}
