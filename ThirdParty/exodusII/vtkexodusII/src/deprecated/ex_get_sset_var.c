/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * exgssv - ex_get_sset_var
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       int     time_step               time step number
 *       int     sset_var_index          sideset variable index
 *       int     sset_blk_id             sideset id
 *       int     num_side_this_sset       number of sides in this sideset
 *
 *
 * exit conditions -
 *       float*  sset_var_vals           array of sideset variable values
 *
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h" // for ex_get_var, ex_entity_id, etc

/*!
 * reads the values of a single sideset variable for one sideset at
 * one time step in the database; assume the first time step and
 * sideset variable index is 1
 * \deprecated Use ex_get_var()(exoid, time_step, EX_SIDE_SET, sset_var_index,
 * sset_id, num_side_this_sset, sset_var_vals) instead
 */

int ex_get_sset_var(int exoid, int time_step, int sset_var_index, ex_entity_id sset_id,
                    int64_t num_side_this_sset, void *sset_var_vals)
{
  return ex_get_var(exoid, time_step, EX_SIDE_SET, sset_var_index, sset_id, num_side_this_sset,
                    sset_var_vals);
}
