// Copyright(C) 1999-2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "Ioss_ElementBlock.h"
#include "Ioss_EntityBlock.h" // for EntityBlock
#include "Ioss_EntityType.h"  // for EntityType, etc
#include "Ioss_Property.h"    // for Property
#include "Ioss_SideSet.h"
#include <cstddef> // for size_t
#include <cstdint> // for int64_t
#include <string>  // for string
#include <vector>  // for vector

#include "Ioss_GroupingEntity.h"
#include "ioss_export.h"
#include "vtk_ioss_mangle.h"

namespace Ioss {
  class DatabaseIO;
} // namespace Ioss
namespace Ioss {
  class ElementTopology;
} // namespace Ioss
namespace Ioss {
  class Field;
} // namespace Ioss

namespace Ioss {

  /** \brief A collection of element sides having the same topology.
   */
  class IOSS_EXPORT SideBlock : public EntityBlock
  {
  public:
    friend class SideSet;

    SideBlock(DatabaseIO *io_database, const std::string &my_name, const std::string &side_type,
              const std::string &element_type, size_t side_count);

    SideBlock(const SideBlock &other);

    /**
     *
     * For externally defined sidesets/sideblocks, attempt to provide
     * the sideblock name that will be generated if the database is
     * read and the sideblocks are generated from the sideset at the
     * read phase.  Since sideblocks are not explicitly stored on some
     * of the database types (e.g. exodus), the IOSS code generates
     * the sideblocks from the sidesets when reading the database.  We
     * want to maximize the possibility that the same sideblock names
     * will be generated at that read step as the application is using
     * for sideblocks that it generates internally to be output to a
     * restart file that is later read...
     *
     * \param[in] sideset_name The name of the sideset that this sideblock will be a member of.
     * \param[in] block_or_element Depending on the `SurfaceSplitType`
     * behavior for this database, this is either the name of the
     * element block that the sideblock is applied to
     * (SPLIT_BY_ELEMENT_BLOCK) or the topology name of the elements
     * that the sideblock faces are part of (SPLIT_BY_TOPOLOGIES) or
     * "UNKNOWN" if mixed topology (SPLIT_BY_DONT_SPLIT)
     * \param[in] face_topology_name The name of the topology of the
     * sideblock faces. "UNKNOWN" if not homogeneous.
     * \returns The generated sideblock name.
     */
    IOSS_NODISCARD static std::string
    generate_sideblock_name(const std::string &sideset_name, const std::string &block_or_element,
                            const std::string &face_topology_name);

    IOSS_NODISCARD std::string type_string() const override { return "SideBlock"; }
    IOSS_NODISCARD std::string short_type_string() const override { return "sideblock"; }
    IOSS_NODISCARD std::string contains_string() const override { return "Element/Side pair"; }
    IOSS_NODISCARD EntityType  type() const override { return SIDEBLOCK; }

    IOSS_NODISCARD const SideSet *owner() const { return owner_; }
    IOSS_NODISCARD const Ioss::GroupingEntity *contained_in() const override { return owner_; }

    void block_membership(Ioss::NameList &block_members) override;

    // Handle implicit properties -- These are calculated from data stored
    // in the grouping entity instead of having an explicit value assigned.
    // An example would be 'element_block_count' for a region.
    IOSS_NODISCARD Property get_implicit_property(const std::string &my_name) const override;

    // For faceblock, edgeblock, if they are split by element block, then this
    // will be non-nullptr and is a pointer to the parent element block for this
    // faceblock or edgeblock. Has no meaning for other EntityBlock types or split
    // types.
    IOSS_NODISCARD const ElementBlock *parent_element_block() const
    {
      return dynamic_cast<const ElementBlock *>(parentBlock_);
    }

    void set_parent_element_block(const ElementBlock *element_block)
    {
      parentBlock_ = element_block;
    }

    IOSS_NODISCARD const EntityBlock *parent_block() const { return parentBlock_; }
    void set_parent_block(const EntityBlock *block) { parentBlock_ = block; }

    // Describes the contained entities element block topology
    IOSS_NODISCARD const ElementTopology *parent_element_topology() const
    {
      return parentTopology_;
    }

    // For faceblock, edgeblock, return whether the surface is applied
    // to the same face/edge for all elements in the surface. If not,
    // return 0; otherwise return the consistent face number.
    IOSS_NODISCARD int get_consistent_side_number() const;
    void               set_consistent_side_number(int side) { consistentSideNumber = side; }

    IOSS_NODISCARD bool operator==(const SideBlock &rhs) const;
    IOSS_NODISCARD bool operator!=(const SideBlock &rhs) const;
    IOSS_NODISCARD bool equal(const SideBlock &rhs) const;

  protected:
    int64_t internal_get_field_data(const Field &field, void *data,
                                    size_t data_size) const override;

    int64_t internal_put_field_data(const Field &field, void *data,
                                    size_t data_size) const override;

    int64_t internal_get_zc_field_data(const Field &field, void **data,
                                       size_t *data_size) const override;

  private:
    bool equal_(const SideBlock &rhs, bool quiet) const;

    const SideSet     *owner_{nullptr};
    ElementTopology   *parentTopology_{nullptr}; // Topology of parent element (if any)
    const EntityBlock *parentBlock_{nullptr};

    // Pointer to the SideSet (if any) that contains this side block.
    Ioss::NameList blockMembership{}; // What element blocks do the
                                      // elements in this sideset belong to.
    mutable int consistentSideNumber{-1};
  };
} // namespace Ioss
