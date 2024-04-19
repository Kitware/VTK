/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h" // for ex_get_partial_var, etc

/*
 * reads the values of a single element variable for one element block at
 * one time step in the database; assume the first time step and
 * element variable index is 1
 */

/*!
\ingroup ResultsData

 * \deprecated Use ex_get_partial_var()(exoid, time_step, var_type, var_index, obj_id, start_index,
                            num_entities, var_vals) instead.
 *
 * reads the values of a single variable for a partial block at one time
 * step from the database; assume the first time step and variable index
 * and start_index are 1
 * \param      exoid           exodus file id
 * \param      time_step       time step number
 * \param      var_type        type (edge block, face block, edge set, ... )
 * \param      var_index       element variable index
 * \param      obj_id          element block id
 * \param      start_index     index of first entity in block to read (1-based)
 * \param      num_entities    number of entries to read in this block/set
 * \param      var_vals        the values to read
 */

int ex_get_n_var(int exoid, int time_step, ex_entity_type var_type, int var_index,
                 ex_entity_id obj_id, int64_t start_index, int64_t num_entities, void *var_vals)
{
  return ex_get_partial_var(exoid, time_step, var_type, var_index, obj_id, start_index,
                            num_entities, var_vals);
}
