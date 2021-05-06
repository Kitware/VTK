/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * exgcss - ex_get_concat_side_sets
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *
 * exit conditions -
 *       int     *side_set_ids           array of side set ids
 *       int     *num_elem_per_set       number of elements/sides/faces  per set
 *       int     *num_dist_per_set       number of distribution factors per set
 *       int     *side_sets_elem_index   index array of elements into elem list
 *       int     *side_sets_dist_index   index array of df into df list
 *       int     *side_sets_elem_list    array of elements
 *       int     *side_sets_side_list    array of sides
 *       void    *side_sets_dist_fact    array of distribution factors
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h" // for ex_set_specs, void_int, etc

/*!
 * \undoc reads the side set ID's, side set element count array, side set node
 * count
 * array, side set element pointers array, side set node pointers array, side
 * set element list, side set node list, and side set distribution factors
 * \deprecated Use ex_get_concat_sets()(exoid, EX_SIDE_SET, set_specs) instead
 */

int ex_get_concat_side_sets(int exoid, void_int *side_set_ids, void_int *num_elem_per_set,
                            void_int *num_dist_per_set, void_int *side_sets_elem_index,
                            void_int *side_sets_dist_index, void_int *side_sets_elem_list,
                            void_int *side_sets_side_list, void *side_sets_dist_fact)
{
  struct ex_set_specs set_specs;

  set_specs.sets_ids            = side_set_ids;
  set_specs.num_entries_per_set = num_elem_per_set;
  set_specs.num_dist_per_set    = num_dist_per_set;
  set_specs.sets_entry_index    = side_sets_elem_index;
  set_specs.sets_dist_index     = side_sets_dist_index;
  set_specs.sets_entry_list     = side_sets_elem_list;
  set_specs.sets_extra_list     = side_sets_side_list;
  set_specs.sets_dist_fact      = side_sets_dist_fact;

  return ex_get_concat_sets(exoid, EX_SIDE_SET, &set_specs);
}
