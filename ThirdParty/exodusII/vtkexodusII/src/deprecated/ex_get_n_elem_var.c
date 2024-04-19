/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h" // for ex_get_partial_var, etc
#include "exodusII_int.h"
/*!
\ingroup ResultsData

 * \deprecated Use ex_get_partial_var()(exoid, time_step, EX_ELEM_BLOCK, elem_var_index,
elem_blk_id, start_elem_num, num_elem, elem_var_vals) instead
 */

int ex_get_n_elem_var(int exoid, int time_step, int elem_var_index, ex_entity_id elem_blk_id,
                      int64_t num_elem_this_blk, int64_t start_elem_num, int64_t num_elem,
                      void *elem_var_vals)
{
  EX_UNUSED(num_elem_this_blk);
  return ex_get_partial_var(exoid, time_step, EX_ELEM_BLOCK, elem_var_index, elem_blk_id,
                            start_elem_num, num_elem, elem_var_vals);
}
