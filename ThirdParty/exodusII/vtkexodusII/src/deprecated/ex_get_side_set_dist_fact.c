/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * exgssd - ex_get_side_set_dist_fact
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       int     side_set_id             side set id
 *
 * exit conditions -
 *       float*  side_set_dist_fact      array of dist factors for side set
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h" // for ex_get_set_dist_fact, etc

/*!
 * reads the distribution factors for a single side set
 * \deprecated Use ex_get_set_dist_fact()(exoid, EX_SIDE_SET, side_set_id,
 * side_set_dist_fact)
 */

int ex_get_side_set_dist_fact(int exoid, ex_entity_id side_set_id, void *side_set_dist_fact)
{
  return ex_get_set_dist_fact(exoid, EX_SIDE_SET, side_set_id, side_set_dist_fact);
}
