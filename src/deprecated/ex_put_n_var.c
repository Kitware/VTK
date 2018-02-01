/*
 * Copyright (c) 2005-2017 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of NTESS nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
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
#include <stdint.h>   // for int64_t

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
