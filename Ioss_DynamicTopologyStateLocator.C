// Copyright(C) 2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_DynamicTopologyStateLocator.h"

#include "Ioss_ChangeSet.h"
#include "Ioss_ChangeSetFactory.h"
#include "Ioss_CodeTypes.h"
#include "Ioss_DBUsage.h"
#include "Ioss_DatabaseIO.h"
#include "Ioss_ParallelUtils.h"
#include "Ioss_Region.h"

#include <assert.h>
#include <climits>
#include <cstddef>
#include <functional>
#include <sstream>

namespace Ioss {

  DynamicTopologyStateLocator::DynamicTopologyStateLocator(Region *region, bool loadAllFiles)
      : m_database(region->get_database()),
        m_ioDB(region->get_property("base_filename").get_string()),
        m_dbType(region->get_property("database_type").get_string()),
        m_fileCyclicCount(region->get_file_cyclic_count()), m_loadAllFiles(loadAllFiles)
  {
  }

  DynamicTopologyStateLocator::DynamicTopologyStateLocator(Ioss::DatabaseIO  *db,
                                                           const std::string &dbName,
                                                           const std::string &dbType,
                                                           unsigned           fileCyclicCount,
                                                           bool               loadAllFiles)
      : m_database(db), m_ioDB(dbName), m_dbType(dbType), m_fileCyclicCount(fileCyclicCount),
        m_loadAllFiles(loadAllFiles)
  {
  }

  DynamicTopologyStateLocator::DynamicTopologyStateLocator(Ioss::DatabaseIO *db,
                                                           unsigned          fileCyclicCount,
                                                           bool              loadAllFiles)
      : m_database(db),
        m_ioDB(db->get_property_manager().get_optional("base_filename", db->get_filename())),
        m_dbType(db->get_property_manager().get_optional("database_type", "")),
        m_fileCyclicCount(fileCyclicCount), m_loadAllFiles(loadAllFiles)
  {
  }

  DynamicTopologyStateLocator::~DynamicTopologyStateLocator() {}

  const ParallelUtils &DynamicTopologyStateLocator::util() const { return get_database()->util(); }

  DatabaseIO *DynamicTopologyStateLocator::get_database() const { return m_database; }

  std::tuple<std::string, int, double>
  DynamicTopologyStateLocator::locate_db_state(double targetTime) const
  {
    auto          db = get_database();
    DatabaseState loc(db);

    locate_db_state_impl(targetTime, loc);

    return std::make_tuple(loc.changeSet, loc.state, loc.time);
  }

  std::tuple<std::string, int, double> DynamicTopologyStateLocator::get_db_max_time() const
  {
    auto          db = get_database();
    DatabaseState loc(db);

    double              init_time = -std::numeric_limits<double>::max();
    StateLocatorCompare compare   = [](double a, double b) { return (a > b); };

    get_db_time_impl(init_time, compare, loc);

    return std::make_tuple(loc.changeSet, loc.state, loc.time);
  }

  std::tuple<std::string, int, double> DynamicTopologyStateLocator::get_db_min_time() const
  {
    auto          db = get_database();
    DatabaseState loc(db);

    double              init_time = std::numeric_limits<double>::max();
    StateLocatorCompare compare   = [](double a, double b) { return (a < b); };

    get_db_time_impl(init_time, compare, loc);

    return std::make_tuple(loc.changeSet, loc.state, loc.time);
  }

  void DynamicTopologyStateLocator::locate_state_impl(Ioss::DatabaseIO *db, double targetTime,
                                                      StateLocatorCompare comparator,
                                                      DatabaseState      &loc) const
  {
    std::vector<double> timesteps = db->get_db_step_times();
    size_t              stepCount = timesteps.size();

    double minTimeDiff =
        loc.state < 0 ? std::numeric_limits<double>::max() : std::fabs(loc.time - targetTime);

    for (size_t istep = 1; istep <= stepCount; istep++) {
      double stateTime    = timesteps[istep - 1];
      double stepTimeDiff = std::fabs(stateTime - targetTime);
      if (comparator(stepTimeDiff, minTimeDiff)) {
        minTimeDiff   = stepTimeDiff;
        loc.time      = stateTime;
        loc.state     = static_cast<int>(istep);
        loc.changeSet = db->supports_internal_change_set() ? db->get_internal_change_set_name()
                                                           : db->get_filename();
      }
    }
  }

  void DynamicTopologyStateLocator::locate_state(Ioss::DatabaseIO *db, double targetTime,
                                                 DatabaseState &loc) const
  {
    if (targetTime < 0.0) {
      // Round towards 0
      StateLocatorCompare compare = [](double a, double b) { return (a <= b); };
      locate_state_impl(db, targetTime, compare, loc);
    }
    else {
      // Round towards 0
      StateLocatorCompare compare = [](double a, double b) { return (a < b); };
      locate_state_impl(db, targetTime, compare, loc);
    }
  }

  void DynamicTopologyStateLocator::locate_db_state_impl(double         targetTime,
                                                         DatabaseState &loc) const
  {
    auto changeSet =
        Ioss::ChangeSetFactory::create(m_database, m_ioDB, m_dbType, m_fileCyclicCount);
    changeSet->populate_change_sets(m_loadAllFiles);

    for (size_t csIndex = 0; csIndex < changeSet->size(); csIndex++) {
      auto csdb = changeSet->open_change_set(csIndex, Ioss::QUERY_TIMESTEPS_ONLY);
      locate_state(csdb, targetTime, loc);
      changeSet->close_change_set(csIndex);
    }
  }

  void DynamicTopologyStateLocator::get_db_time_impl(double              init_time,
                                                     StateLocatorCompare comparator,
                                                     DatabaseState      &loc) const
  {
    auto changeSet =
        Ioss::ChangeSetFactory::create(m_database, m_ioDB, m_dbType, m_fileCyclicCount);
    changeSet->populate_change_sets(m_loadAllFiles);

    double best_time = init_time;

    for (size_t csIndex = 0; csIndex < changeSet->size(); csIndex++) {
      auto csdb = changeSet->open_change_set(csIndex, Ioss::QUERY_TIMESTEPS_ONLY);

      std::vector<double> timesteps = csdb->get_db_step_times();
      int                 stepCount = static_cast<int>(timesteps.size());

      for (int i = 1; i <= stepCount; i++) {
        if (comparator(timesteps[i - 1], best_time)) {
          loc.time      = timesteps[i - 1];
          loc.state     = i;
          loc.changeSet = changeSet->get_change_set_name(csIndex);
          best_time     = timesteps[i - 1];
        }
      }

      changeSet->close_change_set(csIndex);
    }
  }

} // namespace Ioss
