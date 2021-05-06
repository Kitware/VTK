/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * exppem - ex_put_partial_elem_map
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       int     map_id                  element map id
 *       int     ent_start               first entry in map
 *       int     ent_count               number of entries in map
 *       int     *elem_map               element map
 *
 * exit conditions -
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h" // for ex_put_partial_num_map, etc

/*!
 * writes an element map; this is a vector of integers of length number
 * of elements
 * \deprecated Use ex_put_partial_num_map()(exoid, EX_ELEM_MAP, map_id,
 * ent_start, ent_count, elem_map) instead
 */
int ex_put_partial_elem_map(int exoid, ex_entity_id map_id, int64_t ent_start, int64_t ent_count,
                            const void_int *elem_map)
{
  return ex_put_partial_num_map(exoid, EX_ELEM_MAP, map_id, ent_start, ent_count, elem_map);
}
