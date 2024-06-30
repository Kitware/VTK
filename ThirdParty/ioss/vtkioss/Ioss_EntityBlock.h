// Copyright(C) 1999-2020, 2022, 2023, 2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "Ioss_GroupingEntity.h" // for GroupingEntity
#include "Ioss_Property.h"       // for Property
#include <cstddef>               // for size_t
#include <string>                // for string

#include "ioss_export.h"
#include "vtk_ioss_mangle.h"

namespace Ioss {
  class DatabaseIO;
} // namespace Ioss
namespace Ioss {
  class ElementTopology;
} // namespace Ioss

namespace Ioss {
  class ElementBlock;

  /** \brief Base class for all 'block'-type grouping entities, which means all
   *         members of the block are similar or have the same topology.
   *
   *   The following derived classes are typical:
   *
   *   -- NodeBlock -- grouping of 'similar' nodes (same degree of freedom, ...)
   *
   *   -- ElementBlock -- grouping of 'similar' elements (same element topology,
   *                      attributes, ...)
   *      0d, 1d, 2d, 3d topology possible -- e.g., sphere, bar, quad, hex
   */
  class IOSS_EXPORT EntityBlock : public GroupingEntity
  {
  public:
    EntityBlock &operator=(const EntityBlock &) = delete;

    IOSS_NODISCARD Property get_implicit_property(const std::string &my_name) const override = 0;

    /** \brief Get the topology of the entities in the block.
     *
     *  \returns The topology.
     */
    IOSS_NODISCARD const ElementTopology *topology() const { return topology_; }

    /** \brief Determine whether the block contains the entity with a given id.
     *
     *  \param[in] local_id The id to check.
     *  \returns True if the block contains the entity.
     */
    IOSS_NODISCARD bool contains(size_t local_id) const
    {
      return idOffset < local_id && local_id <= idOffset + entityCount;
    }
    /** \brief Set the 'offset' for the block.
     *
     *  The 'offset' is used to map an element location within an
     *  element block to the element 'file descriptor'.
     *  For example, the file descriptor of the 37th element in the 4th
     *  block is calculated by:
     *
     *  file_descriptor = offset of block 4 + 37
     *
     *  This can also be used to determine which element block
     *  an element with a file_descriptor maps into. An particular
     *  element block contains all elements in the range:
     *
     *  offset < file_descriptor <= offset+number_elements_per_block
     */
    void set_offset(size_t offset) { idOffset = offset; }

    /** \brief Get the 'offset' for the block.
     *
     *  The 'offset' is used to map an element location within an
     *  element block to the element 'file descriptor'.
     *  For example, the file descriptor of the 37th element in the 4th
     *  block is calculated by:
     *
     *  file_descriptor = offset of block 4 + 37
     *
     *  This can also be used to determine which element block
     *  an element with a file_descriptor maps into. An particular
     *  element block contains all elements in the range:
     *
     *  offset < file_descriptor <= offset+number_elements_per_block
     */
    IOSS_NODISCARD size_t get_offset() const { return idOffset; }

    IOSS_NODISCARD bool operator==(const Ioss::EntityBlock &rhs) const;
    IOSS_NODISCARD bool operator!=(const Ioss::EntityBlock &rhs) const;
    IOSS_NODISCARD bool equal(const Ioss::EntityBlock &rhs) const;

  protected:
    EntityBlock(DatabaseIO *io_database, const std::string &my_name, const std::string &entity_type,
                size_t entity_cnt);

    EntityBlock(const EntityBlock &) = default;

    ElementTopology *topology_{nullptr};

    size_t idOffset{0};

    bool equal_(const Ioss::EntityBlock &rhs, bool quiet) const;
  };
} // namespace Ioss
