/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h" // for ex_get_partial_set, etc

/*!
 * \deprecated Use ex_get_partial_side_set()(exoid, EX_SIDE_SET, side_set_id, start_side_num,
 num_sides, side_set_elem_list, side_set_side_list) instead
 */

int ex_get_n_side_set(int exoid, ex_entity_id side_set_id, int64_t start_side_num,
                      int64_t num_sides, void_int *side_set_elem_list, void_int *side_set_side_list)
{
  return ex_get_partial_set(exoid, EX_SIDE_SET, side_set_id, start_side_num, num_sides,
                            side_set_elem_list, side_set_side_list);
}
