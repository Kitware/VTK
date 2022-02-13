/*
 * Copyright(C) 1999-2022 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
#pragma once

#include "vtk_ioss_mangle.h"

#include <vtk_exodusII.h>
#if defined PARALLEL_AWARE_EXODUS

#include <Ioss_CodeTypes.h>
#include <vector>
#if !defined(NO_PARMETIS_SUPPORT)
#include <parmetis.h>
#endif

#undef MPICPP
#if !defined(NO_ZOLTAN_SUPPORT)
#include <zoltan_cpp.h>
#endif
#include <Ioss_Decomposition.h>
#include <Ioss_Map.h>
#include <Ioss_PropertyManager.h>

namespace Ioss {
  class Field;
}
namespace Ioex {

  class DecompositionDataBase
  {
  public:
    DecompositionDataBase(MPI_Comm comm) : comm_(comm) {}

    virtual ~DecompositionDataBase()            = default;
    virtual int    int_size() const             = 0;
    virtual void   decompose_model(int filePtr) = 0;
    virtual size_t ioss_node_count() const      = 0;
    virtual size_t ioss_elem_count() const      = 0;

    virtual int    spatial_dimension() const = 0;
    virtual size_t global_node_count() const = 0;
    virtual size_t global_elem_count() const = 0;

    virtual size_t decomp_node_offset() const = 0;
    virtual size_t decomp_node_count() const  = 0;
    virtual size_t decomp_elem_offset() const = 0;
    virtual size_t decomp_elem_count() const  = 0;

    virtual std::vector<double> &centroids() = 0;

    MPI_Comm comm_;

    int m_processor{0};
    int m_processorCount{0};

    std::vector<Ioss::BlockDecompositionData> el_blocks;
    std::vector<Ioss::SetDecompositionData>   node_sets;
    std::vector<Ioss::SetDecompositionData>   side_sets;

    const Ioss::SetDecompositionData &get_decomp_set(ex_entity_type type, ex_entity_id id) const;

    template <typename T>
    void communicate_node_data(T *file_data, T *ioss_data, size_t comp_count) const;

    template <typename T>
    void communicate_element_data(T *file_data, T *ioss_data, size_t comp_count) const;

    void get_block_connectivity(int filePtr, void *data, int64_t id, size_t blk_seq,
                                size_t nnpe) const;

    void read_elem_proc_map(int filePtr, void *data) const;

    void get_node_entity_proc_data(void *entity_proc, const Ioss::MapContainer &node_map,
                                   bool do_map) const;

    int get_set_mesh_var(int filePtr, ex_entity_type type, ex_entity_id id,
                         const Ioss::Field &field, void *ioss_data) const;

    int get_set_mesh_double(int filePtr, ex_entity_type type, ex_entity_id id,
                            const Ioss::Field &field, double *ioss_data) const;

    virtual size_t get_commset_node_size() const = 0;

    virtual int get_node_coordinates(int filePtr, double *ioss_data,
                                     const Ioss::Field &field) const         = 0;
    virtual int get_one_attr(int exoid, ex_entity_type obj_type, ex_entity_id obj_id,
                             int attrib_index, double *attrib) const         = 0;
    virtual int get_attr(int exoid, ex_entity_type obj_type, ex_entity_id obj_id, size_t attr_count,
                         double *attrib) const                               = 0;
    virtual int get_var(int filePtr, int step, ex_entity_type type, int var_index, ex_entity_id id,
                        int64_t num_entity, std::vector<double> &data) const = 0;
  };

  template <typename INT> class DecompositionData : public DecompositionDataBase
  {
  public:
    DecompositionData(const Ioss::PropertyManager &props, MPI_Comm communicator);
    ~DecompositionData() = default;

    int int_size() const { return sizeof(INT); }

    void decompose_model(int filePtr);

    int spatial_dimension() const { return m_decomposition.m_spatialDimension; }

    size_t global_node_count() const { return m_decomposition.global_node_count(); }
    size_t global_elem_count() const { return m_decomposition.global_elem_count(); }

    size_t ioss_node_count() const { return m_decomposition.ioss_node_count(); }
    size_t ioss_elem_count() const { return m_decomposition.ioss_elem_count(); }

    size_t decomp_node_offset() const { return m_decomposition.file_node_offset(); }
    size_t decomp_node_count() const { return m_decomposition.file_node_count(); }
    size_t decomp_elem_offset() const { return m_decomposition.file_elem_offset(); }
    size_t decomp_elem_count() const { return m_decomposition.file_elem_count(); }

    std::vector<double> &centroids() { return m_decomposition.m_centroids; }

    template <typename T>
    void communicate_element_data(T *file_data, T *ioss_data, size_t comp_count) const
    {
      m_decomposition.communicate_element_data(file_data, ioss_data, comp_count);
    }

    template <typename T>
    void communicate_set_data(T *file_data, T *ioss_data, const Ioss::SetDecompositionData &set,
                              size_t comp_count) const
    {
      m_decomposition.communicate_set_data(file_data, ioss_data, set, comp_count);
    }

    template <typename T>
    void communicate_node_data(T *file_data, T *ioss_data, size_t comp_count) const
    {
      m_decomposition.communicate_node_data(file_data, ioss_data, comp_count);
    }

    void   get_block_connectivity(int filePtr, INT *data, int64_t id, size_t blk_seq,
                                  size_t nnpe) const;
    size_t get_commset_node_size() const { return m_decomposition.m_nodeCommMap.size() / 2; }

    int get_attr(int filePtr, ex_entity_type obj_type, ex_entity_id id, size_t attr_count,
                 double *attrib) const;
    int get_one_attr(int filePtr, ex_entity_type obj_type, ex_entity_id id, int attrib_index,
                     double *attrib) const;

    int get_var(int filePtr, int step, ex_entity_type type, int var_index, ex_entity_id id,
                int64_t num_entity, std::vector<double> &data) const;

    template <typename T>
    int get_set_mesh_var(int filePtr, ex_entity_type type, ex_entity_id id,
                         const Ioss::Field &field, T *ioss_data) const;

    size_t get_block_seq(ex_entity_type type, ex_entity_id id) const;
    size_t get_block_element_count(size_t blk_seq) const;
    size_t get_block_element_offset(size_t blk_seq) const;

    void create_implicit_global_map(const std::vector<int> &owning_proc,
                                    std::vector<int64_t> &global_implicit_map, Ioss::Map &node_map,
                                    int64_t *locally_owned_count, int64_t *processor_offset);

  private:
#if !defined(NO_ZOLTAN_SUPPORT)
    void zoltan_decompose(const std::string &method);
#endif

#if !defined(NO_PARMETIS_SUPPORT)
    void metis_decompose(const std::string &method, const std::vector<INT> &element_dist);

    void internal_metis_decompose(const std::string &method, idx_t *element_dist, idx_t *pointer,
                                  idx_t *adjacency, idx_t *elem_partition);
#endif

    void simple_decompose(const std::string &method, const std::vector<INT> &element_dist)
    {
      m_decomposition.simple_decompose(method, element_dist);
    }

    void simple_node_decompose(const std::string &method, const std::vector<INT> &node_dist);

    template <typename T>
    int handle_sset_df(int filePtr, ex_entity_id id, const Ioss::Field &field, T *ioss_data) const;

    int get_one_set_attr(int filePtr, ex_entity_type type, ex_entity_id id, int attr_index,
                         double *ioss_data) const;
    int get_one_node_attr(int filePtr, ex_entity_id id, int attr_index, double *ioss_data) const;
    int get_one_elem_attr(int filePtr, ex_entity_id id, int attr_index, double *ioss_data) const;

    int get_set_attr(int filePtr, ex_entity_type type, ex_entity_id id, size_t comp_count,
                     double *ioss_data) const;
    int get_node_attr(int filePtr, ex_entity_id id, size_t comp_count, double *ioss_data) const;
    int get_elem_attr(int filePtr, ex_entity_id id, size_t comp_count, double *ioss_data) const;

    int get_node_var(int filePtr, int step, int var_index, ex_entity_id id, int64_t num_entity,
                     std::vector<double> &ioss_data) const;

    int get_elem_var(int filePtr, int step, int var_index, ex_entity_id id, int64_t num_entity,
                     std::vector<double> &ioss_data) const;

    int get_set_var(int filePtr, int step, int var_index, ex_entity_type type, ex_entity_id id,
                    int64_t num_entity, std::vector<double> &ioss_data) const;

    bool i_own_node(size_t node)
        const // T/F if node with global index node owned by this processors ioss-decomp.
    {
      return m_decomposition.i_own_node(node);
    }

    bool i_own_elem(size_t elem)
        const // T/F if node with global index elem owned by this processors ioss-decomp.
    {
      return m_decomposition.i_own_elem(elem);
    }

    // global_index is 1-based index into global list of nodes [1..global_node_count]
    // return value is 1-based index into local list of nodes on this
    // processor (ioss-decomposition)
    size_t node_global_to_local(size_t global_index) const
    {
      return m_decomposition.node_global_to_local(global_index);
    }

    size_t elem_global_to_local(size_t global_index) const
    {
      return m_decomposition.elem_global_to_local(global_index);
    }

    void build_global_to_local_elem_map()
    {
      return m_decomposition.build_global_to_local_elem_map();
    }

    void get_element_block_communication()
    {
      m_decomposition.get_element_block_communication(el_blocks);
    }

    void generate_adjacency_list(int filePtr, Ioss::Decomposition<INT> &decomposition);

    void get_common_set_data(int filePtr, ex_entity_type set_type,
                             std::vector<Ioss::SetDecompositionData> &sets,
                             const std::string                       &set_type_name);

    void get_nodeset_data(int filePtr, size_t set_count);

    void get_sideset_data(int filePtr, size_t set_count);

    void calculate_element_centroids(int filePtr, const std::vector<INT> &node_dist);
#if !defined(NO_ZOLTAN_SUPPORT)
    void get_local_element_list(const ZOLTAN_ID_PTR &export_global_ids, size_t export_count);
#endif

    void get_shared_node_list() { m_decomposition.get_shared_node_list(); }

    int get_node_coordinates(int filePtr, double *ioss_data, const Ioss::Field &field) const;

    void get_local_node_list() { m_decomposition.get_local_node_list(); }

  public:
    Ioss::Decomposition<INT> m_decomposition;
  };
} // namespace Ioex
#endif
