/*
 * Copyright (c) 2005-2017 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of NTESS nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
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
