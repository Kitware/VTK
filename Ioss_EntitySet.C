// Copyright(C) 1999-2020 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

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
