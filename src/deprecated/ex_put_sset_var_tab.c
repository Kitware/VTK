/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * expsstt - ex_put_sset_var_tab
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       int     num_sset                number of sidesets
 *       int     num_sset_var            number of sideset variables
 *       int*    sset_var_tab            sideset variable truth table array
 *
 * exit conditions -
 *
 *
 *****************************************************************************/

#include "exodusII.h" // for ex_put_truth_table, etc

/*!
 * writes the EXODUS sideset variable truth table to the database;
 * also, creates netCDF variables in which to store EXODUS sideset
 * variable values; although this table isn't required (because the
 * netCDF variables can also be created in ex_put_sset_var), this call
 * will save tremendous time because all of the variables are defined
 * at once while the file is in define mode, rather than going in and out
 * of define mode (causing the entire file to be copied over and over)
 * which is what occurs when the sideset variable values variables are
 * defined in ex_put_sset_var
 * \param      exoid                   exodus file id
 * \param      num_sset                number of sidesets
 * \param      num_sset_var            number of sideset variables
 * \param     *sset_var_tab            sideset variable truth table array
 * \deprecated Use ex_put_truth_table()(exoid, EX_SIDE_SET, num_sset,
 * num_sset_var, sset_var_tab)
 */

int ex_put_sset_var_tab(int exoid, int num_sset, int num_sset_var, int *sset_var_tab)
{
  return ex_put_truth_table(exoid, EX_SIDE_SET, num_sset, num_sset_var, sset_var_tab);
}
