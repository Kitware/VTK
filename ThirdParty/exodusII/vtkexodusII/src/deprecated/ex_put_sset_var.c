/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * expev - ex_put_sset_var
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       int     time_step               time step number
 *       int     sset_var_index          sideset variable index
 *       int     sset_id                 sideset id
 *       int     num_faces_this_sset     number of faces in this sideset
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

#include "exodusII.h" // for ex_put_var, ex_entity_id, etc

/*!
 * writes the values of a single sideset variable for one sideset at
 * one time step to the database; assume the first time step and
 * sideset variable index are 1
 * \param      exoid                   exodus file id
 * \param      time_step               time step number
 * \param      sset_var_index          sideset variable index
 * \param      sset_id                 sideset id
 * \param      num_faces_this_sset     number of faces in this sideset
 * \param      sset_var_vals           the variable values to be written
 * \deprecated Use ex_put_var()(exoid, time_step, EX_SIDE_SET, sset_var_index,
 * sset_id, num_faces_this_sset, sset_var_vals)
 */

int ex_put_sset_var(int exoid, int time_step, int sset_var_index, ex_entity_id sset_id,
                    int64_t num_faces_this_sset, const void *sset_var_vals)
{
  return ex_put_var(exoid, time_step, EX_SIDE_SET, sset_var_index, sset_id, num_faces_this_sset,
                    sset_var_vals);
}
