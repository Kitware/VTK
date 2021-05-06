/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * expoea - ex_put_one_elem_attr
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       int     elem_blk_id             element block id
 *       int     attrib_index            index of attribute to write
 *       float*  attrib                  array of attributes
 *
 * exit conditions -
 *
 * revision history -
 *   20061003 - David Thompson - moved to ex_put_one_attr
 *
 *
 *****************************************************************************/

#include "exodusII.h" // for ex_put_one_attr, etc

/*!
 * writes the specified attribute for an element block
 * \param      exoid                   exodus file id
 * \param      elem_blk_id             element block id
 * \param      attrib_index            index of attribute to write
 * \param      attrib                  array of attributes
 * \deprecated Use ex_put_one_attr()(exoid, EX_ELEM_BLOCK, elem_blk_id,
 attrib_index, attrib)

 */

int ex_put_one_elem_attr(int exoid, ex_entity_id elem_blk_id, int attrib_index, const void *attrib)
{
  return ex_put_one_attr(exoid, EX_ELEM_BLOCK, elem_blk_id, attrib_index, attrib);
}
