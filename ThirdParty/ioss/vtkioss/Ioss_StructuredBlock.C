// Copyright(C) 1999-2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_BoundingBox.h"  // for AxisAlignedBoundingBox
#include "Ioss_DatabaseIO.h"   // for DatabaseIO
#include "Ioss_Field.h"        // for Field, etc
#include "Ioss_FieldManager.h" // for FieldManager
#include "Ioss_Hex8.h"
#include "Ioss_Property.h" // for Property
#include "Ioss_SmartAssert.h"
#include "Ioss_StructuredBlock.h"
#include <cmath>
#include <cstddef> // for size_t
#include "vtk_fmt.h"
#include VTK_FMT(fmt/format.h)
#include VTK_FMT(fmt/ostream.h)
#include VTK_FMT(fmt/ranges.h)
#include <iostream>
#include <stdlib.h>
#include <string> // for string
#include <vector> // for vector

#include "Ioss_CodeTypes.h"
#include "Ioss_EntityBlock.h"
#include "Ioss_NodeBlock.h"
#include "Ioss_ParallelUtils.h"
#include "Ioss_PropertyManager.h"
#include "Ioss_Utils.h"
#include "Ioss_ZoneConnectivity.h"

namespace {
  template <typename T> bool vec_equal(const std::vector<T> &lhs, const std::vector<T> &rhs)
  {
    bool same = true;
    if (lhs.size() != rhs.size()) {
      same = false;
    }
    auto size = std::min(lhs.size(), rhs.size());
    for (size_t i = 0; i < size; i++) {
      if (!lhs[i].equal(rhs[i])) {
        same = false;
      }
    }
    return same;
  }

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
      cell_count = static_cast<int64_t>(ni) * nj * (nk == 0 ? 1 : nk);
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

  int64_t get_cell_count(const Ioss::IJK_t &ijk, int index_dim)
  {
    return get_cell_count(ijk[0], ijk[1], ijk[2], index_dim);
  }

  int64_t get_node_count(const Ioss::IJK_t &ijk, int index_dim)
  {
    return get_node_count(ijk[0], ijk[1], ijk[2], index_dim);
  }
} // namespace

namespace Ioss {

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
                                   int index_dim, const Ioss::IJK_t &ordinal,
                                   const Ioss::IJK_t &offset, const Ioss::IJK_t &global_ordinal)
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
        m_ijk{ni, nj, nk}, m_offset{off_i, off_j, off_k},
        m_nodeBlock(io_database, my_name + "_nodes",
                    get_node_count(m_ijk[0], m_ijk[1], m_ijk[2], index_dim), index_dim)
  {
    m_nodeBlock.property_add(Property("IOSS_INTERNAL_CONTAINED_IN", this));

    SMART_ASSERT(index_dim == 1 || index_dim == 2 || index_dim == 3)(index_dim);

    m_ijkGlobal[0] = (glo_ni == 0 ? m_ijk[0] : glo_ni);
    m_ijkGlobal[1] = (glo_nj == 0 ? m_ijk[1] : glo_nj);
    m_ijkGlobal[2] = (glo_nk == 0 ? m_ijk[2] : glo_nk);

    int64_t cell_count        = get_cell_count(m_ijk, index_dim);
    int64_t node_count        = get_node_count(m_ijk, index_dim);
    int64_t global_cell_count = get_cell_count(m_ijkGlobal, index_dim);
    int64_t global_node_count = get_node_count(m_ijkGlobal, index_dim);

    SMART_ASSERT(global_cell_count >= cell_count)(global_cell_count)(cell_count);
    SMART_ASSERT(global_node_count >= node_count)(global_node_count)(node_count);

    if ((m_ijkGlobal[0] < m_ijk[0] + m_offset[0]) || (m_ijkGlobal[1] < m_ijk[1] + m_offset[1]) ||
        (m_ijkGlobal[2] < m_ijk[2] + m_offset[2])) {
      auto               util = get_database()->util();
      std::ostringstream errmsg;
      fmt::print(errmsg,
                 "\nERROR: Inconsistent Structured Block parameters for block {} on rank {}.\n"
                 "       Global IJK: {} x {} x {}; Local IJK: {} x {} x {}; Offset: {} x {} x {}\n"
                 "       Global must be >= Local + Offset.\n",
                 my_name, util.parallel_rank(), m_ijkGlobal[0], m_ijkGlobal[1], m_ijkGlobal[2],
                 m_ijk[0], m_ijk[1], m_ijk[2], m_offset[0], m_offset[1], m_offset[2]);
      std::cerr << errmsg.str();
      IOSS_ERROR(errmsg);
    }

    SMART_ASSERT(m_ijkGlobal[0] >= m_ijk[0])(m_ijkGlobal[0])(m_ijk[0]);
    SMART_ASSERT(m_ijkGlobal[1] >= m_ijk[1])(m_ijkGlobal[1])(m_ijk[1]);
    SMART_ASSERT(m_ijkGlobal[2] >= m_ijk[2])(m_ijkGlobal[2])(m_ijk[2]);
    SMART_ASSERT(m_ijkGlobal[0] >= m_ijk[0] + m_offset[0])(m_ijkGlobal[0])(m_ijk[0])(m_offset[0]);
    SMART_ASSERT(m_ijkGlobal[1] >= m_ijk[1] + m_offset[1])(m_ijkGlobal[1])(m_ijk[1])(m_offset[1]);
    SMART_ASSERT(m_ijkGlobal[2] >= m_ijk[2] + m_offset[2])(m_ijkGlobal[2])(m_ijk[2])(m_offset[2]);

    properties.add(Property("component_degree", index_dim));
    properties.add(Property("node_count", node_count));
    properties.add(Property("cell_count", cell_count));
    properties.add(Property("global_node_count", global_node_count));
    properties.add(Property("global_cell_count", global_cell_count));

    properties.add(Property("ni", m_ijk[0]));
    properties.add(Property("nj", m_ijk[1]));
    properties.add(Property("nk", m_ijk[2]));

    properties.add(Property(this, "ni_global", Ioss::Property::INTEGER));
    properties.add(Property(this, "nj_global", Ioss::Property::INTEGER));
    properties.add(Property(this, "nk_global", Ioss::Property::INTEGER));

    properties.add(Property(this, "offset_i", Ioss::Property::INTEGER));
    properties.add(Property(this, "offset_j", Ioss::Property::INTEGER));
    properties.add(Property(this, "offset_k", Ioss::Property::INTEGER));

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

  StructuredBlock *StructuredBlock::clone(DatabaseIO *database) const
  {
    int   index_dim = properties.get("component_degree").get_int();
    auto *block = new StructuredBlock(database, name(), index_dim, m_ijk, m_offset, m_ijkGlobal);

    block->m_zoneConnectivity    = m_zoneConnectivity;
    block->m_boundaryConditions  = m_boundaryConditions;
    block->m_blockLocalNodeIndex = m_blockLocalNodeIndex;
    block->m_globalIdMap         = m_globalIdMap;

    return block;
  }

  void StructuredBlock::set_ijk_offset(int axis, size_t offset)
  {
    SMART_ASSERT(axis == 0 || axis == 1 || axis == 2)(axis);
    if (axis == 0) {
      m_offset[0] = offset;
    }
    else if (axis == 1) {
      m_offset[1] = offset;
    }
    else if (axis == 2) {
      m_offset[2] = offset;
    }
  }

  void StructuredBlock::set_ijk_global(int axis, size_t global)
  {
    SMART_ASSERT(axis == 0 || axis == 1 || axis == 2)(axis);
    if (axis == 0) {
      m_ijkGlobal[0] = global;
      SMART_ASSERT(m_ijkGlobal[0] >= m_ijk[0] + m_offset[0])(m_ijkGlobal[0])(m_ijk[0])(m_offset[0]);
    }
    else if (axis == 1) {
      m_ijkGlobal[1] = global;
      SMART_ASSERT(m_ijkGlobal[1] >= m_ijk[1] + m_offset[1])(m_ijkGlobal[1])(m_ijk[1])(m_offset[1]);
    }
    else if (axis == 2) {
      m_ijkGlobal[2] = global;
      SMART_ASSERT(m_ijkGlobal[2] >= m_ijk[2] + m_offset[2])(m_ijkGlobal[2])(m_ijk[2])(m_offset[2]);
    }
  }

  void StructuredBlock::set_ijk_offset(const IJK_t &offset) { m_offset = offset; }

  void StructuredBlock::set_ijk_global(const IJK_t &global)
  {
    m_ijkGlobal = global;

    SMART_ASSERT(m_ijkGlobal[0] >= m_ijk[0] + m_offset[0])(m_ijkGlobal[0])(m_ijk[0])(m_offset[0]);
    SMART_ASSERT(m_ijkGlobal[1] >= m_ijk[1] + m_offset[1])(m_ijkGlobal[1])(m_ijk[1])(m_offset[1]);
    SMART_ASSERT(m_ijkGlobal[2] >= m_ijk[2] + m_offset[2])(m_ijkGlobal[2])(m_ijk[2])(m_offset[2]);
  }

  Property StructuredBlock::get_implicit_property(const std::string &my_name) const
  {
    if (my_name == "ni_global") {
      return {my_name, m_ijkGlobal[0]};
    }
    if (my_name == "nj_global") {
      return {my_name, m_ijkGlobal[1]};
    }
    if (my_name == "nk_global") {
      return {my_name, m_ijkGlobal[2]};
    }
    if (my_name == "offset_i") {
      return {my_name, m_offset[0]};
    }
    if (my_name == "offset_j") {
      return {my_name, m_offset[1]};
    }
    if (my_name == "offset_k") {
      return {my_name, m_offset[2]};
    }
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

  int64_t StructuredBlock::internal_get_zc_field_data(const Field &field, void **data,
                                                      size_t *data_size) const
  {
    return get_database()->get_zc_field(this, field, data, data_size);
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
    fmt::print(os, "\t\tBC Name '{}' owns {:10} faces.\tRange: [{}..{}, {}..{}, {}..{}]",
               bc.m_bcName, fmt::group_digits(bc.get_face_count()), bc.m_rangeBeg[0],
               bc.m_rangeEnd[0], bc.m_rangeBeg[1], bc.m_rangeEnd[1], bc.m_rangeBeg[2],
               bc.m_rangeEnd[2]);
    return os;
  }

  bool BoundaryCondition::equal_(const Ioss::BoundaryCondition &rhs, bool quiet) const
  {
    // If `quiet` is true, then this is a helper function for the operator=
    // and should return as soon as the equal/not-equal status is determined
    //
    // If `quiet` is false, then this is a helper function for the mesh compare
    // utility and should report as many differences as it finds, so don't
    // return until end...
    bool same = true;

    if (this->m_bcName != rhs.m_bcName) {
      if (quiet) {
        return false;
      }
      fmt::print(Ioss::OUTPUT(), "BoundaryCondition: m_bcName MISMATCH ('{}' vs. '{}')\n",
                 this->m_bcName, rhs.m_bcName);
      same = false;
    }

    if (this->m_famName != rhs.m_famName) {
      if (quiet) {
        return false;
      }
      fmt::print(Ioss::OUTPUT(), "BoundaryCondition: m_famName MISMATCH ('{}' vs. '{}')\n",
                 this->m_famName, rhs.m_famName);
      same = false;
    }

    if (this->m_rangeBeg != rhs.m_rangeBeg) {
      if (quiet) {
        return false;
      }
      fmt::print(Ioss::OUTPUT(), "BoundaryCondition: m_rangeBeg MISMATCH ({} vs. {})\n",
                 fmt::join(this->m_rangeBeg, ":"), fmt::join(rhs.m_rangeBeg, ":"));
      same = false;
    }

    if (this->m_rangeEnd != rhs.m_rangeEnd) {
      if (quiet) {
        return false;
      }
      fmt::print(Ioss::OUTPUT(), "BoundaryCondition: m_rangeEnd MISMATCH ({} vs. {})\n",
                 fmt::join(this->m_rangeEnd, ":"), fmt::join(rhs.m_rangeEnd, ":"));
      same = false;
    }
    return same;
  }

  bool BoundaryCondition::operator==(const Ioss::BoundaryCondition &rhs) const
  {
    return equal_(rhs, true);
  }

  bool BoundaryCondition::equal(const Ioss::BoundaryCondition &rhs) const
  {
    return equal_(rhs, false);
  }

  bool StructuredBlock::equal_(const Ioss::StructuredBlock &rhs, bool quiet) const
  {
    bool same = true;
    if (this->m_ijk != rhs.m_ijk) {
      if (quiet) {
        return false;
      }
      fmt::print(Ioss::OUTPUT(), "StructuredBlock: N mismatch ({} vs. {})\n",
                 fmt::join(this->m_ijk, ":"), fmt::join(rhs.m_ijk, ":"));
      same = false;
    }

    if (this->m_offset != rhs.m_offset) {
      if (quiet) {
        return false;
      }
      fmt::print(Ioss::OUTPUT(), "StructuredBlock: OFFSET mismatch ({} vs. {})\n",
                 fmt::join(this->m_offset, ":"), fmt::join(rhs.m_offset, ":"));
      same = false;
    }

    if (this->m_ijkGlobal != rhs.m_ijkGlobal) {
      if (quiet) {
        return false;
      }
      fmt::print(Ioss::OUTPUT(), "StructuredBlock: Global N mismatch ({} vs. {})\n",
                 fmt::join(this->m_ijkGlobal, ":"), fmt::join(rhs.m_ijkGlobal, ":"));
      same = false;
    }

    if (this->m_nodeOffset != rhs.m_nodeOffset) {
      if (quiet) {
        return false;
      }
      fmt::print(Ioss::OUTPUT(), "StructuredBlock: Node Offset mismatch ({} vs. {})\n",
                 this->m_nodeOffset, rhs.m_nodeOffset);
      same = false;
    }

    if (this->m_cellOffset != rhs.m_cellOffset) {
      if (quiet) {
        return false;
      }
      fmt::print(Ioss::OUTPUT(), "StructuredBlock: Cell Offset mismatch ({} vs. {})\n",
                 this->m_cellOffset, rhs.m_cellOffset);
      same = false;
    }

    if (this->m_nodeGlobalOffset != rhs.m_nodeGlobalOffset) {
      if (!quiet) {
        return false;
      }
      fmt::print(Ioss::OUTPUT(), "StructuredBlock: Node Global Offset mismatch ({} vs. {})\n",
                 this->m_nodeGlobalOffset, rhs.m_nodeGlobalOffset);
      same = false;
    }

    if (this->m_cellGlobalOffset != rhs.m_cellGlobalOffset) {
      if (quiet) {
        return false;
      }
      fmt::print(Ioss::OUTPUT(), "StructuredBlock: Cell Global Offset mismatch ({} vs. {})\n",
                 this->m_cellGlobalOffset, rhs.m_cellGlobalOffset);
      same = false;
    }

    if (this->m_blockLocalNodeIndex != rhs.m_blockLocalNodeIndex) {
      if (quiet) {
        return false;
      }
      fmt::print(Ioss::OUTPUT(),
                 "StructuredBlock: Block Local Node Index mismatch ({} entries vs. {} entries)\n",
                 this->m_blockLocalNodeIndex.size(), rhs.m_blockLocalNodeIndex.size());
      same = false;
    }

    // NOTE: this comparison assumes that the elements of this vector will
    // appear in the same order in two databases that are equivalent.
    if (this->m_globalIdMap != rhs.m_globalIdMap) {
      if (quiet) {
        return false;
      }
      fmt::print(Ioss::OUTPUT(), "StructuredBlock: Global ID Map mismatch\n");

      same = false;
    }

    // NOTE: this comparison assumes that the elements of this vector will
    // appear in the same order in two databases that are equivalent.
    if (quiet && this->m_zoneConnectivity != rhs.m_zoneConnectivity) {
      return false;
    }
    if (!vec_equal(this->m_zoneConnectivity, rhs.m_zoneConnectivity)) {
      fmt::print(Ioss::OUTPUT(), "StructuredBlock: Zone Connectivity mismatch (size {} vs {})\n",
                 this->m_zoneConnectivity.size(), rhs.m_zoneConnectivity.size());
      same = false;
    }

    // NOTE: this comparison assumes that the elements of this vector will
    // appear in the same order in two databases that are equivalent.
    if (quiet && this->m_boundaryConditions != rhs.m_boundaryConditions) {
      return false;
    }
    if (!vec_equal(this->m_boundaryConditions, rhs.m_boundaryConditions)) {
      fmt::print(Ioss::OUTPUT(), "StructuredBlock: Boundary Conditions mismatch\n");
      same = false;
    }

    if (!quiet) {
      if (!Ioss::EntityBlock::equal(rhs)) {
        fmt::print(Ioss::OUTPUT(), "StructuredBlock: EntityBlock mismatch\n");
        same = false;
      }
    }
    else {
      if (Ioss::EntityBlock::operator!=(rhs)) {
        return false;
      }
    }

    return same;
  }

  bool StructuredBlock::operator==(const Ioss::StructuredBlock &rhs) const
  {
    return equal_(rhs, true);
  }

  bool StructuredBlock::operator!=(const Ioss::StructuredBlock &rhs) const
  {
    return !(*this == rhs);
  }

  bool StructuredBlock::equal(const Ioss::StructuredBlock &rhs) const { return equal_(rhs, false); }
} // namespace Ioss
