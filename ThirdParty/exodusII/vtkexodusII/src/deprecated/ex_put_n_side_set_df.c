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
 *      ex_put_n_side_set_df()
 *
 *****************************************************************************
 *
 *  Variable Index:
 *
 *      exoid               - The NetCDF ID of an already open NemesisI file.
 *      side_set_id        - ID of side set to written.
 *      start_num          - The starting index of the df's to be written.
 *      num_df_to_get      - The number of sides to write.
 *      side_set_dist_fact - List of distribution factors for the side set.
 */
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
#include "exodusII.h"

/*!
 * \deprecated use ex_put_partial_set_dist_fact()(exoid, EX_SIDE_SET, side_set_id, start_num,
 num_df_to_get, side_set_dist_fact)
 */

int ex_put_n_side_set_df(int exoid, ex_entity_id side_set_id, int64_t start_num,
                         int64_t num_df_to_get, void *side_set_dist_fact)
{
  return ex_put_partial_set_dist_fact(exoid, EX_SIDE_SET, side_set_id, start_num, num_df_to_get,
                                      side_set_dist_fact);
}
