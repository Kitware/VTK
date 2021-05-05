/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * expnstt - ex_put_nset_var_tab
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       int     num_nset                number of nodesets
 *       int     num_nset_var            number of nodeset variables
 *       int*    nset_var_tab            nodeset variable truth table array
 *
 * exit conditions -
 *
 *
 *****************************************************************************/

#include "exodusII.h" // for ex_put_truth_table, etc

/*!
 * writes the EXODUS nodeset variable truth table to the database;
 * also, creates netCDF variables in which to store EXODUS nodeset
 * variable values; although this table isn't required (because the
 * netCDF variables can also be created in ex_put_nset_var), this call
 * will save tremendous time because all of the variables are defined
 * at once while the file is in define mode, rather than going in and out
 * of define mode (causing the entire file to be copied over and over)
 * which is what occurs when the nodeset variable values variables are
 * defined in ex_put_nset_var
 * \param      exoid                   exodus file id
 * \param      num_nset                number of nodesets
 * \param      num_nset_var            number of nodeset variables
 * \param      nset_var_tab            nodeset variable truth table array
 * \deprecated Use ex_put_truth_table()(exoid, EX_NODE_SET, num_nset,
 * num_nset_var, nset_var_tab)
 */

int ex_put_nset_var_tab(int exoid, int num_nset, int num_nset_var, int *nset_var_tab)
{
  return ex_put_truth_table(exoid, EX_NODE_SET, num_nset, num_nset_var, nset_var_tab);
}
