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
#include "exodusII.h"

/**
\deprecated Use ex_get_block()(exoid, EX_ELEM_BLOCK, elem_blk_id, elem_type, num_elem_this_blk, num_nodes_per_elem, num_attr) instead

The function ex_get_elem_block() reads the parameters used to describe
an element block. IDs of all element blocks stored can be determined
by calling ex_get_elem_blk_ids().

\return In case of an error, ex_get_elem_block() returns a negative
number; a warning will return a positive number. Possible causes of
errors include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  element block with specified ID is not stored in the data file.

\param[in]  exoid              exodus file ID returned from a previous call to ex_create() or ex_open().
\param[in]  elem_blk_id        The element block ID.
\param[out] elem_type          Returned element typetype of elements in the element block. 
                               The maximum length of this string is \p MAX_STR_LENGTH .
\param[out] num_elem_this_blk  Returned number of elements in the element block.
\param[out] num_nodes_per_elem Returned number of nodes per element in the element block.
\param[out] num_attr           Returned number of attributes per element in the element block.

As an example, the following code segment will read the parameters for
the element block with an ID of 10 and read the connectivity and
element attributes arrays from an open exodus file :

\code
#include "exodusII.h"
int id, error, exoid, num_el_in_blk, num_nod_per_el, num_attr, 
    *connect;
float *attrib;
char elem_type[MAX_STR_LENGTH+1];

\comment{read element block parameters}
id = 10;

error = ex_get_elem_block(exoid, id, elem_type, &num_el_in_blk, 
                          &num_nod_per_elem, &num_attr);

\comment{read element connectivity}
connect = (int *) calloc(num_nod_per_el*num_el_in_blk, 
                         sizeof(int));

error = ex_get_elem_conn(exoid, id, connect);

\comment{read element block attributes}
attrib = (float *) calloc (num_attr * num_el_in_blk, sizeof(float));
error = ex_get_elem_attr (exoid, id, attrib);

\comment{Same result using non-deprecated functions}
error = ex_get_block(exoid, EX_ELEM_BLOCK, id, elem_type, &num_el_in_blk, 
                     &num_nod_per_elem, 0, 0, &num_attr);
error = ex_get_conn (exoid, EX_ELEM_BLOCK, id, connect);
error = ex_get_attr (exoid, EX_ELEM_BLOCK, id, attrib);

\endcode

 */

int ex_get_elem_block (int   exoid,
                       int   elem_blk_id,
                       char *elem_type,
                       int  *num_elem_this_blk, 
                       int  *num_nodes_per_elem,
                       int  *num_attr)

{
  return ex_get_block( exoid, EX_ELEM_BLOCK, elem_blk_id, elem_type,
    num_elem_this_blk, num_nodes_per_elem, 0, 0, num_attr );
}
