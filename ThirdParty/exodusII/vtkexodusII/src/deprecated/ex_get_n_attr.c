/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h" // for ex_get_partial_attr, etc

/*!
 * \deprecated Use ex_get_partial_attr()(exoid, obj_type, obj_id, start_num, num_ent, attrib)
 * instead reads the specified attribute for a subsect of a block \param      exoid         exodus
 * file id \param      obj_type      object type (edge, face, elem block) \param      obj_id
 * object id (edge, face, elem block ID) \param      start_num     the starting index of the
 * attributes to be returned. \param      num_ent       the number of entities to read attributes
 * for. \param      attrib         array of attributes
 */
/*
 */
int ex_get_n_attr(int exoid, ex_entity_type obj_type, ex_entity_id obj_id, int64_t start_num,
                  int64_t num_ent, void *attrib)

{
  return ex_get_partial_attr(exoid, obj_type, obj_id, start_num, num_ent, attrib);
}
