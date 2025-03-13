// Copyright(C) 1999-2020, 2022, 2023, 2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "Ioss_ChangeSet.h"        // for ChangeSet
#include "Ioss_ChangeSetFactory.h" // for ChangeSetFactory
#include "Ioss_CodeTypes.h"
#include "Ioss_DBUsage.h" // for DatabaseUsage
#include "Ioss_Map.h"     // for Map
#include <cstddef>        // for size_t
#include <cstdint>        // for int64_t
#include <string>         // for string
#include <vector>         // for vector

#include "Ioss_State.h" // for State
#include "ioex_export.h"
#include "vtk_ioss_mangle.h"

namespace Ioss {
  class Region;
} // namespace Ioss

/** \brief A namespace for the exodus change set type.
 */
namespace Ioex {
  class IOEX_EXPORT ChangeSetFactory : public Ioss::ChangeSetFactory
  {
  public:
    static const ChangeSetFactory *factory();

  private:
    ChangeSetFactory();
    Ioss::ChangeSet *make_ChangeSet(Ioss::Region *region) const override;
    Ioss::ChangeSet *make_ChangeSet(Ioss::DatabaseIO *db, const std::string &dbName,
                                    const std::string &dbType,
                                    unsigned           fileCyclicCount) const override;
  };

  class IOEX_EXPORT ChangeSet : public Ioss::ChangeSet
  {
  public:
    explicit ChangeSet(Ioss::Region *region);
    ChangeSet(Ioss::DatabaseIO *db, const std::string &dbName, const std::string &dbType,
              unsigned fileCyclicCount);

    ~ChangeSet() override;

    void populate_change_sets(bool loadAllFiles = true) override;

    IOSS_NODISCARD Ioss::DatabaseIO *open_change_set(unsigned            index,
                                                     Ioss::DatabaseUsage usage) override;
    void                             close_change_set(unsigned index) override;

  private:
    ChangeSet()                  = delete;
    ChangeSet(const ChangeSet &) = delete;

    std::string m_currentChangeSet;

  protected:
    void get_group_change_sets();
    void clear_change_sets() override;
    bool supports_group();
  };

} // namespace Ioex
