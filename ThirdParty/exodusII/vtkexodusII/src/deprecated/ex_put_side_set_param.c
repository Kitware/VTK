/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * expsp - ex_put_side_set_param
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       int     side_set_id             side set id
 *       int     num_side_in_set         number of sides in the side set
 *       int     num_dist_fact_in_set    number of distribution factors in the
 *                                       side set
 *
 * exit conditions -
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h" // for ex_put_set_param, etc

/*!
 * writes the side set id and the number of sides (edges or faces)
 * which describe a single side set
 * \param  exoid                   exodus file id
 * \param  side_set_id             side set id
 * \param  num_side_in_set         number of sides in the side set
 * \param  num_dist_fact_in_set    number of distribution factors in the side
 * set
 * \deprecated Use ex_put_set_param()(exoid, EX_SIDE_SET, side_set_id,
 * num_side_in_set, num_dist_fact_in_set)
 */

int ex_put_side_set_param(int exoid, ex_entity_id side_set_id, int64_t num_side_in_set,
                          int64_t num_dist_fact_in_set)
{
  return ex_put_set_param(exoid, EX_SIDE_SET, side_set_id, num_side_in_set, num_dist_fact_in_set);
}
