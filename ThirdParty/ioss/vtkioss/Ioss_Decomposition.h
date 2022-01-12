/*
 * Copyright(C) 1999-2022 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
#pragma once

#include "vtk_ioss_mangle.h"

#include <Ioss_CodeTypes.h>
#include <Ioss_Map.h>
#include <Ioss_ParallelUtils.h>
#include <Ioss_PropertyManager.h>
#include <algorithm>
#include <cassert>
#include <map>
#include <string>
#include <vector>

#if !defined(NO_PARMETIS_SUPPORT)
#include <parmetis.h>
#endif

#if !defined(NO_ZOLTAN_SUPPORT)
#undef MPICPP
#include <zoltan_cpp.h>
#endif

#define DC_USE_HOPSCOTCH
#if defined DC_USE_HOPSCOTCH
#include <hopscotch_map.h>
#elif defined DC_USE_ROBIN
#include <robin_map.h>
#endif

namespace Ioss {
  const std::vector<std::string> &valid_decomp_methods();

  class BlockDecompositionData
  {
  public:
    BlockDecompositionData() = default;

    const std::string &name() const { return name_; }
    int                zone() const { return zone_; }
    int                section() const { return section_; }
    int64_t            id() const { return id_; }
    size_t             file_count() const { return fileCount; }
    size_t             ioss_count() const { return iossCount; }
    size_t             global_count() const { return globalCount; }

    std::string name_{};
    int         zone_{0};
    int         section_{0};

    size_t  fileSectionOffset{0}; // In partial read, where start
    int64_t id_{0};
    size_t  fileCount{0};
    size_t  iossCount{0};
    size_t  globalCount{0};

    size_t      zoneNodeOffset{0};
    std::string topologyType{"unknown"};
    int         nodesPerEntity{0};
    int         attributeCount{0};

    // maps from file-block data to ioss-block data
    // The local_map.size() elements starting at localIossOffset are local.
    // ioss[localIossOffset+i] = file[local_map[i]];
    size_t           localIossOffset{0};
    std::vector<int> localMap;

    // Maps from file-block data to export list.
    // export[i] = file[export_map[i]
    std::vector<int> exportMap;
    std::vector<int> exportCount;
    std::vector<int> exportIndex;

    // Maps from import data to ioss-block data.
    // ioss[import_map[i] = local_map[i];
    std::vector<int> importMap;
    std::vector<int> importCount;
    std::vector<int> importIndex;
  };

  class SetDecompositionData
  {
  public:
    SetDecompositionData()                             = default;
    SetDecompositionData(const SetDecompositionData &) = delete;
    SetDecompositionData(SetDecompositionData &&)      = default;

    ~SetDecompositionData()
    {
      if (setComm_ != MPI_COMM_NULL) {
        MPI_Comm_free(&setComm_);
      }
    }

    const std::string &name() const { return name_; }
    const std::string &ss_name() const { return ss_name_.empty() ? name_ : ss_name_; }
    int64_t            id() const { return id_; }
    int                zone() const { return zone_; }
    int                section() const { return section_; }
    size_t             file_count() const { return fileCount; }
    size_t             ioss_count() const { return entitylist_map.size(); }
    size_t             df_count() const { return distributionFactorCount; }

    // contains global entity-list positions for all entities in this set on this processor.
    std::vector<size_t> entitylist_map;
    std::vector<bool>   hasEntities; // T/F if this set exists on processor p

    std::string name_{};
    std::string ss_name_{};
    int64_t     id_{0};
    int         zone_{0}; // Zone of the element block that this set is on
    int         section_{0};
    size_t      fileCount{0}; // Number of nodes in nodelist for file decomposition
    int         root_{0};     // Lowest number processor that has nodes for this nodest
    std::string topologyType{};
    size_t      parentBlockIndex{0};

    int distributionFactorValsPerEntity{-1}; // number of df / element or node. -1 if nonconstant.
    size_t distributionFactorCount{0};
    double distributionFactorValue{
        0.0}; // If distributionFactorConstant == true, the constant value
    MPI_Comm setComm_{MPI_COMM_NULL};
    bool     distributionFactorConstant{false}; // T if all distribution factors the same value.
  };

  template <typename INT> class Decomposition
  {
  public:
    Decomposition(const Ioss::PropertyManager &props, MPI_Comm comm);

    size_t global_node_count() const { return m_globalNodeCount; }
    size_t global_elem_count() const { return m_globalElementCount; }
    size_t ioss_node_count() const { return nodeGTL.size(); }
    size_t ioss_elem_count() const { return localElementMap.size() + importElementMap.size(); }
    size_t file_node_count() const { return m_nodeCount; }
    size_t file_elem_count() const { return m_elementCount; }
    size_t file_node_offset() const { return m_nodeOffset; }
    size_t file_elem_offset() const { return m_elementOffset; }

    bool needs_centroids() const
    {
      return (m_method == "RCB" || m_method == "RIB" || m_method == "HSFC" ||
              m_method == "GEOM_KWAY" || m_method == "KWAY_GEOM" || m_method == "METIS_SFC");
    }

    void generate_entity_distributions(size_t globalNodeCount, size_t globalElementCount);

    // T/F if node with global index node owned by this processors ioss-decomp.
    bool i_own_node(size_t global_index) const
    {
      // global_index is 1-based index into global list of nodes [1..global_node_count]
      return std::binary_search(nodeGTL.begin(), nodeGTL.end(), global_index);
    }

    // T/F if element with global index elem owned by this processors ioss-decomp.
    bool i_own_elem(size_t global_index) const
    {
      // global_index is 1-based index into global list of elements [1..global_element_count]
      return elemGTL.find(global_index) != elemGTL.end();
    }

    size_t node_global_to_local(size_t global_index) const
    {
      // global_index is 1-based index into global list of nodes [1..global_node_count]
      // return value is 1-based index into local list of nodes on this
      // processor (ioss-decomposition)
      // Note that for 'int', equivalence and equality are the same, so
      // lower_bound is OK here (EffectiveSTL, Item 19)
      typename std::vector<INT>::const_iterator I =
          lower_bound(nodeGTL.begin(), nodeGTL.end(), global_index);
      assert(I != nodeGTL.end());
      return std::distance(nodeGTL.begin(), I) + 1; // Convert to 1-based index.
    }

    size_t elem_global_to_local(size_t global_index) const
    {
      // global_index is 1-based index into global list of elements [1..global_node_count]
      // return value is 1-based index into local list of elements on this
      // processor (ioss-decomposition)
      auto I = elemGTL.find(global_index);
      assert(I != elemGTL.end());
      return I->second;
    }

    void show_progress(const std::string &message) const
    {
      if (m_showProgress) {
        Ioss::ParallelUtils pu(m_comm);
        pu.progress(message);
      }
    }

    // Zero out some large arrays usually not needed after decomposition
    void release_memory();
    void decompose_model(
#if !defined(NO_ZOLTAN_SUPPORT)
        Zoltan &zz,
#endif
        std::vector<BlockDecompositionData> &element_blocks);

    void simple_decompose();
    void simple_node_decompose();
    void guided_decompose();

    void calculate_element_centroids(const std::vector<double> &x, const std::vector<double> &y,
                                     const std::vector<double> &z);

#if !defined(NO_ZOLTAN_SUPPORT)
    void zoltan_decompose(Zoltan &zz);

    void get_local_element_list(const ZOLTAN_ID_PTR &export_global_ids, size_t export_count);
#endif

#if !defined(NO_PARMETIS_SUPPORT)
    void metis_decompose(idx_t *pointer, idx_t *adjacency,
                         std::vector<BlockDecompositionData> &el_blocks);

    void internal_metis_decompose(std::vector<BlockDecompositionData> &el_blocks,
                                  idx_t *element_dist, idx_t *pointer, idx_t *adjacency,
                                  idx_t *elem_partition);
#endif

    void get_node_entity_proc_data(INT *entity_proc, const Ioss::MapContainer &node_map,
                                   bool do_map) const
    {
      show_progress(__func__);
      size_t j = 0;
      if (do_map) {
        for (size_t i = 0; i < m_nodeCommMap.size(); i += 2) {
          INT local_id     = m_nodeCommMap[i];
          entity_proc[j++] = node_map[local_id];
          entity_proc[j++] = m_nodeCommMap[i + 1];
        }
      }
      else {
        for (size_t i = 0; i < m_nodeCommMap.size(); i += 2) {
          entity_proc[j++] = m_nodeCommMap[i + 0];
          entity_proc[j++] = m_nodeCommMap[i + 1];
        }
      }
    }

    void get_element_block_communication(std::vector<BlockDecompositionData> &el_blocks);
    void build_global_to_local_elem_map();
    void get_local_node_list();
    void get_shared_node_list();

    // The following function is used if reading all element data on a
    // processor instead of just an element blocks worth...
    template <typename T>
    void communicate_element_data(T *file_data, T *ioss_data, size_t comp_count) const
    {
      show_progress(__func__);
      if (m_method == "LINEAR") {
        assert(m_importPreLocalElemIndex == 0);
        assert(exportElementMap.size() == 0);
        assert(importElementMap.size() == 0);
        // For "LINEAR" decomposition method, the `file_data` is the
        // same as `ioss_data` Transfer all local data from file_data
        // to ioss_data...
        auto size = localElementMap.size() * comp_count;
        std::copy(file_data, file_data + size, ioss_data);
        return;
      }

      // Transfer the file-decomposition based data in 'file_data' to
      // the ioss-decomposition based data in 'ioss_data'
      std::vector<T> export_data(exportElementMap.size() * comp_count);
      std::vector<T> import_data(importElementMap.size() * comp_count);

      if (comp_count == 1) {
        for (size_t i = 0; i < exportElementMap.size(); i++) {
          size_t index   = exportElementMap[i] - m_elementOffset;
          export_data[i] = file_data[index];
        }

        // Transfer all local data from file_data to ioss_data...
        for (size_t i = 0; i < localElementMap.size(); i++) {
          size_t index                             = localElementMap[i];
          ioss_data[m_importPreLocalElemIndex + i] = file_data[index];
        }

        // Get my imported data and send my exported data...
        Ioss::MY_Alltoallv(export_data, exportElementCount, exportElementIndex, import_data,
                           importElementCount, importElementIndex, m_comm);
        show_progress("\tCommunication 1a finished");

        // Copy the imported data into ioss_data...
        // Some comes before the local data...
        for (size_t i = 0; i < m_importPreLocalElemIndex; i++) {
          ioss_data[i] = import_data[i];
        }

        // Some comes after the local data...
        size_t offset = m_importPreLocalElemIndex + localElementMap.size();
        for (size_t i = 0; i < importElementMap.size() - m_importPreLocalElemIndex; i++) {
          ioss_data[offset + i] = import_data[m_importPreLocalElemIndex + i];
        }
      }
      else {
        for (size_t i = 0; i < exportElementMap.size(); i++) {
          size_t index = exportElementMap[i] - m_elementOffset;
          for (size_t j = 0; j < comp_count; j++) {
            export_data[comp_count * i + j] = file_data[comp_count * index + j];
          }
        }

        // Transfer all local data from file_data to ioss_data...
        for (size_t i = 0; i < localElementMap.size(); i++) {
          size_t index = localElementMap[i];
          for (size_t j = 0; j < comp_count; j++) {
            ioss_data[comp_count * (m_importPreLocalElemIndex + i) + j] =
                file_data[comp_count * index + j];
          }
        }

        std::vector<INT> export_count(exportElementCount.begin(), exportElementCount.end());
        std::vector<INT> export_disp(exportElementIndex.begin(), exportElementIndex.end());
        std::vector<INT> import_count(importElementCount.begin(), importElementCount.end());
        std::vector<INT> import_disp(importElementIndex.begin(), importElementIndex.end());

        for (int i = 0; i < m_processorCount; i++) {
          export_count[i] *= comp_count;
          export_disp[i] *= comp_count;
          import_count[i] *= comp_count;
          import_disp[i] *= comp_count;
        }

        // Get my imported data and send my exported data...
        Ioss::MY_Alltoallv(export_data, export_count, export_disp, import_data, import_count,
                           import_disp, m_comm);
        show_progress("\tCommunication 1b finished");

        // Copy the imported data into ioss_data...
        // Some comes before the local data...
        for (size_t i = 0; i < m_importPreLocalElemIndex; i++) {
          for (size_t j = 0; j < comp_count; j++) {
            ioss_data[comp_count * i + j] = import_data[comp_count * i + j];
          }
        }

        // Some comes after the local data...
        size_t offset = m_importPreLocalElemIndex + localElementMap.size();
        for (size_t i = 0; i < importElementMap.size() - m_importPreLocalElemIndex; i++) {
          for (size_t j = 0; j < comp_count; j++) {
            ioss_data[comp_count * (offset + i) + j] =
                import_data[comp_count * (m_importPreLocalElemIndex + i) + j];
          }
        }
      }
    }

    template <typename T>
    void communicate_set_data(T *file_data, T *ioss_data, const SetDecompositionData &set,
                              size_t comp_count) const
    {
      show_progress(__func__);
      std::vector<T> recv_data;

      size_t size = set.file_count() * comp_count;
      if (size == 0)
        return;

      if (set.setComm_ != MPI_COMM_NULL) {
        recv_data.resize(size);
        if (m_processor == set.root_) {
          std::copy(file_data, file_data + size, recv_data.begin());
        }
        // NOTE: This broadcast uses a split communicator, so possibly
        // not all processors participating.
        MPI_Bcast(recv_data.data(), size, Ioss::mpi_type(T(0)), 0, set.setComm_);
      }
      if (comp_count == 1) {
        if (set.root_ == m_processor) {
          for (size_t i = 0; i < set.ioss_count(); i++) {
            size_t index = set.entitylist_map[i];
            ioss_data[i] = file_data[index];
          }
        }
        else {
          // Receiving data from root...
          for (size_t i = 0; i < set.ioss_count(); i++) {
            size_t index = set.entitylist_map[i];
            ioss_data[i] = recv_data[index];
          }
        }
      }
      else {
        if (set.root_ == m_processor) {
          for (size_t i = 0; i < set.ioss_count(); i++) {
            size_t index = set.entitylist_map[i];
            for (size_t j = 0; j < comp_count; j++) {
              ioss_data[comp_count * i + j] = file_data[comp_count * index + j];
            }
          }
        }
        else {
          // Receiving data from root...
          for (size_t i = 0; i < set.ioss_count(); i++) {
            size_t index = set.entitylist_map[i];
            for (size_t j = 0; j < comp_count; j++) {
              ioss_data[comp_count * i + j] = recv_data[comp_count * index + j];
            }
          }
        }
      }
    }

    template <typename T, typename U>
    void communicate_block_data(T *file_data, U *ioss_data, const BlockDecompositionData &block,
                                size_t comp_count) const
    {
      show_progress(__func__);
      if (m_method == "LINEAR") {
        assert(block.localIossOffset == 0);
        assert(block.exportMap.size() == 0);
        assert(block.importMap.size() == 0);
        // For "LINEAR" decomposition method, the `file_data` is the
        // same as `ioss_data` Transfer all local data from file_data
        // to ioss_data...
        auto size = block.localMap.size() * comp_count;
        std::copy(file_data, file_data + size, ioss_data);
        return;
      }

      std::vector<U> exports;
      exports.reserve(comp_count * block.exportMap.size());
      std::vector<U> imports(comp_count * block.importMap.size());

      if (comp_count == 1) {
        for (int i : block.exportMap) {
          exports.push_back(file_data[i]);
        }

        // Get my imported data and send my exported data...
        Ioss::MY_Alltoallv(exports, block.exportCount, block.exportIndex, imports,
                           block.importCount, block.importIndex, m_comm);

        // Map local and imported data to ioss_data.
        for (size_t i = 0; i < block.localMap.size(); i++) {
          ioss_data[i + block.localIossOffset] = file_data[block.localMap[i]];
        }

        for (size_t i = 0; i < block.importMap.size(); i++) {
          ioss_data[block.importMap[i]] = imports[i];
        }
      }
      else {
        for (int i : block.exportMap) {
          for (size_t j = 0; j < comp_count; j++) {
            exports.push_back(file_data[i * comp_count + j]);
          }
        }

        std::vector<int> export_count(block.exportCount.begin(), block.exportCount.end());
        std::vector<int> export_disp(block.exportIndex.begin(), block.exportIndex.end());
        std::vector<int> import_count(block.importCount.begin(), block.importCount.end());
        std::vector<int> import_disp(block.importIndex.begin(), block.importIndex.end());

        for (int i = 0; i < m_processorCount; i++) {
          export_count[i] *= comp_count;
          export_disp[i] *= comp_count;
          import_count[i] *= comp_count;
          import_disp[i] *= comp_count;
        }

        // Get my imported data and send my exported data...
        Ioss::MY_Alltoallv(exports, export_count, export_disp, imports, import_count, import_disp,
                           m_comm);
        show_progress("\tCommunication 1 finished");

        // Map local and imported data to ioss_data.
        for (size_t i = 0; i < block.localMap.size(); i++) {
          for (size_t j = 0; j < comp_count; j++) {
            ioss_data[(i + block.localIossOffset) * comp_count + j] =
                file_data[block.localMap[i] * comp_count + j];
          }
        }

        for (size_t i = 0; i < block.importMap.size(); i++) {
          for (size_t j = 0; j < comp_count; j++) {
            ioss_data[block.importMap[i] * comp_count + j] = imports[i * comp_count + j];
          }
        }
      }
    }

    template <typename T>
    void communicate_node_data(T *file_data, T *ioss_data, size_t comp_count) const
    {
      show_progress(__func__);
      // Transfer the file-decomposition based data in 'file_data' to
      // the ioss-decomposition based data in 'ioss_data'
      std::vector<T> export_data(exportNodeMap.size() * comp_count);
      std::vector<T> import_data(importNodeMap.size() * comp_count);

      if (comp_count == 1) {
        for (size_t i = 0; i < exportNodeMap.size(); i++) {
          size_t index = exportNodeMap[i] - m_nodeOffset;
          assert(index < m_nodeCount);
          export_data[i] = file_data[index];
        }

        // Transfer all local data from file_data to ioss_data...
        for (size_t i = 0; i < localNodeMap.size(); i++) {
          size_t index = localNodeMap[i] - m_nodeOffset;
          assert(index < m_nodeCount);
          ioss_data[m_importPreLocalNodeIndex + i] = file_data[index];
        }

        // Get my imported data and send my exported data...
        Ioss::MY_Alltoallv(export_data, exportNodeCount, exportNodeIndex, import_data,
                           importNodeCount, importNodeIndex, m_comm);
        show_progress("\tCommunication 1a finished");

        // Copy the imported data into ioss_data...
        for (size_t i = 0; i < importNodeMap.size(); i++) {
          size_t index = importNodeMap[i];
          assert(index < ioss_node_count());
          ioss_data[index] = import_data[i];
        }
      }
      else { // Comp_count > 1
        for (size_t i = 0; i < exportNodeMap.size(); i++) {
          size_t index = exportNodeMap[i] - m_nodeOffset;
          assert(index < m_nodeCount);
          for (size_t j = 0; j < comp_count; j++) {
            export_data[comp_count * i + j] = file_data[comp_count * index + j];
          }
        }

        // Transfer all local data from file_data to ioss_data...
        for (size_t i = 0; i < localNodeMap.size(); i++) {
          size_t index = localNodeMap[i] - m_nodeOffset;
          assert(index < m_nodeCount);
          for (size_t j = 0; j < comp_count; j++) {
            ioss_data[comp_count * (m_importPreLocalNodeIndex + i) + j] =
                file_data[comp_count * index + j];
          }
        }

        std::vector<INT> export_count(exportNodeCount.begin(), exportNodeCount.end());
        std::vector<INT> export_disp(exportNodeIndex.begin(), exportNodeIndex.end());
        std::vector<INT> import_count(importNodeCount.begin(), importNodeCount.end());
        std::vector<INT> import_disp(importNodeIndex.begin(), importNodeIndex.end());

        for (int i = 0; i < m_processorCount; i++) {
          export_count[i] *= comp_count;
          export_disp[i] *= comp_count;
          import_count[i] *= comp_count;
          import_disp[i] *= comp_count;
        }

        // Get my imported data and send my exported data...
        Ioss::MY_Alltoallv(export_data, export_count, export_disp, import_data, import_count,
                           import_disp, m_comm);
        show_progress("\tCommunication 1b finished");

        // Copy the imported data into ioss_data...
        for (size_t i = 0; i < importNodeMap.size(); i++) {
          size_t index = importNodeMap[i];
          assert(index < ioss_node_count());
          for (size_t j = 0; j < comp_count; j++) {
            ioss_data[comp_count * index + j] = import_data[comp_count * i + j];
          }
        }
      }
    }

    MPI_Comm            m_comm;
    Ioss::ParallelUtils m_pu;
    int                 m_processor{};
    int                 m_processorCount{};
    std::string         m_method{};
    std::string         m_decompExtra{};

    // Values for the file decomposition
    int    m_spatialDimension{3};
    int    m_commonNodeCount{0};
    size_t m_globalElementCount{0};
    size_t m_elementCount{0};
    size_t m_elementOffset{0};
    size_t m_importPreLocalElemIndex{0};

    size_t m_globalNodeCount{0};
    size_t m_nodeCount{0};
    size_t m_nodeOffset{0};
    size_t m_importPreLocalNodeIndex{0};

    bool m_retainFreeNodes{true};
    bool m_showProgress{false};
    bool m_showHWM{false};

    std::vector<INT>    m_elementToProc; // Used by "MAP" scheme...
    std::vector<double> m_centroids;
    std::vector<INT>    m_pointer;   // Index into adjacency, processor list for each element...
    std::vector<INT>    m_adjacency; // Size is sum of element connectivity sizes

    std::vector<INT> m_nodeCommMap; // node/processor pair of the
    // nodes I communicate with.  Stored node#,proc,node#,proc, ...

    // The global element at index 'I' (0-based) is on block B in the
    // file decomposition.
    // if m_fileBlockIndex[B] <= I && m_fileBlockIndex[B+1] < I
    std::vector<size_t> m_fileBlockIndex;

  private:
    // This processor "manages" the elements on the exodus mesh file from
    // m_elementOffset to m_elementOffset+m_elementCount (0-based). This is
    // 'file' data
    //
    // This processor also appears to the Ioss clients to own other
    // element and node data based on the decomposition.  This is the
    // 'ioss' data.
    //
    // The indices in `localElementMap` are the elements that are
    // common to both the 'file' data and the 'ioss' data.
    // `localElementMap[i]` contains the location in 'file' data for
    // the 'ioss' data at location `i+m_importPreLocalElemIndex`
    //
    // local_element_map[i]+m_elementOffset is the 0-based global index
    //
    // The indices in 'import_element_map' map the data received via
    // mpi communication from other processors into 'ioss' data.
    // if 'ind=import_element_map[i]', then ioss[ind] = comm_recv[i]
    // Note that this is the reverse direction of the
    // local_element_map mapping.
    //
    // The indices in 'export_element_map' are used to pull from
    // 'file' data into the comm_send vector.  if 'ind =
    // export_element_map[i]', then 'comm_send[i] = file[ind]' for i =
    // 0..#exported_elements
    //
    // local_element_map.size() + import_element_map.size() ==
    // #ioss elements on this processor.
    //
    // local_element_map.size() + export_element_map.size() ==
    // #file elements on this processor.
    //
    // export_element_map and import_element_map are sorted.
    // The primary key is processor order followed by global id.
    // The processor association is via 'export_proc_disp' and
    // 'import_proc_disp' Both are of size '#processors+1' and
    // the elements for processor p range from [X_proc_disp[p] to
    // X_proc_disp[p+1])

    std::vector<INT> localElementMap;

    std::vector<INT> importElementMap;
    std::vector<INT> importElementCount;
    std::vector<INT> importElementIndex;

    // The list of my `file decomp` elements that will be exported to some other rank.
    std::vector<INT> exportElementMap;
    // The number of elements that I will export to each other rank.
    std::vector<INT> exportElementCount;
    // The index into `exportElementMap` for the elements that will be exported to each rank.
    std::vector<INT> exportElementIndex;

    std::vector<INT> nodeIndex;

    std::vector<INT> exportNodeMap;
    std::vector<INT> exportNodeCount;
    std::vector<INT> exportNodeIndex;

    // Where to put each imported nodes data in the list of all data...
    std::vector<INT> importNodeMap;
    std::vector<INT> importNodeCount;
    std::vector<INT> importNodeIndex;

    std::vector<INT> localNodeMap;

    std::vector<INT> m_elementDist;
    std::vector<INT> m_nodeDist;

    // Note that nodeGTL is a sorted vector.
    std::vector<INT> nodeGTL; // Convert from global index to local index (1-based)

#if defined DC_USE_HOPSCOTCH
    tsl::hopscotch_pg_map<INT, INT> elemGTL; // Convert from global index to local index (1-based)
#elif defined DC_USE_ROBIN
    tsl::robin_pg_map<INT, INT> elemGTL; // Convert from global index to local index (1-based)
#else
    // This is the original method that was used in IOSS prior to using hopscotch or robin map.
    std::map<INT, INT> elemGTL; // Convert from global index to local index (1-based)
#endif
  };
} // namespace Ioss
