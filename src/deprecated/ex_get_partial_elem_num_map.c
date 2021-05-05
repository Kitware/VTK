/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * ne_gennm - ex_get_partial_elem_num_map
 *
 * environment - UNIX
 *
 * entry conditions -
 *   input parameters:
 *      int     exoid                   exodus file id
 *      int     start_ent               starting location for read
 *      int     num_ents                number of elemental points
 *
 * exit conditions -
 *      int*    elem_map                element number map array
 *
 * revision history -
 *
 *****************************************************************************/

#include "exodusII.h" // for ex_get_partial_id_map, etc

/*
 *  reads the element numbering map from the database; allows element numbers
 *  to be noncontiguous
 */

/*!
 * \deprecated Use ex_get_partial_id_map()(exoid, EX_ELEM_MAP, start_ent, num_ents, elem_map)
 */
int ex_get_partial_elem_num_map(int exoid, int64_t start_ent, int64_t num_ents, void_int *elem_map)
{
  return ex_get_partial_id_map(exoid, EX_ELEM_MAP, start_ent, num_ents, elem_map);
}
