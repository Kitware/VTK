/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * exgvan - ex_get_var_names
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       char*   var_type                variable type: G,N, or E
 *       int     num_vars                # of variables to read
 *
 * exit conditions -
 *       char*   var_names               ptr array of variable names
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h" // for ex_get_variable_names, etc

/*!
\ingroup ResultsData

 * reads the names of the results variables from the database
 * \deprecated Use ex_get_variable_names()(exoid, obj_type, num_vars, var_names)
 */

int ex_get_var_names(int exoid, const char *var_type, int num_vars, char *var_names[])
{
  ex_entity_type obj_type;
  obj_type = ex_var_type_to_ex_entity_type(*var_type);
  return ex_get_variable_names(exoid, obj_type, num_vars, var_names);
}
