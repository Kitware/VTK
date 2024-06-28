// Copyright(C) 1999-2020, 2022, 2023, 2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "Ioss_GroupingEntity.h"
#include <cstddef> // for size_t
#include <cstdint> // for int64_t
#include <string>  // for string
#include <vector>

#include "Ioss_EntityType.h" // for EntityType, etc
#include "Ioss_Property.h"   // for Property
#include "ioss_export.h"
#include "vtk_ioss_mangle.h"

namespace Ioss {
  class DatabaseIO;
  class Field;
} // namespace Ioss

namespace Ioss {

  using EntityContainer = std::vector<const Ioss::GroupingEntity *>;

  /** \brief A homogeneous collection of other GroupingEntities.
   */
  class IOSS_EXPORT Blob : public GroupingEntity
  {
  public:
    Blob()                  = default; // Used for template typing only
    Blob(const Blob &other) = default;

    Blob(DatabaseIO *io_database, const std::string &my_name, int64_t item_count);

    IOSS_NODISCARD std::string type_string() const override { return "Blob"; }
    IOSS_NODISCARD std::string short_type_string() const override { return "blob"; }
    IOSS_NODISCARD std::string contains_string() const override { return "Entries"; }
    IOSS_NODISCARD EntityType  type() const override { return BLOB; }

    // Handle implicit properties -- These are calculated from data stored
    // in the grouping entity instead of having an explicit value assigned.
    // An example would be 'element_block_count' for a region.
    IOSS_NODISCARD Property get_implicit_property(const std::string &my_name) const override;

  protected:
    int64_t internal_get_field_data(const Field &field, void *data,
                                    size_t data_size) const override;

    int64_t internal_put_field_data(const Field &field, void *data,
                                    size_t data_size) const override;

    int64_t internal_get_zc_field_data(const Field &field, void **data,
                                       size_t *data_size) const override;
  };
} // namespace Ioss
