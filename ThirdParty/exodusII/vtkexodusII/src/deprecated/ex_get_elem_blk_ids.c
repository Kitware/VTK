/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h"

/*!
\deprecated Use ex_get_ids()(exoid, EX_ELEM_BLOCK, ids) instead

The function ex_get_elem_blk_ids() reads the IDs of all of the element
blocks. Memory must be allocated for the returned array of
({num_elem_blk}) IDs before this function is invoked. The required
size(num_elem_blk) can be determined via a call to ex_inquire() or
ex_inquire_int().

\return In case of an error, ex_get_elem_blk_ids() returns a negative
number; a warning will return a positive number.  Possible causes of
errors include:
  -  data file not properly opened with call to ex_create() or ex_open()

\param[in]   exoid         exodus file ID returned from a previous call to
ex_create() or ex_open().
\param[out]  ids           Returned array of the element blocks IDs. The order
of the IDs in this
                           array reflects the sequence that the element blocks
were introduced
                           into the file.

The following code segment reads all the element block IDs:

~~~{.c}
int error, exoid, *idelbs, num_elem_blk;
idelbs = (int *) calloc(num_elem_blk, sizeof(int));

error = ex_get_elem_blk_ids (exoid, idelbs);

\comment{Same result using non-deprecated functions}
error = ex_get_ids (exoid, EX_ELEM_BLOCK, idelbs);

~~~

 */

int ex_get_elem_blk_ids(int exoid, void_int *ids)
{
  /* ex_get_elem_blk_ids should be deprecated. */
  return ex_get_ids(exoid, EX_ELEM_BLOCK, ids);
}
