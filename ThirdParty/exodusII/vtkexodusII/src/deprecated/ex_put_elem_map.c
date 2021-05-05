/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * expem - ex_put_elem_map
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       int     map_id                  element map id
 *       int     *elem_map               element map
 *
 * exit conditions -
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h" // for ex_put_num_map, etc

/*!
 * writes an element map; this is a vector of integers of length number
 * of elements
 * \param  exoid                   exodus file id
 * \param  map_id                  element map id
 * \param  elem_map                element map
 * \deprecated Use ex_put_num_map()(exoid, EX_ELEM_MAP, map_id, elem_map)
 */

int ex_put_elem_map(int exoid, ex_entity_id map_id, const void_int *elem_map)
{
  return ex_put_num_map(exoid, EX_ELEM_MAP, map_id, elem_map);
}
