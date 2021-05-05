// Copyright(C) 1999-2020 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#ifndef IOSS_Ioss_CommSet_h
#define IOSS_Ioss_CommSet_h

#include "vtk_ioss_mangle.h"

#include "Ioss_EntityType.h"     // for EntityType, etc
#include <Ioss_GroupingEntity.h> // for GroupingEntity
#include <Ioss_Property.h>       // for Property
#include <cstddef>               // for size_t
#include <cstdint>               // for int64_t
#include <string>                // for string
namespace Ioss {
  class DatabaseIO;
} // namespace Ioss
namespace Ioss {
  class Field;
} // namespace Ioss

namespace Ioss {

  class CommSet : public GroupingEntity
  {
  public:
    CommSet(DatabaseIO *io_database, const std::string &my_name, const std::string &entity_type,
            size_t entity_cnt);
    CommSet(const CommSet &) = default;

    std::string type_string() const override { return "CommSet"; }
    std::string short_type_string() const override { return "commlist"; }
    std::string contains_string() const override { return "Entity/Processor pair"; }
    EntityType  type() const override { return COMMSET; }

    // Handle implicit properties -- These are calcuated from data stored
    // in the grouping entity instead of having an explicit value assigned.
    // An example would be 'element_block_count' for a region.
    Property get_implicit_property(const std::string &my_name) const override;

  protected:
    int64_t internal_get_field_data(const Field &field, void *data,
                                    size_t data_size) const override;

    int64_t internal_put_field_data(const Field &field, void *data,
                                    size_t data_size) const override;
  };
} // namespace Ioss
#endif
