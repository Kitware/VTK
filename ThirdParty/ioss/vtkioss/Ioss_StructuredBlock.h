// Copyright(C) 1999-2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "Ioss_BoundingBox.h"
#include "Ioss_CodeTypes.h"
#include "Ioss_EntityBlock.h"
#include "Ioss_NodeBlock.h"
#include "Ioss_Property.h"
#include "Ioss_ZoneConnectivity.h"
#include <array>
#include <cassert>
#include <iosfwd>
#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>

#include "Ioss_EntityType.h"
#include "ioss_export.h"
#include "vtk_ioss_mangle.h"

namespace Ioss {
  class Region;
  class Field;

  struct IOSS_EXPORT BoundaryCondition
  {
    BoundaryCondition(std::string name, std::string fam_name, Ioss::IJK_t range_beg,
                      Ioss::IJK_t range_end)
        : m_bcName(std::move(name)), m_famName(std::move(fam_name)), m_rangeBeg(range_beg),
          m_rangeEnd(range_end)
    {
    }

    // Deprecated... Use the constructor above with both name and fam_name
    BoundaryCondition(std::string name, Ioss::IJK_t range_beg, Ioss::IJK_t range_end)
        : m_bcName(name), m_famName(std::move(name)), m_rangeBeg(range_beg), m_rangeEnd(range_end)
    {
    }

    // cereal requires a default constructor when de-serializing vectors of objects.  Because
    // StructuredBlock contains a vector of BoundaryCondition objects, this default constructor is
    // necessary.
    BoundaryCondition() = default;

    BoundaryCondition(const BoundaryCondition &copy_from)            = default;
    BoundaryCondition &operator=(const BoundaryCondition &copy_from) = default;

    // Determine which "face" of the parent block this BC is applied to.
    IOSS_NODISCARD int which_face() const;

    // Does range specify a valid face
    IOSS_NODISCARD bool is_valid() const;

    // Return number of cell faces in the BC
    IOSS_NODISCARD size_t get_face_count() const;

    IOSS_NODISCARD bool operator==(const Ioss::BoundaryCondition &rhs) const;
    IOSS_NODISCARD bool operator!=(const Ioss::BoundaryCondition &rhs) const;
    IOSS_NODISCARD bool equal(const Ioss::BoundaryCondition &rhs) const;

    std::string m_bcName{};
    std::string m_famName{};

    // These are potentially subsetted due to parallel decompositions...
    Ioss::IJK_t m_rangeBeg{};
    Ioss::IJK_t m_rangeEnd{};

    mutable int m_face{-1};

    template <class Archive> void serialize(Archive &archive)
    {
      archive(m_bcName, m_famName, m_rangeBeg, m_rangeEnd, m_face);
    }

  private:
    bool equal_(const Ioss::BoundaryCondition &rhs, bool quiet) const;
  };

  IOSS_EXPORT std::ostream &operator<<(std::ostream &os, const BoundaryCondition &bc);

  class DatabaseIO;

  /** \brief A structured zone -- i,j,k
   */
  class IOSS_EXPORT StructuredBlock : public EntityBlock
  {
  public:
    StructuredBlock(DatabaseIO *io_database, const std::string &my_name, int index_dim, int ni,
                    int nj, int nk, int off_i, int off_j, int off_k, int glo_ni, int glo_nj,
                    int glo_nk);
    StructuredBlock(DatabaseIO *io_database, const std::string &my_name, int index_dim,
                    const Ioss::IJK_t &ordinal, const Ioss::IJK_t &offset,
                    const Ioss::IJK_t &global_ordinal);

    // Useful for serial
    StructuredBlock(DatabaseIO *io_database, const std::string &my_name, int index_dim, int ni,
                    int nj, int nk);
    StructuredBlock(DatabaseIO *io_database, const std::string &my_name, int index_dim,
                    Ioss::IJK_t &ordinal);

    StructuredBlock *clone(DatabaseIO *database) const;

    IOSS_NODISCARD std::string type_string() const override { return "StructuredBlock"; }
    IOSS_NODISCARD std::string short_type_string() const override { return "structuredblock"; }
    IOSS_NODISCARD std::string contains_string() const override { return "Cell"; }
    IOSS_NODISCARD EntityType  type() const override { return STRUCTUREDBLOCK; }

    IOSS_NODISCARD const Ioss::NodeBlock &get_node_block() const { return m_nodeBlock; }
    IOSS_NODISCARD Ioss::NodeBlock &get_node_block() { return m_nodeBlock; }

    /** \brief Does block contain any cells
     */
    IOSS_NODISCARD bool is_active() const { return m_ijk[0] * m_ijk[1] * m_ijk[2] > 0; }

    // Handle implicit properties -- These are calculated from data stored
    // in the grouping entity instead of having an explicit value assigned.
    // An example would be 'element_block_count' for a region.
    IOSS_NODISCARD Property get_implicit_property(const std::string &my_name) const override;

    IOSS_NODISCARD AxisAlignedBoundingBox get_bounding_box() const;

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

    IOSS_NODISCARD size_t get_node_offset() const { return m_nodeOffset; }
    IOSS_NODISCARD size_t get_cell_offset() const { return m_cellOffset; }
    IOSS_NODISCARD size_t get_node_global_offset() const { return m_nodeGlobalOffset; }
    IOSS_NODISCARD size_t get_cell_global_offset() const { return m_cellGlobalOffset; }

    void set_ijk_offset(int axis, size_t offset);
    void set_ijk_global(int axis, size_t global);

    void set_ijk_offset(const IJK_t &offset);
    void set_ijk_global(const IJK_t &global);

    IOSS_NODISCARD IJK_t get_ijk_offset() const { return m_offset; }
    IOSS_NODISCARD IJK_t get_ijk_local() const { return m_ijk; }
    IOSS_NODISCARD IJK_t get_ijk_global() const { return m_ijkGlobal; }

    // Get the global (over all processors) cell
    // id at the specified i,j,k location (1 <= i,j,k <= ni,nj,nk).  1-based.
    IOSS_NODISCARD size_t get_global_cell_id(int i, int j, int k) const
    {
      return m_cellGlobalOffset + static_cast<size_t>(k - 1) * m_ijkGlobal[0] * m_ijkGlobal[1] +
             static_cast<size_t>(j - 1) * m_ijkGlobal[0] + i;
    }

    IOSS_NODISCARD size_t get_global_cell_id(IJK_t index) const
    {
      return get_global_cell_id(index[0], index[1], index[2]);
    }

    // Get the global (over all processors) node
    // offset at the specified i,j,k location (1 <= i,j,k <= ni,nj,nk).  0-based, does not account
    // for shared nodes.
    IOSS_NODISCARD size_t get_global_node_offset(int i, int j, int k) const
    {
      return m_nodeGlobalOffset +
             static_cast<size_t>(k - 1) * (m_ijkGlobal[0] + 1) * (m_ijkGlobal[1] + 1) +
             static_cast<size_t>(j - 1) * (m_ijkGlobal[0] + 1) + i - 1;
    }

    IOSS_NODISCARD size_t get_global_node_offset(IJK_t index) const
    {
      return get_global_node_offset(index[0], index[1], index[2]);
    }

    // Get the local (relative to this block on this processor) node id at the specified
    // i,j,k location (1 <= i,j,k <= ni+1,nj+1,nk+1).  0-based.
    IOSS_NODISCARD size_t get_block_local_node_offset(int ii, int jj, int kk) const
    {
      auto i = ii - m_offset[0];
      auto j = jj - m_offset[1];
      auto k = kk - m_offset[2];
      assert(i > 0 && i <= m_ijk[0] + 1 && j > 0 && j <= m_ijk[1] + 1 && k > 0 &&
             k <= m_ijk[2] + 1);
      return static_cast<size_t>(k - 1) * (m_ijk[0] + 1) * (m_ijk[1] + 1) +
             static_cast<size_t>(j - 1) * (m_ijk[0] + 1) + i - 1;
    }

    IOSS_NODISCARD size_t get_block_local_node_offset(IJK_t index) const
    {
      return get_block_local_node_offset(index[0], index[1], index[2]);
    }

    // Get the local (on this processor) cell-node offset at the specified
    // i,j,k location (1 <= i,j,k <= ni+1,nj+1,nk+1).  0-based.
    IOSS_NODISCARD size_t get_local_node_offset(int i, int j, int k) const
    {
      return get_block_local_node_offset(i, j, k) + m_nodeOffset;
    }

    IOSS_NODISCARD size_t get_local_node_offset(IJK_t index) const
    {
      return get_local_node_offset(index[0], index[1], index[2]);
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

      if (m_ijk[2] == 0 && m_ijk[1] == 0 && m_ijk[0] == 0) {
        return index;
      }

      for (int kk = 0; kk < m_ijk[2] + 1; kk++) {
        size_t k = m_offset[2] + kk;
        for (int jj = 0; jj < m_ijk[1] + 1; jj++) {
          size_t j = m_offset[1] + jj;
          for (int ii = 0; ii < m_ijk[0] + 1; ii++) {
            size_t i = m_offset[0] + ii;

            size_t ind =
                k * (m_ijkGlobal[0] + 1) * (m_ijkGlobal[1] + 1) + j * (m_ijkGlobal[0] + 1) + i;

            idata[index++] = ind + offset + 1;
          }
        }
      }

      for (const auto &idx_id : m_globalIdMap) {
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

      if (m_ijk[2] == 0 && m_ijk[1] == 0 && m_ijk[0] == 0) {
        return index;
      }

      for (int kk = 0; kk < m_ijk[2]; kk++) {
        size_t k = m_offset[2] + kk;
        for (int jj = 0; jj < m_ijk[1]; jj++) {
          size_t j = m_offset[1] + jj;
          for (int ii = 0; ii < m_ijk[0]; ii++) {
            size_t i = m_offset[0] + ii;

            size_t ind = k * m_ijkGlobal[0] * m_ijkGlobal[1] + j * m_ijkGlobal[0] + i;

            idata[index++] = ind + offset + 1;
          }
        }
      }
      return index;
    }

    IOSS_NODISCARD bool contains_node(size_t global_offset) const
    {
      return (global_offset >= m_nodeOffset &&
              global_offset < m_nodeOffset + get_property("node_count").get_int());
    }

    /* COMPARE two StructuredBlocks */
    IOSS_NODISCARD bool operator==(const Ioss::StructuredBlock &rhs) const;
    IOSS_NODISCARD bool operator!=(const Ioss::StructuredBlock &rhs) const;
    IOSS_NODISCARD bool equal(const Ioss::StructuredBlock &rhs) const;

  protected:
    int64_t internal_get_field_data(const Field &field, void *data,
                                    size_t data_size) const override;

    int64_t internal_put_field_data(const Field &field, void *data,
                                    size_t data_size) const override;

    int64_t internal_get_zc_field_data(const Field &field, void **data,
                                       size_t *data_size) const override;

  private:
    bool  equal_(const Ioss::StructuredBlock &rhs, bool quiet) const;
    IJK_t m_ijk;
    IJK_t m_offset;    // Valid 'i' ordinal runs from m_offset[i]+1 to m_offset[i]+m_ijk[i]
    IJK_t m_ijkGlobal; // The ni,nj,nk of the master block this is a subset of.

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

    template <class Archive> void serialize(Archive &archive)
    {
      archive(m_zoneConnectivity, m_boundaryConditions, m_blockLocalNodeIndex, m_globalIdMap);
    }
  };
} // namespace Ioss
