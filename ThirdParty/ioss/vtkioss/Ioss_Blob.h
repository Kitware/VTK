// Copyright(C) 1999-2020, 2022 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "ioss_export.h"

#include "vtk_ioss_mangle.h"

#include "Ioss_EntityType.h" // for EntityType, etc
#include "Ioss_Property.h"   // for Property
#include <Ioss_GroupingEntity.h>
#include <cstddef> // for size_t
#include <cstdint> // for int64_t
#include <string>  // for string
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
    ~Blob() override        = default;

    Blob(DatabaseIO *io_database, const std::string &my_name, int64_t item_count);

    std::string type_string() const override { return "Blob"; }
    std::string short_type_string() const override { return "blob"; }
    std::string contains_string() const override { return "Entries"; }
    EntityType  type() const override { return BLOB; }

    // Handle implicit properties -- These are calcuated from data stored
    // in the grouping entity instead of having an explicit value assigned.
    // An example would be 'element_block_count' for a region.
    Property get_implicit_property(const std::string &my_name) const override;

  protected:
    int64_t internal_get_field_data(const Field &field, void *data,
                                    size_t data_size) const override;

    int64_t internal_put_field_data(const Field &field, void *data,
                                    size_t data_size) const override;
  };
} // namespace Ioss
