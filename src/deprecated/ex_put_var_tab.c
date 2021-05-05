/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * expvtt - ex_put_var_tab
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       char*   type                    'e', 'm', 's' element, nodeset, sideset
 *       int     num_blk            number of blocks
 *       int     num_var            number of variables
 *       int*    var_tab            variable truth table array
 *
 * exit conditions -
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h" // for ex_put_truth_table, etc

/*!
\ingroup ResultsData

 * writes the EXODUS variable truth table to the database; also,
 * creates netCDF variables in which to store EXODUS variable
 * values; although this table isn't required (because the netCDF
 * variables can also be created in ex_put_var), this call will save
 * tremendous time because all of the variables are defined at once
 * while the file is in define mode, rather than going in and out of
 * define mode (causing the entire file to be copied over and over)
 * which is what occurs when the variables are defined in ex_put_var
 * \param      exoid              exodus file id
 * \param     *var_type               'e', 'm', 's' element, nodeset, sideset
 * \param      num_blk            number of blocks
 * \param      num_var            number of variables
 * \param     *var_tab            variable truth table array
 * \deprecated Use ex_put_truth_table()(exoid, obj_type, num_blk, num_var,
 * var_tab)
 */

int ex_put_var_tab(int exoid, const char *var_type, int num_blk, int num_var, int *var_tab)
{
  ex_entity_type obj_type;
  obj_type = ex_var_type_to_ex_entity_type(*var_type);
  return ex_put_truth_table(exoid, obj_type, num_blk, num_var, var_tab);
}
