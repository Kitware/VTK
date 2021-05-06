/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * exgem - ex_get_elem_map
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       int     map_id                  element map id
 *
 * exit conditions -
 *       int*    elem_map                element map
 *
 *****************************************************************************/

#include "exodusII.h" // for ex_get_num_map, etc

/*!
 * reads the element map with specified ID
 * \deprecated Use ex_get_num_map()(exoid, EX_ELEM_MAP, map_id, elem_map)
 * instead
 */

int ex_get_elem_map(int exoid, ex_entity_id map_id, void_int *elem_map)
{
  return ex_get_num_map(exoid, EX_ELEM_MAP, map_id, elem_map);
}
