// Copyright(C) 1999-2023 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include <vtk_cgns.h> // xxx(kitware)
#include VTK_CGNS(cgnsconfig.h)
#if CG_BUILD_PARALLEL
#include "cgns/Iocgns_Defines.h"

#include "Ioss_CodeTypes.h"
#include "Ioss_ParallelUtils.h"
#include "Ioss_SmartAssert.h"
#include "Ioss_Sort.h"
#include "Ioss_StructuredBlock.h"
#include "Ioss_Utils.h"
#include "cgns/Iocgns_DecompositionData.h"
#include "cgns/Iocgns_Utils.h"
#include "vtk_fmt.h"
#include VTK_FMT(fmt/color.h)
#include VTK_FMT(fmt/format.h)
#include VTK_FMT(fmt/ostream.h)
#include <tokenize.h>

#include VTK_CGNS(pcgnslib.h)

#include <algorithm>
#include <cassert>
#include <iomanip>
#include <numeric>

namespace {
  int rank = 0;

  // ZOLTAN Callback functions...

#if !defined(NO_ZOLTAN_SUPPORT)
  int zoltan_num_dim(void *data, int *ierr)
  {
    // Return dimensionality of coordinate data.
    auto *zdata = (Iocgns::DecompositionDataBase *)(data);

    *ierr = ZOLTAN_OK;
    return zdata->spatial_dimension();
  }

  int zoltan_num_obj(void *data, int *ierr)
  {
    // Return number of objects (element count) on this processor...
    auto *zdata = (Iocgns::DecompositionDataBase *)(data);

    *ierr = ZOLTAN_OK;
    return zdata->decomp_elem_count();
  }

  void zoltan_obj_list(void *data, int ngid_ent, int /* nlid_ent */, ZOLTAN_ID_PTR gids,
                       ZOLTAN_ID_PTR lids, int wdim, float *wgts, int *ierr)
  {
    // Return list of object IDs, both local and global.
    auto *zdata = (Iocgns::DecompositionDataBase *)(data);

    // At the time this is called, we don't have much information
    // These routines are the ones that are developing that
    // information...
    size_t element_count  = zdata->decomp_elem_count();
    size_t element_offset = zdata->decomp_elem_offset();

    *ierr = ZOLTAN_OK;

    if (lids != nullptr) {
      std::iota(lids, lids + element_count, 0);
    }

    if (wdim != 0) {
      std::fill(wgts, wgts + element_count, 1.0);
    }

    if (ngid_ent == 1) {
      std::iota(gids, gids + element_count, element_offset);
    }
    else if (ngid_ent == 2) {
      auto *global_ids = (int64_t *)gids;
      std::iota(global_ids, global_ids + element_count, element_offset);
    }
    else {
      *ierr = ZOLTAN_FATAL;
    }
  }

  void zoltan_geom(void *data, int /* ngid_ent */, int /* nlid_ent */, int /* nobj */,
                   ZOLTAN_ID_PTR /* gids */, ZOLTAN_ID_PTR /* lids */, int /* ndim */, double *geom,
                   int *ierr)
  {
    // Return coordinates for objects.
    auto *zdata = (Iocgns::DecompositionDataBase *)(data);

    std::copy(zdata->centroids().begin(), zdata->centroids().end(), &geom[0]);

    *ierr = ZOLTAN_OK;
  }
#endif

  // These are used for structured parallel decomposition...
  void create_zone_data(int cgns_file_ptr, std::vector<Iocgns::StructuredZoneData *> &zones,
                        Ioss_MPI_Comm comm)
  {
    Ioss::ParallelUtils par_util(comm);
    int                 myProcessor = par_util.parallel_rank(); // To make error macro work...
    int                 base        = 1;
    int                 num_zones   = 0;

    CGCHECK(cg_nzones(cgns_file_ptr, base, &num_zones));

    std::map<std::string, int> zone_name_map;

    for (cgsize_t zone = 1; zone <= num_zones; zone++) {
      cgsize_t size[9];
      char     zone_name[CGNS_MAX_NAME_LENGTH + 1];
      CGCHECK(cg_zone_read(cgns_file_ptr, base, zone, zone_name, size));
      zone_name_map[zone_name] = zone;

      SMART_ASSERT(size[0] - 1 == size[3])(size[0])(size[3]);
      SMART_ASSERT(size[1] - 1 == size[4])(size[1])(size[4]);
      SMART_ASSERT(size[2] - 1 == size[5])(size[2])(size[5]);

      assert(size[6] == 0);
      assert(size[7] == 0);
      assert(size[8] == 0);

      auto *zone_data = new Iocgns::StructuredZoneData(zone_name, zone, size[3], size[4], size[5]);
      zones.push_back(zone_data);

      // Handle zone-grid-connectivity...
      int nconn = 0;
      CGCHECK(cg_n1to1(cgns_file_ptr, base, zone, &nconn));
      for (int i = 0; i < nconn; i++) {
        char                    connectname[CGNS_MAX_NAME_LENGTH + 1];
        char                    donorname[CGNS_MAX_NAME_LENGTH + 1];
        std::array<cgsize_t, 6> range;
        std::array<cgsize_t, 6> donor_range;
        Ioss::IJK_t             transform;

        CGCHECK(cg_1to1_read(cgns_file_ptr, base, zone, i + 1, connectname, donorname, Data(range),
                             Data(donor_range), Data(transform)));

        // Get number of nodes shared with other "previous" zones...
        // A "previous" zone will have a lower zone number this this zone...
        int  donor_zone = -1;
        auto donor_iter = zone_name_map.find(donorname);
        if (donor_iter != zone_name_map.end()) {
          donor_zone = (*donor_iter).second;
        }
        Ioss::IJK_t range_beg{{(int)range[0], (int)range[1], (int)range[2]}};
        Ioss::IJK_t range_end{{(int)range[3], (int)range[4], (int)range[5]}};
        Ioss::IJK_t donor_beg{{(int)donor_range[0], (int)donor_range[1], (int)donor_range[2]}};
        Ioss::IJK_t donor_end{{(int)donor_range[3], (int)donor_range[4], (int)donor_range[5]}};

#if IOSS_DEBUG_OUTPUT
        if (rank == 0) {
          fmt::print(Ioss::DebugOut(), "Adding zgc {} to {} donor: {}\n", connectname, zone_name,
                     donorname);
        }
#endif
        zone_data->m_zoneConnectivity.emplace_back(connectname, zone, donorname, donor_zone,
                                                   transform, range_beg, range_end, donor_beg,
                                                   donor_end);
      }
    }

    // If there are any Structured blocks, need to iterate them and their 1-to-1 connections
    // and update the donor_zone id for zones that had not yet been processed at the time of
    // definition...
    for (auto &zone : zones) {
      for (auto &conn : zone->m_zoneConnectivity) {
        if (conn.m_donorZone < 0) {
          auto donor_iter = zone_name_map.find(conn.m_donorName);
          assert(donor_iter != zone_name_map.end());
          conn.m_donorZone = (*donor_iter).second;
        }
      }
    }
  }
} // namespace

namespace Iocgns {
  template DecompositionData<int>::DecompositionData(const Ioss::PropertyManager &props,
                                                     Ioss_MPI_Comm                communicator);
  template DecompositionData<int64_t>::DecompositionData(const Ioss::PropertyManager &props,
                                                         Ioss_MPI_Comm                communicator);

  template <typename INT>
  DecompositionData<INT>::DecompositionData(const Ioss::PropertyManager &props,
                                            Ioss_MPI_Comm                communicator)
      : DecompositionDataBase(), m_decomposition(props, communicator)
  {
    rank = m_decomposition.m_processor;

    if (props.exists("LOAD_BALANCE_THRESHOLD")) {
      if (props.get("LOAD_BALANCE_THRESHOLD").get_type() == Ioss::Property::STRING) {
        std::string lb_thresh  = props.get("LOAD_BALANCE_THRESHOLD").get_string();
        m_loadBalanceThreshold = std::stod(lb_thresh);
      }
      else if (props.get("LOAD_BALANCE_THRESHOLD").get_type() == Ioss::Property::REAL) {
        m_loadBalanceThreshold = props.get("LOAD_BALANCE_THRESHOLD").get_real();
      }
    }
    if (props.exists("LINE_DECOMPOSITION")) {
      m_lineDecomposition = props.get("LINE_DECOMPOSITION").get_string();
    }
  }

  template <typename INT>
  void DecompositionData<INT>::decompose_model(int filePtr, Ioss::MeshType mesh_type)
  {
    if (mesh_type == Ioss::MeshType::UNSTRUCTURED) {
      decompose_unstructured(filePtr);
    }
    else if (mesh_type == Ioss::MeshType::STRUCTURED) {
      decompose_structured(filePtr);
    }
#if IOSS_ENABLE_HYBRID
    else if (mesh_type == Ioss::MeshType::HYBRID) {
      std::ostringstream errmsg;
      fmt::print(errmsg, "ERROR: CGNS: The mesh type is HYBRID which is not supported for parallel "
                         "decomposition yet.");
      IOSS_ERROR(errmsg);
    }
#endif
    else {
      std::ostringstream errmsg;
      fmt::print(errmsg, "ERROR: CGNS: The mesh type is not Unstructured or Structured "
                         "which are the only types currently supported");
      IOSS_ERROR(errmsg);
    }
  }

  template <typename INT> void DecompositionData<INT>::decompose_structured(int filePtr)
  {
    m_decomposition.show_progress(__func__);
    create_zone_data(filePtr, m_structuredZones, m_decomposition.m_comm);
    if (m_structuredZones.empty()) {
      return;
    }

#if IOSS_DEBUG_OUTPUT
    bool verbose = true;
#else
    bool verbose = false;
#endif

    // Determine whether user has specified "line decompositions" for any of the zones.
    // The line decomposition is an ordinal which will not be split during the
    // decomposition.
    if (!m_lineDecomposition.empty()) {
      // See if the ordinal is specified as "__ordinal_{ijk}" which is used for testing...
      if (m_lineDecomposition.find("__ordinal_") == 0) {
        auto         sub = m_lineDecomposition.substr(10);
        unsigned int ord = 0;
        for (size_t i = 0; i < sub.size(); i++) {
          char ordinal = sub[i];
          ord |= ordinal == 'i' ? Ordinal::I : ordinal == 'j' ? Ordinal::J : Ordinal::K;
        }
        for (auto &zone : m_structuredZones) {
          if (zone->is_active()) {
            zone->m_lineOrdinal |= ord;
          }
        }
      }
      else {
        Utils::set_line_decomposition(filePtr, m_lineDecomposition, m_structuredZones, rank,
                                      verbose);
      }
    }

    // Do the processor decomposition.
    Utils::decompose_model(m_structuredZones, m_decomposition.m_processorCount, rank,
                           m_loadBalanceThreshold, verbose);

    Ioss::sort(m_structuredZones.begin(), m_structuredZones.end(),
               [](Iocgns::StructuredZoneData *a, Iocgns::StructuredZoneData *b) {
                 return a->m_zone < b->m_zone;
               });

    for (auto &zone : m_structuredZones) {
      if (zone->is_active()) {
        zone->resolve_zgc_split_donor(m_structuredZones);
      }
    }

    // Update and Output the processor assignments
    for (auto &zone : m_structuredZones) {
      if (zone->is_active()) {
        zone->update_zgc_processor(m_structuredZones);
#if IOSS_DEBUG_OUTPUT
        if (rank == 0) {
          auto zone_node_count =
              (zone->m_ordinal[0] + 1) * (zone->m_ordinal[1] + 1) * (zone->m_ordinal[2] + 1);
          fmt::print(
              Ioss::DebugOut(),
              "Zone {}({}) assigned to processor {}, Adam zone = {}, Cells = {}, Nodes = {}\n",
              zone->m_name, zone->m_zone, zone->m_proc, zone->m_adam->m_zone, zone->work(),
              zone_node_count);
          auto zgcs = zone->m_zoneConnectivity;
#if 0
	  // This should work, but doesn't...
          fmt::print(Ioss::DebugOut(), "{}\n", fmt::join(zgcs, "\n"));
#else
          for (auto &zgc : zgcs) {
            fmt::print(Ioss::DebugOut(), "{}\n", zgc);
          }
#endif
        }
#endif
      }
    }

    // Output the processor assignments in form similar to 'split' file
    if (rank == 0) {
      int z = 1;
      fmt::print(
          Ioss::OUTPUT(),
          "     n    proc  parent    imin    imax    jmin    jmax    kmin    kmax          work\n");
      auto tmp_zone(m_structuredZones);
      Ioss::sort(tmp_zone.begin(), tmp_zone.end(),
                 [](Iocgns::StructuredZoneData *a, Iocgns::StructuredZoneData *b) {
                   return a->m_proc < b->m_proc;
                 });

      for (auto &zone : tmp_zone) {
        if (zone->is_active()) {
          fmt::print(Ioss::OUTPUT(), "{:6d}{:8d}{:8d}{:8d}{:8d}{:8d}{:8d}{:8d}{:8d}{:14}\n", z++,
                     zone->m_proc, zone->m_adam->m_zone, zone->m_offset[0] + 1,
                     zone->m_ordinal[0] + zone->m_offset[0] + 1, zone->m_offset[1] + 1,
                     zone->m_ordinal[1] + zone->m_offset[1] + 1, zone->m_offset[2] + 1,
                     zone->m_ordinal[2] + zone->m_offset[2] + 1, fmt::group_digits(zone->work()));
        }
      }
    }

    for (auto &zone : m_structuredZones) {
      if (!zone->is_active()) {
        zone->m_proc = -1;
      }
    }

#if IOSS_DEBUG_OUTPUT
    MPI_Barrier(m_decomposition.m_comm);
    if (rank == 0) {
      fmt::print(Ioss::DebugOut(), "{}",
                 fmt::format(fg(fmt::color::green), "Returning from decomposition\n"));
    }
#endif
  }

  template <typename INT> void DecompositionData<INT>::decompose_unstructured(int filePtr)
  {
    m_decomposition.show_progress(__func__);

    // Initial decomposition is linear where processor #p contains
    // elements from (#p * #element/#proc) to (#p+1 * #element/#proc)

    // ========================================================================
    // Get the number of zones (element blocks) in the mesh...
    int num_zones = 0;
    int base      = 1; // Only single base supported so far.

    {
      int  cell_dimension = 0;
      int  phys_dimension = 0;
      char base_name[CGNS_MAX_NAME_LENGTH + 1];
      CGCHECK2(cg_base_read(filePtr, base, base_name, &cell_dimension, &phys_dimension));
      m_decomposition.m_spatialDimension = phys_dimension;
    }

    CGCHECK2(cg_nzones(filePtr, base, &num_zones));
    m_zones.resize(num_zones + 1); // Use 1-based zones.

    size_t global_cell_node_count = 0;
    size_t global_element_count   = 0;
    for (int zone = 1; zone <= num_zones; zone++) {
      // All zones are "Unstructured" since this was checked prior to
      // calling this function...
      cgsize_t size[3];
      char     zone_name[CGNS_MAX_NAME_LENGTH + 1];
      CGCHECK2(cg_zone_read(filePtr, base, zone, zone_name, size));

      INT total_block_nodes = size[0];
      INT total_block_elem  = size[1];

      m_zones[zone].m_nodeCount     = total_block_nodes;
      m_zones[zone].m_nodeOffset    = global_cell_node_count;
      m_zones[zone].m_name          = zone_name;
      m_zones[zone].m_elementOffset = global_element_count;
      global_cell_node_count += total_block_nodes;
      global_element_count += total_block_elem;
    }

    if (global_element_count < (size_t)m_decomposition.m_processorCount) {
      std::ostringstream errmsg;
      fmt::print(errmsg,
                 "ERROR: CGNS: Element Count ({}) is less than Processor Count ({}). No "
                 "decomposition possible.",
                 global_element_count, m_decomposition.m_processorCount);
      IOSS_ERROR(errmsg);
    }

    // Generate element_dist/node_dist --  size m_decomposition.m_processorCount + 1
    // processor p contains all elements/nodes from X_dist[p] .. X_dist[p+1]
    m_decomposition.generate_entity_distributions(global_cell_node_count, global_element_count);

    generate_adjacency_list(filePtr, m_decomposition);

    // Get min and max node used on this processor...
    auto min_max =
        std::minmax_element(m_decomposition.m_adjacency.begin(), m_decomposition.m_adjacency.end());
    INT min_node = *(min_max.first);
    INT max_node = *(min_max.second);
    generate_zone_shared_nodes(filePtr, min_node, max_node);

    // Now iterate adjacency list and update any "zone_shared_node" nodes
    // with their "sharee"
    if (!m_zoneSharedMap.empty()) {
      for (auto &node : m_decomposition.m_adjacency) {
        auto alias = m_zoneSharedMap.find(node);
        if (alias != m_zoneSharedMap.end()) {
          node = (*alias).second;
        }
      }
    }

#if IOSS_DEBUG_OUTPUT
    if (rank == 0) {
      fmt::print(Ioss::DebugOut(),
                 "Processor {0} has {1} elements; offset = {2}\n"
                 "Processor {0} has {3} nodes; offset = {4}.\n",
                 m_decomposition.m_processor, decomp_elem_count(), decomp_elem_offset(),
                 decomp_node_count(), decomp_node_offset());
    }
#endif

    if (m_decomposition.needs_centroids()) {
      // Get my coordinate data using direct cgns calls
      std::vector<double> x(decomp_node_count());
      std::vector<double> y;
      std::vector<double> z;

      get_file_node_coordinates(filePtr, 0, Data(x));
      if (m_decomposition.m_spatialDimension > 1) {
        y.resize(decomp_node_count());
        get_file_node_coordinates(filePtr, 1, Data(y));
      }
      if (m_decomposition.m_spatialDimension > 2) {
        z.resize(decomp_node_count());
        get_file_node_coordinates(filePtr, 2, Data(z));
      }

      m_decomposition.calculate_element_centroids(x, y, z);
    }

#if !defined(NO_ZOLTAN_SUPPORT)
    float version = 0.0;
    Zoltan_Initialize(0, nullptr, &version);

    Zoltan zz(m_decomposition.m_comm);

    // Register Zoltan Callback functions...
    zz.Set_Num_Obj_Fn(zoltan_num_obj, this);
    zz.Set_Obj_List_Fn(zoltan_obj_list, this);
    zz.Set_Num_Geom_Fn(zoltan_num_dim, this);
    zz.Set_Geom_Multi_Fn(zoltan_geom, this);
#endif

    m_decomposition.decompose_model(
#if !defined(NO_ZOLTAN_SUPPORT)
        zz,
#endif
        m_elementBlocks);

    if (!m_sideSets.empty()) {
      // Create elemGTL map which is used for sidesets (also element sets)
      build_global_to_local_elem_map();
    }

    get_sideset_data(filePtr);

    // Have all the decomposition data needed
    // Can now populate the Ioss metadata...
  }

  template <typename INT>
  void DecompositionData<INT>::generate_zone_shared_nodes(int filePtr, INT min_node, INT max_node)
  {
    // Begin of Zone-Shared node information

    // Modify adjacency list based on shared nodes between zones...
    // Need the map from "global" to "global-shared"
    // * This is not necessarily nodes only on my processor since connectivity can include
    //   nodes other than those I own.
    // * Potentially large number of shared nodes; practically small(?)

    // * Maintain hash map from old id to new (if any)
    // * TODO: Make more scalable

    int base = 1; // Only single base supported so far.

    // Donor zone is always lower numbered, so zone 1 has no donor zone. Start at zone 2.
    for (cgsize_t zone = 2; zone < (cgsize_t)m_zones.size(); zone++) {

      // Determine number of "shared" nodes (shared with other zones)
      int nconn = 0;
      CGCHECK2(cg_nconns(filePtr, base, zone, &nconn));
      for (int i = 0; i < nconn; i++) {
        char connectname[CGNS_MAX_NAME_LENGTH + 1];
        CGNS_ENUMT(GridLocation_t) location;
        CGNS_ENUMT(GridConnectivityType_t) connect_type;
        CGNS_ENUMT(PointSetType_t) ptset_type;
        cgsize_t npnts = 0;
        char     donorname[CGNS_MAX_NAME_LENGTH + 1];
        CGNS_ENUMT(ZoneType_t) donor_zonetype;
        CGNS_ENUMT(PointSetType_t) donor_ptset_type;
        CGNS_ENUMT(DataType_t) donor_datatype;
        cgsize_t ndata_donor;

        CGCHECK2(cg_conn_info(filePtr, base, zone, i + 1, connectname, &location, &connect_type,
                              &ptset_type, &npnts, donorname, &donor_zonetype, &donor_ptset_type,
                              &donor_datatype, &ndata_donor));

        if (connect_type != CGNS_ENUMV(Abutting1to1) || ptset_type != CGNS_ENUMV(PointList) ||
            donor_ptset_type != CGNS_ENUMV(PointListDonor)) {
          std::ostringstream errmsg;
          fmt::print(errmsg,
                     "ERROR: CGNS: Zone {} adjacency data is not correct type. Require "
                     "Abutting1to1 and PointList. {}\t{}\t{}",
                     zone, connect_type, ptset_type, donor_ptset_type);
          IOSS_ERROR(errmsg);
        }

        // Verify data consistency...
        if (npnts != ndata_donor) {
          std::ostringstream errmsg;
          fmt::print(errmsg,
                     "ERROR: CGNS: Zone {} point count ({}) does not match donor point count ({}).",
                     zone, npnts, ndata_donor);
          IOSS_ERROR(errmsg);
        }

        // Get number of nodes shared with other "previous" zones...
        // A "previous" zone will have a lower zone number this this zone...
        std::string dz_name(donorname);
        int         dz = 1;
        for (; dz < zone; dz++) {
          if (m_zones[dz].m_name == dz_name)
            break;
        }

        if (dz != zone) {
#if IOSS_DEBUG_OUTPUT
          if (m_decomposition.m_processor == 0) {
            fmt::print(Ioss::DebugOut(), "Zone {} shares {} nodes with {}\n", zone, npnts,
                       donorname);
          }
#endif
          // The 'ids' in 'points' and 'donors' will be zone-local 1-based.
          CGNSIntVector points(npnts);
          CGNSIntVector donors(npnts);

          CGCHECK2(
              cg_conn_read(filePtr, base, zone, i + 1, Data(points), donor_datatype, Data(donors)));

          for (int j = 0; j < npnts; j++) {
            // Convert to 0-based global id by subtracting 1 and adding zone.m_nodeOffset
            cgsize_t point = points[j] - 1 + m_zones[zone].m_nodeOffset;
            cgsize_t donor = donors[j] - 1 + m_zones[dz].m_nodeOffset;

            // See if 'donor' is mapped to a different node already
            auto donor_map = m_zoneSharedMap.find(donor);
            if (donor_map != m_zoneSharedMap.end()) {
              donor = (*donor_map).second;
            }
            m_zoneSharedMap.insert({point, donor});
#if IOSS_DEBUG_OUTPUT
            if (m_decomposition.m_processor == 0) {
              fmt::print(Ioss::DebugOut(), "Inserted {} to {}\n", point, donor);
            }
#endif
          }
        }
      }
    }
    // Filter m_zoneSharedMap down to nodes on this processor...
    // This processor contains global zone ids from `min_node` to `max_node`
    // global zone ids are the first entry in m_zoneShardedMap.
    for (auto it = m_zoneSharedMap.cbegin(); it != m_zoneSharedMap.cend(); /* no increment */) {
      if ((*it).first < min_node || (*it).first > max_node) {
        it = m_zoneSharedMap.erase(it);
      }
      else {
        ++it;
      }
    }
  }

  template <typename INT>
  void DecompositionData<INT>::generate_adjacency_list(int                       filePtr,
                                                       Ioss::Decomposition<INT> &decomposition)
  {
    int base = 1; // Only single base supported so far.

    // Range of elements currently handled by this processor [)
    size_t p_start = decomp_elem_offset();
    size_t p_end   = p_start + decomp_elem_count();

    size_t sum    = 0; // Size of adjacency vector.
    size_t offset = 0;

    int num_zones        = 0;
    INT zone_node_offset = 0;

    CGCHECK2(cg_nzones(filePtr, base, &num_zones));
    for (int zone = 1; zone <= num_zones; zone++) {
      // ========================================================================
      // Read the ZoneBC_t node to get list of SideBlocks to define on this zone
      // The BC_t nodes in the ZoneBC_t give the element range for each SideBlock
      // which can be matched up below with the Elements_t nodes to get contents
      // of the SideBlocks.
      auto zonebc =
          Utils::parse_zonebc_sideblocks(filePtr, base, zone, m_decomposition.m_processor);

      cgsize_t size[3];
      char     zone_name[CGNS_MAX_NAME_LENGTH + 1];
      CGCHECK2(cg_zone_read(filePtr, base, zone, zone_name, size));

      INT total_elements = size[1];
      // NOTE: A Zone will have a single set of nodes, but can have
      //       multiple sections each with their own element type...
      //       Keep treating sections as element blocks until we
      //       have handled 'size[1]' number of elements; the remaining
      //       sections are then the boundary faces (?)
      int num_sections = 0;
      CGCHECK2(cg_nsections(filePtr, base, zone, &num_sections));

      size_t last_blk_location = 0;
      for (int is = 1; is <= num_sections; is++) {
        char section_name[CGNS_MAX_NAME_LENGTH + 1];
        CGNS_ENUMT(ElementType_t) e_type;
        cgsize_t el_start    = 0;
        cgsize_t el_end      = 0;
        int      num_bndry   = 0;
        int      parent_flag = 0;

        // Get the type of elements in this section...
        CGCHECK2(cg_section_read(filePtr, base, zone, is, section_name, &e_type, &el_start, &el_end,
                                 &num_bndry, &parent_flag));

        INT num_entity = el_end - el_start + 1;

        if (parent_flag == 0 && total_elements > 0) {
          total_elements -= num_entity;

          // Range of elements in element block b [)
          size_t b_start = offset; // offset is index of first element in this block...
          offset += num_entity;
          size_t b_end = offset;

          int element_nodes;
          CGCHECK2(cg_npe(e_type, &element_nodes));

          if (b_start < p_end && p_start < b_end) {
            // Some of this blocks elements are on this processor...
            size_t overlap = std::min(b_end, p_end) - std::max(b_start, p_start);
            sum += overlap * element_nodes;
          }

          Ioss::BlockDecompositionData block;
          block.zone_          = zone;
          block.section_       = is;
          block.name_          = zone_name;
          block.topologyType   = Utils::map_cgns_to_topology_type(e_type);
          block.nodesPerEntity = element_nodes;
          block.fileCount      = num_entity;
          block.zoneNodeOffset = zone_node_offset;

          last_blk_location = m_elementBlocks.size();
          m_elementBlocks.push_back(block);
        }
        else {
          // This is a boundary-condition -- sideset (?)
          std::string bc_name(section_name);
          std::string ss_name;
          // Search zonebc (if it exists) for an entry such that the element ranges overlap.
          if (!zonebc.empty()) {
            size_t i = 0;
            for (; i < zonebc.size(); i++) {
              if (zonebc[i].range_beg >= el_start && zonebc[i].range_end <= el_end) {
                break;
              }
            }
            if (i < zonebc.size()) {
              ss_name = zonebc[i].name;
            }
          }
          else {
            ss_name = section_name;
          }

          Ioss::SetDecompositionData sset;
          sset.zone_            = zone;
          sset.section_         = is;
          sset.name_            = bc_name;
          sset.ss_name_         = ss_name;
          sset.fileCount        = num_entity;
          sset.topologyType     = Utils::map_cgns_to_topology_type(e_type);
          sset.parentBlockIndex = last_blk_location;
          m_sideSets.emplace_back(std::move(sset));
        }
      }
      zone_node_offset += size[0];
    }
    int block_count = (int)m_elementBlocks.size();

    // Get the global element block index list at this time also.
    // The global element at index 'I' (0-based) is on block B
    // if global_block_index[B] <= I && global_block_index[B+1] < I
    // allocate and TODO: Fill
    m_decomposition.m_fileBlockIndex.reserve(block_count + 1);
    for (auto &block : m_elementBlocks) {
      m_decomposition.m_fileBlockIndex.push_back(block.file_count());
    }
    m_decomposition.m_fileBlockIndex.push_back(0);
    Ioss::Utils::generate_index(m_decomposition.m_fileBlockIndex);

    // Make sure 'sum' can fit in INT...
    INT tmp_sum = (INT)sum;
    if ((size_t)tmp_sum != sum) {
      std::ostringstream errmsg;
      fmt::print(
          errmsg,
          "ERROR: The decomposition of this mesh requires 64-bit integers, but is being\n"
          "       run with 32-bit integer code. Please rerun with the property INTEGER_SIZE_API\n"
          "       set to 8. The details of how to do this vary with the code that is being run.\n"
          "       Contact gdsjaar@sandia.gov for more details.\n");
      IOSS_ERROR(errmsg);
    }

    // Now, populate the vectors...
    decomposition.m_pointer.reserve(decomp_elem_count() + 1);
    decomposition.m_adjacency.reserve(sum);
    offset = 0;
    sum    = 0; // Size of adjacency vector.

    for (auto &block : m_elementBlocks) {
      // Range of elements in element block b [)
      size_t b_start = offset; // offset is index of first element in this block...
      offset += block.file_count();
      size_t b_end = b_start + block.file_count();

      int64_t overlap      = std::min(b_end, p_end) - std::max(b_start, p_start);
      overlap              = std::max(overlap, (int64_t)0);
      block.fileCount      = overlap;
      size_t element_nodes = block.nodesPerEntity;
      int    zone          = block.zone_;
      int    section       = block.section_;

      // Get the connectivity (raw) for this portion of elements...
      CGNSIntVector connectivity(overlap * element_nodes);
      INT           blk_start = std::max(b_start, p_start) - b_start + 1;
      INT           blk_end   = blk_start + overlap - 1;
      blk_start               = blk_start < 0 ? 0 : blk_start;
      blk_end                 = blk_end < 0 ? 0 : blk_end;
#if IOSS_DEBUG_OUTPUT
      if (rank == 0) {
        fmt::print(Ioss::DebugOut(),
                   "Processor {} has {} elements on element block {}\t({} to {})\n",
                   m_decomposition.m_processor, overlap, block.name(), blk_start, blk_end);
      }
#endif
      block.fileSectionOffset = blk_start;
      CGCHECK2(cgp_elements_read_data(filePtr, base, zone, section, blk_start, blk_end,
                                      Data(connectivity)));
      size_t el          = 0;
      INT    zone_offset = block.zoneNodeOffset;

      for (size_t elem = 0; elem < static_cast<size_t>(overlap); elem++) {
        decomposition.m_pointer.push_back(decomposition.m_adjacency.size());
        for (size_t k = 0; k < element_nodes; k++) {
          INT node = connectivity[el++] - 1 + zone_offset; // 0-based node
          decomposition.m_adjacency.push_back(node);
        }
      }
      sum += overlap * element_nodes;
    }
    decomposition.m_pointer.push_back(decomposition.m_adjacency.size());
  }

  template <typename INT> void DecompositionData<INT>::get_sideset_data(int filePtr)
  {
    // NOTE: Not currently used; assume can read all on single processor...
    // Calculate the max "buffer" size usable for storing sideset
    // elemlists. This is basically the space used to store the file
    // decomposition nodal coordinates. The "decomp_node_count()/2*2" is to
    // equalize the decomp_node_count() among processors since some procs have 1
    // more node than others. For small models, assume we can handle
    // at least 10000 nodes.
    //    size_t max_size = std::max(10000, (decomp_node_count() / 2) * 2 * 3 *sizeof(double) /
    //    sizeof(cgsize_t));

    bool subsetting = false;

    if (subsetting) {
      assert(1 == 0);
    }
    else {
      for (auto &sset : m_sideSets) {

        auto          topology       = Ioss::ElementTopology::factory(sset.topologyType, true);
        int           nodes_per_face = topology->number_nodes();
        CGNSIntVector nodes(nodes_per_face * sset.file_count());

        // We get:
        // *  num_to_get parent elements,
        // *  num_to_get zeros (other parent element for face, but on boundary so 0)
        // *  num_to_get face_on_element
        // *  num_to_get zeros (face on other parent element)
        CGNSIntVector parent(4 * sset.file_count());

        int base = 1; // Only single base supported so far.
        CGCHECK2(cg_elements_read(filePtr, base, sset.zone(), sset.section(), Data(nodes),
                                  Data(parent)));

        if (parent[0] == 0) {
          // Get rid of 'parent' list -- not used.
          Ioss::Utils::clear(parent);

          // This model does not contain parent/face data; it only contains the
          // face connectivity (nodes) data.  Need to construct parent/face data
          // The faces in the list should all be boundaries of the block and should
          // therefore, not be shared with another block or shared across processors.
          // If we check the 'corner_node_cnt' nodes of the face and they are all
          // on this processor, then assume the face is on this processor...
          if (m_boundaryFaces[sset.zone()].empty()) {
            // Need map of proc-zone-implicit element ids to global-zone-implicit ids
            // so we can assign the correct element id to the faces.
            auto             blk = m_elementBlocks[sset.zone() - 1];
            std::vector<INT> file_data(blk.fileCount);
            std::iota(file_data.begin(), file_data.end(), blk.fileSectionOffset);
            std::vector<INT> zone_local_zone_global(blk.iossCount);
            communicate_element_data(Data(file_data), Data(zone_local_zone_global), 1);
            Ioss::Utils::clear(file_data);

            std::vector<INT> connectivity(blk.ioss_count() * blk.nodesPerEntity);
            get_block_connectivity(filePtr, Data(connectivity), sset.zone() - 1, true);

            auto topo = Ioss::ElementTopology::factory(blk.topologyType, true);
            // Should map the connectivity from cgns to ioss, but only use the lower order which is
            // same.
            Iocgns::Utils::generate_block_faces(topo, blk.ioss_count(), connectivity,
                                                m_boundaryFaces[sset.zone()],
                                                zone_local_zone_global);
          }

          // TODO: Should we filter down to just corner nodes?
          // Now, iterate the face connectivity vector and see if
          // there is a match in `m_boundaryFaces`
          size_t offset           = 0;
          auto  &boundary         = m_boundaryFaces[sset.zone()];
          int    num_corner_nodes = topology->number_corner_nodes();
          SMART_ASSERT(num_corner_nodes == 3 || num_corner_nodes == 4)(num_corner_nodes);

          for (size_t iface = 0; iface < sset.file_count(); iface++) {
            std::array<size_t, 4> conn = {{0, 0, 0, 0}};

            for (int i = 0; i < num_corner_nodes; i++) {
              conn[i] = nodes[offset + i];
            }
            offset += nodes_per_face;

            Ioss::Face face(conn);
            // See if face is in m_boundaryFaces
            // If not, then owned by another rank
            // If so, then get parent element and element side.
            auto it = boundary.find(face);
            if (it != boundary.end()) {
              sset.entitylist_map.push_back(iface);
            }
          }
        }
        else {
          size_t zone_element_id_offset = m_zones[sset.zone()].m_elementOffset;
          for (size_t i = 0; i < sset.file_count(); i++) {
            cgsize_t elem = parent[i] + zone_element_id_offset;
            // See if elem owned by this processor...
            if (i_own_elem(elem)) {
              // Save elem in this processors element list for this set.
              // The saved data is this elements location in the global
              // element list (parent) for this set.
              sset.entitylist_map.push_back(i);
            }
          }
        }
      }

      // Each processor knows how many of the sideset elems it owns;
      // broadcast that information (the count) to the other
      // processors. The first processor with non-zero elem count is
      // the "root" for this sideset.
      {
        std::vector<int> has_elems_local(m_sideSets.size());
        for (size_t i = 0; i < m_sideSets.size(); i++) {
          has_elems_local[i] = m_sideSets[i].entitylist_map.empty() ? 0 : 1;
        }

        std::vector<int> has_elems(m_sideSets.size() * m_decomposition.m_processorCount);
        MPI_Allgather(Data(has_elems_local), has_elems_local.size(), MPI_INT, Data(has_elems),
                      has_elems_local.size(), MPI_INT, m_decomposition.m_comm);

        for (size_t i = 0; i < m_sideSets.size(); i++) {
          m_sideSets[i].hasEntities.resize(m_decomposition.m_processorCount);
          m_sideSets[i].root_ = m_decomposition.m_processorCount;
          for (int p = 0; p < m_decomposition.m_processorCount; p++) {
            if (p < m_sideSets[i].root_ && has_elems[p * m_sideSets.size() + i] != 0) {
              m_sideSets[i].root_ = p;
            }
            m_sideSets[i].hasEntities[p] = has_elems[p * m_sideSets.size() + i];
          }
          int color = m_sideSets[i].hasEntities[m_decomposition.m_processor] ? 1 : MPI_UNDEFINED;
          MPI_Comm_split(m_decomposition.m_comm, color, m_decomposition.m_processor,
                         &m_sideSets[i].setComm_);
        }
      }
    }
  }

  template <typename INT>
  void DecompositionData<INT>::get_file_node_coordinates(int filePtr, int direction,
                                                         double *data) const
  {
    int      base        = 1; // Only single base supported so far.
    cgsize_t beg         = 0;
    cgsize_t end         = 0;
    cgsize_t offset      = 0;
    cgsize_t node_count  = decomp_node_count();
    cgsize_t node_offset = decomp_node_offset();

    int num_zones = (int)m_zones.size() - 1;
    for (int zone = 1; zone <= num_zones; zone++) {
      end += m_zones[zone].m_nodeCount;

      cgsize_t start  = std::max(node_offset, beg);
      cgsize_t finish = std::min(end, node_offset + node_count);
      cgsize_t count  = (finish > start) ? finish - start : 0;

      // Now adjust start for 1-based node numbering and the start of this zone...
      start  = start - beg + 1;
      finish = finish - beg;
      if (count == 0) {
        start  = 0;
        finish = 0;
      }
#if IOSS_DEBUG_OUTPUT
      if (rank == 0) {
        fmt::print(
            Ioss::DebugOut(),
            "{}: reading {} nodes from zone {} starting at {} with an offset of {} ending at {}\n",
            m_decomposition.m_processor, count, zone, start, offset, finish);
      }
#endif
      double *coords = nullptr;
      if (count > 0) {
        coords = &data[offset];
      }
      CGCHECK2(cgp_coord_read_data(filePtr, base, zone, direction + 1, &start, &finish, coords));
      offset += count;
      beg = end;
    }
  }

  template <typename INT>
  void DecompositionData<INT>::get_node_coordinates(int filePtr, double *ioss_data,
                                                    const Ioss::Field &field) const
  {
    std::vector<double> tmp(decomp_node_count());
    if (field.get_name() == "mesh_model_coordinates_x") {
      get_file_node_coordinates(filePtr, 0, Data(tmp));
      communicate_node_data(Data(tmp), ioss_data, 1);
    }

    else if (field.get_name() == "mesh_model_coordinates_y") {
      get_file_node_coordinates(filePtr, 1, Data(tmp));
      communicate_node_data(Data(tmp), ioss_data, 1);
    }

    else if (field.get_name() == "mesh_model_coordinates_z") {
      get_file_node_coordinates(filePtr, 2, Data(tmp));
      communicate_node_data(Data(tmp), ioss_data, 1);
    }

    else if (field.get_name() == "mesh_model_coordinates") {
      // Data required by upper classes store x0, y0, z0, ... xn,
      // yn, zn. Data stored in cgns file is x0, ..., xn, y0,
      // ..., yn, z0, ..., zn so we have to allocate some scratch
      // memory to read in the data and then map into supplied
      // 'data'

      std::vector<double> ioss_tmp(ioss_node_count());

      // This implementation trades off extra communication for
      // reduced memory overhead.
      // * This method uses 'ioss_node_count' extra memory; 3
      // reads; and 3 communicate_node_data calls.
      //
      // * Other method uses 6*ioss_node_count extra memory; 3 reads;
      // and 1 communicate_node_data call.
      //
      for (int d = 0; d < m_decomposition.m_spatialDimension; d++) {
        get_file_node_coordinates(filePtr, d, Data(tmp));
        communicate_node_data(Data(tmp), Data(ioss_tmp), 1);

        size_t index = d;
        for (size_t i = 0; i < ioss_node_count(); i++) {
          ioss_data[index] = ioss_tmp[i];
          index += m_decomposition.m_spatialDimension;
        }
      }
    }
  }

  template <typename INT>
  void DecompositionData<INT>::get_node_field(int filePtr, int step, int field_offset,
                                              double *ioss_data) const
  {
    std::vector<double> tmp(decomp_node_count());

    int      base        = 1; // Only single base supported so far.
    cgsize_t beg         = 0;
    cgsize_t end         = 0;
    cgsize_t offset      = 0;
    cgsize_t node_count  = decomp_node_count();
    cgsize_t node_offset = decomp_node_offset();

    int num_zones = (int)m_zones.size() - 1;
    for (int zone = 1; zone <= num_zones; zone++) {
      end += m_zones[zone].m_nodeCount;

      int solution_index =
          Utils::find_solution_index(filePtr, base, zone, step, CGNS_ENUMV(Vertex));

      cgsize_t start  = std::max(node_offset, beg);
      cgsize_t finish = std::min(end, node_offset + node_count);
      cgsize_t count  = (finish > start) ? finish - start : 0;

      // Now adjust start for 1-based node numbering and the start of this zone...
      start  = (count == 0) ? 0 : start - beg + 1;
      finish = (count == 0) ? 0 : finish - beg;

      double  *data         = (count > 0) ? &tmp[offset] : nullptr;
      cgsize_t range_min[1] = {start};
      cgsize_t range_max[1] = {finish};

      CGCHECK2(cgp_field_read_data(filePtr, base, zone, solution_index, field_offset, range_min,
                                   range_max, data));

      offset += count;
      beg = end;
    }
    communicate_node_data(Data(tmp), ioss_data, 1);
  }

  template void DecompositionData<int>::get_sideset_element_side(
      int filePtr, const Ioss::SetDecompositionData &sset, int *data) const;
  template void DecompositionData<int64_t>::get_sideset_element_side(
      int filePtr, const Ioss::SetDecompositionData &sset, int64_t *data) const;
  template <typename INT>
  void DecompositionData<INT>::get_sideset_element_side(int                               filePtr,
                                                        const Ioss::SetDecompositionData &sset,
                                                        INT *ioss_data) const
  {
    std::vector<INT> element_side;
    int              base = 1;

    auto          topology       = Ioss::ElementTopology::factory(sset.topologyType, true);
    int           nodes_per_face = topology->number_nodes();
    CGNSIntVector nodes(nodes_per_face * sset.file_count());

    CGNSIntVector parent(4 * sset.file_count());

    CGCHECK2(
        cg_elements_read(filePtr, base, sset.zone(), sset.section(), Data(nodes), Data(parent)));

    if (parent[0] == 0) {
      // Get rid of 'parent' list -- not used.
      Ioss::Utils::clear(parent);

      // This model does not contain parent/face data; it only contains the
      // face connectivity (nodes) data.  Need to construct parent/face data
      // The faces in the list should all be boundaries of the block and should
      // therefore, not be shared with another block or shared across processors.
      // If we check the 'corner_node_cnt' nodes of the face and they are all
      // on this processor, then assume the face is on this processor...

      // TODO: Should we filter down to just corner nodes?
      CGNSIntVector face_nodes(sset.entitylist_map.size() * nodes_per_face);
      communicate_set_data(Data(nodes), Data(face_nodes), sset, nodes_per_face);

      // Now, iterate the face connectivity vector and find a match in `m_boundaryFaces`
      size_t offset = 0;
      size_t j      = 0;

      // NOTE: The boundary face generation doesn't filter proc-boundary faces,
      //       so all zones will have boundary faces generated in 'get_sideset_data`
      assert(!m_boundaryFaces[sset.zone()].empty());
      auto &boundary = m_boundaryFaces[sset.zone()];

      int num_corner_nodes = topology->number_corner_nodes();
      SMART_ASSERT(num_corner_nodes == 3 || num_corner_nodes == 4)(num_corner_nodes);

      for (size_t iface = 0; iface < sset.ioss_count(); iface++) {
        std::array<size_t, 4> conn = {{0, 0, 0, 0}};

        for (int i = 0; i < num_corner_nodes; i++) {
          conn[i] = face_nodes[offset + i];
        }
        offset += nodes_per_face;

        size_t     zone_element_id_offset = m_zones[sset.zone()].m_elementOffset;
        Ioss::Face face(conn);
        // See if face is in m_boundaryFaces
        // If not, error
        // If so, then get parent element and element side.
        auto it = boundary.find(face);
        if (it != boundary.end()) {
          cgsize_t fid = (*it).element[0];
#if IOSS_DEBUG_OUTPUT
          fmt::print(Ioss::DebugOut(), "Connectivity: {} {} {} {} maps to element {}, face {}\n",
                     conn[0], conn[1], conn[2], conn[3], fid / 10, fid % 10 + 1);
#endif
          ioss_data[j++] = fid / 10 + zone_element_id_offset;
          ioss_data[j++] = fid % 10 + 1;
        }
        else {
          std::ostringstream errmsg;
          fmt::print(errmsg,
                     "ERROR: CGNS: Could not find face with connectivity {} {} {} {} on "
                     "sideblock {}.",
                     conn[0], conn[1], conn[2], conn[3], sset.name());
          IOSS_ERROR(errmsg);
        }
      }
    }
    else {
      // Get rid of 'nodes' list -- not used.
      Ioss::Utils::clear(nodes);

      // `parent` contains:
      // `num_to_get` parent elements, followed by --
      // `num_to_get` zeros (other parent element for face, but on boundary so 0)
      // `num_to_get` face_on_element
      // `num_to_get` zeros (face on other parent element)

      // Move from 'parent' to 'element_side' and interleave. element, side, element, side, ...
      element_side.reserve(sset.file_count() * 2);
      size_t zone_element_id_offset = m_zones[sset.zone()].m_elementOffset;
      for (size_t i = 0; i < sset.file_count(); i++) {
        element_side.push_back(parent[0 * sset.file_count() + i] + zone_element_id_offset);
        element_side.push_back(parent[2 * sset.file_count() + i]);
      }
      auto blk  = m_elementBlocks[sset.zone() - 1];
      auto topo = Ioss::ElementTopology::factory(blk.topologyType, true);
      Utils::map_cgns_face_to_ioss(topo, sset.file_count(), Data(element_side));
      // The above was all on root processor for this side set, now need to send data to other
      // processors that own any of the elements in the sideset.
      communicate_set_data(Data(element_side), ioss_data, sset, 2);
    }
  }

#ifndef DOXYGEN_SKIP_THIS
  template void DecompositionData<int>::get_block_connectivity(int filePtr, int *data, int blk_seq,
                                                               bool raw_ids) const;
  template void DecompositionData<int64_t>::get_block_connectivity(int filePtr, int64_t *data,
                                                                   int blk_seq, bool raw_ids) const;
#endif

  template <typename INT>
  void DecompositionData<INT>::get_block_connectivity(int filePtr, INT *data, int blk_seq,
                                                      bool raw_ids) const
  {
    auto          blk = m_elementBlocks[blk_seq];
    CGNSIntVector file_conn(blk.file_count() * blk.nodesPerEntity);
    int           base = 1;
    CGCHECK2(cgp_elements_read_data(filePtr, base, blk.zone(), blk.section(), blk.fileSectionOffset,
                                    blk.fileSectionOffset + blk.file_count() - 1, Data(file_conn)));

    if (!raw_ids) {
      // Map from zone-local node numbers to global implicit
      if (blk.zoneNodeOffset != 0) {
        for (auto &node : file_conn) {
          node += blk.zoneNodeOffset;
        }
      }

      if (!m_zoneSharedMap.empty()) {
        for (auto &node : file_conn) {
          ZoneSharedMap::const_iterator alias = m_zoneSharedMap.find(node - 1);
          if (alias != m_zoneSharedMap.end()) {
            node = (*alias).second + 1;
          }
        }
      }
    }

    communicate_block_data(Data(file_conn), data, blk, (size_t)blk.nodesPerEntity);
  }

#ifndef DOXYGEN_SKIP_THIS
  template void DecompositionData<int>::get_element_field(int filePtr, int solution_index,
                                                          int blk_seq, int field_index,
                                                          double *data) const;
  template void DecompositionData<int64_t>::get_element_field(int filePtr, int solution_index,
                                                              int blk_seq, int field_index,
                                                              double *data) const;
#endif

  template <typename INT>
  void DecompositionData<INT>::get_element_field(int filePtr, int solution_index, int blk_seq,
                                                 int field_index, double *data) const
  {
    const auto          blk = m_elementBlocks[blk_seq];
    std::vector<double> cgns_data(blk.file_count());
    int                 base         = 1;
    cgsize_t            range_min[1] = {(cgsize_t)blk.fileSectionOffset};
    cgsize_t            range_max[1] = {(cgsize_t)(blk.fileSectionOffset + blk.file_count() - 1)};

    CGCHECK2(cgp_field_read_data(filePtr, base, blk.zone(), solution_index, field_index, range_min,
                                 range_max, Data(cgns_data)));

    communicate_block_data(Data(cgns_data), data, blk, (size_t)1);
  }

  DecompositionDataBase::~DecompositionDataBase()
  {
    for (const auto &zone : m_structuredZones) {
      delete zone;
    }
  }

#ifndef DOXYGEN_SKIP_THIS
  template void DecompositionDataBase::communicate_node_data(int *file_data, int *ioss_data,
                                                             size_t comp_count) const;
  template void DecompositionDataBase::communicate_node_data(int64_t *file_data, int64_t *ioss_data,
                                                             size_t comp_count) const;
  template void DecompositionDataBase::communicate_node_data(double *file_data, double *ioss_data,
                                                             size_t comp_count) const;
#endif

  template <typename T>
  void DecompositionDataBase::communicate_node_data(T *file_data, T *ioss_data,
                                                    size_t comp_count) const
  {
    if (int_size() == sizeof(int)) {
      const DecompositionData<int> *this32 = dynamic_cast<const DecompositionData<int> *>(this);
      Ioss::Utils::check_dynamic_cast(this32);
      this32->communicate_node_data(file_data, ioss_data, comp_count);
    }
    else {
      const DecompositionData<int64_t> *this64 =
          dynamic_cast<const DecompositionData<int64_t> *>(this);
      Ioss::Utils::check_dynamic_cast(this64);
      this64->communicate_node_data(file_data, ioss_data, comp_count);
    }
  }

#ifndef DOXYGEN_SKIP_THIS
  template void DecompositionDataBase::communicate_element_data(int *file_data, int *ioss_data,
                                                                size_t comp_count) const;
  template void DecompositionDataBase::communicate_element_data(int64_t *file_data,
                                                                int64_t *ioss_data,
                                                                size_t   comp_count) const;
  template void DecompositionDataBase::communicate_element_data(double *file_data,
                                                                double *ioss_data,
                                                                size_t  comp_count) const;
#endif

  template <typename T>
  void DecompositionDataBase::communicate_element_data(T *file_data, T *ioss_data,
                                                       size_t comp_count) const
  {
    if (int_size() == sizeof(int)) {
      const DecompositionData<int> *this32 = dynamic_cast<const DecompositionData<int> *>(this);
      Ioss::Utils::check_dynamic_cast(this32);
      this32->communicate_element_data(file_data, ioss_data, comp_count);
    }
    else {
      const DecompositionData<int64_t> *this64 =
          dynamic_cast<const DecompositionData<int64_t> *>(this);
      Ioss::Utils::check_dynamic_cast(this64);
      this64->communicate_element_data(file_data, ioss_data, comp_count);
    }
  }

  void DecompositionDataBase::get_node_entity_proc_data(void                     *entity_proc,
                                                        const Ioss::MapContainer &node_map,
                                                        bool                      do_map) const
  {
    if (int_size() == sizeof(int)) {
      const DecompositionData<int> *this32 = dynamic_cast<const DecompositionData<int> *>(this);
      Ioss::Utils::check_dynamic_cast(this32);
      this32->m_decomposition.get_node_entity_proc_data((int *)entity_proc, node_map, do_map);
    }
    else {
      const DecompositionData<int64_t> *this64 =
          dynamic_cast<const DecompositionData<int64_t> *>(this);
      Ioss::Utils::check_dynamic_cast(this64);
      this64->m_decomposition.get_node_entity_proc_data((int64_t *)entity_proc, node_map, do_map);
    }
  }

  void DecompositionDataBase::get_block_connectivity(int filePtr, void *data, int blk_seq,
                                                     bool raw_ids) const
  {
    if (int_size() == sizeof(int)) {
      const DecompositionData<int> *this32 = dynamic_cast<const DecompositionData<int> *>(this);
      Ioss::Utils::check_dynamic_cast(this32);
      this32->get_block_connectivity(filePtr, (int *)data, blk_seq, raw_ids);
    }
    else {
      const DecompositionData<int64_t> *this64 =
          dynamic_cast<const DecompositionData<int64_t> *>(this);
      Ioss::Utils::check_dynamic_cast(this64);
      this64->get_block_connectivity(filePtr, (int64_t *)data, blk_seq, raw_ids);
    }
  }

  void DecompositionDataBase::get_element_field(int filePtr, int solution_index, int blk_seq,
                                                int field_index, double *data) const
  {
    if (int_size() == sizeof(int)) {
      const DecompositionData<int> *this32 = dynamic_cast<const DecompositionData<int> *>(this);
      Ioss::Utils::check_dynamic_cast(this32);
      this32->get_element_field(filePtr, solution_index, blk_seq, field_index, data);
    }
    else {
      const DecompositionData<int64_t> *this64 =
          dynamic_cast<const DecompositionData<int64_t> *>(this);
      Ioss::Utils::check_dynamic_cast(this64);
      this64->get_element_field(filePtr, solution_index, blk_seq, field_index, data);
    }
  }

  void DecompositionDataBase::get_node_field(int filePtr, int step, int field_index,
                                             double *data) const
  {
    if (int_size() == sizeof(int)) {
      const DecompositionData<int> *this32 = dynamic_cast<const DecompositionData<int> *>(this);
      Ioss::Utils::check_dynamic_cast(this32);
      this32->get_node_field(filePtr, step, field_index, data);
    }
    else {
      const DecompositionData<int64_t> *this64 =
          dynamic_cast<const DecompositionData<int64_t> *>(this);
      Ioss::Utils::check_dynamic_cast(this64);
      this64->get_node_field(filePtr, step, field_index, data);
    }
  }

  void DecompositionDataBase::get_sideset_element_side(int                               filePtr,
                                                       const Ioss::SetDecompositionData &sset,
                                                       void                             *data) const
  {
    if (int_size() == sizeof(int)) {
      const DecompositionData<int> *this32 = dynamic_cast<const DecompositionData<int> *>(this);
      Ioss::Utils::check_dynamic_cast(this32);
      this32->get_sideset_element_side(filePtr, sset, (int *)data);
    }
    else {
      const DecompositionData<int64_t> *this64 =
          dynamic_cast<const DecompositionData<int64_t> *>(this);
      Ioss::Utils::check_dynamic_cast(this64);
      this64->get_sideset_element_side(filePtr, sset, (int64_t *)data);
    }
  }
} // namespace Iocgns
#else
/*
 * Prevent warning in some versions of ranlib(1) because the object
 * file has no symbols.
 */
const char ioss_cgns_decomposition_data_unused_symbol_dummy = '\0';
#endif
