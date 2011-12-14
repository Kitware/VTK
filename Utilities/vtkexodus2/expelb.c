/*
 * Copyright (c) 2006 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000 with Sandia Corporation, the U.S. Governement
 * retains certain rights in this software.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 * 
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.  
 * 
 *     * Neither the name of Sandia Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
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

#include "exodusII.h"

/*!
\deprecated Use ex_put_block()(exoid, EX_ELEM_BLOCK, elem_blk_id, elem_type, num_elem_this_blk, num_nodes_per_elem, 0, 0, num_attr_per_elem)

The function ex_put_elem_block() writes the parameters used to
describe an element block.

\return In case of an error, ex_put_elem_block() returns a negative
number; a warning will return a positive number. Possible causes of
errors include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  data file opened for read only.
  -  data file not initialized properly with call to ex_put_init().
  -  an element block with the same ID has already been specified.
  -  the number of element blocks specified in the call to ex_put_init() has been exceeded.

\param[in] exoid              exodus file ID returned from a previous call to ex_create() or ex_open().
\param[in] elem_blk_id        The element block ID.
\param[in] elem_type         The type of elements in the element block. The maximum length of this
                              string is \p MAX_STR_LENGTH .
\param[in] num_elem_this_blk  The number of elements in the element block.
\param[in] num_nodes_per_elem The number of nodes per element in the element block.
\param[in] num_attr_per_elem  The number of attributes per element in the element block.

For example, the following code segment will initialize an element
block with an ID of 10, write out the connectivity array, and write
out the element attributes array:

\code
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

\comment{fill connect with node numbers; nodes for first elemen}
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

\endcode

 */

int ex_put_elem_block (int   exoid,
                       int   elem_blk_id,
                       const char *elem_type,
                       int   num_elem_this_blk,
                       int   num_nodes_per_elem,
                       int   num_attr_per_elem)
{
  return ex_put_block( exoid, EX_ELEM_BLOCK, elem_blk_id,
    elem_type, num_elem_this_blk, num_nodes_per_elem,
    0 /*num_edge_per_elem*/, 0 /*num_face_per_elem*/, num_attr_per_elem );
}
