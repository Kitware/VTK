/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * exgvnm - ex_get_var_name
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       char*   var_type                variable type: G,N, or E
 *       int     var_num                 variable index to read 1..num_var
 *
 * exit conditions -
 *       char*   var_name                ptr to variable name
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h" // for ex_get_variable_name, etc

/*!
\ingroup ResultsData

 * reads the name of a particular results variable from the database
 * \deprecated use ex_get_variable_name()(exoid, obj_type, var_num, *var_name)
 */

int ex_get_var_name(int exoid, const char *var_type, int var_num, char *var_name)
{
  ex_entity_type obj_type;
  obj_type = ex_var_type_to_ex_entity_type(*var_type);
  return ex_get_variable_name(exoid, obj_type, var_num, var_name);
}
