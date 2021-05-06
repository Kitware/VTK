// Copyright(C) 1999-2020 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#ifndef IOSS_Ioss_EntitySet_h
#define IOSS_Ioss_EntitySet_h

#include "vtk_ioss_mangle.h"

#include <Ioss_GroupingEntity.h> // for GroupingEntity
#include <Ioss_Property.h>       // for Property
#include <cstddef>               // for size_t
#include <string>                // for string
namespace Ioss {
  class DatabaseIO;
} // namespace Ioss

namespace Ioss {
  class ElementSet;

  /** \brief Base class for all 'set'-type grouping entities, which means that members
   *         of the set are not necessarily similar, or do not necessarily have the
   *         same topology.
   *
   *   The following derived classes are typical:
   *
   *   -- NodeSet  -- grouping of nodes (0d topology)
   *
   *   -- EdgeSet  -- grouping of edges (1d topology)
   *
   *   -- FaceSet  -- grouping of faces (2d topology) [Surface]
   *
   *
   */
  class EntitySet : public GroupingEntity
  {
  public:
    EntitySet &operator=(const EntitySet &)                                     = delete;
    Property   get_implicit_property(const std::string &my_name) const override = 0;

  protected:
    EntitySet(DatabaseIO *io_database, const std::string &my_name, size_t entity_cnt);
    EntitySet(const EntitySet &) = default;
    ~EntitySet() override        = default;

  protected:
    void count_attributes() const;
  };
} // namespace Ioss
#endif
