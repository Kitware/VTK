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

#include <Ioss_BoundingBox.h>  // for AxisAlignedBoundingBox
#include <Ioss_DatabaseIO.h>   // for DatabaseIO
#include <Ioss_Field.h>        // for Field, etc
#include <Ioss_FieldManager.h> // for FieldManager
#include <Ioss_Hex8.h>
#include <Ioss_Property.h> // for Property
#include <Ioss_Region.h>
#include <Ioss_SmartAssert.h>
#include <Ioss_StructuredBlock.h>
#include <fmt/ostream.h>

#include <cstddef> // for size_t
#include <numeric>
#include <string> // for string
#include <vector> // for vector

namespace {
  int64_t get_cell_count(int ni, int nj, int nk, int index_dim)
  {
    int64_t cell_count = 0;
    if (index_dim == 1) {
      cell_count = static_cast<int64_t>(ni);
    }
    else if (index_dim == 2) {
      cell_count = static_cast<int64_t>(ni) * nj;
    }
    else if (index_dim == 3) {
      cell_count = static_cast<int64_t>(ni) * nj * nk;
    }
    return cell_count;
  }

  int64_t get_node_count(int ni, int nj, int nk, int index_dim)
  {
    int64_t cell_count = get_cell_count(ni, nj, nk, index_dim);
    int64_t node_count = 0;
    if (cell_count > 0) {
      if (index_dim == 1) {
        node_count = static_cast<int64_t>(ni + 1);
      }
      else if (index_dim == 2) {
        node_count = static_cast<int64_t>(ni + 1) * (nj + 1);
      }
      else if (index_dim == 3) {
        node_count = static_cast<int64_t>(ni + 1) * (nj + 1) * (nk + 1);
      }
    }
    return node_count;
  }
} // namespace

namespace Ioss {
  class Field;

  /** \brief Create a structured block.
   *
   *  \param[in] io_database The database associated with the region containing the structured
   * block.
   *  \param[in] my_name The structured block's name.
   *  \param[in] index_dim The dimensionality of the block -- 1D, 2D, 3D
   *  \param[in] ni The number of intervals in the (i) direction.
   *  \param[in] nj The number of intervals in the (j) direction. Zero if 1D
   *  \param[in] nk The number of intervals in the (k) direction. Zero if 2D
   */
  // Serial
  StructuredBlock::StructuredBlock(DatabaseIO *io_database, const std::string &my_name,
                                   int index_dim, int ni, int nj, int nk)
      : StructuredBlock(io_database, my_name, index_dim, ni, nj, nk, 0, 0, 0, ni, nj, nk)
  {
  }

  // Parallel
  StructuredBlock::StructuredBlock(DatabaseIO *io_database, const std::string &my_name,
                                   int index_dim, Ioss::IJK_t &ordinal, Ioss::IJK_t &offset,
                                   Ioss::IJK_t &global_ordinal)
      : StructuredBlock(io_database, my_name, index_dim, ordinal[0], ordinal[1], ordinal[2],
                        offset[0], offset[1], offset[2], global_ordinal[0], global_ordinal[1],
                        global_ordinal[2])
  {
  }

  // Serial
  StructuredBlock::StructuredBlock(DatabaseIO *io_database, const std::string &my_name,
                                   int index_dim, Ioss::IJK_t &ordinal)
      : StructuredBlock(io_database, my_name, index_dim, ordinal[0], ordinal[1], ordinal[2], 0, 0,
                        0, ordinal[0], ordinal[1], ordinal[2])
  {
  }

  // Parallel
  StructuredBlock::StructuredBlock(DatabaseIO *io_database, const std::string &my_name,
                                   int index_dim, int ni, int nj, int nk, int off_i, int off_j,
                                   int off_k, int glo_ni, int glo_nj, int glo_nk)
      : EntityBlock(io_database, my_name, Ioss::Hex8::name, get_cell_count(ni, nj, nk, index_dim)),
        m_ni(ni), m_nj(nj), m_nk(nk), m_offsetI(off_i), m_offsetJ(off_j), m_offsetK(off_k),
        m_niGlobal(glo_ni == 0 ? m_ni : glo_ni), m_njGlobal(glo_nj == 0 ? m_nj : glo_nj),
        m_nkGlobal(glo_nk == 0 ? m_nk : glo_nk),
        m_nodeBlock(io_database, my_name + "_nodes", get_node_count(m_ni, m_nj, m_nk, index_dim),
                    index_dim)
  {
    m_nodeBlock.property_add(Property("IOSS_INTERNAL_CONTAINED_IN", this));

    SMART_ASSERT(index_dim == 1 || index_dim == 2 || index_dim == 3)(index_dim);

    int64_t cell_count        = get_cell_count(m_ni, m_nj, m_nk, index_dim);
    int64_t node_count        = get_node_count(m_ni, m_nj, m_nk, index_dim);
    int64_t global_cell_count = get_cell_count(m_niGlobal, m_njGlobal, m_nkGlobal, index_dim);
    int64_t global_node_count = get_node_count(m_niGlobal, m_njGlobal, m_nkGlobal, index_dim);

    SMART_ASSERT(global_cell_count >= cell_count)(global_cell_count)(cell_count);
    SMART_ASSERT(global_node_count >= node_count)(global_node_count)(node_count);
    SMART_ASSERT(m_niGlobal >= m_ni)(m_niGlobal)(m_ni);
    SMART_ASSERT(m_njGlobal >= m_nj)(m_njGlobal)(m_nj);
    SMART_ASSERT(m_nkGlobal >= m_nk)(m_nkGlobal)(m_nk);
    SMART_ASSERT(m_niGlobal >= m_ni + m_offsetI)(m_niGlobal)(m_ni)(m_offsetI);
    SMART_ASSERT(m_njGlobal >= m_nj + m_offsetJ)(m_njGlobal)(m_nj)(m_offsetJ);
    SMART_ASSERT(m_nkGlobal >= m_nk + m_offsetK)(m_nkGlobal)(m_nk)(m_offsetK);

    properties.add(Property("component_degree", index_dim));
    properties.add(Property("node_count", node_count));
    properties.add(Property("cell_count", cell_count));
    properties.add(Property("global_node_count", global_node_count));
    properties.add(Property("global_cell_count", global_cell_count));

    properties.add(Property("ni", m_ni));
    properties.add(Property("nj", m_nj));
    properties.add(Property("nk", m_nk));

    properties.add(Property("ni_global", m_niGlobal));
    properties.add(Property("nj_global", m_njGlobal));
    properties.add(Property("nk_global", m_nkGlobal));

    properties.add(Property("offset_i", m_offsetI));
    properties.add(Property("offset_j", m_offsetJ));
    properties.add(Property("offset_k", m_offsetK));

    std::string vector_name;
    if (index_dim == 1) {
      vector_name = IOSS_SCALAR();
    }
    else if (index_dim == 2) {
      vector_name = IOSS_VECTOR_2D();
    }
    else if (index_dim == 3) {
      vector_name = IOSS_VECTOR_3D();
    }
    fields.add(Ioss::Field("cell_ids", Ioss::Field::INTEGER, IOSS_SCALAR(), Ioss::Field::MESH,
                           cell_count));

    fields.add(Ioss::Field("cell_node_ids", Ioss::Field::INTEGER, IOSS_SCALAR(), Ioss::Field::MESH,
                           node_count));

    fields.add(Ioss::Field("mesh_model_coordinates", Ioss::Field::REAL, vector_name,
                           Ioss::Field::MESH, node_count));

    // Permit access 1-coordinate at a time
    fields.add(Ioss::Field("mesh_model_coordinates_x", Ioss::Field::REAL, IOSS_SCALAR(),
                           Ioss::Field::MESH, node_count));
    if (index_dim > 1) {
      fields.add(Ioss::Field("mesh_model_coordinates_y", Ioss::Field::REAL, IOSS_SCALAR(),
                             Ioss::Field::MESH, node_count));
    }
    if (index_dim > 2) {
      fields.add(Ioss::Field("mesh_model_coordinates_z", Ioss::Field::REAL, IOSS_SCALAR(),
                             Ioss::Field::MESH, node_count));
    }
  }

  StructuredBlock::~StructuredBlock() = default;

  StructuredBlock *StructuredBlock::clone(DatabaseIO *database) const
  {
    int index_dim = properties.get("component_degree").get_int();

    IJK_t ijk{{m_ni, m_nj, m_nk}};
    IJK_t offset{{m_offsetI, m_offsetJ, m_offsetK}};
    IJK_t ijk_glob{{m_niGlobal, m_njGlobal, m_nkGlobal}};

    auto block = new StructuredBlock(database, name(), index_dim, ijk, offset, ijk_glob);

    block->m_zoneConnectivity   = m_zoneConnectivity;
    block->m_boundaryConditions = m_boundaryConditions;

    return block;
  }

  Property StructuredBlock::get_implicit_property(const std::string &my_name) const
  {
    return EntityBlock::get_implicit_property(my_name);
  }

  int64_t StructuredBlock::internal_get_field_data(const Field &field, void *data,
                                                   size_t data_size) const
  {
    return get_database()->get_field(this, field, data, data_size);
  }

  int64_t StructuredBlock::internal_put_field_data(const Field &field, void *data,
                                                   size_t data_size) const
  {
    return get_database()->put_field(this, field, data, data_size);
  }

  AxisAlignedBoundingBox StructuredBlock::get_bounding_box() const
  {
    return get_database()->get_bounding_box(this);
  }

  size_t BoundaryCondition::get_face_count() const
  {
    if (m_rangeBeg[0] == 0 || m_rangeEnd[0] == 0 || m_rangeBeg[1] == 0 || m_rangeEnd[1] == 0 ||
        m_rangeBeg[2] == 0 || m_rangeEnd[2] == 0) {
      return 0;
    }

    auto diff0 = std::abs(m_rangeEnd[0] - m_rangeBeg[0]);
    auto diff1 = std::abs(m_rangeEnd[1] - m_rangeBeg[1]);
    auto diff2 = std::abs(m_rangeEnd[2] - m_rangeBeg[2]);

    int same_count = (diff0 == 0 ? 1 : 0) + (diff1 == 0 ? 1 : 0) + (diff2 == 0 ? 1 : 0);

    if (same_count > 1) {
      return 0;
    }
    diff0 = std::max(diff0, 1);
    diff1 = std::max(diff1, 1);
    diff2 = std::max(diff2, 1);

    return diff0 * diff1 * diff2;
  }

  bool BoundaryCondition::is_valid() const
  {
    // Return true/false if range specifies a valid face
    bool is_x = m_rangeBeg[0] == m_rangeEnd[0];
    bool is_y = m_rangeBeg[1] == m_rangeEnd[1];
    bool is_z = m_rangeBeg[2] == m_rangeEnd[2];

    return ((is_x ? 1 : 0) + (is_y ? 1 : 0) + (is_z ? 1 : 0) == 1);
  }

  int BoundaryCondition::which_face() const
  {
    if (m_face == -1) {
      // Determine which "face" of the parent block this BC is applied to.
      // min X, max X, min Y, max Y, min Z, max Z -- 0, 3, 1, 4, 2, 5
      if (m_rangeBeg[0] == 0 || m_rangeEnd[0] == 0 || m_rangeBeg[1] == 0 || m_rangeEnd[1] == 0 ||
          m_rangeBeg[2] == 0 || m_rangeEnd[2] == 0) {
        m_face = -1;
      }
      else if (m_rangeBeg[0] == m_rangeEnd[0]) {
        m_face = (m_rangeBeg[0] == 1) ? 0 : 3;
      }
      else if (m_rangeBeg[1] == m_rangeEnd[1]) {
        m_face = (m_rangeBeg[1] == 1) ? 1 : 4;
      }
      else if (m_rangeBeg[2] == m_rangeEnd[2]) {
        m_face = (m_rangeBeg[2] == 1) ? 2 : 5;
      }
    }
    return m_face;
  }

  std::ostream &operator<<(std::ostream &os, const BoundaryCondition &bc)
  {
    fmt::print(os, "\t\tBC Name '{}' owns {:10n} faces.\tRange: [{}..{}, {}..{}, {}..{}]",
               bc.m_bcName, bc.get_face_count(), bc.m_rangeBeg[0], bc.m_rangeEnd[0],
               bc.m_rangeBeg[1], bc.m_rangeEnd[1], bc.m_rangeBeg[2], bc.m_rangeEnd[2]);
    return os;
  }

} // namespace Ioss
