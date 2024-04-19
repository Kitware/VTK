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
 *      ex_put_n_side_set()
 *
 *****************************************************************************
 *
 *  Variable Index:
 *
 *      exoid               - The NetCDF ID of an already open NemesisI file.
 *      side_set_id        - ID of side set to read.
 *      start_side_num     - The starting index of the sides to be read.
 *      num_sides          - The number of sides to read in.
 *      side_set_elem_list - List of element IDs in side set.
 *      side_set_side_list - List of side IDs in side set.
 */
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
#include "exodusII.h" // for ex_put_partial_set, etc

/*!
 * \deprecated use ex_put_partial_set()(exoid, EX_SIDE_SET, side_set_id, start_side_num, num_sides,
                            side_set_elem_list, side_set_side_list) instead
 */

int ex_put_n_side_set(int exoid, ex_entity_id side_set_id, int64_t start_side_num,
                      int64_t num_sides, const void_int *side_set_elem_list,
                      const void_int *side_set_side_list)
{
  return ex_put_partial_set(exoid, EX_SIDE_SET, side_set_id, start_side_num, num_sides,
                            side_set_elem_list, side_set_side_list);
}
