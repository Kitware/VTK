/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * expvp - ex_put_var_param
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       char*   var_type                variable type G,N, or E
 *       int*    num_vars                number of variables in database
 *
 * exit conditions -
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h" // for ex_put_variable_param, etc

/*!
\ingroup ResultsData

 * writes the number and names of global, nodal, or element variables
 * that will be written to the database
 * \param       exoid                   exodus file id
 * \param       var_type                variable type G,N, or E
 * \param       num_vars                number of variables in database
 * \deprecated Use ex_put_variable_param()(exoid, obj_type, num_vars)
 */

int ex_put_var_param(int exoid, const char *var_type, int num_vars)
{
  ex_entity_type obj_type;
  obj_type = ex_var_type_to_ex_entity_type(*var_type);
  return (ex_put_variable_param(exoid, obj_type, num_vars));
}
