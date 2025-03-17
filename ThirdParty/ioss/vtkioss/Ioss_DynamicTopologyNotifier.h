// Copyright(C) 2024, 2025 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "Ioss_DynamicTopology.h"
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

  class IOSS_EXPORT DynamicTopologyNotifier
  {
  public:
    explicit DynamicTopologyNotifier(const std::string &model_name) : m_modelName(model_name) {}

    virtual ~DynamicTopologyNotifier() = default;

    IOSS_NODISCARD std::string name() const { return m_modelName; }

    IOSS_NODISCARD const std::vector<std::shared_ptr<DynamicTopologyObserver>>                      &
    get_observers() const
    {
      return m_observers;
    }

    IOSS_NODISCARD std::vector<std::shared_ptr<DynamicTopologyObserver>> &get_observers()
    {
      return m_observers;
    }

    void register_observer(std::shared_ptr<DynamicTopologyObserver> observer);

    void unregister_observer(std::shared_ptr<DynamicTopologyObserver> observer);

    void reset_topology_modification();

    void set_topology_modification(unsigned int type);

    template <typename ObserverType> bool has_observer_type() const
    {
      bool found = false;

      for (const std::shared_ptr<DynamicTopologyObserver> &observer : m_observers) {
        if (dynamic_cast<const ObserverType *>(observer.get()) != nullptr) {
          found = true;
          break;
        }
      }
      return found;
    }

    template <typename ObserverType>
    std::vector<std::shared_ptr<ObserverType>> get_observer_type() const
    {
      std::vector<std::shared_ptr<ObserverType>> typed_observers;

      for (const std::shared_ptr<DynamicTopologyObserver> &observer : m_observers) {
        ObserverType *typed_observer = dynamic_cast<ObserverType *>(observer.get());
        if (typed_observer != nullptr) {
          typed_observers.push_back(std::dynamic_pointer_cast<ObserverType>(observer));
        }
      }

      return typed_observers;
    }

  private:
    const std::string                                     m_modelName;
    std::vector<std::shared_ptr<DynamicTopologyObserver>> m_observers;
  };

} // namespace Ioss
