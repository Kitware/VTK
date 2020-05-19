/*
 * Copyright(C) 1999-2017, 2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of NTESS nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <Ioss_Decomposition.h>
#include <Ioss_ElementTopology.h>
#include <Ioss_ParallelUtils.h>
#include <Ioss_Sort.h>
#include <Ioss_Utils.h>
#include <algorithm>
#include <cassert>
#include <fmt/ostream.h>
#include <numeric>

#if !defined(NO_ZOLTAN_SUPPORT)
extern "C" int Zoltan_get_global_id_type(char **name);
#endif

#if defined(SEACAS_HAVE_CGNS)
#include <cgns/Iocgns_IOFactory.h>
#endif

namespace {
  template <typename INT>
  inline std::vector<INT> get_entity_dist(size_t proc_count, size_t my_proc, size_t entity_count,
                                          size_t *offset, size_t *count)
  {
    std::vector<INT> dist(proc_count + 1);

    size_t per_proc = entity_count / proc_count;
    size_t extra    = entity_count % proc_count;

    *count = per_proc + (my_proc < extra ? 1 : 0);

    if (my_proc < extra) {
      *offset = (per_proc + 1) * my_proc;
    }
    else {
      *offset = (per_proc + 1) * extra + per_proc * (my_proc - extra);
    }

    // This processors range of elements is
    // [element_offset..element_offset+element_count)

    // Fill in element_dist vector.  Range of elements on each processor...
    size_t sum = 0;
    for (size_t i = 0; i < proc_count; i++) {
      dist[i] = sum;
      sum += per_proc;
      if (i < extra) {
        sum++;
      }
    }
    dist[proc_count] = sum;
    return dist;
  }

  bool check_valid_decomp_method(const std::string &method) 
  {
    const auto& valid_methods = Ioss::valid_decomp_methods();
    if (std::find(valid_methods.begin(), valid_methods.end(), method) != valid_methods.end()) {
      return true;
    }
    return false;
  }

  std::string get_decomposition_method(const Ioss::PropertyManager &properties, int my_processor)
  {
    std::string method = "LINEAR";

    if (properties.exists("DECOMPOSITION_METHOD")) {
      method = properties.get("DECOMPOSITION_METHOD").get_string();
      method = Ioss::Utils::uppercase(method);
    }
    else if (properties.exists("RESTART_DECOMPOSITION_METHOD")) {
      method = properties.get("RESTART_DECOMPOSITION_METHOD").get_string();
      method = Ioss::Utils::uppercase(method);
    }
    else if (properties.exists("MODEL_DECOMPOSITION_METHOD")) {
      method = properties.get("MODEL_DECOMPOSITION_METHOD").get_string();
      method = Ioss::Utils::uppercase(method);
    }

    if (!check_valid_decomp_method(method)) {
      std::ostringstream errmsg;
      fmt::print(errmsg,
                 "ERROR: Invalid decomposition method specified: '{}'\n"
                 "       Valid methods: {}\n", method, fmt::join(Ioss::valid_decomp_methods(), ", "));
      IOSS_ERROR(errmsg);
    }
    return method;
  }

#if !defined(NO_PARMETIS_SUPPORT)
  int get_common_node_count(const std::vector<Ioss::BlockDecompositionData> &el_blocks,
                            MPI_Comm                                         comm)
  {
    // Determine number of nodes that elements must share to be
    // considered connected.  A 8-node hex-only mesh would have 4
    // A 3D shell mesh should have 2.  Basically, use the minimum
    // number of nodes per side for all element blocks...  Omit sphere
    // elements; ignore bars(?)...

    int common_nodes = INT_MAX;

    for (const auto block : el_blocks) {
      if (block.global_count() == 0) {
        continue;
      }
      std::string            type     = Ioss::Utils::lowercase(block.topologyType);
      Ioss::ElementTopology *topology = Ioss::ElementTopology::factory(type, false);
      if (topology != nullptr) {
        Ioss::ElementTopology *boundary = topology->boundary_type(0);
        if (boundary != nullptr) {
          common_nodes = std::min(common_nodes, boundary->number_boundaries());
        }
        else {
          // Different topologies on some element faces...
          size_t nb = topology->number_boundaries();
          for (size_t b = 1; b <= nb; b++) {
            boundary = topology->boundary_type(b);
            if (boundary != nullptr) {
              common_nodes = std::min(common_nodes, boundary->number_boundaries());
            }
          }
        }
      }
    }
    common_nodes = std::max(1, common_nodes);
    Ioss::ParallelUtils par_util(comm);
    common_nodes = par_util.global_minmax(common_nodes, Ioss::ParallelUtils::DO_MIN);

#if IOSS_DEBUG_OUTPUT
    fmt::print(Ioss::DEBUG(), "Setting common_nodes to {}\n", common_nodes);
#endif
    return common_nodes;
  }
#endif
} // namespace

namespace Ioss {

  const std::vector<std::string> &valid_decomp_methods()
  {
    static const std::vector<std::string> valid_methods{
      "LINEAR"
#if !defined(NO_ZOLTAN_SUPPORT)
	,"BLOCK", "CYCLIC", "RANDOM", "RCB", "RIB", "HSFC"
#endif
#if !defined(NO_PARMETIS_SUPPORT)
	,"KWAY", "KWAY_GEOM", "GEOM_KWAY", "METIS_SFC"
#endif
	};
    return valid_methods;
  }

  template Decomposition<int>::Decomposition(const Ioss::PropertyManager &props, MPI_Comm comm);
  template Decomposition<int64_t>::Decomposition(const Ioss::PropertyManager &props, MPI_Comm comm);

  template <typename INT>
  Decomposition<INT>::Decomposition(const Ioss::PropertyManager &props, MPI_Comm comm)
      : m_comm(comm)
  {
    MPI_Comm_rank(m_comm, &m_processor);
    MPI_Comm_size(m_comm, &m_processorCount);
    m_method = get_decomposition_method(props, m_processor);

    Utils::check_set_bool_property(props, "RETAIN_FREE_NODES", m_retainFreeNodes);
    Utils::check_set_bool_property(props, "DECOMP_SHOW_HWM", m_showHWM);
    Utils::check_set_bool_property(props, "DECOMP_SHOW_PROGRESS", m_showProgress);
    if (!m_showProgress) {
      Utils::check_set_bool_property(props, "ENABLE_TRACING", m_showProgress);
    }
  }

  template void Decomposition<int>::generate_entity_distributions(size_t globalNodeCount,
                                                                  size_t globalElementCount);
  template void Decomposition<int64_t>::generate_entity_distributions(size_t globalNodeCount,
                                                                      size_t globalElementCount);

  template <typename INT>
  void Decomposition<INT>::generate_entity_distributions(size_t globalNodeCount,
                                                         size_t globalElementCount)
  {
    show_progress(__func__);
    m_globalNodeCount    = globalNodeCount;
    m_globalElementCount = globalElementCount;

    m_elementDist = get_entity_dist<INT>(m_processorCount, m_processor, m_globalElementCount,
                                         &m_elementOffset, &m_elementCount);
    m_nodeDist    = get_entity_dist<INT>(m_processorCount, m_processor, m_globalNodeCount,
                                      &m_nodeOffset, &m_nodeCount);
  }

  template <typename INT>
  void Decomposition<INT>::get_element_block_communication(
      std::vector<BlockDecompositionData> &el_blocks)
  {
    show_progress(__func__);
    for (auto &block : el_blocks) {
      block.exportCount.resize(m_processorCount);
      block.exportIndex.resize(m_processorCount);
      block.importCount.resize(m_processorCount);
      block.importIndex.resize(m_processorCount);
    }

    // First iterate the local element indices and count number in
    // each block.
    size_t b = 0;
    for (auto loc_elem : localElementMap) {
      size_t elem = loc_elem + m_elementOffset;
      b           = Ioss::Utils::find_index_location(elem, m_fileBlockIndex);

      assert(elem >= m_fileBlockIndex[b] && elem < m_fileBlockIndex[b + 1]);
      size_t off = std::max(m_fileBlockIndex[b], m_elementOffset);
      el_blocks[b].localMap.push_back(elem - off);
    }

    // Now iterate the imported element list...
    // Find number of imported elements that are less than the current
    // local_map[0]
    b                        = 0;
    size_t              proc = 0;
    std::vector<size_t> imp_index(el_blocks.size());
    for (size_t i = 0; i < importElementMap.size(); i++) {
      size_t elem = importElementMap[i];
      while (i >= (size_t)importElementIndex[proc + 1]) {
        proc++;
      }

      b          = Ioss::Utils::find_index_location(elem, m_fileBlockIndex);
      size_t off = std::max(m_fileBlockIndex[b], m_elementOffset);

      if (!el_blocks[b].localMap.empty() && elem < el_blocks[b].localMap[0] + off) {
        el_blocks[b].localIossOffset++;
        el_blocks[b].importMap.push_back(imp_index[b]++);
      }
      else {
        el_blocks[b].importMap.push_back(el_blocks[b].localMap.size() + imp_index[b]++);
      }
      el_blocks[b].importCount[proc]++;
    }

    // Now for the exported data...
    proc = 0;
    b    = 0;
    for (size_t i = 0; i < exportElementMap.size(); i++) {
      size_t elem = exportElementMap[i];
      while (i >= (size_t)exportElementIndex[proc + 1]) {
        proc++;
      }

      b = Ioss::Utils::find_index_location(elem, m_fileBlockIndex);

      size_t off = std::max(m_fileBlockIndex[b], m_elementOffset);
      el_blocks[b].exportMap.push_back(elem - off);
      el_blocks[b].exportCount[proc]++;
    }

    for (auto &block : el_blocks) {
      block.iossCount = block.localMap.size() + block.importMap.size();
      std::copy(block.exportCount.begin(), block.exportCount.end(), block.exportIndex.begin());
      std::copy(block.importCount.begin(), block.importCount.end(), block.importIndex.begin());
      Ioss::Utils::generate_index(block.exportIndex);
      Ioss::Utils::generate_index(block.importIndex);
    }
  }

  template void Decomposition<int>::decompose_model(
#if !defined(NO_ZOLTAN_SUPPORT)
      Zoltan &zz,
#endif
      std::vector<BlockDecompositionData> &element_blocks);
  template void Decomposition<int64_t>::decompose_model(
#if !defined(NO_ZOLTAN_SUPPORT)
      Zoltan &zz,
#endif
      std::vector<BlockDecompositionData> &element_blocks);
  template <typename INT>
  void Decomposition<INT>::decompose_model(
#if !defined(NO_ZOLTAN_SUPPORT)
      Zoltan &zz,
#endif
      std::vector<BlockDecompositionData> &element_blocks)
  {
    show_progress(__func__);
    if (m_processor == 0) {
      fmt::print(Ioss::OUTPUT(),
                 "\nIOSS: Using decomposition method '{}' for {:n} elements on {} processors.\n",
                 m_method, m_globalElementCount, m_processorCount);

      if ((size_t)m_processorCount > m_globalElementCount) {
        fmt::print(Ioss::WARNING(),
                   "Decomposing {} elements across {} processors will "
                   "result in some processors with *NO* elements.\n",
                   m_globalElementCount, m_processorCount);
      }
    }
#if !defined(NO_PARMETIS_SUPPORT)
    if (m_method == "KWAY" || m_method == "GEOM_KWAY" || m_method == "KWAY_GEOM" ||
        m_method == "METIS_SFC") {
      metis_decompose((idx_t *)m_pointer.data(), (idx_t *)m_adjacency.data(), element_blocks);
    }
#endif
#if !defined(NO_ZOLTAN_SUPPORT)
    if (m_method == "RCB" || m_method == "RIB" || m_method == "HSFC" || m_method == "BLOCK" ||
        m_method == "CYCLIC" || m_method == "RANDOM") {
      zoltan_decompose(zz);
    }
#endif
    if (m_method == "LINEAR") {
      if (m_globalElementCount > 0) {
        simple_decompose();
      }
      else {
        simple_node_decompose();
      }
    }

    show_progress("\tfinished with decomposition method");
    Ioss::qsort(importElementMap);
    show_progress("\tfinished with sort");

    std::copy(importElementCount.begin(), importElementCount.end(), importElementIndex.begin());
    Ioss::Utils::generate_index(importElementIndex);

    // Find the number of imported elements that precede the elements
    // that remain locally owned...
    m_importPreLocalElemIndex = 0;
    for (size_t i = 0; i < importElementMap.size(); i++) {
      if ((size_t)importElementMap[i] >= m_elementOffset) {
        break;
      }
      m_importPreLocalElemIndex++;
    }

    // Determine size of this processors element blocks...
    get_element_block_communication(element_blocks);

    // Now need to determine the nodes that are on this processor,
    // both owned and shared...
    if (m_globalElementCount > 0) {
      get_local_node_list();
      get_shared_node_list();
    }

    show_progress("\tprior to releasing some temporary decomposition memory");

    // Release some memory...
    Ioss::Utils::clear(m_adjacency);
    Ioss::Utils::clear(m_pointer);
    Ioss::Utils::clear(m_elementDist);
    Ioss::Utils::clear(m_nodeDist);
    show_progress("\tIoss::decompose model finished");
  }

  template void Decomposition<int>::calculate_element_centroids(const std::vector<double> &x,
                                                                const std::vector<double> &y,
                                                                const std::vector<double> &z);
  template void Decomposition<int64_t>::calculate_element_centroids(const std::vector<double> &x,
                                                                    const std::vector<double> &y,
                                                                    const std::vector<double> &z);

  template <typename INT>
  void Decomposition<INT>::calculate_element_centroids(const std::vector<double> &x,
                                                       const std::vector<double> &y,
                                                       const std::vector<double> &z)
  {
    // recv_count is the number of nodes that I need to recv from the other
    // processors
    // send_count is the number of nodes that I need to send to the other
    // processors
    show_progress(__func__);
    std::vector<INT> recv_count(m_processorCount);
    std::vector<INT> send_count(m_processorCount);

    std::vector<int> owner; // Size is sum of element connectivity sizes (same as
                            // adjacency list)
    owner.reserve(m_adjacency.size());

    for (auto node : m_adjacency) {
      INT owning_processor = Ioss::Utils::find_index_location(node, m_nodeDist);
      owner.push_back(owning_processor);
      recv_count[owning_processor]++;
    }

    // Zero out myProcessor entry in recv_count and sum the
    // remainder...
    recv_count[m_processor] = 0;

    // Tell each processor how many nodes worth of data to send to
    // every other processor...
    MPI_Alltoall(recv_count.data(), 1, Ioss::mpi_type((INT)0), send_count.data(), 1,
                 Ioss::mpi_type((INT)0), m_comm);

    send_count[m_processor] = 0;

    std::vector<INT> recv_disp(m_processorCount);
    std::vector<INT> send_disp(m_processorCount);
    size_t           sums = 0;
    size_t           sumr = 0;
    for (int p = 0; p < m_processorCount; p++) {
      recv_disp[p] = sumr;
      sumr += recv_count[p];

      send_disp[p] = sums;
      sums += send_count[p];
    }

#if IOSS_DEBUG_OUTPUT
    fmt::print(Ioss::DEBUG(),
               "Processor {} communicates {} nodes from and {} nodes to other processors\n",
               m_processor, sumr, sums);
#endif
    // Build the list telling the other processors which of their nodes I will
    // need data from...
    std::vector<INT> node_comm_recv(sumr);
    std::vector<INT> node_comm_send(sums);
    {
      std::vector<INT> recv_tmp(m_processorCount);
      for (size_t i = 0; i < owner.size(); i++) {
        int proc = owner[i];
        if (proc != m_processor) {
          INT    node              = m_adjacency[i];
          size_t position          = recv_disp[proc] + recv_tmp[proc]++;
          node_comm_recv[position] = node;
        }
      }
    }

    assert(node_comm_recv.size() == sumr);

    Ioss::MY_Alltoallv(node_comm_recv, recv_count, recv_disp, node_comm_send, send_count, send_disp,
                       m_comm);

    Ioss::Utils::clear(node_comm_recv);

// At this point, 'node_comm_send' contains the list of nodes that I
// need to provide coordinate data for.

// DEBUG: == Check that all nodes in node_comm_send are in the range
//           m_nodeOffset..m_nodeOffset+m_nodeCount
#ifndef NDEBUG
    for (auto node : node_comm_send) {
      assert((size_t)node >= m_nodeOffset && (size_t)node < m_nodeOffset + m_nodeCount);
    }
#endif

    // The total vector size I need to send data in is node_comm_send.size()*3
    std::vector<double> coord_send;
    coord_send.reserve(node_comm_send.size() * m_spatialDimension);
    std::vector<double> coord_recv(sumr * m_spatialDimension);
    for (auto node : node_comm_send) {
      node -= m_nodeOffset;
      coord_send.push_back(x[node]);
      if (m_spatialDimension > 1) {
        coord_send.push_back(y[node]);
      }
      if (m_spatialDimension > 2) {
        coord_send.push_back(z[node]);
      }
    }
    assert(coord_send.size() == node_comm_send.size() * m_spatialDimension);

    // Send the coordinate data back to the processors that requested it...
    for (int i = 0; i < m_processorCount; i++) {
      send_count[i] *= m_spatialDimension;
      recv_count[i] *= m_spatialDimension;
      send_disp[i] *= m_spatialDimension;
      recv_disp[i] *= m_spatialDimension;
    }

    Ioss::MY_Alltoallv(coord_send, send_count, send_disp, coord_recv, recv_count, recv_disp,
                       m_comm);

    // Don't need coord_send data anymore ... clean out the vector.
    Ioss::Utils::clear(coord_send);

    // Should have all needed coordinate data at this time.
    // Some in x,y,z vectors and some in coord_recv vector.

    // Note that in the current data structure, adjacency contains the
    // connectivity for all elements on this processor. 'owner' is a
    // parallel datastructure containing the owning processor for that
    // node.  If it is off-processor, then its coordinates will be
    // stored in coord_recv in processor order, but will be hit in the
    // correct order... The 'pointer' array tells the number of nodes
    // per element...

    // Calculate the centroid into the DecompositionData structure 'centroids'
    m_centroids.reserve(m_elementCount * m_spatialDimension);
    std::vector<INT> recv_tmp(m_processorCount);

    for (size_t i = 0; i < m_elementCount; i++) {
      size_t nnpe = m_pointer[i + 1] - m_pointer[i];
      double cx   = 0.0;
      double cy   = 0.0;
      double cz   = 0.0;
      for (INT jj = m_pointer[i]; jj < m_pointer[i + 1]; jj++) {
        INT node = m_adjacency[jj];
        INT proc = owner[jj];
        if (proc == m_processor) {
          cx += x[node - m_nodeOffset];
          if (m_spatialDimension > 1) {
            cy += y[node - m_nodeOffset];
          }
          if (m_spatialDimension > 2) {
            cz += z[node - m_nodeOffset];
          }
        }
        else {
          INT coffset = recv_disp[proc] + recv_tmp[proc];
          recv_tmp[proc] += m_spatialDimension;
          cx += coord_recv[coffset + 0];
          if (m_spatialDimension > 1) {
            cy += coord_recv[coffset + 1];
          }
          if (m_spatialDimension > 2) {
            cz += coord_recv[coffset + 2];
          }
        }
      }
      m_centroids.push_back(cx / nnpe);
      if (m_spatialDimension > 1) {
        m_centroids.push_back(cy / nnpe);
      }
      if (m_spatialDimension > 2) {
        m_centroids.push_back(cz / nnpe);
      }
    }
  }

  template <typename INT> void Decomposition<INT>::simple_decompose()
  {
    show_progress(__func__);
    if (m_method == "LINEAR") {
      // The "ioss_decomposition" is the same as the "file_decomposition"
      // Nothing is imported or exported, everything stays "local"

      size_t local = m_elementDist[m_processor + 1] - m_elementDist[m_processor];
      assert(local == m_elementCount);
      localElementMap.resize(local);
      std::iota(localElementMap.begin(), localElementMap.end(), 0);

      // All values are 0
      exportElementCount.resize(m_processorCount + 1);
      exportElementIndex.resize(m_processorCount + 1);
      importElementCount.resize(m_processorCount + 1);
      importElementIndex.resize(m_processorCount + 1);
    }
  }

  template <typename INT> void Decomposition<INT>::simple_node_decompose()
  {
    // Used if there are no elements on the model...
    show_progress(__func__);
    if (m_method == "LINEAR") {
      // The "ioss_decomposition" is the same as the "file_decomposition"
      // Nothing is imported or exported, everything stays "local"

      size_t local_elem = 0;

      // All values are 0
      localElementMap.resize(local_elem);
      exportElementCount.resize(m_processorCount + 1);
      exportElementIndex.resize(m_processorCount + 1);
      importElementCount.resize(m_processorCount + 1);
      importElementIndex.resize(m_processorCount + 1);

      size_t local = m_nodeDist[m_processor + 1] - m_nodeDist[m_processor];
      assert(local == m_nodeCount);

      localNodeMap.resize(local);
      nodeGTL.resize(local);
      std::iota(localNodeMap.begin(), localNodeMap.end(), m_nodeOffset);
      std::iota(nodeGTL.begin(), nodeGTL.end(), m_nodeOffset + 1);

      // All values are 0
      exportNodeCount.resize(m_processorCount + 1);
      exportNodeIndex.resize(m_processorCount + 1);
      importNodeCount.resize(m_processorCount + 1);
      importNodeIndex.resize(m_processorCount + 1);
    }
  }

#if !defined(NO_PARMETIS_SUPPORT)
  template <typename INT>
  void Decomposition<INT>::metis_decompose(idx_t *pointer, idx_t *adjacency,
                                           std::vector<BlockDecompositionData> &el_blocks)
  {
    show_progress(__func__);
    std::vector<idx_t> elem_partition(m_elementCount);

    // Determine whether sizeof(INT) matches sizeof(idx_t).
    // If not, decide how to proceed...
    if (sizeof(INT) == sizeof(idx_t)) {
      internal_metis_decompose(el_blocks, (idx_t *)m_elementDist.data(), pointer, adjacency,
                               elem_partition.data());
    }

    // Now know that they don't match... Are we widening or narrowing...
    else if (sizeof(idx_t) > sizeof(INT)) {
      assert(sizeof(idx_t) == 8);
      // ... Widening; just create new wider arrays
      std::vector<idx_t> dist_cv(m_elementDist.begin(), m_elementDist.end());
      std::vector<idx_t> pointer_cv(m_pointer.begin(), m_pointer.end());
      std::vector<idx_t> adjacency_cv(m_adjacency.begin(), m_adjacency.end());
      internal_metis_decompose(el_blocks, dist_cv.data(), pointer_cv.data(), adjacency_cv.data(),
                               elem_partition.data());
    }

    else if (sizeof(idx_t) < sizeof(INT)) {
      // ... Narrowing.  See if data range (#elements and/or #nodes) fits in
      // 32-bit idx_t
      // Can determine this by checking the pointer[
      assert(sizeof(idx_t) == 4);
      if (m_globalElementCount >= INT_MAX || m_globalNodeCount >= INT_MAX ||
          m_pointer[m_elementCount] >= INT_MAX) {
        // Can't narrow...
        std::ostringstream errmsg;
        fmt::print(
            errmsg,
            "ERROR: The metis/parmetis libraries being used with this application only support\n"
            "       32-bit integers, but the mesh being decomposed requires 64-bit integers.\n"
            "       You must either choose a different, non-metis decomposition method, or\n"
            "       rebuild your metis/parmetis libraries with 64-bit integer support.\n"
            "       Contact gdsjaar@sandia.gov for more details.\n");
        IOSS_ERROR(errmsg);
      }
      else {
        // Should be able to narrow...
        std::vector<idx_t> dist_cv(m_elementDist.begin(), m_elementDist.end());
        std::vector<idx_t> pointer_cv(m_pointer.begin(), m_pointer.end());
        std::vector<idx_t> adjacency_cv(m_adjacency.begin(), m_adjacency.end());
        internal_metis_decompose(el_blocks, dist_cv.data(), pointer_cv.data(), adjacency_cv.data(),
                                 elem_partition.data());
      }
    }
    // ------------------------------------------------------------------------
    // Done with metis functions...
    show_progress("\tDone with metis functions");

    // Determine how many elements I send to the other processors...
    // and how many remain local (on this processor)
    exportElementCount.resize(m_processorCount + 1);
    for (auto element : elem_partition) {
      exportElementCount[element]++;
    }

    size_t local = exportElementCount[m_processor];
    localElementMap.reserve(local);
    for (size_t i = 0; i < elem_partition.size(); i++) {
      if (elem_partition[i] == m_processor) {
        localElementMap.push_back(i);
      }
    }

    // Zero out the local element count so local elements aren't communicated.
    exportElementCount[m_processor] = 0;

    importElementCount.resize(m_processorCount + 1);
    MPI_Alltoall(exportElementCount.data(), 1, Ioss::mpi_type((INT)0), importElementCount.data(), 1,
                 Ioss::mpi_type((INT)0), m_comm);
    show_progress("\tmetis_decompose Communication 1 finished");

    // Now fill the vectors with the elements ...
    size_t exp_size = std::accumulate(exportElementCount.begin(), exportElementCount.end(), 0);

    exportElementMap.resize(exp_size);
    exportElementIndex.resize(m_processorCount + 1);
    std::copy(exportElementCount.begin(), exportElementCount.end(), exportElementIndex.begin());
    Ioss::Utils::generate_index(exportElementIndex);

    {
      std::vector<INT> tmp_disp(exportElementIndex);
      for (size_t i = 0; i < elem_partition.size(); i++) {
        if (elem_partition[i] != m_processor) {
          exportElementMap[tmp_disp[elem_partition[i]]++] = m_elementOffset + i;
        }
      }
    }
    Ioss::Utils::clear(elem_partition);

    size_t imp_size = std::accumulate(importElementCount.begin(), importElementCount.end(), 0);
    importElementMap.resize(imp_size);
    importElementIndex.resize(m_processorCount + 1);
    std::copy(importElementCount.begin(), importElementCount.end(), importElementIndex.begin());
    Ioss::Utils::generate_index(importElementIndex);

    Ioss::MY_Alltoallv(exportElementMap, exportElementCount, exportElementIndex, importElementMap,
                       importElementCount, importElementIndex, m_comm);
    show_progress("\tmetis_decompose Communication 2 finished");

#if IOSS_DEBUG_OUTPUT
    fmt::print(Ioss::DEBUG(), "Processor {}:\t{} local, {} imported and {} exported elements\n",
               m_processor, m_elementCount - exp_size, imp_size, exp_size);
#endif
  }

  template <typename INT>
  void Decomposition<INT>::internal_metis_decompose(std::vector<BlockDecompositionData> &el_blocks,
                                                    idx_t *element_dist, idx_t *pointer,
                                                    idx_t *adjacency, idx_t *elem_partition)
  {
    idx_t  wgt_flag     = 0; // No weights
    idx_t *elm_wgt      = nullptr;
    idx_t  ncon         = 1;
    idx_t  num_flag     = 0; // Use C-based numbering
    idx_t  common_nodes = get_common_node_count(el_blocks, m_comm);

    idx_t               nparts = m_processorCount;
    idx_t               ndims  = m_spatialDimension;
    std::vector<real_t> tp_wgts(ncon * nparts, 1.0 / nparts);

    std::vector<real_t> ub_vec(ncon, 1.01);

    idx_t edge_cuts = 0;

    std::vector<idx_t> options(3);
    options[0] = 1;       // Use my values instead of default
    options[1] = 0;       // PARMETIS_DBGLVL_TIME;
    options[2] = 1234567; // Random number seed

    show_progress(__func__);
    if (m_method == "KWAY") {
      int rc =
          ParMETIS_V3_PartMeshKway(element_dist, pointer, adjacency, elm_wgt, &wgt_flag, &num_flag,
                                   &ncon, &common_nodes, &nparts, tp_wgts.data(), ub_vec.data(),
                                   options.data(), &edge_cuts, elem_partition, &m_comm);
#if IOSS_DEBUG_OUTPUT
      fmt::print(Ioss::DEBUG(), "Edge Cuts = {}\n", edge_cuts);
#endif
      if (rc != METIS_OK) {
        std::ostringstream errmsg;
        fmt::print(errmsg, "ERROR: Problem during call to ParMETIS_V3_PartMeshKWay "
                           "decomposition\n");
        IOSS_ERROR(errmsg);
      }
    }
    else if (m_method == "GEOM_KWAY" || m_method == "KWAY_GEOM") {

      idx_t *dual_xadj      = nullptr;
      idx_t *dual_adjacency = nullptr;
      int    rc = ParMETIS_V3_Mesh2Dual(element_dist, pointer, adjacency, &num_flag, &common_nodes,
                                     &dual_xadj, &dual_adjacency, &m_comm);

      if (rc != METIS_OK) {
        std::ostringstream errmsg;
        fmt::print(errmsg,
                   "ERROR: Problem during call to ParMETIS_V3_Mesh2Dual graph conversion\n");
        IOSS_ERROR(errmsg);
      }

      if (sizeof(double) == sizeof(real_t)) {
	rc = ParMETIS_V3_PartGeomKway(element_dist, dual_xadj, dual_adjacency, elm_wgt, elm_wgt,
				      &wgt_flag, &num_flag, &ndims, (real_t *)m_centroids.data(),
				      &ncon, &nparts, tp_wgts.data(), ub_vec.data(), options.data(),
				      &edge_cuts, elem_partition, &m_comm);
      }
      else {
	std::vector<real_t> centroids(m_centroids.begin(), m_centroids.end());
	rc = ParMETIS_V3_PartGeomKway(element_dist, dual_xadj, dual_adjacency, elm_wgt, elm_wgt,
				      &wgt_flag, &num_flag, &ndims, centroids.data(),
				      &ncon, &nparts, tp_wgts.data(), ub_vec.data(), options.data(),
				      &edge_cuts, elem_partition, &m_comm);
      }

#if IOSS_DEBUG_OUTPUT
      fmt::print(Ioss::DEBUG(), "Edge Cuts = {}\n", edge_cuts);
#endif
      METIS_Free(dual_xadj);
      METIS_Free(dual_adjacency);

      if (rc != METIS_OK) {
        std::ostringstream errmsg;
        fmt::print(errmsg,
                   "ERROR: Problem during call to ParMETIS_V3_PartGeomKWay decomposition\n");
        IOSS_ERROR(errmsg);
      }
    }
    else if (m_method == "METIS_SFC") {
      int rc = METIS_OK;
      if (sizeof(double) == sizeof(real_t)) {
	rc = ParMETIS_V3_PartGeom(element_dist, &ndims, (real_t *)m_centroids.data(),
				  elem_partition, &m_comm);
      }
      else {
	std::vector<real_t> centroids(m_centroids.begin(), m_centroids.end());
	rc = ParMETIS_V3_PartGeom(element_dist, &ndims, centroids.data(),
				  elem_partition, &m_comm);
      }

      if (rc != METIS_OK) {
        std::ostringstream errmsg;
        fmt::print(errmsg, "ERROR: Problem during call to ParMETIS_V3_PartGeom decomposition\n");
        IOSS_ERROR(errmsg);
      }
    }
    m_centroids.clear();
  }
#endif

#if !defined(NO_ZOLTAN_SUPPORT)
  template <typename INT> void Decomposition<INT>::zoltan_decompose(Zoltan &zz)
  {
    show_progress(__func__);
    // Set Zoltan parameters
    std::string num_proc = std::to_string(m_processorCount);
    zz.Set_Param("DEBUG_LEVEL", "0");
    zz.Set_Param("NUM_GLOBAL_PARTS", num_proc);

    int num_global = sizeof(INT) / sizeof(ZOLTAN_ID_TYPE);
    num_global     = num_global < 1 ? 1 : num_global;

    int lib_global_id_type_size = Zoltan_get_global_id_type(nullptr);
    if (lib_global_id_type_size != sizeof(ZOLTAN_ID_TYPE)) {
      std::ostringstream errmsg;
      fmt::print(errmsg,
                 "ERROR: The compile-time ZOLTAN_ID_TYPE size ({}) does not match the run-time "
                 "ZOLTAN_ID_TYPE size ({}). There is an error in the build/link procedure for this "
                 "application.\n",
                 sizeof(ZOLTAN_ID_TYPE), lib_global_id_type_size);
      IOSS_ERROR(errmsg);
    }

    zz.Set_Param("NUM_GID_ENTRIES", std::to_string(num_global));
    zz.Set_Param("NUM_LID_ENTRIES", "0");
    zz.Set_Param("LB_METHOD", m_method);
    zz.Set_Param("REMAP", "0");
    zz.Set_Param("RETURN_LISTS", "ALL");

    int           changes           = 0;
    int           num_local         = 0;
    int           num_import        = 1;
    int           num_export        = 1;
    ZOLTAN_ID_PTR import_global_ids = nullptr;
    ZOLTAN_ID_PTR import_local_ids  = nullptr;
    ZOLTAN_ID_PTR export_global_ids = nullptr;
    ZOLTAN_ID_PTR export_local_ids  = nullptr;
    int *         import_procs      = nullptr;
    int *         import_to_part    = nullptr;
    int *         export_procs      = nullptr;
    int *         export_to_part    = nullptr;

    num_local = 1;

    int rc = zz.LB_Partition(changes, num_global, num_local, num_import, import_global_ids,
                             import_local_ids, import_procs, import_to_part, num_export,
                             export_global_ids, export_local_ids, export_procs, export_to_part);

    if (rc != ZOLTAN_OK) {
      std::ostringstream errmsg;
      fmt::print(errmsg, "ERROR: Problem during call to Zoltan LB_Partition.\n");
      IOSS_ERROR(errmsg);
    }
    show_progress("\tZoltan lb_partition finished");

#if IOSS_DEBUG_OUTPUT
    fmt::print(Ioss::DEBUG(), "Processor {}:\t{} local, {} imported and {} exported elements\n",
               m_processor, m_elementCount - num_export, num_import, num_export);
#endif

    // Don't need centroid data anymore... Free up space
    Ioss::Utils::clear(m_centroids);

    // Find all elements that remain locally owned...
    get_local_element_list(export_global_ids, num_export);

    // Build exportElementMap and importElementMap...
    importElementMap.reserve(num_import);
    importElementIndex.resize(m_processorCount + 1);
    importElementCount.resize(m_processorCount + 1);

    if (num_global == 1) {
      if (num_export > 0 && export_procs == nullptr) {
        std::ostringstream errmsg;
        fmt::print(errmsg, "ERROR: Internal error in zoltan_decompose.  export_procs is null.\n");
        IOSS_ERROR(errmsg);
      }

      std::vector<std::pair<int, int>> export_map;
      export_map.reserve(num_export);
      for (int i = 0; i < num_export; i++) {
        export_map.emplace_back(std::make_pair(export_procs[i], export_global_ids[i]));
      }

      std::sort(export_map.begin(), export_map.end());
      exportElementMap.reserve(num_export);
      exportElementIndex.resize(m_processorCount + 1);
      exportElementCount.resize(m_processorCount + 1);
      for (auto elem_count : export_map) {
        exportElementMap.push_back(elem_count.second);
        exportElementCount[elem_count.first]++;
      }

      for (int i = 0; i < num_import; i++) {
        importElementMap.push_back(import_global_ids[i]);
        importElementCount[import_procs[i]]++;
      }
    }
    else {
      if (num_export > 0 && export_procs == nullptr) {
        std::ostringstream errmsg;
        fmt::print(errmsg, "ERROR: Internal error in zoltan_decompose.  export_procs is null.\n");
        IOSS_ERROR(errmsg);
      }
      std::vector<std::pair<int, int64_t>> export_map;
      export_map.reserve(num_export);
      int64_t *export_glob = reinterpret_cast<int64_t *>(export_global_ids);
      for (int i = 0; i < num_export; i++) {
        export_map.push_back(std::make_pair(export_procs[i], export_glob[i]));
      }

      std::sort(export_map.begin(), export_map.end());
      exportElementMap.reserve(num_export);
      exportElementIndex.resize(m_processorCount + 1);
      exportElementCount.resize(m_processorCount + 1);
      for (auto elem_count : export_map) {
        exportElementMap.push_back(elem_count.second);
        exportElementCount[elem_count.first]++;
      }

      int64_t *import_glob = reinterpret_cast<int64_t *>(import_global_ids);
      for (int i = 0; i < num_import; i++) {
        importElementMap.push_back(import_glob[i]);
        importElementCount[import_procs[i]]++;
      }
    }

    std::copy(exportElementCount.begin(), exportElementCount.end(), exportElementIndex.begin());
    Ioss::Utils::generate_index(exportElementIndex);

    zz.LB_Free_Part(&import_global_ids, &import_local_ids, &import_procs, &import_to_part);
    zz.LB_Free_Part(&export_global_ids, &export_local_ids, &export_procs, &export_to_part);
  }
#endif

#if !defined(NO_ZOLTAN_SUPPORT)
  template <typename INT>
  void Decomposition<INT>::get_local_element_list(const ZOLTAN_ID_PTR &export_global_ids,
                                                  size_t               export_count)
  {
    show_progress(__func__);
    std::vector<size_t> elements(m_elementCount);

    size_t global_id_size = sizeof(INT) / sizeof(int);

    if (global_id_size == 1) {
      for (size_t i = 0; i < export_count; i++) {
        // flag all elements to be exported...
        size_t elem                      = export_global_ids[i];
        elements[elem - m_elementOffset] = 1;
      }
    }
    else {
      assert(global_id_size == 2);
      int64_t *export_glob = reinterpret_cast<int64_t *>(export_global_ids);

      for (size_t i = 0; i < export_count; i++) {
        // flag all elements to be exported...
        size_t elem                      = export_glob[i];
        elements[elem - m_elementOffset] = 1;
      }
    }

    localElementMap.reserve(m_elementCount - export_count);
    for (size_t i = 0; i < m_elementCount; i++) {
      if (elements[i] == 0) {
        localElementMap.push_back(i);
      }
    }
  }
#endif

  template void Decomposition<int>::build_global_to_local_elem_map();
  template void Decomposition<int64_t>::build_global_to_local_elem_map();

  template <typename INT> void Decomposition<INT>::build_global_to_local_elem_map()
  {
    show_progress(__func__);
    // global_index is 1-based index into global list of elems
    // [1..global_elem_count]
    for (size_t i = 0; i < localElementMap.size(); i++) {
      size_t global_index   = localElementMap[i] + m_elementOffset + 1;
      size_t local_index    = i + m_importPreLocalElemIndex + 1;
      elemGTL[global_index] = local_index;
    }

    for (size_t i = 0; i < m_importPreLocalElemIndex; i++) {
      size_t global_index   = importElementMap[i] + 1;
      size_t local_index    = i + 1;
      elemGTL[global_index] = local_index;
    }

    for (size_t i = m_importPreLocalElemIndex; i < importElementMap.size(); i++) {
      size_t global_index   = importElementMap[i] + 1;
      size_t local_index    = localElementMap.size() + i + 1;
      elemGTL[global_index] = local_index;
    }
  }

  template <typename INT> void Decomposition<INT>::get_local_node_list()
  {
    // Get the connectivity of all imported elements...
    // First, determine how many nodes the exporting processors are
    // going to send me and how many nodes my exported elements
    // have...
    show_progress(__func__);

    std::vector<INT> export_conn_size(m_processorCount);
    std::vector<INT> import_conn_size(m_processorCount);
    for (int p = 0; p < m_processorCount; p++) {
      size_t el_begin = exportElementIndex[p];
      size_t el_end   = exportElementIndex[p + 1];
      for (size_t i = el_begin; i < el_end; i++) {
        INT    elem = exportElementMap[i] - m_elementOffset;
        size_t nnpe = m_pointer[elem + 1] - m_pointer[elem];
        export_conn_size[p] += nnpe;
      }
    }

    MPI_Alltoall(export_conn_size.data(), 1, Ioss::mpi_type((INT)0), import_conn_size.data(), 1,
                 Ioss::mpi_type((INT)0), m_comm);
    show_progress("\tCommunication 1 finished");

    // Now fill the vectors with the nodes ...
    size_t exp_size = std::accumulate(export_conn_size.begin(), export_conn_size.end(), 0);
    size_t imp_size = std::accumulate(import_conn_size.begin(), import_conn_size.end(), 0);
    std::vector<INT> export_conn;
    export_conn.reserve(exp_size);

    std::vector<INT> export_disp(m_processorCount);
    std::vector<INT> import_disp(m_processorCount);
    for (int p = 1; p < m_processorCount; p++) {
      export_disp[p] = export_disp[p - 1] + export_conn_size[p - 1];
      import_disp[p] = import_disp[p - 1] + import_conn_size[p - 1];
    }

    for (int p = 0; p < m_processorCount; p++) {
      size_t el_begin = exportElementIndex[p];
      size_t el_end   = exportElementIndex[p + 1];
      for (size_t i = el_begin; i < el_end; i++) {
        INT elem = exportElementMap[i] - m_elementOffset;
        for (INT n = m_pointer[elem]; n < m_pointer[elem + 1]; n++) {
          export_conn.push_back(m_adjacency[n]);
        }
      }
    }

    // Count number of nodes on local elements...
    size_t node_sum = 0;
    for (auto elem : localElementMap) {
      node_sum += m_pointer[elem + 1] - m_pointer[elem];
    }
    // Also holds imported nodes...
    node_sum += imp_size;

    std::vector<INT> nodes;

    {
      std::vector<INT> import_conn(imp_size);

      Ioss::MY_Alltoallv(export_conn, export_conn_size, export_disp, import_conn, import_conn_size,
                         import_disp, m_comm);
      show_progress("\tCommunication 2 finished");

      // Done with export_conn...
      Ioss::Utils::clear(export_conn);

      // Find list of unique nodes used by the elements on this
      // processor... adjacency list contains connectivity for local
      // elements and import_conn contains connectivity for imported
      // elements.

      // Nodes on Imported elements...
      nodes.reserve(node_sum);
      for (auto node : import_conn) {
        nodes.push_back(node);
      }
    }

    // Nodes on local elements...
    for (auto elem : localElementMap) {
      for (INT n = m_pointer[elem]; n < m_pointer[elem + 1]; n++) {
        nodes.push_back(m_adjacency[n]);
      }
    }

    // Now need to sort and Ioss::Utils::uniquify 'nodes'
    Ioss::Utils::uniquify(nodes);
    show_progress("\tUniquify finished");

    // Determine owning 'file' processor for each node...
    nodeIndex.resize(m_processorCount + 1);

    for (auto node : nodes) {
      INT owning_processor = Ioss::Utils::find_index_location(node, m_nodeDist);
      nodeIndex[owning_processor]++;
    }
    importNodeCount.resize(nodeIndex.size());
    std::copy(nodeIndex.begin(), nodeIndex.end(), importNodeCount.begin());
    exportNodeCount.resize(m_processorCount);
    Ioss::Utils::generate_index(nodeIndex);

    // Tell other processors how many nodes I will be importing from
    // them...
    importNodeCount[m_processor] = 0;
    MPI_Alltoall(importNodeCount.data(), 1, Ioss::mpi_type((INT)0), exportNodeCount.data(), 1,
                 Ioss::mpi_type((INT)0), m_comm);
    show_progress("\tCommunication 3 finished");

    size_t import_sum = std::accumulate(importNodeCount.begin(), importNodeCount.end(), 0);
    size_t export_sum = std::accumulate(exportNodeCount.begin(), exportNodeCount.end(), 0);

    std::vector<INT> import_nodes;
    import_nodes.reserve(import_sum);
    importNodeMap.reserve(import_sum);
    for (int p = 0; p < m_processorCount; p++) {
      size_t beg = nodeIndex[p];
      size_t end = nodeIndex[p + 1];

      if (p == m_processor) {
        m_importPreLocalNodeIndex = beg;
        localNodeMap.reserve(end - beg);
        for (size_t n = beg; n < end; n++) {
          localNodeMap.push_back(nodes[n]);
        }
      }
      else {
        for (size_t n = beg; n < end; n++) {
          import_nodes.push_back(nodes[n]);
          importNodeMap.push_back(n);
        }
      }
    }
    assert(import_nodes.size() == import_sum);
    exportNodeMap.resize(export_sum);
    exportNodeIndex.resize(m_processorCount + 1);
    std::copy(exportNodeCount.begin(), exportNodeCount.end(), exportNodeIndex.begin());
    Ioss::Utils::generate_index(exportNodeIndex);

    // Now send the list of nodes that I need to import from each
    // processor...
    importNodeIndex.resize(importNodeCount.size());
    std::copy(importNodeCount.begin(), importNodeCount.end(), importNodeIndex.begin());
    Ioss::Utils::generate_index(importNodeIndex);

    Ioss::MY_Alltoallv(import_nodes, importNodeCount, importNodeIndex, exportNodeMap,
                       exportNodeCount, exportNodeIndex, m_comm);
    Ioss::Utils::clear(import_nodes);
    show_progress("\tCommunication 4 finished");

    if (m_retainFreeNodes) {
      // See if all nodes have been accounted for (i.e., process non-connected nodes)
      std::vector<bool> file_nodes(m_nodeCount);
      for (const auto &node : exportNodeMap) {
        file_nodes[node - m_nodeOffset] = true;
      }
      for (const auto &node : localNodeMap) {
        file_nodes[node - m_nodeOffset] = true;
      }

      size_t found_count = 0;
      for (size_t i = 0; i < file_nodes.size(); i++) {
        if (!file_nodes[i]) {
          localNodeMap.push_back(i + m_nodeOffset);
          nodes.push_back(i + m_nodeOffset);
          found_count++;
#if IOSS_DEBUG_OUTPUT
          fmt::print(Ioss::DEBUG(), "Processor {}:\tNode {} not connected to any elements\n",
                     m_processor, i + m_nodeOffset + 1);
#endif
        }
      }

      if (found_count > 0) {
        nodes.shrink_to_fit();
        localNodeMap.shrink_to_fit();
        std::sort(nodes.begin(), nodes.end());
        std::sort(localNodeMap.begin(), localNodeMap.end());
        for (int proc = m_processor + 1; proc < m_processorCount + 1; proc++) {
          nodeIndex[proc] += found_count;
        }

        assert((size_t)nodeIndex[m_processorCount] == nodes.size());

        // Also need to update importNodeMap for all nodes being
        // imported from processors higher than m_processor...
        size_t beg = importNodeIndex[m_processor + 1];
        size_t end = importNodeIndex[m_processorCount];
        for (size_t i = beg; i < end; i++) {
          importNodeMap[i] += found_count;
        }
      }
    }

// Map that converts nodes from the global index (1-based) to a
// local-per-processor index (1-based)
#if IOSS_DEBUG_OUTPUT
    fmt::print(Ioss::DEBUG(), "Processor {}:\tNode Count = {}\n", m_processor, nodes.size());
#endif
    nodeGTL.swap(nodes);
    for (size_t i = 0; i < nodeGTL.size(); i++) {
      nodeGTL[i]++; // convert from 0-based index to 1-based index
    }
    show_progress(__func__);
  }

  template <typename INT> void Decomposition<INT>::get_shared_node_list()
  {
    // Need a list of all "shared" nodes (nodes on more than one
    // processor) and the list of processors that they are on for the
    // ioss decomposition.
    //
    // * iterate all local nodes (those that are in both file and ioss
    // decomposition)
    //   on this procesor and all exported nodes,
    // * put in a vector and sort on (id,proc).
    // * iterate and create a vector of all shared nodes and the
    //   processor they are on..
    show_progress(__func__);
    size_t local_node_count = nodeIndex[m_processor + 1] - nodeIndex[m_processor];
    std::vector<std::pair<INT, int>> node_proc_list;
    node_proc_list.reserve(local_node_count + exportNodeMap.size());

    for (auto local_node : localNodeMap) {
      node_proc_list.push_back(std::make_pair(local_node, m_processor));
    }

    for (int p = 0; p < m_processorCount; p++) {
      if (p == m_processor) {
        continue;
      }
      size_t beg = exportNodeIndex[p];
      size_t end = exportNodeIndex[p + 1];
      for (size_t i = beg; i < end; i++) {
        node_proc_list.push_back(std::make_pair(exportNodeMap[i], p));
      }
    }
    std::sort(node_proc_list.begin(), node_proc_list.end());

    std::vector<std::pair<INT, int>> shared_nodes;
    for (size_t i = 0; i < node_proc_list.size(); i++) {
      INT node = node_proc_list[i].first;
      if (i + 1 < node_proc_list.size() && node_proc_list[i + 1].first == node) {
        shared_nodes.push_back(node_proc_list[i]);
      }

      while (i + 1 < node_proc_list.size() && node_proc_list[i + 1].first == node) {
        shared_nodes.push_back(node_proc_list[++i]);
      }
    }

    // The shared_nodes list contains all nodes that I know about that
    // are shared.

    // Determine the counts...
    std::vector<INT> send_comm_map_count(m_processorCount);
    for (size_t i = 0; i < shared_nodes.size(); i++) {
      size_t beg = i;
      size_t end = ++i;
      while (i + 1 < shared_nodes.size() && shared_nodes[beg].first == shared_nodes[i + 1].first) {
        end = ++i;
      }
      for (size_t p = beg; p <= end; p++) {
        int proc = shared_nodes[p].second;
        for (size_t j = beg; j <= end; j++) {
          if (j == p) {
            continue;
          }
          assert(shared_nodes[p].first == shared_nodes[j].first);
          send_comm_map_count[proc] += 2;
        }
      }
    }

    // Determine total count... (including m_processor for now just to
    // see whether it simplifies/complicates coding)
    std::vector<INT> send_comm_map_disp(m_processorCount + 1);
    std::copy(send_comm_map_count.begin(), send_comm_map_count.end(), send_comm_map_disp.begin());
    Ioss::Utils::generate_index(send_comm_map_disp);

    std::vector<INT> send_comm_map(send_comm_map_disp[m_processorCount]);
    std::vector<INT> nc_offset(m_processorCount);

    for (size_t i = 0; i < shared_nodes.size(); i++) {
      size_t beg = i;
      size_t end = ++i;
      while (i + 1 < shared_nodes.size() && shared_nodes[beg].first == shared_nodes[i + 1].first) {
        end = ++i;
      }
      for (size_t p = beg; p <= end; p++) {
        int proc = shared_nodes[p].second;
        for (size_t j = beg; j <= end; j++) {
          if (j == p) {
            continue;
          }
          assert(shared_nodes[p].first == shared_nodes[j].first);
          size_t location             = send_comm_map_disp[proc] + nc_offset[proc];
          send_comm_map[location + 0] = shared_nodes[j].first;
          send_comm_map[location + 1] = shared_nodes[j].second;
          nc_offset[proc] += 2;
        }
      }
    }

    // Tell other processors how many nodes/procs I am sending them...
    std::vector<INT> recv_comm_map_count(m_processorCount);
    MPI_Alltoall(send_comm_map_count.data(), 1, Ioss::mpi_type((INT)0), recv_comm_map_count.data(),
                 1, Ioss::mpi_type((INT)0), m_comm);
    show_progress("\tCommuniation 1 finished");

    std::vector<INT> recv_comm_map_disp(recv_comm_map_count);
    Ioss::Utils::generate_index(recv_comm_map_disp);
    m_nodeCommMap.resize(recv_comm_map_disp[m_processorCount - 1] +
                         recv_comm_map_count[m_processorCount - 1]);
    Ioss::MY_Alltoallv(send_comm_map, send_comm_map_count, send_comm_map_disp, m_nodeCommMap,
                       recv_comm_map_count, recv_comm_map_disp, m_comm);
    Ioss::Utils::clear(send_comm_map);
    show_progress("\tCommuniation 2 finished");

    // Map global 0-based index to local 1-based index.
    for (size_t i = 0; i < m_nodeCommMap.size(); i += 2) {
      m_nodeCommMap[i] = node_global_to_local(m_nodeCommMap[i] + 1);
    }
#if IOSS_DEBUG_OUTPUT
    fmt::print(Ioss::DEBUG(), "Processor {} has {} shared nodes\n", m_processor,
               m_nodeCommMap.size() / 2);
#endif
    show_progress(__func__);
  }
} // namespace Ioss
