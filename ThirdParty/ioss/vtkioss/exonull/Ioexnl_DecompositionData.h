/*
 * Copyright(C) 1999-2024 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
#pragma once

#include <vtk_exodusII.h>
#if defined PARALLEL_AWARE_EXODUS

#include "Ioss_CodeTypes.h"
#include <vector>

#include "ioexnl_export.h"
#include "vtk_ioss_mangle.h"
#if !defined(NO_PARMETIS_SUPPORT)
#include <parmetis.h>
#endif

#undef MPICPP
#if !defined(NO_ZOLTAN_SUPPORT)
#include <zoltan_cpp.h>
#endif
#include "Ioss_Decomposition.h"
#include "Ioss_Map.h"
#include "Ioss_PropertyManager.h"

namespace Ioss {
  class Field;
}
namespace Ioexnl {

  class IOEXNL_EXPORT DecompositionDataBase
  {
  public:
    DecompositionDataBase(Ioss_MPI_Comm comm) : comm_(comm) {}
    DecompositionDataBase(const DecompositionDataBase &)            = delete;
    DecompositionDataBase &operator=(const DecompositionDataBase &) = delete;

    virtual ~DecompositionDataBase()                      = default;
    IOSS_NODISCARD virtual int    int_size() const        = 0;
    IOSS_NODISCARD virtual size_t ioss_node_count() const = 0;
    IOSS_NODISCARD virtual size_t ioss_elem_count() const = 0;

    IOSS_NODISCARD virtual int    spatial_dimension() const = 0;
    IOSS_NODISCARD virtual size_t global_node_count() const = 0;
    IOSS_NODISCARD virtual size_t global_elem_count() const = 0;

    IOSS_NODISCARD virtual size_t decomp_node_offset() const = 0;
    IOSS_NODISCARD virtual size_t decomp_node_count() const  = 0;
    IOSS_NODISCARD virtual size_t decomp_elem_offset() const = 0;
    IOSS_NODISCARD virtual size_t decomp_elem_count() const  = 0;

    Ioss_MPI_Comm comm_;

    int m_processor{0};
    int m_processorCount{0};

    std::vector<Ioss::BlockDecompositionData> el_blocks;
    std::vector<Ioss::SetDecompositionData>   node_sets;
    std::vector<Ioss::SetDecompositionData>   side_sets;
  };

  template <typename INT> class DecompositionData : public DecompositionDataBase
  {
  public:
    DecompositionData(const Ioss::PropertyManager &props, Ioss_MPI_Comm communicator);
    ~DecompositionData() = default;

    IOSS_NODISCARD int int_size() const { return sizeof(INT); }

    IOSS_NODISCARD int spatial_dimension() const { return m_decomposition.m_spatialDimension; }

    IOSS_NODISCARD size_t global_node_count() const { return m_decomposition.global_node_count(); }
    IOSS_NODISCARD size_t global_elem_count() const { return m_decomposition.global_elem_count(); }

    IOSS_NODISCARD size_t ioss_node_count() const { return m_decomposition.ioss_node_count(); }
    IOSS_NODISCARD size_t ioss_elem_count() const { return m_decomposition.ioss_elem_count(); }

    IOSS_NODISCARD size_t decomp_node_offset() const { return m_decomposition.file_node_offset(); }
    IOSS_NODISCARD size_t decomp_node_count() const { return m_decomposition.file_node_count(); }
    IOSS_NODISCARD size_t decomp_elem_offset() const { return m_decomposition.file_elem_offset(); }
    IOSS_NODISCARD size_t decomp_elem_count() const { return m_decomposition.file_elem_count(); }

    void create_implicit_global_map(const std::vector<int> &owning_proc,
                                    std::vector<int64_t> &global_implicit_map, Ioss::Map &node_map,
                                    int64_t *locally_owned_count, int64_t *processor_offset);

  private:
    void simple_decompose(const std::string &method, const std::vector<INT> &element_dist)
    {
      m_decomposition.simple_decompose(method, element_dist);
    }

    void simple_node_decompose(const std::string &method, const std::vector<INT> &node_dist);

    template <typename T>
    int handle_sset_df(int filePtr, ex_entity_id id, const Ioss::Field &field, T *ioss_data) const;

    IOSS_NODISCARD bool i_own_node(size_t node) const
    {
      // T/F if the node with global index `node` is owned by this processors ioss-decomp.
      return m_decomposition.i_own_node(node);
    }

    IOSS_NODISCARD bool i_own_elem(size_t elem) const
    // T/F if the element with global index `elem` is owned by this processors ioss-decomp.
    {
      return m_decomposition.i_own_elem(elem);
    }

    // global_index is 1-based index into global list of nodes [1..global_node_count]
    // return value is 1-based index into local list of nodes on this
    // processor (ioss-decomposition)
    IOSS_NODISCARD size_t node_global_to_local(size_t global_index) const
    {
      return m_decomposition.node_global_to_local(global_index);
    }

    IOSS_NODISCARD size_t elem_global_to_local(size_t global_index) const
    {
      return m_decomposition.elem_global_to_local(global_index);
    }

    void build_global_to_local_elem_map()
    {
      return m_decomposition.build_global_to_local_elem_map();
    }

    void get_shared_node_list() { m_decomposition.get_shared_node_list(); }

    void get_local_node_list() { m_decomposition.get_local_node_list(); }

  public:
    Ioss::Decomposition<INT> m_decomposition;
  };
} // namespace Ioexnl
#endif
