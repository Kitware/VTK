// Copyright(C) 1999-2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "Ioss_CodeTypes.h"
#include <array>
#include <cassert>
#include <cmath>
#include <iosfwd>
#include <stdlib.h>
#include <string>
#include <vector>

#include "ioss_export.h"
#include "vtk_ioss_mangle.h"

namespace Ioss {
  class Region;

  struct IOSS_EXPORT ZoneConnectivity
  {
    // cereal requires a default constructor when de-serializing vectors of objects.  Because
    // StructuredBlock contains a vector of ZoneConnectivity objects, this default constructor is
    // necessary.
    ZoneConnectivity() = default;

    ZoneConnectivity(std::string name, int owner_zone, std::string donor_name, int donor_zone,
                     const Ioss::IJK_t p_transform, const Ioss::IJK_t range_beg,
                     const Ioss::IJK_t range_end, const Ioss::IJK_t donor_beg,
                     const Ioss::IJK_t donor_end, const Ioss::IJK_t owner_offset = IJK_t(),
                     const Ioss::IJK_t donor_offset = IJK_t())
        : m_connectionName(std::move(name)), m_donorName(std::move(donor_name)),
          m_transform(p_transform), m_ownerRangeBeg(range_beg), m_ownerRangeEnd(range_end),
          m_ownerOffset(owner_offset), m_donorRangeBeg(donor_beg), m_donorRangeEnd(donor_end),
          m_donorOffset(donor_offset), m_ownerZone(owner_zone), m_donorZone(donor_zone)
    {
      assert(is_valid());
      m_ownsSharedNodes = m_ownerZone < m_donorZone || m_donorZone == -1;
      m_isActive        = has_faces();
    }

    ZoneConnectivity(std::string name, int owner_zone, std::string donor_name, int donor_zone,
                     Ioss::IJK_t p_transform, Ioss::IJK_t range_beg, Ioss::IJK_t range_end,
                     Ioss::IJK_t donor_beg, Ioss::IJK_t donor_end, bool owns_nodes,
                     bool from_decomp)
        : m_connectionName(std::move(name)), m_donorName(std::move(donor_name)),
          m_transform(p_transform), m_ownerRangeBeg(range_beg), m_ownerRangeEnd(range_end),
          m_donorRangeBeg(donor_beg), m_donorRangeEnd(donor_end), m_ownerZone(owner_zone),
          m_donorZone(donor_zone), m_ownsSharedNodes(owns_nodes), m_fromDecomp(from_decomp)
    {
      // This constructor typically called from decomposition process.
      assert(is_valid());
      m_isActive = has_faces();
    }

    ZoneConnectivity(const ZoneConnectivity &copy_from)            = default;
    ZoneConnectivity &operator=(const ZoneConnectivity &copy_from) = default;

    // Return number of nodes in the connection shared with the donor zone.
    IOSS_NODISCARD size_t get_shared_node_count() const
    {
      size_t snc = 1;
      for (int i = 0; i < 3; i++) {
        snc *= (std::abs(m_ownerRangeEnd[i] - m_ownerRangeBeg[i]) + 1);
      }
      return snc;
    }

    // Validate zgc -- if is_active(), then must have non-zero entries for all ranges.
    // transform must have valid entries.
    IOSS_NODISCARD bool is_valid() const;
    IOSS_NODISCARD bool has_faces() const;
    IOSS_NODISCARD bool retain_original() const; // True if need to retain in parallel decomp

    IOSS_NODISCARD std::array<int, 9> transform_matrix() const;
    IOSS_NODISCARD Ioss::IJK_t transform(const Ioss::IJK_t &index_1) const;
    IOSS_NODISCARD Ioss::IJK_t inverse_transform(const Ioss::IJK_t &index_1) const;

    IOSS_NODISCARD std::vector<int> get_range(int ordinal) const;

    /* COMPARE two ZoneConnectivity objects  */
    IOSS_NODISCARD bool operator==(const Ioss::ZoneConnectivity &rhs) const;
    IOSS_NODISCARD bool operator!=(const Ioss::ZoneConnectivity &rhs) const;
    IOSS_NODISCARD bool equal(const Ioss::ZoneConnectivity &rhs) const;

    IOSS_NODISCARD bool is_from_decomp() const { return m_fromDecomp; }
    IOSS_NODISCARD bool is_active() const { return m_isActive && has_faces(); }

    std::string m_connectionName{}; // Name of the connection; either generated or from file
    std::string m_donorName{}; // Name of the zone (m_donorZone) to which this zone is connected via
                               // this connection.
    Ioss::IJK_t m_transform{}; // The transform.  In the same form as defined by CGNS

    template <class Archive> void serialize(Archive &archive)
    {
      archive(m_connectionName, m_donorName, m_transform, m_ownerRangeBeg, m_ownerRangeEnd,
              m_ownerOffset, m_donorRangeBeg, m_donorRangeEnd, m_donorOffset, m_ownerGUID,
              m_donorGUID, m_ownerZone, m_donorZone, m_ownerProcessor, m_donorProcessor,
              m_sameRange, m_ownsSharedNodes, m_fromDecomp, m_isActive);
    }

    // The following are all subsetted down to the portion that is actually on this zone
    // This can be different than m_ownerRange and m_donorRange in a parallel run if the
    // decomposition splits the connection.  In a serial run, they are the same.
    //
    // 1 of ijk should be the same for rangeBeg and rangeEnd defining a surface.
    Ioss::IJK_t m_ownerRangeBeg{}; // ijk triplet defining beginning of range on this zone
    Ioss::IJK_t m_ownerRangeEnd{}; // ijk triplet defining end of range on this zone
    Ioss::IJK_t m_ownerOffset{};   // ijk triplet with offset of this zone.  Used to convert
                                   // rangeBeg and rangeEnd global indices to local indices
    Ioss::IJK_t m_donorRangeBeg{}; // ijk triplet defining beginning of range on the connected zone
    Ioss::IJK_t m_donorRangeEnd{}; // ijk triplet defining end of range on the connected zone
    Ioss::IJK_t m_donorOffset{};   // ijk triplet with offset of the donor zone.  Used to convert
                                   // donorRangeBeg and End global indices to local indices

    size_t m_ownerGUID{}; // globally-unique id of owner
    size_t m_donorGUID{}; // globally-unique id of donor

    // NOTE: Shared nodes are "owned" by the zone with the lowest zone id.
    int  m_ownerZone{};        // "id" of zone that owns this connection
    int  m_donorZone{};        // "id" of zone that is donor (or other side) of this connection
    int  m_ownerProcessor{-1}; // processor that owns the owner zone
    int  m_donorProcessor{-1}; // processor that owns the donor zone
    bool m_sameRange{false};   // True if owner and donor range should always match...(special use
                               // during decomp)
    // True if it is the "lower" zone id in the connection. Uses adam unless both have same adam.
    bool m_ownsSharedNodes{false}; // Deprecate soon

    // True if this zc is created due to processor decompositions in a parallel run
    mutable bool m_fromDecomp{false};

    bool m_isActive{true}; // True if non-zero range. That is, it has at least one face

  private:
    bool equal_(const Ioss::ZoneConnectivity &rhs, bool quiet) const;
  };

  IOSS_EXPORT std::ostream &operator<<(std::ostream &os, const ZoneConnectivity &zgc);
} // namespace Ioss
