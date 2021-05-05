/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h"

/*!
\deprecated Use ex_get_ids()(exoid, EX_NODE_SET, ids)

The function ex_get_node_set_ids() reads the IDs of all of the node
sets. Memory must be allocated for the returned array of
({num_node_sets}) IDs before this function is invoked.

\return In case of an error, ex_get_node_set_ids() returns a negative
number; a warning will return a positive number.  Possible causes of
errors include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  a warning value is returned if no node sets are stored in the file.

\param[in]  exoid    exodus file ID returned from a previous call to ex_create()
or ex_open().
\param[out] ids      Returned array of the node sets IDs. The order of the IDs
in this array
                     reflects the sequence the node sets were introduced into
the file.

As an example, the following code will read all of the node set IDs
from an open data file:

~~~{.c}
int *ids, num_node_sets, error, exoid;

\comment{read node sets IDs}
ids = (int *) calloc(num_node_sets, sizeof(int));

error = ex_get_node_set_ids (exoid, ids);

\comment{Same result using non-deprecated functions.}
error = ex_get_ids (exoid, EX_NODE_SET, ids);
~~~
 */

int ex_get_node_set_ids(int exoid, void_int *ids) { return ex_get_ids(exoid, EX_NODE_SET, ids); }
