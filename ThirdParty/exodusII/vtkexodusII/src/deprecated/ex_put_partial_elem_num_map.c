/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * ne_pennm - ex_put_partial_elem_num_map
 *
 * environment - UNIX
 *
 * entry conditions -
 *   input parameters:
 *      int     exoid                   exodus file id
 *      int     start_ent               first entry in elem_map
 *      int     num_ents                number of entries in node_map
 *       int*    elem_map                element numbering map array
 *
 * exit conditions -
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h" // for ex_put_partial_id_map, etc

/*!
 * \deprecated Use ex_put_partial_id_map()(exoid, EX_ELEM_MAP, start_ent, num_ents, elem_map)
 */

/*
 * writes out a portion of the element numbering map to the database;
 * this allows element numbers to be non-contiguous
 */

int ex_put_partial_elem_num_map(int exoid, int64_t start_ent, int64_t num_ents,
                                const void_int *elem_map)
{
  return ex_put_partial_id_map(exoid, EX_ELEM_MAP, start_ent, num_ents, elem_map);
}
