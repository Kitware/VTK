/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h"

/*!
\deprecated Use ex_get_attr()(exoid, EX_ELEM_BLOCK, elem_blk_id, attrib) instead

The function ex_get_elem_attr() reads the attributes for an element
block. Memory must be allocated for(num_attr x num_elem_this_blk)
attributes before this routine is called.

Because the attributes are floating point values, the application code
must declare the array passed to be the appropriate type (float or
double) to match the compute word size passed in ex_create() or
ex_open().

\return In case of an error, ex_get_elem_attr() returns a negative
number; a warning will return a positive number.  Possible causes of
errors include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  invalid element block ID.
  -  a warning value is returned if no attributes are stored in the file.

\param[in]  exoid         exodus file ID returned from a previous call to
ex_create() or ex_open().
\param[in]  elem_blk_id   The element block ID.
\param[out] attrib        Size [num_elem_this_blk*num_attr].
                          Returned list of(num_attr x num_elem_this_blk)
attributes for
                          the element block, with the num_attr index cycling
faster.

Refer to the code example in ex_get_elem_block() for an example
of reading the element attributes for an element block.
 */

int ex_get_elem_attr(int exoid, ex_entity_id elem_blk_id, void *attrib)
{
  return ex_get_attr(exoid, EX_ELEM_BLOCK, elem_blk_id, attrib);
}
