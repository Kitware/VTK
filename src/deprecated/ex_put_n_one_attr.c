/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * expoea - ex_put_n_one_attr
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       int     obj_type                object type (edge, face, elem block)
 *       int     obj_id                  object id (edge, face, elem block ID)
 *       int     attrib_index            index of attribute to write
 *       float*  attrib                  array of attributes
 *
 * exit conditions -
 *
 *
 *****************************************************************************/

#include "exodusII.h" // for ex_put_partial_one_attr, etc

/*!
 * \deprecated Use ex_put_partial_one_attr()(exoid, obj_type, obj_id, start_num, num_ent,
 * attrib_index, attrib) writes the specified attribute for a block \param      exoid         exodus
 * file id \param      obj_type      object type (edge, face, elem block) \param      obj_id
 * object id (edge, face, elem block ID) \param      start_num     the starting index of the
 * attributes to be written \param      num_ent       the number of entities to write attributes
 * for. \param      attrib_index  index of attribute to write \param      attrib        array of
 * attributes
 */

int ex_put_n_one_attr(int exoid, ex_entity_type obj_type, ex_entity_id obj_id, int64_t start_num,
                      int64_t num_ent, int attrib_index, const void *attrib)
{
  return ex_put_partial_one_attr(exoid, obj_type, obj_id, start_num, num_ent, attrib_index, attrib);
}
