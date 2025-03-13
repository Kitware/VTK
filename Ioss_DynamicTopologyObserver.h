// Copyright(C) 2024, 2025 National Technology & Engineering Solutions
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
  class DynamicTopologyNotifier;

  class IOSS_EXPORT DynamicTopologyObserver
  {
  public:
    explicit DynamicTopologyObserver(Region *region) : m_region(region) {}

    virtual ~DynamicTopologyObserver() = default;

    virtual void reset_topology_modification_all();
    virtual void reset_topology_modification();
    virtual void set_topology_modification(unsigned int type);
    virtual void sync_topology_modification(unsigned int modFlag, unsigned int cumulativeModFlag);
    IOSS_NODISCARD virtual unsigned int get_topology_modification() const;

    IOSS_NODISCARD virtual unsigned int get_cumulative_topology_modification() const;
    virtual void                        set_cumulative_topology_modification(unsigned int type);

    IOSS_NODISCARD int get_cumulative_topology_modification_field();

    IOSS_NODISCARD virtual bool is_topology_modified() const;
    IOSS_NODISCARD virtual bool is_automatic_restart() const { return m_automaticRestart; }
    IOSS_NODISCARD virtual bool is_restart_requested() const { return m_restartRequested; }

    void set_automatic_restart(bool flag) { m_automaticRestart = flag; }
    void set_restart_requested(bool flag) { m_restartRequested = flag; }

    IOSS_NODISCARD static std::string topology_modification_change_name()
    {
      return std::string("CUMULATIVE_TOPOLOGY_MODIFICATION");
    }

    void                   register_region(Region *region);
    IOSS_NODISCARD Region *get_region() const { return m_region; }

    void                                    register_notifier(DynamicTopologyNotifier *notifier);
    IOSS_NODISCARD DynamicTopologyNotifier *get_notifier() const { return m_notifier; }

    virtual void define_model();
    virtual void write_model();
    virtual void define_transient();

    IOSS_NODISCARD virtual FileControlOption get_control_option() const
    {
      return FileControlOption::CONTROL_NONE;
    }

    IOSS_NODISCARD virtual bool needs_new_output_file() const;

    virtual void initialize_region();

  protected:
    Region      *m_region{nullptr};
    unsigned int m_topologyModification{TOPOLOGY_SAME};
    unsigned int m_cumulativeTopologyModification{TOPOLOGY_SAME};

    bool m_automaticRestart{false};
    bool m_restartRequested{false};

    DynamicTopologyNotifier *m_notifier{nullptr};

    void                                verify_region_is_registered() const;
    IOSS_NODISCARD const ParallelUtils &util() const;
    void                                synchronize_topology_modified_flags();

    void set_topology_modification_nl(unsigned int type);

  private:
    DynamicTopologyObserver();
  };

} // namespace Ioss
