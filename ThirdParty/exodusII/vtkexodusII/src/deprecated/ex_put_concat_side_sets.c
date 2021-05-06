/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * expcss - ex_put_concat_side_sets
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       int     *side_set_ids           array of side set ids
 *       int     *num_elem_per_set       number of elements/sides/faces per set
 *       int     *num_dist_per_set       number of distribution factors per set
 * ----------pass in NULL for remaining args if just want to set params
 *-------------
 *       int     *side_sets_elem_index   index array of elements into elem list
 *       int     *side_sets_dist_index   index array of df into df list
 *       int     *side_sets_elem_list    array of elements
 *       int     *side_sets_side_list    array of sides/faces
 *       void    *side_sets_dist_fact    array of distribution factors
 *
 * exit conditions -
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h" // for ex_set_specs, void_int, etc

/*!
 * writes the side set ID's, side set element count array,
 * side set element pointers array, side set element list,
 * side set side list, and distribution factors list.
 * \param   exoid                   exodus file id
 * \param   side_set_ids           array of side set ids
 * \param   num_elem_per_set       number of elements/sides/faces per set
 * \param   num_dist_per_set       number of distribution factors per set
 * \param   side_sets_elem_index   index array of elements into elem list
 * \param   side_sets_dist_index   index array of df into df list
 * \param   side_sets_elem_list    array of elements
 * \param   side_sets_side_list    array of sides/faces
 * \param   side_sets_dist_fact    array of distribution factors
 * \deprecated Use ex_put_concat_sets()(exoid, EX_SIDE_SET, set_specs)
 */

int ex_put_concat_side_sets(int exoid, void_int *side_set_ids, void_int *num_elem_per_set,
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

  return ex_put_concat_sets(exoid, EX_SIDE_SET, &set_specs);
}
