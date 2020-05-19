// Copyright(C) 2017 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//
//     * Neither the name of NTESS nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef IOSS_Ioss_ZoneConnectivity_h
#define IOSS_Ioss_ZoneConnectivity_h

#include "vtk_ioss_mangle.h"

#include <Ioss_CodeTypes.h>
#include <array>
#include <cassert>
#include <string>

#if defined(SEACAS_HAVE_CGNS) && !defined(BUILT_IN_SIERRA)
#include <cgnstypes.h>
using INT = cgsize_t;
#else
// If this is not being built with CGNS, then default to using 32-bit integers.
// Currently there is no way to input/output a structured mesh without CGNS,
// so this block is simply to get things to compile and probably has no use.
using INT = int;
#endif

namespace Ioss {
  class Region;

  struct ZoneConnectivity
  {
    ZoneConnectivity(const std::string name, int owner_zone, const std::string donor_name,
                     int donor_zone, const Ioss::IJK_t p_transform, const Ioss::IJK_t range_beg,
                     const Ioss::IJK_t range_end, const Ioss::IJK_t donor_beg,
                     const Ioss::IJK_t donor_end, const Ioss::IJK_t owner_offset = IJK_t(),
                     const Ioss::IJK_t donor_offset = IJK_t())
        : m_connectionName(std::move(name)), m_donorName(std::move(donor_name)),
          m_transform(std::move(p_transform)), m_ownerRangeBeg(std::move(range_beg)),
          m_ownerRangeEnd(std::move(range_end)), m_ownerOffset(std::move(owner_offset)),
          m_donorRangeBeg(std::move(donor_beg)), m_donorRangeEnd(std::move(donor_end)),
          m_donorOffset(std::move(donor_offset)), m_ownerZone(owner_zone), m_donorZone(donor_zone)
    {
      assert(is_valid());
      m_ownsSharedNodes = m_ownerZone < m_donorZone || m_donorZone == -1;
      m_isActive        = has_faces();
    }

    ZoneConnectivity(const std::string name, int owner_zone, const std::string donor_name,
                     int donor_zone, const Ioss::IJK_t p_transform, const Ioss::IJK_t range_beg,
                     const Ioss::IJK_t range_end, const Ioss::IJK_t donor_beg,
                     const Ioss::IJK_t donor_end, bool owns_nodes, bool from_decomp)
        : m_connectionName(std::move(name)), m_donorName(std::move(donor_name)),
          m_transform(std::move(p_transform)), m_ownerRangeBeg(std::move(range_beg)),
          m_ownerRangeEnd(std::move(range_end)), m_donorRangeBeg(std::move(donor_beg)),
          m_donorRangeEnd(std::move(donor_end)), m_ownerZone(owner_zone), m_donorZone(donor_zone),
          m_ownsSharedNodes(owns_nodes), m_fromDecomp(from_decomp)
    {
      // This constructor typically called from decomposition process.
      assert(is_valid());
      m_isActive = has_faces();
    }

    ZoneConnectivity(const ZoneConnectivity &copy_from) = default;

    // Return number of nodes in the connection shared with the donor zone.
    size_t get_shared_node_count() const
    {
      size_t snc = 1;
      for (int i = 0; i < 3; i++) {
        snc *= (std::abs(m_ownerRangeEnd[i] - m_ownerRangeBeg[i]) + 1);
      }
      return snc;
    }

    // Validate zgc -- if is_active(), then must have non-zero entries for all ranges.
    // transform must have valid entries.
    bool is_valid() const;
    bool has_faces() const;

    std::array<INT, 9> transform_matrix() const;
    Ioss::IJK_t        transform(const Ioss::IJK_t &index_1) const;
    Ioss::IJK_t        inverse_transform(const Ioss::IJK_t &index_1) const;

    std::vector<int>     get_range(int ordinal) const;
    friend std::ostream &operator<<(std::ostream &os, const ZoneConnectivity &zgc);

    bool is_from_decomp() const { return m_fromDecomp; }
    bool is_active() const { return m_isActive && has_faces(); }

    std::string m_connectionName{}; // Name of the connection; either generated or from file
    std::string m_donorName{}; // Name of the zone (m_donorZone) to which this zone is connected via
                               // this connection.
    Ioss::IJK_t m_transform{}; // The transform.  In the same form as defined by CGNS

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
    bool m_fromDecomp{false};

    bool m_isActive{true}; // True if non-zero range. That is, it has at least one face
  };
} // namespace Ioss
#endif
