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
  int status;

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* inquire previously defined dimensions and variables  */
  int rootid = exoid & EX_FILE_ID_MASK;
  int dimid;
  if ((status = nc_inq_dimid(rootid, DIM_NUM_INFO, &dimid)) != NC_NOERR) {
    char errmsg[MAX_ERR_LENGTH];
    snprintf(errmsg, MAX_ERR_LENGTH,
             "Warning: failed to locate number of info records in file id %d", rootid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_WARN);
  }

  size_t num_info;
  if ((status = nc_inq_dimlen(rootid, dimid, &num_info)) != NC_NOERR) {
    char errmsg[MAX_ERR_LENGTH];
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get number of info records in file id %d",
             rootid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* do this only if there are any information records */
  if (num_info > 0) {
    int varid;
    if ((status = nc_inq_varid(rootid, VAR_INFO, &varid)) != NC_NOERR) {
      char errmsg[MAX_ERR_LENGTH];
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate info record data in file id %d",
               rootid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* read the information records */
    for (size_t i = 0; i < num_info; i++) {
      size_t start[] = {i, 0};
      size_t count[] = {1, MAX_LINE_LENGTH + 1};

      if ((status = nc_get_vara_text(rootid, varid, start, count, info[i])) != NC_NOERR) {
        char errmsg[MAX_ERR_LENGTH];
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get info record data in file id %d",
                 rootid);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }
      info[i][MAX_LINE_LENGTH] = '\0';
      exi_trim(info[i]);
    }
  }
  EX_FUNC_LEAVE(EX_NOERR);
}
