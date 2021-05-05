/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * exgeat - ex_get_elem_attr_names
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       int     elem_blk_id             element block id
 *
 * exit conditions -
 *       char*   names[]                 ptr array of attribute names
 *
 *
 *****************************************************************************/

#include "exodusII.h" // for ex_get_attr_names, etc

/*!
 * reads the attribute names for an element block
 * \deprecated Use ex_get_attr_names()(exoid, EX_ELEM_BLOCK, elem_blk_id, names)
 * instead
 */
int ex_get_elem_attr_names(int exoid, ex_entity_id elem_blk_id, char **names)
{
  return ex_get_attr_names(exoid, EX_ELEM_BLOCK, elem_blk_id, names);
}
