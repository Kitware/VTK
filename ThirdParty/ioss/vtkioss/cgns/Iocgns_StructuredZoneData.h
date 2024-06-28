/*
 * Copyright(C) 1999-2022, 2024 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
#pragma once

#include "Ioss_CodeTypes.h"
#include "Ioss_StructuredBlock.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "Ioss_ZoneConnectivity.h"
#include "iocgns_export.h"
#include "vtk_ioss_mangle.h"

namespace Iocgns {
  enum Ordinal {
    I = 1,
    J = 2,
    K = 4,
  };

  class IOCGNS_EXPORT StructuredZoneData
  {
  public:
    StructuredZoneData() { m_adam = this; }

    StructuredZoneData(std::string name, int zone, int ni, int nj, int nk)
        : m_name(std::move(name)), m_ordinal{{ni, nj, nk}}, m_zone(zone)
    {
      m_adam = this;
    }

    // Used for regression tests to make it easier to define...
    // Last argument is of the form "5x12x32"
    StructuredZoneData(int zone, const std::string &nixnjxnk);

    std::string m_name{};
    Ioss::IJK_t m_ordinal{{0, 0, 0}};

    // Offset of this block relative to its
    // adam block. ijk_adam = ijk_me + m_offset[ijk];
    Ioss::IJK_t m_offset{{0, 0, 0}};

    // If value is 0, 1, or 2, then do not split along that ordinal
    unsigned int m_lineOrdinal{0};

    int m_zone{0};

    // The zone in the undecomposed model that this zone is a
    // descendant of.  If not decomposed, then m_zone == m_adam;
    StructuredZoneData *m_adam{nullptr};

    // If this zone was the result of splitting another zone, then
    // what is the zone number of that zone.  Zones are kept in a
    // vector and the zone number is its position in that vector+1
    // to make it 1-based and match numbering on file.
    StructuredZoneData *m_parent{nullptr};

    int m_proc{-1}; // The processor this block might be run on...

    // Which ordinal of the parent was split to generate this zone and its sibling.
    int m_splitOrdinal{0};

    // The two zones that were split off from this zone.
    // Might be reasonable to do a 3-way or n-way split, but for now
    // just do a 2-way.
    StructuredZoneData *m_child1{nullptr};
    StructuredZoneData *m_child2{nullptr};

    StructuredZoneData *m_sibling{nullptr};

    std::vector<Ioss::ZoneConnectivity> m_zoneConnectivity{};

    // ========================================================================
    IOSS_NODISCARD bool is_active() const
    {
      // Zone is active if it hasn't been split.
      return m_child1 == nullptr && m_child2 == nullptr;
    }

    // ========================================================================
    // Assume the "work" or computational effort required for a
    // block is proportional to the number of cells.
    IOSS_NODISCARD size_t work() const
    {
      return (size_t)m_ordinal[0] * m_ordinal[1] * m_ordinal[2];
    }
    IOSS_NODISCARD size_t cell_count() const
    {
      return (size_t)m_ordinal[0] * m_ordinal[1] * m_ordinal[2];
    }

    IOSS_NODISCARD size_t node_count() const
    {
      return (size_t)(m_ordinal[0] + 1) * (m_ordinal[1] + 1) * (m_ordinal[2] + 1);
    }

    IOSS_NODISCARD std::pair<StructuredZoneData *, StructuredZoneData *>
                   split(int zone_id, double avg_work, int rank, bool verbose);
    void           resolve_zgc_split_donor(const std::vector<Iocgns::StructuredZoneData *> &zones);
    void           update_zgc_processor(const std::vector<Iocgns::StructuredZoneData *> &zones);
  };
} // namespace Iocgns
