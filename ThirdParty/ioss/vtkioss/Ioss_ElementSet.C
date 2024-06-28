// Copyright(C) 1999-2020, 2023 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_DatabaseIO.h"
#include "Ioss_ElementSet.h"
#include "Ioss_Property.h"
#include <cstddef>
#include <vector>

#include "Ioss_EntitySet.h"

namespace Ioss {
  class Field;
} // namespace Ioss

Ioss::ElementSet::ElementSet() : Ioss::EntitySet(nullptr, "invalid", 0) {}

/** \brief Create an element set.
 *
 *  \param[in] io_database The database associated with the region containing the element set.
 *  \param[in] my_name The element set's name.
 *  \param[in] number_elements The number of elements in the element set.
 */
Ioss::ElementSet::ElementSet(Ioss::DatabaseIO *io_database, const std::string &my_name,
                             int64_t number_elements)
    : Ioss::EntitySet(io_database, my_name, number_elements)
{
}

int64_t Ioss::ElementSet::internal_get_field_data(const Ioss::Field &field, void *data,
                                                  size_t data_size) const
{
  return get_database()->get_field(this, field, data, data_size);
}

int64_t Ioss::ElementSet::internal_put_field_data(const Ioss::Field &field, void *data,
                                                  size_t data_size) const
{
  return get_database()->put_field(this, field, data, data_size);
}

int64_t Ioss::ElementSet::internal_get_zc_field_data(const Field &field, void **data,
                                                     size_t *data_size) const
{
  return get_database()->get_zc_field(this, field, data, data_size);
}

Ioss::Property Ioss::ElementSet::get_implicit_property(const std::string &my_name) const
{
  return Ioss::EntitySet::get_implicit_property(my_name);
}

void Ioss::ElementSet::block_membership(Ioss::NameList & /*block_members*/) {}
