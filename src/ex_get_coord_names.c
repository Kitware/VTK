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

The function ex_get_coord_names() reads the names of the coordinate
arrays from the database. Memory must be allocated for the character
strings before this function is invoked.

\return In case of an error, ex_get_coord_names() returns a negative
number; a warning will return a positive number.  Possible causes of
errors include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  a warning value is returned if coordinate names were not stored.

\param[in]   exoid        exodus file ID returned from a previous call to
ex_create() or ex_open().
\param[out]  coord_names  Returned pointer to a vector containing num_dim
names of the nodal
                          coordinate arrays.

The following code segment will read the coordinate names from an open
exodus file :

~~~{.c}
int error, exoid;
char *coord_names[3];

for (i=0; i < num_dim; i++) {
   coord_names[i] = (char *)calloc((..size of names...+1), sizeof(char));
}

error = ex_get_coord_names (exoid, coord_names);
~~~

*/

int ex_get_coord_names(int exoid, char **coord_names)
{
  int    status;
  int    ndimdim, varid;
  size_t num_dim;
  char   errmsg[MAX_ERR_LENGTH];

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* inquire previously defined dimensions and variables  */

  if ((status = nc_inq_dimid(exoid, DIM_NUM_DIM, &ndimdim)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate number of dimensions in file id %d",
             exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if ((status = nc_inq_dimlen(exoid, ndimdim, &num_dim)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get number of dimensions in file id %d",
             exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if ((status = nc_inq_varid(exoid, VAR_NAME_COOR, &varid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "Warning: failed to locate coordinate names in file id %d",
             exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_WARN);
  }

  /* read the coordinate names */
  status = exi_get_names(exoid, varid, num_dim, coord_names, EX_COORDINATE, __func__);
  if (status != NC_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }
  EX_FUNC_LEAVE(EX_NOERR);
}
