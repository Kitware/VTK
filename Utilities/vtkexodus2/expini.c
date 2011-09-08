/*
 * Copyright (c) 2005 Sandia Corporation. Under the terms of Contract
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
#include "exodusII_int.h"
#include <string.h>

/*!

The function ex_put_init() writes the initialization parameters to the
exodus file. This function must be called once (and only once) before
writing any data to the file.

\return In case of an error, ex_put_init() returns a negative number;
a warning will return a positive number.  Possible causes of errors
include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  data file opened for read only.
  -  this routine has been called previously.

\param exoid         exodus file ID returned from a previous call to ex_create() or ex_open().
\param title         Database title. Maximum length is \c MAX_LINE_LENGTH.
\param num_dim       The dimensionality of the database. This is the number of coordinates per node.
\param num_nodes     The number of nodal points.
\param num_elem      The number of elements.
\param num_elem_blk  The number of element blocks.
\param num_node_sets The number of node sets.
\param num_side_sets The number of side sets.

The following code segment will initialize an open exodus file with
the specified parameters:

\code
int num_dim, num_nods, num_el, num_el_blk, num_ns, num_ss, error, exoid;

\comment{initialize file with parameters}
num_dim = 3; num_nods = 46; num_el = 5; num_el_blk = 5;
num_ns = 2; num_ss = 5;

error = ex_put_init (exoid, "This is the title", num_dim, 
                     num_nods, num_el,num_el_blk, num_ns, num_ss);
\endcode

*/

int ex_put_init (int   exoid,
                 const char *title,
                 int   num_dim,
                 int   num_nodes,
                 int   num_elem,
                 int   num_elem_blk,
                 int   num_node_sets,
                 int   num_side_sets)
{
  ex_init_params par;

  strcpy( par.title, title );
  par.num_dim = num_dim;
  par.num_nodes = num_nodes;
  par.num_edge = 0;
  par.num_edge_blk = 0;
  par.num_face = 0;
  par.num_face_blk = 0;
  par.num_elem = num_elem;
  par.num_elem_blk = num_elem_blk;
  par.num_node_sets = num_node_sets;
  par.num_edge_sets = 0;
  par.num_face_sets = 0;
  par.num_side_sets = num_side_sets;
  par.num_elem_sets = 0;
  par.num_node_maps = 0;
  par.num_edge_maps = 0;
  par.num_face_maps = 0;
  par.num_elem_maps = 0;

  return (ex_put_init_ext( exoid, &par ));
}
