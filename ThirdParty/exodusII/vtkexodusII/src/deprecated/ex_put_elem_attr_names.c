/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
*
* expeat - ex_put_elem_attr_names
*
* entry conditions -
*   input parameters:
*       int           exoid             exodus file id
*       int           elem_blk_id       element block id
*       char*         names[]           ptr array of attribute names

*
* exit conditions -
*
* revision history -
*
*
*****************************************************************************/

#include "exodusII.h" // for ex_put_attr_names, etc

/*!
 * writes the attribute names for an element block
 * \param    exoid             exodus file id
 * \param    elem_blk_id       element block id
 * \param    names[]           ptr array of attribute names
 * \deprecated Use ex_put_attr_names()(exoid, EX_ELEM_BLOCK, elem_blk_id, names)
 */
int ex_put_elem_attr_names(int exoid, ex_entity_id elem_blk_id, char *names[])
{
  return ex_put_attr_names(exoid, EX_ELEM_BLOCK, elem_blk_id, names);
}
