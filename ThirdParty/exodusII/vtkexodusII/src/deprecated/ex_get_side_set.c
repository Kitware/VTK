/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * exgss - ex_get_side_set
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       int     side_set_id             side set id
 *
 * exit conditions -
 *       int*    side_set_elem_list      array of elements in side set
 *       int*    side_set_side_list      array of sides in side set
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h" // for ex_get_set, void_int, etc

/*!
 * reads the side set element list and side set side list for a single side set
 * \deprecated Use ex_get_set()(exoid, EX_SIDE_SET, side_set_id,
 * side_set_elem_list, side_set_side_list)
 */

int ex_get_side_set(int exoid, ex_entity_id side_set_id, void_int *side_set_elem_list,
                    void_int *side_set_side_list)
{
  return ex_get_set(exoid, EX_SIDE_SET, side_set_id, side_set_elem_list, side_set_side_list);
}
