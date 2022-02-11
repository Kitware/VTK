// CGNS Assumptions:
// * All boundary conditions are listed as Family nodes at the "top" level.
// * Single element block per zone.
// * Single Base.
// * ZoneGridConnectivity is 1to1 with point lists for unstructured

// Copyright(C) 1999-2022 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include <Ioss_CodeTypes.h>
#include <Ioss_Sort.h>
#include <Ioss_Utils.h>
#include <bitset>
#include <cgns/Iocgns_DatabaseIO.h>
#include <cgns/Iocgns_Utils.h>
#include <cstddef>
#include <ctime>
#include "vtk_fmt.h"
#include VTK_FMT(fmt/ostream.h)
#include <fstream>
#include <iostream>
#include <numeric>
#include <string>
#if !defined(__IOSS_WINDOWS__)
#include <sys/select.h>
#endif
#include <tokenize.h>
#include <vector>

#include "vtk_cgns.h"
#include VTK_CGNS(cgnslib.h)
#if CG_BUILD_PARALLEL
#include VTK_CGNS(pcgnslib.h)
#endif

#include <cgns/Iocgns_Defines.h>

#if !defined(CGNSLIB_H)
#error "Could not include cgnslib.h"
#endif

#include "Ioss_FileInfo.h"
#include "Ioss_Hex8.h"
#include "Ioss_Quad4.h"
#include "Ioss_SmartAssert.h"
#include "Ioss_SubSystem.h"

// extern char hdf5_access[64];

namespace {
  size_t global_to_zone_local_idx(size_t i, const Ioss::Map *block_map, const Ioss::Map &nodeMap,
                                  bool isParallel)
  {
    auto global = block_map->map()[i + 1]; // 1-based index over all nodes in model (all procs)
    if (isParallel) {
      return nodeMap.global_to_local(global) - 1;
    }
    return global - 1;
  }

#if CG_BUILD_PARALLEL
  bool has_decomp_descriptor(int cgns_file_ptr, int base, int zone, int zgc_idx)
  {
    bool has_decomp_flag = false;
    if (cg_goto(cgns_file_ptr, base, "Zone_t", zone, "ZoneGridConnectivity", 0,
                "GridConnectivity1to1_t", zgc_idx, "end") == CG_OK) {
      int ndescriptor = 0;
      cg_ndescriptors(&ndescriptor);
      if (ndescriptor > 0) {
        for (int i = 0; i < ndescriptor; i++) {
          char  name[33];
          char *text;
          cg_descriptor_read(i + 1, name, &text);
          if (strcmp(name, "Decomp") == 0) {
            has_decomp_flag = true;
            break;
          }
          cg_free(text);
        }
      }
    }
    return has_decomp_flag;
  }

  bool has_decomp_name_kluge(int cgns_file_ptr, int base, int zone, int zgc_idx)
  {
    // Check the `zgc_idx`-th ZGC node to see if the name matches the
    // form described in the `name_is_decomp` function below.  We want to
    // see if there are *any* names that match this form and if so, we can
    // use the kluge; otherwise we can't and need to rely on other hueristics.
    char                    connectname[CGNS_MAX_NAME_LENGTH + 1];
    char                    donorname[CGNS_MAX_NAME_LENGTH + 1];
    std::array<cgsize_t, 6> range;
    std::array<cgsize_t, 6> donor_range;
    Ioss::IJK_t             transform;

    cg_1to1_read(cgns_file_ptr, base, zone, zgc_idx, connectname, donorname, range.data(),
                 donor_range.data(), transform.data());

    std::string name{connectname};
    bool        has_decomp_name = ((name.find_first_not_of("0123456789_-") == std::string::npos) &&
                            (name.find("--", 1 != std::string::npos)));

    return has_decomp_name;
  }

  bool name_is_decomp(const std::string &name)
  {
    // Major kluge to deal with fpp files which don't have the
    // decomp descriptor.  Usually only required if the model is
    // periodic...
    //
    // A zgc name that is generated as part of the decomposition process
    // has the following characteristics:
    // * is all [0-9_-] characters
    // * has "--" in the middle (approx) of the name
    return ((name.find_first_not_of("0123456789_-") == std::string::npos) &&
            (name.find("--", 1 != std::string::npos)));
  }

  void zgc_check_descriptor(int cgns_file_ptr, int base, int zone, int zgc_idx,
                            Ioss::ZoneConnectivity &zgc)
  {
    if (cg_goto(cgns_file_ptr, base, "Zone_t", zone, "ZoneGridConnectivity", 0,
                "GridConnectivity1to1_t", zgc_idx, "end") == CG_OK) {
      int ndescriptor = 0;
      cg_ndescriptors(&ndescriptor);
      if (ndescriptor > 0) {
        for (int i = 0; i < ndescriptor; i++) {
          char  name[33];
          char *text = nullptr;
          cg_descriptor_read(i + 1, name, &text);
          if (strcmp(name, "OriginalName") == 0) {
            zgc.m_connectionName = text;
            cg_free(text);
            break;
          }
          if (strcmp(name, "Decomp") == 0) {
            zgc.m_fromDecomp = true;
            cg_free(text);
            break;
          }
          cg_free(text);
        }
      }
    }
  }
#endif

  template <typename T> void pack(int &idx, std::vector<int> &pack, T *from, int count)
  {
    for (int i = 0; i < count; i++) {
      pack[idx++] = from[i];
    }
  }

  template <typename T> void unpack(int &idx, const T *pack, T *to, int count)
  {
    for (int i = 0; i < count; i++) {
      to[i] = pack[idx++];
    }
  }

  struct SBlock
  {
    SBlock() = default;
    SBlock(char *names, const int *data) : name(std::string{names})
    {
      int idx = 0;
      proc    = data[idx++];
      unpack(idx, data, range.data(), 3);
      local_zone = data[idx++];
    }
    std::string                      name{};
    int                              proc{-1};
    int                              local_zone{};
    std::vector<std::pair<int, int>> adjacency; // face, proc pairs
    std::array<int, 3>               range{{0, 0, 0}};
    std::array<int, 3>               glob_range{{0, 0, 0}};
    std::array<int, 3>               offset{{0, 0, 0}};
    std::bitset<6>                   face_adj{};

#if CG_BUILD_PARALLEL
    bool split() const { return face_adj.any(); }
#endif
  };

#if CG_BUILD_PARALLEL
  void add_zgc_fpp(int cgns_file_ptr, Ioss::StructuredBlock *block,
                   const std::map<std::string, int> &zone_name_map, int myProcessor,
                   bool isParallel)
  {
    int base    = block->get_property("base").get_int();
    int zone    = block->get_property("zone").get_int();
    int db_zone = Iocgns::Utils::get_db_zone(block);
    int nconn   = 0;
    CGCHECK(cg_n1to1(cgns_file_ptr, base, db_zone, &nconn));

    for (int ii = 0; ii < nconn; ii++) {
      char                    connectname[CGNS_MAX_NAME_LENGTH + 1];
      char                    donorname[CGNS_MAX_NAME_LENGTH + 1];
      std::array<cgsize_t, 6> range;
      std::array<cgsize_t, 6> donor_range;
      Ioss::IJK_t             transform;

      CGCHECK(cg_1to1_read(cgns_file_ptr, base, db_zone, ii + 1, connectname, donorname,
                           range.data(), donor_range.data(), transform.data()));

      auto        donorname_proc = Iocgns::Utils::decompose_name(donorname, isParallel);
      std::string donor_name     = donorname_proc.first;

      // Get number of nodes shared with other "previous" zones...
      // A "previous" zone will have a lower zone number this this zone...
      int  donor_zone = -1;
      auto donor_iter = zone_name_map.find(donor_name);
      if (donor_iter != zone_name_map.end()) {
        donor_zone = (*donor_iter).second;
      }
      Ioss::IJK_t range_beg{{(int)range[0], (int)range[1], (int)range[2]}};
      Ioss::IJK_t range_end{{(int)range[3], (int)range[4], (int)range[5]}};
      Ioss::IJK_t donor_beg{{(int)donor_range[0], (int)donor_range[1], (int)donor_range[2]}};
      Ioss::IJK_t donor_end{{(int)donor_range[3], (int)donor_range[4], (int)donor_range[5]}};

      Ioss::IJK_t offset = block->get_ijk_offset();
      range_beg[0] += offset[0];
      range_beg[1] += offset[1];
      range_beg[2] += offset[2];
      range_end[0] += offset[0];
      range_end[1] += offset[1];
      range_end[2] += offset[2];

      auto con_name = Iocgns::Utils::decompose_name(connectname, isParallel).first;
      block->m_zoneConnectivity.emplace_back(con_name, zone, donor_name, donor_zone, transform,
                                             range_beg, range_end, donor_beg, donor_end, offset);

      block->m_zoneConnectivity.back().m_ownerProcessor = myProcessor;
      block->m_zoneConnectivity.back().m_donorProcessor = donorname_proc.second;

      if (isParallel) {
        zgc_check_descriptor(cgns_file_ptr, base, db_zone, ii + 1,
                             block->m_zoneConnectivity.back());
      }
    }
  }

  int adjacent_block(const SBlock &b, int ijk, std::map<int, int> &proc_block_map)
  {
    // Find a block the the 'left|right|up|down|front|back' (ijk) of blocks[br]
    if (b.face_adj[ijk]) {
      for (auto adj : b.adjacency) {
        if (adj.first == ijk) {
          int proc = adj.second;
          return proc_block_map[proc];
        }
      }
    }
    return -1;
  }

  void set_block_offset(size_t begin, size_t end, std::vector<SBlock> &blocks,
                        std::map<int, int> &proc_block_map)
  {
    for (size_t p = 0; p < (end - begin); p++) {
      for (size_t j = begin; j < end; j++) {
        auto &block = blocks[j];
        // See which blocks are below/left/under this block which means
        // that this blocks offset is affected.
        for (int ijk = 0; ijk < 3; ijk++) {
          int br = adjacent_block(block, ijk, proc_block_map);
          if (br >= 0) {
            block.offset[ijk] = blocks[br].offset[ijk] + blocks[br].range[ijk];
          }
        }
      }
    }
  }

  void set_global_extent(size_t begin, size_t end, std::vector<SBlock> &blocks,
                         std::map<int, int> &proc_block_map)
  {
    // Determine the global ijk extent for the block which is spread over multiple processors
    // and is in the range [begin, end) in blocks.
    Ioss::IJK_t global{0, 0, 0};
    for (int ijk = 0; ijk < 3; ijk++) {
      // Find a block in range [bbeg, bend) with no block to the "left|below|behind
      for (size_t bb = begin; bb < end; bb++) {
        if (blocks[bb].face_adj[ijk] == 0) {
          // No blocks to min 'ijk' direction...
          // Traverse all blocks toward max 'ijk' direction setting offsets and global range.
          size_t iter = 0;
          int    br   = bb;
          do {
            global[ijk] += blocks[br].range[ijk];
#if IOSS_DEBUG_OUTPUT
            const auto b = blocks[br];
            fmt::print(Ioss::DEBUG(), "Min {}: {} {} ({} {} {})  [{}]\n",
                       (ijk == 0   ? 'i'
                        : ijk == 1 ? 'j'
                                   : 'k'),
                       b.name, b.face_adj[ijk], b.range[0], b.range[1], b.range[2],
                       b.face_adj.to_string('.', '+'));
#endif
            br = adjacent_block(blocks[br], ijk + 3, proc_block_map);
            if (++iter > end - begin) {
              auto               bp = adjacent_block(blocks[br], ijk + 3, proc_block_map);
              std::ostringstream errmsg;
              fmt::print(errmsg,
                         "ERROR: CGNS: Block '{}' is in infinite loop calculating processor "
                         "adjacencies for direction "
                         "'{}' on processors {} and {}.  Check decomposition.",
                         blocks[bb].name,
                         (ijk == 0   ? 'i'
                          : ijk == 1 ? 'j'
                                     : 'k'),
                         blocks[bp].proc, blocks[br].proc);
              IOSS_ERROR(errmsg);
            }
          } while (br >= 0);
          break;
        }
      }
    }
    for (size_t bb = begin; bb < end; bb++) {
      blocks[bb].glob_range = global;
    }
  }

  int find_face(const std::array<cgsize_t, 6> &range)
  {
    // 0,1,2 == min x,y,z; 3,4,5 == Max x,y,z
    bool is_x = range[0] == range[3];
    bool is_y = range[1] == range[4];
#ifndef NDEBUG
    bool is_z = range[2] == range[5];
#endif
    SMART_ASSERT(is_x || is_y || is_z);
    SMART_ASSERT((is_x ? 1 : 0) + (is_y ? 1 : 0) + (is_z ? 1 : 0) == 1);
    int idx = is_x ? 0 : is_y ? 1 : 2;

    // Which face on this block?
    int face = idx;
    if (range[idx] != 1) {
      face += 3;
    }
    return face;
  }

  bool generate_inter_proc_adjacency(int cgns_file_ptr, int base, int zone, int myProcessor,
                                     const std::string &zone_name, std::vector<int> &adjacency)
  {
    // Handle zone-grid-connectivity... At this point we only want
    // the zgc that are inter-proc between the same "base zone".
    // That is, the zgc which are result of parallel decomp.

    // Stored in format:  "-myproc, -local_zone, face, shared_proc" for each shared face.
    bool zone_added = false;
    int  nconn      = 0;
    CGCHECK(cg_n1to1(cgns_file_ptr, base, zone, &nconn));

    // See if any of the zgc have a "Decomp" descriptor node.  If so, then
    // We can unambiguously determine whether a ZGC is from decomp or is
    // normal inter-zone ZGC. If the descriptor does not exist, then have
    // to rely on hueristics...
    bool has_decomp_flag  = false;
    bool has_decomp_names = false;
    for (int i = 0; i < nconn; i++) {
      if (has_decomp_descriptor(cgns_file_ptr, base, zone, i + 1)) {
        has_decomp_flag = true;
      }
      if (has_decomp_name_kluge(cgns_file_ptr, base, zone, i + 1)) {
        has_decomp_names = true;
      }
    }

#if IOSS_DEBUG_OUTPUT
    fmt::print("CGNS DatabaseIO has decomp flag? {}\n", has_decomp_flag);
#endif

    for (int i = 0; i < nconn; i++) {
      char                    connectname[CGNS_MAX_NAME_LENGTH + 1];
      char                    donorname[CGNS_MAX_NAME_LENGTH + 1];
      std::array<cgsize_t, 6> range;
      std::array<cgsize_t, 6> donor_range;
      Ioss::IJK_t             transform;

      CGCHECK(cg_1to1_read(cgns_file_ptr, base, zone, i + 1, connectname, donorname, range.data(),
                           donor_range.data(), transform.data()));

      auto        donorname_proc = Iocgns::Utils::decompose_name(donorname, true);
      std::string donor_name     = donorname_proc.first;
      auto        donor_proc     = donorname_proc.second;

      bool is_from_decomp = false;
      if (has_decomp_flag) {
        is_from_decomp = has_decomp_descriptor(cgns_file_ptr, base, zone, i + 1);
      }
      else {
#if IOSS_DEBUG_OUTPUT
        fmt::print("Name: {}, decomp? = {}\n", connectname, name_is_decomp(connectname));
#endif
        is_from_decomp = donor_name == zone_name && donor_proc >= 0 && donor_proc != myProcessor &&
                         (!has_decomp_names || name_is_decomp(connectname));
      }

      if (is_from_decomp) {
        // See if the descriptor named "Decomp" exists as a child of this ZGC.
        // If so, then
        // Determine which face of the zone on this processor is
        // shared with the other processor...
        int face = find_face(range);
        adjacency.push_back(-myProcessor);
        adjacency.push_back(-zone);
        adjacency.push_back(face);
        adjacency.push_back(donorname_proc.second);
        zone_added = true;
      }
    }
    return zone_added;
  }

  void set_adjacency(SBlock &b, std::vector<int> &adjacency)
  {
    // Stored in format:  "-myproc, -local_zone, face, shared_proc" for each shared face.
    for (size_t i = 0; i < adjacency.size(); i += 4) {
      SMART_ASSERT(adjacency[i] <= 0); // -proc
      if (adjacency[i] == -b.proc) {
        SMART_ASSERT(adjacency[i + 1] < 0);
        if (adjacency[i + 1] == -b.local_zone) {
          b.adjacency.emplace_back(adjacency[i + 2], adjacency[i + 3]);
          b.face_adj.set(adjacency[i + 2]);
        }
      }
      else if (adjacency[i] < -b.proc) {
        return;
      }
    }
  }

  void add_empty_bc(Ioss::SideSet *sset, Ioss::StructuredBlock *block, int base, int zone, int face,
                    const std::string &fam_name, const std::string &boco_name)
  {
    SMART_ASSERT(sset != nullptr);

    Ioss::IJK_t empty_range{{0, 0, 0}};

    auto sbc   = Ioss::BoundaryCondition(boco_name, fam_name, empty_range, empty_range);
    sbc.m_face = face;
    block->m_boundaryConditions.push_back(sbc);

    std::string name = boco_name + "/" + block->name();

    auto sb =
        new Ioss::SideBlock(block->get_database(), name, Ioss::Quad4::name, Ioss::Hex8::name, 0);
    sb->set_parent_block(block);
    sset->add(sb);
    sb->property_add(Ioss::Property("base", base));
    sb->property_add(Ioss::Property("zone", zone));
    sb->property_add(Ioss::Property("section", face + 1));
    sb->property_add(Ioss::Property("id", sset->get_property("id").get_int()));
    sb->property_add(Ioss::Property(
        "guid", block->get_database()->util().generate_guid(sset->get_property("id").get_int())));
  }
#endif

  size_t handle_block_ids(const Ioss::EntityBlock *eb, Ioss::Map &entity_map, void *ids,
                          size_t num_to_get, const Ioss::Field::BasicType &size)
  {
    /*!
     * CGNS doesn't support element global ids, so the only use of this
     * routine is the case where we may be translating from a mesh that
     * *does* support global ids and we will then need to map those
     * global ids back to local ids in, for example, the sideset element list.
     *
     * There will be two maps the 'entity_map.map' map is a 'direct lookup'
     * map which maps current local position to global id and the
     * 'entity_map.reverse' is an associative lookup which maps the
     * global id to 'original local'.  There is also a
     * 'entity_map.reorder' which is direct lookup and maps current local
     * position to original local.

     * The ids coming in are the global ids; their position is the
     * local id -1 (That is, data[0] contains the global id of local
     * element 1 in this element block).  The 'model-local' id is
     * given by eb_offset + 1 + position:
     *
     * int local_position = entity_map.reverse[ElementMap[i+1]]
     * (the entity_map.map and entity_map.reverse are 1-based)
     *
     * But, this assumes 1..numel elements are being output at the same
     * time; we are actually outputting a blocks worth of elements at a
     * time, so we need to consider the block offsets.
     * So... local-in-block position 'i' is index 'eb_offset+i' in
     * 'entity_map.map' and the 'local_position' within the element
     * blocks data arrays is 'local_position-eb_offset'.  With this, the
     * position within the data array of this element block is:
     *
     * int eb_position =
     * entity_map.reverse[entity_map.map[eb_offset+i+1]]-eb_offset-1
     *
     * NOTE: the maps are built an element block at a time...
     */

    // Overwrite this portion of the 'entity_map.map', but keep other
    // parts as they were.  We are adding elements starting at position
    // 'eb_offset+offset' and ending at
    // 'eb_offset+offset+num_to_get'. If the entire block is being
    // processed, this reduces to the range 'eb_offset..eb_offset+my_element_count'

    int64_t eb_offset = eb->get_offset();
    if (size == Ioss::Field::INT64) {
      entity_map.set_map(static_cast<int64_t *>(ids), num_to_get, eb_offset, true);
    }
    else {
      entity_map.set_map(static_cast<int *>(ids), num_to_get, eb_offset, true);
    }

    return num_to_get;
  }
} // namespace

namespace Iocgns {

  DatabaseIO::DatabaseIO(Ioss::Region *region, const std::string &filename,
                         Ioss::DatabaseUsage db_usage, MPI_Comm communicator,
                         const Ioss::PropertyManager &props)
      : Ioss::DatabaseIO(region, filename, db_usage, communicator, props)
  {
    dbState = Ioss::STATE_UNKNOWN;

#if IOSS_DEBUG_OUTPUT
    if (myProcessor == 0) {
      fmt::print("CGNS DatabaseIO using {}-bit integers\n", CG_SIZEOF_SIZE);
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

  DatabaseIO::~DatabaseIO()
  {
    for (auto &gtb : m_globalToBlockLocalNodeMap) {
      delete gtb.second;
    }
    try {
      if (m_cgnsBasePtr > 0) {
        CGCHECKM(cg_close(m_cgnsBasePtr));
        m_cgnsBasePtr = -1;
      }
      closeDatabase__();
    }
    catch (...) {
    }
  }

  int DatabaseIO::get_file_pointer() const
  {
    if (m_cgnsFilePtr < 0) {
      openDatabase__();
    }
    return m_cgnsFilePtr;
  }

  void DatabaseIO::openDatabase__() const
  {
    if (m_cgnsFilePtr < 0) {
#if 0
      // This is currently disabled due to a recent change in CGNS
      // that changed how `hdf5_access` was dealt with...  Since
      // memory_read and memory_write are experimental in SEACAS/IOSS,
      // I disabled until we can determine how best to handle this in
      // current CGNS.
      if ((is_input() && properties.exists("MEMORY_READ")) ||
          (!is_input() && properties.exists("MEMORY_WRITE"))) {
	Ioss::Utils::copy_string(hdf5_access, "PARALLEL");
      }
#endif

      CGCHECKM(cg_set_file_type(CG_FILE_HDF5));

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

#if CG_BUILD_PARALLEL
      cgp_mpi_comm(MPI_COMM_SELF);
      int ierr = cgp_open(decoded_filename().c_str(), mode, &m_cgnsFilePtr);
      cgp_mpi_comm(util().communicator());
#else
      int ierr = cg_open(decoded_filename().c_str(), mode, &m_cgnsFilePtr);
#endif
      // Will not return if error...
      check_valid_file_open(ierr);
#if 0
      if ((is_input() && properties.exists("MEMORY_READ")) ||
          (!is_input() && properties.exists("MEMORY_WRITE"))) {
        Ioss::Utils::copy_string(hdf5_access, "NATIVE");
      }
#endif

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
        Utils::update_db_zone_property(m_cgnsFilePtr, get_region(), myProcessor, isParallel, false);
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
    SMART_ASSERT(m_cgnsFilePtr >= 0);
  }

  void DatabaseIO::closeDatabase__() const
  {
    if (m_cgnsFilePtr > 0) {
      CGCHECKM(cg_close(m_cgnsFilePtr));
      closeDW();
      m_cgnsFilePtr = -1;
    }
  }

  bool DatabaseIO::check_valid_file_open(int status) const
  {
    int global_status = status;
    if (isParallel) {
      global_status = util().global_minmax(status, Ioss::ParallelUtils::DO_MAX);
    }

    if (global_status != CG_OK) {
      Ioss::IntVector err_status;
      if (isParallel) {
        util().all_gather(status, err_status);
      }
      else {
        err_status.push_back(status);
      }

      // See which processors could not open/create the file...
      std::ostringstream errmsg;
      int                ok_count = 0;
      if (isParallel) {
        ok_count = std::count(err_status.begin(), err_status.end(), CG_OK);
        if (ok_count == 0 && util().parallel_size() > 2) {
          fmt::print(errmsg,
                     "ERROR: Unable to open CGNS decomposed database files:\n\t\t{} ...\n\t\t{}\n",
                     Ioss::Utils::decode_filename(get_filename(), 0, util().parallel_size()),
                     Ioss::Utils::decode_filename(get_filename(), util().parallel_size() - 1,
                                                  util().parallel_size()));
        }
        else {
          fmt::print(errmsg, "ERROR: Unable to open CGNS decomposed database files:\n");
          for (int i = 0; i < util().parallel_size(); i++) {
            if (err_status[i] != CG_OK) {
              fmt::print(errmsg, "\t\t{}\n",
                         Ioss::Utils::decode_filename(get_filename(), i, util().parallel_size()));
            }
          }
        }
        fmt::print(errmsg, "       for {} access.\n", (is_input() ? "read" : "write"));
      }
      else {
        fmt::print(errmsg, "ERROR: Unable to open CGNS database '{}' for {} access.\n",
                   get_filename(), (is_input() ? "read" : "write"));
      }
      if (status != CG_OK) {
        if (ok_count != 0 || util().parallel_size() <= 2) {
          fmt::print(errmsg, "[{}] CGNS Error: '{}'\n", myProcessor, cg_get_error());
        }
        else {
          // Since error on all processors, assume the same error on all and only print
          // the error from processor 0.
          if (myProcessor == 0) {
            fmt::print(errmsg, "CGNS Error: '{}'\n", cg_get_error());
          }
        }
      }

      IOSS_ERROR(errmsg);
      return false;
    }
    return true;
  }

  void DatabaseIO::finalize_database() const
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
      Utils::finalize_database(file_ptr, m_timesteps, get_region(), myProcessor, false);
      m_dbFinalized = true;
    }
  }

  int64_t DatabaseIO::node_global_to_local__(int64_t global, bool /*must_exist*/) const
  {
    return global;
  }

  int64_t DatabaseIO::element_global_to_local__(int64_t global) const { return global; }

  void DatabaseIO::create_structured_block_fpp(int base, int num_zones, size_t & /* num_node */)
  {
    SMART_ASSERT(isParallel);
    PAR_UNUSED(base);
    PAR_UNUSED(num_zones);
#if CG_BUILD_PARALLEL
    // Each processor may have a different set of zones.  This routine
    // will sync the information such that at return, each procesosr
    // has a consistent set of structuredBlocks defined with the
    // correct local and global, i,j,k ranges and offsets.
    // First each processor sends their zone count to processor 0...

    // name, proc (int) , cell-range (3 int), boundary-with

    // First, get basenames of all zones on all processors so we can
    // work with consistent set...
    int                        id               = 0;
    int                        in               = 0;
    const int                  INT_PER_ZONE     = 5;  // proc, range[3], zone
    const int                  OUT_INT_PER_ZONE = 10; // proc, range[3], glob_range[3], offset[3]
    std::vector<int>           zone_data(num_zones * INT_PER_ZONE);
    std::vector<char>          zone_names(num_zones * (CGNS_MAX_NAME_LENGTH + 1));
    std::map<std::string, int> zone_id_map;
    std::vector<int>           adjacency;

    for (int zone = 1; zone <= num_zones; zone++) {
      cgsize_t size[9];
      char     zname[CGNS_MAX_NAME_LENGTH + 1];
      CGCHECKM(cg_zone_read(get_file_pointer(), base, zone, zname, size));

      SMART_ASSERT(size[0] - 1 == size[3]);
      SMART_ASSERT(size[1] - 1 == size[4]);
      SMART_ASSERT(size[2] - 1 == size[5]);

      SMART_ASSERT(size[6] == 0);
      SMART_ASSERT(size[7] == 0);
      SMART_ASSERT(size[8] == 0);

      auto        name_proc = Iocgns::Utils::decompose_name(zname, isParallel);
      std::string zone_name = name_proc.first;
      int         proc      = name_proc.second;
      SMART_ASSERT(proc == myProcessor);

      zone_data[id++] = proc;
      pack(id, zone_data, &size[3], 3); // Packing 3,4,5
      Ioss::Utils::copy_string(&zone_names[in], zone_name, CGNS_MAX_NAME_LENGTH + 1);
      in += CGNS_MAX_NAME_LENGTH + 1;
      zone_id_map[zone_name] = zone;

      // Handle zone-grid-connectivity... At this point we only want
      // the zgc that are inter-proc between the same "base zone".
      // That is, the zgc which are result of parallel decomp.

      // Stored as -P, -Z, f1, p1, -P, -Z, f2, p2, ..., -P, -Z, f1, ...
      generate_inter_proc_adjacency(get_file_pointer(), base, zone, myProcessor, zone_name,
                                    adjacency);

      zone_data[id++] = zone;
      SMART_ASSERT(id % INT_PER_ZONE == 0);
    }

    // Now gather all information to processor 0
    std::vector<char> all_names;
    std::vector<int>  all_data;
    std::vector<int>  all_adj;
    util().gather(num_zones, CGNS_MAX_NAME_LENGTH + 1, zone_names, all_names);
    int tot_zones = util().gather(num_zones, INT_PER_ZONE, zone_data, all_data);
    util().gather((int)adjacency.size(), 1, adjacency, all_adj);

    if (myProcessor == 0) {
      std::vector<SBlock> blocks;
      int                 off_name = 0;
      int                 off_data = 0;
      for (int i = 0; i < tot_zones; i++) {
        blocks.emplace_back(&all_names[off_name], &all_data[off_data]);
        off_name += CGNS_MAX_NAME_LENGTH + 1;
        off_data += INT_PER_ZONE;

        // Add inter-processor adjacency information to the block
        auto &b = blocks.back();
        set_adjacency(b, all_adj);
      }
      all_adj.clear();
      all_adj.shrink_to_fit();

      // Sort blocks to get similar zones adjacent -- will have same name, but different proc
      Ioss::sort(blocks.begin(), blocks.end(), [](const SBlock &b1, const SBlock &b2) {
        return (b1.name == b2.name ? b1.proc < b2.proc : b1.name < b2.name);
      });

      int                 proc_count = util().parallel_size();
      std::vector<SBlock> resolved_blocks;

      for (size_t i = 0; i < blocks.size(); i++) {
        auto &b = blocks[i];
        if (b.split()) {
          // The blocks it is split with should be adjacent in list.
          // Get range of indices referring to this block and build
          // a map from processor to index, so build that now...
          std::map<int, int> proc_block_map;
          proc_block_map[b.proc] = i;
          size_t j               = i + 1;
          for (; j < blocks.size(); j++) {
            if (blocks[j].name != b.name) {
              break;
            }
            proc_block_map[blocks[j].proc] = j;
          }
          auto bbeg = i;
          auto bend = j;

          // Get global ijk extent in each direction...
          set_global_extent(bbeg, bend, blocks, proc_block_map);

          // Iterate to get correct offset for these blocks on all processors...
          set_block_offset(bbeg, bend, blocks, proc_block_map);

#if IOSS_DEBUG_OUTPUT
          fmt::print(Ioss::DEBUG(), "Range of blocks for {} is {} to {} Global I,J,K = {} {} {}\n",
                     b.name, i, j - 1, b.glob_range[0], b.glob_range[1], b.glob_range[2]);
#endif
          // All processors need to know about it...
          for (int p = 0; p < proc_count; p++) {
            auto iter = proc_block_map.find(p);
            if (iter == proc_block_map.end()) {
              SBlock newb;
              newb.name       = b.name;
              newb.proc       = p;
              newb.glob_range = b.glob_range;
              resolved_blocks.push_back(newb);
            }
            else {
              auto   idx  = (*iter).second;
              SBlock newb = blocks[idx];
              resolved_blocks.push_back(newb);
            }
          }
          i = bend - 1;
        }
        else {
          // If not split, then global size = local size and offset = 0
          b.glob_range = b.range;

          // All processors need to know about it...
          for (int p = 0; p < proc_count; p++) {
            SBlock newb;
            newb.name       = b.name;
            newb.proc       = p;
            newb.glob_range = b.glob_range;
            if (p == b.proc) {
              newb.range = b.range;
            }
            resolved_blocks.push_back(newb);
          }
        }
      }

      int num_unique = (int)resolved_blocks.size() / proc_count;

#if IOSS_DEBUG_OUTPUT
      for (const auto &b : resolved_blocks) {
        fmt::print(Ioss::DEBUG(), "{} {} {} ({} {} {}) ({} {} {}) ({} {} {}) [{}]\n", b.name,
                   b.proc, b.local_zone, b.range[0], b.range[1], b.range[2], b.glob_range[0],
                   b.glob_range[1], b.glob_range[2], b.offset[0], b.offset[1], b.offset[2],
                   b.face_adj.to_string('.', '+'));
      }
#endif

      // Data now consistent for all zones.  Send back to their "owning" processor
      tot_zones = num_unique;
      all_names.resize(num_unique * (CGNS_MAX_NAME_LENGTH + 1));
      all_data.resize(resolved_blocks.size() * OUT_INT_PER_ZONE);
      id = in = 0;
      for (int off = 0; off < proc_count; off++) {
        for (int b = 0; b < num_unique; b++) {
          int         idx   = off + b * proc_count;
          const auto &block = resolved_blocks[idx];
          if (off == 0) {
            Ioss::Utils::copy_string(&all_names[in], block.name, CGNS_MAX_NAME_LENGTH + 1);
            in += CGNS_MAX_NAME_LENGTH + 1;
          }
          all_data[id++] = block.proc;
          pack(id, all_data, block.range.data(), 3);
          pack(id, all_data, block.glob_range.data(), 3);
          pack(id, all_data, block.offset.data(), 3);
        }
      }
      SMART_ASSERT(id % OUT_INT_PER_ZONE == 0);
    }
    MPI_Bcast(&tot_zones, 1, MPI_INT, 0, util().communicator());
    zone_data.resize(tot_zones * OUT_INT_PER_ZONE);
    all_names.resize(tot_zones * (CGNS_MAX_NAME_LENGTH + 1));
    MPI_Bcast(all_names.data(), tot_zones * (CGNS_MAX_NAME_LENGTH + 1), MPI_CHAR, 0,
              util().communicator());
    MPI_Scatter(all_data.data(), tot_zones * OUT_INT_PER_ZONE, MPI_INT, zone_data.data(),
                tot_zones * OUT_INT_PER_ZONE, MPI_INT, 0, util().communicator());

    // Each processor now has a consistent set of structured blocks.
    // Create the Ioss::StructuredBlocks objects and add to region.
    id = in = 0;
    for (int i = 0; i < tot_zones; i++) {
      std::string zone_name(&all_names[in]);
      in += CGNS_MAX_NAME_LENGTH + 1;
      Ioss::IJK_t local_ijk;
      Ioss::IJK_t global_ijk;
      Ioss::IJK_t offset_ijk;

      zone_data[id++]; // proc field. Not currently used.
      unpack(id, zone_data.data(), local_ijk.data(), 3);
      unpack(id, zone_data.data(), global_ijk.data(), 3);
      unpack(id, zone_data.data(), offset_ijk.data(), 3);

      Ioss::StructuredBlock *block =
          new Ioss::StructuredBlock(this, zone_name, 3, local_ijk, offset_ijk, global_ijk);

      // See if this zone exists on this processor's file, or is just for interprocessor
      // consistency.
      int  zone   = tot_zones + i;
      bool native = false;
      auto iter   = zone_id_map.find(zone_name);
      if (iter != zone_id_map.end()) {
        zone   = (*iter).second;
        native = true;
      }

      block->property_add(Ioss::Property("base", base));
      if (native) {
        block->property_add(Ioss::Property("db_zone", zone));
      }
      block->property_add(Ioss::Property("zone", i + 1));
      block->property_add(Ioss::Property("id", i + 1));
      // Note that 'zone' is not consistent among processors
      block->property_add(Ioss::Property("guid", util().generate_guid(i + 1)));
      get_region()->add(block);
      m_zoneNameMap[zone_name] = i + 1;

      if (native) {
        // Handle zone-grid-connectivity...
        add_zgc_fpp(get_file_pointer(), block, m_zoneNameMap, myProcessor, isParallel);

        // Handle boundary conditions...
        Utils::add_structured_boundary_conditions(get_file_pointer(), block, false);
      }

      // See if this zone/block is a member of any assemblies...
      Utils::add_to_assembly(get_file_pointer(), get_region(), block, base, zone);

      // Need to get a count of number of unique BC's.
      // Note that possible to assign multiple BC to a single face, so can't do this based on faces
      // Assume that if a BC is on multiple processors, then its name will be the same on all
      // processors.
      // * Gather all names to processor 0;
      // * Get unique ordered set
      // * Broadcast back to each processor
      int               in_bc  = 0;
      size_t            num_bc = block->m_boundaryConditions.size();
      std::vector<char> bc_names(num_bc * (CGNS_MAX_NAME_LENGTH + 1));
      for (size_t ibc = 0; ibc < num_bc; ibc++) {
        std::string name = block->m_boundaryConditions[ibc].m_famName + "/" +
                           block->m_boundaryConditions[ibc].m_bcName;
        Ioss::Utils::copy_string(&bc_names[in_bc], name, CGNS_MAX_NAME_LENGTH + 1);
        in_bc += CGNS_MAX_NAME_LENGTH + 1;
      }
      std::vector<char> all_bc_names;
      int tot_names = util().gather(num_bc, CGNS_MAX_NAME_LENGTH + 1, bc_names, all_bc_names);

      if (myProcessor == 0) {
        int                      off_name = 0;
        std::vector<std::string> bc;
        for (int ibc = 0; ibc < tot_names; ibc++) {
          bc.emplace_back(&all_bc_names[off_name]);
          off_name += CGNS_MAX_NAME_LENGTH + 1;
        }
        Ioss::Utils::uniquify(bc);
        tot_names = (int)bc.size();
        all_bc_names.clear();
        all_bc_names.shrink_to_fit();
        bc_names.resize(tot_names * (CGNS_MAX_NAME_LENGTH + 1));
        in_bc = 0;
        for (const auto &name : bc) {
          Ioss::Utils::copy_string(&bc_names[in_bc], name, CGNS_MAX_NAME_LENGTH + 1);
          in_bc += CGNS_MAX_NAME_LENGTH + 1;
        }
      }
      MPI_Bcast(&tot_names, 1, MPI_INT, 0, util().communicator());
      bc_names.resize(tot_names * (CGNS_MAX_NAME_LENGTH + 1));
      MPI_Bcast(bc_names.data(), tot_names * (CGNS_MAX_NAME_LENGTH + 1), MPI_CHAR, 0,
                util().communicator());

      std::vector<std::string> bc;
      int                      off_name = 0;
      for (int ibc = 0; ibc < tot_names; ibc++) {
        bc.emplace_back(&bc_names[off_name]);
        off_name += CGNS_MAX_NAME_LENGTH + 1;
      }
      bc_names.clear();
      bc_names.shrink_to_fit();

      // Each processor now has a unique set of BC names for this block.
      // Now create the missing (empty) BC on each processor.
      for (const auto &bc_name : bc) {
        auto split_name = Ioss::tokenize(bc_name, "/");
        SMART_ASSERT(split_name.size() == 2);
        bool has_name = false;
        for (auto &sbc : block->m_boundaryConditions) {
          if (sbc.m_bcName == split_name[1]) {
            has_name = true;
            break;
          }
        }
        if (!has_name) {
          // Create an empty BC with that name...
          int            face = -1;
          Ioss::SideSet *sset = get_region()->get_sideset(split_name[0]);
          SMART_ASSERT(sset != nullptr);
          add_empty_bc(sset, block, base, zone, face, split_name[0], split_name[1]);
        }
      }

      Ioss::sort(block->m_boundaryConditions.begin(), block->m_boundaryConditions.end(),
                 [](const Ioss::BoundaryCondition &b1, const Ioss::BoundaryCondition &b2) {
                   return (b1.m_bcName < b2.m_bcName);
                 });
    }

    // Need to iterate the blocks again and make the assembly information consistent
    // across processors.
    // If a block belongs to an assembly, it will have the property "assembly"
    // defined on it.
    // This assumes that a block can belong to at most one assembly.
    const auto &assemblies = get_region()->get_assemblies();
    if (!assemblies.empty()) {
      std::map<unsigned int, std::string> assembly_hash_map;
      for (const auto &assem : assemblies) {
        auto hash               = Ioss::Utils::hash(assem->name());
        assembly_hash_map[hash] = assem->name();
      }

      const auto               &blocks = get_region()->get_structured_blocks();
      std::vector<unsigned int> assem_ids;
      assem_ids.reserve(blocks.size());

      for (const auto &sb : blocks) {
        unsigned int hash = 0;
        if (sb->property_exists("assembly")) {
          std::string assembly = sb->get_property("assembly").get_string();
          hash                 = Ioss::Utils::hash(assembly);
        }
        assem_ids.push_back(hash);
      }

      // Hash will be >= 0, so we will take the maximum over all
      // ranks and that will give the assembly (if any) that each block belongs to.
      util().global_array_minmax(assem_ids, Ioss::ParallelUtils::DO_MAX);

      int idx = 0;
      for (const auto &sb : blocks) {
        unsigned    assem_hash = assem_ids[idx++];
        std::string name       = assembly_hash_map[assem_hash];
        auto       *assembly   = get_region()->get_assembly(name);
        assert(assembly != nullptr);
        if (!sb->property_exists("assembly")) {
          assembly->add(sb);
          Ioss::StructuredBlock *new_sb = const_cast<Ioss::StructuredBlock *>(sb);
          new_sb->property_add(Ioss::Property("assembly", assembly->name()));
        }
        SMART_ASSERT(sb->get_property("assembly").get_string() == assembly->name())
        (sb->get_property("assembly").get_string())(assembly->name());
      }
    }
#endif
  }

  void DatabaseIO::create_structured_block(int base, int zone, size_t &num_node)
  {
    SMART_ASSERT(!isParallel);

    cgsize_t size[9];
    char     zone_name[CGNS_MAX_NAME_LENGTH + 1];
    CGCHECKM(cg_zone_read(get_file_pointer(), base, zone, zone_name, size));

    auto        name_proc = Iocgns::Utils::decompose_name(zone_name, isParallel);
    std::string zname     = name_proc.first;
    int         proc      = name_proc.second;
    if (proc != myProcessor) {
      std::ostringstream errmsg;
      fmt::print(errmsg,
                 "ERROR: CGNS: Zone {} has a name that specifies it should be on processor {}, but "
                 "it is actually on processor {}",
                 zone, proc, myProcessor);
      IOSS_ERROR(errmsg);
    }

    m_zoneNameMap[zname] = zone;

    SMART_ASSERT(size[0] - 1 == size[3]);
    SMART_ASSERT(size[1] - 1 == size[4]);
    SMART_ASSERT(size[2] - 1 == size[5]);

    SMART_ASSERT(size[6] == 0);
    SMART_ASSERT(size[7] == 0);
    SMART_ASSERT(size[8] == 0);

    int index_dim = 0;
    CGCHECKM(cg_index_dim(get_file_pointer(), base, zone, &index_dim));
    // An Ioss::StructuredBlock corresponds to a CGNS_ENUMV(Structured) zone...
    auto *block = new Ioss::StructuredBlock(this, zname, index_dim, size[3], size[4], size[5]);

    block->property_add(Ioss::Property("base", base));
    block->property_add(Ioss::Property("db_zone", zone));
    block->property_add(Ioss::Property("zone", zone));
    block->property_add(Ioss::Property("id", zone));
    block->property_add(Ioss::Property("guid", zone));
    get_region()->add(block);

    num_node += block->get_property("node_count").get_int();

    // Handle zone-grid-connectivity...
    int nconn = 0;
    CGCHECKM(cg_n1to1(get_file_pointer(), base, zone, &nconn));
    for (int i = 0; i < nconn; i++) {
      char                    connectname[CGNS_MAX_NAME_LENGTH + 1];
      char                    donorname[CGNS_MAX_NAME_LENGTH + 1];
      std::array<cgsize_t, 6> range;
      std::array<cgsize_t, 6> donor_range;
      Ioss::IJK_t             transform;

      CGCHECKM(cg_1to1_read(get_file_pointer(), base, zone, i + 1, connectname, donorname,
                            range.data(), donor_range.data(), transform.data()));

      auto        donorname_proc = Iocgns::Utils::decompose_name(donorname, isParallel);
      std::string donor_name     = donorname_proc.first;

      // Get number of nodes shared with other "previous" zones...
      // A "previous" zone will have a lower zone number this this zone...
      int  donor_zone = -1;
      auto donor_iter = m_zoneNameMap.find(donor_name);
      if (donor_iter != m_zoneNameMap.end()) {
        donor_zone = (*donor_iter).second;
      }
      Ioss::IJK_t range_beg{{(int)range[0], (int)range[1], (int)range[2]}};
      Ioss::IJK_t range_end{{(int)range[3], (int)range[4], (int)range[5]}};
      Ioss::IJK_t donor_beg{{(int)donor_range[0], (int)donor_range[1], (int)donor_range[2]}};
      Ioss::IJK_t donor_end{{(int)donor_range[3], (int)donor_range[4], (int)donor_range[5]}};

      block->m_zoneConnectivity.emplace_back(connectname, zone, donor_name, donor_zone, transform,
                                             range_beg, range_end, donor_beg, donor_end);

      block->m_zoneConnectivity.back().m_ownerProcessor = myProcessor;
      block->m_zoneConnectivity.back().m_donorProcessor = donorname_proc.second;
    }

    // Handle boundary conditions...
    Utils::add_structured_boundary_conditions(get_file_pointer(), block, false);

    // See if this zone/block is a member of any assemblies...
    Utils::add_to_assembly(get_file_pointer(), get_region(), block, base, zone);
  }

  size_t DatabaseIO::finalize_structured_blocks()
  {
    const auto &blocks = get_region()->get_structured_blocks();

    int              proc_count = util().parallel_size();
    std::vector<int> my_offsets;
    std::vector<int> all_offsets;

    if (proc_count > 1) {
      my_offsets.reserve(blocks.size() * 3 * proc_count);
#ifndef NDEBUG
      int zone = 1;
#endif
      for (const auto &sb : blocks) {
        SMART_ASSERT(sb->get_property("zone").get_int() ==
                     zone++); // Modification of zone OK in SMART_ASSERT
        my_offsets.push_back(sb->get_property("offset_i").get_int());
        my_offsets.push_back(sb->get_property("offset_j").get_int());
        my_offsets.push_back(sb->get_property("offset_k").get_int());
      }
      util().all_gather(my_offsets, all_offsets);
    }

    // If there are any Structured blocks, need to iterate them and their 1-to-1 connections
    // and update the donor_zone id for zones that had not yet been processed at the time of
    // definition...

    // If parallel, then all need to update the donor offset field since that was not known
    // at time of definition...
    for (auto &block : blocks) {
      for (auto &conn : block->m_zoneConnectivity) {
        if (conn.m_donorZone < 0) {
          auto donor_iter = m_zoneNameMap.find(conn.m_donorName);
          if (donor_iter == m_zoneNameMap.end()) {
            if (proc_count == 1) {
              // This is most likely a parallel decomposed model, but only a single
              // part is being accessed.  Do the best we can without being able to
              // access the data on the other processor files...
              auto zname_proc       = Iocgns::Utils::decompose_name(conn.m_donorName, true);
              conn.m_donorProcessor = zname_proc.second;
              auto donor_block      = get_region()->get_structured_block(zname_proc.first);
              if (donor_block != nullptr) {
                conn.m_donorZone = Iocgns::Utils::get_db_zone(donor_block);
              }
              else {
                // Since we are only accessing a single file in a decomposed
                // set of fpp files, we can't access the donor zone on the
                // other processor(s), so we have to set the ZGC to inactive.
                conn.m_isActive = false;
              }
            }
          }
          else {
            SMART_ASSERT(donor_iter != m_zoneNameMap.end());
            conn.m_donorZone = (*donor_iter).second;
          }
        }
        if (proc_count > 1) {
          int         offset = (conn.m_donorProcessor * blocks.size() + (conn.m_donorZone - 1)) * 3;
          Ioss::IJK_t donor_offset{
              {all_offsets[offset + 0], all_offsets[offset + 1], all_offsets[offset + 2]}};

          conn.m_donorOffset = donor_offset;
          conn.m_donorRangeBeg[0] += donor_offset[0];
          conn.m_donorRangeBeg[1] += donor_offset[1];
          conn.m_donorRangeBeg[2] += donor_offset[2];
          conn.m_donorRangeEnd[0] += donor_offset[0];
          conn.m_donorRangeEnd[1] += donor_offset[1];
          conn.m_donorRangeEnd[2] += donor_offset[2];
        }
        conn.m_donorGUID = util().generate_guid(conn.m_donorZone, conn.m_donorProcessor);
        conn.m_ownerGUID = util().generate_guid(conn.m_ownerZone, conn.m_ownerProcessor);
      }
    }

    size_t num_nodes = Utils::resolve_nodes(*get_region(), myProcessor, isParallel);
    return num_nodes;
  }

  void DatabaseIO::create_unstructured_block(int base, int zone, size_t &num_node)
  {
    cgsize_t size[9];
    char     zone_name[CGNS_MAX_NAME_LENGTH + 1];
    CGCHECKM(cg_zone_read(get_file_pointer(), base, zone, zone_name, size));
    m_zoneNameMap[zone_name] = zone;

    size_t total_block_nodes = size[0];
    m_blockLocalNodeMap[zone].resize(total_block_nodes, -1);

    // Determine number of "shared" nodes (shared with other zones)
    if (zone > 1) { // Donor zone is always lower numbered, so zone 1 has no donor zone.
      int nconn = 0;
      CGCHECKM(cg_nconns(get_file_pointer(), base, zone, &nconn));
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

        CGCHECKM(cg_conn_info(get_file_pointer(), base, zone, i + 1, connectname, &location,
                              &connect_type, &ptset_type, &npnts, donorname, &donor_zonetype,
                              &donor_ptset_type, &donor_datatype, &ndata_donor));

        if (connect_type != CGNS_ENUMV(Abutting1to1) || ptset_type != CGNS_ENUMV(PointList) ||
            donor_ptset_type != CGNS_ENUMV(PointListDonor)) {
          std::ostringstream errmsg;
          fmt::print(errmsg,
                     "ERROR: CGNS: Zone {} adjacency data is not correct type. Require "
                     "Abutting1to1 and PointList."
                     " {}\t{}\t{}",
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
        auto donor_iter = m_zoneNameMap.find(donorname);
        if (donor_iter != m_zoneNameMap.end() && (*donor_iter).second < zone) {
#if IOSS_DEBUG_OUTPUT
          fmt::print("Zone {} shares {} nodes with {}\n", zone, npnts, donorname);
#endif
          CGNSIntVector points(npnts);
          CGNSIntVector donors(npnts);

          CGCHECKM(cg_conn_read(get_file_pointer(), base, zone, i + 1, points.data(),
                                donor_datatype, donors.data()));

          // Fill in entries in m_blockLocalNodeMap for the shared nodes...
          auto &donor_map = m_blockLocalNodeMap[(*donor_iter).second];
          auto &block_map = m_blockLocalNodeMap[zone];
          for (int j = 0; j < npnts; j++) {
            cgsize_t point       = points[j];
            cgsize_t donor       = donors[j];
            block_map[point - 1] = donor_map[donor - 1];
          }
        }
      }
    }

    auto  &block_map = m_blockLocalNodeMap[zone];
    size_t offset    = num_node;
    for (size_t i = 0; i < total_block_nodes; i++) {
      if (block_map[i] == -1) {
        block_map[i] = offset++;
      }
    }
    num_node = offset;

    size_t num_elem    = size[1];
    m_zoneOffset[zone] = m_zoneOffset[zone - 1] + num_elem;

    // NOTE: A Zone will have a single set of nodes, but can have
    //       multiple sections each with their own element type...
    //       Keep treating sections as element blocks until we
    //       have handled 'size[1]' number of elements; the remaining
    //       sections are then the boundary faces (?)
    int num_sections = 0;
    CGCHECKM(cg_nsections(get_file_pointer(), base, zone, &num_sections));

    // ========================================================================
    // Read the ZoneBC_t node to get list of SideBlocks to define on this zone
    // The BC_t nodes in the ZoneBC_t give the element range for each SideBlock
    // which can be matched up below with the Elements_t nodes to get contents
    // of the SideBlocks.
    auto zonebc = Utils::parse_zonebc_sideblocks(get_file_pointer(), base, zone, myProcessor);

    // ========================================================================
    // Read the sections and create an element block for the ones that
    // define elements.  Some define boundary conditions...
    Ioss::ElementBlock *eblock = nullptr;

    for (int is = 1; is <= num_sections; is++) {
      char section_name[CGNS_MAX_NAME_LENGTH + 1];
      CGNS_ENUMT(ElementType_t) e_type;
      cgsize_t el_start    = 0;
      cgsize_t el_end      = 0;
      int      num_bndry   = 0;
      int      parent_flag = 0;

      // Get the type of elements in this section...
      CGCHECKM(cg_section_read(get_file_pointer(), base, zone, is, section_name, &e_type, &el_start,
                               &el_end, &num_bndry, &parent_flag));

      cgsize_t num_entity = el_end - el_start + 1;

      if (parent_flag == 0 && num_elem > 0) {
        num_elem -= num_entity;
        std::string element_topo = Utils::map_cgns_to_topology_type(e_type);
#if IOSS_DEBUG_OUTPUT
        fmt::print("Added block {}: CGNS topology = '{}', IOSS topology = '{}' with {} elements\n",
                   zone_name, cg_ElementTypeName(e_type), element_topo, num_entity);
#endif
        eblock = new Ioss::ElementBlock(this, zone_name, element_topo, num_entity);
        eblock->property_add(Ioss::Property("base", base));
        eblock->property_add(Ioss::Property("zone", zone));
        eblock->property_add(Ioss::Property("db_zone", zone));
        eblock->property_add(Ioss::Property("id", zone));
        eblock->property_add(Ioss::Property("guid", zone));
        eblock->property_add(Ioss::Property("section", is));
        eblock->property_add(Ioss::Property("node_count", (int64_t)total_block_nodes));
        eblock->property_add(Ioss::Property("original_block_order", zone));

        // See if this zone/block is a member of any assemblies...
        Utils::add_to_assembly(get_file_pointer(), get_region(), eblock, base, zone);

        SMART_ASSERT(is == 1); // For now, assume each zone has only a single element block.
        bool added = get_region()->add(eblock);
        if (!added) {
          delete eblock;
          eblock = nullptr;
        }
      }
      else {
        // This is a boundary-condition -- sideset (?)
        // Search zonebc (if it exists) for an entry such that the element ranges overlap.
        Ioss::SideSet *sset = nullptr;

        if (!zonebc.empty()) {
          size_t i = 0;
          for (; i < zonebc.size(); i++) {
            if (zonebc[i].range_beg >= el_start && zonebc[i].range_end <= el_end) {
              break;
            }
          }
          if (i < zonebc.size()) {
            // See if there is an existing sideset with this name...
            sset = get_region()->get_sideset(zonebc[i].name);
          }
        }
        else {
          sset = get_region()->get_sideset(section_name);
        }

        if (sset != nullptr) {
          std::string block_name = fmt::format("{}/{}", zone_name, section_name);
          std::string face_topo  = Utils::map_cgns_to_topology_type(e_type);
#if IOSS_DEBUG_OUTPUT
          fmt::print("Added sideblock {} of topo '{}' with {} faces\n", block_name, face_topo,
                     num_entity);
#endif
          std::string parent_topo = eblock == nullptr ? "unknown" : eblock->topology()->name();
          auto sblk = new Ioss::SideBlock(this, block_name, face_topo, parent_topo, num_entity);
          // IF name is of form "surface_" + "#", then extract # and use as id...
          int id = Ioss::Utils::extract_id(block_name);
          if (id != 0) {
            sblk->property_add(Ioss::Property("id", id));
            sblk->property_add(Ioss::Property("guid", id));
          }
          else {
            sblk->property_add(Ioss::Property("id", zone));
            sblk->property_add(Ioss::Property("guid", zone));
          }
          sblk->property_add(Ioss::Property("base", base));
          sblk->property_add(Ioss::Property("zone", zone));
          sblk->property_add(Ioss::Property("section", is));
          if (eblock != nullptr) {
            sblk->set_parent_element_block(eblock);
          }
          sset->add(sblk);
        }
      }
    }
  }

  void DatabaseIO::read_meta_data__()
  {
    // Determine the number of bases in the grid.
    // Currently only handle 1.
    int n_bases = 0;
    CGCHECKM(cg_nbases(get_file_pointer(), &n_bases));
    if (n_bases != 1) {
      std::ostringstream errmsg;
      fmt::print(
          errmsg,
          "ERROR: CGNS: Too many bases; only support files with a single bases at this time");
      IOSS_ERROR(errmsg);
    }

    get_step_times__();

    if (open_create_behavior() == Ioss::DB_APPEND) {
      return;
    }

    // ========================================================================
    // Get the number of sidesets in the mesh...
    // Will be the 'families' that are of the type "FamilyBC_t"
    Utils::add_sidesets(get_file_pointer(), this);

    // ========================================================================
    // Get the number of assemblies in the mesh...
    // Will be the 'families' that contain nodes of 'FamVC_*'
    Utils::add_assemblies(get_file_pointer(), this);

    // ========================================================================
    // Get the number of zones (element blocks) in the mesh...
    int num_zones = 0;
    int base      = 1;
    CGCHECKM(cg_nzones(get_file_pointer(), base, &num_zones));
    m_blockLocalNodeMap.resize(num_zones + 1); // Let's use 1-based zones...
    m_zoneOffset.resize(num_zones + 1);        // Let's use 1-based zones...

    // ========================================================================
    size_t num_node  = 0;
    auto   mesh_type = Utils::check_mesh_type(get_file_pointer());

    if (isParallel && mesh_type == Ioss::MeshType::STRUCTURED) {
      // Handle the file-per-processor parallel case separately for
      // now. Hopefully can consolidate at some later time.
      create_structured_block_fpp(base, num_zones, num_node);
    }
    else {
      for (int zone = 1; zone <= num_zones; zone++) {
        if (mesh_type == Ioss::MeshType::STRUCTURED) {
          create_structured_block(base, zone, num_node);
        }
        else if (mesh_type == Ioss::MeshType::UNSTRUCTURED) {
          create_unstructured_block(base, zone, num_node);
        }
#if IOSS_ENABLE_HYBRID
        else if (mesh_type == Ioss::MeshType::HYBRID) {
        }
#endif
        else {
          std::ostringstream errmsg;
          fmt::print(errmsg,
                     "ERROR: CGNS: Zone {} is not of type Unstructured or Structured "
                     "which are the only types currently supported",
                     zone);
          IOSS_ERROR(errmsg);
        }
      }
    }

    if (mesh_type == Ioss::MeshType::STRUCTURED || mesh_type == Ioss::MeshType::HYBRID) {
      num_node = finalize_structured_blocks();
    }

    char basename[CGNS_MAX_NAME_LENGTH + 1];
    int  cell_dimension = 0;
    int  phys_dimension = 0;
    CGCHECKM(cg_base_read(get_file_pointer(), base, basename, &cell_dimension, &phys_dimension));
    if (phys_dimension != 3) {
      std::ostringstream errmsg;
      fmt::print(errmsg, "ERROR: The model is {}D.  Only 3D models are supported.", phys_dimension);
      IOSS_ERROR(errmsg);
    }

    auto *nblock = new Ioss::NodeBlock(this, "nodeblock_1", num_node, phys_dimension);
    nblock->property_add(Ioss::Property("base", base));
    get_region()->add(nblock);
    nodeCount = num_node;

    Utils::add_transient_variables(get_file_pointer(), m_timesteps, get_region(), myProcessor,
                                   false);
  }

  void DatabaseIO::write_meta_data()
  {
    int num_zones = get_region()->get_property("element_block_count").get_int() +
                    get_region()->get_property("structured_block_count").get_int();
    m_bcOffset.resize(num_zones + 1);   // use 1-based zones...
    m_zoneOffset.resize(num_zones + 1); // use 1-based zones...

    elementCount =
        Utils::common_write_meta_data(get_file_pointer(), *get_region(), m_zoneOffset, false);
  }

  void DatabaseIO::get_step_times__()
  {
    Utils::get_step_times(get_file_pointer(), m_timesteps, get_region(), timeScaleFactor,
                          myProcessor);
  }

  void DatabaseIO::write_adjacency_data()
  {
    // Determine adjacency information between unstructured blocks.
    // Could save this information from the input mesh, but then
    // could not read an exodus mesh and write a cgns mesh.
    // However, in long run may still want to read/save input adjacency
    // data if doing cgns -> cgns...  For now, try generating information.

    // If block I is adjacent to block J, then they will share at
    // least 1 "side" (face 3D or edge 2D).
    // Currently, assuming they are adjacent if they share at least one node...
    const auto &blocks = get_region()->get_element_blocks();
    for (auto I = blocks.cbegin(); I != blocks.cend(); I++) {
      int base = (*I)->get_property("base").get_int();
      int zone = Iocgns::Utils::get_db_zone(*I);

      const auto &I_map = m_globalToBlockLocalNodeMap[zone];

      for (auto J = I + 1; J != blocks.end(); J++) {
        int           dzone = (*J)->get_property("zone").get_int();
        const auto   &J_map = m_globalToBlockLocalNodeMap[dzone];
        CGNSIntVector point_list;
        CGNSIntVector point_list_donor;
        for (size_t i = 0; i < J_map->size(); i++) {
          auto global = J_map->map()[i + 1];
          // See if this global id exists in I_map...
          auto i_zone_local = I_map->global_to_local(global, false);
          if (i_zone_local > 0) {
            // Have a match between nodes used by two different blocks,
            // They are adjacent...
            point_list.push_back(i_zone_local);
            point_list_donor.push_back(i + 1);
          }
        }

        // If point_list non_empty, then output this adjacency node...
        if (!point_list.empty()) {
          int         gc_idx  = 0;
          std::string name    = fmt::format("{}_to_{}", (*I)->name(), (*J)->name());
          const auto &d1_name = (*J)->name();
          CGCHECKM(cg_conn_write(get_file_pointer(), base, zone, name.c_str(), CGNS_ENUMV(Vertex),
                                 CGNS_ENUMV(Abutting1to1), CGNS_ENUMV(PointList), point_list.size(),
                                 point_list.data(), d1_name.c_str(), CGNS_ENUMV(Unstructured),
                                 CGNS_ENUMV(PointListDonor), CGNS_ENUMV(DataTypeNull),
                                 point_list_donor.size(), point_list_donor.data(), &gc_idx));

          name                = fmt::format("{}_to_{}", (*J)->name(), (*I)->name());
          const auto &d2_name = (*I)->name();

          CGCHECKM(cg_conn_write(get_file_pointer(), base, dzone, name.c_str(), CGNS_ENUMV(Vertex),
                                 CGNS_ENUMV(Abutting1to1), CGNS_ENUMV(PointList),
                                 point_list_donor.size(), point_list_donor.data(), d2_name.c_str(),
                                 CGNS_ENUMV(Unstructured), CGNS_ENUMV(PointListDonor),
                                 CGNS_ENUMV(DataTypeNull), point_list.size(), point_list.data(),
                                 &gc_idx));
        }
      }
    }
  }

  bool DatabaseIO::begin__(Ioss::State state)
  {
    dbState = state;
    return true;
  }

  void DatabaseIO::free_state_pointer()
  {
    // If this is the first state file created, then we need to save a reference
    // to the base CGNS file so we can update the metadata and create links to
    // the state files.
    if (m_cgnsBasePtr < 0) {
      m_cgnsBasePtr = m_cgnsFilePtr;
      m_cgnsFilePtr = -1;
    }
    closeDatabase__();
  }

  void DatabaseIO::open_state_file(int state)
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

    Iocgns::Utils::write_state_meta_data(get_file_pointer(), *get_region(), false);
  }

  bool DatabaseIO::end__(Ioss::State state)
  {
    // Transitioning out of state 'state'
    switch (state) {
    case Ioss::STATE_DEFINE_MODEL:
      if (!is_input() && open_create_behavior() != Ioss::DB_APPEND &&
          open_create_behavior() != Ioss::DB_MODIFY) {
        write_meta_data();
      }
      if (!is_input() && (open_create_behavior() == Ioss::DB_APPEND ||
                          open_create_behavior() == Ioss::DB_MODIFY)) {
        Utils::update_db_zone_property(m_cgnsFilePtr, get_region(), myProcessor, isParallel, false);
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

    dbState = Ioss::STATE_UNKNOWN;
    return true;
  }

  bool DatabaseIO::begin_state__(int state, double /* time */)
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
                                        &m_currentCellCenterSolutionIndex, false);

    return true;
  }

  bool DatabaseIO::end_state__(int state, double time)
  {
    if (!is_input()) {
      m_timesteps.push_back(time);
      SMART_ASSERT(m_timesteps.size() == (size_t)state);
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

  void DatabaseIO::flush_database__() const
  {
    // For HDF5 files, it looks like we need to close the database between
    // writes if we want to have a valid database for external access or
    // to protect against a crash corrupting the file.
    Utils::finalize_database(get_file_pointer(), m_timesteps, get_region(), myProcessor, false);
    closeDatabase__();
    m_cgnsFilePtr = -2; // Tell openDatabase__ that we want to append
  }

  int64_t DatabaseIO::get_field_internal(const Ioss::Region *reg, const Ioss::Field &field,
                                         void * /* data */, size_t /* data_size */) const
  {
    return Ioss::Utils::field_warning(reg, field, "input");
  }

  int64_t DatabaseIO::get_field_internal(const Ioss::NodeBlock *nb, const Ioss::Field &field,
                                         void *data, size_t data_size) const
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

    Ioss::Field::RoleType role       = field.get_role();
    int                   base       = nb->get_property("base").get_int();
    size_t                num_to_get = field.verify(data_size);
    cgsize_t              first      = 1;

    // Create a lambda to eliminate lots of duplicate code in coordinate outputs...
    auto coord_lambda = [this, &data, &first, base](const char *ordinate) {
      auto *rdata = static_cast<double *>(data);

      for (int zone = 1; zone < static_cast<int>(m_blockLocalNodeMap.size()); zone++) {
        auto               &block_map = m_blockLocalNodeMap[zone];
        cgsize_t            num_coord = block_map.size();
        std::vector<double> coord(num_coord);
        CGCHECKM(cg_coord_read(get_file_pointer(), base, zone, ordinate, CGNS_ENUMV(RealDouble),
                               &first, &num_coord, coord.data()));

        // Map to global coordinate position...
        for (cgsize_t i = 0; i < num_coord; i++) {
          rdata[block_map[i]] = coord[i];
        }
      }
    };
    // End of lambda...

    if (role == Ioss::Field::MESH) {
      if (field.get_name() == "mesh_model_coordinates_x") {
        // Use the lambda...
        coord_lambda("CoordinateX");
      }

      else if (field.get_name() == "mesh_model_coordinates_y") {
        coord_lambda("CoordinateY");
      }

      else if (field.get_name() == "mesh_model_coordinates_z") {
        coord_lambda("CoordinateZ");
      }

      else if (field.get_name() == "mesh_model_coordinates") {
        int  cell_dimension = 0;
        int  phys_dimension = 0;
        char basename[CGNS_MAX_NAME_LENGTH + 1];
        CGCHECKM(
            cg_base_read(get_file_pointer(), base, basename, &cell_dimension, &phys_dimension));

        auto *rdata = static_cast<double *>(data);

        // Data required by upper classes store x0, y0, z0, ... xn,
        // yn, zn. Data stored in exodusII file is x0, ..., xn, y0,
        // ..., yn, z0, ..., zn so we have to allocate some scratch
        // memory to read in the data and then map into supplied
        // 'data'
        for (int zone = 1; zone < static_cast<int>(m_blockLocalNodeMap.size()); zone++) {
          auto               &block_map = m_blockLocalNodeMap[zone];
          cgsize_t            num_coord = block_map.size();
          std::vector<double> coord(num_coord);

          // ========================================================================
          // Repetitive code for each coordinate direction; use a lambda to consolidate...
          auto blk_coord_lambda = [this, block_map, base, zone, &coord, first, num_coord,
                                   phys_dimension, &rdata](const char *ord_name, int ordinate) {
            CGCHECKM(cg_coord_read(get_file_pointer(), base, zone, ord_name, CGNS_ENUMV(RealDouble),
                                   &first, &num_coord, coord.data()));

            // Map to global coordinate position...
            for (cgsize_t i = 0; i < num_coord; i++) {
              rdata[phys_dimension * block_map[i] + ordinate] = coord[i];
            }
          };
          // End of lambda...
          // ========================================================================

          blk_coord_lambda("CoordinateX", 0);

          if (phys_dimension >= 2) {
            blk_coord_lambda("CoordinateY", 1);
          }

          if (phys_dimension >= 3) {
            blk_coord_lambda("CoordinateZ", 2);
          }
        }
      }
      else if (field.get_name() == "ids") {
        // Map the local ids in this node block
        // (1...node_count) to global node ids.
        if (field.get_type() == Ioss::Field::INT64) {
          auto *idata = static_cast<int64_t *>(data);
          std::iota(idata, idata + num_to_get, 1);
        }
        else {
          SMART_ASSERT(field.get_type() == Ioss::Field::INT32);
          int *idata = static_cast<int *>(data);
          std::iota(idata, idata + num_to_get, 1);
        }
      }
      else {
        num_to_get = Ioss::Utils::field_warning(nb, field, "input");
      }
    }
    else if (role == Ioss::Field::TRANSIENT) {
      // Locate the FlowSolution node corresponding to the correct state/step/time
      // TODO: do this at read_meta_data() and store...
      int step = get_region()->get_current_state();

      for (int zone = 1; zone < static_cast<int>(m_blockLocalNodeMap.size()); zone++) {
        int solution_index =
            Utils::find_solution_index(get_file_pointer(), base, zone, step, CGNS_ENUMV(Vertex));
        auto    &block_map      = m_blockLocalNodeMap[zone];
        cgsize_t num_block_node = block_map.size();

        auto               *rdata        = static_cast<double *>(data);
        cgsize_t            range_min[1] = {1};
        cgsize_t            range_max[1] = {num_block_node};
        int                 comp_count   = field.get_component_count(Ioss::Field::InOut::INPUT);
        std::vector<double> cgns_data(num_block_node);
        if (comp_count == 1) {
          CGCHECKM(cg_field_read(get_file_pointer(), base, zone, solution_index,
                                 field.get_name().c_str(), CGNS_ENUMV(RealDouble), range_min,
                                 range_max, cgns_data.data()));

          // Map to global nodal field position...
          for (cgsize_t i = 0; i < num_block_node; i++) {
            rdata[block_map[i]] = cgns_data[i];
          }
        }
        else {
          for (int i = 0; i < comp_count; i++) {
            std::string var_name = get_component_name(field, Ioss::Field::InOut::INPUT, i + 1);

            CGCHECKM(cg_field_read(get_file_pointer(), base, zone, solution_index, var_name.c_str(),
                                   CGNS_ENUMV(RealDouble), range_min, range_max, cgns_data.data()));
            for (cgsize_t j = 0; j < num_block_node; j++) {
              auto global                    = block_map[j];
              rdata[comp_count * global + i] = cgns_data[j];
            }
          }
        }
      }
    }
    else {
      num_to_get = Ioss::Utils::field_warning(nb, field, "input");
    }
    return num_to_get;
  }

  int64_t DatabaseIO::get_field_internal_sub_nb(const Ioss::NodeBlock *nb, const Ioss::Field &field,
                                                void *data, size_t data_size) const
  {
    // Reads field data on a NodeBlock which is a "sub" NodeBlock -- contains the nodes for a
    // StructuredBlock instead of for the entire model.
    // Currently only TRANSIENT fields are input this way.  No valid reason, but that is the current
    // use case.

    // In this routine, if isParallel, then reading
    // file-per-processor; not parallel io from single file.
    cgsize_t num_to_get = field.verify(data_size);
    if (isParallel && num_to_get == 0) {
      return 0;
    }

    Ioss::Field::RoleType role = field.get_role();
    if (role == Ioss::Field::TRANSIENT) {
      // Get the StructuredBlock that this NodeBlock is contained in:

      // Locate the FlowSolution node corresponding to the correct state/step/time
      // TODO: do this at read_meta_data() and store...
      int                         step = get_region()->get_current_state();
      int                         base = 1;
      const Ioss::GroupingEntity *sb   = nb->contained_in();
      int                         zone = Iocgns::Utils::get_db_zone(sb);
      int                         solution_index =
          Utils::find_solution_index(get_file_pointer(), base, zone, step, CGNS_ENUMV(Vertex));

      auto *rdata = static_cast<double *>(data);
      SMART_ASSERT(num_to_get == sb->get_property("node_count").get_int());
      cgsize_t rmin[3] = {0, 0, 0};
      cgsize_t rmax[3] = {0, 0, 0};
      if (num_to_get > 0) {
        rmin[0] = 1;
        rmin[1] = 1;
        rmin[2] = 1;

        rmax[0] = rmin[0] + sb->get_property("ni").get_int();
        rmax[1] = rmin[1] + sb->get_property("nj").get_int();
        rmax[2] = rmin[2] + sb->get_property("nk").get_int();

        SMART_ASSERT(num_to_get ==
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

  int64_t DatabaseIO::get_field_internal(const Ioss::EdgeBlock *eb, const Ioss::Field &field,
                                         void * /* data */, size_t /* data_size */) const
  {
    return Ioss::Utils::field_warning(eb, field, "input");
  }

  int64_t DatabaseIO::get_field_internal(const Ioss::FaceBlock *fb, const Ioss::Field &field,
                                         void * /* data */, size_t /* data_size */) const
  {
    return Ioss::Utils::field_warning(fb, field, "input");
  }

  int64_t DatabaseIO::get_field_internal(const Ioss::ElementBlock *eb, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    size_t num_to_get = field.verify(data_size);
    if (num_to_get > 0) {

      int                   base             = eb->get_property("base").get_int();
      int                   zone             = Iocgns::Utils::get_db_zone(eb);
      int                   sect             = eb->get_property("section").get_int();
      cgsize_t              my_element_count = eb->entity_count();
      Ioss::Field::RoleType role             = field.get_role();

      if (role == Ioss::Field::MESH) {
        // Handle the MESH fields required for a CGNS file model.
        // (The 'genesis' portion)

        if (field.get_name() == "connectivity" || field.get_name() == "connectivity_raw") {
          // TODO(gdsjaar): Need to map local to global...
          int element_nodes = eb->topology()->number_nodes();
          SMART_ASSERT(field.raw_storage()->component_count() == element_nodes);

          if (my_element_count > 0) {
            int field_byte_size = (field.get_type() == Ioss::Field::INT32) ? 32 : 64;
            if (field_byte_size == CG_SIZEOF_SIZE) {
              auto *idata = reinterpret_cast<cgsize_t *>(data);
              CGCHECKM(cg_elements_read(get_file_pointer(), base, zone, sect, idata, nullptr));
              Utils::map_cgns_connectivity(eb->topology(), num_to_get, idata);
            }
            else {
              CGNSIntVector connect(element_nodes * num_to_get);
              CGCHECKM(
                  cg_elements_read(get_file_pointer(), base, zone, sect, connect.data(), nullptr));
              if (field.get_type() == Ioss::Field::INT32) {
                auto  *idata = reinterpret_cast<int *>(data);
                size_t i     = 0;
                for (auto node : connect) {
                  idata[i++] = node;
                }
                Utils::map_cgns_connectivity(eb->topology(), num_to_get, idata);
              }
              else {
                auto  *idata = reinterpret_cast<int64_t *>(data);
                size_t i     = 0;
                for (auto node : connect) {
                  idata[i++] = node;
                }
                Utils::map_cgns_connectivity(eb->topology(), num_to_get, idata);
              }
            }
          }

          // Now need to map block-local node connectivity to global nodes...
          // This is done for both connectivity and connectivity_raw
          // since the "global id" is the same as the "local id"
          // The connectivities we currently have are "block local"
          const auto &block_map = m_blockLocalNodeMap[zone];
          if (field.get_type() == Ioss::Field::INT32) {
            int *idata = static_cast<int *>(data);
            for (size_t i = 0; i < element_nodes * num_to_get; i++) {
              idata[i] = block_map[idata[i] - 1] + 1;
            }
          }
          else {
            auto *idata = static_cast<int64_t *>(data);
            for (size_t i = 0; i < element_nodes * num_to_get; i++) {
              idata[i] = block_map[idata[i] - 1] + 1;
            }
          }
        }
        else if (field.get_name() == "ids") {
          // Map the local ids in this element block
          // (eb_offset+1...eb_offset+1+my_element_count) to global element ids.
          size_t eb_offset_plus_one = eb->get_offset() + 1;
          if (field.get_type() == Ioss::Field::INT64) {
            auto *idata = static_cast<int64_t *>(data);
            std::iota(idata, idata + my_element_count, static_cast<int64_t>(eb_offset_plus_one));
          }
          else {
            SMART_ASSERT(field.get_type() == Ioss::Field::INT32);
            int *idata = static_cast<int *>(data);
            std::iota(idata, idata + my_element_count, static_cast<int>(eb_offset_plus_one));
          }
        }
        else if (field.get_name() == "implicit_ids") {
          size_t eb_offset_plus_one = eb->get_offset() + 1;
          if (field.get_type() == Ioss::Field::INT64) {
            auto *idata = static_cast<int64_t *>(data);
            std::iota(idata, idata + my_element_count, static_cast<int64_t>(eb_offset_plus_one));
          }
          else {
            SMART_ASSERT(field.get_type() == Ioss::Field::INT32);
            int *idata = static_cast<int *>(data);
            std::iota(idata, idata + my_element_count, static_cast<int>(eb_offset_plus_one));
          }
        }
        else {
          num_to_get = Ioss::Utils::field_warning(eb, field, "input");
        }
      }
      else if (role == Ioss::Field::TRANSIENT) {
        // Locate the FlowSolution node corresponding to the correct state/step/time
        // TODO: do this at read_meta_data() and store...
        int step           = get_region()->get_current_state();
        int solution_index = Utils::find_solution_index(get_file_pointer(), base, zone, step,
                                                        CGNS_ENUMV(CellCenter));

        auto    *rdata        = static_cast<double *>(data);
        cgsize_t range_min[1] = {1};
        cgsize_t range_max[1] = {my_element_count};

        int comp_count = field.get_component_count(Ioss::Field::InOut::INPUT);
        if (comp_count == 1) {
          CGCHECKM(cg_field_read(get_file_pointer(), base, zone, solution_index,
                                 field.get_name().c_str(), CGNS_ENUMV(RealDouble), range_min,
                                 range_max, rdata));
        }
        else {
          std::vector<double> cgns_data(my_element_count);
          for (int i = 0; i < comp_count; i++) {
            std::string var_name = get_component_name(field, Ioss::Field::InOut::INPUT, i + 1);

            CGCHECKM(cg_field_read(get_file_pointer(), base, zone, solution_index, var_name.c_str(),
                                   CGNS_ENUMV(RealDouble), range_min, range_max, cgns_data.data()));
            for (cgsize_t j = 0; j < my_element_count; j++) {
              rdata[comp_count * j + i] = cgns_data[j];
            }
          }
        }
      }
      else {
        num_to_get = Ioss::Utils::field_warning(eb, field, "output");
      }
    }
    return num_to_get;
  }

  int64_t DatabaseIO::get_field_internal(const Ioss::StructuredBlock *sb, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    Ioss::Field::RoleType role = field.get_role();
    int                   base = sb->get_property("base").get_int();
    int                   zone = Iocgns::Utils::get_db_zone(sb);

    cgsize_t num_to_get = field.verify(data_size);

    // In this routine, if isParallel, then reading file-per-processor; not parallel io from single
    // file.
    if (isParallel && num_to_get == 0) {
      return 0;
    }

    cgsize_t rmin[3] = {0, 0, 0};
    cgsize_t rmax[3] = {0, 0, 0};

    bool cell_field = Utils::is_cell_field(field);
    if ((cell_field && sb->get_property("cell_count").get_int() == 0) ||
        (!cell_field && sb->get_property("node_count").get_int() == 0)) {
      return 0;
    }

    if (cell_field) {
      SMART_ASSERT(num_to_get == sb->get_property("cell_count").get_int());
      if (num_to_get > 0) {
        rmin[0] = 1;
        rmin[1] = 1;
        rmin[2] = 1;

        rmax[0] = rmin[0] + sb->get_property("ni").get_int() - 1;
        rmax[1] = rmin[1] + sb->get_property("nj").get_int() - 1;
        rmax[2] = rmin[2] + sb->get_property("nk").get_int() - 1;
      }
    }
    else {
      // cell nodal field.
      SMART_ASSERT(num_to_get == sb->get_property("node_count").get_int());
      if (num_to_get > 0) {
        rmin[0] = 1;
        rmin[1] = 1;
        rmin[2] = 1;

        rmax[0] = rmin[0] + sb->get_property("ni").get_int();
        rmax[1] = rmin[1] + sb->get_property("nj").get_int();
        rmax[2] = rmin[2] + sb->get_property("nk").get_int();
      }
    }

    SMART_ASSERT(num_to_get ==
                 (rmax[0] - rmin[0] + 1) * (rmax[1] - rmin[1] + 1) * (rmax[2] - rmin[2] + 1));

    auto *rdata = static_cast<double *>(data);

    if (role == Ioss::Field::MESH) {
      if (field.get_name() == "mesh_model_coordinates_x") {
        CGCHECKM(cg_coord_read(get_file_pointer(), base, zone, "CoordinateX",
                               CGNS_ENUMV(RealDouble), rmin, rmax, rdata));
      }

      else if (field.get_name() == "mesh_model_coordinates_y") {
        CGCHECKM(cg_coord_read(get_file_pointer(), base, zone, "CoordinateY",
                               CGNS_ENUMV(RealDouble), rmin, rmax, rdata));
      }

      else if (field.get_name() == "mesh_model_coordinates_z") {
        CGCHECKM(cg_coord_read(get_file_pointer(), base, zone, "CoordinateZ",
                               CGNS_ENUMV(RealDouble), rmin, rmax, rdata));
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

        // ========================================================================
        // Repetitive code for each coordinate direction; use a lambda to consolidate...
        auto coord_lambda = [this, base, zone, &coord, &rmin, &rmax, phys_dimension, num_to_get,
                             &rdata](const char *ord_name, int ordinate) {
          CGCHECKM(cg_coord_read(get_file_pointer(), base, zone, ord_name, CGNS_ENUMV(RealDouble),
                                 rmin, rmax, coord.data()));

          // Map to global coordinate position...
          for (cgsize_t i = 0; i < num_to_get; i++) {
            rdata[phys_dimension * i + ordinate] = coord[i];
          }
        };
        // End of lambda...
        // ========================================================================

        coord_lambda("CoordinateX", 0);

        if (phys_dimension >= 2) {
          coord_lambda("CoordinateY", 1);
        }

        if (phys_dimension == 3) {
          coord_lambda("CoordinateZ", 2);
        }
      }
      else if (field.get_name() == "cell_node_ids") {
        if (field.get_type() == Ioss::Field::INT64) {
          auto *idata = static_cast<int64_t *>(data);
          sb->get_cell_node_ids(idata, true);
        }
        else {
          SMART_ASSERT(field.get_type() == Ioss::Field::INT32);
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
          SMART_ASSERT(field.get_type() == Ioss::Field::INT32);
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

      if (comp_count == 1) {
        CGCHECKM(cg_field_read(get_file_pointer(), base, zone, sol_index, field.get_name().c_str(),
                               CGNS_ENUMV(RealDouble), rmin, rmax, rdata));
      }
      else {
        std::vector<double> cgns_data(num_to_get);
        for (int i = 0; i < comp_count; i++) {
          std::string var_name = get_component_name(field, Ioss::Field::InOut::INPUT, i + 1);
          CGCHECKM(cg_field_read(get_file_pointer(), base, zone, sol_index, var_name.c_str(),
                                 CGNS_ENUMV(RealDouble), rmin, rmax, cgns_data.data()));
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

  int64_t DatabaseIO::get_field_internal(const Ioss::NodeSet *ns, const Ioss::Field &field,
                                         void * /* data */, size_t /* data_size */) const
  {
    return Ioss::Utils::field_warning(ns, field, "input");
  }
  int64_t DatabaseIO::get_field_internal(const Ioss::EdgeSet *es, const Ioss::Field &field,
                                         void * /* data */, size_t /* data_size */) const
  {
    return Ioss::Utils::field_warning(es, field, "input");
  }
  int64_t DatabaseIO::get_field_internal(const Ioss::FaceSet *fs, const Ioss::Field &field,
                                         void * /* data */, size_t /* data_size */) const
  {
    return Ioss::Utils::field_warning(fs, field, "input");
  }
  int64_t DatabaseIO::get_field_internal(const Ioss::ElementSet *es, const Ioss::Field &field,
                                         void * /* data */, size_t /* data_size */) const
  {
    return Ioss::Utils::field_warning(es, field, "input");
  }
  int64_t DatabaseIO::get_field_internal(const Ioss::SideBlock *sb, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    int base = sb->get_property("base").get_int();
    int zone = Iocgns::Utils::get_db_zone(sb);
    int sect = sb->get_property("section").get_int();

    int64_t num_to_get = field.verify(data_size);
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

        // TODO(gdsjaar): ? Possibly rewrite using cgi_read_int_data so can skip reading element
        // connectivity
        int           nodes_per_face = sb->topology()->number_nodes();
        CGNSIntVector elements(nodes_per_face * num_to_get); // Not needed, but can't skip

        // The parent information will be formatted as:
        // *  `num_to_get` parent elements,
        // *  `num_to_get` zeros (other parent element for face, but on boundary so 0)
        // *  `num_to_get` face_on_element
        // *  `num_to_get` zeros (face on other parent element)
        CGNSIntVector parent(4 * num_to_get);

        CGCHECKM(
            cg_elements_read(get_file_pointer(), base, zone, sect, elements.data(), parent.data()));

        // See if the file contained `parent` data -- Some mesh generators only write the face
        // connectivity information.  We prefer the `parent/face_on_element` data, but if that does
        // not exist, then need to generate it based on the face connectivity information...

        if (parent[0] == 0) {
          // Don't have the parent/face_on_element data ... generate.
          Ioss::Utils::clear(parent);

          if (m_boundaryFaces.empty()) {
            Utils::generate_boundary_faces(get_region(), m_boundaryFaces, field.get_type());
          }

          // Now, iterate the face connectivity vector and find a match in `m_boundaryFaces`
          int  *i32data = reinterpret_cast<int *>(data);
          auto *i64data = reinterpret_cast<int64_t *>(data);

          size_t             offset           = 0;
          size_t             j                = 0;
          const std::string &name             = sb->parent_block()->name();
          auto              &boundary         = m_boundaryFaces[name];
          int                num_corner_nodes = sb->topology()->number_corner_nodes();
          SMART_ASSERT(num_corner_nodes == 3 || num_corner_nodes == 4)(num_corner_nodes);

          for (int64_t iface = 0; iface < num_to_get; iface++) {
            std::array<size_t, 4> conn = {{0, 0, 0, 0}};

            for (int i = 0; i < num_corner_nodes; i++) {
              conn[i] = elements[offset + i];
            }
            offset += nodes_per_face;

            Ioss::Face face(conn);
            // See if face is in m_boundaryFaces
            // If not, error
            // If so, then get parent element and element side.
            auto it = boundary.find(face);
            if (it != boundary.end()) {
              cgsize_t fid = (*it).element[0];
#if IOSS_DEBUG_OUTPUT
              fmt::print("Connectivity: {} {} {} {} maps to element {}, face {}\n", conn[0],
                         conn[1], conn[2], conn[3], fid / 10, fid % 10 + 1);
#endif
              if (field.get_type() == Ioss::Field::INT32) {
                i32data[j++] = fid / 10;
                i32data[j++] = fid % 10 + 1;
              }
              else {
                i64data[j++] = fid / 10;
                i64data[j++] = fid % 10 + 1;
              }
            }
            else {
              std::ostringstream errmsg;
              fmt::print(errmsg,
                         "ERROR: CGNS: Could not find face with connectivity {} {} {} {} on "
                         "sideblock {} with parent {}.",
                         conn[0], conn[1], conn[2], conn[3], sb->name(), name);
              IOSS_ERROR(errmsg);
            }
          }
        }
        else {
          size_t offset = m_zoneOffset[zone - 1];
          if (field.get_type() == Ioss::Field::INT32) {
            int   *idata = reinterpret_cast<int *>(data);
            size_t j     = 0;
            for (int64_t i = 0; i < num_to_get; i++) {
              idata[j++] = parent[num_to_get * 0 + i] + offset; // Element
              idata[j++] = parent[num_to_get * 2 + i];
              SMART_ASSERT(parent[num_to_get * 1 + i] == 0);
              SMART_ASSERT(parent[num_to_get * 3 + i] == 0);
            }
            // Adjust face numbers to IOSS convention instead of CGNS convention...
            Utils::map_cgns_face_to_ioss(sb->parent_element_topology(), num_to_get, idata);
          }
          else {
            auto  *idata = reinterpret_cast<int64_t *>(data);
            size_t j     = 0;
            for (int64_t i = 0; i < num_to_get; i++) {
              idata[j++] = parent[num_to_get * 0 + i] + offset; // Element
              idata[j++] = parent[num_to_get * 2 + i];
              SMART_ASSERT(parent[num_to_get * 1 + i] == 0);
              SMART_ASSERT(parent[num_to_get * 3 + i] == 0);
            }
            // Adjust face numbers to IOSS convention instead of CGNS convention...
            Utils::map_cgns_face_to_ioss(sb->parent_element_topology(), num_to_get, idata);
          }
        }
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

  int64_t DatabaseIO::get_field_internal(const Ioss::SideSet *fs, const Ioss::Field &field,
                                         void * /* data */, size_t /* data_size */) const
  {
    return Ioss::Utils::field_warning(fs, field, "input");
  }
  int64_t DatabaseIO::get_field_internal(const Ioss::CommSet *cs, const Ioss::Field &field,
                                         void * /* data */, size_t /* data_size */) const
  {
    return Ioss::Utils::field_warning(cs, field, "input");
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::Region *region, const Ioss::Field &field,
                                         void * /*data*/, size_t /*data_size*/) const
  {
    return Ioss::Utils::field_warning(region, field, "output");
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::StructuredBlock *sb, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    Ioss::Field::RoleType role = field.get_role();
    int                   base = sb->get_property("base").get_int();
    int                   zone = Iocgns::Utils::get_db_zone(sb);

    cgsize_t num_to_get = field.verify(data_size);

    // In this routine, if isParallel, then writing file-per-processor; not parallel io to single
    // file.
    if (isParallel && num_to_get == 0) {
      return 0;
    }

    if (role == Ioss::Field::MESH) {
      bool cell_field = Utils::is_cell_field(field);

      if (cell_field) {
        SMART_ASSERT(num_to_get == sb->get_property("cell_count").get_int());
      }

      auto *rdata = static_cast<double *>(data);

      int crd_idx = 0;
      if (field.get_name() == "mesh_model_coordinates_x") {
        SMART_ASSERT(!cell_field);
        CGCHECKM(cg_coord_write(get_file_pointer(), base, zone, CGNS_ENUMV(RealDouble),
                                "CoordinateX", rdata, &crd_idx));
      }

      else if (field.get_name() == "mesh_model_coordinates_y") {
        SMART_ASSERT(!cell_field);
        CGCHECKM(cg_coord_write(get_file_pointer(), base, zone, CGNS_ENUMV(RealDouble),
                                "CoordinateY", rdata, &crd_idx));
      }

      else if (field.get_name() == "mesh_model_coordinates_z") {
        SMART_ASSERT(!cell_field);
        CGCHECKM(cg_coord_write(get_file_pointer(), base, zone, CGNS_ENUMV(RealDouble),
                                "CoordinateZ", rdata, &crd_idx));
      }

      else if (field.get_name() == "mesh_model_coordinates") {
        SMART_ASSERT(!cell_field);
        int phys_dimension = get_region()->get_property("spatial_dimension").get_int();

        // Data required by upper classes store x0, y0, z0, ... xn,
        // yn, zn. Data stored in cgns file is x0, ..., xn, y0,
        // ..., yn, z0, ..., zn so we have to allocate some scratch
        // memory to read in the data and then map into supplied
        // 'data'
        std::vector<double> coord(num_to_get);

        // ========================================================================
        // Repetitive code for each coordinate direction; use a lambda to consolidate...
        auto coord_lambda = [this, &coord, num_to_get, phys_dimension, &rdata, base,
                             zone](const char *ord_name, int ordinate) {
          int crd_index = 0;

          // Map to global coordinate position...
          for (cgsize_t i = 0; i < num_to_get; i++) {
            coord[i] = rdata[phys_dimension * i + ordinate];
          }

          CGCHECKM(cg_coord_write(get_file_pointer(), base, zone, CGNS_ENUMV(RealDouble), ord_name,
                                  coord.data(), &crd_index));
        };
        // End of lambda...
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
      auto *rdata      = static_cast<double *>(data);
      int   cgns_field = 0;
      int   comp_count = field.get_component_count(Ioss::Field::InOut::OUTPUT);
      if (comp_count == 1) {
        CGCHECKM(cg_field_write(get_file_pointer(), base, zone, m_currentCellCenterSolutionIndex,
                                CGNS_ENUMV(RealDouble), field.get_name().c_str(), rdata,
                                &cgns_field));
        Utils::set_field_index(field, cgns_field, CGNS_ENUMV(CellCenter));
      }
      else {
        std::vector<double> cgns_data(num_to_get);
        for (int i = 0; i < comp_count; i++) {
          for (cgsize_t j = 0; j < num_to_get; j++) {
            cgns_data[j] = rdata[comp_count * j + i];
          }
          std::string var_name = get_component_name(field, Ioss::Field::InOut::OUTPUT, i + 1);

          CGCHECKM(cg_field_write(get_file_pointer(), base, zone, m_currentCellCenterSolutionIndex,
                                  CGNS_ENUMV(RealDouble), var_name.c_str(), cgns_data.data(),
                                  &cgns_field));
          if (i == 0) {
            Utils::set_field_index(field, cgns_field, CGNS_ENUMV(CellCenter));
          }
        }
      }
    }
    else {
      num_to_get = Ioss::Utils::field_warning(sb, field, "output");
    }

    return num_to_get;
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::ElementBlock *eb, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    size_t num_to_get = field.verify(data_size);
    if (num_to_get > 0) {

      Ioss::Field::RoleType role = field.get_role();

      if (role == Ioss::Field::MESH) {
        // Handle the MESH fields required for a CGNS file model.
        // (The 'genesis' portion)
        if (field.get_name() == "ids") {
          elemMap.set_size(elementCount);
          handle_block_ids(eb, elemMap, data, num_to_get, field.get_type());
        }
        else if (field.get_name() == "connectivity") {
          // This blocks zone has not been defined.
          // Get the "node block" for this element block...
          int element_nodes = eb->topology()->number_nodes();
          SMART_ASSERT(field.raw_storage()->component_count() == element_nodes);

          Ioss::MapContainer nodes;
          nodes.reserve(element_nodes * num_to_get + 1);
          nodes.push_back(0); // Unknown whether one-to-one map.

          if (field.get_type() == Ioss::Field::INT32) {
            auto *idata = reinterpret_cast<int *>(data);
            for (size_t i = 0; i < element_nodes * num_to_get; i++) {
              nodes.push_back(idata[i]);
            }
          }
          else {
            auto *idata = reinterpret_cast<int64_t *>(data);
            for (size_t i = 0; i < element_nodes * num_to_get; i++) {
              nodes.push_back(idata[i]);
            }
          }
          Ioss::Utils::uniquify(nodes, true);
          SMART_ASSERT(nodes[0] == 0);

          // Now, we have the node count and cell count so we can create a zone...
          int      base    = 1;
          int      zone    = 0;
          cgsize_t size[3] = {0, 0, 0};
          size[1]          = eb->entity_count();
          size[0]          = nodes.size() - 1;

          CGCHECKM(cg_zone_write(get_file_pointer(), base, eb->name().c_str(), size,
                                 CGNS_ENUMV(Unstructured), &zone));
          eb->property_update("db_zone", zone);
          eb->property_update("zone", zone);
          eb->property_update("id", zone);
          eb->property_update("guid", zone);
          eb->property_update("section", 1);
          eb->property_update("base", base);
          eb->property_update("zone_node_count", size[0]);
          eb->property_update("zone_element_count", size[1]);

          if (eb->property_exists("assembly")) {
            std::string assembly = eb->get_property("assembly").get_string();
            CGCHECKM(cg_goto(get_file_pointer(), base, "Zone_t", zone, "end"));
            CGCHECKM(cg_famname_write(assembly.c_str()));
          }

          // Now we have a valid zone so can update some data structures...
          m_zoneOffset[zone]                = m_zoneOffset[zone - 1] + size[1];
          m_globalToBlockLocalNodeMap[zone] = new Ioss::Map("element", "unknown", myProcessor);
          m_globalToBlockLocalNodeMap[zone]->map().swap(nodes);
          m_globalToBlockLocalNodeMap[zone]->build_reverse_map_no_lock();

          // Need to map global nodes to block-local node connectivity
          const auto &block_map = m_globalToBlockLocalNodeMap[zone];
          block_map->reverse_map_data(data, field, num_to_get * element_nodes);

          if (eb->entity_count() > 0) {
            CGNS_ENUMT(ElementType_t) type = Utils::map_topology_to_cgns(eb->topology()->name());
            int sect                       = 0;
            int field_byte_size            = (field.get_type() == Ioss::Field::INT32) ? 32 : 64;
            if (field_byte_size == CG_SIZEOF_SIZE) {
              Utils::unmap_cgns_connectivity(eb->topology(), num_to_get, (cgsize_t *)data);
              CGCHECKM(cg_section_write(get_file_pointer(), base, zone, "HexElements", type, 1,
                                        num_to_get, 0, (cgsize_t *)data, &sect));
            }
            else {
              CGNSIntVector connect;
              connect.reserve(element_nodes * num_to_get);
              if (field.get_type() == Ioss::Field::INT32) {
                auto *idata = reinterpret_cast<int *>(data);
                for (size_t i = 0; i < element_nodes * num_to_get; i++) {
                  connect.push_back(idata[i]);
                }
              }
              else {
                auto *idata = reinterpret_cast<int64_t *>(data);
                for (size_t i = 0; i < element_nodes * num_to_get; i++) {
                  connect.push_back(idata[i]);
                }
              }
              Utils::unmap_cgns_connectivity(eb->topology(), num_to_get, connect.data());
              CGCHECKM(cg_section_write(get_file_pointer(), base, zone, "HexElements", type, 1,
                                        num_to_get, 0, connect.data(), &sect));
            }
            m_bcOffset[zone] += num_to_get;
            eb->property_update("section", sect);
          }
        }
        else {
          num_to_get = Ioss::Utils::field_warning(eb, field, "output");
        }
      }
      else if (role == Ioss::Field::TRANSIENT) {
        int   base       = eb->get_property("base").get_int();
        int   zone       = Iocgns::Utils::get_db_zone(eb);
        auto *rdata      = static_cast<double *>(data);
        int   cgns_field = 0;
        int   comp_count = field.get_component_count(Ioss::Field::InOut::OUTPUT);
        if (comp_count == 1) {
          CGCHECKM(cg_field_write(get_file_pointer(), base, zone, m_currentCellCenterSolutionIndex,
                                  CGNS_ENUMV(RealDouble), field.get_name().c_str(), rdata,
                                  &cgns_field));
          Utils::set_field_index(field, cgns_field, CGNS_ENUMV(CellCenter));
        }
        else {
          std::vector<double> cgns_data(num_to_get);
          for (int i = 0; i < comp_count; i++) {
            for (size_t j = 0; j < num_to_get; j++) {
              cgns_data[j] = rdata[comp_count * j + i];
            }
            std::string var_name = get_component_name(field, Ioss::Field::InOut::OUTPUT, i + 1);

            CGCHECKM(cg_field_write(get_file_pointer(), base, zone,
                                    m_currentCellCenterSolutionIndex, CGNS_ENUMV(RealDouble),
                                    var_name.c_str(), cgns_data.data(), &cgns_field));
            if (i == 0) {
              Utils::set_field_index(field, cgns_field, CGNS_ENUMV(CellCenter));
            }
          }
        }
      }
      else {
        num_to_get = Ioss::Utils::field_warning(eb, field, "output");
      }
    }
    return num_to_get;
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::FaceBlock *fb, const Ioss::Field &field,
                                         void * /* data */, size_t /* data_size */) const
  {
    return Ioss::Utils::field_warning(fb, field, "output");
  }
  int64_t DatabaseIO::put_field_internal(const Ioss::EdgeBlock *eb, const Ioss::Field &field,
                                         void * /* data */, size_t /* data_size */) const
  {
    return Ioss::Utils::field_warning(eb, field, "output");
  }
  int64_t DatabaseIO::put_field_internal(const Ioss::NodeBlock *nb, const Ioss::Field &field,
                                         void *data, size_t data_size) const
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

    Ioss::Field::RoleType role       = field.get_role();
    int                   base       = 1;
    cgsize_t              num_to_get = field.verify(data_size);

    if (role == Ioss::Field::MESH) {
      if (field.get_name() == "ids") {
        // Only needed for parallel, but will be sequential in serial, so no space saving to not
        // use.
        nodeMap.set_size(num_to_get);
        if (int_byte_size_api() == 4) {
          nodeMap.set_map(static_cast<int *>(data), num_to_get, 0);
        }
        else {
          nodeMap.set_map(static_cast<int64_t *>(data), num_to_get, 0);
        }
      }
      else if (field.get_name() == "mesh_model_coordinates" ||
               field.get_name() == "mesh_model_coordinates_x" ||
               field.get_name() == "mesh_model_coordinates_y" ||
               field.get_name() == "mesh_model_coordinates_z") {

        // 'rdata' is of size number-nodes-on-this-rank
        auto *rdata = static_cast<double *>(data);

        if (field.get_name() == "mesh_model_coordinates") {
          int spatial_dim = nb->get_property("component_degree").get_int();
          for (const auto &block : m_globalToBlockLocalNodeMap) {
            auto zone = block.first;
            // NOTE: 'block_map' is 1-based indexing
            const auto         &block_map = block.second;
            std::vector<double> x(block_map->size());
            std::vector<double> y(block_map->size());
            std::vector<double> z(block_map->size());

            for (size_t i = 0; i < block_map->size(); i++) {
              auto idx = global_to_zone_local_idx(i, block_map, nodeMap, isParallel);
              SMART_ASSERT(idx < (size_t)num_to_get)(i)(idx)(num_to_get);
              x[i] = rdata[idx * spatial_dim + 0];
              if (spatial_dim > 1) {
                y[i] = rdata[idx * spatial_dim + 1];
              }
              if (spatial_dim > 2) {
                z[i] = rdata[idx * spatial_dim + 2];
              }
            }

            // Create the zone
            // Output this zones coordinates...
            int crd_idx = 0;
            CGCHECKM(cg_coord_write(get_file_pointer(), base, zone, CGNS_ENUMV(RealDouble),
                                    "CoordinateX", x.data(), &crd_idx));

            if (spatial_dim > 1) {
              CGCHECKM(cg_coord_write(get_file_pointer(), base, zone, CGNS_ENUMV(RealDouble),
                                      "CoordinateY", y.data(), &crd_idx));
            }

            if (spatial_dim > 2) {
              CGCHECKM(cg_coord_write(get_file_pointer(), base, zone, CGNS_ENUMV(RealDouble),
                                      "CoordinateZ", z.data(), &crd_idx));
            }
          }
        }
        else {
          // Outputting only a single coordinate value...
          for (const auto &block : m_globalToBlockLocalNodeMap) {
            auto zone = block.first;
            // NOTE: 'block_map' is 1-based indexing
            const auto         &block_map = block.second;
            std::vector<double> xyz(block_map->size());

            for (size_t i = 0; i < block_map->size(); i++) {
              auto idx = global_to_zone_local_idx(i, block_map, nodeMap, isParallel);
              SMART_ASSERT(idx < (size_t)num_to_get)(i)(idx)(num_to_get);
              xyz[i] = rdata[idx];
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
            CGCHECKM(cg_coord_write(get_file_pointer(), base, zone, CGNS_ENUMV(RealDouble),
                                    cgns_name.c_str(), xyz.data(), &crd_idx));
          }
        }
      }
      else {
        num_to_get = Ioss::Utils::field_warning(nb, field, "output");
      }
    }
    else if (role == Ioss::Field::TRANSIENT) {
      auto *rdata      = static_cast<double *>(data);
      int   cgns_field = 0;

      for (const auto &block : m_globalToBlockLocalNodeMap) {
        auto zone = block.first;
        // NOTE: 'block_map' has one more entry than node_count.
        // First entry is for something else.  'block_map' is
        // 1-based.
        const auto         &block_map = block.second;
        std::vector<double> blk_data(block_map->size());

        int comp_count = field.get_component_count(Ioss::Field::InOut::OUTPUT);

        if (comp_count == 1) {
          for (size_t j = 0; j < block_map->size(); j++) {
            auto idx    = global_to_zone_local_idx(j, block_map, nodeMap, isParallel);
            blk_data[j] = rdata[idx];
          }
          CGCHECKM(cg_field_write(get_file_pointer(), base, zone, m_currentVertexSolutionIndex,
                                  CGNS_ENUMV(RealDouble), field.get_name().c_str(), blk_data.data(),
                                  &cgns_field));
          Utils::set_field_index(field, cgns_field, CGNS_ENUMV(Vertex));
        }
        else {
          for (int i = 0; i < comp_count; i++) {
            for (size_t j = 0; j < block_map->size(); j++) {
              auto idx    = global_to_zone_local_idx(j, block_map, nodeMap, isParallel);
              blk_data[j] = rdata[comp_count * idx + i];
            }
            std::string var_name = get_component_name(field, Ioss::Field::InOut::OUTPUT, i + 1);
            CGCHECKM(cg_field_write(get_file_pointer(), base, zone, m_currentVertexSolutionIndex,
                                    CGNS_ENUMV(RealDouble), var_name.c_str(), blk_data.data(),
                                    &cgns_field));
            if (i == 0) {
              Utils::set_field_index(field, cgns_field, CGNS_ENUMV(Vertex));
            }
          }
        }
      }
    }
    else {
      num_to_get = Ioss::Utils::field_warning(nb, field, "output");
    }
    return num_to_get;
  }

  int64_t DatabaseIO::put_field_internal_sub_nb(const Ioss::NodeBlock *nb, const Ioss::Field &field,
                                                void *data, size_t data_size) const
  {
    // Outputs field data on a NodeBlock which is a "sub" NodeBlock -- contains the nodes for a
    // StructuredBlock instead of for the entire model.
    // Currently only TRANSIENT fields are output this way.  No valid reason, but that is the
    // current use case.

    // Get the StructuredBlock that this NodeBlock is contained in:
    const Ioss::GroupingEntity *sb         = nb->contained_in();
    int                         zone       = Iocgns::Utils::get_db_zone(sb);
    cgsize_t                    num_to_get = field.verify(data_size);

    // In this routine, if isParallel, then writing file-per-processor; not parallel io to single
    // file.
    if (isParallel && num_to_get == 0) {
      return 0;
    }

    Ioss::Field::RoleType role = field.get_role();
    if (role == Ioss::Field::TRANSIENT) {
      int   base       = 1;
      auto *rdata      = static_cast<double *>(data);
      int   cgns_field = 0;
      int   comp_count = field.get_component_count(Ioss::Field::InOut::OUTPUT);

      if (comp_count == 1) {
        CGCHECKM(cg_field_write(get_file_pointer(), base, zone, m_currentVertexSolutionIndex,
                                CGNS_ENUMV(RealDouble), field.get_name().c_str(), rdata,
                                &cgns_field));
        Utils::set_field_index(field, cgns_field, CGNS_ENUMV(Vertex));
      }
      else {
        std::vector<double> cgns_data(num_to_get);
        for (int i = 0; i < comp_count; i++) {
          for (cgsize_t j = 0; j < num_to_get; j++) {
            cgns_data[j] = rdata[comp_count * j + i];
          }
          std::string var_name = get_component_name(field, Ioss::Field::InOut::OUTPUT, i + 1);

          CGCHECKM(cg_field_write(get_file_pointer(), base, zone, m_currentVertexSolutionIndex,
                                  CGNS_ENUMV(RealDouble), var_name.c_str(), cgns_data.data(),
                                  &cgns_field));
          if (i == 0) {
            Utils::set_field_index(field, cgns_field, CGNS_ENUMV(Vertex));
          }
        }
      }
    }
    // Ignoring all other field role types...
    return num_to_get;
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::NodeSet *ns, const Ioss::Field &field,
                                         void * /* data */, size_t /* data_size */) const
  {
    return Ioss::Utils::field_warning(ns, field, "output");
  }
  int64_t DatabaseIO::put_field_internal(const Ioss::EdgeSet *es, const Ioss::Field &field,
                                         void * /* data */, size_t /* data_size */) const
  {
    return Ioss::Utils::field_warning(es, field, "output");
  }
  int64_t DatabaseIO::put_field_internal(const Ioss::FaceSet *fs, const Ioss::Field &field,
                                         void * /* data */, size_t /* data_size */) const
  {
    return Ioss::Utils::field_warning(fs, field, "output");
  }
  int64_t DatabaseIO::put_field_internal(const Ioss::ElementSet *es, const Ioss::Field &field,
                                         void * /* data */, size_t /* data_size */) const
  {
    return Ioss::Utils::field_warning(es, field, "output");
  }
  int64_t DatabaseIO::put_field_internal(const Ioss::SideBlock *sb, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    const Ioss::EntityBlock *parent_block = sb->parent_block();
    if (parent_block == nullptr) {
      std::ostringstream errmsg;
      fmt::print(errmsg,
                 "ERROR: CGNS: SideBlock '{}' does not have a parent-block specified.  This is "
                 "required for CGNS output.",
                 sb->name());
      IOSS_ERROR(errmsg);
    }

    int  base       = parent_block->get_property("base").get_int();
    int  zone       = Iocgns::Utils::get_db_zone(parent_block);
    auto num_to_get = field.verify(data_size);

    if (num_to_get == 0) {
      return num_to_get;
    }

    Ioss::Field::RoleType role = field.get_role();

    if (role == Ioss::Field::MESH) {
      // Handle the MESH fields required for a CGNS file model.
      // (The 'genesis' portion)
      if (field.get_name() == "element_side") {
        CGNS_ENUMT(ElementType_t) type = Utils::map_topology_to_cgns(sb->topology()->name());
        int sect                       = 0;

        cgsize_t cg_start = m_bcOffset[zone] + 1;
        cgsize_t cg_end   = m_bcOffset[zone] + num_to_get;
        m_bcOffset[zone] += num_to_get;

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

        CGCHECKM(cg_section_partial_write(get_file_pointer(), base, zone, sb_name.c_str(), type,
                                          cg_start, cg_end, 0, &sect));

        sb->property_update("section", sect);

        size_t        offset = m_zoneOffset[zone - 1];
        CGNSIntVector parent(4 * num_to_get);

        if (field.get_type() == Ioss::Field::INT32) {
          int   *idata = reinterpret_cast<int *>(data);
          size_t j     = 0;
          for (size_t i = 0; i < num_to_get; i++) {
            cgsize_t element           = elemMap.global_to_local(idata[j++]) - offset;
            parent[num_to_get * 0 + i] = element;
            parent[num_to_get * 2 + i] = idata[j++]; // side
          }
          // Adjust face numbers to IOSS convention instead of CGNS convention...
          Utils::map_ioss_face_to_cgns(sb->parent_element_topology(), num_to_get, parent);
        }
        else {
          auto  *idata = reinterpret_cast<int64_t *>(data);
          size_t j     = 0;
          for (size_t i = 0; i < num_to_get; i++) {
            cgsize_t element           = elemMap.global_to_local(idata[j++]) - offset;
            parent[num_to_get * 0 + i] = element; // Element
            parent[num_to_get * 2 + i] = idata[j++];
          }
          // Adjust face numbers to IOSS convention instead of CGNS convention...
          Utils::map_ioss_face_to_cgns(sb->parent_element_topology(), num_to_get, parent);
        }

        CGCHECKM(cg_parent_data_write(get_file_pointer(), base, zone, sect, parent.data()));
        return num_to_get;
      }
      else if (field.get_name() == "distribution_factors") {
        static bool warning_output = false;
        if (!warning_output) {
          fmt::print(Ioss::WARNING(),
                     "For CGNS output, the sideset distribution factors are not output.\n");
          warning_output = true;
        }
        return 0;
      }
      num_to_get = Ioss::Utils::field_warning(sb, field, "output");
    }
    else {
      num_to_get = Ioss::Utils::field_warning(sb, field, "output");
    }
    return num_to_get;
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::SideSet * /* ss */,
                                         const Ioss::Field & /* field */, void * /* data */,
                                         size_t /* data_size */) const
  {
    return 0;
  }
  int64_t DatabaseIO::put_field_internal(const Ioss::CommSet *cs, const Ioss::Field &field,
                                         void * /* data */, size_t /* data_size */) const
  {
    return Ioss::Utils::field_warning(cs, field, "output");
  }

  void DatabaseIO::write_results_meta_data() {}

  unsigned DatabaseIO::entity_field_support() const
  {
    return Ioss::NODEBLOCK | Ioss::ELEMENTBLOCK | Ioss::STRUCTUREDBLOCK | Ioss::NODESET |
           Ioss::SIDESET | Ioss::REGION;
  }

} // namespace Iocgns
