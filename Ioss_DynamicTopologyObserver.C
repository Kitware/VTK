// Copyright(C) 2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_ChangeSetFactory.h"
#include "Ioss_DBUsage.h"
#include "Ioss_DatabaseIO.h"
#include "Ioss_DynamicTopology.h"
#include "Ioss_DynamicTopologyNotifier.h"
#include "Ioss_DynamicTopologyObserver.h"
#include "Ioss_FileInfo.h"
#include "Ioss_IOFactory.h"
#include "Ioss_ParallelUtils.h"
#include "Ioss_Region.h"

#include "vtk_fmt.h"
#include VTK_FMT(fmt/core.h)
#include VTK_FMT(fmt/format.h)
#include VTK_FMT(fmt/ostream.h)

#include <assert.h>
#include <climits>
#include <cstddef>
#include <functional>
#include <sstream>

namespace Ioss {

  bool DynamicTopologyObserver::needs_new_output_file() const
  {
    // See what type of topology modification has occurred.  If a
    // simple REORDER, then we don't need a new file, just have to
    // inform database of new order; otherwise, need a new file (for
    // ExodusII). Baseline implementation

    if ((get_topology_modification() & Ioss::TOPOLOGY_HADAPT) ||
        (get_topology_modification() & Ioss::TOPOLOGY_CREATEFACE) ||
        (get_topology_modification() & Ioss::TOPOLOGY_CREATEELEM) ||
        (get_topology_modification() & Ioss::TOPOLOGY_CREATENODE) ||
        (get_topology_modification() & Ioss::TOPOLOGY_UNKNOWN) ||
        (get_topology_modification() & Ioss::TOPOLOGY_SHUFFLE)) {
      // See if database has been written to since being opened...
      if (get_cumulative_topology_modification() != 0) {
        return true;
      }
    }
    return false;
  }

  void DynamicTopologyObserver::verify_region_is_registered() const
  {
    if (nullptr == m_region) {
      std::ostringstream errmsg;
      fmt::print(errmsg, "ERROR: A region has not been registered with the "
                         "Dynamic Topology Observer.\n\n");
      IOSS_ERROR(errmsg);
    }
  }

  void DynamicTopologyObserver::register_region(Region *region)
  {
    if (nullptr != region && nullptr != m_region && region != m_region) {
      std::ostringstream errmsg;
      fmt::print(errmsg, "ERROR: Attempt to re-register different region on "
                         "Dynamic Topology Observer.\n\n");
      IOSS_ERROR(errmsg);
    }

    m_region = region;
  }

  void DynamicTopologyObserver::register_notifier(DynamicTopologyNotifier *notifier)
  {
    if (nullptr != notifier && nullptr != m_notifier && notifier != m_notifier) {
      std::ostringstream errmsg;
      fmt::print(errmsg, "ERROR: Attempt to re-register different notifier on "
                         "Dynamic Topology Observer.\n\n");
      IOSS_ERROR(errmsg);
    }

    m_notifier = notifier;
  }

  void DynamicTopologyObserver::set_cumulative_topology_modification(unsigned int type)
  {
    m_cumulativeTopologyModification = type;
  }

  unsigned int DynamicTopologyObserver::get_cumulative_topology_modification() const
  {
    return m_cumulativeTopologyModification;
  }

  unsigned int DynamicTopologyObserver::get_topology_modification() const
  {
    return m_topologyModification;
  }

  void DynamicTopologyObserver::set_topology_modification_nl(unsigned int type)
  {
    m_topologyModification |= type;
    m_cumulativeTopologyModification |= type;
  }

  void DynamicTopologyObserver::sync_topology_modification(unsigned int modFlag,
                                                           unsigned int cumulativeModFlag)
  {
    m_topologyModification           = modFlag;
    m_cumulativeTopologyModification = cumulativeModFlag;
  }

  void DynamicTopologyObserver::set_topology_modification(unsigned int type)
  {
    if (!(m_topologyModification & type)) {
      set_topology_modification_nl(type);

      if (nullptr != m_notifier) {
        for (auto &observer : m_notifier->get_observers()) {
          observer->set_topology_modification_nl(type);
        }
      }
    }
  }

  void DynamicTopologyObserver::reset_topology_modification()
  {
    m_topologyModification = TOPOLOGY_SAME;
  }

  void DynamicTopologyObserver::reset_topology_modification_all()
  {
    if (m_topologyModification != TOPOLOGY_SAME) {
      reset_topology_modification();

      if (nullptr != m_notifier) {
        for (auto &observer : m_notifier->get_observers()) {
          observer->reset_topology_modification();
        }
      }
    }
  }

  bool DynamicTopologyObserver::is_topology_modified() const
  {
    return m_topologyModification != TOPOLOGY_SAME;
  }

  const ParallelUtils &DynamicTopologyObserver::util() const
  {
    verify_region_is_registered();
    return m_region->get_database()->util();
  }

  void DynamicTopologyObserver::synchronize_topology_modified_flags()
  {
    verify_region_is_registered();
    int num_processors = m_region->get_database()->parallel_size();
    // Synchronize the topology flags between all processors in case
    // it has not been set consistently.
    if (num_processors > 1) {
      static unsigned int buffer[2];
      buffer[0] = m_cumulativeTopologyModification;
      buffer[1] = m_topologyModification;

      util().attribute_reduction(2 * sizeof(unsigned int), reinterpret_cast<char *>(buffer));

      m_cumulativeTopologyModification = buffer[0];
      m_topologyModification           = buffer[1];
    }
  }

  int DynamicTopologyObserver::get_cumulative_topology_modification_field()
  {
    verify_region_is_registered();
    const std::string variable_name = topology_modification_change_name();

    int ivalue = 0;

    if (m_region->field_exists(variable_name)) {
      Field topo_field = m_region->get_field(variable_name);
      if (topo_field.get_type() == Field::INTEGER) {
        m_region->get_field_data(variable_name, &ivalue, sizeof(int));
      }
      else {
        double value;
        m_region->get_field_data(variable_name, &value, sizeof(double));
        ivalue = (int)value;
      }
    }

    int num_processors = m_region->get_database()->parallel_size();
    // Synchronize the value between all processors in case
    // it has not been set consistently.
    if (num_processors > 1) {
      unsigned int buffer[1];
      buffer[0] = ivalue;

      util().attribute_reduction(sizeof(unsigned int), reinterpret_cast<char *>(buffer));

      ivalue = (int)buffer[0];
    }

    m_cumulativeTopologyModification = ivalue;

    return ivalue;
  }

  void DynamicTopologyObserver::define_model() {}

  void DynamicTopologyObserver::write_model() {}

  void DynamicTopologyObserver::define_transient() {}

  void DynamicTopologyObserver::initialize_region()
  {
    if (nullptr != m_region) {
      m_region->reset_region();
    }
  }

} // namespace Ioss
