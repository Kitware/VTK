/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
*
* expss - ex_put_side_set
*
* entry conditions -
*   input parameters:
*       int     exoid                   exodus file id
*       int     side_set_id             side set id
*       int*    side_set_elem_list      array of elements in side set
*       int*    side_set_side_list      array of sides in side set

* exit conditions -
*
* revision history -
*
*
*****************************************************************************/

#include "exodusII.h" // for ex_put_set, void_int, etc

/*!
 * writes the side set element list and side set side list for a single side set
 * \param   exoid                   exodus file id
 * \param   side_set_id             side set id
 * \param  *side_set_elem_list      array of elements in side set
 * \param  *side_set_side_list      array of sides in side set
 * \deprecated  Use ex_put_set()(exoid, EX_SIDE_SET, side_set_id,
 * side_set_elem_list, side_set_side_list)
 */

int ex_put_side_set(int exoid, ex_entity_id side_set_id, const void_int *side_set_elem_list,
                    const void_int *side_set_side_list)
{
  return ex_put_set(exoid, EX_SIDE_SET, side_set_id, side_set_elem_list, side_set_side_list);
}
