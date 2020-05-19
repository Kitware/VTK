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

#include <Ioss_CommSet.h>
#include <Ioss_DatabaseIO.h>
#include <Ioss_Field.h>
#include <Ioss_Property.h>
#include <cassert>
#include <cstddef>
#include <string>

#include "Ioss_FieldManager.h"
#include "Ioss_GroupingEntity.h"
#include "Ioss_PropertyManager.h"

Ioss::CommSet::CommSet(Ioss::DatabaseIO *io_database, const std::string &my_name,
                       const std::string &entity_type, size_t entity_cnt)
    : Ioss::GroupingEntity(io_database, my_name, entity_cnt)
{
  assert(entity_type == "node" || entity_type == "side");
  properties.add(Ioss::Property("entity_type", entity_type));

  if (entity_type == "node") {
    // Field contains a pair of type [entity_id, shared_cpu]
    fields.add(Ioss::Field("entity_processor", field_int_type(), "pair", Ioss::Field::COMMUNICATION,
                           entity_cnt));
    fields.add(Ioss::Field("entity_processor_raw", field_int_type(), "pair",
                           Ioss::Field::COMMUNICATION, entity_cnt));
  }
  else {
    // Field contains a triplet of type [entity_id, local_side, shared_cpu]
    fields.add(Ioss::Field("entity_processor", field_int_type(), "Real[3]",
                           Ioss::Field::COMMUNICATION, entity_cnt));
    fields.add(Ioss::Field("entity_processor_raw", field_int_type(), "Real[3]",
                           Ioss::Field::COMMUNICATION, entity_cnt));
  }
}

int64_t Ioss::CommSet::internal_get_field_data(const Ioss::Field &field, void *data,
                                               size_t data_size) const
{
  return get_database()->get_field(this, field, data, data_size);
}

int64_t Ioss::CommSet::internal_put_field_data(const Ioss::Field &field, void *data,
                                               size_t data_size) const
{
  return get_database()->put_field(this, field, data, data_size);
}

Ioss::Property Ioss::CommSet::get_implicit_property(const std::string &my_name) const
{
  return Ioss::GroupingEntity::get_implicit_property(my_name);
}
