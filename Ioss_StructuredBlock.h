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

#ifndef IOSS_Ioss_StructuredBlock_h
#define IOSS_Ioss_StructuredBlock_h

#include "vtk_ioss_mangle.h"

#include <Ioss_BoundingBox.h>
#include <Ioss_CodeTypes.h>
#include <Ioss_EntityBlock.h>
#include <Ioss_NodeBlock.h>
#include <Ioss_Property.h>
#include <Ioss_ZoneConnectivity.h>
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

  struct BoundaryCondition
  {
    BoundaryCondition(const std::string name, const std::string fam_name,
                      const Ioss::IJK_t range_beg, const Ioss::IJK_t range_end)
        : m_bcName(std::move(name)), m_famName(std::move(fam_name)),
          m_rangeBeg(std::move(range_beg)), m_rangeEnd(std::move(range_end))
    {
    }

    // Deprecated... Use the constructor above with both name and fam_name
    BoundaryCondition(const std::string name, const Ioss::IJK_t range_beg,
                      const Ioss::IJK_t range_end)
        : m_bcName(name), m_famName(std::move(name)), m_rangeBeg(std::move(range_beg)),
          m_rangeEnd(std::move(range_end))
    {
    }

    BoundaryCondition(const BoundaryCondition &copy_from) = default;

    // Determine which "face" of the parent block this BC is applied to.
    int which_face() const;

    // Does range specify a valid face
    bool is_valid() const;

    // Return number of cell faces in the BC
    size_t get_face_count() const;

    std::string m_bcName{};
    std::string m_famName{};

    // These are potentially subsetted due to parallel decompositions...
    Ioss::IJK_t m_rangeBeg{};
    Ioss::IJK_t m_rangeEnd{};

    mutable int m_face{-1};

    friend std::ostream &operator<<(std::ostream &os, const BoundaryCondition &bc);
  };

  class DatabaseIO;

  /** \brief A structured zone -- i,j,k
   */
  class StructuredBlock : public EntityBlock
  {
  public:
    StructuredBlock(DatabaseIO *io_database, const std::string &my_name, int index_dim, int ni,
                    int nj, int nk, int off_i, int off_j, int off_k, int glo_ni, int glo_nj,
                    int glo_nk);
    StructuredBlock(DatabaseIO *io_database, const std::string &my_name, int index_dim,
                    Ioss::IJK_t &ordinal, Ioss::IJK_t &offset, Ioss::IJK_t &global_ordinal);

    // Useful for serial
    StructuredBlock(DatabaseIO *io_database, const std::string &my_name, int index_dim, int ni,
                    int nj, int nk);
    StructuredBlock(DatabaseIO *io_database, const std::string &my_name, int index_dim,
                    Ioss::IJK_t &ordinal);

    StructuredBlock *clone(DatabaseIO *database) const;

    ~StructuredBlock() override;

    std::string type_string() const override { return "StructuredBlock"; }
    std::string short_type_string() const override { return "structuredblock"; }
    std::string contains_string() const override { return "Cell"; }
    EntityType  type() const override { return STRUCTUREDBLOCK; }

    const Ioss::NodeBlock &get_node_block() const { return m_nodeBlock; }
    Ioss::NodeBlock &      get_node_block() { return m_nodeBlock; }

    /** \brief Does block contain any cells
     */
    bool is_active() const { return m_ni * m_nj * m_nk > 0; }

    // Handle implicit properties -- These are calcuated from data stored
    // in the grouping entity instead of having an explicit value assigned.
    // An example would be 'element_block_count' for a region.
    Property get_implicit_property(const std::string &my_name) const override;

    AxisAlignedBoundingBox get_bounding_box() const;

    /** \brief Set the 'offset' for the block.
     *
     *  The 'offset' is used to map a cell or node location within a
     *  structured block to the model implicit cell or node location
     *  on a single processor.  zero-based.
     *
     *  The 'global' offsets do the same except for they apply over
     *  the entire model on all processors. zero-based.
     *
     *  For example, the file descriptor (1-based) of
     *  the 37th cell in the 4th block is calculated by:
     *
     *  file_descriptor = offset of block 4 + 37
     *
     *  This can also be used to determine which structured block
     *  a cell with a file_descriptor maps into. An particular
     *  structured block contains all cells in the range:
     *
     *  offset < file_descriptor <= offset+number_cells_per_block
     *
     *  Note that for nodes, the nodeOffset does not take into account
     *  the nodes that are shared between blocks.
     */
    void set_node_offset(size_t offset) { m_nodeOffset = offset; }
    void set_cell_offset(size_t offset) { m_cellOffset = offset; }
    void set_node_global_offset(size_t offset) { m_nodeGlobalOffset = offset; }
    void set_cell_global_offset(size_t offset) { m_cellGlobalOffset = offset; }

    size_t get_node_offset() const { return m_nodeOffset; }
    size_t get_cell_offset() const { return m_cellOffset; }
    size_t get_node_global_offset() const { return m_nodeGlobalOffset; }
    size_t get_cell_global_offset() const { return m_cellGlobalOffset; }

    // Get the global (over all processors) cell
    // id at the specified i,j,k location (1 <= i,j,k <= ni,nj,nk).  1-based.
    size_t get_global_cell_id(int i, int j, int k) const
    {
      return m_cellGlobalOffset + static_cast<size_t>(k - 1) * m_niGlobal * m_njGlobal +
             static_cast<size_t>(j - 1) * m_niGlobal + i;
    }

    size_t get_global_cell_id(IJK_t index) const
    {
      return get_global_cell_id(index[0], index[1], index[2]);
    }

    // Get the global (over all processors) node
    // offset at the specified i,j,k location (1 <= i,j,k <= ni,nj,nk).  0-based, does not account
    // for shared nodes.
    size_t get_global_node_offset(int i, int j, int k) const
    {
      return m_nodeGlobalOffset + static_cast<size_t>(k - 1) * (m_niGlobal + 1) * (m_njGlobal + 1) +
             static_cast<size_t>(j - 1) * (m_niGlobal + 1) + i - 1;
    }

    size_t get_global_node_offset(IJK_t index) const
    {
      return get_global_node_offset(index[0], index[1], index[2]);
    }

    // Get the local (relative to this block on this processor) node id at the specified
    // i,j,k location (1 <= i,j,k <= ni+1,nj+1,nk+1).  0-based.
    size_t get_block_local_node_offset(int ii, int jj, int kk) const
    {
      auto i = ii - m_offsetI;
      auto j = jj - m_offsetJ;
      auto k = kk - m_offsetK;
      assert(i > 0 && i <= m_ni + 1 && j > 0 && j <= m_nj + 1 && k > 0 && k <= m_nk + 1);
      return static_cast<size_t>(k - 1) * (m_ni + 1) * (m_nj + 1) +
             static_cast<size_t>(j - 1) * (m_ni + 1) + i - 1;
    }

    size_t get_block_local_node_offset(IJK_t index) const
    {
      return get_block_local_node_offset(index[0], index[1], index[2]);
    }

    // Get the local (on this processor) cell-node offset at the specified
    // i,j,k location (1 <= i,j,k <= ni+1,nj+1,nk+1).  0-based.
    size_t get_local_node_offset(int i, int j, int k) const
    {
      return get_block_local_node_offset(i, j, k) + m_nodeOffset;
    }

    size_t get_local_node_offset(IJK_t index) const
    {
      return get_local_node_offset(index[0], index[1], index[2]);
    }

    std::vector<INT> get_cell_node_ids(bool add_offset) const
    {
      size_t           node_count = get_property("node_count").get_int();
      std::vector<INT> ids(node_count);
      get_cell_node_ids(ids.data(), add_offset);
      return ids;
    }

    template <typename INT_t> size_t get_cell_node_ids(INT_t *idata, bool add_offset) const
    {
      // Fill 'idata' with the cell node ids which are the
      // 1-based location of each node in this zone
      // The location is based on the "model" (all processors) zone.
      // If this is a parallel decomposed model, then
      // this block may be a subset of the "model" zone
      //
      // if 'add_offset' is true, then add the m_cellGlobalOffset
      // which changes the location to be the location in the
      // entire "mesh" instead of within a "zone" (all processors)

      size_t index  = 0;
      size_t offset = add_offset ? m_nodeGlobalOffset : 0;

      if (m_nk == 0 && m_nj == 0 && m_ni == 0) {
        return index;
      }

      for (int kk = 0; kk < m_nk + 1; kk++) {
        size_t k = m_offsetK + kk;
        for (int jj = 0; jj < m_nj + 1; jj++) {
          size_t j = m_offsetJ + jj;
          for (int ii = 0; ii < m_ni + 1; ii++) {
            size_t i = m_offsetI + ii;

            size_t ind = k * (m_niGlobal + 1) * (m_njGlobal + 1) + j * (m_niGlobal + 1) + i;

            idata[index++] = ind + offset + 1;
          }
        }
      }

      for (auto idx_id : m_globalIdMap) {
        idata[idx_id.first] = idx_id.second;
      }

      return index;
    }

    template <typename INT_t> size_t get_cell_ids(INT_t *idata, bool add_offset) const
    {
      // Fill 'idata' with the cell ids which are the
      // 1-based location of each cell in this zone
      // The location is based on the "model" zone.
      // If this is a parallel decomposed model, then
      // this block may be a subset of the "model" zone
      //
      // if 'add_offset' is true, then add the m_cellGlobalOffset
      // which changes the location to be the location in the
      // entire "mesh" instead of within a "zone"

      size_t index  = 0;
      size_t offset = add_offset ? m_cellGlobalOffset : 0;

      if (m_nk == 0 && m_nj == 0 && m_ni == 0) {
        return index;
      }

      for (int kk = 0; kk < m_nk; kk++) {
        size_t k = m_offsetK + kk;
        for (int jj = 0; jj < m_nj; jj++) {
          size_t j = m_offsetJ + jj;
          for (int ii = 0; ii < m_ni; ii++) {
            size_t i = m_offsetI + ii;

            size_t ind = k * m_niGlobal * m_njGlobal + j * m_niGlobal + i;

            idata[index++] = ind + offset + 1;
          }
        }
      }
      return index;
    }

    bool contains(size_t global_offset) const
    {
      return (global_offset >= m_nodeOffset &&
              global_offset < m_nodeOffset + get_property("node_count").get_int());
    }

  protected:
    int64_t internal_get_field_data(const Field &field, void *data,
                                    size_t data_size) const override;

    int64_t internal_put_field_data(const Field &field, void *data,
                                    size_t data_size) const override;

  private:
    int m_ni{};
    int m_nj{};
    int m_nk{};

    int m_offsetI{}; // Valid 'i' ordinal runs from m_offsetI+1 to m_offsetI+m_ni
    int m_offsetJ{};
    int m_offsetK{};

    int m_niGlobal{}; // The ni,nj,nk of the master block this is a subset of.
    int m_njGlobal{};
    int m_nkGlobal{};

    size_t m_nodeOffset{};
    size_t m_cellOffset{};

    size_t m_nodeGlobalOffset{};
    size_t m_cellGlobalOffset{};

    Ioss::NodeBlock m_nodeBlock;

  public:
    std::vector<ZoneConnectivity>          m_zoneConnectivity;
    std::vector<BoundaryCondition>         m_boundaryConditions;
    std::vector<size_t>                    m_blockLocalNodeIndex;
    std::vector<std::pair<size_t, size_t>> m_globalIdMap;
  };
} // namespace Ioss
#endif
