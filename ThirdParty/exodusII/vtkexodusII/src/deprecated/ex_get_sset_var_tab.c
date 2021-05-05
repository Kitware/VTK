/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * exgvtt - ex_get_sset_var_tab
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       int     num_sidesets            number of sidesets
 *       int     num_sset_var            number of sideset variables
 *
 * exit conditions -
 *       int*    sset_var_tab            sideset variable truth table array
 *
 *
 *****************************************************************************/

#include "exodusII.h" // for ex_get_truth_table, etc

/*!
 * reads the EXODUS sideset variable truth table from the database
 * \deprecated Use ex_get_truth_table()(exoid, EX_SIDE_SET, num_sidesets,
 * num_sset_var, sset_var_tab)
 */

int ex_get_sset_var_tab(int exoid, int num_sidesets, int num_sset_var, int *sset_var_tab)
{
  return ex_get_truth_table(exoid, EX_SIDE_SET, num_sidesets, num_sset_var, sset_var_tab);
}
