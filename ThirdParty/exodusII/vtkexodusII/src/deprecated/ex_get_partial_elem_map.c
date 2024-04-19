/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h" // for ex_get_partial_num_map, etc

/*
 * reads the element map with specified ID
 * \deprecated Use ex_get_partial_num_map()(exoid, EX_ELEM_MAP, map_id,
 * ent_start, ent_count, elem_map) instead
 */

int ex_get_partial_elem_map(int exoid, ex_entity_id map_id, int64_t ent_start, int64_t ent_count,
                            void_int *elem_map)
{
  return ex_get_partial_num_map(exoid, EX_ELEM_MAP, map_id, ent_start, ent_count, elem_map);
}
