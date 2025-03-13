// Copyright(C) 2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_DynamicTopologyBroker.h"
#include "Ioss_Region.h"

#include <algorithm>
#include <assert.h>
#include <climits>
#include <cstddef>
#include <functional>

namespace Ioss {

  DynamicTopologyBroker *DynamicTopologyBroker::broker()
  {
    static DynamicTopologyBroker broker_;
    return &broker_;
  }

  void DynamicTopologyBroker::register_model(const std::string &model_name)
  {
    auto iter = m_notifiers.find(model_name);
    if (iter != m_notifiers.end()) {
      return;
    }

    m_notifiers[model_name] = std::make_shared<DynamicTopologyNotifier>(model_name);
  }

  std::shared_ptr<DynamicTopologyNotifier>
  DynamicTopologyBroker::get_notifier(const std::string &model_name) const
  {
    auto iter = m_notifiers.find(model_name);
    if (iter != m_notifiers.end()) {
      return iter->second;
    }

    return {};
  }

  std::vector<std::shared_ptr<DynamicTopologyObserver>>
  DynamicTopologyBroker::get_observers(const std::string &model_name) const
  {
    std::vector<std::shared_ptr<DynamicTopologyObserver>> observers;

    auto notifier = get_notifier(model_name);

    if (notifier) {
      return notifier->get_observers();
    }

    return observers;
  }

  void DynamicTopologyBroker::remove_model(const std::string &model_name)
  {
    auto iter = m_notifiers.find(model_name);
    if (iter != m_notifiers.end()) {
      m_notifiers.erase(iter);
    }
  }

  void DynamicTopologyBroker::clear_models() { m_notifiers.clear(); }

  void DynamicTopologyBroker::register_observer(const std::string                       &model_name,
                                                std::shared_ptr<DynamicTopologyObserver> observer)
  {
    auto notifier = get_notifier(model_name);

    if (!notifier) {
      register_model(model_name);
      notifier = get_notifier(model_name);
    }

    notifier->register_observer(observer);
  }

  void DynamicTopologyBroker::register_observer(const std::string                       &model_name,
                                                std::shared_ptr<DynamicTopologyObserver> observer,
                                                Region                                  &region)
  {
    region.register_mesh_modification_observer(observer);
    register_observer(model_name, observer);
  }

  void DynamicTopologyBroker::reset_topology_modification(const std::string &model_name)
  {
    auto notifier = get_notifier(model_name);

    if (!notifier)
      return;

    notifier->reset_topology_modification();
  }

  void DynamicTopologyBroker::set_topology_modification(const std::string &model_name,
                                                        unsigned int       type)
  {
    auto notifier = get_notifier(model_name);

    if (!notifier)
      return;

    notifier->set_topology_modification(type);
  }

} // namespace Ioss
