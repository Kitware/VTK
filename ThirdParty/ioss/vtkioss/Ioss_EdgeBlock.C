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

#include <Ioss_CodeTypes.h> // for IntVector
#include <Ioss_DatabaseIO.h>
#include <Ioss_EdgeBlock.h>
#include <Ioss_ElementTopology.h>
#include <Ioss_Property.h>
#include <cstddef>
#include <string>

#include "Ioss_EntityBlock.h"
#include "Ioss_PropertyManager.h"

namespace Ioss {
  class Field;
} // namespace Ioss

/** \brief Create an edge block.
 *
 *  \param[in] io_database The database associated with the region containing the edge block.
 *  \param[in] my_name The edge block's name.
 *  \param[in] edge_type The name of the edge topology type for the edge block.
 *  \param[in] number_edges The number of edges in the edge block.
 */
Ioss::EdgeBlock::EdgeBlock(Ioss::DatabaseIO *io_database, const std::string &my_name,
                           const std::string &edge_type, int64_t number_edges)
    : Ioss::EntityBlock(io_database, my_name, edge_type, number_edges)
{
  if (topology()->master_element_name() != edge_type && topology()->name() != edge_type) {
    // Maintain original edge type on output database if possible.
    properties.add(Ioss::Property("original_edge_type", edge_type));
  }
}

Ioss::EdgeBlock::~EdgeBlock() = default;

Ioss::Property Ioss::EdgeBlock::get_implicit_property(const std::string &my_name) const
{
  return Ioss::EntityBlock::get_implicit_property(my_name);
}

int64_t Ioss::EdgeBlock::internal_get_field_data(const Ioss::Field &field, void *data,
                                                 size_t data_size) const
{
  return get_database()->get_field(this, field, data, data_size);
}

int64_t Ioss::EdgeBlock::internal_put_field_data(const Ioss::Field &field, void *data,
                                                 size_t data_size) const
{
  return get_database()->put_field(this, field, data, data_size);
}
