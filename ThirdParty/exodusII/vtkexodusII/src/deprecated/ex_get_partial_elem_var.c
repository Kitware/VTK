/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/* Function(s) contained in this file:
 *
 *      ex_get_partial_elem_var()
 *
 *****************************************************************************
 *
 *  Variable Index:
 *
 *      exoid               - The NetCDF ID of an already open NemesisI file.
 *      time_step          - The time step to write this data to.
 *      elem_var_index     - The index of this elemental variable.
 *      elem_blk_id        - The ID of the element block being written to.
 *      num_elem_this_blk  - The number of elements in this block.
 *      start_elem_num     - The start point for outputting data.
 *      num_elem           - The number of values to be output.
 *      elem_var_vals      - Pointer to the vector of values to be output.
 */
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

#include "exodusII.h" // for ex_get_partial_var, etc
#include "exodusII_int.h"

/*!
\ingroup ResultsData

 * \deprecated use ex_get_partial_var()(exoid, time_step, EX_ELEM_BLOCK, elem_var_index,
elem_blk_id, start_elem_num, num_elem, elem_var_vals)
 */

/*
 * reads the values of a single element variable for one element block at
 * one time step in the database; assume the first time step and
 * element variable index is 1
 */

int ex_get_partial_elem_var(int exoid, int time_step, int elem_var_index, ex_entity_id elem_blk_id,
                            int64_t num_elem_this_blk, int64_t start_elem_num, int64_t num_elem,
                            void *elem_var_vals)
{
  EX_UNUSED(num_elem_this_blk);
  return ex_get_partial_var(exoid, time_step, EX_ELEM_BLOCK, elem_var_index, elem_blk_id,
                            start_elem_num, num_elem, elem_var_vals);
}
