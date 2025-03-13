// Copyright(C) 1999-2020, 2022, 2023 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioex_ChangeSet.h"
#include "Ioex_DatabaseIO.h"

#include "Ioss_CodeTypes.h" // for Int64Vector, IntVector
#include "Ioss_SmartAssert.h"
#include <cassert> // for assert
#include <cmath>   // for sqrt
#include "vtk_fmt.h"
#include VTK_FMT(fmt/format.h)
#include VTK_FMT(fmt/ostream.h)
#include <iostream> // for ostringstream
#include <stdlib.h>
#include <string> // for string, operator==, etc
#include <vector>

#include "Ioss_ChangeSetFactory.h" // for ChangeSetFactory
#include "Ioss_Property.h"         // for Property
#include "Ioss_PropertyManager.h"  // for PropertyManager
#include "Ioss_Region.h"           // for Region

namespace Ioex {
  // ========================================================================
  const ChangeSetFactory *ChangeSetFactory::factory()
  {
    static ChangeSetFactory registerThis;
    return &registerThis;
  }

  ChangeSetFactory::ChangeSetFactory() : Ioss::ChangeSetFactory("exodus")
  {
    Ioss::ChangeSetFactory::alias("exodus", "exodusii");
    Ioss::ChangeSetFactory::alias("exodus", "exodusII");
    Ioss::ChangeSetFactory::alias("exodus", "genesis");
#if defined(PARALLEL_AWARE_EXODUS)
    Ioss::ChangeSetFactory::alias("exodus", "dof_exodus");
    Ioss::ChangeSetFactory::alias("exodus", "dof");
#endif
  }

  Ioss::ChangeSet *ChangeSetFactory::make_ChangeSet(Ioss::Region *region) const
  {
    return new ChangeSet(region);
  }

  Ioss::ChangeSet *ChangeSetFactory::make_ChangeSet(Ioss::DatabaseIO *db, const std::string &dbName,
                                                    const std::string &dbType,
                                                    unsigned           fileCyclicCount) const
  {
    return new ChangeSet(db, dbName, dbType, fileCyclicCount);
  }

  // ========================================================================
  ChangeSet::ChangeSet(Ioss::Region *region) : Ioss::ChangeSet(region)
  {
    Ioss::ChangeSet::m_supportedFormats =
        (Ioss::CHANGE_SET_INTERNAL_FILES | Ioss::CHANGE_SET_LINEAR_MULTI_FILES |
         Ioss::CHANGE_SET_CYCLIC_MULTI_FILES);
  }

  ChangeSet::ChangeSet(Ioss::DatabaseIO *db, const std::string &dbName, const std::string &dbType,
                       unsigned fileCyclicCount)
      : Ioss::ChangeSet(db, dbName, dbType, fileCyclicCount)
  {
    Ioss::ChangeSet::m_supportedFormats =
        (Ioss::CHANGE_SET_INTERNAL_FILES | Ioss::CHANGE_SET_LINEAR_MULTI_FILES |
         Ioss::CHANGE_SET_CYCLIC_MULTI_FILES);
  }

  ChangeSet::~ChangeSet() {}

  void ChangeSet::get_group_change_sets()
  {
    auto                  iossDB = get_database();
    Ioex::BaseDatabaseIO *ioexDB = dynamic_cast<Ioex::BaseDatabaseIO *>(iossDB);

    if (nullptr == ioexDB) {
      std::ostringstream errmsg;
      fmt::print(errmsg, "ERROR: The database file named '{}' is not Exodus format\n",
                 iossDB->get_filename());
      IOSS_ERROR(errmsg);
    }

    m_databaseFormat   = Ioss::CHANGE_SET_INTERNAL_FILES;
    m_currentChangeSet = ioexDB->get_internal_change_set_name();

    Ioss::NameList names = ioexDB->groups_describe(false);

    // Downshift by 1 since the first is the root group "/"
    int numNames = static_cast<int>(names.size());
    for (int i = 0; i < numNames - 1; i++) {
      m_changeSetNames.push_back(names[i + 1]);
    }
  }

  bool ChangeSet::supports_group()
  {
    auto                  iossDB = get_database();
    Ioex::BaseDatabaseIO *ioexDB = dynamic_cast<Ioex::BaseDatabaseIO *>(iossDB);

    if (nullptr == ioexDB) {
      std::ostringstream errmsg;
      fmt::print(errmsg, "ERROR: The database file named '{}' is not Exodus format\n",
                 iossDB->get_filename());
      IOSS_ERROR(errmsg);
    }

    return ioexDB->supports_group() && (ioexDB->num_child_group() > 0);
  }

  void ChangeSet::populate_change_sets(bool loadAllFiles)
  {
    if (supports_group()) {
      get_group_change_sets();
    }
    else {
      Ioss::ChangeSet::populate_change_sets(loadAllFiles);
    }
  }

  void ChangeSet::clear_change_sets()
  {
    m_changeSetNames.clear();
    m_currentChangeSet = "";
  }

  void ChangeSet::close_change_set(unsigned index)
  {
    if (Ioss::CHANGE_SET_INTERNAL_FILES != m_databaseFormat) {
      Ioss::ChangeSet::close_change_set(index);
      return;
    }

    verify_change_set_index(index);

    auto iossDB = get_database();
    if (!iossDB->open_internal_change_set(m_currentChangeSet)) {
      std::ostringstream errmsg;
      fmt::print(errmsg, "ERROR: The database file named '{}' could not open group '{}\n",
                 iossDB->get_filename(), m_currentChangeSet);
      IOSS_ERROR(errmsg);
    }
  }

  Ioss::DatabaseIO *ChangeSet::open_change_set(unsigned index, Ioss::DatabaseUsage usage)
  {
    if (Ioss::CHANGE_SET_INTERNAL_FILES != m_databaseFormat) {
      return Ioss::ChangeSet::open_change_set(index, usage);
    }

    verify_change_set_index(index);

    auto db = get_database();
    db->open_internal_change_set(index);

    return db;
  }
} // namespace Ioex
