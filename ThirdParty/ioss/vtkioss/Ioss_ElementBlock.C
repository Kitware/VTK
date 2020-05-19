// Copyright(C) 1999-2017 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//
//     * Neither the name of NTESS nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "Ioss_BoundingBox.h"  // for AxisAlignedBoundingBox
#include "Ioss_EntityBlock.h"  // for EntityBlock
#include "Ioss_FieldManager.h" // for FieldManager
#include <Ioss_DatabaseIO.h>   // for DatabaseIO
#include <Ioss_ElementBlock.h>
#include <Ioss_Field.h>    // for Field, etc
#include <Ioss_Property.h> // for Property
#include <cstddef>         // for size_t
#include <string>          // for string
#include <vector>          // for vector

namespace Ioss {
  class Field;

  /** \brief Create an element block.
   *
   *  \param[in] io_database The database associated with the region containing the element block.
   *  \param[in] my_name The element block's name.
   *  \param[in] element_type The name of the element topology type for the element block.
   *  \param[in] number_elements The number of elements in the element block.
   */
  ElementBlock::ElementBlock(DatabaseIO *io_database, const std::string &my_name,
                             const std::string &element_type, int64_t number_elements)
      : EntityBlock(io_database, my_name, element_type, number_elements)
  {
    // The 1..global_element_count id.  In a parallel-decomposed run,
    // if maps the element back to its implicit position in the serial
    // undecomposed mesh file.  This is ONLY provided for backward-
    // compatibility and should not be used unless absolutely required.
    fields.add(Ioss::Field("implicit_ids", field_int_type(), "scalar", Ioss::Field::MESH,
                           number_elements));
  }

  ElementBlock::~ElementBlock() = default;

  Property ElementBlock::get_implicit_property(const std::string &my_name) const
  {
    return EntityBlock::get_implicit_property(my_name);
  }

  int64_t ElementBlock::internal_get_field_data(const Field &field, void *data,
                                                size_t data_size) const
  {
    return get_database()->get_field(this, field, data, data_size);
  }

  int64_t ElementBlock::internal_put_field_data(const Field &field, void *data,
                                                size_t data_size) const
  {
    return get_database()->put_field(this, field, data, data_size);
  }

  void ElementBlock::get_block_adjacencies(std::vector<std::string> &block_adjacency) const
  {
    get_database()->get_block_adjacencies(this, block_adjacency);
  }

  AxisAlignedBoundingBox ElementBlock::get_bounding_box() const
  {
    return get_database()->get_bounding_box(this);
  }
} // namespace Ioss
