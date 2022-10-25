// Copyright(C) 1999-2022 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include <Ioss_CodeTypes.h>

#include <Ioss_CommSet.h>
#include <Ioss_DBUsage.h>
#include <Ioss_DatabaseIO.h>
#include <Ioss_ElementBlock.h>
#include <Ioss_ElementTopology.h>
#include <Ioss_FaceGenerator.h>
#include <Ioss_IOFactory.h>
#include <Ioss_NodeBlock.h>
#include <Ioss_ParallelUtils.h>
#include <Ioss_Property.h>
#include <Ioss_Region.h>

#include <algorithm>
#include <chrono>
#include "vtk_fmt.h"
#include VTK_FMT(fmt/format.h)
#include VTK_FMT(fmt/ostream.h)
#include <functional>
#include <numeric>
#include <random>
#include <utility>

// Options for generating hash function key...
#define USE_MURMUR
//#define USE_RANDOM

#define DO_TIMING 0

namespace {
#ifdef SEACAS_HAVE_MPI
  template <typename T> void generate_index(std::vector<T> &index)
  {
    T sum = 0;
    for (auto &idx : index) {
      T cnt = idx;
      idx   = sum;
      sum += cnt;
    }
  }
#endif

#if defined(USE_MURMUR)
  uint64_t MurmurHash64A(const size_t key);
#endif

  void create_face(Ioss::FaceUnorderedSet &faces, size_t id, std::array<size_t, 4> &conn,
                   size_t element, int local_face)
  {
    Ioss::Face face(id, conn);
    auto       face_iter = faces.insert(face);

    (*(face_iter.first)).add_element(element, local_face);
  }

  template <typename INT>
  void internal_generate_faces(Ioss::ElementBlock *eb, Ioss::FaceUnorderedSet &faces,
                               const std::vector<INT> &ids, const std::vector<size_t> &hash_ids,
                               bool local_ids, INT /*dummy*/)
  {
    const Ioss::ElementTopology *topo = eb->topology();

    // Only handle continuum elements at this time...
    if (topo->parametric_dimension() != 3) {
      return;
    }

    std::vector<INT> connectivity;
    eb->get_field_data("connectivity_raw", connectivity);

    std::vector<INT> elem_ids;
    if (local_ids) {
      elem_ids.resize(eb->entity_count());
      std::iota(elem_ids.begin(), elem_ids.end(), static_cast<INT>(eb->get_offset() + 1));
    }
    else {
      eb->get_field_data("ids", elem_ids);
    }

    int num_face_per_elem = topo->number_faces();
    assert(num_face_per_elem <= 6);
    std::array<Ioss::IntVector, 6> face_conn;
    std::array<int, 6>             face_node_count{};
    for (int face = 0; face < num_face_per_elem; face++) {
      face_conn[face]       = topo->face_connectivity(face + 1);
      face_node_count[face] = topo->face_type(face + 1)->number_corner_nodes();
    }

    int    num_node_per_elem = topo->number_nodes();
    size_t num_elem          = eb->entity_count();

    for (size_t elem = 0, offset = 0; elem < num_elem; elem++, offset += num_node_per_elem) {
      for (int face = 0; face < num_face_per_elem; face++) {
        size_t id = 0;
        assert(face_node_count[face] <= 4);
        std::array<size_t, 4> conn = {{0, 0, 0, 0}};
        for (int j = 0; j < face_node_count[face]; j++) {
          size_t fnode = offset + face_conn[face][j];
          size_t lnode = connectivity[fnode]; // local since "connectivity_raw"
          conn[j]      = ids[lnode - 1];      // Convert to global
          id += hash_ids[lnode - 1];
        }
        create_face(faces, id, conn, elem_ids[elem], face);
      }
    }
  }

  template <typename INT>
  void resolve_parallel_faces(Ioss::Region &region, Ioss::FaceUnorderedSet &faces,
                              const std::vector<size_t> &hash_ids, INT /*dummy*/)
  {
    PAR_UNUSED(region);
    PAR_UNUSED(faces);
    PAR_UNUSED(hash_ids);

#ifdef SEACAS_HAVE_MPI
    size_t proc_count = region.get_database()->util().parallel_size();

    if (proc_count > 1) {
      // If parallel, resolve faces on processor boundaries.
      // For each boundary face, need to check whether all of the nodes on
      // the face are shared with the same processor.  If so, then that face
      // is *possibly* shared with that processor.
      //
      // With the current continuum element only restriction, then a face
      // can only be shared with one other processor...

      // get nodal communication data CommSet...
      Ioss::CommSet *css = region.get_commset("commset_node");

      std::vector<std::pair<INT, INT>> proc_entity;
      {
        // entity_processor consists of node,proc, node,proc, entries.
        std::vector<INT> entity_processor;
        css->get_field_data("entity_processor_raw", entity_processor);

        proc_entity.reserve(entity_processor.size() / 2);
        for (size_t i = 0; i < entity_processor.size(); i += 2) {
          // Converts from 1-based to 0-based local nodes.
          proc_entity.push_back(std::make_pair(entity_processor[i + 1], entity_processor[i] - 1));
        }
      }

      // 'id_span' gives index into proc_entity for all nodes.
      // 'id_span[local_node_id] .. id_span[local_node_id+1]' gives
      // the location in 'proc_entity' of the sharing information
      // for node 'local_node_id'
      std::vector<size_t> id_span(hash_ids.size() + 1);
      for (const auto &pe : proc_entity) {
        INT node = pe.second;
        assert(node >= 0 && node < (INT)id_span.size() - 1);
        id_span[node]++;
      }
      generate_index(id_span);

      // Each boundary face ...
      // .. See if all of its nodes are shared with same processor.
      //  .. Iterate face nodes
      //  .. Determine shared proc.
      //  .. (for now, use a vector[proc] = count
      //   .. if potentially shared with 'proc', then count == num_nodes_face

      std::vector<INT> potential_count(proc_count);
      std::vector<int> shared_nodes(proc_count);
      for (auto &face : faces) {
        if (face.elementCount_ == 1) {
          // On 'boundary' -- try to determine whether on processor or exterior
          // boundary
          int face_node_count = 0;
          for (auto &gnode : face.connectivity_) {
            if (gnode > 0) {
              auto node = region.get_database()->node_global_to_local(gnode, true) - 1;
              face_node_count++;
              size_t begin = id_span[node];
              size_t end   = id_span[node + 1];
              for (size_t j = begin; j < end; j++) {
                assert(proc_entity[j].second == node);
                int proc = proc_entity[j].first;
                shared_nodes[proc]++;
              }
            }
          }
          for (size_t i = 0; i < proc_count; i++) {
            if (shared_nodes[i] == face_node_count) {
              potential_count[i]++;
            }
            shared_nodes[i] = 0; // Reset for next trip through face.connectivity_ loop
          }
        }
      }

      std::vector<INT> potential_offset(potential_count.begin(), potential_count.end());
      generate_index(potential_offset);

      size_t potential = potential_offset[proc_count - 1] + potential_count[proc_count - 1];
      std::vector<int64_t> potential_faces(6 * potential);

      for (auto &face : faces) {
        if (face.elementCount_ == 1) {
          // On 'boundary' -- try to determine whether on processor or exterior
          // boundary
          int face_node_count = 0;
          for (auto &gnode : face.connectivity_) {
            if (gnode > 0) {
              auto node = region.get_database()->node_global_to_local(gnode, true) - 1;
              face_node_count++;
              size_t begin = id_span[node];
              size_t end   = id_span[node + 1];
              for (size_t j = begin; j < end; j++) {
                assert(proc_entity[j].second == node);
                int proc = proc_entity[j].first;
                shared_nodes[proc]++;
              }
            }
          }
          for (size_t i = 0; i < proc_count; i++) {
            if (shared_nodes[i] == face_node_count) {
              size_t offset                   = potential_offset[i];
              potential_faces[6 * offset + 0] = face.hashId_;
              potential_faces[6 * offset + 1] = face.connectivity_[0];
              potential_faces[6 * offset + 2] = face.connectivity_[1];
              potential_faces[6 * offset + 3] = face.connectivity_[2];
              potential_faces[6 * offset + 4] = face.connectivity_[3];
              potential_faces[6 * offset + 5] = face.element[0];
              assert(face.elementCount_ == 1);
              potential_offset[i]++;
            }
            shared_nodes[i] = 0; // Reset for next trip through face.connectivity_ loop
          }
        }
      }

      // Regenerate potential_offset since it was modified above...
      std::copy(potential_count.begin(), potential_count.end(), potential_offset.begin());
      generate_index(potential_offset);

      // Now need to send to the other processors...
      // For now, use all-to-all; optimization is just send to processors with
      // data...
      std::vector<INT> check_count(proc_count);
      MPI_Alltoall(potential_count.data(), 1, Ioss::mpi_type((INT)0), check_count.data(), 1,
                   Ioss::mpi_type((INT)0), region.get_database()->util().communicator());

      const int            values_per_face = 6; // id, 4-node-conn, element
      auto                 sum = std::accumulate(check_count.begin(), check_count.end(), 0);
      std::vector<int64_t> check_faces(values_per_face * sum);

      std::vector<INT> check_offset(check_count.begin(), check_count.end());
      generate_index(check_offset);

      // Need to adjust counts and offsets to account for sending 6 values per
      // face...
      for (size_t i = 0; i < proc_count; i++) {
        potential_count[i] *= values_per_face;
        potential_offset[i] *= values_per_face;
        check_count[i] *= values_per_face;
        check_offset[i] *= values_per_face;
      }

      Ioss::MY_Alltoallv(potential_faces, potential_count, potential_offset, check_faces,
                         check_count, check_offset, region.get_database()->util().communicator());

      // Now iterate the check_faces and see if any of them match one
      // of this processors faces...  If so, then mark as shared and
      // add the element...
      for (size_t i = 0; i < check_faces.size(); i += values_per_face) {
        size_t                id = check_faces[i + 0];
        std::array<size_t, 4> conn{};
        conn[0]            = check_faces[i + 1];
        conn[1]            = check_faces[i + 2];
        conn[2]            = check_faces[i + 3];
        conn[3]            = check_faces[i + 4];
        size_t     element = check_faces[i + 5];
        Ioss::Face face(id, conn);
        auto       face_iter = faces.find(face, face.hashId_);
        if (face_iter != faces.end()) {
          // we have a match... This is a shared interior face
          (*face_iter).add_element(element); // Already has face multiplied in.

          int proc = 0;
          for (size_t j = 0; j < check_count.size(); j++) {
            if (check_count[j] > 0 && check_offset[j] == (INT)i) {
              break;
            }
            proc++;
          }
        }
      }
    }
#endif
  }
} // namespace

namespace Ioss {
  Face::Face(std::array<size_t, 4> conn) : connectivity_(conn)
  {
    for (auto &node : connectivity_) {
      hashId_ += Ioss::FaceGenerator::id_hash(node);
    }
  }

  void Face::face_element_error(size_t element_id) const
  {
    std::ostringstream errmsg;
    fmt::print(errmsg,
               "ERROR: Face {} has more than two elements using it.\n"
               "       The element/local_face are: {}:{}, {}:{}, and {}:{}.\n"
               "       The face connectivity is {} {} {} {}.\n",
               hashId_, element[0] / 10, element[0] % 10, element[1] / 10, element[1] % 10,
               element_id / 10, element_id % 10, connectivity_[0], connectivity_[1],
               connectivity_[2], connectivity_[3]);
    IOSS_ERROR(errmsg);
  }

  size_t FaceGenerator::id_hash(size_t global_id)
  {
#if defined(USE_RANDOM)
    std::mt19937_64 rng;
    rng.seed(global_id);
    return rng();
#elif defined(USE_MURMUR)
    return MurmurHash64A(global_id);
#else
    return global_id;
#endif
  }

  FaceGenerator::FaceGenerator(Ioss::Region &region) : region_(region) {}

  template void FaceGenerator::generate_faces(int, bool, bool);
  template void FaceGenerator::generate_faces(int64_t, bool, bool);

  template <typename INT>
  void FaceGenerator::generate_faces(INT /*dummy*/, bool block_by_block, bool local_ids)
  {
    if (block_by_block) {
      generate_block_faces(INT(0), local_ids);
    }
    else {
      generate_model_faces(INT(0), local_ids);
    }
  }

  template <typename INT> void FaceGenerator::hash_node_ids(const std::vector<INT> &node_ids)
  {
    hashIds_.reserve(node_ids.size());
    for (auto &id : node_ids) {
      hashIds_.push_back(id_hash(id));
    }
  }

  template <typename INT> void FaceGenerator::generate_block_faces(INT /*dummy*/, bool local_ids)
  {
    // Convert ids into hashed-ids
    Ioss::NodeBlock *nb = region_.get_node_blocks()[0];

    std::vector<INT> ids;
    if (local_ids) {
      ids.resize(nb->entity_count());
      std::iota(ids.begin(), ids.end(), 1);
    }
    else {
      nb->get_field_data("ids", ids);
    }
#if DO_TIMING
    auto starth = std::chrono::steady_clock::now();
#endif
    hash_node_ids(ids);
#if DO_TIMING
    auto endh = std::chrono::steady_clock::now();
#endif

    const auto &ebs = region_.get_element_blocks();
    for (auto &eb : ebs) {
      const std::string &name    = eb->name();
      size_t             numel   = eb->entity_count();
      size_t             reserve = 3.2 * numel;
      faces_[name].reserve(reserve);
      internal_generate_faces(eb, faces_[name], ids, hashIds_, local_ids, (INT)0);
    }

#if DO_TIMING
    auto endf = std::chrono::steady_clock::now();
#endif
    for (auto &eb : ebs) {
      resolve_parallel_faces(region_, faces_[eb->name()], hashIds_, (INT)0);
    }
#if DO_TIMING
    size_t face_count = 0;
    for (auto &eb : ebs) {
      face_count += faces_[eb->name()].size();
    }
    auto endp  = std::chrono::steady_clock::now();
    auto diffh = endh - starth;
    auto difff = endf - endh;
    fmt::print(
        "Node ID hash time:   \t{:.6} ms\t{:12} nodes/second\n"
        "Face generation time:\t{:.6} ms\t{:12} faces/second\n",
        std::chrono::duration<double, std::milli>(diffh).count(),
        fmt::group_digits(INT(hashIds_.size() / std::chrono::duration<double>(diffh).count())),
        std::chrono::duration<double, std::milli>(difff).count(),
        fmt::group_digits(INT(face_count / std::chrono::duration<double>(difff).count())));
#ifdef SEACAS_HAVE_MPI
    auto   diffp      = endp - endf;
    size_t proc_count = region_.get_database()->util().parallel_size();

    if (proc_count > 1) {
      fmt::print("Parallel time:       \t{:.6} ms\t{:12} faces/second.\n",
                 std::chrono::duration<double, std::milli>(diffp).count(),
                 fmt::group_digits(INT(face_count / std::chrono::duration<double>(diffp).count())));
    }
#endif
    fmt::print("Total time:          \t{:.6} ms\n\n",
               std::chrono::duration<double, std::milli>(endp - starth).count());
#endif
  }

  template <typename INT> void FaceGenerator::generate_model_faces(INT /*dummy*/, bool local_ids)
  {
    // Convert ids into hashed-ids
    Ioss::NodeBlock *nb = region_.get_node_blocks()[0];

    std::vector<INT> ids;
    if (local_ids) {
      ids.resize(nb->entity_count());
      std::iota(ids.begin(), ids.end(), 1);
    }
    else {
      nb->get_field_data("ids", ids);
    }
#if DO_TIMING
    auto starth = std::chrono::steady_clock::now();
#endif
    hash_node_ids(ids);
#if DO_TIMING
    auto endh = std::chrono::steady_clock::now();
#endif

    auto  &my_faces = faces_["ALL"];
    size_t numel    = region_.get_property("element_count").get_int();

    size_t reserve = 3.2 * numel;
    my_faces.reserve(reserve);
    const auto &ebs = region_.get_element_blocks();
    for (auto &eb : ebs) {
      internal_generate_faces(eb, my_faces, ids, hashIds_, local_ids, (INT)0);
    }

#if DO_TIMING
    auto endf = std::chrono::steady_clock::now();
#endif
    resolve_parallel_faces(region_, my_faces, hashIds_, (INT)0);

#if DO_TIMING
    auto endp  = std::chrono::steady_clock::now();
    auto diffh = endh - starth;
    auto difff = endf - endh;
    fmt::print("Node ID hash time:   \t{:.3f} ms\t{:.3} nodes/second\n"
               "Face generation time:\t{:.3f} ms\t{:.3} faces/second.\n",
               std::chrono::duration<double, std::milli>(diffh).count(),
               hashIds_.size() / std::chrono::duration<double>(diffh).count(),
               std::chrono::duration<double, std::milli>(difff).count(),
               my_faces.size() / std::chrono::duration<double>(difff).count());
#ifdef SEACAS_HAVE_MPI
    auto   diffp      = endp - endf;
    size_t proc_count = region_.get_database()->util().parallel_size();

    if (proc_count > 1) {
      fmt::print("Parallel time:       \t{:.3f} ms\t{:.3} faces/second.\n",
                 std::chrono::duration<double, std::milli>(diffp).count(),
                 my_faces.size() / std::chrono::duration<double>(diffp).count());
    }
#endif
    fmt::print("Total time:          \t{:.3f} ms\n\n",
               std::chrono::duration<double, std::milli>(endp - starth).count());
#endif
  }
} // namespace Ioss

namespace {
#if defined(USE_MURMUR)
//-----------------------------------------------------------------------------
// MurmurHash2 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.

// Note - This code makes a few assumptions about how your machine behaves -

// 1. We can read a 4-byte value from any address without crashing
// 2. sizeof(int) == 4

// And it has a few limitations -

// 1. It will not work incrementally.
// 2. It will not produce the same results on little-endian and big-endian
//    machines.

//-----------------------------------------------------------------------------
// MurmurHash2, 64-bit versions, by Austin Appleby

// The same caveats as 32-bit MurmurHash2 apply here - beware of alignment
// and endian-ness issues if used across multiple platforms.

// 64-bit hash for 64-bit platforms
#define BIG_CONSTANT(x) (x##LLU)
  uint64_t MurmurHash64A(const size_t key)
  {
    // NOTE: Not general purpose -- optimized for single 'size_t key'
    const uint64_t m    = BIG_CONSTANT(0xc6a4a7935bd1e995) * 8;
    const int      r    = 47;
    const int      seed = 24713;

    uint64_t h = seed ^ (m);

    uint64_t k = key;

    k *= m;
    k ^= k >> r;
    k *= m;

    h ^= k;
    h *= m;

    h ^= h >> r;
    h *= m;
    h ^= h >> r;

    return h;
  }
#endif
} // namespace
