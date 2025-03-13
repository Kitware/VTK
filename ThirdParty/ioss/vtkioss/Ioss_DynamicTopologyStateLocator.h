// Copyright(C) 2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "Ioss_DynamicTopology.h"

#include "Ioss_CodeTypes.h"
#include "Ioss_DBUsage.h"
#include "Ioss_DatabaseIO.h"      // for DatabaseIO
#include "Ioss_ParallelUtils.h"   // for ParallelUtils
#include "Ioss_PropertyManager.h" // for PropertyManager
#include "Ioss_Utils.h"
#include "ioss_export.h"
#include "vtk_ioss_mangle.h"

#include <cstddef> // for size_t, nullptr
#include <cstdint> // for int64_t
#include <iomanip>
#include <sstream>
#include <string> // for string, operator<

namespace Ioss {
  class Region;

  class IOSS_EXPORT DynamicTopologyStateLocator
  {
  public:
    DynamicTopologyStateLocator(Region *region, bool loadAllFiles = true);
    DynamicTopologyStateLocator(Ioss::DatabaseIO *db, const std::string &dbName,
                                const std::string &dbType, unsigned fileCyclicCount = 0,
                                bool loadAllFiles = true);
    DynamicTopologyStateLocator(Ioss::DatabaseIO *db, unsigned fileCyclicCount = 0,
                                bool loadAllFiles = true);

    virtual ~DynamicTopologyStateLocator();
    DynamicTopologyStateLocator()                                    = delete;
    DynamicTopologyStateLocator(const DynamicTopologyStateLocator &) = delete;

    DatabaseIO *get_database() const;

    std::tuple<std::string, int, double> locate_db_state(double targetTime) const;
    std::tuple<std::string, int, double> get_db_min_time() const;
    std::tuple<std::string, int, double> get_db_max_time() const;

  private:
    struct DatabaseState
    {
      explicit DatabaseState(Ioss::DatabaseIO *db)
      {
        if (!db->supports_internal_change_set()) {
          changeSet = db->get_filename();
        }
      }

      std::string changeSet{"/"};
      int         state{-1};
      double      time{-std::numeric_limits<double>::max()};
    };

    using StateLocatorCompare = std::function<bool(double, double)>;

    void locate_state_impl(Ioss::DatabaseIO *db, double targetTime, StateLocatorCompare comparator,
                           DatabaseState &loc) const;

    void locate_state(Ioss::DatabaseIO *db, double targetTime, DatabaseState &loc) const;

    void locate_db_state_impl(double targetTime, DatabaseState &loc) const;

    void get_db_time_impl(double init_time, StateLocatorCompare comparator,
                          DatabaseState &loc) const;

    IOSS_NODISCARD const ParallelUtils &util() const;

    Ioss::DatabaseIO *m_database{nullptr};
    std::string       m_ioDB;
    std::string       m_dbType;
    unsigned          m_fileCyclicCount{0};
    bool              m_loadAllFiles{true};
  };

} // namespace Ioss
