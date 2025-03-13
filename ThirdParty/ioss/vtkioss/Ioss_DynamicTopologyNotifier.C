// Copyright(C) 2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_DynamicTopologyNotifier.h"

#include <algorithm>
#include <assert.h>
#include <climits>
#include <cstddef>
#include <functional>

namespace Ioss {

  struct DynamicTopologyObserverCompare
  {
    bool operator()(const std::shared_ptr<DynamicTopologyObserver> &lhs,
                    const std::shared_ptr<DynamicTopologyObserver> &rhs) const
    {
      assert(lhs && (lhs->get_region() != nullptr));
      assert(rhs && (rhs->get_region() != nullptr));
      return (lhs->get_region() < rhs->get_region());
    }
  };

  void DynamicTopologyNotifier::register_observer(std::shared_ptr<DynamicTopologyObserver> observer)
  {
    observer->register_notifier(this);
    m_observers.push_back(observer);
    std::sort(m_observers.begin(), m_observers.end(), DynamicTopologyObserverCompare());
  }

  void
  DynamicTopologyNotifier::unregister_observer(std::shared_ptr<DynamicTopologyObserver> observer)
  {
    auto iter = std::find(m_observers.begin(), m_observers.end(), observer);
    if (iter != m_observers.end()) {
      (*iter)->register_notifier(nullptr);
      m_observers.erase(iter);
    }
  }

  void DynamicTopologyNotifier::reset_topology_modification()
  {
    for (std::shared_ptr<DynamicTopologyObserver> &observer : m_observers) {
      observer->reset_topology_modification();
    }
  }

  void DynamicTopologyNotifier::set_topology_modification(unsigned int type)
  {
    for (std::shared_ptr<DynamicTopologyObserver> &observer : m_observers) {
      observer->set_topology_modification(type);
    }
  }

} // namespace Ioss
