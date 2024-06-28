// Copyright(C) 1999-2020, 2022, 2023, 2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "Ioss_CodeTypes.h"
#include "Ioss_GroupingEntity.h"
#include <stddef.h>
#include <stdint.h>
#include <string>

#include "Ioss_EntityType.h"
#include "Ioss_Property.h"
#include "ioss_export.h"
#include "vtk_ioss_mangle.h"

namespace Ioss {
  class DatabaseIO;
  class Field;

  class IOSS_EXPORT NullEntity : public GroupingEntity
  {
  public:
    NullEntity();

    IOSS_NODISCARD std::string type_string() const override { return "NullEntity"; }
    IOSS_NODISCARD std::string short_type_string() const override { return "null"; }
    IOSS_NODISCARD std::string contains_string() const override { return "Nothing"; }
    IOSS_NODISCARD EntityType  type() const override { return INVALID_TYPE; }

    // Handle implicit properties -- These are calculated from data stored
    // in the grouping entity instead of having an explicit value assigned.
    // An example would be 'element_block_count' for a region.
    IOSS_NODISCARD Property get_implicit_property(const std::string &my_name) const override
    {
      return Ioss::GroupingEntity::get_implicit_property(my_name);
    }

  protected:
    int64_t internal_get_field_data(const Field &, void *, size_t) const override { return 0; }

    int64_t internal_put_field_data(const Field &, void *, size_t) const override { return 0; }

    int64_t internal_get_zc_field_data(const Field &, void **, size_t *) const override
    {
      return 0;
    }
  };
} // namespace Ioss
