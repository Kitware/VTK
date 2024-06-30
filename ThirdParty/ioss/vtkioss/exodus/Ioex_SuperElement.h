// Copyright(C) 1999-2020, 2022, 2023, 2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details
#pragma once

#include <cstddef> // for size_t
#include <cstdint> // for int64_t
#include <string>  // for string

#include "Ioss_CodeTypes.h"
#include "Ioss_EntityType.h"     // for EntityType, etc
#include "Ioss_GroupingEntity.h" // for GroupingEntity
#include "Ioss_Property.h"       // for Property
#include "ioex_export.h"
#include "vtk_ioss_mangle.h"

namespace Ioss {
  class Field;
} // namespace Ioss

namespace Ioss {
  class Property;
} // namespace Ioss

namespace Ioex {
  class IOEX_EXPORT SuperElement : public Ioss::GroupingEntity
  {
  public:
    SuperElement(std::string filename, const std::string &my_name);
    ~SuperElement() override;

    IOSS_NODISCARD std::string type_string() const override { return "SuperElement"; }
    IOSS_NODISCARD std::string short_type_string() const override { return "superelement"; }
    IOSS_NODISCARD std::string contains_string() const override { return "Element"; }
    IOSS_NODISCARD Ioss::EntityType type() const override { return Ioss::SUPERELEMENT; }

    // Handle implicit properties -- These are calculated from data stored
    // in the grouping entity instead of having an explicit value assigned.
    // An example would be 'element_block_count' for a region.
    IOSS_NODISCARD Ioss::Property get_implicit_property(const std::string &the_name) const override;

  protected:
    int64_t internal_get_field_data(const Ioss::Field &field, void *data,
                                    size_t data_size) const override;

    int64_t internal_put_field_data(const Ioss::Field &field, void *data,
                                    size_t data_size) const override;

    int64_t internal_get_zc_field_data(const Ioss::Field &, void **, size_t *) const override
    {
      return -1;
    }

  private:
    std::string fileName{};
    size_t      numDOF{0};
    size_t      num_nodes{0};
    size_t      numEIG{0};
    size_t      numRBM{0};
    size_t      num_dim{0};
    int         filePtr{-1};
  };
} // namespace Ioex
