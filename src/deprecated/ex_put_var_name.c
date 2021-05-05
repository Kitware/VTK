/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * expvnm - ex_put_var_name
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       char*   var_type                variable type: G,N, or E
 *       int     var_num                 variable number name to write 1..num_var
 *       char*   var_name                ptr of variable name
 *
 * exit conditions -
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h" // for ex_put_variable_name, etc

/*!
\ingroup ResultsData

 * writes the name of a particular results variable to the database
 * \param       exoid                   exodus file id
 * \param      *var_type                variable type: G,N, or E
 * \param       var_num                 variable number name to write 1..num_var
 * \param      *var_name                ptr of variable name
 * \deprecated use ex_put_variable_name()(exoid, obj_type, var_num, *var_name)
 */

int ex_put_var_name(int exoid, const char *var_type, int var_num, const char *var_name)
{
  ex_entity_type obj_type;
  obj_type = ex_var_type_to_ex_entity_type(*var_type);
  return ex_put_variable_name(exoid, obj_type, var_num, var_name);
}
