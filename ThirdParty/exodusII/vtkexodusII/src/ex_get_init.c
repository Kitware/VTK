/*
 * Copyright(C) 1999-2020, 2022 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * exgini - ex_get_init
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *
 * exit conditions -
 *       char*   title                   title of file
 *       int*    num_dim                 number of dimensions (per node)
 *       int*    num_nodes               number of nodes
 *       int*    num_elem                number of elements
 *       int*    num_elem_blk            number of element blocks
 *       int*    num_node_sets           number of node sets
 *       int*    num_side_sets           number of side sets
 *
 * revision history -
 *          David Thompson  - Moved to exginix.c (exgini.c now a special case)
 *
 *
 *****************************************************************************/

#include "exodusII.h" // for ex_init_params, void_int, etc
#include "exodusII_int.h"

/*!
  \ingroup ModelDescription

The function ex_get_init() reads the initialization
parameters from an opened exodus file.

\return In case of an error, ex_get_init() returns a negative number;
a warning will return a positive number. Possible causes of errors
include:
  -  data file not properly opened with call to ex_create() or ex_open().

\param exoid              exodus file ID returned from a previous call to
ex_create() or ex_open().
\param[out] title         Returned database title. String length may be up to \c
MAX_LINE_LENGTH+1 bytes.
\param[out] num_dim       Returned dimensionality of the database. This is the
number of coordinates per node.
\param[out] num_nodes     Returned number of nodal points.
\param[out] num_elem      Returned number of elements
\param[out] num_elem_blk  Returned number of element blocks
\param[out] num_node_sets Returned number of node sets
\param[out] num_side_sets Returned number of side sets

\sa ex_get_init_ext()

The following code segment will read the initialization parameters
from the open exodus file:

~~~{.c}
int num_dim, num_nodes, num_elem, num_elem_blk,
    num_node_sets, num_side_sets, error, exoid;

char title[MAX_LINE_LENGTH+1];

\comment{read database parameters}
error = ex_get_init (exoid, title, &num_dim, &num_nodes,
                     &num_elem, &num_elem_blk, &num_node_sets, &num_side_sets);
~~~

*/

int ex_get_init(int exoid, char *title, void_int *num_dim, void_int *num_nodes, void_int *num_elem,
                void_int *num_elem_blk, void_int *num_node_sets, void_int *num_side_sets)
{
  ex_init_params info;
  int            errval;

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  info.title[0] = '\0';
  errval        = ex_get_init_ext(exoid, &info);
  if (errval < 0) {
    EX_FUNC_LEAVE(errval);
  }

  if (ex_int64_status(exoid) & EX_BULK_INT64_API) {
    int64_t *n_dim       = num_dim;
    int64_t *n_nodes     = num_nodes;
    int64_t *n_elem      = num_elem;
    int64_t *n_elem_blk  = num_elem_blk;
    int64_t *n_node_sets = num_node_sets;
    int64_t *n_side_sets = num_side_sets;

    *n_dim       = info.num_dim;
    *n_nodes     = info.num_nodes;
    *n_elem      = info.num_elem;
    *n_elem_blk  = info.num_elem_blk;
    *n_node_sets = info.num_node_sets;
    *n_side_sets = info.num_side_sets;
  }
  else {
    int *n_dim       = num_dim;
    int *n_nodes     = num_nodes;
    int *n_elem      = num_elem;
    int *n_elem_blk  = num_elem_blk;
    int *n_node_sets = num_node_sets;
    int *n_side_sets = num_side_sets;

    *n_dim       = (int)info.num_dim;
    *n_nodes     = (int)info.num_nodes;
    *n_elem      = (int)info.num_elem;
    *n_elem_blk  = (int)info.num_elem_blk;
    *n_node_sets = (int)info.num_node_sets;
    *n_side_sets = (int)info.num_side_sets;
  }
  ex_copy_string(title, info.title, MAX_LINE_LENGTH + 1);

  EX_FUNC_LEAVE(EX_NOERR);
}
