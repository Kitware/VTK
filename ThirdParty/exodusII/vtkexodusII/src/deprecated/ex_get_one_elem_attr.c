/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * exgeat - ex_get_one_elem_attr
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       int     elem_blk_id             element block id
 *
 * exit conditions -
 *       float*  attrib                  array of attributes
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h" // for ex_get_one_attr, etc

/*!
 * reads the attributes for an element block
 * \deprecated Use ex_get_one_attr()(exoid, EX_ELEM_BLOCK, elem_blk_id,
 * attrib_index, attrib)
 */
int ex_get_one_elem_attr(int exoid, ex_entity_id elem_blk_id, int attrib_index, void *attrib)

{
  return ex_get_one_attr(exoid, EX_ELEM_BLOCK, elem_blk_id, attrib_index, attrib);
}
