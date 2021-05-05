/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
*
* expssd - ex_put_side_set_dist_fact
*
* entry conditions -
*   input parameters:
*       int     exoid                   exodus file id
*       int     side_set_id             side set id
*       void*   side_set_dist_fact      array of dist factors for side set

* exit conditions -
*
* revision history -
*
*
*****************************************************************************/

#include "exodusII.h" // for ex_put_set_dist_fact, etc

/*!
 * writes the distribution factors for a single side set
 * \param       exoid                   exodus file id
 * \param       side_set_id             side set id
 * \param      *side_set_dist_fact      array of dist factors for side set
 * \deprecated Use ex_put_set_dist_fact()(exoid, EX_SIDE_SET, side_set_id,
 * side_set_dist_fact)
 */

int ex_put_side_set_dist_fact(int exoid, ex_entity_id side_set_id, const void *side_set_dist_fact)
{
  return ex_put_set_dist_fact(exoid, EX_SIDE_SET, side_set_id, side_set_dist_fact);
}
