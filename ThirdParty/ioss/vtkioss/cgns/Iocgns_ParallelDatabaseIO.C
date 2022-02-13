// CGNS Assumptions:
// * All boundary conditions are listed as Family nodes at the "top" level.
// * Single Base.
// * ZoneGridConnectivity is 1to1 with point lists for unstructured

// Copyright(C) 1999-2022 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include <vtk_cgns.h> // xxx(kitware)
#include VTK_CGNS(cgnsconfig.h)
#if CG_BUILD_PARALLEL
#include <cgns/Iocgns_Defines.h>

#include <Ioss_CodeTypes.h>
#include <Ioss_Sort.h>
#include <Ioss_Utils.h>
#include <cassert>
#include <cgns/Iocgns_ParallelDatabaseIO.h>
#include <cgns/Iocgns_Utils.h>
#include <cstddef>
#include <ctime>
#include "vtk_fmt.h"
#include VTK_FMT(fmt/ostream.h)
#include <fstream>
#include <iostream>
#include <mpi.h>
#include <numeric>
#include <string>
#include <sys/select.h>
#include <vector>

#include VTK_CGNS(pcgnslib.h)

#if !defined(CGNSLIB_H)
#error "Could not include cgnslib.h"
#endif

#include "Ioss_CommSet.h"
#include "Ioss_DBUsage.h"
#include "Ioss_DatabaseIO.h"
#include "Ioss_ElementBlock.h"
#include "Ioss_ElementTopology.h"
#include "Ioss_EntityType.h"
#include "Ioss_Field.h"
#include "Ioss_FileInfo.h"
#include "Ioss_IOFactory.h"
#include "Ioss_NodeBlock.h"
#include "Ioss_ParallelUtils.h"
#include "Ioss_Property.h"
#include "Ioss_Region.h"
#include "Ioss_SideBlock.h"
#include "Ioss_SideSet.h"
#include "Ioss_State.h"
#include "Ioss_StructuredBlock.h"
#include "Ioss_Utils.h"
#include "Ioss_VariableType.h"

using GL_IdVector = std::vector<std::pair<int, int>>;

namespace {
  MPI_Datatype cgns_mpi_type()
  {
#if CG_SIZEOF_SIZE == 8
    return MPI_LONG_LONG_INT;
#else
    return MPI_INT;
#endif
  }

  void map_local_to_global_implicit(CGNSIntVector &data, size_t count,
                                    const CGNSIntVector &global_implicit_map)
  {
    for (size_t i = 0; i < count; i++) {
      data[i] = global_implicit_map[data[i] - 1];
    }
  }

  GL_IdVector gather_nodes_to_proc0(Ioss::Map &global_id_map, int processor, int64_t offset,
                                    const Ioss::ParallelUtils &util, size_t min_id,
                                    size_t max_id = std::numeric_limits<size_t>::max())
  {
    GL_IdVector I_nodes;
    GL_IdVector I_nodes_recv;
    for (size_t i = 0; i < global_id_map.size(); i++) {
      auto global_id = global_id_map.map()[i + 1];
      if ((size_t)global_id >= min_id && (size_t)global_id <= max_id) {
        I_nodes.emplace_back((int)global_id, (int)i + 1 + offset);
      }
    }

    std::vector<int> recv_count;
    util.gather(2 * (int)I_nodes.size(), recv_count);
    std::vector<int> recv_off(recv_count);

    auto count = std::accumulate(recv_count.begin(), recv_count.end(), 0);
    if (processor == 0) {
      I_nodes_recv.resize(count / 2);
      Ioss::Utils::generate_index(recv_off);
    }

    MPI_Gatherv(I_nodes.data(), (int)I_nodes.size() * 2, MPI_INT, I_nodes_recv.data(),
                recv_count.data(), recv_off.data(), MPI_INT, 0, util.communicator());

    if (processor == 0) {
      Ioss::sort(I_nodes_recv.begin(), I_nodes_recv.end());
    }
    return I_nodes_recv;
  }

  GL_IdVector intersect(const GL_IdVector &I, const GL_IdVector &J)
  {
    // Find all common global_ids (entry.first) between 'I' and 'J'.
    // When found, store the proc-zone-local position (entry.second)
    // in 'common'
    // PRECONDITION: 'I' and 'J' are sorted.

    auto        min_size = std::min(I.size(), J.size());
    GL_IdVector common;
    common.reserve(min_size);

    auto II = I.begin();
    auto JJ = J.begin();
    while (II != I.end() && JJ != J.end()) {
      if ((*II).first < (*JJ).first) {
        ++II;
      }
      else {
        if (!((*JJ).first < (*II).first)) {
          common.emplace_back((*II).second, (*JJ).second);
          ++II;
        }
        ++JJ;
      }
    }
    common.shrink_to_fit();
    return common;
  }
} // namespace

namespace Iocgns {

  ParallelDatabaseIO::ParallelDatabaseIO(Ioss::Region *region, const std::string &filename,
                                         Ioss::DatabaseUsage db_usage, MPI_Comm communicator,
                                         const Ioss::PropertyManager &props)
      : Ioss::DatabaseIO(region, filename, db_usage, communicator, props)
  {
    usingParallelIO = true;
    dbState         = Ioss::STATE_UNKNOWN;

#if IOSS_DEBUG_OUTPUT
    if (myProcessor == 0) {
      fmt::print("CGNS ParallelDatabaseIO using {}-bit integers.\n"
                 "                        using the parallel CGNS library and API.\n",
                 CG_SIZEOF_SIZE);
    }
#endif
    if (!is_input()) {
      if (properties.exists("FLUSH_INTERVAL")) {
        m_flushInterval = properties.get("FLUSH_INTERVAL").get_int();
      }

      {
        bool file_per_state = false;
        Ioss::Utils::check_set_bool_property(properties, "FILE_PER_STATE", file_per_state);
        if (file_per_state) {
          set_file_per_state(true);
        }
      }
    }

    Ioss::DatabaseIO::openDatabase__();
  }

  ParallelDatabaseIO::~ParallelDatabaseIO()
  {
    for (auto &gtb : m_globalToBlockLocalNodeMap) {
      delete gtb.second;
    }
    try {
      closeBaseDatabase__();
      closeDatabase__();
    }
    catch (...) {
    }
  }

  int ParallelDatabaseIO::get_file_pointer() const
  {
    if (m_cgnsFilePtr < 0) {
      openDatabase__();
    }
    return m_cgnsFilePtr;
  }

  void ParallelDatabaseIO::openDatabase__() const
  {
    if (m_cgnsFilePtr < 0) {
      int mode = is_input() ? CG_MODE_READ : CG_MODE_WRITE;
      if (!is_input()) {
        if (m_cgnsFilePtr == -2) {
          // Writing multiple steps with a "flush" (cg_close() / cg_open())
          mode = CG_MODE_MODIFY;
        }
        else {
          // Check whether appending to existing file...
          if (open_create_behavior() == Ioss::DB_APPEND ||
              open_create_behavior() == Ioss::DB_MODIFY) {
            // Append to file if it already exists -- See if the file exists.
            Ioss::FileInfo file = Ioss::FileInfo(decoded_filename());
            if (file.exists()) {
              mode = CG_MODE_MODIFY;
            }
          }
        }
      }

      bool do_timer = false;
      Ioss::Utils::check_set_bool_property(properties, "IOSS_TIME_FILE_OPEN_CLOSE", do_timer);
      double t_begin = (do_timer ? Ioss::Utils::timer() : 0);

      CGCHECKM(cg_set_file_type(CG_FILE_HDF5));

#if CGNS_VERSION > 3320
      CGCHECKM(cgp_mpi_comm(util().communicator()));
#else
      // Older versions of cgp_mpi_comm returned an internal NO_ERROR
      // value which is equal to -1.
      cgp_mpi_comm(util().communicator());
#endif
      CGCHECKM(cgp_pio_mode(CGP_COLLECTIVE));
      Ioss::DatabaseIO::openDatabase__();
      int ierr = cgp_open(get_dwname().c_str(), mode, &m_cgnsFilePtr);

      if (do_timer) {
        double t_end    = Ioss::Utils::timer();
        double duration = util().global_minmax(t_end - t_begin, Ioss::ParallelUtils::DO_MAX);
        if (myProcessor == 0) {
          fmt::print(Ioss::DEBUG(), "{} File Open Time = {}\n", is_input() ? "Input" : "Output",
                     duration);
        }
      }

      if (ierr != CG_OK) {
        // NOTE: Code will not continue past this call...
        std::ostringstream errmsg;
        fmt::print(errmsg, "ERROR: Problem opening file '{}' for {} access. CGNS Error: '{}'",
                   get_filename(), (is_input() ? "read" : "write"), cg_get_error());
        IOSS_ERROR(errmsg);
      }

      if (properties.exists("INTEGER_SIZE_API")) {
        int isize = properties.get("INTEGER_SIZE_API").get_int();
        if (isize == 8) {
          set_int_byte_size_api(Ioss::USE_INT64_API);
        }
        if (isize == 4) {
          set_int_byte_size_api(Ioss::USE_INT32_API);
        }
      }
      else if (CG_SIZEOF_SIZE == 64) {
        set_int_byte_size_api(Ioss::USE_INT64_API);
      }

      if (mode == CG_MODE_MODIFY && get_region() != nullptr) {
        Utils::update_db_zone_property(m_cgnsFilePtr, get_region(), myProcessor, true, true);
      }
#if 0
      // This isn't currently working since CGNS currently has chunking
      // disabled for HDF5 files and compression requires chunking.
      if (!is_input()) {
        if (properties.exists("COMPRESSION_LEVEL")) {
          int comp = properties.get("COMPRESSION_LEVEL").get_int();
          cg_configure(CG_CONFIG_HDF5_COMPRESS, (void*)comp);
        }
      }
#endif
    }
    assert(m_cgnsFilePtr >= 0);
  }

  void ParallelDatabaseIO::closeBaseDatabase__() const
  {
    if (m_cgnsBasePtr > 0) {
      bool do_timer = false;
      Ioss::Utils::check_set_bool_property(properties, "IOSS_TIME_FILE_OPEN_CLOSE", do_timer);
      double t_begin = (do_timer ? Ioss::Utils::timer() : 0);

      CGCHECKM(cg_close(m_cgnsBasePtr));
      m_cgnsBasePtr = -1;

      if (do_timer) {
        double t_end    = Ioss::Utils::timer();
        double duration = util().global_minmax(t_end - t_begin, Ioss::ParallelUtils::DO_MAX);
        if (myProcessor == 0) {
          fmt::print(Ioss::DEBUG(), "{} Base File Close Time = {}\n",
                     is_input() ? "Input" : "Output", duration);
        }
      }
    }
  }

  void ParallelDatabaseIO::closeDatabase__() const
  {
    if (m_cgnsFilePtr > 0) {
      bool do_timer = false;
      Ioss::Utils::check_set_bool_property(properties, "IOSS_TIME_FILE_OPEN_CLOSE", do_timer);
      double t_begin = (do_timer ? Ioss::Utils::timer() : 0);

      CGCHECKM(cgp_close(m_cgnsFilePtr));

      if (do_timer) {
        double t_end    = Ioss::Utils::timer();
        double duration = util().global_minmax(t_end - t_begin, Ioss::ParallelUtils::DO_MAX);
        if (myProcessor == 0) {
          fmt::print(Ioss::DEBUG(), "{} File Close Time = {}\n", is_input() ? "Input" : "Output",
                     duration);
        }
      }
      closeDW();
      m_cgnsFilePtr = -1;
    }
  }

  void ParallelDatabaseIO::finalize_database() const
  {
    if (is_input()) {
      return;
    }

    if (m_timesteps.empty()) {
      return;
    }

    if (!m_dbFinalized) {
      int file_ptr;
      if (get_file_per_state()) {
        file_ptr = m_cgnsBasePtr;
      }
      else {
        file_ptr = get_file_pointer();
      }
      Utils::finalize_database(file_ptr, m_timesteps, get_region(), myProcessor, true);
      m_dbFinalized = true;
    }
  }

  void ParallelDatabaseIO::release_memory__()
  {
    nodeMap.release_memory();
    elemMap.release_memory();
    try {
      decomp.reset();
    }
    catch (...) {
    }
  }

  int64_t ParallelDatabaseIO::node_global_to_local__(int64_t global, bool /* must_exist */) const
  {
    // TODO: Fix
    return global;
  }

  int64_t ParallelDatabaseIO::element_global_to_local__(int64_t global) const
  {
    // TODO: Fix
    return global;
  }

  void ParallelDatabaseIO::read_meta_data__()
  {
    openDatabase__();

    // Determine the number of bases in the grid.
    // Currently only handle 1.
    int n_bases = 0;
    CGCHECKM(cg_nbases(get_file_pointer(), &n_bases));
    if (n_bases != 1) {
      std::ostringstream errmsg;
      fmt::print(errmsg,
                 "CGNS: Too many bases ({}); only support files with a single base at this time",
                 n_bases);
      IOSS_ERROR(errmsg);
    }

    get_step_times__();

    if (open_create_behavior() == Ioss::DB_APPEND) {
      return;
    }

    m_meshType = Utils::check_mesh_type(get_file_pointer());

    // In CGNS, there are duplicated nodes at block boundaries.
    // We typically only want to retain one copy of these and ignore the other.
    properties.add(Ioss::Property("RETAIN_FREE_NODES", "NO"));

    if (int_byte_size_api() == 8) {
      decomp = std::unique_ptr<DecompositionDataBase>(
          new DecompositionData<int64_t>(properties, util().communicator()));
    }
    else {
      decomp = std::unique_ptr<DecompositionDataBase>(
          new DecompositionData<int>(properties, util().communicator()));
    }
    assert(decomp != nullptr);
    decomp->decompose_model(get_file_pointer(), m_meshType);

    // ========================================================================
    // Get the number of assemblies in the mesh...
    // Will be the 'families' that contain nodes of 'FamVC_*'
    Utils::add_assemblies(get_file_pointer(), this);

    if (m_meshType == Ioss::MeshType::STRUCTURED) {
      handle_structured_blocks();
    }
    else if (m_meshType == Ioss::MeshType::UNSTRUCTURED) {
      handle_unstructured_blocks();
    }
#if IOSS_ENABLE_HYBRID
    else if (mesh_type == Ioss::MeshType::HYBRID) {
    }
#endif
    else {
      std::ostringstream errmsg;
      fmt::print(errmsg, "ERROR: CGNS: Mesh is not Unstructured or Structured which are the only "
                         "types currently supported");
      IOSS_ERROR(errmsg);
    }

    Utils::add_transient_variables(get_file_pointer(), m_timesteps, get_region(), myProcessor,
                                   true);
  }

  void ParallelDatabaseIO::handle_unstructured_blocks()
  {
    get_region()->property_add(
        Ioss::Property("global_node_count", (int64_t)decomp->global_node_count()));
    get_region()->property_add(
        Ioss::Property("global_element_count", (int64_t)decomp->global_elem_count()));

    nodeCount    = decomp->ioss_node_count();
    elementCount = decomp->ioss_elem_count();

    // ========================================================================
    // Get the number of families in the mesh...
    // Will treat these as sidesets if they are of the type "FamilyBC_t"
    Utils::add_sidesets(get_file_pointer(), this);

    // ========================================================================
    // Get the number of zones (element blocks) in the mesh...
    int base = 1;
    int i    = 0;
    for (auto &block : decomp->m_elementBlocks) {
      std::string element_topo = block.topologyType;
      auto *eblock = new Ioss::ElementBlock(this, block.name(), element_topo, block.ioss_count());
      eblock->property_add(Ioss::Property("base", base));
      eblock->property_add(Ioss::Property("zone", block.zone()));
      eblock->property_add(Ioss::Property("id", block.zone()));
      eblock->property_add(Ioss::Property("guid", util().generate_guid(block.zone())));
      eblock->property_add(Ioss::Property("section", block.section()));
      eblock->property_add(Ioss::Property("original_block_order", i++));
      get_region()->add(eblock);
#if IOSS_DEBUG_OUTPUT
      fmt::print(Ioss::DEBUG(), "Added block {}, IOSS topology = '{}' with {} element.\n",
                 block.name(), element_topo, block.ioss_count());
#endif
      // See if this zone/block is a member of any assemblies...
      Utils::add_to_assembly(get_file_pointer(), get_region(), eblock, base, block.zone());
    }

    // ========================================================================
    // Have sidesets, now create sideblocks for each sideset...
    int id = 0;
    for (auto &sset : decomp->m_sideSets) {
      // See if there is an Ioss::SideSet with a matching name...
      Ioss::SideSet *ioss_sset = get_region()->get_sideset(sset.ss_name());
      if (ioss_sset != nullptr) {
        auto        zone       = decomp->m_zones[sset.zone()];
        std::string block_name = fmt::format("{}/{}", zone.m_name, sset.name());
        std::string face_topo  = sset.topologyType;
#if IOSS_DEBUG_OUTPUT
        fmt::print(Ioss::DEBUG(),
                   "Processor {}: Added sideblock '{}' of topo {} with {} faces to sset '{}'\n",
                   myProcessor, block_name, face_topo, sset.ioss_count(), ioss_sset->name());
#endif
        const auto &block = decomp->m_elementBlocks[sset.parentBlockIndex];

        std::string      parent_topo = block.topologyType;
        Ioss::SideBlock *sblk =
            new Ioss::SideBlock(this, block_name, face_topo, parent_topo, sset.ioss_count());
        sblk->property_add(Ioss::Property("id", id));
        sblk->property_add(Ioss::Property("guid", util().generate_guid(id + 1)));
        sblk->property_add(Ioss::Property("base", 1));
        sblk->property_add(Ioss::Property("zone", sset.zone()));
        sblk->property_add(Ioss::Property("section", sset.section()));
        Ioss::ElementBlock *eblock = get_region()->get_element_block(block.name());
        if (eblock != nullptr) {
          sblk->set_parent_element_block(eblock);
        }
        ioss_sset->add(sblk);
      }
      id++; // Really just index into m_sideSets list.
    }

    auto *nblock = new Ioss::NodeBlock(this, "nodeblock_1", nodeCount, 3);

    nblock->property_add(Ioss::Property("base", base));
    get_region()->add(nblock);

    // Create a single node commset
    auto *commset =
        new Ioss::CommSet(this, "commset_node", "node", decomp->get_commset_node_size());
    commset->property_add(Ioss::Property("id", 1));
    commset->property_add(Ioss::Property("guid", util().generate_guid(1)));

    get_region()->add(commset);
  }

  size_t ParallelDatabaseIO::finalize_structured_blocks()
  {
    // If there are any Structured blocks, need to iterate them and their 1-to-1 connections
    // and update the donor_zone id for zones that had not yet been processed at the time of
    // definition...
    const auto &blocks = get_region()->get_structured_blocks();
    for (auto &block : blocks) {
      int64_t guid = block->get_property("guid").get_int();
      for (auto &conn : block->m_zoneConnectivity) {
        if (conn.m_donorZone < 0) {
          auto donor_iter = m_zoneNameMap.find(conn.m_donorName);
          assert(donor_iter != m_zoneNameMap.end());
          conn.m_donorZone = (*donor_iter).second;
        }
        conn.m_donorGUID = util().generate_guid(conn.m_donorZone, conn.m_donorProcessor);
        conn.m_ownerGUID = guid;
      }
    }

    size_t num_nodes = Utils::resolve_nodes(*get_region(), myProcessor, true);
    return num_nodes;
  }

  void ParallelDatabaseIO::handle_structured_blocks()
  {
    int base = 1;

    Utils::add_sidesets(get_file_pointer(), this);

    char basename[CGNS_MAX_NAME_LENGTH + 1];
    int  cell_dimension = 0;
    int  phys_dimension = 0;
    CGCHECKM(cg_base_read(get_file_pointer(), base, basename, &cell_dimension, &phys_dimension));

    // Iterate all structured blocks and set the intervals to zero
    // if the m_proc field does not match current processor...
    const auto &zones = decomp->m_structuredZones;

    for (auto &zone : zones) {
      if (zone->m_adam == zone) {
        // This is a "root" zone from the undecomposed mesh...
        // Now see if there are any non-empty blocks with
        // this m_adam on this processor.  If exists, then create
        // a StructuredBlock; otherwise, create an empty block.
        auto block_name = zone->m_name;

        Ioss::StructuredBlock *block = nullptr;
        Ioss::IJK_t            zeros{{0, 0, 0}};
        for (auto &pzone : zones) {
          if (pzone->m_proc == myProcessor && pzone->m_adam == zone) {
            // Create a non-empty structured block on this processor...
            block = new Ioss::StructuredBlock(this, block_name, phys_dimension, pzone->m_ordinal,
                                              pzone->m_offset, pzone->m_adam->m_ordinal);

            for (auto &zgc : pzone->m_zoneConnectivity) {
              // Update donor_zone to point to adam zone instead of child.
              auto dz = zones[zgc.m_donorZone - 1];
              assert(dz->m_zone == zgc.m_donorZone);
              auto oz = zones[zgc.m_ownerZone - 1];
              assert(oz->m_zone == zgc.m_ownerZone);
              zgc.m_donorZone = dz->m_adam->m_zone;
              zgc.m_ownerZone = oz->m_adam->m_zone;
              block->m_zoneConnectivity.push_back(zgc);
            }
            break;
          }
        }
        if (block == nullptr) {
          // There is no block on this processor corresponding to the m_adam
          // block.  Create an empty block...
          block = new Ioss::StructuredBlock(this, block_name, phys_dimension, zeros, zeros,
                                            zone->m_adam->m_ordinal);
          for (auto &zgc : zone->m_zoneConnectivity) {
            zgc.m_isActive = false;
            // Update donor_zone to point to adam zone instead of child.
            auto dz = zones[zgc.m_donorZone - 1];
            assert(dz->m_zone == zgc.m_donorZone);
            auto oz = zones[zgc.m_ownerZone - 1];
            assert(oz->m_zone == zgc.m_ownerZone);
            zgc.m_donorZone = dz->m_adam->m_zone;
            zgc.m_ownerZone = oz->m_adam->m_zone;
            block->m_zoneConnectivity.push_back(zgc);
          }
        }
        assert(block != nullptr);
        get_region()->add(block);

        block->property_add(Ioss::Property("base", base));
        block->property_add(Ioss::Property("zone", zone->m_adam->m_zone));
        block->property_add(Ioss::Property("db_zone", zone->m_adam->m_zone));
        block->property_add(Ioss::Property("id", zone->m_adam->m_zone));
        int64_t guid = util().generate_guid(zone->m_adam->m_zone);
        block->property_add(Ioss::Property("guid", guid));

        // See if this zone/block is a member of any assemblies...
        Utils::add_to_assembly(get_file_pointer(), get_region(), block, base, zone->m_adam->m_zone);

#if IOSS_DEBUG_OUTPUT
        fmt::print(Ioss::DEBUG(), "Added block {}, Structured with ID = {}, GUID = {}\n",
                   block_name, zone->m_adam->m_zone, guid);
#endif
      }
    }

    // ========================================================================
    // Iterate each StructuredBlock, get its zone. For that zone, get the number of
    // boundary conditions and then iterate those and create sideblocks in the
    // corresponding sideset.
    const auto &sbs = get_region()->get_structured_blocks();
    for (const auto &block : sbs) {
      // Handle boundary conditions...
      Utils::add_structured_boundary_conditions(get_file_pointer(), block, true);
    }

    size_t node_count = finalize_structured_blocks();
    auto  *nblock     = new Ioss::NodeBlock(this, "nodeblock_1", node_count, phys_dimension);
    nblock->property_add(Ioss::Property("base", base));
    get_region()->add(nblock);
  }

  void ParallelDatabaseIO::resolve_zone_shared_nodes(const CGNSIntVector &nodes,
                                                     CGNSIntVector       &connectivity_map,
                                                     size_t              &owned_node_count,
                                                     size_t              &owned_node_offset) const
  {
    // Determine number of processors that have nodes for this zone.
    // Avoid mpi_comm_split call if possible.
    int have_nodes             = nodes.empty() ? 0 : 1;
    int shared_zone_proc_count = 0;
    MPI_Allreduce(&have_nodes, &shared_zone_proc_count, 1, Ioss::mpi_type(int(0)), MPI_SUM,
                  util().communicator());

    if (shared_zone_proc_count <= 1) {
      // There are no shared nodes in this zone.
      owned_node_count = nodes.size();
      std::iota(connectivity_map.begin(), connectivity_map.end(), 1);
      return;
    }

    have_nodes = have_nodes == 0 ? MPI_UNDEFINED : 1;
    MPI_Comm pcomm;
    MPI_Comm_split(util().communicator(), have_nodes, myProcessor, &pcomm);

    if (have_nodes == 1) {
      // From here on down, only processors that have nodes are involved...
      // This zone has nodes/cells on two or more processors.  Need to determine
      // which nodes are shared.

      Ioss::ParallelUtils pm(pcomm);
      size_t              proc_count = pm.parallel_size();
      assert((int)proc_count == shared_zone_proc_count);

      // Distribute each node to an "owning" processor based on its id
      // and assuming a linear distribution (e.g., if on 3 processors, each
      // proc will "own" 1/3 of the id range.
      // nodes is sorted.
      int64_t min = nodes[0];
      int64_t max = nodes.back();
      min         = pm.global_minmax(min, Ioss::ParallelUtils::DO_MIN);
      max         = pm.global_minmax(max, Ioss::ParallelUtils::DO_MAX);

      std::vector<int> p_count(proc_count);
      size_t           per_proc = (max - min + proc_count) / proc_count;
      size_t           proc     = 0;
      int64_t          top      = min + per_proc;

      // NOTE: nodes is sorted...
      for (auto &node : nodes) {
        while (node >= top) {
          top += per_proc;
          proc++;
        }
        assert(proc < proc_count);
        p_count[proc]++;
      }

      // Tell each processor how many nodes it will be getting...
      // Each processor will be responsible for a subsetted range of the overall range.
      // This processor, should range from min + my_proc*per_proc to min + (my_proc+1)*per_proc.
      std::vector<int> r_count(proc_count);
      MPI_Alltoall(p_count.data(), 1, Ioss::mpi_type(int(0)), r_count.data(), 1,
                   Ioss::mpi_type(int(0)), pcomm);

      std::vector<int> recv_disp(proc_count);
      std::vector<int> send_disp(proc_count);
      size_t           sums = 0;
      size_t           sumr = 0;
      for (size_t p = 0; p < proc_count; p++) {
        recv_disp[p] = sumr;
        sumr += r_count[p];

        send_disp[p] = sums;
        sums += p_count[p];
      }
      CGNSIntVector r_nodes(sumr);
      Ioss::MY_Alltoallv(nodes, p_count, send_disp, r_nodes, r_count, recv_disp, pcomm);

      // Iterate r_nodes list to find duplicate nodes...
      auto             delta = min + pm.parallel_rank() * per_proc;
      std::vector<int> dup_nodes(per_proc);
      for (auto &r_node : r_nodes) {
        auto n = r_node - delta;
        assert(n < per_proc);
        dup_nodes[n]++;
        if (dup_nodes[n] > 1) {
          r_node = 0;
        }
      }

      // Send filtered list back to original processors -- store in 'u_nodes'
      // This is set of unique block nodes owned by this processor.
      // If an entry in 'u_nodes' is 0, then that is a non-owned shared node.
      CGNSIntVector u_nodes(nodes.size());
      Ioss::MY_Alltoallv(r_nodes, r_count, recv_disp, u_nodes, p_count, send_disp, pcomm);

      // Count non-zero entries in u_nodes...
      int64_t local_node_count =
          std::count_if(u_nodes.cbegin(), u_nodes.cend(), [](int64_t i) { return i > 0; });
      owned_node_count = local_node_count; // Calling code wants to know this

      // Determine offset into the zone node block for each processors "chunk"
      int64_t local_node_offset = 0;
      MPI_Exscan(&local_node_count, &local_node_offset, 1, Ioss::mpi_type(local_node_count),
                 MPI_SUM, pcomm);
      owned_node_offset = local_node_offset; // Calling code wants to know this

      // This generates the position of each owned node in this zone consistent
      // over all processors that this zone is active on.
      for (auto &u_node : u_nodes) {
        if (u_node > 0) {
          u_node = ++local_node_offset; // 1-based local node id for all owned nodes.
        }
      }

      // u_nodes now contains the global -> block-local node map for all owned nodes
      // on the processor.
      // The zeroes in u_nodes are shared nodes on the processor boundary.
      // Resend nodes and u_nodes so can resolve the ids of the shared nodes.
      CGNSIntVector g_to_zone_local(sumr);
      Ioss::MY_Alltoallv(nodes, p_count, send_disp, r_nodes, r_count, recv_disp, pcomm);
      Ioss::MY_Alltoallv(u_nodes, p_count, send_disp, g_to_zone_local, r_count, recv_disp, pcomm);

      // Iterate g_to_zone_local to find a zero entry.
      for (size_t i = 0; i < g_to_zone_local.size(); i++) {
        if (g_to_zone_local[i] == 0) {
          // The global id is r_nodes[i] which must also appear earlier in the list...
          for (size_t j = 0; j < i; j++) {
            if (r_nodes[j] == r_nodes[i]) {
              g_to_zone_local[i] = g_to_zone_local[j];
              break;
            }
          }
        }
      }

      // Now, send updated g_to_zone_local back to original processors...
      Ioss::MY_Alltoallv(g_to_zone_local, r_count, recv_disp, u_nodes, p_count, send_disp, pcomm);

// At this point:
//   'nodes' contains the global node ids that are referenced in this zones connectivity.
//   'u_nodes' contains the zone-local 1-based position of that node in this zones node list.
//
//
#ifndef NDEBUG
      for (auto &u_node : u_nodes) {
        assert(u_node > 0);
      }
#endif
      std::swap(connectivity_map, u_nodes);
      MPI_Comm_free(&pcomm);
    }
  }

  void ParallelDatabaseIO::write_meta_data()
  {
    int num_zones = get_region()->get_property("element_block_count").get_int() +
                    get_region()->get_property("structured_block_count").get_int();
    m_bcOffset.resize(num_zones + 1);   // use 1-based zones...
    m_zoneOffset.resize(num_zones + 1); // use 1-based zones...

    elementCount =
        Utils::common_write_meta_data(get_file_pointer(), *get_region(), m_zoneOffset, true);
  }

  void ParallelDatabaseIO::get_step_times__()
  {
    Utils::get_step_times(get_file_pointer(), m_timesteps, get_region(), timeScaleFactor,
                          myProcessor);
  }

  void ParallelDatabaseIO::write_adjacency_data()
  {
    // Determine adjacency information between unstructured blocks.
    // If block I is adjacent to block J, then they will share at
    // least 1 "side" (face 3D or edge 2D).
    // Currently, assuming they are adjacent if they share at least one node...

    // TODO: All calculations are done on processor 0 instead of being distributed.
    //       this will not scale well...

    const auto &blocks = get_region()->get_element_blocks();
    if (blocks.size() <= 1) {
      return; // No adjacent blocks if only one block...
    }

    // =================
    // Determine the minimum and maximum global node id for each zone.
    // This will be used When determining whether 2 zones are
    // connected by checking whether the global id node ranges overlap
    std::vector<int64_t> zone_min_id(blocks.size() + 1, std::numeric_limits<int64_t>::max());
    std::vector<int64_t> zone_max_id(blocks.size() + 1, std::numeric_limits<int64_t>::min());

    for (const auto &block : blocks) {
      int zone = block->get_property("zone").get_int();
      assert((size_t)zone < blocks.size() + 1);

      const auto &I_map = m_globalToBlockLocalNodeMap[zone];

      // Get min and max global node id for each zone...
      if (I_map->size() > 0) {
        auto min_max = std::minmax_element(std::next(I_map->map().begin()), I_map->map().end());
        zone_min_id[zone] = *(min_max.first);
        zone_max_id[zone] = *(min_max.second);
      }
    }

    // Get min/max over all processors for each zone...
    util().global_array_minmax(zone_min_id, Ioss::ParallelUtils::DO_MIN);
    util().global_array_minmax(zone_max_id, Ioss::ParallelUtils::DO_MAX);
    // =================

    auto node_offset = get_processor_zone_node_offset();

    // Now iterate the blocks again.  If the node ranges overlap, then
    // there is a possibility that there are contiguous nodes; if the
    // ranges don't overlap, then no possibility...
    for (auto I = blocks.cbegin(); I != std::prev(blocks.cend()); I++) {
      int base = (*I)->get_property("base").get_int();
      int zone = (*I)->get_property("zone").get_int();

      // See how many zone I nodes Proc x has that can potentially
      // overlap with the zones I+1 to end.  This will be all nodes
      // with id > min(zone_min_id[I+1..])
      auto min_I   = std::min_element(&zone_min_id[zone + 1], &zone_min_id[blocks.size()]);
      auto I_nodes = gather_nodes_to_proc0(*m_globalToBlockLocalNodeMap[zone], myProcessor,
                                           node_offset[zone - 1], util(), *min_I);

      for (auto J = I + 1; J != blocks.end(); J++) {
        cgsize_t dzone = (*J)->get_property("zone").get_int();

        if (zone_min_id[dzone] <= zone_max_id[zone] && zone_max_id[dzone] >= zone_min_id[zone]) {
          // Now gather all zone J nodes that can potentially overlap
          // with zone I to processor 0...
          auto J_nodes = gather_nodes_to_proc0(*m_globalToBlockLocalNodeMap[dzone], myProcessor,
                                               node_offset[dzone - 1], util(), zone_min_id[zone],
                                               zone_max_id[zone]);
          GL_IdVector common;
          if (myProcessor == 0) {
            common = intersect(I_nodes, J_nodes);

#if IOSS_DEBUG_OUTPUT
            fmt::print(Ioss::DEBUG(), "Zone {}: {}, Donor Zone {}: {} Common: {}\n\t", zone,
                       I_nodes.size(), dzone, J_nodes.size(), common.size());

            for (const auto &p : common) {
              fmt::print(Ioss::DEBUG(), "{}, ", p.first);
            }
            fmt::print(Ioss::DEBUG(), "\n\t");
            for (const auto &p : common) {
              fmt::print(Ioss::DEBUG(), "{}, ", p.second);
            }
            fmt::print(Ioss::DEBUG(), "\n");
#endif
          }

          int size = (int)common.size();
          MPI_Bcast(&size, 1, MPI_INT, 0, util().communicator());

          if (size > 0) {
            // This 'cg_conn_write' should probably be a parallel
            // function.  Since one does not exist, we output the same
            // data on all processors.  Seems to work, but is klugy.

            common.resize(size);
            MPI_Bcast(common.data(), 2 * size, MPI_INT, 0, util().communicator());

            CGNSIntVector point_list;
            CGNSIntVector point_list_donor;
            point_list.reserve(common.size());
            point_list_donor.reserve(common.size());

            for (auto &pnt : common) {
              point_list.push_back(pnt.first);
              point_list_donor.push_back(pnt.second);
            }

            int         gc_idx  = 0;
            std::string name    = fmt::format("{}_to_{}", (*I)->name(), (*J)->name());
            const auto &d1_name = (*J)->name();

            CGCHECKM(cg_conn_write(get_file_pointer(), base, zone, name.c_str(), CGNS_ENUMV(Vertex),
                                   CGNS_ENUMV(Abutting1to1), CGNS_ENUMV(PointList),
                                   point_list.size(), point_list.data(), d1_name.c_str(),
                                   CGNS_ENUMV(Unstructured), CGNS_ENUMV(PointListDonor),
                                   CGNS_ENUMV(DataTypeNull), point_list_donor.size(),
                                   point_list_donor.data(), &gc_idx));

            name                = fmt::format("{}_to_{}", (*J)->name(), (*I)->name());
            const auto &d2_name = (*I)->name();

            CGCHECKM(cg_conn_write(
                get_file_pointer(), base, dzone, name.c_str(), CGNS_ENUMV(Vertex),
                CGNS_ENUMV(Abutting1to1), CGNS_ENUMV(PointList), point_list_donor.size(),
                point_list_donor.data(), d2_name.c_str(), CGNS_ENUMV(Unstructured),
                CGNS_ENUMV(PointListDonor), CGNS_ENUMV(DataTypeNull), point_list.size(),
                point_list.data(), &gc_idx));
          }
        }
      }
    }
  }

  bool ParallelDatabaseIO::begin__(Ioss::State state)
  {
    dbState = state;
    return true;
  }

  void ParallelDatabaseIO::free_state_pointer()
  {
    // If this is the first state file created, then we need to save a reference
    // to the base CGNS file so we can update the metadata and create links to
    // the state files (if we are using the file-per-state option)
    if (m_cgnsBasePtr < 0) {
      m_cgnsBasePtr = m_cgnsFilePtr;
      m_cgnsFilePtr = -1;
    }
    closeDatabase__();
  }

  void ParallelDatabaseIO::open_state_file(int state)
  {
    // Close current state file (if any)...
    free_state_pointer();

    // Update filename to append state count...
    decodedFilename.clear();

    Ioss::FileInfo db(originalDBFilename);
    std::string    new_filename;
    if (!db.pathname().empty()) {
      new_filename += db.pathname() + "/";
    }

    new_filename += fmt::format("{}-SolutionAtStep{:05}.{}", db.basename(), state, db.extension());

    DBFilename = new_filename;

    Iocgns::Utils::write_state_meta_data(get_file_pointer(), *get_region(), true);
  }

  bool ParallelDatabaseIO::end__(Ioss::State state)
  {
    // Transitioning out of state 'state'
    switch (state) {
    case Ioss::STATE_DEFINE_MODEL:
      if (!is_input() && open_create_behavior() != Ioss::DB_APPEND &&
          open_create_behavior() != Ioss::DB_MODIFY) {
        write_meta_data();
      }
      if (!is_input() && open_create_behavior() == Ioss::DB_APPEND) {
        Utils::update_db_zone_property(m_cgnsFilePtr, get_region(), myProcessor, isParallel, true);
      }
      break;
    case Ioss::STATE_MODEL:
      if (!is_input() && open_create_behavior() != Ioss::DB_APPEND &&
          open_create_behavior() != Ioss::DB_MODIFY) {
        write_adjacency_data();
      }
      break;
    case Ioss::STATE_DEFINE_TRANSIENT:
      if (!is_input() && open_create_behavior() != Ioss::DB_APPEND &&
          open_create_behavior() != Ioss::DB_MODIFY) {
        write_results_meta_data();
      }
      break;
    default: // ignore everything else...
      break;
    }
    return true;
  }

  bool ParallelDatabaseIO::begin_state__(int state, double /* time */)
  {
    if (is_input()) {
      return true;
    }
    if (get_file_per_state()) {
      // Close current state file (if any); create new state file and output metadata...
      open_state_file(state);
      write_results_meta_data();
    }
    Utils::write_flow_solution_metadata(get_file_pointer(), m_cgnsBasePtr, get_region(), state,
                                        &m_currentVertexSolutionIndex,
                                        &m_currentCellCenterSolutionIndex, true);
    m_dbFinalized = false;
    return true;
  }

  bool ParallelDatabaseIO::end_state__(int state, double time)
  {
    if (!is_input()) {
      m_timesteps.push_back(time);
      assert(m_timesteps.size() == (size_t)state);
    }

    if (!is_input()) {
      bool do_flush = true;
      if (m_flushInterval != 1) {
        if (m_flushInterval == 0 || state % m_flushInterval != 0) {
          do_flush = false;
        }
      }

      if (do_flush) {
        flush_database__();
      }
    }

    return true;
  }

  void ParallelDatabaseIO::flush_database__() const
  {
    // For HDF5 files, it looks like we need to close the database between
    // writes if we want to have a valid database for external access or
    // to protect against a crash corrupting the file.
    finalize_database();
    closeDatabase__();
    m_cgnsFilePtr = -2; // Tell openDatabase__ that we want to append
  }

  const Ioss::Map &ParallelDatabaseIO::get_map(entity_type type) const
  {
    if (m_meshType == Ioss::MeshType::UNSTRUCTURED) {
      switch (type) {
      case entity_type::NODE: {
        size_t offset = decomp->decomp_node_offset();
        size_t count  = decomp->decomp_node_count();
        return get_map(nodeMap, nodeCount, offset, count, entity_type::NODE);
      }
      case entity_type::ELEM: {
        size_t offset = decomp->decomp_elem_offset();
        size_t count  = decomp->decomp_elem_count();
        return get_map(elemMap, elementCount, offset, count, entity_type::ELEM);
      }
      }
    }
    else {
      assert(1 == 0);
    }
    std::ostringstream errmsg;
    fmt::print(errmsg, "INTERNAL ERROR: Invalid map type. "
                       "Something is wrong in the Iocgns::ParallelDatabaseIO::get_map() function. "
                       "Please report.\n");
    IOSS_ERROR(errmsg);
  }

  const Ioss::Map &ParallelDatabaseIO::get_map(Ioss::Map &entity_map, int64_t entity_count,
                                               int64_t file_offset, int64_t file_count,
                                               entity_type type) const

  {
    // Allocate space for node number map and read it in...
    // Can be called multiple times, allocate 1 time only
    if (entity_map.map().empty()) {
      entity_map.set_size(entity_count);

      if (is_input()) {
        Ioss::MapContainer file_data(file_count);

        // For cgns, my file_data is just nodes from file_offset to file_offset+file_count
        std::iota(file_data.begin(), file_data.end(), file_offset + 1);

        if (type == entity_type::NODE)
          decomp->communicate_node_data(file_data.data(), &entity_map.map()[1], 1);
        else if (type == entity_type::ELEM)
          decomp->communicate_element_data(file_data.data(), &entity_map.map()[1], 1);

        // Check for sequential node map.
        // If not, build the reverse G2L node map...
        entity_map.is_sequential(true);
        entity_map.build_reverse_map();
      }
      else {
        // Output database; entity_map.map not set yet... Build a default map.
        entity_map.set_default(entity_count);
      }
    }
    return entity_map;
  }

  int64_t ParallelDatabaseIO::get_field_internal(const Ioss::Region * /* reg */,
                                                 const Ioss::Field & /* field */, void * /* data */,
                                                 size_t /* data_size */) const
  {
    return -1;
  }

  int64_t ParallelDatabaseIO::get_field_internal(const Ioss::NodeBlock *nb,
                                                 const Ioss::Field &field, void *data,
                                                 size_t data_size) const
  {
    // A CGNS DatabaseIO object can have two "types" of NodeBlocks:
    // * The normal "all nodes in the model" NodeBlock as used by Exodus
    // * A "nodes in a zone" NodeBlock which contains the subset of nodes
    //   "owned" by a specific StructuredBlock or ElementBlock zone.
    //
    // Question: How to determine if the NodeBlock is the "global" Nodeblock
    // or a "sub" NodeBlock: Use the "is_nonglobal_nodeblock()" function.
    if (nb->is_nonglobal_nodeblock()) {
      return get_field_internal_sub_nb(nb, field, data, data_size);
    }

    size_t num_to_get = field.verify(data_size);

    Ioss::Field::RoleType role = field.get_role();
    if (role == Ioss::Field::MESH) {
      if (field.get_name() == "mesh_model_coordinates_x" ||
          field.get_name() == "mesh_model_coordinates_y" ||
          field.get_name() == "mesh_model_coordinates_z" ||
          field.get_name() == "mesh_model_coordinates") {
        decomp->get_node_coordinates(get_file_pointer(), (double *)data, field);
      }

      else if (field.get_name() == "ids") {
        // Map the local ids in this node block
        // (1...node_count) to global node ids.
        get_map(entity_type::NODE).map_implicit_data(data, field, num_to_get, 0);
      }
      // The 1..global_node_count id.  In a parallel-decomposed run,
      // it maps the node back to its implicit position in the serial
      // undecomposed mesh file.  This is ONLY provided for backward-
      // compatibility and should not be used unless absolutely required.
      else if (field.get_name() == "implicit_ids") {
        size_t offset = decomp->decomp_node_offset();
        size_t count  = decomp->decomp_node_count();
        if (int_byte_size_api() == 4) {
          std::vector<int> file_ids(count);
          std::iota(file_ids.begin(), file_ids.end(), offset + 1);
          decomp->communicate_node_data(file_ids.data(), (int *)data, 1);
        }
        else {
          std::vector<int64_t> file_ids(count);
          std::iota(file_ids.begin(), file_ids.end(), offset + 1);
          decomp->communicate_node_data(file_ids.data(), (int64_t *)data, 1);
        }
      }

      else if (field.get_name() == "connectivity") {
        // Do nothing, just handles an idiosyncrasy of the GroupingEntity
      }
      else if (field.get_name() == "connectivity_raw") {
        // Do nothing, just handles an idiosyncrasy of the GroupingEntity
      }
      else if (field.get_name() == "owning_processor") {
        // If parallel, then set the "locally_owned" property on the nodeblocks.
        Ioss::CommSet *css = get_region()->get_commset("commset_node");
        if (int_byte_size_api() == 8) {
          auto *idata = static_cast<int64_t *>(data);
          std::fill(idata, idata + nodeCount, myProcessor);

          // Cannot call:
          //    `css->get_field_data("entity_processor_raw", ent_proc);`
          // directly since it will cause a deadlock (in threaded code),
          // expand out into corresponding `get_field_internal` call.
          Ioss::Field          ep_field = css->get_field("entity_processor_raw");
          std::vector<int64_t> ent_proc(ep_field.raw_count() *
                                        ep_field.raw_storage()->component_count());
          size_t               ep_data_size = ent_proc.size() * sizeof(int64_t);
          get_field_internal(css, ep_field, ent_proc.data(), ep_data_size);
          for (size_t i = 0; i < ent_proc.size(); i += 2) {
            int64_t node = ent_proc[i + 0];
            int64_t proc = ent_proc[i + 1];
            if (proc < idata[node - 1]) {
              idata[node - 1] = proc;
            }
          }
        }
        else {
          int *idata = static_cast<int *>(data);
          std::fill(idata, idata + nodeCount, myProcessor);

          Ioss::Field      ep_field = css->get_field("entity_processor_raw");
          std::vector<int> ent_proc(ep_field.raw_count() *
                                    ep_field.raw_storage()->component_count());
          size_t           ep_data_size = ent_proc.size() * sizeof(int);
          get_field_internal(css, ep_field, ent_proc.data(), ep_data_size);
          for (size_t i = 0; i < ent_proc.size(); i += 2) {
            int node = ent_proc[i + 0];
            int proc = ent_proc[i + 1];
            if (proc < idata[node - 1]) {
              idata[node - 1] = proc;
            }
          }
        }
      }
      else {
        num_to_get = Ioss::Utils::field_warning(nb, field, "input");
      }
    }
    else if (role == Ioss::Field::TRANSIENT) {
      // Locate the FlowSolution node corresponding to the correct state/step/time
      // TODO: do this at read_meta_data() and store...
      int step       = get_region()->get_current_state();
      int comp_count = field.get_component_count(Ioss::Field::InOut::INPUT);

      if (comp_count == 1) {
        decomp->get_node_field(get_file_pointer(), step, Utils::index(field), (double *)data);
      }
      else {
        std::vector<double> ioss_tmp(num_to_get);
        for (int i = 0; i < comp_count; i++) {
          decomp->get_node_field(get_file_pointer(), step, Utils::index(field) + i,
                                 ioss_tmp.data());

          size_t index = i;
          auto  *rdata = static_cast<double *>(data);
          for (size_t j = 0; j < num_to_get; j++) {
            rdata[index] = ioss_tmp[j];
            index += comp_count;
          }
        }
      }
    }
    else {
      num_to_get = Ioss::Utils::field_warning(nb, field, "input");
    }
    return num_to_get;
  }

  int64_t ParallelDatabaseIO::get_field_internal_sub_nb(const Ioss::NodeBlock *nb,
                                                        const Ioss::Field &field, void *data,
                                                        size_t data_size) const
  {
    // Reads field data on a NodeBlock which is a "sub" NodeBlock -- contains the nodes for a
    // StructuredBlock instead of for the entire model.
    // Currently only TRANSIENT fields are input this way.  No valid reason, but that is the current
    // use case.

    // Get the StructuredBlock that this NodeBlock is contained in:
    const Ioss::GroupingEntity *sb         = nb->contained_in();
    int                         zone       = Iocgns::Utils::get_db_zone(sb);
    cgsize_t                    num_to_get = field.verify(data_size);

    Ioss::Field::RoleType role = field.get_role();
    if (role == Ioss::Field::TRANSIENT) {
      // Locate the FlowSolution node corresponding to the correct state/step/time
      // TODO: do this at read_meta_data() and store...
      int step = get_region()->get_current_state();

      int base = 1;
      int solution_index =
          Utils::find_solution_index(get_file_pointer(), base, zone, step, CGNS_ENUMV(Vertex));

      auto *rdata = static_cast<double *>(data);
      assert(num_to_get == sb->get_property("node_count").get_int());
      cgsize_t rmin[3] = {0, 0, 0};
      cgsize_t rmax[3] = {0, 0, 0};
      if (num_to_get > 0) {
        rmin[0] = sb->get_property("offset_i").get_int() + 1;
        rmin[1] = sb->get_property("offset_j").get_int() + 1;
        rmin[2] = sb->get_property("offset_k").get_int() + 1;

        rmax[0] = rmin[0] + sb->get_property("ni").get_int();
        rmax[1] = rmin[1] + sb->get_property("nj").get_int();
        rmax[2] = rmin[2] + sb->get_property("nk").get_int();

        assert(num_to_get ==
               (rmax[0] - rmin[0] + 1) * (rmax[1] - rmin[1] + 1) * (rmax[2] - rmin[2] + 1));
      }

      int comp_count = field.get_component_count(Ioss::Field::InOut::INPUT);
      if (comp_count == 1) {
        CGCHECKM(cg_field_read(get_file_pointer(), base, zone, solution_index,
                               field.get_name().c_str(), CGNS_ENUMV(RealDouble), rmin, rmax,
                               rdata));
      }
      else {
        std::vector<double> cgns_data(num_to_get);
        for (int i = 0; i < comp_count; i++) {
          std::string var_name = get_component_name(field, Ioss::Field::InOut::INPUT, i + 1);
          CGCHECKM(cg_field_read(get_file_pointer(), base, zone, solution_index, var_name.c_str(),
                                 CGNS_ENUMV(RealDouble), rmin, rmax, cgns_data.data()));
          for (cgsize_t j = 0; j < num_to_get; j++) {
            rdata[comp_count * j + i] = cgns_data[j];
          }
        }
      }
    }
    // Ignoring all other field role types...
    return num_to_get;
  }

  int64_t ParallelDatabaseIO::get_field_internal(const Ioss::EdgeBlock * /* nb */,
                                                 const Ioss::Field & /* field */, void * /* data */,
                                                 size_t /* data_size */) const
  {
    return -1;
  }
  int64_t ParallelDatabaseIO::get_field_internal(const Ioss::FaceBlock * /* nb */,
                                                 const Ioss::Field & /* field */, void * /* data */,
                                                 size_t /* data_size */) const
  {
    return -1;
  }

  int64_t ParallelDatabaseIO::get_field_internal(const Ioss::StructuredBlock *sb,
                                                 const Ioss::Field &field, void *data,
                                                 size_t data_size) const
  {
    Ioss::Field::RoleType role = field.get_role();
    cgsize_t              base = sb->get_property("base").get_int();
    cgsize_t              zone = sb->get_property("zone").get_int();

    cgsize_t num_to_get = field.verify(data_size);

    cgsize_t rmin[3] = {0, 0, 0};
    cgsize_t rmax[3] = {0, 0, 0};

    bool cell_field = Utils::is_cell_field(field);
    if (cell_field) {
      assert(num_to_get == sb->get_property("cell_count").get_int());
      if (num_to_get > 0) {
        rmin[0] = sb->get_property("offset_i").get_int() + 1;
        rmin[1] = sb->get_property("offset_j").get_int() + 1;
        rmin[2] = sb->get_property("offset_k").get_int() + 1;

        rmax[0] = rmin[0] + sb->get_property("ni").get_int() - 1;
        rmax[1] = rmin[1] + sb->get_property("nj").get_int() - 1;
        rmax[2] = rmin[2] + sb->get_property("nk").get_int() - 1;
      }
    }
    else {
      // cell nodal field.
      assert(num_to_get == sb->get_property("node_count").get_int());
      if (num_to_get > 0) {
        rmin[0] = sb->get_property("offset_i").get_int() + 1;
        rmin[1] = sb->get_property("offset_j").get_int() + 1;
        rmin[2] = sb->get_property("offset_k").get_int() + 1;

        rmax[0] = rmin[0] + sb->get_property("ni").get_int();
        rmax[1] = rmin[1] + sb->get_property("nj").get_int();
        rmax[2] = rmin[2] + sb->get_property("nk").get_int();
      }
    }

    assert(num_to_get == 0 || num_to_get == (rmax[0] - rmin[0] + 1) * (rmax[1] - rmin[1] + 1) *
                                                (rmax[2] - rmin[2] + 1));
    double *rdata = num_to_get > 0 ? static_cast<double *>(data) : nullptr;

    if (role == Ioss::Field::MESH) {

      if (field.get_name() == "mesh_model_coordinates_x") {
        CGCHECKM(cgp_coord_read_data(get_file_pointer(), base, zone, 1, rmin, rmax, rdata));
      }

      else if (field.get_name() == "mesh_model_coordinates_y") {
        CGCHECKM(cgp_coord_read_data(get_file_pointer(), base, zone, 2, rmin, rmax, rdata));
      }

      else if (field.get_name() == "mesh_model_coordinates_z") {
        CGCHECKM(cgp_coord_read_data(get_file_pointer(), base, zone, 3, rmin, rmax, rdata));
      }

      else if (field.get_name() == "mesh_model_coordinates") {
        char basename[CGNS_MAX_NAME_LENGTH + 1];
        int  cell_dimension = 0;
        int  phys_dimension = 0;
        CGCHECKM(
            cg_base_read(get_file_pointer(), base, basename, &cell_dimension, &phys_dimension));

        // Data required by upper classes store x0, y0, z0, ... xn,
        // yn, zn. Data stored in cgns file is x0, ..., xn, y0,
        // ..., yn, z0, ..., zn so we have to allocate some scratch
        // memory to read in the data and then map into supplied
        // 'data'
        std::vector<double> coord(num_to_get);
        CGCHECKM(cgp_coord_read_data(get_file_pointer(), base, zone, 1, rmin, rmax, coord.data()));

        // Map to global coordinate position...
        for (cgsize_t i = 0; i < num_to_get; i++) {
          rdata[phys_dimension * i + 0] = coord[i];
        }

        if (phys_dimension >= 2) {
          CGCHECKM(
              cgp_coord_read_data(get_file_pointer(), base, zone, 2, rmin, rmax, coord.data()));

          // Map to global coordinate position...
          for (cgsize_t i = 0; i < num_to_get; i++) {
            rdata[phys_dimension * i + 1] = coord[i];
          }
        }

        if (phys_dimension == 3) {
          CGCHECKM(
              cgp_coord_read_data(get_file_pointer(), base, zone, 3, rmin, rmax, coord.data()));

          // Map to global coordinate position...
          for (cgsize_t i = 0; i < num_to_get; i++) {
            rdata[phys_dimension * i + 2] = coord[i];
          }
        }
      }
      else if (field.get_name() == "cell_node_ids") {
        if (field.get_type() == Ioss::Field::INT64) {
          auto *idata = static_cast<int64_t *>(data);
          sb->get_cell_node_ids(idata, true);
        }
        else {
          assert(field.get_type() == Ioss::Field::INT32);
          int *idata = static_cast<int *>(data);
          sb->get_cell_node_ids(idata, true);
        }
      }
      else if (field.get_name() == "cell_ids") {
        if (field.get_type() == Ioss::Field::INT64) {
          auto *idata = static_cast<int64_t *>(data);
          sb->get_cell_ids(idata, true);
        }
        else {
          assert(field.get_type() == Ioss::Field::INT32);
          int *idata = static_cast<int *>(data);
          sb->get_cell_ids(idata, true);
        }
      }
      else {
        num_to_get = Ioss::Utils::field_warning(sb, field, "input");
      }
    }
    else if (role == Ioss::Field::TRANSIENT) {
      int comp_count = field.get_component_count(Ioss::Field::InOut::INPUT);

      int sol_index = 0;
      int step      = get_region()->get_current_state();
      if (cell_field) {
        sol_index = Utils::find_solution_index(get_file_pointer(), base, zone, step,
                                               CGNS_ENUMV(CellCenter));
      }
      else {
        sol_index =
            Utils::find_solution_index(get_file_pointer(), base, zone, step, CGNS_ENUMV(Vertex));
      }
      int field_offset = Utils::index(field);

      if (comp_count == 1) {
        CGCHECKM(cgp_field_read_data(get_file_pointer(), base, zone, sol_index, field_offset, rmin,
                                     rmax, rdata));
      }
      else {
        std::vector<double> cgns_data(num_to_get);
        for (int i = 0; i < comp_count; i++) {
          CGCHECKM(cgp_field_read_data(get_file_pointer(), base, zone, sol_index, field_offset + i,
                                       rmin, rmax, cgns_data.data()));
          for (cgsize_t j = 0; j < num_to_get; j++) {
            rdata[comp_count * j + i] = cgns_data[j];
          }
        }
      }
    }
    else {
      num_to_get = Ioss::Utils::field_warning(sb, field, "input");
    }
    return num_to_get;
  }

  int64_t ParallelDatabaseIO::get_field_internal(const Ioss::ElementBlock *eb,
                                                 const Ioss::Field &field, void *data,
                                                 size_t data_size) const
  {
    int    base       = eb->get_property("base").get_int();
    int    zone       = eb->get_property("zone").get_int();
    size_t num_to_get = field.verify(data_size);
    auto   role       = field.get_role();

    if (role == Ioss::Field::MESH) {
      // Handle the MESH fields required for a CGNS file model.
      // (The 'genesis' portion)

      if (field.get_name() == "connectivity_raw" || field.get_name() == "connectivity") {

        // The connectivity is stored in a 1D array.
        // The element_node index varies fastest
        int order = eb->get_property("original_block_order").get_int();
        decomp->get_block_connectivity(get_file_pointer(), data, order);
        if (field.get_type() == Ioss::Field::INT32) {
          auto *idata = reinterpret_cast<int *>(data);
          Utils::map_cgns_connectivity(eb->topology(), num_to_get, idata);
        }
        else {
          auto *idata = reinterpret_cast<int64_t *>(data);
          Utils::map_cgns_connectivity(eb->topology(), num_to_get, idata);
        }
      }
      else if (field.get_name() == "ids" || field.get_name() == "implicit_ids") {
        // Map the local ids in this element block
        // (1..element_count) to global element ids.
        get_map(entity_type::ELEM).map_implicit_data(data, field, num_to_get, eb->get_offset());
      }
      else {
        num_to_get = Ioss::Utils::field_warning(eb, field, "input");
      }
    }
    else if (role == Ioss::Field::TRANSIENT) {
      // Locate the FlowSolution node corresponding to the correct state/step/time
      // TODO: do this at read_meta_data() and store...
      int step = get_region()->get_current_state();
      int solution_index =
          Utils::find_solution_index(get_file_pointer(), base, zone, step, CGNS_ENUMV(CellCenter));

      int order = eb->get_property("original_block_order").get_int();

      // Read into a double variable
      // TODO: Support other field types...
      size_t              num_entity = eb->entity_count();
      std::vector<double> temp(num_entity);

      // get number of components, cycle through each component
      size_t comp_count = field.get_component_count(Ioss::Field::InOut::INPUT);
      for (size_t i = 0; i < comp_count; i++) {
        int field_offset = Utils::index(field) + i;
        decomp->get_element_field(get_file_pointer(), solution_index, order, field_offset,
                                  temp.data());

        // Transfer to 'data' array.
        size_t k = 0;
        assert(field.get_type() == Ioss::Field::REAL);
        auto *rvar = static_cast<double *>(data);
        for (size_t j = i; j < num_entity * comp_count; j += comp_count) {
          rvar[j] = temp[k++];
        }
        assert(k == num_entity);
      }
    }
    else {
      num_to_get = Ioss::Utils::field_warning(eb, field, "unknown");
    }
    return num_to_get;
  }

  int64_t ParallelDatabaseIO::get_field_internal(const Ioss::NodeSet * /* ns */,
                                                 const Ioss::Field & /* field */, void * /* data */,
                                                 size_t /* data_size */) const
  {
    return -1;
  }
  int64_t ParallelDatabaseIO::get_field_internal(const Ioss::EdgeSet * /* ns */,
                                                 const Ioss::Field & /* field */, void * /* data */,
                                                 size_t /* data_size */) const
  {
    return -1;
  }
  int64_t ParallelDatabaseIO::get_field_internal(const Ioss::FaceSet * /* ns */,
                                                 const Ioss::Field & /* field */, void * /* data */,
                                                 size_t /* data_size */) const
  {
    return -1;
  }
  int64_t ParallelDatabaseIO::get_field_internal(const Ioss::ElementSet * /* ns */,
                                                 const Ioss::Field & /* field */, void * /* data */,
                                                 size_t /* data_size */) const
  {
    return -1;
  }
  int64_t ParallelDatabaseIO::get_field_internal(const Ioss::SideBlock *sb,
                                                 const Ioss::Field &field, void *data,
                                                 size_t data_size) const
  {
    int         id   = sb->get_property("id").get_int();
    const auto &sset = decomp->m_sideSets[id];

    auto num_to_get = field.verify(data_size);
    if (num_to_get > 0) {
      int64_t entity_count = sb->entity_count();
      if (num_to_get != entity_count) {
        std::ostringstream errmsg;
        fmt::print(errmsg, "ERROR: Partial field input not yet implemented for side blocks");
        IOSS_ERROR(errmsg);
      }
    }

    Ioss::Field::RoleType role = field.get_role();
    if (role == Ioss::Field::MESH) {
      if (field.get_name() == "element_side_raw" || field.get_name() == "element_side") {

        decomp->get_sideset_element_side(get_file_pointer(), sset, data);
        return num_to_get;
      }
      else {
        num_to_get = Ioss::Utils::field_warning(sb, field, "input");
      }
    }
    else {
      num_to_get = Ioss::Utils::field_warning(sb, field, "input");
    }
    return num_to_get;
  }

  int64_t ParallelDatabaseIO::get_field_internal(const Ioss::SideSet * /* fs */,
                                                 const Ioss::Field & /* field */, void * /* data */,
                                                 size_t /* data_size */) const
  {
    return -1;
  }
  int64_t ParallelDatabaseIO::get_field_internal(const Ioss::CommSet *cs, const Ioss::Field &field,
                                                 void *data, size_t data_size) const
  {
    size_t num_to_get = field.verify(data_size);

    // Return the <entity (node or side), processor> pair
    if (field.get_name() == "entity_processor" || field.get_name() == "entity_processor_raw") {

      // Check type -- node or side
      std::string type = cs->get_property("entity_type").get_string();

      if (type == "node") {

        bool do_map = field.get_name() == "entity_processor";
        // Convert local node id to global node id and store in 'data'
        const Ioss::MapContainer &map = get_map(entity_type::NODE).map();
        if (int_byte_size_api() == 4) {
          decomp->get_node_entity_proc_data(static_cast<int *>(data), map, do_map);
        }
        else {
          decomp->get_node_entity_proc_data(static_cast<int64_t *>(data), map, do_map);
        }
      }
      else {
        std::ostringstream errmsg;
        fmt::print(errmsg, "ERROR: Invalid commset type {}", type);
        IOSS_ERROR(errmsg);
      }
    }
    else if (field.get_name() == "ids") {
      // Do nothing, just handles an idiosyncrasy of the GroupingEntity
    }
    else {
      num_to_get = Ioss::Utils::field_warning(cs, field, "input");
    }
    return num_to_get;
  }

  int64_t ParallelDatabaseIO::handle_element_ids(const Ioss::ElementBlock *eb, void *ids,
                                                 size_t num_to_get, size_t offset,
                                                 size_t count) const
  {
    bool in_define = (dbState == Ioss::STATE_MODEL) || (dbState == Ioss::STATE_DEFINE_MODEL);
    if (in_define) {
      if (m_elemGlobalImplicitMap.empty()) {
        m_elemGlobalImplicitMap.resize(elementCount);
      }
      // Build the implicit_global map used to map an elements
      // local-implicit position to the global-implicit
      // position. Primarily used for sideset elements.
      // Elements starting at 'eb_offset' map to the global implicit
      // position of 'offset'
      int64_t eb_offset = eb->get_offset();
      for (size_t i = 0; i < count; i++) {
        m_elemGlobalImplicitMap[eb_offset + i] = offset + i + 1;
      }
    }

    elemMap.set_size(elementCount);
    int64_t eb_offset = eb->get_offset();
    if (int_byte_size_api() == 4) {
      elemMap.set_map(static_cast<int *>(ids), num_to_get, eb_offset, in_define);
    }
    else {
      elemMap.set_map(static_cast<int64_t *>(ids), num_to_get, eb_offset, in_define);
    }
    return num_to_get;
  }

  int64_t ParallelDatabaseIO::put_field_internal(const Ioss::Region * /* region */,
                                                 const Ioss::Field & /* field */, void * /* data */,
                                                 size_t /* data_size */) const
  {
    return -1;
  }

  int64_t ParallelDatabaseIO::put_field_internal(const Ioss::NodeBlock *nb,
                                                 const Ioss::Field &field, void *data,
                                                 size_t data_size) const
  {
    // A CGNS DatabaseIO object can have two "types" of NodeBlocks:
    // * The normal "all nodes in the model" NodeBlock as used by Exodus
    // * A "nodes in a zone" NodeBlock which contains the subset of nodes
    //   "owned" by a specific StructuredBlock or ElementBlock zone.
    //
    // Question: How to determine if the NodeBlock is the "global" Nodeblock
    // or a "sub" NodeBlock: Use the "is_nonglobal_nodeblock()" function.
    if (nb->is_nonglobal_nodeblock()) {
      return put_field_internal_sub_nb(nb, field, data, data_size);
    }

    Ioss::Field::RoleType role       = field.get_role();
    cgsize_t              base       = 1;
    size_t                num_to_get = field.verify(data_size);

    // Instead of outputting a global nodeblock's worth of data,
    // the data is output a "zone" at a time.
    // The m_globalToBlockLocalNodeMap[zone] map is used (Ioss::Map pointer)
    // This map is built during the output of block connectivity,
    // so for cgns unstructured mesh, we need to output ElementBlock connectivity
    // prior to outputting nodal coordinates.
    for (const auto &z : m_globalToBlockLocalNodeMap) {
      if (z.second == nullptr) {
        std::ostringstream errmsg;
        fmt::print(errmsg,
                   "ERROR: CGNS: The globalToBlockLocalNodeMap is not defined, so nodal fields "
                   "cannot be output.");
        IOSS_ERROR(errmsg);
      }
    }

    if (role == Ioss::Field::MESH) {
      if (field.get_name() == "ids") {
        // The ids coming in are the global ids; their position is the
        // local id -1 (That is, data[0] contains the global id of local
        // node 1)
        handle_node_ids(data, num_to_get);
      }
      else if (field.get_name() == "mesh_model_coordinates" ||
               field.get_name() == "mesh_model_coordinates_x" ||
               field.get_name() == "mesh_model_coordinates_y" ||
               field.get_name() == "mesh_model_coordinates_z") {
        auto *rdata = static_cast<double *>(data);

        std::vector<int64_t> node_offset = get_processor_zone_node_offset();

        if (field.get_name() == "mesh_model_coordinates") {
          int spatial_dim = nb->get_property("component_degree").get_int();

          // Output all coordinates, a zone's worth of data at a time...

          for (const auto &block : m_globalToBlockLocalNodeMap) {
            auto zone = block.first;
            // NOTE: 'block_map' has one more entry than node_count.  First entry is for something
            // else.  But, ->size() returns correct size (ignoring first entry)
            //       'block_map' is 1-based.
            const auto         &block_map = block.second;
            std::vector<double> x(block_map->size());
            std::vector<double> y(block_map->size());
            std::vector<double> z(block_map->size());

            for (size_t i = 0; i < block_map->size(); i++) {
              auto global = block_map->map()[i + 1];
              auto local  = nodeMap.global_to_local(global) - 1;
              assert(local >= 0 && local < (int64_t)num_to_get);

              x[i] = rdata[local * spatial_dim + 0];
              if (spatial_dim > 1) {
                y[i] = rdata[local * spatial_dim + 1];
              }
              if (spatial_dim > 2) {
                z[i] = rdata[local * spatial_dim + 2];
              }
            }

            // Create the zone
            // Output this zones coordinates...
            int crd_idx = 0;
            CGCHECKM(cgp_coord_write(get_file_pointer(), base, zone, CGNS_ENUMV(RealDouble),
                                     "CoordinateX", &crd_idx));
            cgsize_t start  = node_offset[zone - 1] + 1;
            cgsize_t finish = start + block_map->size() - 1;

            auto xx = block_map->size() > 0 ? x.data() : nullptr;
            CGCHECKM(
                cgp_coord_write_data(get_file_pointer(), base, zone, crd_idx, &start, &finish, xx));

            if (spatial_dim > 1) {
              CGCHECKM(cgp_coord_write(get_file_pointer(), base, zone, CGNS_ENUMV(RealDouble),
                                       "CoordinateY", &crd_idx));
              auto yy = block_map->size() > 0 ? y.data() : nullptr;
              CGCHECKM(cgp_coord_write_data(get_file_pointer(), base, zone, crd_idx, &start,
                                            &finish, yy));
            }

            if (spatial_dim > 2) {
              CGCHECKM(cgp_coord_write(get_file_pointer(), base, zone, CGNS_ENUMV(RealDouble),
                                       "CoordinateZ", &crd_idx));
              auto zz = block_map->size() > 0 ? z.data() : nullptr;
              CGCHECKM(cgp_coord_write_data(get_file_pointer(), base, zone, crd_idx, &start,
                                            &finish, zz));
            }
          }
        }
        else {
          // Outputting only a single coordinate value...
          for (const auto &block : m_globalToBlockLocalNodeMap) {
            auto zone = block.first;
            // NOTE: 'block_map' has one more entry than node_count.  First entry is for something
            // else.
            //       'block_map' is 1-based.
            const auto         &block_map = block.second;
            std::vector<double> xyz(block_map->size());

            for (size_t i = 0; i < block_map->size(); i++) {
              auto global = block_map->map()[i + 1];
              auto local  = nodeMap.global_to_local(global) - 1;
              xyz[i]      = rdata[local];
            }

            std::string cgns_name = "Invalid";
            if (field.get_name() == "mesh_model_coordinates_x") {
              cgns_name = "CoordinateX";
            }
            else if (field.get_name() == "mesh_model_coordinates_y") {
              cgns_name = "CoordinateY";
            }
            else if (field.get_name() == "mesh_model_coordinates_z") {
              cgns_name = "CoordinateZ";
            }
            // Create the zone
            // Output this zones coordinates...
            int crd_idx = 0;
            CGCHECKM(cgp_coord_write(get_file_pointer(), base, zone, CGNS_ENUMV(RealDouble),
                                     cgns_name.c_str(), &crd_idx));
            cgsize_t start  = node_offset[zone - 1] + 1;
            cgsize_t finish = start + block_map->size() - 1;
            auto     xx     = block_map->size() > 0 ? xyz.data() : nullptr;
            CGCHECKM(
                cgp_coord_write_data(get_file_pointer(), base, zone, crd_idx, &start, &finish, xx));
          }
        }
      }
      else {
        num_to_get = Ioss::Utils::field_warning(nb, field, "output");
      }
    }
    else if (role == Ioss::Field::TRANSIENT) {
      // Instead of outputting a global nodeblock's worth of data,
      // the data is output a "zone" at a time.
      // The m_globalToBlockLocalNodeMap[zone] map is used (Ioss::Map pointer)
      // This map is built during the output of block connectivity,
      // so for cgns unstructured mesh, we need to output ElementBlock connectivity
      // prior to outputting nodal coordinates.
      std::vector<int64_t> node_offset = get_processor_zone_node_offset();

      size_t comp_count = field.get_component_count(Ioss::Field::InOut::OUTPUT);

      double *rdata = num_to_get > 0 ? static_cast<double *>(data) : nullptr;

      for (const auto &block : m_globalToBlockLocalNodeMap) {
        auto zone = block.first;
        // NOTE: 'block_map' has one more entry than node_count.
        // First entry is for something else.  'block_map' is
        // 1-based.
        const auto         &block_map = block.second;
        std::vector<double> blk_data(block_map->size());

        cgsize_t start  = node_offset[zone - 1] + 1;
        cgsize_t finish = start + block_map->size() - 1;

        for (size_t i = 0; i < comp_count; i++) {
          for (size_t j = 0; j < block_map->size(); j++) {
            auto global = block_map->map()[j + 1];
            auto local  = nodeMap.global_to_local(global) - 1;
            assert(local >= 0 && local < (int64_t)num_to_get);
            blk_data[j] = rdata[local * comp_count + i];
          }
          std::string var_name   = (comp_count > 1)
                                       ? get_component_name(field, Ioss::Field::InOut::OUTPUT, i + 1)
                                       : field.get_name();
          int         cgns_field = 0;
          CGCHECKM(cgp_field_write(get_file_pointer(), base, zone, m_currentVertexSolutionIndex,
                                   CGNS_ENUMV(RealDouble), var_name.c_str(), &cgns_field));

          CGCHECKM(cgp_field_write_data(get_file_pointer(), base, zone,
                                        m_currentVertexSolutionIndex, cgns_field, &start, &finish,
                                        blk_data.data()));
          if (i == 0)
            Utils::set_field_index(field, cgns_field, CGNS_ENUMV(Vertex));
        }
      }
    }
    else {
      num_to_get = Ioss::Utils::field_warning(nb, field, "output");
    }
    return num_to_get;
  }

  int64_t ParallelDatabaseIO::put_field_internal_sub_nb(const Ioss::NodeBlock *nb,
                                                        const Ioss::Field &field, void *data,
                                                        size_t data_size) const
  {
    // Outputs field data on a NodeBlock which is a "sub" NodeBlock -- contains the nodes for a
    // StructuredBlock instead of for the entire model.
    // Currently only TRANSIENT fields are output this way.  No valid reason, but that is the
    // current use case.

    // Get the StructuredBlock that this NodeBlock is contained in:
    const Ioss::GroupingEntity *sb         = nb->contained_in();
    int                         zone       = Iocgns::Utils::get_db_zone(sb);
    cgsize_t                    num_to_get = field.verify(data_size);

    Ioss::Field::RoleType role = field.get_role();
    if (role == Ioss::Field::TRANSIENT) {
      int   base       = 1;
      auto *rdata      = static_cast<double *>(data);
      int   cgns_field = 0;
      int   comp_count = field.get_component_count(Ioss::Field::InOut::OUTPUT);

      cgsize_t rmin[3] = {0, 0, 0};
      cgsize_t rmax[3] = {0, 0, 0};

      assert(num_to_get == sb->get_property("node_count").get_int());
      if (num_to_get > 0) {
        rmin[0] = sb->get_property("offset_i").get_int() + 1;
        rmin[1] = sb->get_property("offset_j").get_int() + 1;
        rmin[2] = sb->get_property("offset_k").get_int() + 1;

        rmax[0] = rmin[0] + sb->get_property("ni").get_int();
        rmax[1] = rmin[1] + sb->get_property("nj").get_int();
        rmax[2] = rmin[2] + sb->get_property("nk").get_int();
      }

      if (comp_count == 1) {
        CGCHECKM(cgp_field_write(get_file_pointer(), base, zone, m_currentVertexSolutionIndex,
                                 CGNS_ENUMV(RealDouble), field.get_name().c_str(), &cgns_field));
        Utils::set_field_index(field, cgns_field, CGNS_ENUMV(Vertex));

        CGCHECKM(cgp_field_write_data(get_file_pointer(), base, zone, m_currentVertexSolutionIndex,
                                      cgns_field, rmin, rmax, rdata));
      }
      else {
        std::vector<double> cgns_data(num_to_get);
        for (int i = 0; i < comp_count; i++) {
          for (cgsize_t j = 0; j < num_to_get; j++) {
            cgns_data[j] = rdata[comp_count * j + i];
          }
          std::string var_name = get_component_name(field, Ioss::Field::InOut::OUTPUT, i + 1);

          CGCHECKM(cgp_field_write(get_file_pointer(), base, zone, m_currentVertexSolutionIndex,
                                   CGNS_ENUMV(RealDouble), var_name.c_str(), &cgns_field));
          if (i == 0) {
            Utils::set_field_index(field, cgns_field, CGNS_ENUMV(Vertex));
          }

          CGCHECKM(cgp_field_write_data(get_file_pointer(), base, zone,
                                        m_currentVertexSolutionIndex, cgns_field, rmin, rmax,
                                        cgns_data.data()));
        }
      }
    }
    // Ignoring all other field role types...
    return num_to_get;
  }

  int64_t ParallelDatabaseIO::put_field_internal(const Ioss::ElementBlock *eb,
                                                 const Ioss::Field &field, void *data,
                                                 size_t data_size) const
  {
    size_t num_to_get = field.verify(data_size);

    Ioss::Field::RoleType role = field.get_role();

    if (role == Ioss::Field::MESH) {
      // Handle the MESH fields required for a CGNS file model.
      // (The 'genesis' portion)
      if (field.get_name() == "ids") {
        size_t proc_offset = eb->get_property("proc_offset").get_int();
        handle_element_ids(eb, data, num_to_get, proc_offset, num_to_get);
      }
      else if (field.get_name() == "connectivity") {
        // This blocks zone has not been defined.
        // Get the "node block" for this element block...
        size_t element_nodes = eb->topology()->number_nodes();
        assert((size_t)field.raw_storage()->component_count() == element_nodes);

        CGNSIntVector nodes;
        nodes.reserve(element_nodes * num_to_get);

        if (field.get_type() == Ioss::Field::INT32) {
          int *idata = num_to_get > 0 ? reinterpret_cast<int *>(data) : nullptr;
          for (size_t i = 0; i < element_nodes * num_to_get; i++) {
            nodes.push_back(idata[i]);
          }
        }
        else {
          int64_t *idata = num_to_get > 0 ? reinterpret_cast<int64_t *>(data) : nullptr;
          for (size_t i = 0; i < element_nodes * num_to_get; i++) {
            nodes.push_back(idata[i]);
          }
        }
        Ioss::Utils::uniquify(nodes);

        // Resolve zone-shared nodes (nodes used in this zone, but are
        // shared on processor boundaries).
        // This routine determines the mapping of each global id node
        // in 'nodes' to the zone-local position.
        // This mapping is in 'connectivity_map' and is correct for all
        // nodes on this processor whether they are owned or shared.
        //
        // 'resolve_zone_shared_nodes' also returns the number of nodes owned on this
        // processor, and the 'offset' of this processors chunk of nodes into the overall
        // set of nodes for the zone.  Each processors chunk of nodes is contiguous
        //
        // The 'nodes' and 'connectivity_map' vectors are used later below to generate
        // the map of which global node data is written by this processor for this zone.
        CGNSIntVector connectivity_map(nodes.size());
        size_t        owned_node_count  = 0;
        size_t        owned_node_offset = 0;
        resolve_zone_shared_nodes(nodes, connectivity_map, owned_node_count, owned_node_offset);

        // Get total count on all processors...
        // Note that there will be shared nodes on processor boundaries that need to be
        // accounted for...
        cgsize_t size[3] = {0, 0, 0};
        size[0]          = owned_node_count;
        size[1]          = eb->entity_count();

        MPI_Allreduce(MPI_IN_PLACE, size, 3, cgns_mpi_type(), MPI_SUM, util().communicator());

        // Now, we have the node count and cell count so we can create a zone...
        int base = 1;
        int zone = 0;

        CGCHECKM(cg_zone_write(get_file_pointer(), base, eb->name().c_str(), size,
                               CGNS_ENUMV(Unstructured), &zone));
        eb->property_update("zone", zone);
        eb->property_update("id", zone);
        eb->property_update("guid", util().generate_guid(zone));
        eb->property_update("section", 1);
        eb->property_update("base", base);
        eb->property_update("zone_node_count", size[0]);
        eb->property_update("zone_element_count", size[1]);

        if (eb->property_exists("assembly")) {
          std::string assembly = eb->get_property("assembly").get_string();
          CGCHECKM(cg_goto(get_file_pointer(), base, "Zone_t", zone, "end"));
          CGCHECKM(cg_famname_write(assembly.c_str()));
        }

        if (size[1] > 0) {
          CGNS_ENUMT(ElementType_t) type = Utils::map_topology_to_cgns(eb->topology()->name());
          int sect                       = 0;
          CGCHECKM(cgp_section_write(get_file_pointer(), base, zone, "HexElements", type, 1,
                                     size[1], 0, &sect));

          int64_t start = 0;
          MPI_Exscan(&num_to_get, &start, 1, Ioss::mpi_type(start), MPI_SUM, util().communicator());
          // Of the cells/elements in this zone, this proc handles
          // those starting at 'proc_offset+1' to 'proc_offset+num_entity'
          eb->property_update("proc_offset", start);

          // Map connectivity global ids to zone-local 1-based ids.
          CGNSIntVector connect;
          connect.reserve(num_to_get * element_nodes);

          if (field.get_type() == Ioss::Field::INT32) {
            int *idata = num_to_get > 0 ? reinterpret_cast<int *>(data) : nullptr;
            for (size_t i = 0; i < num_to_get * element_nodes; i++) {
              auto id   = idata[i];
              auto iter = std::lower_bound(nodes.cbegin(), nodes.cend(), id);
              assert(iter != nodes.end());
              auto cur_pos = iter - nodes.cbegin();
              connect.push_back(connectivity_map[cur_pos]);
            }
          }
          else {
            int64_t *idata = num_to_get > 0 ? reinterpret_cast<int64_t *>(data) : nullptr;
            for (size_t i = 0; i < num_to_get * element_nodes; i++) {
              auto id   = idata[i];
              auto iter = std::lower_bound(nodes.cbegin(), nodes.cend(), id);
              assert(iter != nodes.cend());
              auto cur_pos = iter - nodes.cbegin();
              connect.push_back(connectivity_map[cur_pos]);
            }
          }

          Utils::unmap_cgns_connectivity(eb->topology(), num_to_get, connect.data());
          CGCHECKM(cgp_elements_write_data(get_file_pointer(), base, zone, sect, start + 1,
                                           start + num_to_get, connect.data()));

          int64_t eb_size = num_to_get;
          MPI_Allreduce(MPI_IN_PLACE, &eb_size, 1, Ioss::mpi_type(eb_size), MPI_SUM,
                        util().communicator());

          m_bcOffset[zone] += eb_size;
          eb->property_update("section", sect);
        }

        // The 'nodes' map needs to be updated to filter out any nodes
        // that are not owned by this processor.  Currently contains both
        // owned and shared so we could update the connectivity...
        // The 'connectivity_map' value indicates whether it is owned or shared --
        // if 'connectivity_map[i] > owned_node_offset, then it is owned; otherwise shared.
        if (!nodes.empty()) {
          for (size_t i = 0; i < nodes.size(); i++) {
            if (connectivity_map[i] <= (cgsize_t)owned_node_offset) {
              nodes[i] = std::numeric_limits<cgsize_t>::max();
            }
          }
          connectivity_map.clear();
          connectivity_map.shrink_to_fit();

          Ioss::Utils::uniquify(nodes);
          if (nodes.back() == std::numeric_limits<cgsize_t>::max()) {
            nodes.pop_back();
          }
          nodes.shrink_to_fit();
        }
        assert(nodes.size() == owned_node_count);

        // Now we have a valid zone so can update some data structures...
        m_zoneOffset[zone] = m_zoneOffset[zone - 1] + size[1];
        m_globalToBlockLocalNodeMap[zone] =
            new Ioss::Map("node", get_filename() + "::" + eb->name(), myProcessor);
        m_globalToBlockLocalNodeMap[zone]->map().reserve(nodes.size() + 1);
        m_globalToBlockLocalNodeMap[zone]->map().push_back(1); // Non one-to-one map
        for (auto &i : nodes) {
          m_globalToBlockLocalNodeMap[zone]->map().push_back(i);
        }
      }
      else {
        num_to_get = Ioss::Utils::field_warning(eb, field, "output");
      }
    }
    else if (role == Ioss::Field::TRANSIENT) {
      double *rdata = num_to_get > 0 ? static_cast<double *>(data) : nullptr;

      int base = eb->get_property("base").get_int();
      int zone = eb->get_property("zone").get_int();

      cgsize_t start        = eb->get_property("proc_offset").get_int();
      cgsize_t range_min[1] = {start + 1};
      cgsize_t range_max[1] = {(cgsize_t)start + (cgsize_t)num_to_get};

      // get number of components, cycle through each component
      size_t comp_count = field.get_component_count(Ioss::Field::InOut::OUTPUT);
      if (comp_count == 1) {
        int cgns_field = 0;
        CGCHECKM(cgp_field_write(get_file_pointer(), base, zone, m_currentCellCenterSolutionIndex,
                                 CGNS_ENUMV(RealDouble), field.get_name().c_str(), &cgns_field));
        CGCHECKM(cgp_field_write_data(get_file_pointer(), base, zone,
                                      m_currentCellCenterSolutionIndex, cgns_field, range_min,
                                      range_max, rdata));
        Utils::set_field_index(field, cgns_field, CGNS_ENUMV(CellCenter));
      }
      else {
        std::vector<double> cgns_data(num_to_get);
        for (size_t i = 0; i < comp_count; i++) {
          for (size_t j = 0; j < num_to_get; j++) {
            cgns_data[j] = rdata[comp_count * j + i];
          }
          std::string var_name   = get_component_name(field, Ioss::Field::InOut::OUTPUT, i + 1);
          int         cgns_field = 0;
          CGCHECKM(cgp_field_write(get_file_pointer(), base, zone, m_currentCellCenterSolutionIndex,
                                   CGNS_ENUMV(RealDouble), var_name.c_str(), &cgns_field));
          CGCHECKM(cgp_field_write_data(get_file_pointer(), base, zone,
                                        m_currentCellCenterSolutionIndex, cgns_field, range_min,
                                        range_max, cgns_data.data()));
          if (i == 0) {
            Utils::set_field_index(field, cgns_field, CGNS_ENUMV(CellCenter));
          }
        }
      }
    }
    else {
      num_to_get = Ioss::Utils::field_warning(eb, field, "unknown");
    }
    return num_to_get;
  }

  int64_t ParallelDatabaseIO::put_field_internal(const Ioss::StructuredBlock *sb,
                                                 const Ioss::Field &field, void *data,
                                                 size_t data_size) const
  {
    Ioss::Field::RoleType role = field.get_role();
    cgsize_t              base = sb->get_property("base").get_int();
    cgsize_t              zone = Iocgns::Utils::get_db_zone(sb);

    cgsize_t num_to_get = field.verify(data_size);

    cgsize_t rmin[3] = {0, 0, 0};
    cgsize_t rmax[3] = {0, 0, 0};

    bool cell_field = Utils::is_cell_field(field);

    if (cell_field) {
      assert(num_to_get == sb->get_property("cell_count").get_int());
      if (num_to_get > 0) {
        rmin[0] = sb->get_property("offset_i").get_int() + 1;
        rmin[1] = sb->get_property("offset_j").get_int() + 1;
        rmin[2] = sb->get_property("offset_k").get_int() + 1;

        rmax[0] = rmin[0] + sb->get_property("ni").get_int() - 1;
        rmax[1] = rmin[1] + sb->get_property("nj").get_int() - 1;
        rmax[2] = rmin[2] + sb->get_property("nk").get_int() - 1;
      }
    }
    else {
      // cell nodal field.
      assert(num_to_get == sb->get_property("node_count").get_int());
      if (num_to_get > 0) {
        rmin[0] = sb->get_property("offset_i").get_int() + 1;
        rmin[1] = sb->get_property("offset_j").get_int() + 1;
        rmin[2] = sb->get_property("offset_k").get_int() + 1;

        rmax[0] = rmin[0] + sb->get_property("ni").get_int();
        rmax[1] = rmin[1] + sb->get_property("nj").get_int();
        rmax[2] = rmin[2] + sb->get_property("nk").get_int();
      }
    }

    assert(num_to_get == 0 || num_to_get == (rmax[0] - rmin[0] + 1) * (rmax[1] - rmin[1] + 1) *
                                                (rmax[2] - rmin[2] + 1));
    double *rdata = num_to_get > 0 ? static_cast<double *>(data) : nullptr;

    if (role == Ioss::Field::MESH) {
      int crd_idx = 0;
      if (field.get_name() == "mesh_model_coordinates_x") {
        CGCHECKM(cgp_coord_write(get_file_pointer(), base, zone, CGNS_ENUMV(RealDouble),
                                 "CoordinateX", &crd_idx));
        CGCHECKM(cgp_coord_write_data(get_file_pointer(), base, zone, crd_idx, rmin, rmax, rdata));
      }

      else if (field.get_name() == "mesh_model_coordinates_y") {
        CGCHECKM(cgp_coord_write(get_file_pointer(), base, zone, CGNS_ENUMV(RealDouble),
                                 "CoordinateY", &crd_idx));
        CGCHECKM(cgp_coord_write_data(get_file_pointer(), base, zone, crd_idx, rmin, rmax, rdata));
      }

      else if (field.get_name() == "mesh_model_coordinates_z") {
        CGCHECKM(cgp_coord_write(get_file_pointer(), base, zone, CGNS_ENUMV(RealDouble),
                                 "CoordinateZ", &crd_idx));
        CGCHECKM(cgp_coord_write_data(get_file_pointer(), base, zone, crd_idx, rmin, rmax, rdata));
      }

      else if (field.get_name() == "mesh_model_coordinates") {
        int phys_dimension = get_region()->get_property("spatial_dimension").get_int();

        std::vector<double> coord(num_to_get);

        // ========================================================================
        // Repetitive code for each coordinate direction; use a lambda to consolidate...
        auto coord_lambda = [=, &coord](const char *ordinate, int ordinal) {
          // Data required by upper classes store x0, y0, z0, ... xn,
          // yn, zn. Data stored in cgns file is x0, ..., xn, y0,
          // ..., yn, z0, ..., zn so we have to allocate some scratch
          // memory to read in the data and then map into supplied
          // 'data'
          // Map to global coordinate position...
          for (cgsize_t i = 0; i < num_to_get; i++) {
            coord[i] = rdata[phys_dimension * i + ordinal];
          }

          int idx = 0;
          CGCHECKM(cgp_coord_write(get_file_pointer(), base, zone, CGNS_ENUMV(RealDouble), ordinate,
                                   &idx));
          CGCHECKM(
              cgp_coord_write_data(get_file_pointer(), base, zone, idx, rmin, rmax, coord.data()));
        };
        // ========================================================================

        coord_lambda("CoordinateX", 0);

        if (phys_dimension >= 2) {
          coord_lambda("CoordinateY", 1);
        }

        if (phys_dimension == 3) {
          coord_lambda("CoordinateZ", 2);
        }
      }
      else {
        num_to_get = Ioss::Utils::field_warning(sb, field, "output");
      }
    }
    else if (role == Ioss::Field::TRANSIENT) {
      int cgns_field = 0;
      int comp_count = field.get_component_count(Ioss::Field::InOut::OUTPUT);
      int sol_index  = 0;
      CGNS_ENUMT(GridLocation_t) location;
      if (cell_field) {
        sol_index = m_currentCellCenterSolutionIndex;
        location  = CGNS_ENUMV(CellCenter);
      }
      else {
        sol_index = m_currentVertexSolutionIndex;
        location  = CGNS_ENUMV(Vertex);
      }
      if (comp_count == 1) {
        CGCHECKM(cgp_field_write(get_file_pointer(), base, zone, sol_index, CGNS_ENUMV(RealDouble),
                                 field.get_name().c_str(), &cgns_field));
        Utils::set_field_index(field, cgns_field, location);

        CGCHECKM(cgp_field_write_data(get_file_pointer(), base, zone, sol_index, cgns_field, rmin,
                                      rmax, rdata));
      }
      else {
        std::vector<double> cgns_data(num_to_get);
        for (int i = 0; i < comp_count; i++) {
          for (cgsize_t j = 0; j < num_to_get; j++) {
            cgns_data[j] = rdata[comp_count * j + i];
          }
          std::string var_name = get_component_name(field, Ioss::Field::InOut::OUTPUT, i + 1);

          CGCHECKM(cgp_field_write(get_file_pointer(), base, zone, sol_index,
                                   CGNS_ENUMV(RealDouble), var_name.c_str(), &cgns_field));
          if (i == 0) {
            Utils::set_field_index(field, cgns_field, location);
          }

          CGCHECKM(cgp_field_write_data(get_file_pointer(), base, zone, sol_index, cgns_field, rmin,
                                        rmax, cgns_data.data()));
        }
      }
    }
    else {
      num_to_get = Ioss::Utils::field_warning(sb, field, "output");
    }
    return num_to_get;
  }

  int64_t ParallelDatabaseIO::put_field_internal(const Ioss::FaceBlock * /* nb */,
                                                 const Ioss::Field & /* field */, void * /* data */,
                                                 size_t /* data_size */) const
  {
    return -1;
  }
  int64_t ParallelDatabaseIO::put_field_internal(const Ioss::EdgeBlock * /* nb */,
                                                 const Ioss::Field & /* field */, void * /* data */,
                                                 size_t /* data_size */) const
  {
    return -1;
  }
  int64_t ParallelDatabaseIO::put_field_internal(const Ioss::NodeSet * /* ns */,
                                                 const Ioss::Field & /* field */, void * /* data */,
                                                 size_t /* data_size */) const
  {
    return -1;
  }
  int64_t ParallelDatabaseIO::put_field_internal(const Ioss::EdgeSet * /* ns */,
                                                 const Ioss::Field & /* field */, void * /* data */,
                                                 size_t /* data_size */) const
  {
    return -1;
  }
  int64_t ParallelDatabaseIO::put_field_internal(const Ioss::FaceSet * /* ns */,
                                                 const Ioss::Field & /* field */, void * /* data */,
                                                 size_t /* data_size */) const
  {
    return -1;
  }
  int64_t ParallelDatabaseIO::put_field_internal(const Ioss::ElementSet * /* ns */,
                                                 const Ioss::Field & /* field */, void * /* data */,
                                                 size_t /* data_size */) const
  {
    return -1;
  }
  int64_t ParallelDatabaseIO::put_field_internal(const Ioss::SideBlock *sb,
                                                 const Ioss::Field &field, void *data,
                                                 size_t data_size) const
  {
    const Ioss::EntityBlock *parent_block = sb->parent_block();
    if (parent_block == nullptr) {
      std::ostringstream errmsg;
      fmt::print(errmsg,
                 "ERROR: CGNS: SideBlock {} does not have a parent-block specified.  This is "
                 "required for CGNS output.",
                 sb->name());
      IOSS_ERROR(errmsg);
    }

    int  base       = parent_block->get_property("base").get_int();
    int  zone       = parent_block->get_property("zone").get_int();
    auto num_to_get = field.verify(data_size);

    Ioss::Field::RoleType role = field.get_role();

    if (role == Ioss::Field::MESH) {
      // Handle the MESH fields required for a CGNS file model.
      // (The 'genesis' portion)
      if (field.get_name() == "element_side") {
        CGNS_ENUMT(ElementType_t) type = Utils::map_topology_to_cgns(sb->topology()->name());
        int sect                       = 0;

        int64_t size = num_to_get;
        MPI_Allreduce(MPI_IN_PLACE, &size, 1, Ioss::mpi_type(size), MPI_SUM, util().communicator());

        int cg_start = m_bcOffset[zone] + 1;
        int cg_end   = m_bcOffset[zone] + size;

        // NOTE: Currently not writing the "ElementConnectivity" data for the
        //       boundary condition.  It isn't used in the read and don't have
        //       the data so would have to generate it.  This may cause problems
        //       with codes that use the downstream data if they base the BC off
        //       of the nodes instead of the element/side info.
        // Get name from parent sideset...  This is name of the ZoneBC entry
        auto &name = sb->owner()->name();
        // This is the name of the BC_t node
        auto sb_name = Iocgns::Utils::decompose_sb_name(sb->name());

        CGNSIntVector point_range{cg_start, cg_end};
        CGCHECKM(cg_boco_write(get_file_pointer(), base, zone, name.c_str(),
                               CGNS_ENUMV(FamilySpecified), CGNS_ENUMV(PointRange), 2,
                               point_range.data(), &sect));
        CGCHECKM(
            cg_goto(get_file_pointer(), base, "Zone_t", zone, "ZoneBC_t", 1, "BC_t", sect, "end"));
        CGCHECKM(cg_famname_write(name.c_str()));
        CGCHECKM(cg_boco_gridlocation_write(get_file_pointer(), base, zone, sect,
                                            CGNS_ENUMV(FaceCenter)));

        CGCHECKM(cgp_section_write(get_file_pointer(), base, zone, sb_name.c_str(), type, cg_start,
                                   cg_end, 0, &sect));

        sb->property_update("section", sect);

        CGNSIntVector parent(4 * num_to_get);

        if (field.get_type() == Ioss::Field::INT32) {
          int   *idata = reinterpret_cast<int *>(data);
          size_t j     = 0;
          for (size_t i = 0; i < num_to_get; i++) {
            parent[num_to_get * 0 + i] = elemMap.global_to_local(idata[j++]); // Element
            parent[num_to_get * 2 + i] = idata[j++];
          }
        }
        else {
          auto  *idata = reinterpret_cast<int64_t *>(data);
          size_t j     = 0;
          for (size_t i = 0; i < num_to_get; i++) {
            parent[num_to_get * 0 + i] = elemMap.global_to_local(idata[j++]); // Element
            parent[num_to_get * 2 + i] = idata[j++];
          }
        }
        // Adjust face numbers to CGNS convention instead of IOSS convention...
        Utils::map_ioss_face_to_cgns(sb->parent_element_topology(), num_to_get, parent);
        map_local_to_global_implicit(parent, num_to_get, m_elemGlobalImplicitMap);

        int64_t local_face_count  = num_to_get;
        int64_t local_face_offset = 0;
        MPI_Exscan(&local_face_count, &local_face_offset, 1, Ioss::mpi_type(local_face_count),
                   MPI_SUM, util().communicator());
        cg_start = m_bcOffset[zone] + local_face_offset + 1;
        cg_end   = cg_start + local_face_count - 1;

        auto xx = num_to_get > 0 ? parent.data() : nullptr;
        if (num_to_get == 0) {
          cg_start = cg_end = 0;
        }
        CGCHECKM(cgp_parent_data_write(get_file_pointer(), base, zone, sect, cg_start, cg_end, xx));
        m_bcOffset[zone] += size;
      }
      else if (field.get_name() == "distribution_factors") {
        static bool warning_output = false;
        if (!warning_output) {
          if (myProcessor == 0) {
            fmt::print(Ioss::WARNING(),
                       "For CGNS output, the sideset distribution factors are not output.\n");
          }
          warning_output = true;
        }
        return 0;
      }
      else {
        num_to_get = Ioss::Utils::field_warning(sb, field, "output");
      }
    }
    else {
      num_to_get = Ioss::Utils::field_warning(sb, field, "output");
    }
    return num_to_get;
  }
  int64_t ParallelDatabaseIO::put_field_internal(const Ioss::SideSet * /* fs */,
                                                 const Ioss::Field & /* field */, void * /* data */,
                                                 size_t /* data_size */) const
  {
    return -1;
  }
  int64_t ParallelDatabaseIO::put_field_internal(const Ioss::CommSet * /* cs */,
                                                 const Ioss::Field & /* field*/, void * /*data*/,
                                                 size_t /*data_size*/) const
  {
    return -1;
  }

  void ParallelDatabaseIO::write_results_meta_data() {}

  unsigned ParallelDatabaseIO::entity_field_support() const
  {
    return Ioss::NODEBLOCK | Ioss::ELEMENTBLOCK | Ioss::STRUCTUREDBLOCK | Ioss::NODESET |
           Ioss::SIDESET | Ioss::REGION;
  }

  int64_t ParallelDatabaseIO::handle_node_ids(void *ids, int64_t num_to_get) const
  {
    /*!
     * There are two modes we need to support in this routine:
     * 1. Initial definition of node map (local->global) and
     * nodeMap.reverse (global->local).
     * 2. Redefinition of node map via 'reordering' of the original
     * map when the nodes on this processor are the same, but their
     * order is changed (or count because of ghosting)
     *
     * So, there will be two maps the 'nodeMap.map' map is a 'direct lookup'
     * map which maps current local position to global id and the
     * 'nodeMap.reverse' is an associative lookup which maps the
     * global id to 'original local'.  There is also a
     * 'nodeMap.reorder' which is direct lookup and maps current local
     * position to original local.

     * The ids coming in are the global ids; their position is the
     * "local id-1" (That is, data[0] contains the global id of local
     * node 1 in this node block).
     *
     * int local_position = nodeMap.reverse[NodeMap[i+1]]
     * (the nodeMap.map and nodeMap.reverse are 1-based)
     *
     * To determine which map to update on a call to this function, we
     * use the following hueristics:
     * -- If the database state is 'STATE_MODEL:', then update the
     *    'nodeMap.reverse' and 'nodeMap.map'
     *
     * -- If the database state is not STATE_MODEL, then leave the
     *    'nodeMap.reverse' and 'nodeMap.map' alone since they correspond to the
     *    information already written to the database. [May want to add a
     *    STATE_REDEFINE_MODEL]
     *
     * -- In both cases, update the nodeMap.reorder
     *
     * NOTE: The mapping is done on TRANSIENT fields only; MODEL fields
     *       should be in the original order...
     */
    if (!nodeMap.defined()) {
      nodeMap.set_size(num_to_get);

      bool in_define = (dbState == Ioss::STATE_MODEL) || (dbState == Ioss::STATE_DEFINE_MODEL);
      if (nodeMap.is_sequential()) {
        if (int_byte_size_api() == 4) {
          nodeMap.set_map(static_cast<int *>(ids), num_to_get, 0, in_define);
        }
        else {
          nodeMap.set_map(static_cast<int64_t *>(ids), num_to_get, 0, in_define);
        }
      }

      // Only a single nodeblock and all set
      assert(get_region()->get_property("node_block_count").get_int() == 1);
      nodeMap.set_defined(true);
    }
    return num_to_get;
  }

  std::vector<int64_t> ParallelDatabaseIO::get_processor_zone_node_offset() const
  {
    size_t               num_zones = m_globalToBlockLocalNodeMap.size();
    std::vector<int64_t> node_count(num_zones);
    std::vector<int64_t> node_offset(num_zones);

    for (const auto &block : m_globalToBlockLocalNodeMap) {
      auto        zone      = block.first;
      const auto &block_map = block.second;
      node_count[zone - 1]  = block_map->size();
    }
    MPI_Exscan(node_count.data(), node_offset.data(), num_zones, Ioss::mpi_type(node_count[0]),
               MPI_SUM, util().communicator());

    return node_offset;
  }

} // namespace Iocgns
#else
/*
 * Prevent warning in some versions of ranlib(1) because the object
 * file has no symbols.
 */
const char ioss_cgns_parallel_database_unused_symbol_dummy = '\0';
#endif
