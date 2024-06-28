// Copyright(C) 1999-2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_CodeTypes.h"
#include "Ioss_SmartAssert.h"
#include "cgns/Iocgns_StructuredZoneData.h"
#include <assert.h>
#include <cstdlib>
#include "vtk_fmt.h"
#include VTK_FMT(fmt/core.h)
#include VTK_FMT(fmt/format.h)
#if !defined __NVCC__
#include VTK_FMT(fmt/color.h)
#endif
#include <cmath>
#include VTK_FMT(fmt/ostream.h)
#include <string>
#include <tokenize.h>

#include "Ioss_Utils.h"

namespace {
  struct Range
  {
    Range(int a, int b) : m_beg(a < b ? a : b), m_end(a < b ? b : a), m_reversed(b < a) {}

    int  begin() const { return m_reversed ? m_end : m_beg; }
    int  end() const { return m_reversed ? m_beg : m_end; }
    int  m_beg;
    int  m_end;
    bool m_reversed;
  };

  bool overlaps(const Range &a, const Range &b) { return a.m_beg <= b.m_end && b.m_beg <= a.m_end; }

  bool zgc_overlaps(const Iocgns::StructuredZoneData *zone, const Ioss::ZoneConnectivity &zgc)
  {
    // Note that zone range is nodes and m_ordinal[] is cells, so need to add 1 to range.
    Range z_i(1 + zone->m_offset[0], zone->m_ordinal[0] + zone->m_offset[0] + 1);
    Range z_j(1 + zone->m_offset[1], zone->m_ordinal[1] + zone->m_offset[1] + 1);
    Range z_k(1 + zone->m_offset[2], zone->m_ordinal[2] + zone->m_offset[2] + 1);

    Range gc_i(zgc.m_ownerRangeBeg[0], zgc.m_ownerRangeEnd[0]);
    Range gc_j(zgc.m_ownerRangeBeg[1], zgc.m_ownerRangeEnd[1]);
    Range gc_k(zgc.m_ownerRangeBeg[2], zgc.m_ownerRangeEnd[2]);

    return overlaps(z_i, gc_i) && overlaps(z_j, gc_j) && overlaps(z_k, gc_k);
  }

  bool zgc_donor_overlaps(const Iocgns::StructuredZoneData *zone, const Ioss::ZoneConnectivity &zgc)
  {
    // Note that zone range is nodes and m_ordinal[] is cells, so need to add 1 to range.
    Range z_i(1 + zone->m_offset[0], zone->m_ordinal[0] + zone->m_offset[0] + 1);
    Range z_j(1 + zone->m_offset[1], zone->m_ordinal[1] + zone->m_offset[1] + 1);
    Range z_k(1 + zone->m_offset[2], zone->m_ordinal[2] + zone->m_offset[2] + 1);

    Range gc_i(zgc.m_donorRangeBeg[0], zgc.m_donorRangeEnd[0]);
    Range gc_j(zgc.m_donorRangeBeg[1], zgc.m_donorRangeEnd[1]);
    Range gc_k(zgc.m_donorRangeBeg[2], zgc.m_donorRangeEnd[2]);

    return overlaps(z_i, gc_i) && overlaps(z_j, gc_j) && overlaps(z_k, gc_k);
  }

  Range subset_range(const Range &a, const Range &b)
  {
    Range ret(std::max(a.m_beg, b.m_beg), std::min(a.m_end, b.m_end));
    ret.m_reversed = a.m_reversed || b.m_reversed;
    return ret;
  }

  void zgc_subset_ranges(const Iocgns::StructuredZoneData *zone, Ioss::ZoneConnectivity &zgc)
  {
    if (zgc_overlaps(zone, zgc)) {
      // NOTE: Updates the range and donor_range in zgc

      // Note that zone range is nodes and m_ordinal[] is cells, so need to add 1 to range.
      Range z_i(1 + zone->m_offset[0], zone->m_ordinal[0] + zone->m_offset[0] + 1);
      Range z_j(1 + zone->m_offset[1], zone->m_ordinal[1] + zone->m_offset[1] + 1);
      Range z_k(1 + zone->m_offset[2], zone->m_ordinal[2] + zone->m_offset[2] + 1);

      Range gc_i(zgc.m_ownerRangeBeg[0], zgc.m_ownerRangeEnd[0]);
      Range gc_j(zgc.m_ownerRangeBeg[1], zgc.m_ownerRangeEnd[1]);
      Range gc_k(zgc.m_ownerRangeBeg[2], zgc.m_ownerRangeEnd[2]);

      Range gc_ii = subset_range(z_i, gc_i);
      Range gc_jj = subset_range(z_j, gc_j);
      Range gc_kk = subset_range(z_k, gc_k);

      Ioss::IJK_t range_beg;
      Ioss::IJK_t range_end;
      range_beg[0] = gc_ii.begin();
      range_end[0] = gc_ii.end();
      range_beg[1] = gc_jj.begin();
      range_end[1] = gc_jj.end();
      range_beg[2] = gc_kk.begin();
      range_end[2] = gc_kk.end();

      if (zgc.m_sameRange) {
        zgc.m_ownerRangeBeg = range_beg;
        zgc.m_ownerRangeEnd = range_end;
        zgc.m_donorRangeBeg = zgc.m_ownerRangeBeg;
        zgc.m_donorRangeEnd = zgc.m_ownerRangeEnd;
      }
      else {
        auto d_range_beg    = zgc.transform(range_beg);
        zgc.m_donorRangeEnd = zgc.transform(range_end);
        zgc.m_donorRangeBeg = d_range_beg;
        zgc.m_ownerRangeBeg = range_beg;
        zgc.m_ownerRangeEnd = range_end;
      }
      zgc.m_ownerOffset = {{zone->m_offset[0], zone->m_offset[1], zone->m_offset[2]}};
      assert(zgc.is_valid());
      zgc.m_isActive = zgc.has_faces();
    }
    else {
      // This zgc does not overlap on this zone, so set all ranges to 0.
      // Still need it in list so can write block out correctly in parallel...
      zgc.m_ownerRangeBeg = {{0, 0, 0}};
      zgc.m_ownerRangeEnd = {{0, 0, 0}};
      zgc.m_donorRangeBeg = {{0, 0, 0}};
      zgc.m_donorRangeEnd = {{0, 0, 0}};
      zgc.m_isActive      = false;
    }
  }

  void zgc_subset_donor_ranges(const Iocgns::StructuredZoneData *don_zone,
                               Ioss::ZoneConnectivity           &zgc)
  {
    // NOTE: Updates the range and donor_range in zgc

    // Note that zone range is nodes and m_ordinal[] is cells, so need to add 1 to range.
    Range z_i(1 + don_zone->m_offset[0], don_zone->m_ordinal[0] + don_zone->m_offset[0] + 1);
    Range z_j(1 + don_zone->m_offset[1], don_zone->m_ordinal[1] + don_zone->m_offset[1] + 1);
    Range z_k(1 + don_zone->m_offset[2], don_zone->m_ordinal[2] + don_zone->m_offset[2] + 1);

    Range gc_i(zgc.m_donorRangeBeg[0], zgc.m_donorRangeEnd[0]);
    Range gc_j(zgc.m_donorRangeBeg[1], zgc.m_donorRangeEnd[1]);
    Range gc_k(zgc.m_donorRangeBeg[2], zgc.m_donorRangeEnd[2]);

    Range gc_ii = subset_range(z_i, gc_i);
    Range gc_jj = subset_range(z_j, gc_j);
    Range gc_kk = subset_range(z_k, gc_k);

    Ioss::IJK_t d_range_beg;
    Ioss::IJK_t d_range_end;
    d_range_beg[0] = gc_ii.begin();
    d_range_end[0] = gc_ii.end();
    d_range_beg[1] = gc_jj.begin();
    d_range_end[1] = gc_jj.end();
    d_range_beg[2] = gc_kk.begin();
    d_range_end[2] = gc_kk.end();

    if (zgc.m_sameRange) {
      zgc.m_donorRangeBeg = d_range_beg;
      zgc.m_donorRangeEnd = d_range_end;
      zgc.m_ownerRangeEnd = zgc.m_donorRangeEnd;
      zgc.m_ownerRangeBeg = zgc.m_donorRangeBeg;
    }
    else {
      auto range_beg      = zgc.inverse_transform(d_range_beg);
      zgc.m_ownerRangeEnd = zgc.inverse_transform(d_range_end);
      zgc.m_ownerRangeBeg = range_beg;
      zgc.m_donorRangeBeg = d_range_beg;
      zgc.m_donorRangeEnd = d_range_end;
    }
    zgc.m_donorOffset = {{don_zone->m_offset[0], don_zone->m_offset[1], don_zone->m_offset[2]}};
    assert(zgc.is_valid());
  }

  void propogate_zgc(Iocgns::StructuredZoneData *parent, Iocgns::StructuredZoneData *child)
  {
    for (auto zgc : parent->m_zoneConnectivity) {
      if (!zgc.is_from_decomp() || zgc_overlaps(child, zgc)) {
        // Modify source and donor range to subset it to new block ranges.
        zgc_subset_ranges(child, zgc);
        zgc.m_ownerZone = child->m_zone;
        child->m_zoneConnectivity.push_back(zgc);
      }
    }
  }

  // Add the zgc corresponding to the new communication path between
  // two child zones arising from a parent split along ordinal 'ordinal'
  void add_proc_split_zgc(Iocgns::StructuredZoneData *parent, Iocgns::StructuredZoneData *c1,
                          Iocgns::StructuredZoneData *c2, int ordinal)
  {
    Ioss::IJK_t transform{{1, 2, 3}};

    // Note that range is specified in terms of 'adam' block i,j,k
    // space which is converted to local block i,j,k space
    // via the m_offset[] field on the local block.
    Ioss::IJK_t range_beg{{1 + c1->m_offset[0], 1 + c1->m_offset[1], 1 + c1->m_offset[2]}};
    Ioss::IJK_t range_end{{c1->m_ordinal[0] + c1->m_offset[0] + 1,
                           c1->m_ordinal[1] + c1->m_offset[1] + 1,
                           c1->m_ordinal[2] + c1->m_offset[2] + 1}};

    Ioss::IJK_t donor_range_beg(range_beg);
    Ioss::IJK_t donor_range_end(range_end);

    donor_range_end[ordinal] = donor_range_beg[ordinal] = range_beg[ordinal] = range_end[ordinal];

    auto c1_base = std::to_string(c1->m_adam->m_zone) + "_" + std::to_string(c1->m_zone);
    auto c2_base = std::to_string(c2->m_adam->m_zone) + "_" + std::to_string(c2->m_zone);

    const auto &adam_name = parent->m_adam->m_name;

    SMART_ASSERT(c1->m_adam->m_zone == c2->m_adam->m_zone)(c1->m_adam->m_zone)(c2->m_adam->m_zone);

    c1->m_zoneConnectivity.emplace_back(c1_base + "--" + c2_base, c1->m_zone, adam_name, c2->m_zone,
                                        transform, range_beg, range_end, donor_range_beg,
                                        donor_range_end, true, true);
    auto &zgc1         = c1->m_zoneConnectivity.back();
    zgc1.m_sameRange   = true;
    zgc1.m_ownerOffset = {{c1->m_offset[0], c1->m_offset[1], c1->m_offset[2]}};
    zgc1.m_donorOffset = {{c2->m_offset[0], c2->m_offset[1], c2->m_offset[2]}};

    c2->m_zoneConnectivity.emplace_back(c2_base + "--" + c1_base, c2->m_zone, adam_name, c1->m_zone,
                                        transform, donor_range_beg, donor_range_end, range_beg,
                                        range_end, false, true);
    auto &zgc2         = c2->m_zoneConnectivity.back();
    zgc2.m_sameRange   = true;
    zgc2.m_ownerOffset = {{c2->m_offset[0], c2->m_offset[1], c2->m_offset[2]}};
    zgc2.m_donorOffset = {{c1->m_offset[0], c1->m_offset[1], c1->m_offset[2]}};
  }
} // namespace

namespace Iocgns {

  StructuredZoneData::StructuredZoneData(int zone, const std::string &nixnjxnk) : m_zone(zone)
  {
    m_name = "zone_" + std::to_string(zone);

    auto ordinals = Ioss::tokenize(nixnjxnk, "x");
    SMART_ASSERT(ordinals.size() == 3)(ordinals.size());

    m_ordinal[0] = std::stoi(ordinals[0]);
    m_ordinal[1] = std::stoi(ordinals[1]);
    m_ordinal[2] = std::stoi(ordinals[2]);

    m_adam = this;
  }

  // ========================================================================
  // Split this StructuredZone along the largest ordinal
  // into two children and return the created zones.
  std::pair<StructuredZoneData *, StructuredZoneData *>
  StructuredZoneData::split(int zone_id, double avg_work, int rank, bool verbose)
  {
    assert(is_active());
    double ratio = avg_work / work();
    if (ratio > 1.0) {
      ratio = 1.0 / ratio;
    }

    auto ord0 = llround((double)m_ordinal[0] * ratio);
    auto ord1 = llround((double)m_ordinal[1] * ratio);
    auto ord2 = llround((double)m_ordinal[2] * ratio);

    size_t work0 = ord0 * m_ordinal[1] * m_ordinal[2];
    size_t work1 = ord1 * m_ordinal[0] * m_ordinal[2];
    size_t work2 = ord2 * m_ordinal[0] * m_ordinal[1];

    // Don't decompose along m_lineOrdinal direction and Avoid decompositions 1-cell thick.
    if (m_lineOrdinal & Ordinal::I || m_ordinal[0] == 1 || ord0 == 1 || m_ordinal[0] - ord0 == 1) {
      work0 = 0;
    }
    if (m_lineOrdinal & Ordinal::J || m_ordinal[1] == 1 || ord1 == 1 || m_ordinal[1] - ord1 == 1) {
      work1 = 0;
    }
    if (m_lineOrdinal & Ordinal::K || m_ordinal[2] == 1 || ord2 == 1 || m_ordinal[2] - ord2 == 1) {
      work2 = 0;
    }

    bool enforce_1cell_constraint = true;
    if (work0 == 0 && work1 == 0 && work2 == 0) {
      // Need to relax the "cells > 1" constraint...
      work0 = ord0 * m_ordinal[1] * m_ordinal[2];
      work1 = ord1 * m_ordinal[0] * m_ordinal[2];
      work2 = ord2 * m_ordinal[0] * m_ordinal[1];
      if (m_lineOrdinal & Ordinal::I || m_ordinal[0] == 1) {
        work0 = 0;
      }
      if (m_lineOrdinal & Ordinal::J || m_ordinal[1] == 1) {
        work1 = 0;
      }
      if (m_lineOrdinal & Ordinal::K || m_ordinal[2] == 1) {
        work2 = 0;
      }
      enforce_1cell_constraint = false;
    }

    auto delta0 = std::make_pair(std::abs((double)work0 - avg_work), -m_ordinal[0]);
    auto delta1 = std::make_pair(std::abs((double)work1 - avg_work), -m_ordinal[1]);
    auto delta2 = std::make_pair(std::abs((double)work2 - avg_work), -m_ordinal[2]);

    auto min_ordinal = 0;
    auto min_delta   = delta0;
    if (delta1 < min_delta) {
      min_ordinal = 1;
      min_delta   = delta1;
    }
    if (delta2 < min_delta) {
      min_ordinal = 2;
    }

    unsigned int ordinal = min_ordinal;

    // One more check to try to produce more "squarish" decompositions.
    // Check the ratio of max ordinal to selected min_ordinal and if > 1.5 (heuristic), choose the
    // max ordinal instead.
    int max_ordinal    = -1;
    int max_ordinal_sz = 0;
    for (int i = 0; i < 3; i++) {
      unsigned ord = i == 0 ? 1 : 2 * i;
      if (!(m_lineOrdinal & ord) && m_ordinal[i] > max_ordinal_sz) {
        max_ordinal    = i;
        max_ordinal_sz = m_ordinal[i];
      }
    }

    if (max_ordinal != -1 && (double)max_ordinal_sz / m_ordinal[ordinal] > 1.5) {
      ordinal = max_ordinal;
    }

    if ((m_ordinal[ordinal] <= (enforce_1cell_constraint ? 1 : 0)) ||
        (work0 == 0 && work1 == 0 && work2 == 0)) {
      return std::make_pair(nullptr, nullptr);
    }

    //    SMART_ASSERT(!(ordinal & m_lineOrdinal))(ordinal)(m_lineOrdinal);

    m_child1 = new StructuredZoneData;
    m_child2 = new StructuredZoneData;

    m_child1->m_name             = m_name + "_c1";
    m_child1->m_ordinal          = m_ordinal;
    m_child1->m_ordinal[ordinal] = llround(m_ordinal[ordinal] * ratio);
    if (m_child1->m_ordinal[ordinal] == 0) {
      m_child1->m_ordinal[ordinal] = 1;
    }
    SMART_ASSERT(!enforce_1cell_constraint || m_child1->m_ordinal[ordinal] != 1)
    (!enforce_1cell_constraint)(m_child1->m_ordinal[ordinal] != 1);

    m_child1->m_offset = m_offset; // Child1 offsets the same as parent;

    m_child1->m_lineOrdinal  = m_lineOrdinal;
    m_child1->m_zone         = zone_id++;
    m_child1->m_adam         = m_adam;
    m_child1->m_parent       = this;
    m_child1->m_splitOrdinal = ordinal;
    m_child1->m_sibling      = m_child2;

    m_child2->m_name             = m_name + "_c2";
    m_child2->m_ordinal          = m_ordinal;
    m_child2->m_ordinal[ordinal] = m_ordinal[ordinal] - m_child1->m_ordinal[ordinal];
    assert(m_child2->m_ordinal[ordinal] > 0);
    assert(!enforce_1cell_constraint || m_child2->m_ordinal[ordinal] != 1);
    m_child2->m_offset = m_offset;
    m_child2->m_offset[ordinal] += m_child1->m_ordinal[ordinal];

    m_child2->m_lineOrdinal  = m_lineOrdinal;
    m_child2->m_zone         = zone_id++;
    m_child2->m_adam         = m_adam;
    m_child2->m_parent       = this;
    m_child2->m_splitOrdinal = ordinal;
    m_child2->m_sibling      = m_child1;

    if (rank == 0 && verbose) {
      fmt::print(
          Ioss::DebugOut(), "{}",
          fmt::format(
#if !defined __NVCC__
              fg(fmt::color::cyan),
#endif
              "\nSplit Zone {} ({}) Adam {} ({}) with intervals {:>12},\twork = {:12}, offset {} "
              "{} {}, ordinal {}, ratio {:.3f}\n",
              m_name, m_zone, m_adam->m_name, m_adam->m_zone,
              fmt::format("{} {} {}", m_ordinal[0], m_ordinal[1], m_ordinal[2]),
              fmt::group_digits(work()), m_offset[0], m_offset[1], m_offset[2], ordinal, ratio));

      fmt::print(Ioss::DebugOut(),
                 "\tChild 1: Zone {} ({}) with intervals {:>12},\twork = {:12}, offset "
                 "{} {} {}\n"
                 "\tChild 2: Zone {} ({}) with intervals {:>12},\twork = {:12}, offset "
                 "{} {} {}\n",
                 m_child1->m_name, m_child1->m_zone,
                 fmt::format("{} {} {}", m_child1->m_ordinal[0], m_child1->m_ordinal[1],
                             m_child1->m_ordinal[2]),
                 fmt::group_digits(m_child1->work()), m_child1->m_offset[0], m_child1->m_offset[1],
                 m_child1->m_offset[2], m_child2->m_name, m_child2->m_zone,
                 fmt::format("{} {} {}", m_child2->m_ordinal[0], m_child2->m_ordinal[1],
                             m_child2->m_ordinal[2]),
                 fmt::group_digits(m_child2->work()), m_child2->m_offset[0], m_child2->m_offset[1],
                 m_child2->m_offset[2]);
    }

    // Add ZoneGridConnectivity instance to account for split...
    add_proc_split_zgc(this, m_child1, m_child2, ordinal);

    // Propagate parent ZoneGridConnectivities to appropriate children.
    // Split if needed...
    propogate_zgc(this, m_child1);
    propogate_zgc(this, m_child2);

    return std::make_pair(m_child1, m_child2);
  }

  namespace {
    void update_zgc(Ioss::ZoneConnectivity &zgc, Iocgns::StructuredZoneData *child,
                    std::vector<Ioss::ZoneConnectivity> &zgc_vec, bool new_zgc)
    {
      zgc.m_donorZone = child->m_zone;
      zgc_subset_donor_ranges(child, zgc);
      // If `!new_zgc`, then the zgc is already in `zgc_vec`
      if (new_zgc) {
        zgc_vec.push_back(zgc);
      }
    }
  } // namespace

  // If a zgc points to a donor zone which was split (has non-null children),
  // then create two zgc that point to each child.  Update range and donor_range
  void StructuredZoneData::resolve_zgc_split_donor(
      const std::vector<Iocgns::StructuredZoneData *> &zones)
  {
    // Updates m_zoneConnectivity in place, but in case a new zgc is created,
    // need a place to store it to avoid invalidating any iterators...
    // Guess at size to avoid as many reallocations as possible.
    // At most 1 new zgc per split...
    std::vector<Ioss::ZoneConnectivity> new_zgc;
    new_zgc.reserve(m_zoneConnectivity.size());

    bool did_split = false;
    do {
      did_split = false;

      for (auto &zgc : m_zoneConnectivity) {
        auto &donor_zone = zones[zgc.m_donorZone - 1];
        if (!donor_zone->is_active()) {
          did_split = true;

          bool overlap_1 = zgc_donor_overlaps(donor_zone->m_child1, zgc);
          bool overlap_2 = zgc_donor_overlaps(donor_zone->m_child2, zgc);
          bool overlap   = overlap_1 || overlap_2;

          // Child 1
          if (overlap_1) {
            if (!overlap_2) {
              // Use `zgc` since don't need it anymore...
              update_zgc(zgc, donor_zone->m_child1, new_zgc, false);
            }
            else {
              auto c1_zgc(zgc);
              update_zgc(c1_zgc, donor_zone->m_child1, new_zgc, true);
            }
          }

          // Child 2
          if (overlap_2) {
            // Use `zgc` since don't need it anymore...
            update_zgc(zgc, donor_zone->m_child2, new_zgc, false);
          }

          if (!overlap) {
            // Need to add at least one copy of this zgc even if no overlap
            // so can maintain the original (un parallel decomposed) ranges
            // for use in output...
            zgc.m_donorZone     = donor_zone->m_child1->m_zone;
            zgc.m_ownerRangeBeg = zgc.m_ownerRangeEnd = {{0, 0, 0}};
            zgc.m_donorRangeBeg = zgc.m_donorRangeEnd = {{0, 0, 0}};
          }
        }
      }
      if (did_split) {
        if (!new_zgc.empty()) {
          m_zoneConnectivity.insert(m_zoneConnectivity.end(), new_zgc.begin(), new_zgc.end());
          new_zgc.clear();
        }
        // Filter out all zgc that do not contain any faces unless needed to maintain original zgc
        // reconstruction...
        m_zoneConnectivity.erase(
            std::remove_if(m_zoneConnectivity.begin(), m_zoneConnectivity.end(),
                           [](Ioss::ZoneConnectivity &zgc) {
                             return zgc.get_shared_node_count() <= 2 && !zgc.retain_original();
                           }),
            m_zoneConnectivity.end());
      }
    } while (did_split);
  }

  void
  StructuredZoneData::update_zgc_processor(const std::vector<Iocgns::StructuredZoneData *> &zones)
  {
    for (auto &zgc : m_zoneConnectivity) {
      const auto &donor_zone = zones[zgc.m_donorZone - 1];
      assert(donor_zone->m_proc >= 0);
      zgc.m_donorProcessor   = donor_zone->m_proc;
      const auto &owner_zone = zones[zgc.m_ownerZone - 1];
      assert(owner_zone->m_proc >= 0);
      zgc.m_ownerProcessor = owner_zone->m_proc;
    }
  }
} // namespace Iocgns
