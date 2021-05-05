/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h" // for ex_init_params, etc
#include "exodusII_int.h"

/*!
\ingroup ModelDescription

The function ex_put_init() writes the initialization parameters to the
exodus file. This function must be called once (and only once) before
writing any data to the file.

\return In case of an error, ex_put_init() returns a negative number;
a warning will return a positive number.  Possible causes of errors
include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  data file opened for read only.
  -  this routine has been called previously.

\param exoid         exodus file ID returned from a previous call to ex_create()
or ex_open().
\param title         Database title. Maximum length is #MAX_LINE_LENGTH.
\param num_dim       The dimensionality of the database. This is the number of
coordinates per node.
\param num_nodes     The number of nodal points.
\param num_elem      The number of elements.
\param num_elem_blk  The number of element blocks.
\param num_node_sets The number of node sets.
\param num_side_sets The number of side sets.

The following code segment will initialize an open exodus file with
the specified parameters:

~~~{.c}
int num_dim, num_nods, num_el, num_el_blk, num_ns, num_ss, error, exoid;

\comment{initialize file with parameters}
num_dim = 3; num_nods = 46; num_el = 5; num_el_blk = 5;
num_ns = 2; num_ss = 5;

error = ex_put_init (exoid, "This is the title", num_dim,
                     num_nods, num_el,num_el_blk, num_ns, num_ss);
~~~

*/

int ex_put_init(int exoid, const char *title, int64_t num_dim, int64_t num_nodes, int64_t num_elem,
                int64_t num_elem_blk, int64_t num_node_sets, int64_t num_side_sets)
{
  ex_init_params par;

  ex_copy_string(par.title, title, MAX_LINE_LENGTH + 1);

  par.num_dim       = num_dim;
  par.num_nodes     = num_nodes;
  par.num_edge      = 0;
  par.num_edge_blk  = 0;
  par.num_face      = 0;
  par.num_face_blk  = 0;
  par.num_elem      = num_elem;
  par.num_elem_blk  = num_elem_blk;
  par.num_node_sets = num_node_sets;
  par.num_edge_sets = 0;
  par.num_face_sets = 0;
  par.num_side_sets = num_side_sets;
  par.num_elem_sets = 0;
  par.num_node_maps = 0;
  par.num_edge_maps = 0;
  par.num_face_maps = 0;
  par.num_elem_maps = 0;
  par.num_assembly  = 0;

  return (ex_put_init_ext(exoid, &par));
}
