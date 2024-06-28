// Copyright(C) 1999-2020, 2023 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_CommSet.h"
#include "Ioss_DatabaseIO.h"
#include "Ioss_Field.h"
#include "Ioss_Property.h"
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

int64_t Ioss::CommSet::internal_get_zc_field_data(const Field &field, void **data,
                                                  size_t *data_size) const
{
  return get_database()->get_zc_field(this, field, data, data_size);
}

Ioss::Property Ioss::CommSet::get_implicit_property(const std::string &my_name) const
{
  return Ioss::GroupingEntity::get_implicit_property(my_name);
}
