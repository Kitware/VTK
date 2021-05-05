/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * exgvtt - ex_get_var_tab
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid              exodus file id
 *       int     num_blk            number of blocks
 *       int     num_var            number of variables
 *
 * exit conditions -
 *       int*    var_tab            element variable truth table array
 *
 *
 *****************************************************************************/

#include "exodusII.h" // for ex_get_truth_table, etc

/*!
\ingroup ResultsData

 * reads the EXODUS specified variable truth table from the database
 * \deprecated Use ex_get_truth_table()(exoid, obj_type, num_blk, num_var,
 * var_tab)
 */

int ex_get_var_tab(int exoid, const char *var_type, int num_blk, int num_var, int *var_tab)
{
  ex_entity_type obj_type;
  obj_type = ex_var_type_to_ex_entity_type(*var_type);
  return ex_get_truth_table(exoid, obj_type, num_blk, num_var, var_tab);
}
