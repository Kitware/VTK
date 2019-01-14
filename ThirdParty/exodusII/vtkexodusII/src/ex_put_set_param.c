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
 * expsetp - ex_put_set_param
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       int     set_type                the type of set
 *       int     set_id                  set id
 *       int     num_entries_in_set       number of entries in the set
 *       int     num_dist_fact_in_set    number of distribution factors in the
 *                                       set
 *
 * exit conditions -
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h" // for ex_set, ex_put_sets, etc
#include <stddef.h>   // for NULL
#include <stdint.h>   // for int64_t

/*!
 * writes the set id and the number of entries which describe a single set
 * \param  exoid                   exodus file id
 * \param  set_type                the type of set
 * \param  set_id                  set id
 * \param  num_entries_in_set      number of entries in the set
 * \param  num_dist_fact_in_set    number of distribution factors in the set
 */

int ex_put_set_param(int exoid, ex_entity_type set_type, ex_entity_id set_id,
                     int64_t num_entries_in_set, int64_t num_dist_fact_in_set)
{
  struct ex_set set[1];
  set[0].type                     = set_type;
  set[0].id                       = set_id;
  set[0].num_entry                = num_entries_in_set;
  set[0].num_distribution_factor  = num_dist_fact_in_set;
  set[0].entry_list               = NULL;
  set[0].extra_list               = NULL;
  set[0].distribution_factor_list = NULL;

  return ex_put_sets(exoid, 1, set);
}
