/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * expelb - ex_put_elem_block: write element block parameters
 *
 * entry conditions -
 *   input parameters:
 *       int     idexo                   exodus file id
 *       int     elem_blk_id             block identifier
 *       char*   elem_type               element type string
 *       int     num_elem_this_blk       number of elements in the element blk
 *       int     num_nodes_per_elem      number of nodes per element block
 *       int     num_attr_per_elem       number of attributes per element
 *
 * exit conditions -
 *
 *
 *****************************************************************************/

#include "exodusII.h" // for ex_put_block, ex_entity_id, etc

/*!
\deprecated Use ex_put_block()(exoid, EX_ELEM_BLOCK, elem_blk_id, elem_type,
num_elem_this_blk, num_nodes_per_elem, 0, 0, num_attr_per_elem)

The function ex_put_elem_block() writes the parameters used to
describe an element block.

\return In case of an error, ex_put_elem_block() returns a negative
number; a warning will return a positive number. Possible causes of
errors include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  data file opened for read only.
  -  data file not initialized properly with call to ex_put_init().
  -  an element block with the same ID has already been specified.
  -  the number of element blocks specified in the call to ex_put_init() has
been exceeded.

\param[in] exoid              exodus file ID returned from a previous call to
ex_create() or ex_open().
\param[in] elem_blk_id        The element block ID.
\param[in] elem_type         The type of elements in the element block. The
maximum length of this
                              string is \p MAX_STR_LENGTH .
\param[in] num_elem_this_blk  The number of elements in the element block.
\param[in] num_nodes_per_elem The number of nodes per element in the element
block.
\param[in] num_attr_per_elem  The number of attributes per element in the
element block.

For example, the following code segment will initialize an element
block with an ID of 10, write out the connectivity array, and write
out the element attributes array:

~~~{.c}
int id, error, exoid, num_elem_in_blk, num_nodes_per_elem, *connect, num_attr;
float *attrib;

\comment{write element block parameters}
id = 10;
num_elem_in_blk = 2;
num_nodes_per_elem = 4;   \comment{elements are 4-node shells}
num_attr = 1;             \comment{one attribute per element}

error = ex_put_elem_block(exoid, id, "SHELL", num_elem_in_blk,
                          num_nodes_per_elem, num_attr);

\comment{write element connectivity}
connect = (int *)calloc(num_elem_in_blk*num_nodes_per_elem, sizeof(int));

\comment{fill connect with node numbers; nodes for first element}
connect[0] = 1; connect[1] = 2; connect[2] = 3; connect[3] = 4;

\comment{nodes for second element}
connect[4] = 5; connect[5] = 6; connect[6] = 7; connect[7] = 8;

error = ex_put_elem_conn (exoid, id, connect);

\comment{write element block attributes}
attrib = (float *) calloc (num_attr*num_elem_in_blk, sizeof(float));

for (i=0, cnt=0; i < num_elem_in_blk; i++) {
   for (j=0; j < num_attr; j++, cnt++) {
      attrib[cnt] = 1.0;
   }
}

error = ex_put_elem_attr (exoid, id, attrib);

\comment{Same result using non-deprecated code}
error = ex_put_block(exoid, EX_ELEM_BLOCK, id, "SHELL", num_elem_in_blk,
                          num_nodes_per_elem, 0, 0, num_attr);
error = ex_put_conn (exoid, EX_ELEM_BLOCK, id, connect);
error = ex_put_attr (exoid, EX_ELEM_BLOCK, id, attrib);

~~~

 */

int ex_put_elem_block(int exoid, ex_entity_id elem_blk_id, const char *elem_type,
                      int64_t num_elem_this_blk, int64_t num_nodes_per_elem,
                      int64_t num_attr_per_elem)
{
  return ex_put_block(exoid, EX_ELEM_BLOCK, elem_blk_id, elem_type, num_elem_this_blk,
                      num_nodes_per_elem, 0 /*num_edge_per_elem*/, 0 /*num_face_per_elem*/,
                      num_attr_per_elem);
}
