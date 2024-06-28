// Copyright(C) 1999-2020, 2022, 2023, 2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "Ioss_GroupingEntity.h"
#include <cstddef> // for size_t
#include <cstdint> // for int64_t
#include <string>  // for string
#include <vector>

#include "Ioss_EntityType.h" // for EntityType, etc
#include "Ioss_Property.h"   // for Property
#include "ioss_export.h"
#include "vtk_ioss_mangle.h"

namespace Ioss {
  class DatabaseIO;
  class Field;
} // namespace Ioss

namespace Ioss {

  using EntityContainer = std::vector<const Ioss::GroupingEntity *>;

  /** \brief A homogeneous collection of other GroupingEntities.
   */
  class IOSS_EXPORT Assembly : public GroupingEntity
  {
  public:
    Assembly()                 = default; // Used for template typing only
    Assembly(const Assembly &) = default;

    Assembly(DatabaseIO *io_database, const std::string &my_name);

    IOSS_NODISCARD std::string type_string() const override { return "Assembly"; }
    IOSS_NODISCARD std::string short_type_string() const override { return "assembly"; }
    IOSS_NODISCARD std::string contains_string() const override
    {
      return m_members.empty() ? "<EMPTY>" : m_members[0]->type_string();
    }
    IOSS_NODISCARD EntityType type() const override { return ASSEMBLY; }

    IOSS_NODISCARD EntityType get_member_type() const { return m_type; }

    bool                                  add(const GroupingEntity *member);
    bool                                  remove(const GroupingEntity *removal);
    IOSS_NODISCARD const EntityContainer &get_members() const;
    IOSS_NODISCARD const GroupingEntity  *get_member(const std::string &my_name) const;
    void                                  remove_members();
    IOSS_NODISCARD size_t                 member_count() const { return m_members.size(); }

    // Handle implicit properties -- These are calculated from data stored
    // in the grouping entity instead of having an explicit value assigned.
    // An example would be 'element_block_count' for a region.
    IOSS_NODISCARD Property get_implicit_property(const std::string &my_name) const override;

  protected:
    int64_t internal_get_field_data(const Field &field, void *data,
                                    size_t data_size) const override;

    int64_t internal_put_field_data(const Field &field, void *data,
                                    size_t data_size) const override;

    int64_t internal_get_zc_field_data(const Field &field, void **data,
                                       size_t *data_size) const override;

  private:
    EntityContainer m_members;
    EntityType      m_type{INVALID_TYPE};
  };
} // namespace Ioss
