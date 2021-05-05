/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * expvar - ex_put_var
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       int     time_step               time step number
 *       int     var_type                type (edge block, face block, edge set,
 *... )
 *       int     var_index               element variable index
 *       int     obj_id                  element block id
 *       int     start_num               starting index of the variables to be
 *written
 *       int     num_ent                 number of entities to write variables
 *for.
 *
 * exit conditions -
 *
 *
 * exit conditions -
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h" // for ex_put_partial_var, etc

/*!
\ingroup ResultsData

 * \deprecated use ex_put_partial_var()(exoid, time_step, var_type, var_index, obj_id, start_index,
                            num_entities, var_vals)
 * writes the values of a single variable for a partial block at one time
 * step to the database; assume the first time step and variable index
 * are 1
 * \param      exoid           exodus file id
 * \param      time_step       time step number
 * \param      var_type        type (edge block, face block, edge set, ... )
 * \param      var_index       element variable index
 * \param      obj_id          element block id
 * \param      start_index     index of first entity in block to write (1-based)
 * \param      num_entities    number of entries in this block/set
 * \param      var_vals        the values to be written
 */

int ex_put_n_var(int exoid, int time_step, ex_entity_type var_type, int var_index,
                 ex_entity_id obj_id, int64_t start_index, int64_t num_entities,
                 const void *var_vals)
{
  return ex_put_partial_var(exoid, time_step, var_type, var_index, obj_id, start_index,
                            num_entities, var_vals);
}
