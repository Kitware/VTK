/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h"

/*!
\deprecated Use ex_put_attr()(exoid, EX_ELEM_BLOCK, elem_blk_id, attrib)

The function ex_put_elem_attr() writes the attributes for an element
block. Each element in the element block must have the same number of
attributes, so there are(num_attr x num_elem_this_blk)
attributes for each element block. The function ex_put_elem_block()
must be invoked before this call is made.

Because the attributes are floating point values, the application code
must declare the array passed to be the appropriate type (float or
double) to match the compute word size passed in ex_create() or
ex_open().

\return In case of an error, ex_put_elem_attr() returns a negative
number; a warning will return a positive number. Possible causes of
errors include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  data file opened for read only.
  -  data file not initialized properly with call to ex_put_init().
  -  ex_put_elem_block() was not called previously for specified element block
ID.
  -  ex_put_elem_block() was called with 0 attributes specified.

\param[in]  exoid       exodus file ID returned from a previous call to
ex_create() or ex_open().
\param[in] elem_blk_id  The element block ID.
\param[in] attrib       Size [num_elem_this_blk*num_attr]
                        The list of attributes for the element block. The
num_attr
                        index cycles faster.

Refer to the code example in ex_put_elem_block() for an example of
writing the attributes array for an element block.

*/

int ex_put_elem_attr(int exoid, ex_entity_id elem_blk_id, const void *attrib)
{
  return ex_put_attr(exoid, EX_ELEM_BLOCK, elem_blk_id, attrib);
}
