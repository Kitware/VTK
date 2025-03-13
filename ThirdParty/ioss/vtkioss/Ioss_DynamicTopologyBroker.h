// Copyright(C) 2024, 2025 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "Ioss_DynamicTopology.h"
#include "Ioss_DynamicTopologyNotifier.h"
#include "Ioss_DynamicTopologyObserver.h"

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

  class IOSS_EXPORT DynamicTopologyBroker
  {
  public:
    static DynamicTopologyBroker *broker();

    void register_model(const std::string &model_name);
    void remove_model(const std::string &model_name);
    void clear_models();

    IOSS_NODISCARD std::shared_ptr<DynamicTopologyNotifier>
                   get_notifier(const std::string &model_name) const;
    IOSS_NODISCARD std::vector<std::shared_ptr<DynamicTopologyObserver>>
                   get_observers(const std::string &model_name) const;

    void register_observer(const std::string                       &model_name,
                           std::shared_ptr<DynamicTopologyObserver> observer);
    void register_observer(const std::string                       &model_name,
                           std::shared_ptr<DynamicTopologyObserver> observer, Region &region);

    void reset_topology_modification(const std::string &model_name);
    void set_topology_modification(const std::string &model_name, unsigned int type);

  private:
    DynamicTopologyBroker()                        = default;
    DynamicTopologyBroker(DynamicTopologyBroker &) = delete;

    std::map<std::string, std::shared_ptr<DynamicTopologyNotifier>> m_notifiers;
  };

} // namespace Ioss
