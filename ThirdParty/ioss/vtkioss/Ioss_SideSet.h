// Copyright(C) 1999-2020, 2022 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "ioss_export.h"

#include "vtk_ioss_mangle.h"

#include "Ioss_EntityType.h"     // for EntityType, etc
#include <Ioss_GroupingEntity.h> // for GroupingEntity
#include <Ioss_Property.h>       // for Property
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
    SideSet(const SideSet &);
    ~SideSet() override;

    std::string type_string() const override { return "SideSet"; }
    std::string short_type_string() const override { return "surface"; }
    std::string contains_string() const override { return "Element/Side pair"; }
    EntityType  type() const override { return SIDESET; }

    bool                      add(SideBlock *side_block);
    const SideBlockContainer &get_side_blocks() const;
    SideBlock                *get_side_block(const std::string &my_name) const;
    size_t                    side_block_count() const { return sideBlocks.size(); }

    size_t     block_count() const { return sideBlocks.size(); }
    SideBlock *get_block(size_t which) const;

    void block_membership(std::vector<std::string> &block_members) override;

    // Handle implicit properties -- These are calcuated from data stored
    // in the grouping entity instead of having an explicit value assigned.
    // An example would be 'element_block_count' for a region.
    Property get_implicit_property(const std::string &my_name) const override;

    int  max_parametric_dimension() const;
    bool operator==(const SideSet &) const;
    bool operator!=(const SideSet &) const;
    bool equal(const SideSet &) const;

  protected:
    int64_t internal_get_field_data(const Field &field, void *data,
                                    size_t data_size) const override;

    int64_t internal_put_field_data(const Field &field, void *data,
                                    size_t data_size) const override;

    bool equal_(const SideSet &rhs, const bool quiet) const;

  private:
    SideBlockContainer       sideBlocks;
    std::vector<std::string> blockMembership; // What element blocks do the
                                              // elements in this sideset belong to.
  };
} // namespace Ioss
