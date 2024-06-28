// Copyright(C) 1999-2020, 2022, 2023, 2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "ioss_export.h"
#include "vtk_ioss_mangle.h"

#include "Ioss_EntityType.h"     // for EntityType, etc
#include "Ioss_GroupingEntity.h" // for GroupingEntity
#include "Ioss_Property.h"       // for Property
#include <cstddef>               // for size_t
#include <cstdint>               // for int64_t
#include <string>                // for string
#include <vector>                // for vector

namespace Ioss {
  class DatabaseIO;
  class Field;
  class SideBlock;

  using SideBlockContainer = std::vector<SideBlock *>;

  /** \brief A collection of element sides.
   */
  class IOSS_EXPORT SideSet : public GroupingEntity
  {
  public:
    SideSet(DatabaseIO *io_database, const std::string &my_name);
    SideSet(const SideSet &other);
    ~SideSet() override;

    IOSS_NODISCARD std::string type_string() const override { return "SideSet"; }
    IOSS_NODISCARD std::string short_type_string() const override { return "surface"; }
    IOSS_NODISCARD std::string contains_string() const override { return "Element/Side pair"; }
    IOSS_NODISCARD EntityType  type() const override { return SIDESET; }

    bool                                     add(SideBlock *side_block);
    IOSS_NODISCARD const SideBlockContainer &get_side_blocks() const;
    IOSS_NODISCARD SideBlock                *get_side_block(const std::string &my_name) const;
    IOSS_NODISCARD size_t                    side_block_count() const { return sideBlocks.size(); }

    IOSS_NODISCARD size_t     block_count() const { return sideBlocks.size(); }
    IOSS_NODISCARD SideBlock *get_block(size_t which) const;

    void block_membership(Ioss::NameList &block_members) override;

    // Handle implicit properties -- These are calculated from data stored
    // in the grouping entity instead of having an explicit value assigned.
    // An example would be 'element_block_count' for a region.
    IOSS_NODISCARD Property get_implicit_property(const std::string &my_name) const override;

    IOSS_NODISCARD int  max_parametric_dimension() const;
    IOSS_NODISCARD bool operator==(const SideSet &rhs) const;
    IOSS_NODISCARD bool operator!=(const SideSet &rhs) const;
    IOSS_NODISCARD bool equal(const SideSet &rhs) const;

  protected:
    int64_t internal_get_field_data(const Field &field, void *data,
                                    size_t data_size) const override;

    int64_t internal_put_field_data(const Field &field, void *data,
                                    size_t data_size) const override;

    int64_t internal_get_zc_field_data(const Field &field, void **data,
                                       size_t *data_size) const override;

    bool equal_(const SideSet &rhs, bool quiet) const;

  private:
    SideBlockContainer sideBlocks;
    Ioss::NameList     blockMembership; // What element blocks do the
                                        // elements in this sideset belong to.
  };
} // namespace Ioss
