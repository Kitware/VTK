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
 *      ex_get_partial_side_set_df()
 *
 *****************************************************************************
 *
 *  Variable Index:
 *
 *      exoid               - The NetCDF ID of an already open NemesisI file.
 *      side_set_id        - ID of side set to read.
 *      start_side_num     - The starting index of the sides to be read.
 *      num_sides          - The number of sides to read in.
 *      side_set_dist_fact - List of side IDs in side set.
 */
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
#include "exodusII.h"

/*!
 * \deprecated Use ex_get_partial_set_dist_fact()(exoid, EX_SIDE_SET, side_set_id, start_num,
 num_df_to_get, side_set_dist_fact)
 */
int ex_get_partial_side_set_df(int exoid, ex_entity_id side_set_id, int64_t start_num,
                               int64_t num_df_to_get, void *side_set_dist_fact)
{
  return ex_get_partial_set_dist_fact(exoid, EX_SIDE_SET, side_set_id, start_num, num_df_to_get,
                                      side_set_dist_fact);
}
