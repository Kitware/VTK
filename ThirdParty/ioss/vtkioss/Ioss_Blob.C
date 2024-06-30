// Copyright(C) 1999-2020, 2023 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_Blob.h"
#include "Ioss_DatabaseIO.h"
#include "Ioss_GroupingEntity.h"
#include "Ioss_Property.h"
#include <cstddef>

namespace Ioss {
  class Field;
} // namespace Ioss

/** \brief Create a blob with no members initially.
 *
 *  \param[in] io_database The database associated with the region containing the blob.
 *  \param[in] my_name The blob's name.
 *  \param[in] item_count The number of items stored in this blob
 */
Ioss::Blob::Blob(Ioss::DatabaseIO *io_database, const std::string &my_name, int64_t item_count)
    : Ioss::GroupingEntity(io_database, my_name, item_count)
{
}

int64_t Ioss::Blob::internal_get_field_data(const Ioss::Field &field, void *data,
                                            size_t data_size) const
{
  return get_database()->get_field(this, field, data, data_size);
}

int64_t Ioss::Blob::internal_put_field_data(const Ioss::Field &field, void *data,
                                            size_t data_size) const
{
  return get_database()->put_field(this, field, data, data_size);
}

int64_t Ioss::Blob::internal_get_zc_field_data(const Field &field, void **data,
                                               size_t *data_size) const
{
  return get_database()->get_zc_field(this, field, data, data_size);
}

Ioss::Property Ioss::Blob::get_implicit_property(const std::string &my_name) const
{
  return Ioss::GroupingEntity::get_implicit_property(my_name);
}
