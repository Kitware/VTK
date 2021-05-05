// Copyright(C) 1999-2020 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#ifndef IOSS_Ioss_NullEntity_h
#define IOSS_Ioss_NullEntity_h

#include "vtk_ioss_mangle.h"

#include <Ioss_CodeTypes.h>
#include <Ioss_GroupingEntity.h>
#include <string>

namespace Ioss {
  class DatabaseIO;

  class NullEntity : public GroupingEntity
  {
  public:
    NullEntity() : Ioss::GroupingEntity(nullptr, "null_entity", 0) {}

    std::string type_string() const override { return "NullEntity"; }
    std::string short_type_string() const override { return "null"; }
    std::string contains_string() const override { return "Nothing"; }
    EntityType  type() const override { return INVALID_TYPE; }

    // Handle implicit properties -- These are calcuated from data stored
    // in the grouping entity instead of having an explicit value assigned.
    // An example would be 'element_block_count' for a region.
    Property get_implicit_property(const std::string &my_name) const override
    {
      return Ioss::GroupingEntity::get_implicit_property(my_name);
    }

  protected:
    int64_t internal_get_field_data(const Field &, void *, size_t) const override { return 0; }

    int64_t internal_put_field_data(const Field &, void *, size_t) const override { return 0; }
  };
} // namespace Ioss
#endif
