// Copyright(C) 1999-2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "Ioss_BoundingBox.h"
#include "Ioss_EntityBlock.h"
#include "Ioss_EntityType.h" // for EntityType, etc
#include "Ioss_Property.h"   // for Property
#include <cstddef>           // for size_t
#include <cstdint>           // for int64_t
#include <string>            // for string

#include "Ioss_GroupingEntity.h"
#include "Ioss_PropertyManager.h"
#include "ioss_export.h"
#include "vtk_ioss_mangle.h"

namespace Ioss {
  class DatabaseIO;
} // namespace Ioss
namespace Ioss {
  class Field;
} // namespace Ioss

namespace Ioss {

  /** \brief A collection of all nodes in the region.
   */
  class IOSS_EXPORT NodeBlock : public EntityBlock
  {
  public:
    NodeBlock(DatabaseIO *io_database, const std::string &my_name, int64_t node_count,
              int64_t degrees_of_freedom);

    NodeBlock(const NodeBlock &other);

    IOSS_NODISCARD std::string type_string() const override { return "NodeBlock"; }
    IOSS_NODISCARD std::string short_type_string() const override { return "nodeblock"; }
    IOSS_NODISCARD std::string contains_string() const override { return "Node"; }
    IOSS_NODISCARD EntityType  type() const override { return NODEBLOCK; }

    IOSS_NODISCARD bool is_nonglobal_nodeblock() const
    {
      return properties.exists("IOSS_INTERNAL_CONTAINED_IN");
    }

    IOSS_NODISCARD const GroupingEntity *contained_in() const override
    {
      if (properties.exists("IOSS_INTERNAL_CONTAINED_IN")) {
        auto *ge = properties.get("IOSS_INTERNAL_CONTAINED_IN").get_pointer();
        return static_cast<const GroupingEntity *>(ge);
      }
      return GroupingEntity::contained_in();
    }

    // Handle implicit properties -- These are calculated from data stored
    // in the grouping entity instead of having an explicit value assigned.
    // An example would be 'element_block_count' for a region.
    IOSS_NODISCARD Property get_implicit_property(const std::string &my_name) const override;

    IOSS_NODISCARD AxisAlignedBoundingBox get_bounding_box() const;
    IOSS_NODISCARD bool                   operator!=(const Ioss::NodeBlock &rhs) const;
    IOSS_NODISCARD bool                   operator==(const Ioss::NodeBlock &rhs) const;
    IOSS_NODISCARD bool                   equal(const Ioss::NodeBlock &rhs) const;

  protected:
    int64_t internal_get_field_data(const Field &field, void *data,
                                    size_t data_size) const override;

    int64_t internal_put_field_data(const Field &field, void *data,
                                    size_t data_size) const override;

    int64_t internal_get_zc_field_data(const Field &field, void **data,
                                       size_t *data_size) const override;
  };
} // namespace Ioss
