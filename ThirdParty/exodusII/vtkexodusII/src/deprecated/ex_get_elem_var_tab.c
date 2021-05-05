/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * exgvtt - ex_get_elem_var_tab
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       int     num_elem_blk            number of element blocks
 *       int     num_elem_var            number of element variables
 *
 * exit conditions -
 *       int*    elem_var_tab            element variable truth table array
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h" // for ex_get_truth_table, etc

/*!
\deprecated Use ex_get_truth_table()(exoid, EX_ELEM_BLOCK, num_elem_blk,
num_elem_var, elem_var_tab)

The function ex_get_elem_var_tab() reads the exodus element variable
truth table from the database. For a description of the truth table,
see the usage of the function ex_put_elem_var_tab(). Memory must be
allocated for the truth table(num_elem_blk x num_elem_var in length)
before this function is invoked. If the truth table is not stored in
the file, it will be created based on information in the file and then
returned.

\return In case of an error, ex_get_elem_var_tab() returns a negative
number; a warning will return a positive number.  Possible causes of
errors include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  data file not initialized properly with call to ex_put_init().
  -  the specified number of element blocks is different than the
     number specified in a call to ex_put_init().
  -  there are no element variables stored in the file or the
     specified number of element variables doesn't match the number
     specified in a call to ex_put_variable_param().

\param[in]   exoid         exodus file ID returned from a previous call to
ex_create() or ex_open().
\param[in]   num_elem_blk  The number of element blocks.
\param[in]   num_elem_var  The number of element variables.
\param[out]  elem_var_tab  Size [num_elem_blk,num_elem_var].
                           Returned 2-dimensional array (with the
                           num_elem_var index cycling
                           faster) containing the element variable truth table.

As an example, the following coding will read the element
variable truth table from an opened exodus file :

~~~{.c}
int *truth_tab, num_elem_blk, num_ele_vars, error, exoid;

truth_tab = (int *) calloc ((num_elem_blk*num_ele_vars),
                            sizeof(int));

error = ex_get_elem_var_tab (exoid, num_elem_blk, num_ele_vars,
                             truth_tab);
~~~

 */

int ex_get_elem_var_tab(int exoid, int num_elem_blk, int num_elem_var, int *elem_var_tab)
{
  return ex_get_truth_table(exoid, EX_ELEM_BLOCK, num_elem_blk, num_elem_var, elem_var_tab);
}
