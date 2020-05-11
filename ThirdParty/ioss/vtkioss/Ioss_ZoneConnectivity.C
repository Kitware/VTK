// Copyright(C) 1999-2017 National Technology & Engineering Solutions
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

#include <Ioss_ZoneConnectivity.h>
#include <cstddef> // for size_t
#include <fmt/ostream.h>
#include <string> // for string
#include <vector> // for vector

namespace {
  int sign(int value) { return value < 0 ? -1 : 1; }

  int del(int v1, int v2) { return static_cast<int>(std::abs(v1) == std::abs(v2)); }

  bool valid_range(int beg, int end, int offset)
  {
    return ((beg - offset > 0) && (end - offset > 0));
  }
} // namespace

namespace Ioss {
  std::ostream &operator<<(std::ostream &os, const ZoneConnectivity &zgc)
  {
#if 0
    std::array<std::string, 7> tf{{"-k", "-j", "-i", " ", "i", "j", "k"}};

    // 0 -3 -k
    // 1 -2 -j
    // 2 -1 -i
    // 3
    // 4  1  i
    // 5  2  j
    // 6  3  k
    std::string transform = "[i..";
    transform += tf[zgc.m_transform[0] + 3];
    transform += " j..";
    transform += tf[zgc.m_transform[1] + 3];
    transform += " k..";
    transform += tf[zgc.m_transform[2] + 3];
    transform += "] ";
#endif

#if 1
    fmt::print(os,
               "\t\t{}[P{}]:\tDZ {}\tName '{}' shares {:n} nodes."
               "\n\t\t\t\t      Range: [{}..{}, {}..{}, {}..{}]\t      Donor Range: [{}..{}, "
               "{}..{}, {}..{}]"
               "\n\t\t\t\tLocal Range: [{}..{}, {}..{}, {}..{}]\tDonor Local Range: [{}..{}, "
               "{}..{}, {}..{}]",
               zgc.m_donorName, zgc.m_donorProcessor, zgc.m_donorZone, zgc.m_connectionName,
               zgc.get_shared_node_count(), zgc.m_ownerRangeBeg[0], zgc.m_ownerRangeEnd[0],
               zgc.m_ownerRangeBeg[1], zgc.m_ownerRangeEnd[1], zgc.m_ownerRangeBeg[2],
               zgc.m_ownerRangeEnd[2], zgc.m_donorRangeBeg[0], zgc.m_donorRangeEnd[0],
               zgc.m_donorRangeBeg[1], zgc.m_donorRangeEnd[1], zgc.m_donorRangeBeg[2],
               zgc.m_donorRangeEnd[2], zgc.m_ownerRangeBeg[0] - zgc.m_ownerOffset[0],
               zgc.m_ownerRangeEnd[0] - zgc.m_ownerOffset[0],
               zgc.m_ownerRangeBeg[1] - zgc.m_ownerOffset[1],
               zgc.m_ownerRangeEnd[1] - zgc.m_ownerOffset[1],
               zgc.m_ownerRangeBeg[2] - zgc.m_ownerOffset[2],
               zgc.m_ownerRangeEnd[2] - zgc.m_ownerOffset[2],
               zgc.m_donorRangeBeg[0] - zgc.m_donorOffset[0],
               zgc.m_donorRangeEnd[0] - zgc.m_donorOffset[0],
               zgc.m_donorRangeBeg[1] - zgc.m_donorOffset[1],
               zgc.m_donorRangeEnd[1] - zgc.m_donorOffset[1],
               zgc.m_donorRangeBeg[2] - zgc.m_donorOffset[2],
               zgc.m_donorRangeEnd[2] - zgc.m_donorOffset[2]);
#else
    // Formats similar to what is needed for Utst_structured_decomp test
    fmt::print(os,
               "zones.back()->m_zoneConnectivity.emplace_back(\n"
               "\"{0}\", zones.back()->m_zone, \"zone{1}\", {1}, "
               "Ioss::IJK_t{{{2}, {3}, {4}}}, Ioss::IJK_t{{{5}, {6}, {7}}},  Ioss::IJK_t{{{8}, "
               "{9}, {10}}}, Ioss::IJK_t{{{11}, {12}, {13}}}, Ioss::IJK_t{{{14}, {15}, {16}}});",
               zgc.m_connectionName, zgc.m_donorZone, zgc.m_transform[0], zgc.m_transform[1],
               zgc.m_transform[2], zgc.m_ownerRangeBeg[0], zgc.m_ownerRangeBeg[1],
               zgc.m_ownerRangeBeg[2], zgc.m_ownerRangeEnd[0], zgc.m_ownerRangeEnd[1],
               zgc.m_ownerRangeEnd[2], zgc.m_donorRangeBeg[0], zgc.m_donorRangeBeg[1],
               zgc.m_donorRangeBeg[2], zgc.m_donorRangeEnd[0], zgc.m_donorRangeEnd[1],
               zgc.m_donorRangeEnd[2]);
#endif
    return os;
  }

  bool ZoneConnectivity::has_faces() const
  {
    // Determine whether the ownerRange specifies faces instead of just a line...
    if (m_ownerRangeBeg[0] == 0 || m_ownerRangeEnd[0] == 0 || m_ownerRangeBeg[1] == 0 ||
        m_ownerRangeEnd[1] == 0 || m_ownerRangeBeg[2] == 0 || m_ownerRangeEnd[2] == 0) {
      return false;
    }

    auto diff0 = std::abs(m_ownerRangeEnd[0] - m_ownerRangeBeg[0]);
    auto diff1 = std::abs(m_ownerRangeEnd[1] - m_ownerRangeBeg[1]);
    auto diff2 = std::abs(m_ownerRangeEnd[2] - m_ownerRangeBeg[2]);

    int same_count = (diff0 == 0 ? 1 : 0) + (diff1 == 0 ? 1 : 0) + (diff2 == 0 ? 1 : 0);

    if (same_count > 1) {
      return false;
    }
    return true;
  }

  bool ZoneConnectivity::is_valid() const
  {
    bool valid = true;
    if (m_isActive) {
      // Validate transform -- values between -3 and 3 (but not 0) and must have |1| |2| |3|
      Ioss::IJK_t trans_test{};
      for (int i = 0; i < 3; i++) {
        if (m_transform[i] < -3 || m_transform[i] > 3 || m_transform[i] == 0) {
          valid = false;
        }
        else {
          trans_test[std::abs(m_transform[i]) - 1]++;
        }
      }
      for (int i = 0; i < 3; i++) {
        if (trans_test[i] != 1) {
          valid = false;
        }
      }

      // Validate Ranges... All values > 0
      for (int i = 0; i < 3; i++) {
        int owner = m_ownerRangeEnd[i] - m_ownerRangeBeg[i];
        int j     = std::abs(m_transform[i]) - 1;
        int donor = m_donorRangeEnd[j] - m_donorRangeBeg[j];
        if (owner != sign(m_transform[i]) * donor) {
          valid = false;
        }
        if (!valid_range(m_ownerRangeBeg[i], m_ownerRangeEnd[i], m_ownerOffset[i])) {
          valid = false;
        }
        if (!valid_range(m_donorRangeBeg[i], m_donorRangeEnd[i], m_donorOffset[i])) {
          valid = false;
        }
      }

      IJK_t test_donor_end = transform(m_ownerRangeEnd);
      if (test_donor_end != m_donorRangeEnd) {
        valid = false;
      }
      IJK_t test_owner_end = inverse_transform(m_donorRangeEnd);
      if (test_owner_end != m_ownerRangeEnd) {
        valid = false;
      }
    }
    return valid;
  }

  std::vector<int> ZoneConnectivity::get_range(int ordinal) const
  {
    // Return the integer values for the specified range for the specified ordinal (1,2,3) ->
    // (i,j,k)
    ordinal--;
    int size  = std::abs(m_ownerRangeBeg[ordinal] - m_ownerRangeEnd[ordinal]) + 1;
    int delta = sign(m_ownerRangeEnd[ordinal] - m_ownerRangeBeg[ordinal]);
    assert(delta == 1 || delta == -1);

    std::vector<int> range(size);
    for (int i = 0; i < size; i++) {
      range[i] = m_ownerRangeBeg[ordinal] + i * delta;
    }
    return range;
  }

  std::array<INT, 9> ZoneConnectivity::transform_matrix() const
  {
    std::array<INT, 9> t_matrix{};
    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) {
        t_matrix[3 * i + j] = sign(m_transform[j]) * del(m_transform[j], i + 1);
      }
    }
    return t_matrix;
  }

  Ioss::IJK_t ZoneConnectivity::transform(const Ioss::IJK_t &index_1) const
  {
    auto t_matrix = transform_matrix();

    Ioss::IJK_t diff{};
    Ioss::IJK_t donor{};

    diff[0] = index_1[0] - m_ownerRangeBeg[0];
    diff[1] = index_1[1] - m_ownerRangeBeg[1];
    diff[2] = index_1[2] - m_ownerRangeBeg[2];

    donor[0] =
        t_matrix[0] * diff[0] + t_matrix[1] * diff[1] + t_matrix[2] * diff[2] + m_donorRangeBeg[0];
    donor[1] =
        t_matrix[3] * diff[0] + t_matrix[4] * diff[1] + t_matrix[5] * diff[2] + m_donorRangeBeg[1];
    donor[2] =
        t_matrix[6] * diff[0] + t_matrix[7] * diff[1] + t_matrix[8] * diff[2] + m_donorRangeBeg[2];

    assert(std::fabs(donor[0] - m_donorRangeBeg[0]) <=
           std::fabs(m_donorRangeBeg[0] - m_donorRangeEnd[0]));
    assert(std::fabs(donor[1] - m_donorRangeBeg[1]) <=
           std::fabs(m_donorRangeBeg[1] - m_donorRangeEnd[1]));
    assert(std::fabs(donor[2] - m_donorRangeBeg[2]) <=
           std::fabs(m_donorRangeBeg[2] - m_donorRangeEnd[2]));
    return donor;
  }

  // ----------------------------------------------------------------------------

  Ioss::IJK_t ZoneConnectivity::inverse_transform(const Ioss::IJK_t &index_1) const
  {
    auto t_matrix = transform_matrix();

    Ioss::IJK_t diff{};
    Ioss::IJK_t index{};

    diff[0] = index_1[0] - m_donorRangeBeg[0];
    diff[1] = index_1[1] - m_donorRangeBeg[1];
    diff[2] = index_1[2] - m_donorRangeBeg[2];

    index[0] =
        t_matrix[0] * diff[0] + t_matrix[3] * diff[1] + t_matrix[6] * diff[2] + m_ownerRangeBeg[0];
    index[1] =
        t_matrix[1] * diff[0] + t_matrix[4] * diff[1] + t_matrix[7] * diff[2] + m_ownerRangeBeg[1];
    index[2] =
        t_matrix[2] * diff[0] + t_matrix[5] * diff[1] + t_matrix[8] * diff[2] + m_ownerRangeBeg[2];

    return index;
  }

} // namespace Ioss
