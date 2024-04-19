/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

/*****************************************************************************
 *
 * exgsp - ex_get_side_set_param
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       int     side_set_id             side set id
 *
 * exit conditions -
 *       int*    num_side_in_set         number of sides in the side set
 *       int*    num_dist_fact_in_set    number of distribution factors in the
 *                                       side set
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h"

/*!
 * reads the number of sides and the number of distribution factors which
 * describe a single side set
 * \param      exoid                   exodus file id
 * \param      side_set_id             side set id
 * \param[out] num_side_in_set         number of sides in the side set
 * \param[out] num_dist_fact_in_set    number of distribution factors in the
 * \deprecated Use ex_get_set_param()(exoid, EX_SIDE_SET, side_set_id,
 * num_side_in_set, num_dist_fact_in_set)
 */

int ex_get_side_set_param(int exoid, ex_entity_id side_set_id, void_int *num_side_in_set,
                          void_int *num_dist_fact_in_set)
{
  return ex_get_set_param(exoid, EX_SIDE_SET, side_set_id, num_side_in_set, num_dist_fact_in_set);
}
