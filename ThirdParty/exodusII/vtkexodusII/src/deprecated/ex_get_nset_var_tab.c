/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * exgvtt - ex_get_nset_var_tab
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       int     num_nodesets            number of nodesets
 *       int     num_nset_var            number of nodeset variables
 *
 * exit conditions -
 *       int*    nset_var_tab            nodeset variable truth table array
 *
 *
 *****************************************************************************/

#include "exodusII.h" // for ex_get_truth_table, etc

/*!
 * reads the EXODUS nodeset variable truth table from the database
 * \deprecated Use ex_get_truth_table()(exoid, EX_NODE_SET, num_nodesets,
 * num_nset_var, nset_var_tab)
 */

int ex_get_nset_var_tab(int exoid, int num_nodesets, int num_nset_var, int *nset_var_tab)
{
  return ex_get_truth_table(exoid, EX_NODE_SET, num_nodesets, num_nset_var, nset_var_tab);
}
