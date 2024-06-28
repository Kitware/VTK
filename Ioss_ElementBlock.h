// Copyright(C) 1999-2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "Ioss_BoundingBox.h"
#include "Ioss_CodeTypes.h"
#include "Ioss_EntityBlock.h"
#include "Ioss_Property.h"
#include <cassert>
#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>

#include "Ioss_EntityType.h"
#include "ioss_export.h"
#include "vtk_ioss_mangle.h"

namespace Ioss {
  class DatabaseIO;
  class Field;

  /** \brief A collection of elements having the same topology.
   */
  class IOSS_EXPORT ElementBlock : public EntityBlock
  {
  public:
    ElementBlock(DatabaseIO *io_database, const std::string &my_name,
                 const std::string &element_type, int64_t number_elements);

    ElementBlock(const ElementBlock &) = default;

    IOSS_NODISCARD std::string type_string() const override { return "ElementBlock"; }
    IOSS_NODISCARD std::string short_type_string() const override { return "block"; }
    IOSS_NODISCARD std::string contains_string() const override { return "Element"; }
    IOSS_NODISCARD EntityType  type() const override { return ELEMENTBLOCK; }

    // Handle implicit properties -- These are calculated from data stored
    // in the grouping entity instead of having an explicit value assigned.
    // An example would be 'element_block_count' for a region.
    IOSS_NODISCARD Property get_implicit_property(const std::string &my_name) const override;

    IOSS_NODISCARD Ioss::NameList get_block_adjacencies() const;
    void                          get_block_adjacencies(Ioss::NameList &block_adjacency) const;
    IOSS_NODISCARD AxisAlignedBoundingBox get_bounding_box() const;
    IOSS_NODISCARD bool                   operator==(const Ioss::ElementBlock &rhs) const;
    IOSS_NODISCARD bool                   operator!=(const Ioss::ElementBlock &rhs) const;
    IOSS_NODISCARD bool                   equal(const Ioss::ElementBlock &rhs) const;

  protected:
    int64_t internal_get_field_data(const Field &field, void *data,
                                    size_t data_size) const override;

    int64_t internal_put_field_data(const Field &field, void *data,
                                    size_t data_size) const override;

    int64_t internal_get_zc_field_data(const Field &field, void **data,
                                       size_t *data_size) const override;

  private:
  };
} // namespace Ioss
