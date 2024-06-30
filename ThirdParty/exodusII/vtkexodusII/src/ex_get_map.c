/*
 * Copyright(C) 1999-2022 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_NOERR, EX_FATAL, etc

/*!
The function ex_get_map() reads the element order map
from the database. If an element order map is not stored in the data
file, a default array (1,2,3,. .. num_elem) is returned. Memory
must be allocated for the element map array ({num_elem} in length)
before this call is made.

\return In case of an error, ex_get_map() returns a negative number; a
warning will return a positive number. Possible causes of errors
include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  if an element order map is not stored, a default map and a
     warning value are returned.

\param[in]  exoid      exodus file ID returned from a previous call to
ex_create() or ex_open().
\param[out] elem_map   Returned element order map.

The following code will read an element order map from an open exodus
file :

~~~{.c}
int *elem_map, error, exoid;

\comment{read element order map}
elem_map = (int *)calloc(num_elem, sizeof(int));

error = ex_get_map(exoid, elem_map);
~~~

 */

int ex_get_map(int exoid, void_int *elem_map)
{
  int status;

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* inquire id's of previously defined dimensions and variables  */

  /* See if file contains any elements...*/
  int numelemdim;
  if (nc_inq_dimid(exoid, DIM_NUM_ELEM, &numelemdim) != NC_NOERR) {
    EX_FUNC_LEAVE(EX_NOERR);
  }

  size_t num_elem;
  if ((status = nc_inq_dimlen(exoid, numelemdim, &num_elem)) != NC_NOERR) {
    char errmsg[MAX_ERR_LENGTH];
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get number of elements in file id %d",
             exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  int mapid;
  if (nc_inq_varid(exoid, VAR_MAP, &mapid) != NC_NOERR) {
    /* generate default map of 1..n, where n is num_elem */
    if (ex_int64_status(exoid) & EX_MAPS_INT64_API) {
      int64_t *lmap = (int64_t *)elem_map;
      for (size_t i = 0; i < num_elem; i++) {
        lmap[i] = i + 1;
      }
    }
    else {
      int *lmap = (int *)elem_map;
      for (size_t i = 0; i < num_elem; i++) {
        lmap[i] = i + 1;
      }
    }

    EX_FUNC_LEAVE(EX_NOERR);
  }

  /* read in the element order map  */
  if (ex_int64_status(exoid) & EX_MAPS_INT64_API) {
    status = nc_get_var_longlong(exoid, mapid, elem_map);
  }
  else {
    status = nc_get_var_int(exoid, mapid, elem_map);
  }

  if (status != NC_NOERR) {
    char errmsg[MAX_ERR_LENGTH];
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get element order map in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  EX_FUNC_LEAVE(EX_NOERR);
}
