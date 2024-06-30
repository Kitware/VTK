/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, etc

/*!

The function ex_put_coord_names() writes the names of the coordinate
arrays to the database. Memory must be allocated for the character
strings before this function is invoked.

In case of an error, ex_put_coord_names() returns a negative number; a
warning will return a positive number.  Possible causes of errors
include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  data file opened for read only.
  -  data file not initialized properly with call to ex_put_init().

\param[in] exoid          exodus file ID returned from a previous call to
ex_create() or ex_open().
\param[in] coord_names    Array containing num_dim names of length \p
...max_name_length...                         of the nodal coordinate arrays.

The following coding will write the coordinate names to an
open exodus file :

~~~{.c}
int error, exoid;

char *coord_names[3];
coord_names[0] = "xcoor";
coord_names[1] = "ycoor";
coord_names[2] = "zcoor";

error = ex_put_coord_names (exoid, coord_names);
~~~

 */

int ex_put_coord_names(int exoid, char *const coord_names[])
{
  int    status;
  int    ndimdim, varid;
  size_t num_dim;
  char   errmsg[MAX_ERR_LENGTH];

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if ((status = nc_inq_dimid(exoid, DIM_NUM_DIM, &ndimdim)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate number of dimensions in file id %d",
             exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if ((status = nc_inq_dimlen(exoid, ndimdim, &num_dim)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: inquire failed to get number of dimensions in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if ((status = nc_inq_varid(exoid, VAR_NAME_COOR, &varid)) == -1) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate coordinate names in file id %d",
             exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* write out coordinate names */
  status = exi_put_names(exoid, varid, num_dim, coord_names, EX_COORDINATE, "", __func__);

  EX_FUNC_LEAVE(status);
}
