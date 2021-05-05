/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * exgenm - ex_get_elem_num_map
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *
 * exit conditions -
 *       int*    elem_map                element number map array
 *
 * revision history -
 *
 *****************************************************************************/

#include "exodusII.h"

/*!
\deprecated Use ex_get_id_map()(exoid, EX_ELEM_MAP, elem_map)

The function ex_get_elem_num_map() reads the optional element number
map from the database. See Section LocalElementIds for a description of
the element number map. If an element number map is not stored in the
data file, a default array (1,2,3,. .. num_elem) is
returned. Memory must be allocated for the element number map array
({num_elem} in length) before this call is made.

\return In case of an error, ex_get_elem_num_map() returns a negative
number; a warning will return a positive number.  Possible causes of
errors include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  if an element number map is not stored, a default map and a warning value
are returned.

\param[in]   exoid     exodus file ID returned from a previous call to
ex_create() or ex_open().
\param[out]  elem_map  Returned element number map.

The following code will read an element number map from an
open exodus file :
~~~{.c}
int *elem_map, error, exoid;

\comment{read element number map}
elem_map = (int *) calloc(num_elem, sizeof(int));
error = ex_get_elem_num_map (exoid, elem_map);

\comment{Equivalent using non-deprecated function}
error = ex_get_id_map(exoid, EX_ELEM_MAP, elem_map);
~~~
 */

int ex_get_elem_num_map(int exoid, void_int *elem_map)
{
  return ex_get_id_map(exoid, EX_ELEM_MAP, elem_map);
}
