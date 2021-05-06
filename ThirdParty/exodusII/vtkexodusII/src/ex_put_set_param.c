/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
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
