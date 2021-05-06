// Copyright(C) 1999-2020 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include <Ioss_DatabaseIO.h>
#include <Ioss_NodeSet.h>
#include <Ioss_Property.h>
#include <cstddef>
#include <string>

#include "Ioss_EntitySet.h"
#include "Ioss_GroupingEntity.h"

namespace Ioss {
  class Field;
} // namespace Ioss

Ioss::NodeSet::NodeSet() : Ioss::EntitySet(nullptr, "invalid", 0) {}

/** \brief Create a node set.
 *
 *  \param[in] io_database The database associated with the region containing the node set.
 *  \param[in] my_name The node set's name.
 *  \param[in] number_nodes The number of nodes in the node set.
 */
Ioss::NodeSet::NodeSet(Ioss::DatabaseIO *io_database, const std::string &my_name,
                       int64_t number_nodes)
    : Ioss::EntitySet(io_database, my_name, number_nodes)
{
}

int64_t Ioss::NodeSet::internal_get_field_data(const Ioss::Field &field, void *data,
                                               size_t data_size) const
{
  return get_database()->get_field(this, field, data, data_size);
}

int64_t Ioss::NodeSet::internal_put_field_data(const Ioss::Field &field, void *data,
                                               size_t data_size) const
{
  return get_database()->put_field(this, field, data, data_size);
}

Ioss::Property Ioss::NodeSet::get_implicit_property(const std::string &my_name) const
{
  return Ioss::GroupingEntity::get_implicit_property(my_name);
}
