/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * expatt - ex_put_n_attr
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       int     blk_type                block type
 *       int     blk_id                  block id
 *       float*  attrib                  array of attributes
 *
 * exit conditions -
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h" // for ex_put_partial_attr, etc

/*!
 * \deprecated use ex_put_partial_attr()(exoid, blk_type, blk_id, start_entity, num_entity, attrib)
 *
 * writes the attributes for an edge/face/element block
 * \param   exoid                   exodus file id
 * \param   blk_type                block type
 * \param   blk_id                  block id
 * \param   start_entity            the starting index (1-based) of the attribute to be written
 * \param   num_entity              the number of entities to write attributes
 * \param   attrib                  array of attributes
 */

int ex_put_n_attr(int exoid, ex_entity_type blk_type, ex_entity_id blk_id, int64_t start_entity,
                  int64_t num_entity, const void *attrib)
{
  return ex_put_partial_attr(exoid, blk_type, blk_id, start_entity, num_entity, attrib);
}
