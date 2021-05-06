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
 *      ex_put_elem_var_slab()
 *
 *****************************************************************************
 *
 *  Variable Index:
 *
 *      exoid               - The NetCDF ID of an already open NemesisI file.
 *      time_step          - The time step to write this data to.
 *      elem_var_index     - The index of this elemental variable.
 *      elem_blk_id        - The ID of the element block being written to.
 *      start_pos          - The start point for outputting data. The
 *                           first value is 0.
 *      num_vals           - The number of values to be output.
 *      elem_var_vals      - Pointer to the vector of values to be output.
 */
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
#include "exodusII.h" // for ex_put_partial_var, etc

/*
 * writes the values of a single element variable for one element block,
 * starting at start_pos, at one time step to the database; assume the
 * first time step and element variable index are 1
 */

/*!
 * \deprecated Use ex_put_partial_var()(exoid, time_step, EX_ELEM_BLOCK, elem_var_index,
 elem_blk_id, start_pos, num_vals, elem_var_vals)
 */
int ex_put_elem_var_slab(int exoid, int time_step, int elem_var_index, ex_entity_id elem_blk_id,
                         int64_t start_pos, int64_t num_vals, void *elem_var_vals)
{
  return ex_put_partial_var(exoid, time_step, EX_ELEM_BLOCK, elem_var_index, elem_blk_id, start_pos,
                            num_vals, elem_var_vals);
}
