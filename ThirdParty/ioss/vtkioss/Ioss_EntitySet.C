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

#include <Ioss_EntitySet.h>
#include <Ioss_Field.h>
#include <Ioss_Property.h>
#include <cstddef>
#include <string>

#include "Ioss_FieldManager.h"
#include "Ioss_GroupingEntity.h"
#include "Ioss_PropertyManager.h"

namespace Ioss {
  class DatabaseIO;
} // namespace Ioss

/** \brief Base class constructor adds "name" and "entity_count" properties to the set.
 *
 *  \param[in] io_database The database associated with the set.
 *  \param[in] my_name The set name.
 *  \param[in] entity_cnt The number of subentities in the set.
 *
 */
Ioss::EntitySet::EntitySet(Ioss::DatabaseIO *io_database, const std::string &my_name,
                           size_t entity_cnt)
    : Ioss::GroupingEntity(io_database, my_name, entity_cnt)
{
  properties.add(Ioss::Property("distribution_factor_count", static_cast<int>(entity_cnt)));

  // Add the standard fields...
  fields.add(Ioss::Field("distribution_factors", Ioss::Field::REAL, "scalar", Ioss::Field::MESH,
                         entity_cnt));
  fields.add(Ioss::Field("ids_raw", field_int_type(), "scalar", Ioss::Field::MESH, entity_cnt));
}

Ioss::Property Ioss::EntitySet::get_implicit_property(const std::string &my_name) const
{
  return Ioss::GroupingEntity::get_implicit_property(my_name);
}
