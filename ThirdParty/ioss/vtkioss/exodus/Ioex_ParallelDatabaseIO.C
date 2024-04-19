// ISSUES:
// 1. Does not handle unconnected nodes (not connected to any element)
//
// 2. SideSet distribution factors are klugy and may not fully work in
//    strange cases
//
//
// Copyright(C) 1999-2022 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include <exodus/Ioex_ParallelDatabaseIO.h>
#if defined PARALLEL_AWARE_EXODUS
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cfloat>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include "vtk_fmt.h"
#include VTK_FMT(fmt/ostream.h)
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <numeric>
#include <set>
#include <string>
#include <tokenize.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <utility>
#include <vector>

#include <Ioss_CodeTypes.h>

#include <exodus/Ioex_DecompositionData.h>
#include <exodus/Ioex_Internals.h>
#include <exodus/Ioex_Utils.h>
#include <vtk_exodusII.h>

#include <Ioss_Assembly.h>
#include <Ioss_Blob.h>
#include <Ioss_CommSet.h>
#include <Ioss_CoordinateFrame.h>
#include <Ioss_DBUsage.h>
#include <Ioss_DatabaseIO.h>
#include <Ioss_EdgeBlock.h>
#include <Ioss_EdgeSet.h>
#include <Ioss_ElementBlock.h>
#include <Ioss_ElementSet.h>
#include <Ioss_ElementTopology.h>
#include <Ioss_EntityBlock.h>
#include <Ioss_EntitySet.h>
#include <Ioss_EntityType.h>
#include <Ioss_FaceBlock.h>
#include <Ioss_FaceSet.h>
#include <Ioss_Field.h>
#include <Ioss_FileInfo.h>
#include <Ioss_GroupingEntity.h>
#include <Ioss_Map.h>
#include <Ioss_NodeBlock.h>
#include <Ioss_NodeSet.h>
#include <Ioss_ParallelUtils.h>
#include <Ioss_Property.h>
#include <Ioss_Region.h>
#include <Ioss_SideBlock.h>
#include <Ioss_SideSet.h>
#include <Ioss_State.h>
#include <Ioss_SurfaceSplit.h>
#include <Ioss_Utils.h>
#include <Ioss_VariableType.h>

#include <Ioss_FileInfo.h>
#undef MPICPP

// ========================================================================
// Static internal helper functions
// ========================================================================
namespace {
  const size_t max_line_length = MAX_LINE_LENGTH;

  const std::string SEP() { return std::string("@"); } // Separator for attribute offset storage
  const char       *complex_suffix[] = {".re", ".im"};

  void check_node_owning_processor_data(const Ioss::IntVector &nop, size_t file_node_count)
  {
    // Verify that the nop (NodeOwningProcessor) vector is not empty and is of the correct size.
    // This vector specifies which rank owns each node on this rank
    // Throws error if problem, otherwise returns quietly.
    if (file_node_count == 0) {
      return;
    }
    if (nop.empty()) {
      std::ostringstream errmsg;
      fmt::print(errmsg, "ERROR: The use of the 'compose' output option requires the definition of "
                         "the 'owning_processor'"
                         " field prior to the output of nodal data.  This field has not yet been "
                         "defined so output is not possible."
                         " For more information, contact gdsjaar@sandia.gov.\n");
      IOSS_ERROR(errmsg);
    }
    else if (nop.size() < file_node_count) {
      std::ostringstream errmsg;
      fmt::print(errmsg,
                 "ERROR: The 'owning_processor' data was defined, but it is not the correct size."
                 "  Its size is {}, but it must be at least this size {}."
                 " For more information, contact gdsjaar@sandia.gov.\n",
                 nop.size(), file_node_count);
      IOSS_ERROR(errmsg);
    }
  }
  void get_connectivity_data(int exoid, void *data, ex_entity_type type, ex_entity_id id,
                             int position, int int_size_api)
  {
    int ierr = 0;
    if (int_size_api == 8) {
      int64_t *conn[3];
      conn[0]        = nullptr;
      conn[1]        = nullptr;
      conn[2]        = nullptr;
      conn[position] = static_cast<int64_t *>(data);
      assert(1 == 0 && "Unimplemented fixme");
      ierr = ex_get_conn(exoid, type, id, conn[0], conn[1], conn[2]); // FIXME
    }
    else {
      int *conn[3];
      conn[0]        = nullptr;
      conn[1]        = nullptr;
      conn[2]        = nullptr;
      conn[position] = static_cast<int *>(data);
      assert(1 == 0 && "Unimplemented fixme");
      ierr = ex_get_conn(exoid, type, id, conn[0], conn[1], conn[2]); // FIXME
    }
    if (ierr < 0) {
      Ioex::exodus_error(exoid, __LINE__, __func__, __FILE__);
    }
  }

  template <typename T>
  void compute_internal_border_maps(T *entities, T *internal, size_t count, size_t entity_count)
  {
    for (size_t ij = 0; ij < count; ij++) {
      internal[ij] = 1;
    }
    for (size_t J = 0; J < entity_count; J++) {
      internal[entities[J] - 1] = 0;
    }

    size_t b = 0;
    for (size_t ij = 0; ij < count; ij++) {
      if (internal[ij] == 0) {
        entities[b++] = ij + 1;
      }
    }

    size_t k = 0;
    for (size_t ij = 0; ij < count; ij++) {
      if (internal[ij] == 1) {
        internal[k++] = ij + 1;
      }
    }
  }

  template <typename INT>
  void map_nodeset_id_data(const Ioss::IntVector &owning_processor, Ioss::Int64Vector &owned_nodes,
                           int this_processor, const INT *ids, size_t ids_size,
                           std::vector<INT> &file_data)
  {
    // Determine which nodes in this nodeset are owned by this processor.
    // Save this mapping in the "owned_nodes" vector for use in
    // mapping nodeset field data (df, transient, attributes, ...)
    for (size_t i = 0; i < ids_size; i++) {
      INT node = ids[i];
      if (owning_processor[node - 1] == this_processor) {
        file_data.push_back(ids[i]);
        owned_nodes.push_back(i);
      }
    }
  }

  template <typename T, typename U>
  void map_nodeset_data(Ioss::Int64Vector &owned_nodes, const T *data, std::vector<U> &file_data,
                        size_t offset = 0, size_t stride = 1)
  {
    // Pull out the locally owned nodeset data
    for (auto owned_node : owned_nodes) {
      file_data.push_back(data[stride * owned_node + offset]);
    }
  }

  template <typename T>
  void extract_data(std::vector<double> &local_data, T *data, size_t num_entity, size_t offset,
                    size_t comp_count)
  {
    local_data.resize(num_entity);
    if (comp_count == 1 && offset == 0) {
      for (size_t j = 0; j < num_entity; j++) {
        local_data[j] = data[j];
      }
    }
    else {
      for (size_t j = 0; j < num_entity; j++) {
        local_data[j] = data[offset];
        offset += comp_count;
      }
    }
  }

  // Ideally, there should only be a single data type for in and out
  // data, but in the node id map mapping, we have an int64_t coming
  // in and either an int or int64_t going out...
  template <typename T, typename U>
  void filter_owned_nodes(const Ioss::IntVector &owning_processor, int this_processor,
                          const T *data, std::vector<U> &file_data, size_t offset = 0,
                          size_t stride = 1)
  {
    size_t index = offset;
    for (auto op : owning_processor) {
      if (op == this_processor) {
        file_data.push_back(data[index]);
      }
      index += stride;
    }
  }

  // This version can be used *if* the input and output types are the same *and* the
  // input `data` can be modified / overwritten.
  template <typename T>
  void filter_owned_nodes(const Ioss::IntVector &owning_processor, int this_processor, T *data)
  {
    size_t index = 0;
    size_t entry = 0;
    for (auto op : owning_processor) {
      if (op == this_processor) {
        data[entry++] = data[index];
      }
      index++;
    }
  }

  template <typename INT>
  void map_local_to_global_implicit(INT *data, size_t count,
                                    const std::vector<int64_t> &global_implicit_map)
  {
    for (size_t i = 0; i < count; i++) {
      data[i] = global_implicit_map[data[i] - 1];
    }
  }

  void update_processor_offset_property(Ioss::Region *region, const Ioex::Mesh &mesh)
  {
    const Ioss::NodeBlockContainer &node_blocks = region->get_node_blocks();
    if (!node_blocks.empty()) {
      node_blocks[0]->property_add(
          Ioss::Property("_processor_offset", mesh.nodeblocks[0].procOffset));
    }
    const Ioss::EdgeBlockContainer &edge_blocks = region->get_edge_blocks();
    for (size_t i = 0; i < edge_blocks.size(); i++) {
      edge_blocks[i]->property_add(
          Ioss::Property("_processor_offset", mesh.edgeblocks[i].procOffset));
    }
    const Ioss::FaceBlockContainer &face_blocks = region->get_face_blocks();
    for (size_t i = 0; i < face_blocks.size(); i++) {
      face_blocks[i]->property_add(
          Ioss::Property("_processor_offset", mesh.faceblocks[i].procOffset));
    }

    int64_t                            offset         = 0; // Offset into global element map...
    const Ioss::ElementBlockContainer &element_blocks = region->get_element_blocks();
    for (size_t i = 0; i < element_blocks.size(); i++) {
      element_blocks[i]->property_add(Ioss::Property("global_map_offset", offset));
      offset += mesh.elemblocks[i].entityCount;
      element_blocks[i]->property_add(
          Ioss::Property("_processor_offset", mesh.elemblocks[i].procOffset));
    }

    const Ioss::NodeSetContainer &nodesets = region->get_nodesets();
    for (size_t i = 0; i < nodesets.size(); i++) {
      nodesets[i]->property_add(Ioss::Property("_processor_offset", mesh.nodesets[i].procOffset));
    }
    const Ioss::EdgeSetContainer &edgesets = region->get_edgesets();
    for (size_t i = 0; i < edgesets.size(); i++) {
      edgesets[i]->property_add(Ioss::Property("_processor_offset", mesh.edgesets[i].procOffset));
    }
    const Ioss::FaceSetContainer &facesets = region->get_facesets();
    for (size_t i = 0; i < facesets.size(); i++) {
      facesets[i]->property_add(Ioss::Property("_processor_offset", mesh.facesets[i].procOffset));
    }
    const Ioss::ElementSetContainer &elementsets = region->get_elementsets();
    for (size_t i = 0; i < facesets.size(); i++) {
      elementsets[i]->property_add(
          Ioss::Property("_processor_offset", mesh.elemsets[i].procOffset));
    }

    const Ioss::SideSetContainer &ssets = region->get_sidesets();
    for (size_t i = 0; i < ssets.size(); i++) {
      ssets[i]->property_add(Ioss::Property("_processor_offset", mesh.sidesets[i].procOffset));
      ssets[i]->property_add(Ioss::Property("processor_df_offset", mesh.sidesets[i].dfProcOffset));

      // Propagate down to owned sideblocks...
      const Ioss::SideBlockContainer &side_blocks = ssets[i]->get_side_blocks();
      for (auto &block : side_blocks) {
        block->property_add(Ioss::Property("_processor_offset", mesh.sidesets[i].procOffset));
        block->property_add(Ioss::Property("processor_df_offset", mesh.sidesets[i].dfProcOffset));
      }
    }
    const auto &blobs = region->get_blobs();
    for (size_t i = 0; i < blobs.size(); i++) {
      blobs[i]->property_add(Ioss::Property("_processor_offset", mesh.blobs[i].procOffset));
    }
  }
} // namespace

namespace Ioex {
  ParallelDatabaseIO::ParallelDatabaseIO(Ioss::Region *region, const std::string &filename,
                                         Ioss::DatabaseUsage db_usage, Ioss_MPI_Comm communicator,
                                         const Ioss::PropertyManager &props)
      : Ioex::BaseDatabaseIO(region, filename, db_usage, communicator, props)
  {
    usingParallelIO = true;
    if (!is_parallel_consistent()) {
      std::ostringstream errmsg;
      fmt::print(errmsg,
                 "ERROR: Parallel IO cannot be used in an application that is not guaranteeing "
                 "parallel consistent calls of the get and put field data functions.\n"
                 "The application created this database with a 'false' setting for the "
                 "isParallelConsistent property.");
      IOSS_ERROR(errmsg);
    }

    if (!is_input()) {
      // Check whether appending to or modifying existing file...
      if (open_create_behavior() == Ioss::DB_APPEND ||
          open_create_behavior() == Ioss::DB_APPEND_GROUP ||
          open_create_behavior() == Ioss::DB_MODIFY) {
        // Append to file if it already exists -- See if the file exists.
        Ioss::FileInfo file = Ioss::FileInfo(get_filename());
        fileExists          = file.exists();
        if (fileExists && myProcessor == 0) {
          fmt::print(Ioss::WarnOut(),
                     "Appending to existing database in parallel single-file "
                     "output mode is a new capability; please check results carefully. File '{}'",
                     get_filename());
        }
      }
    }
  }

  ParallelDatabaseIO::~ParallelDatabaseIO() = default;

  void ParallelDatabaseIO::release_memory__()
  {
    free_file_pointer();
    nodeMap.release_memory();
    edgeMap.release_memory();
    faceMap.release_memory();
    elemMap.release_memory();
    Ioss::Utils::clear(nodeOwningProcessor);
    Ioss::Utils::clear(nodeGlobalImplicitMap);
    Ioss::Utils::clear(elemGlobalImplicitMap);
    nodeGlobalImplicitMapDefined = false;
    elemGlobalImplicitMapDefined = false;
    nodesetOwnedNodes.clear();
    try {
      decomp.reset();
    }
    catch (...) {
    }
  }

  bool ParallelDatabaseIO::check_valid_file_ptr(bool write_message, std::string *error_msg,
                                                int *bad_count, bool abort_if_error) const
  {
    // Check for valid exodus_file_ptr (valid >= 0; invalid < 0)
    assert(isParallel);
    int global_file_ptr = util().global_minmax(m_exodusFilePtr, Ioss::ParallelUtils::DO_MIN);

    if (global_file_ptr < 0) {
      if (write_message || error_msg != nullptr || bad_count != nullptr) {
        Ioss::IntVector status;
        util().all_gather(m_exodusFilePtr, status);

        std::string open_create = is_input() ? "open input" : "create output";
        if (write_message || error_msg != nullptr) {
          std::vector<size_t> procs;
          for (int i = 0; i < util().parallel_size(); i++) {
            if (status[i] < 0) {
              procs.push_back(i);
            }
          }
          std::string error_list = Ioss::Utils::format_id_list(procs, "--");
          // See which processors could not open/create the file...
          std::ostringstream errmsg;
          fmt::print(errmsg, "ERROR: Unable to {} exodus database file '{}' on processors:\n\t{}",
                     open_create, get_filename(), error_list);
          fmt::print(errmsg, "\n");
          if (error_msg != nullptr) {
            *error_msg = errmsg.str();
          }
          if (write_message && myProcessor == 0) {
            fmt::print(Ioss::OUTPUT(), "{}", errmsg.str());
          }
        }
        if (bad_count != nullptr) {
          *bad_count = std::count_if(status.begin(), status.end(), [](int i) { return i < 0; });
        }
        if (abort_if_error) {
          std::ostringstream errmsg;
          fmt::print(errmsg, "ERROR: Cannot {} file '{}'", open_create, get_filename());
          IOSS_ERROR(errmsg);
        }
      }
      return false;
    }
    return true;
  }

  bool ParallelDatabaseIO::open_input_file(bool write_message, std::string *error_msg,
                                           int *bad_count, bool abort_if_error) const
  {
    int   cpu_word_size = sizeof(double);
    int   io_word_size  = 0;
    float version;

    int mode = exodusMode;
    if (int_byte_size_api() == 8) {
      mode |= EX_ALL_INT64_API;
    }

#if defined EX_DISKLESS
    // Experimental -- in memory read by netcdf library
    if (properties.exists("MEMORY_READ")) {
      mode |= EX_DISKLESS;
    }
#endif

    MPI_Info    info     = MPI_INFO_NULL;
    std::string filename = get_filename();

    // See bug description in thread at
    // https://www.open-mpi.org/community/lists/users/2015/01/26167.php and
    // https://prod.sandia.gov/sierra-trac/ticket/14679
    // Kluge is to set cwd to pathname, open file, then set cwd back to original.
    //
    // Since several different mpi implementations are based on the
    // mpich code which introduced this bug, it has been difficult to
    // create an ifdef'd version of the fix which is only applied to the
    // buggy mpiio code.  Therefore, we always do chdir call.  Maybe in several
    // years, we can remove this code and everything will work...

#if !defined(__IOSS_WINDOWS__)
    Ioss::FileInfo file(filename);
    std::string    path = file.pathname();
    filename            = file.tailname();
    char *current_cwd   = getcwd(nullptr, 0);
    if (!path.empty()) {
      auto success = chdir(path.c_str());
      if (success == -1) {
        if (write_message || error_msg != nullptr) {
          std::ostringstream errmsg;
          fmt::print(errmsg,
                     "ERROR: Directory '{}' does not exist.  Error in filename specification.",
                     path);
          fmt::print(errmsg, "\n");
          if (error_msg != nullptr) {
            *error_msg = errmsg.str();
          }
          if (write_message && myProcessor == 0) {
            fmt::print(Ioss::OUTPUT(), "{}", errmsg.str());
          }
          if (bad_count != nullptr) {
            *bad_count = 1;
          }
          if (abort_if_error) {
            IOSS_ERROR(errmsg);
          }
        }
        return false;
      }
    }
#endif

    bool do_timer = false;
    Ioss::Utils::check_set_bool_property(properties, "IOSS_TIME_FILE_OPEN_CLOSE", do_timer);
    double t_begin = (do_timer ? Ioss::Utils::timer() : 0);

    int app_opt_val = ex_opts(EX_VERBOSE);
    m_exodusFilePtr = ex_open_par(filename.c_str(), EX_READ | mode, &cpu_word_size, &io_word_size,
                                  &version, util().communicator(), info);

    if (do_timer) {
      double t_end    = Ioss::Utils::timer();
      double duration = util().global_minmax(t_end - t_begin, Ioss::ParallelUtils::DO_MAX);
      if (myProcessor == 0) {
        fmt::print(Ioss::DebugOut(), "File Open Time = {}\n", duration);
      }
    }

#if !defined(__IOSS_WINDOWS__)
    if (!path.empty()) {
      chdir(current_cwd);
    }
    std::free(current_cwd);
#endif

    bool is_ok = check_valid_file_ptr(write_message, error_msg, bad_count, abort_if_error);

    if (is_ok) {
      finalize_file_open();
    }
    ex_opts(app_opt_val); // Reset back to what it was.
    return is_ok;
  }

  bool ParallelDatabaseIO::handle_output_file(bool write_message, std::string *error_msg,
                                              int *bad_count, bool overwrite,
                                              bool abort_if_error) const
  {
    // If 'overwrite' is false, we do not want to overwrite or clobber
    // the output file if it already exists since the app might be
    // reading the restart data from this file and then later
    // clobbering it and then writing restart data to the same
    // file. So, for output, we first check whether the file exists
    // and if it it and is writable, assume that we can later create a
    // new or append to existing file.

    // if 'overwrite' is true, then clobber/append

    if (!overwrite) {
      // check if file exists and is writeable. If so, return true.
      // Only need to check on processor 0
      int int_is_ok = 0;
      if (myProcessor == 0) {
        Ioss::FileInfo file(get_filename());
        int_is_ok = file.exists() && file.is_writable() ? 1 : 0;
      }
      util().broadcast(int_is_ok);

      if (int_is_ok == 1) {
        // Note that at this point, we cannot totally guarantee that
        // we will be able to create the file when needed, but we have
        // a pretty good chance.  We can't guarantee creation without
        // creating and the app (or calling function) doesn't want us to overwrite...
        return true;
      }
      // File doesn't exist, so fall through and try to
      // create file since we won't be overwriting anything...
    }

    int   cpu_word_size = sizeof(double);
    int   io_word_size  = 0;
    float version;

    int mode = exodusMode;
    if (int_byte_size_api() == 8) {
      mode |= EX_ALL_INT64_API;
    }

#if defined EX_DISKLESS
    // Experimental -- in memory write by netcdf library
    if (properties.exists("MEMORY_WRITE")) {
      mode |= EX_DISKLESS;
    }
#endif

    MPI_Info info        = MPI_INFO_NULL;
    int      app_opt_val = ex_opts(EX_VERBOSE);
    Ioss::DatabaseIO::openDatabase__();

    std::string filename = get_dwname();

    Ioss::FileInfo file(filename);
#if !defined(__IOSS_WINDOWS__)
    std::string path  = file.pathname();
    filename          = file.tailname();
    char *current_cwd = getcwd(nullptr, 0);
    chdir(path.c_str());
#endif

    bool do_timer = false;
    Ioss::Utils::check_set_bool_property(properties, "IOSS_TIME_FILE_OPEN_CLOSE", do_timer);
    double t_begin = (do_timer ? Ioss::Utils::timer() : 0);

    if (fileExists) {
      m_exodusFilePtr = ex_open_par(filename.c_str(), EX_WRITE | mode, &cpu_word_size,
                                    &io_word_size, &version, util().communicator(), info);
    }
    else {
      // If the first write for this file, create it...
      if (int_byte_size_api() == 8) {
        // Check whether client actually wants 4-byte output on db
        // - If they specified INTEGER_SIZE_DB and the size isn't 8,
        //   then don't change mode and use the default 4-byte output.
        if (properties.exists("INTEGER_SIZE_DB")) {
          if (properties.get("INTEGER_SIZE_DB").get_int() == 8) {
            mode |= EX_ALL_INT64_DB;
          }
        }
        else {
          mode |= EX_ALL_INT64_DB;
        }
      }

      // Check whether we are on a NFS filesyste -- composed output is sometimes slow/hangs
      // on NFS
      if (myProcessor == 0) {
        if (file.is_nfs()) {
          fmt::print(
              Ioss::WarnOut(),
              "The database file: '{}'.\n"
              "\tis being written to an NFS filesystem. Some NFS filesystems have difficulty\n"
              "\twith parallel I/O (specifically writes). If you experience slow I/O,\n"
              "\ttry `export OMPI_MCA_fs_ufs_lock_algorithm=1` prior to running or\n"
              "\tnon-composed output or a different filesystem.\n",
              filename);
        }
      }
      m_exodusFilePtr = ex_create_par(filename.c_str(), mode, &cpu_word_size, &dbRealWordSize,
                                      util().communicator(), info);
    }

    if (do_timer) {
      double      t_end       = Ioss::Utils::timer();
      double      duration    = util().global_minmax(t_end - t_begin, Ioss::ParallelUtils::DO_MAX);
      std::string open_create = fileExists ? "Open" : "Create";
      if (myProcessor == 0) {
        fmt::print(Ioss::DebugOut(), "File {} Time = {}\n", open_create, duration);
      }
    }

#if !defined(__IOSS_WINDOWS__)
    chdir(current_cwd);
    std::free(current_cwd);
#endif

    bool is_ok = check_valid_file_ptr(write_message, error_msg, bad_count, abort_if_error);

    if (is_ok) {
      ex_set_max_name_length(m_exodusFilePtr, maximumNameLength);

      // Check properties handled post-create/open...
      if (properties.exists("COMPRESSION_METHOD")) {
        auto method                    = properties.get("COMPRESSION_METHOD").get_string();
        method                         = Ioss::Utils::lowercase(method);
        ex_compression_type exo_method = EX_COMPRESS_ZLIB;
        if (method == "zlib" || method == "libz" || method == "gzip") {
          exo_method = EX_COMPRESS_ZLIB;
        }
        else if (method == "szip") {
#if !defined(NC_HAS_SZIP_WRITE)
#define NC_HAS_SZIP_WRITE 0
#endif
#if NC_HAS_SZIP_WRITE
          exo_method = EX_COMPRESS_SZIP;
#else
          if (myProcessor == 0) {
            fmt::print(Ioss::WarnOut(), "The NetCDF library does not have SZip compression enabled."
                                        " 'zlib' will be used instead.\n\n");
          }
#endif
        }
        else {
          if (myProcessor == 0) {
            fmt::print(Ioss::WarnOut(),
                       "Unrecognized compression method specified: '{}'."
                       " 'zlib' will be used instead.\n\n",
                       method);
          }
        }
        ex_set_option(m_exodusFilePtr, EX_OPT_COMPRESSION_TYPE, exo_method);
      }
      if (properties.exists("COMPRESSION_LEVEL")) {
        int comp_level = properties.get("COMPRESSION_LEVEL").get_int();
        ex_set_option(m_exodusFilePtr, EX_OPT_COMPRESSION_LEVEL, comp_level);
      }
      if (properties.exists("COMPRESSION_SHUFFLE")) {
        int shuffle = properties.get("COMPRESSION_SHUFFLE").get_int();
        ex_set_option(m_exodusFilePtr, EX_OPT_COMPRESSION_SHUFFLE, shuffle);
      }
    }
    ex_opts(app_opt_val); // Reset back to what it was.
    return is_ok;
  }

  int ParallelDatabaseIO::get_file_pointer() const
  {
    return Ioex::BaseDatabaseIO::get_file_pointer();
  }

  int ParallelDatabaseIO::free_file_pointer() const
  {
    int flag;
    MPI_Initialized(&flag);
    if (flag == 0) {
      std::ostringstream errmsg;
      fmt::print(errmsg, "ERROR: MPI is not initialized.");
      IOSS_ERROR(errmsg);
    }

    // Make sure all file pointers are valid...
    int fp_min = util().global_minmax(m_exodusFilePtr, Ioss::ParallelUtils::DO_MIN);
    int fp_max = util().global_minmax(m_exodusFilePtr, Ioss::ParallelUtils::DO_MAX);
    if (fp_min != fp_max && fp_min < 0) {
      std::ostringstream errmsg;
      fmt::print(errmsg, "ERROR: Inconsistent file pointer values.");
      IOSS_ERROR(errmsg);
    }
    return Ioex::BaseDatabaseIO::free_file_pointer();
  }

  void ParallelDatabaseIO::read_meta_data__()
  {
    int exoid = get_file_pointer(); // get_file_pointer() must be called first.

    // APPENDING:
    // If parallel (single file, not fpp), we have assumptions
    // that the writing process (ranks, mesh, decomp, vars) is the
    // same for the original run that created this database and
    // for this run which is appending to the database so the
    // defining of the output database should be the same except
    // we don't write anything since it is already there.  We do
    // need the number of steps though...
    if (open_create_behavior() == Ioss::DB_APPEND) {
      get_step_times__();
      return;
    }

    if (int_byte_size_api() == 8) {
      decomp.reset(new DecompositionData<int64_t>(properties, util().communicator()));
    }
    else {
      decomp.reset(new DecompositionData<int>(properties, util().communicator()));
    }
    assert(decomp != nullptr);
    decomp->decompose_model(exoid);

    read_region();
    get_elemblocks();

    get_step_times__();

    get_nodeblocks();
    get_edgeblocks();
    get_faceblocks();

    check_side_topology();

    get_nodesets();
    get_sidesets();
#if 0
    get_edgesets();
    get_facesets();
    get_elemsets();
#endif

    get_commsets();

    // Add assemblies now that all entities should be defined... consistent across processors
    // (metadata)
    get_assemblies();

    get_blobs();

    handle_groups();

    add_region_fields();

    if (!is_input() && open_create_behavior() == Ioss::DB_APPEND) {
      get_map(EX_NODE_BLOCK);
      get_map(EX_ELEM_BLOCK);
    }
  }

  void ParallelDatabaseIO::read_region()
  {
    // Add properties and fields to the 'owning' region.
    // Also defines member variables of this class...
    ex_init_params info{};
    int            error = ex_get_init_ext(get_file_pointer(), &info);
    if (error < 0) {
      Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
    }

    spatialDimension = decomp->spatial_dimension();
    nodeCount        = decomp->ioss_node_count();
    edgeCount        = 0;
    faceCount        = 0;
    elementCount     = decomp->ioss_elem_count();

    m_groupCount[EX_NODE_BLOCK] = 1;
    m_groupCount[EX_EDGE_BLOCK] = info.num_edge_blk;
    m_groupCount[EX_FACE_BLOCK] = info.num_face_blk;
    m_groupCount[EX_ELEM_BLOCK] = info.num_elem_blk;

    m_groupCount[EX_NODE_SET] = info.num_node_sets;
    m_groupCount[EX_EDGE_SET] = info.num_edge_sets;
    m_groupCount[EX_FACE_SET] = info.num_face_sets;
    m_groupCount[EX_ELEM_SET] = info.num_elem_sets;

    m_groupCount[EX_SIDE_SET] = info.num_side_sets;
    m_groupCount[EX_ASSEMBLY] = info.num_assembly;
    m_groupCount[EX_BLOB]     = info.num_blob;

    // Checks: node, element, blocks > 0; warning if == 0; error if < 0
    check_valid_values();

    Ioss::Region *this_region = get_region();

    // See if any coordinate frames exist on mesh.  If so, define them on region.
    Ioex::add_coordinate_frames(get_file_pointer(), this_region);

    this_region->property_add(
        Ioss::Property("global_node_count", static_cast<int64_t>(decomp->global_node_count())));
    this_region->property_add(
        Ioss::Property("global_element_count", static_cast<int64_t>(decomp->global_elem_count())));

    this_region->property_add(Ioss::Property(std::string("title"), info.title));

    // Get QA records from database and add to qaRecords...
    int num_qa = ex_inquire_int(get_file_pointer(), EX_INQ_QA);
    if (num_qa > 0) {
      struct qa_element
      {
        char *qa_record[1][4];
      };

      auto qa = new qa_element[num_qa];
      for (int i = 0; i < num_qa; i++) {
        for (int j = 0; j < 4; j++) {
          qa[i].qa_record[0][j] = new char[MAX_STR_LENGTH + 1];
        }
      }

      ex_get_qa(get_file_pointer(), qa[0].qa_record);
      for (int i = 0; i < num_qa; i++) {
        add_qa_record(qa[i].qa_record[0][0], qa[i].qa_record[0][1], qa[i].qa_record[0][2],
                      qa[i].qa_record[0][3]);
      }
      for (int i = 0; i < num_qa; i++) {
        for (int j = 0; j < 4; j++) {
          delete[] qa[i].qa_record[0][j];
        }
      }
      delete[] qa;
    }

    // Get information records from database and add to informationRecords...
    int num_info = ex_inquire_int(get_file_pointer(), EX_INQ_INFO);
    if (num_info > 0) {
      char **info_rec = Ioss::Utils::get_name_array(
          num_info, max_line_length); // 'total_lines' pointers to char buffers
      ex_get_info(get_file_pointer(), info_rec);
      for (int i = 0; i < num_info; i++) {
        add_information_record(info_rec[i]);
      }
      Ioss::Utils::delete_name_array(info_rec, num_info);
    }
  }

  void ParallelDatabaseIO::get_step_times__()
  {
    double              last_time      = DBL_MAX;
    int                 timestep_count = 0;
    std::vector<double> tsteps(0);

    {
      timestep_count = ex_inquire_int(get_file_pointer(), EX_INQ_TIME);
      if (timestep_count <= 0) {
        return;
      }

      // For an exodusII file, timesteps are global and are stored in the region.
      // Read the timesteps and add to the region
      tsteps.resize(timestep_count);
      int error = ex_get_all_times(get_file_pointer(), tsteps.data());
      if (error < 0) {
        Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
      }

      // See if the "last_written_time" attribute exists and if it
      // does, check that it matches the largest time in 'tsteps'.
      Ioex::read_last_time_attribute(get_file_pointer(), &last_time);
    }

    // Only add states that are less than or equal to the
    // 'last_time' value which is either DBL_MAX or the value of
    // the last time successfully written to the database and
    // flushed to disk.  This is used to avoid corrupt data arising
    // from a job that crashed during the writing of the last step
    // on the database.  Output a warning message if there is
    // potentially corrupt data on the database...

    // Check whether user or application wants to limit the times even further...
    // One use case is that job is restarting at a time prior to what has been
    // written to the results file, so want to start appending after
    // restart time instead of at end time on database.
    int max_step = properties.get_optional("APPEND_OUTPUT_AFTER_STEP", timestep_count);
    max_step     = std::min(max_step, timestep_count);

    double max_time =
        properties.get_optional("APPEND_OUTPUT_AFTER_TIME", std::numeric_limits<double>::max());
    last_time = std::min(last_time, max_time);

    Ioss::Region *this_region = get_region();
    for (int i = 0; i < max_step; i++) {
      if (tsteps[i] <= last_time) {
        this_region->add_state(tsteps[i] * timeScaleFactor);
      }
      else {
        if (myProcessor == 0 && max_time == std::numeric_limits<double>::max()) {
          // NOTE: Don't want to warn on all processors if there are
          // corrupt steps on all databases, but this will only print
          // a warning if there is a corrupt step on processor
          // 0... Need better warnings which won't overload in the
          // worst case...
          fmt::print(Ioss::WarnOut(),
                     "Skipping step {} at time {} in database file\n\t{}.\nThe data for that step "
                     "is possibly corrupt.\n",
                     fmt::group_digits(i + 1), tsteps[i], get_filename());
        }
      }
    }
  }

  const Ioss::Map &ParallelDatabaseIO::get_map(ex_entity_type type) const
  {
    switch (type) {
    case EX_NODE_BLOCK:
    case EX_NODE_SET: {
      assert(decomp != nullptr);
      size_t offset = decomp->decomp_node_offset();
      size_t count  = decomp->decomp_node_count();
      return get_map(nodeMap, nodeCount, offset, count, EX_NODE_MAP, EX_INQ_NODE_MAP);
    }
    case EX_ELEM_BLOCK:
    case EX_ELEM_SET: {
      assert(decomp != nullptr);
      size_t offset = decomp->decomp_elem_offset();
      size_t count  = decomp->decomp_elem_count();
      return get_map(elemMap, elementCount, offset, count, EX_ELEM_MAP, EX_INQ_ELEM_MAP);
    }

    case EX_FACE_BLOCK:
    case EX_FACE_SET: return get_map(faceMap, faceCount, 0, 0, EX_FACE_MAP, EX_INQ_FACE_MAP);

    case EX_EDGE_BLOCK:
    case EX_EDGE_SET: return get_map(edgeMap, edgeCount, 0, 0, EX_EDGE_MAP, EX_INQ_EDGE_MAP);

    default:
      std::ostringstream errmsg;
      fmt::print(errmsg, "INTERNAL ERROR: Invalid map type. "
                         "Something is wrong in the Ioex::ParallelDatabaseIO::get_map() function. "
                         "Please report.\n");
      IOSS_ERROR(errmsg);
    }
  }

  const Ioss::Map &ParallelDatabaseIO::get_map(Ioss::Map &entity_map, int64_t entity_count,
                                               int64_t file_offset, int64_t file_count,
                                               ex_entity_type entity_type,
                                               ex_inquiry     inquiry_type) const
  {
    // Allocate space for node number map and read it in...
    // Can be called multiple times, allocate 1 time only
    if (entity_map.map().empty()) {
      entity_map.set_size(entity_count);

      if (is_input()) {
        Ioss::MapContainer file_data(file_count);
        int                error = 0;
        // Check whether there is a "original_global_id_map" map on
        // the database. If so, use it instead of the "node_num_map".
        bool map_read  = false;
        int  map_count = ex_inquire_int(get_file_pointer(), inquiry_type);
        if (map_count > 0) {
          char **names = Ioss::Utils::get_name_array(map_count, maximumNameLength);
          int    ierr  = ex_get_names(get_file_pointer(), entity_type, names);
          if (ierr < 0) {
            Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
          }

          if (map_count == 1 && Ioss::Utils::str_equal(names[0], "original_global_id_map")) {
            if (int_byte_size_api() == 8) {
              error = ex_get_partial_num_map(get_file_pointer(), entity_type, 1, file_offset + 1,
                                             file_count, file_data.data());
            }
            else {
              // Ioss stores as 64-bit, read as 32-bit and copy over...
              Ioss::IntVector tmp_map(file_count);
              error = ex_get_partial_num_map(get_file_pointer(), entity_type, 1, file_offset + 1,
                                             file_count, tmp_map.data());
              std::copy(tmp_map.begin(), tmp_map.end(), file_data.begin());
            }
            if (error >= 0) {
              map_read = true;
            }
          }
          Ioss::Utils::delete_name_array(names, map_count);
        }

        if (!map_read) {
          if (int_byte_size_api() == 8) {
            error = ex_get_partial_id_map(get_file_pointer(), entity_type, file_offset + 1,
                                          file_count, file_data.data());
          }
          else {
            // Ioss stores as 64-bit, read as 32-bit and copy over...
            Ioss::IntVector tmp_map(file_count);
            error = ex_get_partial_id_map(get_file_pointer(), entity_type, file_offset + 1,
                                          file_count, tmp_map.data());
            std::copy(tmp_map.begin(), tmp_map.end(), file_data.begin());
          }
        }

        if (error >= 0) {
          if (entity_type == EX_NODE_MAP) {
            decomp->communicate_node_data(file_data.data(), &entity_map.map()[1], 1);
          }
          else if (entity_type == EX_ELEM_MAP) {
            decomp->communicate_element_data(file_data.data(), &entity_map.map()[1], 1);
          }
        }
        else {
          // Clear out the vector...
          Ioss::MapContainer().swap(entity_map.map());
          Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
        }

        // Check for sequential node map.
        // If not, build the reverse G2L node map...
        entity_map.is_sequential(true);
        entity_map.build_reverse_map();
      }
      else {
        // Output database; entity_map.map() not set yet... Build a default map.
        entity_map.set_default(entity_count);
      }
    }
    return entity_map;
  }

  void ParallelDatabaseIO::get_elemblocks() { get_blocks(EX_ELEM_BLOCK, 0, "block"); }

  void ParallelDatabaseIO::get_faceblocks()
  {
    //    get_blocks(EX_FACE_BLOCK, 1, "faceblock");
  }

  void ParallelDatabaseIO::get_edgeblocks()
  {
    //    get_blocks(EX_EDGE_BLOCK, 2, "edgeblock");
  }

  void ParallelDatabaseIO::get_blocks(ex_entity_type entity_type, int rank_offset,
                                      const std::string &basename)
  {
    // Attributes of an X block are:  (X = element, face, or edge)
    // -- id
    // -- name
    // -- X type
    // -- number of Xs
    // -- number of attributes per X
    // -- number of nodes per X (derivable from type)
    // -- number of faces per X (derivable from type)
    // -- number of edges per X (derivable from type)

    // In a parallel execution, it is possible that an X block will have
    // no Xs on a particular processor...

    // NOTE: This routine may be called multiple times on a single database.
    //       make sure it is not dependent on being called one time only...

    // Get exodusII X block metadata
    if (m_groupCount[entity_type] == 0) {
      return;
    }

    assert(entity_type == EX_ELEM_BLOCK);

    Ioss::Int64Vector X_block_ids(m_groupCount[entity_type]);
    int               used_blocks = 0;

    int error;
    if ((ex_int64_status(get_file_pointer()) & EX_IDS_INT64_API) != 0) {
      error = ex_get_ids(get_file_pointer(), entity_type, X_block_ids.data());
    }
    else {
      Ioss::IntVector tmp_set_ids(X_block_ids.size());
      error = ex_get_ids(get_file_pointer(), entity_type, tmp_set_ids.data());
      if (error >= 0) {
        std::copy(tmp_set_ids.begin(), tmp_set_ids.end(), X_block_ids.begin());
      }
    }
    if (error < 0) {
      Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
    }

    // If the model contains assemblies, we want to retain the empty blocks since the blocks
    // might be in an assembly.  This is typically the case when an application is running
    // in parallel, but is telling IOSS that it is "serial" (MPI_COMM_SELF) and taking care
    // of synchronization at the app level instead of down here...
    bool retain_empty_blocks = m_groupCount[EX_ASSEMBLY] > 0;

    // The application can override this setting using the `RETAIN_EMPTY_BLOCKS` property.
    // This can either set to TRUE or FALSE...  Note that `retain_empty_blocks` will not be
    // changed unless the property exists.
    Ioss::Utils::check_set_bool_property(properties, "RETAIN_EMPTY_BLOCKS", retain_empty_blocks);

    int nvar = std::numeric_limits<int>::max(); // Number of 'block' vars on database. Used to
                                                // skip querying if none.
    int nmap = std::numeric_limits<int>::max(); // Number of 'block' maps on database. Used to
                                                // skip querying if none.
    for (int iblk = 0; iblk < m_groupCount[entity_type]; iblk++) {
      if (decomp->el_blocks[iblk].global_count() == 0 && !retain_empty_blocks) {
        continue;
      }

      int64_t id = decomp->el_blocks[iblk].id();

      bool        db_has_name = false;
      std::string alias       = Ioss::Utils::encode_entity_name(basename, id);
      std::string block_name;
      if (ignore_database_names()) {
        block_name = alias;
      }
      else {
        block_name = Ioex::get_entity_name(get_file_pointer(), entity_type, id, basename,
                                           maximumNameLength, db_has_name);
      }
      if (get_use_generic_canonical_name()) {
        std::swap(block_name, alias);
      }

      std::string save_type = decomp->el_blocks[iblk].topologyType;
      std::string type      = Ioss::Utils::fixup_type(decomp->el_blocks[iblk].topologyType,
                                                      decomp->el_blocks[iblk].nodesPerEntity,
                                                      spatialDimension - rank_offset);

      if (decomp->el_blocks[iblk].global_count() == 0 && type.empty()) {
        auto tokens = Ioss::tokenize(block_name, "_");
        if (tokens.size() >= 2) {
          // Check whether last token names an X topology type...
          const Ioss::ElementTopology *topology =
              Ioss::ElementTopology::factory(tokens.back(), true);
          if (topology != nullptr) {
            type = topology->name();
          }
        }
      }

      if (type == "null" || type.empty()) {
        // If we have no idea what the topology type for an empty
        // X block is, call it "unknown"
        type = "unknown";
      }

      Ioss::EntityBlock *io_block = nullptr;
      if (entity_type == EX_ELEM_BLOCK) {
        auto *eblock =
            new Ioss::ElementBlock(this, block_name, type, decomp->el_blocks[iblk].ioss_count());
        io_block = eblock;
        io_block->property_add(Ioss::Property("id", id));
        io_block->property_add(Ioss::Property("guid", util().generate_guid(id)));
        io_block->property_add(Ioss::Property("iblk", iblk)); // Sequence in decomp.

        if (db_has_name) {
          std::string *db_name = &block_name;
          if (get_use_generic_canonical_name()) {
            db_name = &alias;
          }
          io_block->property_add(Ioss::Property("db_name", *db_name));
        }
        get_region()->add(eblock);
#if 0
        } else if (entity_type == EX_FACE_BLOCK) {
          Ioss::FaceBlock *fblock = new Ioss::FaceBlock(this, block_name, type, block.num_entry);
          io_block = fblock;
          io_block->property_add(Ioss::Property("id", id));
          io_block->property_add(Ioss::Property("guid", util().generate_guid(id)));
          if (db_has_name) {
            std::string *db_name = &block_name;
            if (get_use_generic_canonical_name()) {
              db_name = &alias;
            }
            io_block->property_add(Ioss::Property("db_name", *db_name));
          }
          get_region()->add(fblock);
        } else if (entity_type == EX_EDGE_BLOCK) {
          Ioss::EdgeBlock *eblock = new Ioss::EdgeBlock(this, block_name, type, block.num_entry);
          io_block = eblock;
          io_block->property_add(Ioss::Property("id", id));
          io_block->property_add(Ioss::Property("guid", util().generate_guid(id)));
          if (db_has_name) {
            std::string *db_name = &block_name;
            if (get_use_generic_canonical_name()) {
              db_name = &alias;
            }
            io_block->property_add(Ioss::Property("db_name", *db_name));
          }
          get_region()->add(eblock);
#endif
      }
      else {
        std::ostringstream errmsg;
        fmt::print(errmsg, "ERROR: Invalid type in get_blocks()");
        IOSS_ERROR(errmsg);
      }

#if 0
        // See which connectivity options were defined for this block.
        // X -> Node is always defined.
        // X -> Face?
        if (block.num_faces_per_entry > 0 && rank_offset < 1) {
          std::string storage = "Real["+std::to_string(block.num_faces_per_entry)+"]";
          io_block->field_add(Ioss::Field("connectivity_face",
                                          io_block->field_int_type(), storage, Ioss::Field::MESH));
        }
        // X -> Edge?
        if (block.num_edges_per_entry > 0 && rank_offset < 2) {
          std::string storage = "Real["+std::to_string(block.num_edges_per_entry)+"]";
          io_block->field_add(Ioss::Field("connectivity_edge",
                                          io_block->field_int_type(), storage, Ioss::Field::MESH));
        }
#endif

      // Maintain block order on output database...
      io_block->property_add(Ioss::Property("original_block_order", used_blocks++));

      if (save_type != "null" && save_type != "") {
        io_block->property_update("original_topology_type", save_type);
      }

      io_block->property_add(Ioss::Property(
          "global_entity_count", static_cast<int64_t>(decomp->el_blocks[iblk].ioss_count())));

      if (block_name != alias) {
        get_region()->add_alias(block_name, alias, io_block->type());
      }

      // Check for additional variables.
      add_attribute_fields(io_block, decomp->el_blocks[iblk].attributeCount, type);
      if (nvar > 0) {
        nvar = add_results_fields(io_block, iblk);
      }

      if (entity_type == EX_ELEM_BLOCK) {
        if (nmap > 0) {
          nmap =
              Ioex::add_map_fields(get_file_pointer(), dynamic_cast<Ioss::ElementBlock *>(io_block),
                                   decomp->el_blocks[iblk].ioss_count(), maximumNameLength);
        }

        if (!assemblyOmissions.empty() || !assemblyInclusions.empty()) {
          update_block_omissions_from_assemblies();
        }

        assert(blockOmissions.empty() || blockInclusions.empty()); // Only one can be non-empty

        // Handle all block omissions or inclusions...
        // This only affects the generation of surfaces...
        if (!blockOmissions.empty()) {
          for (const auto &name : blockOmissions) {
            auto block = get_region()->get_element_block(name);
            if (block) {
              block->property_add(Ioss::Property(std::string("omitted"), 1));
            }
          }
        }

        if (!blockInclusions.empty()) {
          const auto &blocks = get_region()->get_element_blocks();
          for (auto &block : blocks) {
            block->property_add(Ioss::Property(std::string("omitted"), 1));
          }

          // Now, erase the property on any blocks in the inclusion list...
          for (const auto &name : blockInclusions) {
            auto block = get_region()->get_element_block(name);
            if (block != nullptr) {
              block->property_erase("omitted");
            }
          }
        }
      }
    }
  }

  void ParallelDatabaseIO::compute_node_status() const
  {
    // Create a field for all nodes in the model indicating
    // the connectivity 'status' of the node.  The status values are:
    // 0 -- node not connected to any elements
    // 1 -- node only connected to omitted elements
    // 2 -- node only connected to active elements
    // 3 -- node at border of active and omitted elements.

    /// \todo Get working for parallel...

    if (nodeConnectivityStatusCalculated) {
      return;
    }

    nodeConnectivityStatus.resize(nodeCount);

    const Ioss::ElementBlockContainer &element_blocks = get_region()->get_element_blocks();
    assert(Ioss::Utils::check_block_order(element_blocks));

    for (Ioss::ElementBlock *block : element_blocks) {
      unsigned char status = 2;
      if (Ioss::Utils::block_is_omitted(block)) {
        status = 1;
      }

      int64_t id               = block->get_property("id").get_int();
      int     element_nodes    = block->topology()->number_nodes();
      int64_t my_element_count = block->entity_count();
      int     order            = block->get_property("iblk").get_int();
      if (int_byte_size_api() == 8) {
        std::vector<int64_t> conn(my_element_count * element_nodes);
        decomp->get_block_connectivity(get_file_pointer(), conn.data(), id, order, element_nodes);
        for (auto node : conn) {
          nodeConnectivityStatus[node - 1] |= status;
        }
      }
      else {
        std::vector<int> conn(my_element_count * element_nodes);
        decomp->get_block_connectivity(get_file_pointer(), conn.data(), id, order, element_nodes);
        for (auto node : conn) {
          nodeConnectivityStatus[node - 1] |= status;
        }
      }
    }
    nodeConnectivityStatusCalculated = true;
  }

  namespace {
    void get_element_sides_lists(const std::unique_ptr<DecompositionDataBase> &decomp, int exoid,
                                 int64_t id, int int_byte_size, int64_t number_sides,
                                 Ioss::Int64Vector &element, Ioss::Int64Vector &sides)
    {
      // Check whether we already populated the element/sides vectors.
      if (element.empty() && sides.empty() && number_sides > 0) {
        element.resize(number_sides);
        sides.resize(number_sides);

        // Easier below here if the element and sides are a known 64-bit size...
        // Kluge here to do that...
        if (int_byte_size == 4) {
          Ioss::Field side_field("sides", Ioss::Field::INTEGER, IOSS_SCALAR(), Ioss::Field::MESH,
                                 number_sides);
          Ioss::Field elem_field("ids_raw", Ioss::Field::INTEGER, IOSS_SCALAR(), Ioss::Field::MESH,
                                 number_sides);

          Ioss::IntVector e32(number_sides);
          decomp->get_set_mesh_var(exoid, EX_SIDE_SET, id, elem_field, e32.data());
          std::copy(e32.begin(), e32.end(), element.begin());
          decomp->get_set_mesh_var(exoid, EX_SIDE_SET, id, side_field, e32.data());
          std::copy(e32.begin(), e32.end(), sides.begin());
        }
        else {
          Ioss::Field side_field("sides", Ioss::Field::INT64, IOSS_SCALAR(), Ioss::Field::MESH,
                                 number_sides);
          Ioss::Field elem_field("ids_raw", Ioss::Field::INT64, IOSS_SCALAR(), Ioss::Field::MESH,
                                 number_sides);
          decomp->get_set_mesh_var(exoid, EX_SIDE_SET, id, elem_field, element.data());
          decomp->get_set_mesh_var(exoid, EX_SIDE_SET, id, side_field, sides.data());
        }
      }
    }
  } // namespace

  void ParallelDatabaseIO::get_sidesets()
  {
    // This function creates all sidesets (surfaces) for a
    // model.  Note that a sideset contains 1 or more sideblocks
    // which are homogeneous (same topology). In serial execution,
    // this is fairly straightforward since there are no null sets and
    // we have all the information we need. (...except see below for
    // surface evolution).
    //
    // However, in a parallel execution, we have the possibility that a
    // side set will have no sides or distribution factors on
    // a particular processor.  We then don't know the block topology of
    // the block(s) contained in this set. We could do some
    // communication and get a good idea of the topologies that are in
    // the set.

    if (m_groupCount[EX_SIDE_SET] > 0) {
      check_side_topology();

      // Get exodusII sideset metadata

      // Get the names (may not exist) of all sidesets and see if they are actually
      // side "blocks" (perhaps written by IO system for a restart).  In that case,
      // they were split by a previous run and we need to reconstruct the side "set"
      // that may contain one or more of them.
      Ioex::SideSetMap ss_map;
      Ioex::SideSetSet ss_set;

      {
        int error;

        for (const auto &ss : decomp->side_sets) {
          int64_t           id = ss.id();
          std::vector<char> ss_name(maximumNameLength + 1);
          error = ex_get_name(get_file_pointer(), EX_SIDE_SET, id, ss_name.data());
          if (error < 0) {
            Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
          }
          if (ss_name[0] != '\0') {
            Ioss::Utils::fixup_name(ss_name.data());
            Ioex::decode_surface_name(ss_map, ss_set, ss_name.data());
          }
        }
      }

      // Create sidesets for each entry in the ss_set... These are the
      // sidesets which were probably written by a previous run of the
      // IO system and are already split into homogeneous pieces...
      {
        for (auto &ss_name : ss_set) {
          auto *side_set = new Ioss::SideSet(this, ss_name);
          get_region()->add(side_set);
          int64_t id = Ioex::extract_id(ss_name);
          if (id > 0) {
            side_set->property_add(Ioss::Property("id", id));
            side_set->property_add(Ioss::Property("guid", util().generate_guid(id)));
          }
        }
      }

      for (int iss = 0; iss < m_groupCount[EX_SIDE_SET]; iss++) {
        int64_t           id = decomp->side_sets[iss].id();
        std::string       sid;
        Ioex::TopologyMap topo_map;
        Ioex::TopologyMap side_map; // Used to determine side consistency

        Ioss::SurfaceSplitType split_type = splitType;
        std::string            side_set_name;
        Ioss::SideSet         *side_set = nullptr;

        bool db_has_name = false;
        {
          std::string alias = Ioss::Utils::encode_entity_name("surface", id);
          if (ignore_database_names()) {
            side_set_name = alias;
          }
          else {
            side_set_name = Ioex::get_entity_name(get_file_pointer(), EX_SIDE_SET, id, "surface",
                                                  maximumNameLength, db_has_name);
          }

          if (side_set_name == "universal_sideset") {
            split_type = Ioss::SPLIT_BY_DONT_SPLIT;
          }

          bool in_ss_map = false;
          auto SSM       = ss_map.find(side_set_name);
          if (SSM != ss_map.end()) {
            in_ss_map            = true;
            std::string ess_name = (*SSM).second;
            side_set             = get_region()->get_sideset(ess_name);
            Ioss::Utils::check_non_null(side_set, "sideset", ess_name, __func__);
          }
          else {
            if (get_use_generic_canonical_name()) {
              std::swap(side_set_name, alias);
            }
            side_set = new Ioss::SideSet(this, side_set_name);
            side_set->property_add(Ioss::Property("id", id));
            side_set->property_add(Ioss::Property("guid", util().generate_guid(id)));
            if (db_has_name) {
              std::string *db_name = &side_set_name;
              if (get_use_generic_canonical_name()) {
                db_name = &alias;
              }
              side_set->property_add(Ioss::Property("db_name", *db_name));
            }
            get_region()->add(side_set);

            get_region()->add_alias(side_set_name, alias, Ioss::SIDESET);
            get_region()->add_alias(side_set_name, Ioss::Utils::encode_entity_name("sideset", id),
                                    Ioss::SIDESET);
          }

          //      split_type = SPLIT_BY_ELEMENT_BLOCK;
          //      split_type = SPLIT_BY_TOPOLOGIES;
          //      split_type = SPLIT_BY_DONT_SPLIT;

          // Determine how many side blocks compose this side set.

          int64_t number_sides = decomp->side_sets[iss].ioss_count();
          // FIXME: Support-  number_distribution_factors = decomp->side_sets[iss].df_count();

          Ioss::Int64Vector element;
          Ioss::Int64Vector sides;

          if (!blockOmissions.empty() || !blockInclusions.empty()) {
            get_element_sides_lists(decomp, get_file_pointer(), id, int_byte_size_api(),
                                    number_sides, element, sides);
            Ioex::filter_element_list(get_region(), element, sides, true);
            number_sides = element.size();
            assert(element.size() == sides.size());
          }

          if (split_type == Ioss::SPLIT_BY_TOPOLOGIES && sideTopology.size() == 1) {
            // There is only one side type for all elements in the model
            topo_map[std::make_pair(sideTopology[0].first->name(), sideTopology[0].second)] =
                number_sides;
          }
          else if (split_type == Ioss::SPLIT_BY_DONT_SPLIT) {
            const Ioss::ElementTopology *mixed_topo = Ioss::ElementTopology::factory("unknown");
            topo_map[std::make_pair(std::string("unknown"), mixed_topo)] = number_sides;
          }
          else if (in_ss_map) {
            auto tokens = Ioss::tokenize(side_set_name, "_");
            assert(tokens.size() >= 4);
            // The sideset should have only a single topology which is
            // given by the sideset name...
            const Ioss::ElementTopology *side_topo =
                Ioss::ElementTopology::factory(tokens[tokens.size() - 2]);
            assert(side_topo != nullptr);
            const Ioss::ElementTopology *element_topo =
                Ioss::ElementTopology::factory(tokens[tokens.size() - 3], true);
            std::string name;
            if (element_topo != nullptr) {
              name = element_topo->name();
            }
            else {
              //                           -4   -3   -2     -1
              // Name is of the form name_block_id_sidetopo_id
              name = tokens[tokens.size() - 4] + "_" + tokens[tokens.size() - 3];
            }

            topo_map[std::make_pair(name, side_topo)] = number_sides;

            // We want the id to match the id on the sideset in this
            // case so that the generated name will match the current
            // name.  Instead of converting from string to int back to
            // string, we just set a variable to query later.
            sid = tokens[tokens.size() - 1];
          }
          else if (split_type == Ioss::SPLIT_BY_TOPOLOGIES) {
            // There are multiple side types in the model.
            // Iterate through the elements in the sideset, determine
            // their parent element block using the blocks element
            // topology and the side number, determine the side
            // type.

            for (auto &side_topo : sideTopology) {
              topo_map[std::make_pair(side_topo.first->name(), side_topo.second)] = 0;
              side_map[std::make_pair(side_topo.first->name(), side_topo.second)] = 0;
            }

            get_element_sides_lists(decomp, get_file_pointer(), id, int_byte_size_api(),
                                    number_sides, element, sides);
            Ioex::separate_surface_element_sides(element, sides, get_region(), topo_map, side_map,
                                                 split_type, side_set_name);
          }
          else if (split_type == Ioss::SPLIT_BY_ELEMENT_BLOCK) {
            // There are multiple side types in the model.  Iterate
            // through the elements in the sideset, determine their parent
            // element block using blocks element topology and the side
            // number, determine the side type.

            // Seed the topo_map map with <block->name, side_topo>
            // pairs so we are sure that all processors have the same
            // starting topo_map (size and order).
            const Ioss::ElementBlockContainer &element_blocks = get_region()->get_element_blocks();
            assert(Ioss::Utils::check_block_order(element_blocks));

            for (Ioss::ElementBlock *block : element_blocks) {
              if (!Ioss::Utils::block_is_omitted(block)) {
                const std::string           &name         = block->name();
                const Ioss::ElementTopology *common_ftopo = block->topology()->boundary_type(0);
                if (common_ftopo != nullptr) {
                  // All sides of this element block's topology have the same topology
                  topo_map[std::make_pair(name, common_ftopo)] = 0;
                  side_map[std::make_pair(name, common_ftopo)] = 0;
                }
                else {
                  // The sides have different topology, iterate over
                  // them and create an entry for the unique side
                  // topology types
                  int par_dim = block->topology()->parametric_dimension();
                  if (par_dim == 2 || par_dim == 3) {
                    int64_t my_side_count = block->topology()->number_boundaries();
                    for (int64_t ii = 0; ii < my_side_count; ii++) {
                      const Ioss::ElementTopology *topo = block->topology()->boundary_type(ii + 1);
                      topo_map[std::make_pair(name, topo)] = 0;
                      side_map[std::make_pair(name, topo)] = 0;
                    }
                  }
                }
              }
            }
            get_element_sides_lists(decomp, get_file_pointer(), id, int_byte_size_api(),
                                    number_sides, element, sides);
            Ioex::separate_surface_element_sides(element, sides, get_region(), topo_map, side_map,
                                                 split_type, side_set_name);
          }
        }

        // End of first step in splitting.  Check among all processors
        // to see which potential splits have sides in them...
        Ioss::Int64Vector global_side_counts(topo_map.size());
        {
          int64_t i = 0;
          for (auto &topo : topo_map) {
            global_side_counts[i++] = topo.second;
          }

          // If splitting by element block, also sync the side_map
          // information which specifies whether the sideset has
          // consistent sides for all elements. Only really used for
          // shells, but easier to just set the value on all surfaces
          // in the element block split case.
          if (side_map.size() == topo_map.size()) {
            global_side_counts.resize(topo_map.size() + side_map.size());

            for (auto &side : side_map) {
              global_side_counts[i++] = side.second;
            }
          }

          // See if any processor has non-zero count for the topo_map counts
          // For the side_map, need the max value.
          util().global_array_minmax(global_side_counts, Ioss::ParallelUtils::DO_MAX);
        }

        // Create Side Blocks

        int64_t i = 0;
        for (auto &topo : topo_map) {
          if (global_side_counts[i++] > 0) {
            const std::string            topo_or_block_name = topo.first.first;
            const Ioss::ElementTopology *side_topo          = topo.first.second;
            assert(side_topo != nullptr);
#if 0
            if (side_topo->parametric_dimension() == topology_dimension-1 ||
                split_type == Ioss::SPLIT_BY_DONT_SPLIT ) {
#else
            if (true) {
#endif
            int64_t my_side_count = topo.second;

            std::string side_block_name = "surface_" + topo_or_block_name + "_" + side_topo->name();
            if (split_type == Ioss::SPLIT_BY_DONT_SPLIT) {
              side_block_name = side_set_name;
            }
            else {
              if (db_has_name) {
                side_block_name =
                    side_set->name() + "_" + topo_or_block_name + "_" + side_topo->name();
              }
              else {
                if (sid.empty()) {
                  side_block_name = Ioss::Utils::encode_entity_name(side_block_name, id);
                }
                else {
                  side_block_name += "_";
                  side_block_name += sid;
                }
              }
            }

            Ioss::ElementBlock *block = nullptr;
            // Need to get elem_topo....
            const Ioss::ElementTopology *elem_topo = nullptr;
            if (split_type == Ioss::SPLIT_BY_TOPOLOGIES) {
              elem_topo = Ioss::ElementTopology::factory(topo_or_block_name);
            }
            else if (split_type == Ioss::SPLIT_BY_ELEMENT_BLOCK) {
              block = get_region()->get_element_block(topo_or_block_name);
              if (block == nullptr || Ioss::Utils::block_is_omitted(block)) {
                std::ostringstream errmsg;
                fmt::print(errmsg,
                           "INTERNAL ERROR: Could not find element block '{}'. Something is wrong "
                           "in the Ioex::ParallelDatabaseIO class. Please report.\n",
                           topo_or_block_name);
                IOSS_ERROR(errmsg);
              }
              elem_topo = block->topology();
            }
            if (split_type == Ioss::SPLIT_BY_DONT_SPLIT) {
              // Most likely this is "unknown", but can be a true
              // topology if there is only a single element block in
              // the model.
              elem_topo = Ioss::ElementTopology::factory(topo_or_block_name);
            }
            assert(elem_topo != nullptr);

            auto *side_block = new Ioss::SideBlock(this, side_block_name, side_topo->name(),
                                                   elem_topo->name(), my_side_count);
            assert(side_block != nullptr);
            side_block->property_add(Ioss::Property("id", id));
            side_block->property_add(Ioss::Property("guid", util().generate_guid(id)));
            side_set->add(side_block);

            // Note that all sideblocks within a specific
            // sideset might have the same id.

            // If splitting by element block, need to set the
            // element block member on this side block.
            if (split_type == Ioss::SPLIT_BY_ELEMENT_BLOCK) {
              side_block->set_parent_element_block(block);
            }

            // If we calculated whether the element side is
            // consistent for all sides in this block, then
            // tell the block which side it is, or that they are
            // inconsistent. If it wasn't calculated above, then it
            // will be calculated on the fly when/if requested.
            // This is to avoid reading the sideset bulk data in
            // cases where we don't need to read it, but if we are
            // already reading it (to split the sidesets), then use
            // the data when we have it.
            if (!side_map.empty()) {
              // Set a property indicating which element side
              // (1-based) all sides in this block are applied to.
              // If they are not all assigned to the same element
              // side, indicate this with a side equal to 0.
              //
              // (note: 'i' has already been incremented earlier in
              // the loop.  We need previous value here...)
              int side = global_side_counts[i - 1 + topo_map.size()];
              if (side == 999) {
                side = 0;
              }
              assert(side <= elem_topo->number_boundaries());
              side_block->set_consistent_side_number(side);
            }

            // Add an alias...
            get_region()->add_alias(side_block);

            if (split_type != Ioss::SPLIT_BY_DONT_SPLIT && side_set_name != "universal_sideset") {
              std::string storage = "Real[";
              storage += std::to_string(side_topo->number_nodes());
              storage += "]";
              side_block->field_add(Ioss::Field("distribution_factors", Ioss::Field::REAL, storage,
                                                Ioss::Field::MESH));
            }

            if (side_set_name == "universal_sideset") {
              side_block->field_add(Ioss::Field("side_ids", side_block->field_int_type(), "scalar",
                                                Ioss::Field::MESH));
            }

            int num_attr = 0;
            {
              int ierr = ex_get_attr_param(get_file_pointer(), EX_SIDE_SET, 1, &num_attr);
              if (ierr < 0) {
                Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
              }
            }
            // Add additional fields
            add_attribute_fields(side_block, num_attr, "");
            add_results_fields(side_block, iss);
          }
        }
      }
    }
  }
} // namespace Ioex

template <typename T>
void ParallelDatabaseIO::get_sets(ex_entity_type type, int64_t count, const std::string &base,
                                  const T * /* set_type */)
{
  // Attributes of a Xset are:
  // -- id
  // -- name
  // -- number of nodes
  // -- number of distribution factors (see next comment)
  // ----the #distribution factors should equal #Xs or 0, any
  //     other value does not make sense. If it is 0, then a substitute
  //     list will be created returning 1.0 for the factor

  // In a parallel execution, it is possible that a Xset will have
  // no Xs or distribution factors on a particular processor...

  // Get exodusII Xset metadata
  for (int64_t ins = 0; ins < count; ins++) {
    int64_t id = decomp->node_sets[ins].id();

    int num_attr = 0;
    int ierr     = ex_get_attr_param(get_file_pointer(), type, id, &num_attr);
    if (ierr < 0) {
      Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
    }

    bool        db_has_name = false;
    std::string Xset_name;
    std::string alias = Ioss::Utils::encode_entity_name(base + "list", id);
    if (ignore_database_names()) {
      Xset_name = alias;
    }
    else {
      Xset_name = Ioex::get_entity_name(get_file_pointer(), type, id, base + "list",
                                        maximumNameLength, db_has_name);
    }

    if (get_use_generic_canonical_name()) {
      std::swap(Xset_name, alias);
    }

    T *Xset = new T(this, Xset_name, decomp->node_sets[ins].ioss_count());
    Xset->property_add(Ioss::Property("id", id));
    Xset->property_add(Ioss::Property("guid", util().generate_guid(id)));
    if (db_has_name) {
      std::string *db_name = &Xset_name;
      if (get_use_generic_canonical_name()) {
        db_name = &alias;
      }
      Xset->property_add(Ioss::Property("db_name", *db_name));
    }
    get_region()->add(Xset);
    get_region()->add_alias(Xset_name, alias, Xset->type());
    get_region()->add_alias(Xset_name, Ioss::Utils::encode_entity_name(base + "set", id),
                            Xset->type());
    add_attribute_fields(Xset, num_attr, "");
    add_results_fields(Xset, ins);
  }
}

void ParallelDatabaseIO::get_nodesets()
{
  get_sets(EX_NODE_SET, m_groupCount[EX_NODE_SET], "node", (Ioss::NodeSet *)nullptr);
}

void ParallelDatabaseIO::get_edgesets()
{
  get_sets(EX_EDGE_SET, m_groupCount[EX_EDGE_SET], "edge", (Ioss::EdgeSet *)nullptr);
}

void ParallelDatabaseIO::get_facesets()
{
  get_sets(EX_FACE_SET, m_groupCount[EX_FACE_SET], "face", (Ioss::FaceSet *)nullptr);
}

void ParallelDatabaseIO::get_elemsets()
{
  get_sets(EX_ELEM_SET, m_groupCount[EX_ELEM_SET], "element", (Ioss::ElementSet *)nullptr);
}

void ParallelDatabaseIO::get_commsets()
{
  // Attributes of a commset are:
  // -- id (property)
  // -- name (property)
  // -- number of node--CPU pairs (field)

  // In a parallel execution, it is possible that a commset will have
  // no nodes on a particular processor...

  // If this is a serial execution, there will be no communication
  // nodesets, just return an empty container.

  if (isParallel) {

    // Create a single node commset
    Ioss::CommSet *commset =
        new Ioss::CommSet(this, "commset_node", "node", decomp->get_commset_node_size());
    commset->property_add(Ioss::Property("id", 1));
    commset->property_add(Ioss::Property("guid", util().generate_guid(1)));
    get_region()->add(commset);
  }
}

int64_t ParallelDatabaseIO::get_field_internal(const Ioss::Region *reg, const Ioss::Field &field,
                                               void *data, size_t data_size) const
{
  return Ioex::BaseDatabaseIO::get_field_internal(reg, field, data, data_size);
}

int64_t ParallelDatabaseIO::get_field_internal(const Ioss::NodeBlock *nb, const Ioss::Field &field,
                                               void *data, size_t data_size) const
{
  size_t num_to_get = field.verify(data_size);

#ifndef NDEBUG
  int64_t my_node_count = field.raw_count();
  assert(my_node_count == nodeCount);
#endif

  Ioss::Field::RoleType role = field.get_role();
  if (role == Ioss::Field::MESH) {
    if (field.get_name() == "mesh_model_coordinates_x" ||
        field.get_name() == "mesh_model_coordinates_y" ||
        field.get_name() == "mesh_model_coordinates_z" ||
        field.get_name() == "mesh_model_coordinates") {
      decomp->get_node_coordinates(get_file_pointer(), reinterpret_cast<double *>(data), field);
    }

    else if (field.get_name() == "ids") {
      // Map the local ids in this node block
      // (1...node_count) to global node ids.
      get_map(EX_NODE_BLOCK).map_implicit_data(data, field, num_to_get, 0);
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
        decomp->communicate_node_data(file_ids.data(), reinterpret_cast<int *>(data), 1);
      }
      else {
        std::vector<int64_t> file_ids(count);
        std::iota(file_ids.begin(), file_ids.end(), offset + 1);
        decomp->communicate_node_data(file_ids.data(), reinterpret_cast<int64_t *>(data), 1);
      }
    }

    else if (field.get_name() == "connectivity") {
      // Do nothing, just handles an idiosyncrasy of the GroupingEntity
    }
    else if (field.get_name() == "connectivity_raw") {
      // Do nothing, just handles an idiosyncrasy of the GroupingEntity
    }
    else if (field.get_name() == "node_connectivity_status") {
      compute_node_status();
      char *status = static_cast<char *>(data);
      std::copy(nodeConnectivityStatus.begin(), nodeConnectivityStatus.end(), status);
    }

    else if (field.get_name() == "owning_processor") {
      // If parallel, then set the "locally_owned" property on the nodeblocks.
      Ioss::CommSet *css = get_region()->get_commset("commset_node");
      int *idata         = static_cast<int *>(data); // Owning processor field is 4-byte int always
      for (int64_t i = 0; i < nodeCount; i++) {
        idata[i] = myProcessor;
      }

      if (int_byte_size_api() == 8) {
        // Cannot call:
        //    `css->get_field_data("entity_processor_raw", ent_proc);`
        // directly since it will cause a deadlock (in threaded code),
        // expand out into corresponding `get_field_internal` call.
        Ioss::Field          ep_field = css->get_field("entity_processor_raw");
        std::vector<int64_t> ent_proc(ep_field.raw_count() *
                                      ep_field.get_component_count(Ioss::Field::InOut::INPUT));
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
        Ioss::Field      ep_field = css->get_field("entity_processor_raw");
        std::vector<int> ent_proc(ep_field.raw_count() *
                                  ep_field.get_component_count(Ioss::Field::InOut::INPUT));
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
    // Check if the specified field exists on this node block.
    // Note that 'higher-order' storage types (e.g. SYM_TENSOR)
    // exist on the database as scalars with the appropriate
    // extensions.

    // Read in each component of the variable and transfer into
    // 'data'.  Need temporary storage area of size 'number of
    // nodes in this block.
    num_to_get = read_transient_field(m_variables[EX_NODE_BLOCK], field, nb, data);
  }
  else if (role == Ioss::Field::ATTRIBUTE) {
    num_to_get = read_attribute_field(field, nb, data);
  }
  return num_to_get;
}

int64_t ParallelDatabaseIO::get_field_internal(const Ioss::Blob *blob, const Ioss::Field &field,
                                               void *data, size_t data_size) const
{
  {
    Ioss::SerializeIO serializeIO__(this);

    size_t num_to_get = field.verify(data_size);
    if (num_to_get > 0) {

      Ioss::Field::RoleType role = field.get_role();
      if (role == Ioss::Field::MESH) {
        if (field.get_name() == "ids") {
          // Map the local ids in this node block
          // (1...node_count) to global node ids.
          //          get_map(EX_BLOB).map_implicit_data(data, field, num_to_get, 0);
        }

        else if (field.get_name() == "connectivity") {
          // Do nothing, just handles an idiosyncrasy of the GroupingEntity
        }
        else if (field.get_name() == "connectivity_raw") {
          // Do nothing, just handles an idiosyncrasy of the GroupingEntity
        }
        else {
          num_to_get = Ioss::Utils::field_warning(blob, field, "input");
        }
      }
      else if (role == Ioss::Field::TRANSIENT) {
        // Check if the specified field exists on this blob.
        // Note that 'higher-order' storage types (e.g. SYM_TENSOR)
        // exist on the database as scalars with the appropriate
        // extensions.

        // Read in each component of the variable and transfer into
        // 'data'.  Need temporary storage area of size 'number of
        // items in this blob.
        num_to_get = read_transient_field(m_variables[EX_BLOB], field, blob, data);
      }
      else if (role == Ioss::Field::ATTRIBUTE) {
        num_to_get = read_attribute_field(field, blob, data);
      }
      else if (role == Ioss::Field::REDUCTION) {
        get_reduction_field(field, blob, data);
      }
    }
    return num_to_get;
  }
}

int64_t ParallelDatabaseIO::get_field_internal(const Ioss::Assembly *assembly,
                                               const Ioss::Field &field, void *data,
                                               size_t data_size) const
{
  {
    Ioss::SerializeIO serializeIO__(this);

    size_t num_to_get = field.verify(data_size);
    if (num_to_get > 0) {

      Ioss::Field::RoleType role = field.get_role();
      if (role == Ioss::Field::MESH) {
        if (field.get_name() == "ids") {
          // Map the local ids in this node block
          // (1...node_count) to global node ids.
          //          get_map(EX_ASSEMBLY).map_implicit_data(data, field, num_to_get, 0);
        }

        else if (field.get_name() == "connectivity") {
          // Do nothing, just handles an idiosyncrasy of the GroupingEntity
        }
        else if (field.get_name() == "connectivity_raw") {
          // Do nothing, just handles an idiosyncrasy of the GroupingEntity
        }
        else {
          num_to_get = Ioss::Utils::field_warning(assembly, field, "input");
        }
      }
      else if (role == Ioss::Field::TRANSIENT) {
        // Check if the specified field exists on this assembly.
        // Note that 'higher-order' storage types (e.g. SYM_TENSOR)
        // exist on the database as scalars with the appropriate
        // extensions.

        // Read in each component of the variable and transfer into
        // 'data'.  Need temporary storage area of size 'number of
        // items in this assembly.
        num_to_get = read_transient_field(m_variables[EX_ASSEMBLY], field, assembly, data);
      }
      else if (role == Ioss::Field::ATTRIBUTE) {
        num_to_get = read_attribute_field(field, assembly, data);
      }
    }
    return num_to_get;
  }
}

int64_t ParallelDatabaseIO::get_field_internal(const Ioss::ElementBlock *eb,
                                               const Ioss::Field &field, void *data,
                                               size_t data_size) const
{

  size_t num_to_get = field.verify(data_size);

  int64_t               id               = Ioex::get_id(eb, &ids_);
  size_t                my_element_count = eb->entity_count();
  Ioss::Field::RoleType role             = field.get_role();

  if (role == Ioss::Field::MESH) {
    // Handle the MESH fields required for an ExodusII file model.
    // (The 'genesis' portion)

    if (field.get_name() == "connectivity" || field.get_name() == "connectivity_raw") {
      int element_nodes = eb->topology()->number_nodes();
      assert(field.get_component_count(Ioss::Field::InOut::INPUT) == element_nodes);

      int order = eb->get_property("iblk").get_int();
      // The connectivity is stored in a 1D array.
      // The element_node index varies fastet

      decomp->get_block_connectivity(get_file_pointer(), data, id, order, element_nodes);
      if (field.get_name() == "connectivity") {
        get_map(EX_NODE_BLOCK).map_data(data, field, num_to_get * element_nodes);
      }
    }
#if 0
        else if (field.get_name() == "connectivity_face") {
          int face_count = field.get_component_count(Ioss::Field::InOut::INPUT);

          // The connectivity is stored in a 1D array.
          // The element_face index varies fastest
          if (my_element_count > 0) {
            get_connectivity_data(get_file_pointer(), data, EX_ELEM_BLOCK, id, 2, int_byte_size_api());
            get_map(EX_FACE_BLOCK).map_data(data, field, num_to_get*face_count);
          }
        }
        else if (field.get_name() == "connectivity_edge") {
          int edge_count = field.get_component_count(Ioss::Field::InOut::INPUT);

          // The connectivity is stored in a 1D array.
          // The element_edge index varies fastest
          if (my_element_count > 0) {
            get_connectivity_data(get_file_pointer(), data, EX_ELEM_BLOCK, id, 1, int_byte_size_api());
            get_map(EX_EDGE_BLOCK).map_data(data, field, num_to_get*edge_count);
          }
        }
#endif
    else if (field.get_name() == "ids" || field.get_name() == "implicit_ids") {
      // Map the local ids in this element block
      // (eb_offset+1...eb_offset+1+my_element_count) to global element ids.
      get_map(EX_ELEM_BLOCK).map_implicit_data(data, field, num_to_get, eb->get_offset());
    }
  }
  else if (role == Ioss::Field::MAP) {
    int    component_count = field.get_component_count(Ioss::Field::InOut::INPUT);
    size_t eb_offset       = eb->get_offset();

    if (component_count == 1) {
      // Single component -- can put data directly into return `data`;
      decomp->get_user_map(get_file_pointer(), EX_ELEM_MAP, id, field.get_index(), eb_offset,
                           my_element_count, data);
    }
    else {
      // Multi-component -- need read a component at a time and interleave into return `data`
      if (field.is_type(Ioss::Field::INTEGER)) {
        Ioss::IntVector component(my_element_count);
        auto           *data32 = reinterpret_cast<int *>(data);
        for (int comp = 0; comp < component_count; comp++) {
          decomp->get_user_map(get_file_pointer(), EX_ELEM_MAP, id, field.get_index() + comp,
                               eb_offset, my_element_count, component.data());
          int index = comp;
          for (size_t i = 0; i < my_element_count; i++) {
            data32[index] = component[i];
            index += component_count;
          }
        }
      }
      else {
        Ioss::Int64Vector component(my_element_count);
        auto             *data64 = reinterpret_cast<int64_t *>(data);
        for (int comp = 0; comp < component_count; comp++) {
          decomp->get_user_map(get_file_pointer(), EX_ELEM_MAP, id, field.get_index() + comp,
                               eb_offset, my_element_count, component.data());

          int index = comp;
          for (size_t i = 0; i < my_element_count; i++) {
            data64[index] = component[i];
            index += component_count;
          }
        }
      }
    }
#if 0
    else if (field.get_name() == "skin") {
      // This is (currently) for the skinned body. It maps the
      // side element on the skin to the original element/local
      // side number.  It is a two component field, the first
      // component is the global id of the underlying element in
      // the initial mesh and its local side number (1-based).

      if (field.is_type(Ioss::Field::INTEGER)) {
        Ioss::IntVector element(my_element_count);
        Ioss::IntVector side(my_element_count);
        int            *el_side = reinterpret_cast<int *>(data);

        // FIX: Hardwired map ids....
        size_t eb_offset = eb->get_offset();
        assert(1 == 0 && "Unimplemented FIXME");
        ex_get_partial_num_map(get_file_pointer(), EX_ELEM_MAP, 1, eb_offset + 1, my_element_count,
                               element.data()); // FIXME
        ex_get_partial_num_map(get_file_pointer(), EX_ELEM_MAP, 2, eb_offset + 1, my_element_count,
                               side.data()); // FIXME

        int index = 0;
        for (size_t i = 0; i < my_element_count; i++) {
          el_side[index++] = element[i];
          el_side[index++] = side[i];
        }
      }
      else {
        Ioss::Int64Vector element(my_element_count);
        Ioss::Int64Vector side(my_element_count);
        int64_t          *el_side = reinterpret_cast<int64_t *>(data);

        // FIX: Hardwired map ids....
        size_t eb_offset = eb->get_offset();
        assert(1 == 0 && "Unimplemented FIXME");
        ex_get_partial_num_map(get_file_pointer(), EX_ELEM_MAP, 1, eb_offset + 1, my_element_count,
                               element.data()); // FIXME
        ex_get_partial_num_map(get_file_pointer(), EX_ELEM_MAP, 2, eb_offset + 1, my_element_count,
                               side.data()); // FIXME

        size_t index = 0;
        for (size_t i = 0; i < my_element_count; i++) {
          el_side[index++] = element[i];
          el_side[index++] = side[i];
        }
      }
    }
    else {
      num_to_get = Ioss::Utils::field_warning(eb, field, "input");
    }
#endif
  }
  else if (role == Ioss::Field::ATTRIBUTE) {
    num_to_get = read_attribute_field(field, eb, data);
  }
  else if (role == Ioss::Field::TRANSIENT) {
    // Check if the specified field exists on this element block.
    // Note that 'higher-order' storage types (e.g. SYM_TENSOR)
    // exist on the database as scalars with the appropriate
    // extensions.

    // Read in each component of the variable and transfer into
    // 'data'.  Need temporary storage area of size 'number of
    // elements in this block.
    num_to_get = read_transient_field(m_variables[EX_ELEM_BLOCK], field, eb, data);
  }
  else if (role == Ioss::Field::REDUCTION) {
    num_to_get = Ioss::Utils::field_warning(eb, field, "input reduction");
  }
  return num_to_get;
}

int64_t ParallelDatabaseIO::get_field_internal(const Ioss::FaceBlock *eb, const Ioss::Field &field,
                                               void *data, size_t data_size) const
{
  size_t num_to_get = field.verify(data_size);

  int64_t               id            = Ioex::get_id(eb, &ids_);
  size_t                my_face_count = eb->entity_count();
  Ioss::Field::RoleType role          = field.get_role();

  if (role == Ioss::Field::MESH) {
    // Handle the MESH fields required for an ExodusII file model.
    // (The 'genesis' portion)

    if (field.get_name() == "connectivity") {
      int face_nodes = eb->topology()->number_nodes();
      assert(field.get_component_count(Ioss::Field::InOut::INPUT) == face_nodes);

      // The connectivity is stored in a 1D array.
      // The face_node index varies fastet
      if (my_face_count > 0) {
        get_connectivity_data(get_file_pointer(), data, EX_FACE_BLOCK, id, 0, int_byte_size_api());
        get_map(EX_NODE_BLOCK).map_data(data, field, num_to_get * face_nodes);
      }
    }
    else if (field.get_name() == "connectivity_edge") {
      int edge_count = field.get_component_count(Ioss::Field::InOut::INPUT);

      // The connectivity is stored in a 1D array.
      // The face_edge index varies fastest
      if (my_face_count > 0) {
        get_connectivity_data(get_file_pointer(), data, EX_FACE_BLOCK, id, 1, int_byte_size_api());
        get_map(EX_EDGE_BLOCK).map_data(data, field, num_to_get * edge_count);
      }
    }
    else if (field.get_name() == "connectivity_raw") {
      // "connectivity_raw" has nodes in local id space (1-based)
      assert(field.get_component_count(Ioss::Field::InOut::INPUT) ==
             eb->topology()->number_nodes());

      // The connectivity is stored in a 1D array.
      // The face_node index varies fastet
      if (my_face_count > 0) {
        get_connectivity_data(get_file_pointer(), data, EX_FACE_BLOCK, id, 0, int_byte_size_api());
      }
    }
    else if (field.get_name() == "ids") {
      // Map the local ids in this face block
      // (eb_offset+1...eb_offset+1+my_face_count) to global face ids.
      get_map(EX_FACE_BLOCK).map_implicit_data(data, field, num_to_get, eb->get_offset());
    }
    else {
      num_to_get = Ioss::Utils::field_warning(eb, field, "input");
    }
  }
  else if (role == Ioss::Field::ATTRIBUTE) {
    num_to_get = read_attribute_field(field, eb, data);
  }
  else if (role == Ioss::Field::TRANSIENT) {
    // Check if the specified field exists on this element block.
    // Note that 'higher-order' storage types (e.g. SYM_TENSOR)
    // exist on the database as scalars with the appropriate
    // extensions.

    // Read in each component of the variable and transfer into
    // 'data'.  Need temporary storage area of size 'number of
    // elements in this block.
    num_to_get = read_transient_field(m_variables[EX_FACE_BLOCK], field, eb, data);
  }
  else if (role == Ioss::Field::REDUCTION) {
    num_to_get = Ioss::Utils::field_warning(eb, field, "input reduction");
  }
  return num_to_get;
}

int64_t ParallelDatabaseIO::get_field_internal(const Ioss::EdgeBlock *eb, const Ioss::Field &field,
                                               void *data, size_t data_size) const
{
  size_t num_to_get = field.verify(data_size);

  int64_t               id            = Ioex::get_id(eb, &ids_);
  int64_t               my_edge_count = eb->entity_count();
  Ioss::Field::RoleType role          = field.get_role();

  if (role == Ioss::Field::MESH) {
    // Handle the MESH fields required for an ExodusII file model.
    // (The 'genesis' portion)

    if (field.get_name() == "connectivity") {
      int edge_nodes = eb->topology()->number_nodes();
      assert(field.get_component_count(Ioss::Field::InOut::INPUT) == edge_nodes);

      // The connectivity is stored in a 1D array.
      // The edge_node index varies fastet
      if (my_edge_count > 0) {
        get_connectivity_data(get_file_pointer(), data, EX_EDGE_BLOCK, id, 0, int_byte_size_api());
        get_map(EX_NODE_BLOCK).map_data(data, field, num_to_get * edge_nodes);
      }
    }
    else if (field.get_name() == "connectivity_raw") {
      // "connectivity_raw" has nodes in local id space (1-based)
      assert(field.get_component_count(Ioss::Field::InOut::INPUT) ==
             eb->topology()->number_nodes());

      // The connectivity is stored in a 1D array.
      // The edge_node index varies fastet
      if (my_edge_count > 0) {
        get_connectivity_data(get_file_pointer(), data, EX_EDGE_BLOCK, id, 0, int_byte_size_api());
      }
    }
    else if (field.get_name() == "ids") {
      // Map the local ids in this edge block
      // (eb_offset+1...eb_offset+1+my_edge_count) to global edge ids.
      get_map(EX_EDGE_BLOCK).map_implicit_data(data, field, num_to_get, eb->get_offset());
    }
    else {
      num_to_get = Ioss::Utils::field_warning(eb, field, "input");
    }
  }
  else if (role == Ioss::Field::ATTRIBUTE) {
    num_to_get = read_attribute_field(field, eb, data);
  }
  else if (role == Ioss::Field::TRANSIENT) {
    // Check if the specified field exists on this element block.
    // Note that 'higher-order' storage types (e.g. SYM_TENSOR)
    // exist on the database as scalars with the appropriate
    // extensions.

    // Read in each component of the variable and transfer into
    // 'data'.  Need temporary storage area of size 'number of
    // elements in this block.
    num_to_get = read_transient_field(m_variables[EX_EDGE_BLOCK], field, eb, data);
  }
  else if (role == Ioss::Field::REDUCTION) {
    num_to_get = Ioss::Utils::field_warning(eb, field, "input reduction");
  }
  return num_to_get;
}

int64_t ParallelDatabaseIO::get_Xset_field_internal(const Ioss::EntitySet *ns,
                                                    const Ioss::Field &field, void *data,
                                                    size_t data_size) const
{
  int                   ierr;
  size_t                num_to_get = field.verify(data_size);
  Ioss::Field::RoleType role       = field.get_role();

  // Find corresponding set in file decomp class...
  if (role == Ioss::Field::MESH) {
    ex_entity_type type = Ioex::map_exodus_type(ns->type());
    int64_t        id   = Ioex::get_id(ns, &ids_);

    if (field.get_name() == "ids" || field.get_name() == "ids_raw") {
      if (field.get_type() == Ioss::Field::INTEGER) {
        ierr = decomp->get_set_mesh_var(get_file_pointer(), EX_NODE_SET, id, field,
                                        static_cast<int *>(data));
      }
      else {
        ierr = decomp->get_set_mesh_var(get_file_pointer(), EX_NODE_SET, id, field,
                                        static_cast<int64_t *>(data));
      }
      if (ierr < 0) {
        Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
      }

      if (field.get_name() == "ids") {
        // Convert the local node ids to global ids
        get_map(EX_NODE_BLOCK).map_data(data, field, num_to_get);
      }
    }
    else if (field.get_name() == "orientation") {
      if (field.get_type() == Ioss::Field::INTEGER) {
        ierr = decomp->get_set_mesh_var(get_file_pointer(), EX_NODE_SET, id, field,
                                        static_cast<int *>(data));
      }
      else {
        ierr = decomp->get_set_mesh_var(get_file_pointer(), EX_NODE_SET, id, field,
                                        static_cast<int64_t *>(data));
      }
      if (ierr < 0) {
        Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
      }
    }
    else if (field.get_name() == "distribution_factors") {
      ierr = decomp->get_set_mesh_double(get_file_pointer(), EX_NODE_SET, id, field,
                                         static_cast<double *>(data));
      if (ierr < 0) {
        Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
      }
    }
  }
  else if (role == Ioss::Field::ATTRIBUTE) {
    num_to_get = read_attribute_field(field, ns, data);
  }
  else if (role == Ioss::Field::TRANSIENT) {
    // Check if the specified field exists on this node block.
    // Note that 'higher-order' storage types (e.g. SYM_TENSOR)
    // exist on the database as scalars with the appropriate
    // extensions.

    // Read in each component of the variable and transfer into
    // 'data'.  Need temporary storage area of size 'number of
    // nodes in this block.
    ex_entity_type type = Ioex::map_exodus_type(ns->type());
    num_to_get          = read_transient_field(m_variables[type], field, ns, data);
  }
  return num_to_get;
}

int64_t ParallelDatabaseIO::get_field_internal(const Ioss::NodeSet *ns, const Ioss::Field &field,
                                               void *data, size_t data_size) const
{
  return get_Xset_field_internal(ns, field, data, data_size);
}

int64_t ParallelDatabaseIO::get_field_internal(const Ioss::EdgeSet *ns, const Ioss::Field &field,
                                               void *data, size_t data_size) const
{
  return get_Xset_field_internal(ns, field, data, data_size);
}

int64_t ParallelDatabaseIO::get_field_internal(const Ioss::FaceSet *ns, const Ioss::Field &field,
                                               void *data, size_t data_size) const
{
  return get_Xset_field_internal(ns, field, data, data_size);
}

int64_t ParallelDatabaseIO::get_field_internal(const Ioss::ElementSet *ns, const Ioss::Field &field,
                                               void *data, size_t data_size) const
{
  return get_Xset_field_internal(ns, field, data, data_size);
}

int64_t ParallelDatabaseIO::get_field_internal(const Ioss::SideSet *ss, const Ioss::Field &field,
                                               void *data, size_t data_size) const
{
  size_t num_to_get = field.verify(data_size);
  if (field.get_name() == "ids") {
    // Do nothing, just handles an idiosyncrasy of the GroupingEntity
    // However, make sure that the caller gets a consistent answer, i.e., don't leave the buffer
    // full of junk
    memset(data, 0x00, data_size);
  }
  else {
    num_to_get = Ioss::Utils::field_warning(ss, field, "input");
  }
  return num_to_get;
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
      const Ioss::MapContainer &map = get_map(EX_NODE_BLOCK).map();
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

int64_t ParallelDatabaseIO::get_field_internal(const Ioss::SideBlock *sb, const Ioss::Field &field,
                                               void *data, size_t data_size) const
{
  int64_t num_to_get = field.verify(data_size);
  int     ierr       = 0;

  int64_t id           = Ioex::get_id(sb, &ids_);
  int64_t entity_count = sb->entity_count();
  if (num_to_get != entity_count) {
    std::ostringstream errmsg;
    fmt::print(errmsg, "ERROR: Partial field input not yet implemented for side blocks");
    IOSS_ERROR(errmsg);
  }

  auto &set = decomp->get_decomp_set(EX_SIDE_SET, id);

  int64_t number_sides                = set.ioss_count();
  int64_t number_distribution_factors = set.df_count();

  Ioss::Field::RoleType role = field.get_role();
  if (role == Ioss::Field::MESH) {

    // In exodusII, we may have split the sideset into multiple
    // side blocks if there are multiple side topologies in the
    // sideset.  Because of this, the passed in 'data' may not be
    // large enough to hold the data residing in the sideset and we
    // may need to allocate a temporary array...  This can be checked
    // by comparing the size of the sideset with the 'side_count' of
    // the side block.

    // Get size of data stored on the file...
    // FIX 64: FIX THIS -- STORING INT IN DOUBLE WON'T WORK
    if (field.get_name() == "side_ids" && sb->name() == "universal_sideset") {
      // The side ids are being stored as the distribution factor
      // field on the universal sideset.  There should be no other
      // side sets that request this field...  (Eventually,
      // create an id field to store this info.

      if (number_distribution_factors == num_to_get) {
        std::vector<double> real_ids(num_to_get);
        Ioss::Field df_field("distribution_factor", Ioss::Field::REAL, "scalar", Ioss::Field::MESH,
                             num_to_get);
        decomp->get_set_mesh_double(get_file_pointer(), EX_SIDE_SET, id, df_field, real_ids.data());

        if (field.get_type() == Ioss::Field::INTEGER) {
          // Need to convert 'double' to 'int' for Sierra use...
          int *ids = static_cast<int *>(data);
          for (int64_t i = 0; i < num_to_get; i++) {
            ids[i] = static_cast<int>(real_ids[i]);
          }
        }
        else {
          // Need to convert 'double' to 'int' for Sierra use...
          int64_t *ids = static_cast<int64_t *>(data);
          for (int64_t i = 0; i < num_to_get; i++) {
            ids[i] = static_cast<int64_t>(real_ids[i]);
          }
        }
      }
    }

    else if (field.get_name() == "side_ids") {
    }

    else if (field.get_name() == "ids") {
      // In exodusII, the 'side set' is stored as a sideset.  A
      // sideset has a list of elements and a corresponding local
      // element side (1-based) The side id is: side_id =
      // 10*element_id + local_side_number This assumes that all
      // sides in a sideset are boundary sides.  Since we
      // only have a single array, we need to allocate an extra array
      // to store all of the data.  Note also that the element_id is
      // the global id but only the local id is stored so we need to
      // map from local_to_global prior to generating the side id...

      Ioss::Field       el_side = sb->get_field("element_side");
      std::vector<char> element_side(2 * number_sides * int_byte_size_api());
      get_field_internal(sb, el_side, element_side.data(), element_side.size());

      // At this point, have the 'element_side' data containing
      // the global element ids and the sides...  Iterate
      // through to generate the ids...
      if (int_byte_size_api() == 4) {
        int64_t int_max = std::numeric_limits<int>::max();
        int    *ids     = static_cast<int *>(data);
        int    *els     = reinterpret_cast<int *>(element_side.data());
        size_t  idx     = 0;
        for (int64_t iel = 0; iel < 2 * entity_count; iel += 2) {
          int64_t new_id = static_cast<int64_t>(10) * els[iel] + els[iel + 1];
          if (new_id > int_max) {
            std::ostringstream errmsg;
            fmt::print(errmsg,
                       "ERROR: accessing the sideset field 'ids'\n"
                       "\t\thas exceeded the integer bounds for entity {}, local side id {}.\n"
                       "\t\tTry using 64-bit mode to read the file '{}'.\n",
                       els[iel], els[iel + 1], get_filename());
            IOSS_ERROR(errmsg);
          }

          ids[idx++] = static_cast<int>(new_id);
        }
      }
      else {
        int64_t *ids = static_cast<int64_t *>(data);
        int64_t *els = reinterpret_cast<int64_t *>(element_side.data());
        size_t   idx = 0;
        for (int64_t iel = 0; iel < 2 * entity_count; iel += 2) {
          int64_t new_id = 10 * els[iel] + els[iel + 1];
          ids[idx++]     = new_id;
        }
      }
    }
    else if (field.get_name() == "element_side" || field.get_name() == "element_side_raw") {
      // In exodusII, the 'side set' is stored as a sideset.  A sideset
      // has a list of elements and a corresponding local element side
      // (1-based)

      // Since we only have a single array, we need to allocate an extra
      // array to store all of the data.  Note also that the element_id
      // is the global id but only the local id is stored so we need to
      // map from local_to_global prior to generating the side  id...

      // Get the element number map (1-based)...
      const Ioss::MapContainer &map = get_map(EX_ELEM_BLOCK).map();

      // Allocate space for local side number and element numbers
      // numbers.

      // See if edges or faces...
      int64_t side_offset = Ioss::Utils::get_side_offset(sb);

      if (sb->owner()->block_count() == 1 && number_sides == entity_count) {

        if (int_byte_size_api() == 4) {
          int *element_side = static_cast<int *>(data);
          decomp->get_set_mesh_var(get_file_pointer(), EX_SIDE_SET, id, field, element_side);
          for (int64_t iel = 1; iel < 2 * entity_count; iel += 2) {
            element_side[iel] = element_side[iel] - side_offset;
          }
        }
        else {
          int64_t *element_side = static_cast<int64_t *>(data);
          decomp->get_set_mesh_var(get_file_pointer(), EX_SIDE_SET, id, field, element_side);
          for (int64_t iel = 1; iel < 2 * entity_count; iel += 2) {
            element_side[iel] = element_side[iel] - side_offset;
          }
        }
      }
      else {
        // Need a larger vector to get the entire sideset and then filter down to the correct
        // size...
        std::vector<char> element(number_sides * int_byte_size_api());
        std::vector<char> sides(number_sides * int_byte_size_api());
        if (int_byte_size_api() == 4) {
          Ioss::Field elem_field("ids", Ioss::Field::INTEGER, "scalar", Ioss::Field::MESH,
                                 number_sides);
          Ioss::Field side_field("sides", Ioss::Field::INTEGER, "scalar", Ioss::Field::MESH,
                                 number_sides);
          decomp->get_set_mesh_var(get_file_pointer(), EX_SIDE_SET, id, elem_field,
                                   reinterpret_cast<int *>(element.data()));
          decomp->get_set_mesh_var(get_file_pointer(), EX_SIDE_SET, id, side_field,
                                   reinterpret_cast<int *>(sides.data()));
        }
        else {
          Ioss::Field elem_field("ids", Ioss::Field::INT64, "scalar", Ioss::Field::MESH,
                                 number_sides);
          Ioss::Field side_field("sides", Ioss::Field::INT64, "scalar", Ioss::Field::MESH,
                                 number_sides);
          decomp->get_set_mesh_var(get_file_pointer(), EX_SIDE_SET, id, elem_field,
                                   reinterpret_cast<int64_t *>(element.data()));
          decomp->get_set_mesh_var(get_file_pointer(), EX_SIDE_SET, id, side_field,
                                   reinterpret_cast<int64_t *>(sides.data()));
        }

        Ioss::IntVector is_valid_side;
        Ioss::Utils::calculate_sideblock_membership(is_valid_side, sb, int_byte_size_api(),
                                                    element.data(), sides.data(), number_sides,
                                                    get_region());

        int64_t index = 0;
        if (int_byte_size_api() == 4) {
          int *element_side = static_cast<int *>(data);
          int *element32    = reinterpret_cast<int *>(element.data());
          int *sides32      = reinterpret_cast<int *>(sides.data());
          for (int64_t iel = 0; iel < number_sides; iel++) {
            if (is_valid_side[iel] == 1) {
              // This side  belongs in the side block
              element_side[index++] = element32[iel];
              element_side[index++] = sides32[iel] - side_offset;
            }
          }
        }
        else {
          int64_t *element_side = static_cast<int64_t *>(data);
          int64_t *element64    = reinterpret_cast<int64_t *>(element.data());
          int64_t *sides64      = reinterpret_cast<int64_t *>(sides.data());
          for (int64_t iel = 0; iel < number_sides; iel++) {
            if (is_valid_side[iel] == 1) {
              // This side  belongs in the side block
              element_side[index++] = element64[iel];
              element_side[index++] = sides64[iel] - side_offset;
            }
          }
        }
        assert(index / 2 == entity_count);
      }
      if (field.get_name() == "element_side") {
        if (int_byte_size_api() == 4) {
          int *element_side = static_cast<int *>(data);
          for (int64_t iel = 0; iel < 2 * entity_count; iel += 2) {
            element_side[iel] = map[element_side[iel]];
          }
        }
        else {
          int64_t *element_side = static_cast<int64_t *>(data);
          for (int64_t iel = 0; iel < 2 * entity_count; iel += 2) {
            element_side[iel] = map[element_side[iel]];
          }
        }
      }
    }
    else if (field.get_name() == "connectivity") {
      // The side  connectivity needs to be generated 'on-the-fly' from
      // the element number and local side  of that element. A sideset
      // can span multiple element blocks, and contain multiple side
      // types; the side block contains side of similar topology.
      ierr = get_side_connectivity(sb, id, entity_count, data, true);
      if (ierr < 0) {
        Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
      }
    }
    else if (field.get_name() == "connectivity_raw") {
      // The side  connectivity needs to be generated 'on-the-fly' from
      // the element number and local side  of that element. A sideset
      // can span multiple element blocks, and contain multiple side
      // types; the side block contains side of similar topology.
      ierr = get_side_connectivity(sb, id, entity_count, data, false);
      if (ierr < 0) {
        Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
      }
    }
    else if (field.get_name() == "distribution_factors") {
      ierr = get_side_distributions(sb, id, entity_count, static_cast<double *>(data),
                                    data_size / sizeof(double));
      if (ierr < 0) {
        Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
      }
    }
    else {
      num_to_get = Ioss::Utils::field_warning(sb, field, "input");
    }
  }
  else if (role == Ioss::Field::TRANSIENT) {
    // Check if the specified field exists on this block.
    // Note that 'higher-order' storage types (e.g. SYM_TENSOR)
    // exist on the database as scalars with the appropriate
    // extensions.

    if (sb->owner()->block_count() == 1 && number_sides == entity_count) {
      num_to_get = read_transient_field(m_variables[EX_SIDE_SET], field, sb, data);
    }
    else {
      // Need to read all values for the specified field and then
      // filter down to the elements actually in this side block.

      Ioss::IntVector is_valid_side;
      // Need a larger vector to get the entire sideset and then filter down to the correct size...
      std::vector<char> element(number_sides * int_byte_size_api());
      std::vector<char> sides(number_sides * int_byte_size_api());
      if (int_byte_size_api() == 4) {
        Ioss::Field elem_field("ids", Ioss::Field::INTEGER, "scalar", Ioss::Field::MESH,
                               number_sides);
        Ioss::Field side_field("sides", Ioss::Field::INTEGER, "scalar", Ioss::Field::MESH,
                               number_sides);
        decomp->get_set_mesh_var(get_file_pointer(), EX_SIDE_SET, id, elem_field,
                                 reinterpret_cast<int *>(element.data()));
        decomp->get_set_mesh_var(get_file_pointer(), EX_SIDE_SET, id, side_field,
                                 reinterpret_cast<int *>(sides.data()));
      }
      else {
        Ioss::Field elem_field("ids", Ioss::Field::INT64, "scalar", Ioss::Field::MESH,
                               number_sides);
        Ioss::Field side_field("sides", Ioss::Field::INT64, "scalar", Ioss::Field::MESH,
                               number_sides);
        decomp->get_set_mesh_var(get_file_pointer(), EX_SIDE_SET, id, elem_field,
                                 reinterpret_cast<int64_t *>(element.data()));
        decomp->get_set_mesh_var(get_file_pointer(), EX_SIDE_SET, id, side_field,
                                 reinterpret_cast<int64_t *>(sides.data()));
      }
      Ioss::Utils::calculate_sideblock_membership(is_valid_side, sb, int_byte_size_api(),
                                                  element.data(), sides.data(), number_sides,
                                                  get_region());

      num_to_get = read_ss_transient_field(field, id, data, is_valid_side);
    }
  }
  return num_to_get;
}

int64_t ParallelDatabaseIO::write_attribute_field(const Ioss::Field          &field,
                                                  const Ioss::GroupingEntity *ge, void *data) const
{
  std::string att_name   = ge->name() + SEP() + field.get_name();
  int64_t     num_entity = ge->entity_count();
  int64_t     offset     = field.get_index();

  int64_t id = Ioex::get_id(ge, &ids_);
  assert(offset > 0);
  assert(offset - 1 + field.get_component_count(Ioss::Field::InOut::OUTPUT) <=
         ge->get_property("attribute_count").get_int());

  size_t  proc_offset = ge->get_optional_property("_processor_offset", 0);
  int64_t file_count  = ge->get_optional_property("locally_owned_count", num_entity);

  Ioss::Field::BasicType ioss_type = field.get_type();
  assert(ioss_type == Ioss::Field::REAL || ioss_type == Ioss::Field::INTEGER ||
         ioss_type == Ioss::Field::INT64);

  if (ioss_type == Ioss::Field::INT64) {
    Ioss::Utils::check_int_to_real_overflow(field, (int64_t *)data, num_entity);
  }

  int comp_count = field.get_component_count(Ioss::Field::InOut::OUTPUT);

  ex_entity_type type = Ioex::map_exodus_type(ge->type());
  if (type == EX_NODAL) {
    for (int i = 0; i < comp_count; i++) {
      std::vector<double> file_data;
      file_data.reserve(file_count);
      check_node_owning_processor_data(nodeOwningProcessor, file_count);
      if (ioss_type == Ioss::Field::REAL) {
        filter_owned_nodes(nodeOwningProcessor, myProcessor, static_cast<double *>(data), file_data,
                           i, comp_count);
      }
      else if (ioss_type == Ioss::Field::INTEGER) {
        filter_owned_nodes(nodeOwningProcessor, myProcessor, static_cast<int *>(data), file_data, i,
                           comp_count);
      }
      else if (ioss_type == Ioss::Field::INT64) {
        filter_owned_nodes(nodeOwningProcessor, myProcessor, static_cast<int64_t *>(data),
                           file_data, i, comp_count);
      }
      int ierr = ex_put_partial_one_attr(get_file_pointer(), type, id, proc_offset + 1, file_count,
                                         offset + i, file_data.data());
      if (ierr < 0) {
        Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
      }
    }
  }
  else if (type == EX_NODE_SET) {
    for (int i = 0; i < comp_count; i++) {
      std::vector<double> file_data;
      file_data.reserve(file_count);
      if (ioss_type == Ioss::Field::REAL) {
        map_nodeset_data(nodesetOwnedNodes[ge], static_cast<double *>(data), file_data, i,
                         comp_count);
      }
      else if (ioss_type == Ioss::Field::INTEGER) {
        map_nodeset_data(nodesetOwnedNodes[ge], static_cast<int *>(data), file_data, i, comp_count);
      }
      else if (ioss_type == Ioss::Field::INT64) {
        map_nodeset_data(nodesetOwnedNodes[ge], static_cast<int64_t *>(data), file_data, i,
                         comp_count);
      }

      int ierr = ex_put_partial_one_attr(get_file_pointer(), type, id, proc_offset + 1, file_count,
                                         offset + i, file_data.data());
      if (ierr < 0) {
        Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
      }
    }
  }
  else {
    assert(file_count == num_entity);
    std::vector<double> file_data(file_count);
    for (int i = 0; i < comp_count; i++) {
      if (ioss_type == Ioss::Field::REAL) {
        extract_data(file_data, static_cast<double *>(data), num_entity, i, comp_count);
      }
      else if (ioss_type == Ioss::Field::INTEGER) {
        extract_data(file_data, static_cast<int *>(data), num_entity, i, comp_count);
      }
      else if (ioss_type == Ioss::Field::INT64) {
        extract_data(file_data, static_cast<int64_t *>(data), num_entity, i, comp_count);
      }

      int ierr = ex_put_partial_one_attr(get_file_pointer(), type, id, proc_offset + 1, file_count,
                                         offset + i, file_data.data());
      if (ierr < 0) {
        Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
      }
    }
  }
  return num_entity;
}

int64_t ParallelDatabaseIO::read_attribute_field(const Ioss::Field          &field,
                                                 const Ioss::GroupingEntity *ge, void *data) const
{
  int64_t num_entity = ge->entity_count();

  int     attribute_count = ge->get_property("attribute_count").get_int();
  int64_t id              = Ioex::get_id(ge, &ids_);

  Ioss::Field::BasicType ioss_type = field.get_type();
  if (ioss_type == Ioss::Field::INTEGER || ioss_type == Ioss::Field::INT64) {
    std::ostringstream errmsg;
    fmt::print(errmsg, "INTERNAL ERROR: Integer attribute fields are not yet handled for read. "
                       "Please report.\n");
    IOSS_ERROR(errmsg);
  }

  std::string    att_name = ge->name() + SEP() + field.get_name();
  ex_entity_type type     = Ioex::map_exodus_type(ge->type());
  int64_t        offset   = field.get_index();
  assert(offset - 1 + field.get_component_count(Ioss::Field::InOut::INPUT) <= attribute_count);
  if (offset == 1 && field.get_component_count(Ioss::Field::InOut::INPUT) == attribute_count) {
    // Read all attributes in one big chunk...
    int ierr = decomp->get_attr(get_file_pointer(), type, id, attribute_count,
                                static_cast<double *>(data));
    if (ierr < 0) {
      Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
    }
  }
  else {
    // Read a subset of the attributes.  If scalar, read one;
    // if higher-order (vector3d, ..) read each component and
    // put into correct location...
    if (field.get_component_count(Ioss::Field::InOut::INPUT) == 1) {
      int ierr =
          decomp->get_one_attr(get_file_pointer(), type, id, offset, static_cast<double *>(data));
      if (ierr < 0) {
        Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
      }
    }
    else {
      // Multi-component...
      // Need a local memory space to read data into and
      // then push that into the user-supplied data block...
      std::vector<double> local_data(num_entity);
      int                 comp_count = field.get_component_count(Ioss::Field::InOut::INPUT);
      double             *rdata      = static_cast<double *>(data);
      for (int i = 0; i < comp_count; i++) {
        int ierr =
            decomp->get_one_attr(get_file_pointer(), type, id, offset + i, local_data.data());
        if (ierr < 0) {
          Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
        }

        size_t k = i;
        for (int64_t j = 0; j < num_entity; j++) {
          rdata[k] = local_data[j];
          k += comp_count;
        }
      }
    }
  }
  return num_entity;
}

int64_t ParallelDatabaseIO::read_transient_field(const Ioex::VariableNameMap &variables,
                                                 const Ioss::Field           &field,
                                                 const Ioss::GroupingEntity *ge, void *data) const
{
  // Read into a double variable since that is all ExodusII can store...
  size_t              num_entity = ge->entity_count();
  std::vector<double> temp(num_entity);

  size_t step = get_current_state();

  // get number of components, cycle through each component
  // and add suffix to base 'field_name'.  Look up index
  // of this name in 'nodeVariables' map
  size_t comp_count = field.get_component_count(Ioss::Field::InOut::INPUT);

  for (size_t i = 0; i < comp_count; i++) {
    std::string var_name = get_component_name(field, Ioss::Field::InOut::INPUT, i + 1);
    if (lowerCaseVariableNames) {
      Ioss::Utils::fixup_name(var_name);
    }

    // Read the variable...
    ex_entity_type type     = Ioex::map_exodus_type(ge->type());
    int64_t        id       = Ioex::get_id(ge, &ids_);
    int            ierr     = 0;
    auto           var_iter = variables.find(var_name);
    if (var_iter == variables.end()) {
      std::ostringstream errmsg;
      fmt::print(errmsg, "ERROR: Could not find field '{}'\n", var_name);
      IOSS_ERROR(errmsg);
    }
    size_t var_index = var_iter->second;
    assert(var_index > 0);
    if (type == EX_BLOB) {
      size_t offset = ge->get_property("_processor_offset").get_int();
      ierr          = ex_get_partial_var(get_file_pointer(), step, type, var_index, id, offset + 1,
                                         num_entity, temp.data());
    }
    else {
      ierr = decomp->get_var(get_file_pointer(), step, type, var_index, id, num_entity, temp);
    }
    if (ierr < 0) {
      Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
    }

    // Transfer to 'data' array.
    size_t k = 0;
    if (field.get_type() == Ioss::Field::INTEGER) {
      int *ivar = static_cast<int *>(data);
      for (size_t j = i; j < num_entity * comp_count; j += comp_count) {
        ivar[j] = static_cast<int>(temp[k++]);
      }
    }
    else if (field.get_type() == Ioss::Field::INT64) { // FIX 64 UNSAFE
      int64_t *ivar = static_cast<int64_t *>(data);
      for (size_t j = i; j < num_entity * comp_count; j += comp_count) {
        ivar[j] = static_cast<int64_t>(temp[k++]);
      }
    }
    else if (field.get_type() == Ioss::Field::REAL) {
      double *rvar = static_cast<double *>(data);
      for (size_t j = i; j < num_entity * comp_count; j += comp_count) {
        rvar[j] = temp[k++];
      }
    }
    else {
      std::ostringstream errmsg;
      fmt::print(errmsg,
                 "IOSS_ERROR: Field storage type must be either integer or double.\n"
                 "       Field '{}' is invalid.\n",
                 field.get_name());
      IOSS_ERROR(errmsg);
    }
    assert(k == num_entity);
  }
  return num_entity;
}

int64_t ParallelDatabaseIO::read_ss_transient_field(const Ioss::Field &field, int64_t id,
                                                    void            *variables,
                                                    Ioss::IntVector &is_valid_side) const
{
  size_t              num_valid_sides = 0;
  size_t              my_side_count   = is_valid_side.size();
  std::vector<double> temp(my_side_count);

  size_t step = get_current_state();

  // get number of components, cycle through each component
  // and add suffix to base 'field_name'.  Look up index
  // of this name in 'nodeVariables' map
  size_t comp_count = field.get_component_count(Ioss::Field::InOut::INPUT);

  for (size_t i = 0; i < comp_count; i++) {
    std::string var_name = get_component_name(field, Ioss::Field::InOut::INPUT, i + 1);

    // Read the variable...
    int  ierr     = 0;
    auto var_iter = m_variables[EX_SIDE_SET].find(var_name);
    if (var_iter == m_variables[EX_SIDE_SET].end()) {
      std::ostringstream errmsg;
      fmt::print(errmsg, "ERROR: Could not find Sideset field '{}'\n", var_name);
      IOSS_ERROR(errmsg);
    }
    size_t var_index = var_iter->second;
    assert(var_index > 0);
    ierr =
        decomp->get_var(get_file_pointer(), step, EX_SIDE_SET, var_index, id, my_side_count, temp);
    if (ierr < 0) {
      Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
    }

    // Transfer to 'variables' array.
    size_t j = i;
    if (field.get_type() == Ioss::Field::INTEGER) {
      int *ivar = static_cast<int *>(variables);
      for (size_t k = 0; k < my_side_count; k++) {
        if (is_valid_side[k] == 1) {
          ivar[j] = static_cast<int>(temp[k]);
          j += comp_count;
        }
      }
    }
    else if (field.get_type() == Ioss::Field::INT64) { // FIX 64 UNSAFE
      int64_t *ivar = static_cast<int64_t *>(variables);
      for (size_t k = 0; k < my_side_count; k++) {
        if (is_valid_side[k] == 1) {
          ivar[j] = static_cast<int64_t>(temp[k]);
          j += comp_count;
        }
      }
    }
    else if (field.get_type() == Ioss::Field::REAL) {
      double *rvar = static_cast<double *>(variables);
      for (size_t k = 0; k < my_side_count; k++) {
        if (is_valid_side[k] == 1) {
          rvar[j] = temp[k];
          j += comp_count;
        }
      }
    }
    else {
      std::ostringstream errmsg;
      fmt::print(errmsg,
                 "IOSS_ERROR: Field storage type must be either integer or double.\n"
                 "       Field '{}' is invalid.\n",
                 field.get_name());
      IOSS_ERROR(errmsg);
    }
    if (i + 1 == comp_count) {
      num_valid_sides = j / comp_count;
    }
  }
  return num_valid_sides;
}

int64_t ParallelDatabaseIO::get_side_connectivity(const Ioss::SideBlock *sb, int64_t id,
                                                  int64_t /*unused*/, void          *fconnect,
                                                  bool map_ids) const
{
  // Get size of data stored on the file...
  ex_set set_param[1];
  set_param[0].id                       = id;
  set_param[0].type                     = EX_SIDE_SET;
  set_param[0].entry_list               = nullptr;
  set_param[0].extra_list               = nullptr;
  set_param[0].distribution_factor_list = nullptr;
  int ierr                              = ex_get_sets(get_file_pointer(), 1, set_param);
  if (ierr < 0) {
    Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
  }

  int64_t number_sides = set_param[0].num_entry;

  // Allocate space for element and local side number
  assert(number_sides > 0);

  // Need a larger vector to get the entire sideset and then filter down to the correct size...
  std::vector<char> element(number_sides * int_byte_size_api());
  std::vector<char> side(number_sides * int_byte_size_api());
  if (int_byte_size_api() == 4) {
    Ioss::Field elem_field("ids", Ioss::Field::INTEGER, "scalar", Ioss::Field::MESH, number_sides);
    Ioss::Field side_field("sides", Ioss::Field::INTEGER, "scalar", Ioss::Field::MESH,
                           number_sides);
    decomp->get_set_mesh_var(get_file_pointer(), EX_SIDE_SET, id, elem_field,
                             reinterpret_cast<int *>(element.data()));
    decomp->get_set_mesh_var(get_file_pointer(), EX_SIDE_SET, id, side_field,
                             reinterpret_cast<int *>(side.data()));
  }
  else {
    Ioss::Field elem_field("ids", Ioss::Field::INT64, "scalar", Ioss::Field::MESH, number_sides);
    Ioss::Field side_field("sides", Ioss::Field::INT64, "scalar", Ioss::Field::MESH, number_sides);
    decomp->get_set_mesh_var(get_file_pointer(), EX_SIDE_SET, id, elem_field,
                             reinterpret_cast<int64_t *>(element.data()));
    decomp->get_set_mesh_var(get_file_pointer(), EX_SIDE_SET, id, side_field,
                             reinterpret_cast<int64_t *>(side.data()));
  }

  Ioss::IntVector is_valid_side;
  Ioss::Utils::calculate_sideblock_membership(is_valid_side, sb, int_byte_size_api(),
                                              (void *)element.data(), (void *)side.data(),
                                              number_sides, get_region());

  std::vector<char>   elconnect;
  int64_t             elconsize  = 0;       // Size of currently allocated connectivity block
  Ioss::ElementBlock *conn_block = nullptr; // Block that we currently
  // have connectivity for

  Ioss::ElementBlock *block = nullptr;

  int     *element32 = nullptr;
  int64_t *element64 = nullptr;
  int     *side32    = nullptr;
  int64_t *side64    = nullptr;

  int     *elconn32 = nullptr;
  int64_t *elconn64 = nullptr;
  int     *fconn32  = nullptr;
  int64_t *fconn64  = nullptr;

  if (int_byte_size_api() == 4) {
    element32 = reinterpret_cast<int *>(element.data());
    side32    = reinterpret_cast<int *>(side.data());
    fconn32   = reinterpret_cast<int *>(fconnect);
  }
  else {
    element64 = reinterpret_cast<int64_t *>(element.data());
    side64    = reinterpret_cast<int64_t *>(side.data());
    fconn64   = reinterpret_cast<int64_t *>(fconnect);
  }

  Ioss::IntVector side_elem_map; // Maps the side into the elements
  // connectivity array
  int64_t current_side = -1;
  int     nelnode      = 0;
  int     nfnodes      = 0;
  int     ieb          = 0;
  size_t  offset       = 0;
  for (int64_t iel = 0; iel < number_sides; iel++) {
    if (is_valid_side[iel] == 1) {

      int64_t elem_id = 0;
      if (int_byte_size_api() == 4) {
        elem_id = element32[iel];
      }
      else {
        elem_id = element64[iel];
      }

      // ensure we have correct connectivity
      block = get_region()->get_element_block(elem_id);
      if (conn_block != block) {
        int64_t nelem = block->entity_count();
        nelnode       = block->topology()->number_nodes();
        // Used to map element number into position in connectivity array.
        // E.g., element 97 is the (97-offset)th element in this block and
        // is stored in array index (97-offset-1).
        offset = block->get_offset() + 1;
        if (elconsize < nelem * nelnode) {
          elconsize = nelem * nelnode;
          elconnect.resize(elconsize * int_byte_size_api());
          if (int_byte_size_api() == 4) {
            elconn32 = reinterpret_cast<int *>(elconnect.data());
          }
          else {
            elconn64 = reinterpret_cast<int64_t *>(elconnect.data());
          }
        }
        if (map_ids) {
          get_field_internal(block, block->get_field("connectivity"), elconnect.data(),
                             nelem * nelnode * int_byte_size_api());
        }
        else {
          get_field_internal(block, block->get_field("connectivity_raw"), elconnect.data(),
                             nelem * nelnode * int_byte_size_api());
        }
        conn_block   = block;
        current_side = -1;
      }

      // NOTE: Element connectivity is returned with nodes in global id space if "map_ids" false,
      //       otherwise it is in local space.
      int64_t side_id = 0;
      if (int_byte_size_api() == 4) {
        side_id = side32[iel];
      }
      else {
        side_id = side64[iel];
      }

      if (current_side != side_id) {
        side_elem_map = block->topology()->boundary_connectivity(side_id);
        current_side  = side_id;
        nfnodes       = block->topology()->boundary_type(side_id)->number_nodes();
      }
      for (int inode = 0; inode < nfnodes; inode++) {
        size_t index = (elem_id - offset) * nelnode + side_elem_map[inode];
        if (int_byte_size_api() == 4) {
          fconn32[ieb++] = elconn32[index];
        }
        else {
          fconn64[ieb++] = elconn64[index];
        }
      }
    }
  }
  return ierr;
}

// Get distribution factors for the specified side block
int64_t ParallelDatabaseIO::get_side_distributions(const Ioss::SideBlock *sb, int64_t id,
                                                   int64_t my_side_count, double *dist_fact,
                                                   size_t /* data_size */) const
{
  // Allocate space for elements and local side numbers
  // Get size of data stored on the file...

  auto   &set                         = decomp->get_decomp_set(EX_SIDE_SET, id);
  int64_t number_sides                = set.ioss_count();
  int64_t number_distribution_factors = set.df_count();

  const Ioss::ElementTopology *ftopo   = sb->topology();
  int                          nfnodes = ftopo->number_nodes();

  if (set.distributionFactorConstant) {
    // Fill in the array with the constant value...
    for (int64_t i = 0; i < nfnodes * my_side_count; i++) {
      dist_fact[i] = set.distributionFactorValue;
    }
    return 0;
  }

  // Take care of the easy situation -- If 'side_count' ==
  // 'number_sides' then the sideset is stored in a single sideblock
  // and all distribution factors on the database are transferred
  // 1-to-1 into 'dist_fact' array.
  int64_t entity_count = sb->entity_count();
  if (sb->owner()->block_count() == 1 && number_sides == entity_count) {
    assert(number_sides == 0 || number_distribution_factors % number_sides == 0);
    assert(number_sides == 0 || number_distribution_factors / number_sides == nfnodes);
    if (number_sides * nfnodes != number_distribution_factors &&
        number_sides != number_distribution_factors) {
      std::ostringstream errmsg;
      fmt::print(errmsg,
                 "ERROR: SideBlock '{}' has incorrect distribution factor count.\n"
                 "\tThere are {} '{}' sides with {} nodes per side, but there are {}"
                 " distribution factors which is not correct.\n"
                 "\tThere should be either {} or {} distribution factors.\n",
                 sb->name(), number_sides, ftopo->name(), nfnodes, number_distribution_factors,
                 number_sides, number_sides * nfnodes);
      IOSS_ERROR(errmsg);
    }
    std::string storage = "Real[" + std::to_string(nfnodes) + "]";
    Ioss::Field dist("distribution_factors", Ioss::Field::REAL, storage, Ioss::Field::MESH,
                     number_sides);
    decomp->get_set_mesh_double(get_file_pointer(), EX_SIDE_SET, id, dist, dist_fact);
    return 0;
  }

  std::string         storage = "Real[" + std::to_string(nfnodes) + "]";
  Ioss::Field         field("distribution_factors", Ioss::Field::REAL, storage, Ioss::Field::MESH,
                            number_distribution_factors / nfnodes);
  std::vector<double> dist(number_distribution_factors);
  decomp->get_set_mesh_double(get_file_pointer(), EX_SIDE_SET, id, field, dist.data());

  // Another easy situation (and common for exodusII) is if the input
  // distribution factors are all the same value (typically 1).  In
  // that case, we only have to fill in the output array with that
  // value.
  {
    double value    = number_distribution_factors > 0 ? dist[0] : 0.0;
    bool   constant = true;
    for (auto df : dist) {
      if (df != value) {
        constant = false;
        break;
      }
    }

    constant = util().global_minmax(constant ? 1 : 0, Ioss::ParallelUtils::DO_MIN);

    if (constant) {
      if (value == 0.0) {
        value = 1.0; // Take care of some buggy mesh generators
      }
      for (int64_t j = 0; j < my_side_count * nfnodes; j++) {
        dist_fact[j] = value;
      }
      return 0;
    }
  }

  // If we get to here, the underlying sideset contains multiple side
  // topologies and the distribution factors are non-constant. Need to
  // allocate space to store all distribution factors and then pull
  // out those that are applied to sides with the correct topology.

  // Allocate space for element and local side number (this is bulk
  // data...)
  //----
  std::vector<char> element(number_sides * int_byte_size_api());
  std::vector<char> sides(number_sides * int_byte_size_api());
  if (int_byte_size_api() == 4) {
    Ioss::Field elem_field("ids", Ioss::Field::INTEGER, "scalar", Ioss::Field::MESH, number_sides);
    Ioss::Field side_field("sides", Ioss::Field::INTEGER, "scalar", Ioss::Field::MESH,
                           number_sides);
    decomp->get_set_mesh_var(get_file_pointer(), EX_SIDE_SET, id, elem_field,
                             reinterpret_cast<int *>(element.data()));
    decomp->get_set_mesh_var(get_file_pointer(), EX_SIDE_SET, id, side_field,
                             reinterpret_cast<int *>(sides.data()));
  }
  else {
    Ioss::Field elem_field("ids", Ioss::Field::INT64, "scalar", Ioss::Field::MESH, number_sides);
    Ioss::Field side_field("sides", Ioss::Field::INT64, "scalar", Ioss::Field::MESH, number_sides);
    decomp->get_set_mesh_var(get_file_pointer(), EX_SIDE_SET, id, elem_field,
                             reinterpret_cast<int64_t *>(element.data()));
    decomp->get_set_mesh_var(get_file_pointer(), EX_SIDE_SET, id, side_field,
                             reinterpret_cast<int64_t *>(sides.data()));
  }
  //----

  Ioss::IntVector is_valid_side;
  Ioss::Utils::calculate_sideblock_membership(is_valid_side, sb, int_byte_size_api(),
                                              element.data(), sides.data(), number_sides,
                                              get_region());

  int64_t             ieb   = 0; // counter for distribution factors in this sideblock
  int64_t             idb   = 0; // counter for distribution factors read from database
  Ioss::ElementBlock *block = nullptr;

  int     *element32 = nullptr;
  int64_t *element64 = nullptr;
  int     *side32    = nullptr;
  int64_t *side64    = nullptr;

  if (int_byte_size_api() == 4) {
    element32 = reinterpret_cast<int *>(element.data());
    side32    = reinterpret_cast<int *>(sides.data());
  }
  else {
    element64 = reinterpret_cast<int64_t *>(element.data());
    side64    = reinterpret_cast<int64_t *>(sides.data());
  }

  for (int64_t iel = 0; iel < number_sides; iel++) {
    int64_t elem_id = 0;
    int64_t side_id = 0;
    if (int_byte_size_api() == 4) {
      elem_id = element32[iel];
      side_id = side32[iel];
    }
    else {
      elem_id = element64[iel];
      side_id = side64[iel];
    }

    if (block == nullptr || !block->contains(elem_id)) {
      block = get_region()->get_element_block(elem_id);
    }

    if (block == nullptr) {
      std::ostringstream errmsg;
      fmt::print(errmsg,
                 "INTERNAL ERROR: Could not find element block containing element with id {}."
                 " Something is wrong in the Ioex::ParallelDatabaseIO class. Please report.\n",
                 elem_id);
      IOSS_ERROR(errmsg);
    }

    const Ioss::ElementTopology *topo = block->topology()->boundary_type(side_id);

    if (topo == nullptr) {
      std::ostringstream errmsg;
      fmt::print(errmsg,
                 "INTERNAL ERROR: Could not find topology of element block boundary. "
                 "Something is wrong in the Ioex::ParallelDatabaseIO class. Please report.\n");
      IOSS_ERROR(errmsg);
    }

    int nside_nodes = topo->number_nodes();

    if (is_valid_side[iel] == 1) {
      // This side belongs in the sideblock
      for (int64_t i = 0; i < nside_nodes; i++) {
        dist_fact[ieb++] = dist[idb++];
      }
    }
    else {
      // Skip over unused 'dist' factors
      idb += topo->number_nodes();
    }
  }

  assert(ieb == my_side_count * nfnodes);
  // If the following assert fails, it may be due to bug in Patran
  // which writes too many distribution factors to the database in a
  // mixed element case. Note that this is checked earlier also with a
  // better error message.
  assert(idb == number_distribution_factors);
  return 0;
}

int64_t ParallelDatabaseIO::put_field_internal(const Ioss::Region *reg, const Ioss::Field &field,
                                               void *data, size_t data_size) const
{
  return Ioex::BaseDatabaseIO::put_field_internal(reg, field, data, data_size);
}

int64_t ParallelDatabaseIO::put_field_internal(const Ioss::NodeBlock *nb, const Ioss::Field &field,
                                               void *data, size_t data_size) const
{
  size_t num_to_get = field.verify(data_size);

  size_t proc_offset = nb->get_optional_property("_processor_offset", 0);
  size_t file_count  = nb->get_optional_property("locally_owned_count", num_to_get);

  Ioss::Field::RoleType role = field.get_role();

  if (role == Ioss::Field::MESH) {
    if (field.get_name() == "owning_processor") {
      // Set the nodeOwningProcessor vector for all nodes on this processor.
      // Value is the processor that owns the node.

      // NOTE: The owning_processor field is always int32
      nodeOwningProcessor.reserve(num_to_get);
      int *owned = (int *)data;
      for (size_t i = 0; i < num_to_get; i++) {
        nodeOwningProcessor.push_back(owned[i]);
      }

      // Now create the "implicit local" to "implicit global"
      // map which maps data from its local implicit position
      // to its implicit (1..num_global_node) position in the
      // global file.  This is needed for the global-to-local
      // mapping of element connectivity and nodeset nodelists.
      create_implicit_global_map();
    }

    else if (field.get_name() == "mesh_model_coordinates_x") {
      double             *rdata = static_cast<double *>(data);
      std::vector<double> file_data;
      file_data.reserve(file_count);
      check_node_owning_processor_data(nodeOwningProcessor, file_count);
      filter_owned_nodes(nodeOwningProcessor, myProcessor, rdata, file_data);

      int ierr = ex_put_partial_coord_component(get_file_pointer(), proc_offset + 1, file_count, 1,
                                                file_data.data());
      if (ierr < 0) {
        Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
      }
    }

    else if (field.get_name() == "mesh_model_coordinates_y") {
      double             *rdata = static_cast<double *>(data);
      std::vector<double> file_data;
      file_data.reserve(file_count);
      check_node_owning_processor_data(nodeOwningProcessor, file_count);
      filter_owned_nodes(nodeOwningProcessor, myProcessor, rdata, file_data);
      int ierr = ex_put_partial_coord_component(get_file_pointer(), proc_offset + 1, file_count, 2,
                                                file_data.data());
      if (ierr < 0) {
        Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
      }
    }

    else if (field.get_name() == "mesh_model_coordinates_z") {
      double             *rdata = static_cast<double *>(data);
      std::vector<double> file_data;
      file_data.reserve(file_count);
      check_node_owning_processor_data(nodeOwningProcessor, file_count);
      filter_owned_nodes(nodeOwningProcessor, myProcessor, rdata, file_data);
      int ierr = ex_put_partial_coord_component(get_file_pointer(), proc_offset + 1, file_count, 3,
                                                file_data.data());
      if (ierr < 0) {
        Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
      }
    }

    else if (field.get_name() == "mesh_model_coordinates") {
      // Data required by upper classes store x0, y0, z0, ... xn, yn, zn
      // Data stored in exodusII file is x0, ..., xn, y0, ..., yn, z0, ..., zn
      // so we have to allocate some scratch memory to read in the data
      // and then map into supplied 'data'
      std::vector<double> x;
      std::vector<double> y;
      std::vector<double> z;

      x.reserve(file_count > 0 ? file_count : 1);
      if (spatialDimension > 1) {
        y.reserve(file_count > 0 ? file_count : 1);
      }
      if (spatialDimension == 3) {
        z.reserve(file_count > 0 ? file_count : 1);
      }

      // Cast 'data' to correct size -- double
      double *rdata = static_cast<double *>(data);
      check_node_owning_processor_data(nodeOwningProcessor, file_count);
      filter_owned_nodes(nodeOwningProcessor, myProcessor, rdata, x, 0, spatialDimension);
      if (spatialDimension > 1) {
        filter_owned_nodes(nodeOwningProcessor, myProcessor, rdata, y, 1, spatialDimension);
      }
      if (spatialDimension == 3) {
        filter_owned_nodes(nodeOwningProcessor, myProcessor, rdata, z, 2, spatialDimension);
      }

      int ierr = ex_put_partial_coord(get_file_pointer(), proc_offset + 1, file_count, x.data(),
                                      y.data(), z.data());
      if (ierr < 0) {
        Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
      }
    }
    else if (field.get_name() == "ids") {
      // The ids coming in are the global ids; their position is the
      // local id -1 (That is, data[0] contains the global id of local
      // node 1)

      // Another 'const-cast' since we are modifying the database just
      // for efficiency; which the client does not see...
      handle_node_ids(data, num_to_get, proc_offset, file_count);
    }
    else if (field.get_name() == "connectivity") {
      // Do nothing, just handles an idiosyncrasy of the GroupingEntity
    }
    else if (field.get_name() == "connectivity_raw") {
      // Do nothing, just handles an idiosyncrasy of the GroupingEntity
    }
    else if (field.get_name() == "node_connectivity_status") {
      // Do nothing, input only field.
    }
    else if (field.get_name() == "implicit_ids") {
      // Do nothing, input only field.
    }
    else {
      return Ioss::Utils::field_warning(nb, field, "mesh output");
    }
  }
  else if (role == Ioss::Field::TRANSIENT) {
    // Check if the specified field exists on this node block.
    // Note that 'higher-order' storage types (e.g. SYM_TENSOR)
    // exist on the database as scalars with the appropriate
    // extensions.

    // Transfer each component of the variable into 'data' and then
    // output.  Need temporary storage area of size 'number of
    // nodes in this block.
    write_nodal_transient_field(field, nb, num_to_get, data);
  }
  else if (role == Ioss::Field::REDUCTION) {
    store_reduction_field(field, nb, data);
  }
  return num_to_get;
}

int64_t ParallelDatabaseIO::put_field_internal(const Ioss::Blob *blob, const Ioss::Field &field,
                                               void *data, size_t data_size) const
{
  {
    Ioss::SerializeIO serializeIO__(this);

    size_t num_to_get = field.verify(data_size);
    if (num_to_get > 0) {

      Ioss::Field::RoleType role = field.get_role();

      if (role == Ioss::Field::MESH) {
        if (field.get_name() == "ids") {
          // The ids coming in are the global ids; their position is the
          // local id -1 (That is, data[0] contains the global id of local
          // node 1)
          //          handle_node_ids(data, num_to_get);
        }
        else if (field.get_name() == "connectivity") {
          // Do nothing, just handles an idiosyncrasy of the GroupingEntity
        }
        else if (field.get_name() == "connectivity_raw") {
          // Do nothing, just handles an idiosyncrasy of the GroupingEntity
        }
        else if (field.get_name() == "node_connectivity_status") {
          // Do nothing, input only field.
        }
        else if (field.get_name() == "implicit_ids") {
          // Do nothing, input only field.
        }
        else {
          return Ioss::Utils::field_warning(blob, field, "mesh output");
        }
      }
      else if (role == Ioss::Field::TRANSIENT) {
        // Check if the specified field exists on this node block.
        // Note that 'higher-order' storage types (e.g. SYM_TENSOR)
        // exist on the database as scalars with the appropriate
        // extensions.

        // Transfer each component of the variable into 'data' and then
        // output.  Need temporary storage area of size 'number of
        // nodes in this block.
        write_entity_transient_field(field, blob, num_to_get, data);
      }
      else if (role == Ioss::Field::REDUCTION) {
        store_reduction_field(field, blob, data);
      }
      else if (role == Ioss::Field::ATTRIBUTE) {
        num_to_get = write_attribute_field(field, blob, data);
      }
    }
    return num_to_get;
  }
}

int64_t ParallelDatabaseIO::put_field_internal(const Ioss::Assembly *assembly,
                                               const Ioss::Field &field, void *data,
                                               size_t data_size) const
{
  {
    Ioss::SerializeIO serializeIO__(this);

    size_t num_to_get = field.verify(data_size);
    if (num_to_get > 0) {

      Ioss::Field::RoleType role = field.get_role();

      if (role == Ioss::Field::MESH) {
        if (field.get_name() == "ids") {
          // The ids coming in are the global ids; their position is the
          // local id -1 (That is, data[0] contains the global id of local
          // node 1)
          //          handle_node_ids(data, num_to_get);
        }
        else if (field.get_name() == "connectivity") {
          // Do nothing, just handles an idiosyncrasy of the GroupingEntity
        }
        else if (field.get_name() == "connectivity_raw") {
          // Do nothing, just handles an idiosyncrasy of the GroupingEntity
        }
        else if (field.get_name() == "node_connectivity_status") {
          // Do nothing, input only field.
        }
        else if (field.get_name() == "implicit_ids") {
          // Do nothing, input only field.
        }
        else {
          return Ioss::Utils::field_warning(assembly, field, "mesh output");
        }
      }
      else if (role == Ioss::Field::TRANSIENT) {
        // Check if the specified field exists on this node block.
        // Note that 'higher-order' storage types (e.g. SYM_TENSOR)
        // exist on the database as scalars with the appropriate
        // extensions.

        // Transfer each component of the variable into 'data' and then
        // output.  Need temporary storage area of size 'number of
        // nodes in this block.
        write_entity_transient_field(field, assembly, num_to_get, data);
      }
      else if (role == Ioss::Field::REDUCTION) {
        store_reduction_field(field, assembly, data);
      }
      else if (role == Ioss::Field::ATTRIBUTE) {
        num_to_get = write_attribute_field(field, assembly, data);
      }
    }
    return num_to_get;
  }
}

int64_t ParallelDatabaseIO::put_field_internal(const Ioss::ElementBlock *eb,
                                               const Ioss::Field &field, void *data,
                                               size_t data_size) const
{
  size_t num_to_get = field.verify(data_size);

  int ierr = 0;

  // Get the element block id and element count
  int64_t               id               = Ioex::get_id(eb, &ids_);
  int64_t               my_element_count = eb->entity_count();
  Ioss::Field::RoleType role             = field.get_role();

  size_t proc_offset = eb->get_optional_property("_processor_offset", 0);
  size_t file_count  = eb->get_optional_property("locally_owned_count", num_to_get);

  if (role == Ioss::Field::MESH) {
    // Handle the MESH fields required for an ExodusII file model.
    // (The 'genesis' portion)
    if (field.get_name() == "connectivity") {
      // Map element connectivity from global node id to local node id.
      int element_nodes = eb->topology()->number_nodes();

      // Maps global to local
      nodeMap.reverse_map_data(data, field, num_to_get * element_nodes);

      // Maps local to "global_implicit"
      if (int_byte_size_api() == 4) {
        map_local_to_global_implicit(reinterpret_cast<int *>(data), num_to_get * element_nodes,
                                     nodeGlobalImplicitMap);
      }
      else {
        map_local_to_global_implicit(reinterpret_cast<int64_t *>(data), num_to_get * element_nodes,
                                     nodeGlobalImplicitMap);
      }

      ierr = ex_put_partial_conn(get_file_pointer(), EX_ELEM_BLOCK, id, proc_offset + 1, file_count,
                                 data, nullptr, nullptr);
      if (ierr < 0) {
        Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
      }
    }
    else if (field.get_name() == "connectivity_edge") {
      // Map element connectivity from global edge id to local edge id.
      int element_edges = field.get_component_count(Ioss::Field::InOut::OUTPUT);
      edgeMap.reverse_map_data(data, field, num_to_get * element_edges);
      ierr = ex_put_conn(get_file_pointer(), EX_ELEM_BLOCK, id, nullptr, data, nullptr);
      if (ierr < 0) {
        Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
      }
    }
    else if (field.get_name() == "connectivity_face") {
      // Map element connectivity from global face id to local face id.
      int element_faces = field.get_component_count(Ioss::Field::InOut::OUTPUT);
      faceMap.reverse_map_data(data, field, num_to_get * element_faces);
      ierr = ex_put_conn(get_file_pointer(), EX_ELEM_BLOCK, id, nullptr, nullptr, data);
      if (ierr < 0) {
        Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
      }
    }
    else if (field.get_name() == "connectivity_raw") {
      // Element connectivity is already in local node id, map local to "global_implicit"
      int element_nodes = eb->topology()->number_nodes();
      if (int_byte_size_api() == 4) {
        map_local_to_global_implicit(reinterpret_cast<int *>(data), num_to_get * element_nodes,
                                     nodeGlobalImplicitMap);
      }
      else {
        map_local_to_global_implicit(reinterpret_cast<int64_t *>(data), num_to_get * element_nodes,
                                     nodeGlobalImplicitMap);
      }

      ierr = ex_put_partial_conn(get_file_pointer(), EX_ELEM_BLOCK, id, proc_offset + 1, file_count,
                                 data, nullptr, nullptr);
      if (ierr < 0) {
        Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
      }
    }
    else if (field.get_name() == "ids") {
      size_t glob_map_offset = eb->get_property("global_map_offset").get_int();
      handle_element_ids(eb, data, num_to_get, glob_map_offset + proc_offset, file_count);
    }
    else if (field.get_name() == "implicit_ids") {
      // Do nothing, input only field.
    }
  }
  else if (role == Ioss::Field::MAP) {
    int comp_count = field.get_component_count(Ioss::Field::InOut::OUTPUT);
    for (int comp = 0; comp < comp_count; comp++) {
      std::vector<char> component(my_element_count * int_byte_size_api());

      if (int_byte_size_api() == 4) {
        int *data32 = reinterpret_cast<int *>(data);
        int *comp32 = reinterpret_cast<int *>(component.data());

        int index = comp;
        for (size_t i = 0; i < my_element_count; i++) {
          comp32[i] = data32[index];
          index += comp_count;
        }
      }
      else {
        int64_t *data64 = reinterpret_cast<int64_t *>(data);
        int64_t *comp64 = reinterpret_cast<int64_t *>(component.data());

        int index = comp;
        for (size_t i = 0; i < my_element_count; i++) {
          comp64[i] = data64[index];
          index += comp_count;
        }
      }
      size_t eb_offset =
          eb->get_offset(); // Offset of beginning of the element block elements for this block
      size_t proc_offset = eb->get_optional_property(
          "_processor_offset", 0); // Offset of this processors elements within that block.
      size_t file_count = eb->get_optional_property("locally_owned_count", my_element_count);
      int    index =
          -1 * (field.get_index() + comp); // Negative since specifying index, not id to exodus API.

      ierr = ex_put_partial_num_map(get_file_pointer(), EX_ELEM_MAP, index,
                                    proc_offset + eb_offset + 1, file_count, component.data());
    }
  }
  else if (role == Ioss::Field::ATTRIBUTE) {
    num_to_get = write_attribute_field(field, eb, data);
  }
  else if (role == Ioss::Field::TRANSIENT) {
    // Check if the specified field exists on this element block.
    // Note that 'higher-order' storage types (e.g. SYM_TENSOR)
    // exist on the database as scalars with the appropriate
    // extensions.

    // Transfer each component of the variable into 'data' and then
    // output.  Need temporary storage area of size 'number of
    // elements in this block.
    auto global_entity_count = eb->get_property("global_entity_count").get_int();
    if (global_entity_count > 0) {
      write_entity_transient_field(field, eb, my_element_count, data);
    }
  }
  else if (role == Ioss::Field::REDUCTION) {
    store_reduction_field(field, eb, data);
  }
  return num_to_get;
}

int64_t ParallelDatabaseIO::put_field_internal(const Ioss::FaceBlock *eb, const Ioss::Field &field,
                                               void *data, size_t data_size) const
{
  size_t num_to_get = field.verify(data_size);

  int ierr = 0;

  // Get the face block id and face count
  int64_t               id            = Ioex::get_id(eb, &ids_);
  int64_t               my_face_count = eb->entity_count();
  Ioss::Field::RoleType role          = field.get_role();

  if (role == Ioss::Field::MESH) {
    // Handle the MESH fields required for an ExodusII file model.
    // (The 'genesis' portion)
    if (field.get_name() == "connectivity") {
      if (my_face_count > 0) {
        // Map face connectivity from global node id to local node id.
        int face_nodes = eb->topology()->number_nodes();
        nodeMap.reverse_map_data(data, field, num_to_get * face_nodes);
        ierr = ex_put_conn(get_file_pointer(), EX_FACE_BLOCK, id, data, nullptr, nullptr);
        if (ierr < 0) {
          Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
        }
      }
    }
    else if (field.get_name() == "connectivity_edge") {
      if (my_face_count > 0) {
        // Map face connectivity from global edge id to local edge id.
        // Do it in 'data' ...
        int face_edges = field.get_component_count(Ioss::Field::InOut::OUTPUT);
        edgeMap.reverse_map_data(data, field, num_to_get * face_edges);
        ierr = ex_put_conn(get_file_pointer(), EX_FACE_BLOCK, id, nullptr, data, nullptr);
        if (ierr < 0) {
          Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
        }
      }
    }
    else if (field.get_name() == "connectivity_raw") {
      // Do nothing, input only field.
    }
    else if (field.get_name() == "ids") {
      handle_face_ids(eb, data, num_to_get);
    }
    else {
      num_to_get = Ioss::Utils::field_warning(eb, field, "mesh output");
    }
  }
  else if (role == Ioss::Field::ATTRIBUTE) {
    num_to_get = write_attribute_field(field, eb, data);
  }
  else if (role == Ioss::Field::TRANSIENT) {
    // Check if the specified field exists on this face block.
    // Note that 'higher-order' storage types (e.g. SYM_TENSOR)
    // exist on the database as scalars with the appropriate
    // extensions.

    // Transfer each component of the variable into 'data' and then
    // output.  Need temporary storage area of size 'number of
    // faces in this block.
    write_entity_transient_field(field, eb, my_face_count, data);
  }
  else if (role == Ioss::Field::REDUCTION) {
    store_reduction_field(field, eb, data);
  }
  return num_to_get;
}

int64_t ParallelDatabaseIO::put_field_internal(const Ioss::EdgeBlock *eb, const Ioss::Field &field,
                                               void *data, size_t data_size) const
{
  size_t num_to_get = field.verify(data_size);

  int ierr = 0;

  // Get the edge block id and edge count
  int64_t               id            = Ioex::get_id(eb, &ids_);
  int64_t               my_edge_count = eb->entity_count();
  Ioss::Field::RoleType role          = field.get_role();

  if (role == Ioss::Field::MESH) {
    // Handle the MESH fields required for an ExodusII file model. (The 'genesis' portion)
    if (field.get_name() == "connectivity") {
      if (my_edge_count > 0) {
        // Map edge connectivity from global node id to local node id.
        int edge_nodes = eb->topology()->number_nodes();
        nodeMap.reverse_map_data(data, field, num_to_get * edge_nodes);
        ierr = ex_put_conn(get_file_pointer(), EX_EDGE_BLOCK, id, data, nullptr, nullptr);
        if (ierr < 0) {
          Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
        }
      }
    }
    else if (field.get_name() == "connectivity_raw") {
      // Do nothing, input only field.
    }
    else if (field.get_name() == "ids") {
      handle_edge_ids(eb, data, num_to_get);
    }
    else {
      num_to_get = Ioss::Utils::field_warning(eb, field, "mesh output");
    }
  }
  else if (role == Ioss::Field::ATTRIBUTE) {
    num_to_get = write_attribute_field(field, eb, data);
  }
  else if (role == Ioss::Field::TRANSIENT) {
    // Check if the specified field exists on this edge block.
    // Note that 'higher-order' storage types (e.g. SYM_TENSOR)
    // exist on the database as scalars with the appropriate
    // extensions.

    // Transfer each component of the variable into 'data' and then
    // output.  Need temporary storage area of size 'number of
    // edges in this block.
    write_entity_transient_field(field, eb, my_edge_count, data);
  }
  else if (role == Ioss::Field::REDUCTION) {
    store_reduction_field(field, eb, data);
  }
  return num_to_get;
}

int64_t ParallelDatabaseIO::handle_node_ids(void *ids, int64_t num_to_get, size_t /* offset */,
                                            size_t /*count*/) const
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
  nodeMap.set_size(num_to_get);

  bool in_define = (dbState == Ioss::STATE_MODEL) || (dbState == Ioss::STATE_DEFINE_MODEL);
  if (int_byte_size_api() == 4) {
    nodeMap.set_map(static_cast<int *>(ids), num_to_get, 0, in_define);
  }
  else {
    nodeMap.set_map(static_cast<int64_t *>(ids), num_to_get, 0, in_define);
  }

  nodeMap.set_defined(true);
  return num_to_get;
}

int64_t ParallelDatabaseIO::handle_element_ids(const Ioss::ElementBlock *eb, void *ids,
                                               size_t num_to_get, size_t offset, size_t count) const
{
  if (dbState == Ioss::STATE_MODEL) {
    if (elemGlobalImplicitMap.empty()) {
      elemGlobalImplicitMap.resize(elementCount);
    }
    // Build the implicit_global map used to map an elements
    // local-implicit position to the global-implicit
    // position. Primarily used for sideset elements.  'count'
    // Elements starting at 'eb_offset' map to the global implicit
    // position of 'offset'
    int64_t eb_offset = eb->get_offset();
    for (size_t i = 0; i < count; i++) {
      elemGlobalImplicitMap[eb_offset + i] = offset + i + 1;
    }
    elemGlobalImplicitMapDefined = true;
  }

  elemMap.set_size(elementCount);
  return handle_block_ids(eb, EX_ELEM_MAP, elemMap, ids, num_to_get, offset);
}

int64_t ParallelDatabaseIO::handle_face_ids(const Ioss::FaceBlock *eb, void *ids,
                                            size_t num_to_get) const
{
  faceMap.set_size(faceCount);
  return handle_block_ids(eb, EX_FACE_MAP, faceMap, ids, num_to_get, 0);
}

int64_t ParallelDatabaseIO::handle_edge_ids(const Ioss::EdgeBlock *eb, void *ids,
                                            size_t num_to_get) const
{
  edgeMap.set_size(edgeCount);
  return handle_block_ids(eb, EX_EDGE_MAP, edgeMap, ids, num_to_get, 0);
}

void ParallelDatabaseIO::write_nodal_transient_field(const Ioss::Field     &field,
                                                     const Ioss::NodeBlock *nb, int64_t count,
                                                     void *variables) const
{
  Ioss::Field::BasicType ioss_type = field.get_type();
  assert(ioss_type == Ioss::Field::REAL || ioss_type == Ioss::Field::INTEGER ||
         ioss_type == Ioss::Field::INT64 || ioss_type == Ioss::Field::COMPLEX);

  if (ioss_type == Ioss::Field::INT64) {
    Ioss::Utils::check_int_to_real_overflow(field, (int64_t *)variables, count);
  }

  // Note that if the field's basic type is COMPLEX, then each component of
  // the VariableType is a complex variable consisting of a real and
  // imaginary part.  Since exodus cannot handle complex variables,
  // we have to output a (real and imaginary) X (number of
  // components) fields. For example, if V is a 3d vector of complex
  // data, the data in the 'variables' array are v_x, v.im_x, v_y,
  // v.im_y, v_z, v.im_z which need to be output in six separate
  // exodus fields.  These fields were already defined in
  // "write_results_metadata".

  std::vector<double> temp(count);

  int step = get_current_state();
  step     = get_database_step(step);

  // get number of components, cycle through each component
  // and add suffix to base 'field_name'.  Look up index
  // of this name in 'm_variables[EX_NODE_BLOCK]' map
  int comp_count = field.get_component_count(Ioss::Field::InOut::OUTPUT);

  int re_im = 1;
  if (ioss_type == Ioss::Field::COMPLEX) {
    re_im = 2;
  }
  for (int complex_comp = 0; complex_comp < re_im; complex_comp++) {
    std::string field_name = field.get_name();
    if (re_im == 2) {
      field_name += complex_suffix[complex_comp];
    }

    for (int i = 0; i < comp_count; i++) {
      std::string var_name = get_component_name(field, Ioss::Field::InOut::OUTPUT, i + 1);

      auto var_iter = m_variables[EX_NODE_BLOCK].find(var_name);
      if (var_iter == m_variables[EX_NODE_BLOCK].end()) {
        std::ostringstream errmsg;
        fmt::print(errmsg, "ERROR: Could not find nodal variable '{}'\n", var_name);
        IOSS_ERROR(errmsg);
      }

      int var_index = var_iter->second;

      size_t begin_offset = (re_im * i) + complex_comp;
      size_t stride       = re_im * comp_count;
      size_t num_out      = 0;

      if (ioss_type == Ioss::Field::REAL || ioss_type == Ioss::Field::COMPLEX) {
        num_out = nodeMap.map_field_to_db_scalar_order(static_cast<double *>(variables), temp,
                                                       begin_offset, count, stride, 0);
      }
      else if (ioss_type == Ioss::Field::INTEGER) {
        num_out = nodeMap.map_field_to_db_scalar_order(static_cast<int *>(variables), temp,
                                                       begin_offset, count, stride, 0);
      }
      else if (ioss_type == Ioss::Field::INT64) {
        num_out = nodeMap.map_field_to_db_scalar_order(static_cast<int64_t *>(variables), temp,
                                                       begin_offset, count, stride, 0);
      }

      if (num_out != static_cast<size_t>(nodeCount)) {
        std::ostringstream errmsg;
        fmt::print(errmsg,
                   "ERROR: Problem outputting nodal variable '{}' with index = {} to file '{}' on "
                   "processor {}\n"
                   "\tShould have output {} values, but instead only output {} values.\n",
                   var_name, var_index, get_filename(), myProcessor, fmt::group_digits(nodeCount),
                   fmt::group_digits(num_out));
        IOSS_ERROR(errmsg);
      }

      // Write the variable...
      size_t proc_offset = nb->get_optional_property("_processor_offset", 0);
      size_t file_count  = nb->get_optional_property("locally_owned_count", num_out);

      check_node_owning_processor_data(nodeOwningProcessor, file_count);
      filter_owned_nodes(nodeOwningProcessor, myProcessor, temp.data());
      int ierr = ex_put_partial_var(get_file_pointer(), step, EX_NODE_BLOCK, var_index, 0,
                                    proc_offset + 1, file_count, temp.data());
      if (ierr < 0) {
        std::ostringstream errmsg;
        fmt::print(errmsg,
                   "Problem outputting nodal variable '{}' with index = {} on processor {}\n",
                   var_name, var_index, myProcessor);
        Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__, errmsg.str());
      }
    }
  }
}

void ParallelDatabaseIO::write_entity_transient_field(const Ioss::Field          &field,
                                                      const Ioss::GroupingEntity *ge, int64_t count,
                                                      void *variables) const
{
  static Ioss::Map    non_element_map; // Used as an empty map for ge->type() != element block.
  std::vector<double> temp(count);

  int step = get_current_state();
  step     = get_database_step(step);

  Ioss::Map *map       = nullptr;
  int64_t    eb_offset = 0;
  if (ge->type() == Ioss::ELEMENTBLOCK) {
    const Ioss::ElementBlock *elb = dynamic_cast<const Ioss::ElementBlock *>(ge);
    Ioss::Utils::check_dynamic_cast(elb);
    eb_offset = elb->get_offset();
    map       = &elemMap;
  }
  else {
    map = &non_element_map;
  }

  Ioss::Field::BasicType ioss_type = field.get_type();
  assert(ioss_type == Ioss::Field::REAL || ioss_type == Ioss::Field::INTEGER ||
         ioss_type == Ioss::Field::INT64 || ioss_type == Ioss::Field::COMPLEX);

  if (ioss_type == Ioss::Field::INT64) {
    Ioss::Utils::check_int_to_real_overflow(field, (int64_t *)variables, count);
  }

  // Note that if the field's basic type is COMPLEX, then each component of
  // the VariableType is a complex variable consisting of a real and
  // imaginary part.  Since exodus cannot handle complex variables,
  // we have to output a (real and imaginary) X (number of
  // components) fields. For example, if V is a 3d vector of complex
  // data, the data in the 'variables' array are v_x, v.im_x, v_y,
  // v.im_y, v_z, v.im_z which need to be output in six separate
  // exodus fields.  These fields were already defined in
  // "write_results_metadata".

  // get number of components, cycle through each component
  // and add suffix to base 'field_name'.  Look up index
  // of this name in 'm_variables[type]' map
  int            comp_count = field.get_component_count(Ioss::Field::InOut::OUTPUT);
  ex_entity_type type       = Ioex::map_exodus_type(ge->type());

  int re_im = 1;
  if (ioss_type == Ioss::Field::COMPLEX) {
    re_im = 2;
  }
  for (int complex_comp = 0; complex_comp < re_im; complex_comp++) {
    std::string field_name = field.get_name();
    if (re_im == 2) {
      field_name += complex_suffix[complex_comp];
    }

    for (int i = 0; i < comp_count; i++) {
      std::string var_name = get_component_name(field, Ioss::Field::InOut::OUTPUT, i + 1);

      auto var_iter = m_variables[type].find(var_name);
      if (var_iter == m_variables[type].end()) {
        std::ostringstream errmsg;
        fmt::print(errmsg, "ERROR: Could not find field '{}'\n", var_name);
        IOSS_ERROR(errmsg);
      }
      int var_index = var_iter->second;
      assert(var_index > 0);

      // var is a [count,comp,re_im] array;  re_im = 1(real) or 2(complex)
      // beg_offset = (re_im*i)+complex_comp
      // number_values = count
      // stride = re_im*comp_count
      int64_t begin_offset = (re_im * i) + complex_comp;
      int64_t stride       = re_im * comp_count;

      if (ioss_type == Ioss::Field::REAL || ioss_type == Ioss::Field::COMPLEX) {
        map->map_field_to_db_scalar_order(static_cast<double *>(variables), temp, begin_offset,
                                          count, stride, eb_offset);
      }
      else if (ioss_type == Ioss::Field::INTEGER) {
        map->map_field_to_db_scalar_order(static_cast<int *>(variables), temp, begin_offset, count,
                                          stride, eb_offset);
      }
      else if (ioss_type == Ioss::Field::INT64) {
        map->map_field_to_db_scalar_order(static_cast<int64_t *>(variables), temp, begin_offset,
                                          count, stride, eb_offset);
      }

      // Write the variable...
      size_t proc_offset = ge->get_optional_property("_processor_offset", 0);
      size_t file_count  = ge->get_optional_property("locally_owned_count", count);

      int64_t id = Ioex::get_id(ge, &ids_);
      int     ierr;
      if (type == EX_SIDE_SET) {
        size_t offset = ge->get_property("set_offset").get_int();
        ierr          = ex_put_partial_var(get_file_pointer(), step, type, var_index, id,
                                           proc_offset + offset + 1, count, temp.data());
      }
      else if (type == EX_NODE_SET) {
        std::vector<double> file_data;
        file_data.reserve(file_count);
        map_nodeset_data(nodesetOwnedNodes[ge], temp.data(), file_data);
        ierr = ex_put_partial_var(get_file_pointer(), step, type, var_index, id, proc_offset + 1,
                                  file_count, file_data.data());
      }
      else {
        ierr = ex_put_partial_var(get_file_pointer(), step, type, var_index, id, proc_offset + 1,
                                  file_count, temp.data());
      }

      if (ierr < 0) {
        std::ostringstream extra_info;
        fmt::print(extra_info, "Outputting component {} of field '{}' at step {} on {} '{}'.", i,
                   field_name, fmt::group_digits(step), ge->type_string(), ge->name());
        Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__, extra_info.str());
      }
    }
  }
}

int64_t ParallelDatabaseIO::put_Xset_field_internal(const Ioss::EntitySet *ns,
                                                    const Ioss::Field &field, void *data,
                                                    size_t data_size) const
{
  size_t entity_count = ns->entity_count();
  size_t num_to_get   = field.verify(data_size);

  int64_t               id   = Ioex::get_id(ns, &ids_);
  Ioss::Field::RoleType role = field.get_role();

  if (role == Ioss::Field::MESH) {

    void                *out_data = data;
    std::vector<int>     i32data;
    std::vector<int64_t> i64data;
    std::vector<double>  dbldata;

    size_t proc_offset = ns->get_optional_property("_processor_offset", 0);
    size_t file_count  = ns->get_optional_property("locally_owned_count", num_to_get);

    ex_entity_type type = Ioex::map_exodus_type(ns->type());
    if (field.get_name() == "ids" || field.get_name() == "ids_raw") {
      // Map node id from global node id to local node id.
      // Do it in 'data' ...

      if (field.get_name() == "ids") {
        nodeMap.reverse_map_data(data, field, num_to_get);
      }

      if (type == EX_NODE_SET) {
        nodesetOwnedNodes[ns].reserve(file_count);
        if (int_byte_size_api() == 4) {
          i32data.reserve(file_count);
          check_node_owning_processor_data(nodeOwningProcessor, file_count);
          map_nodeset_id_data(nodeOwningProcessor, nodesetOwnedNodes[ns], myProcessor,
                              reinterpret_cast<int *>(data), num_to_get, i32data);
          assert(i32data.size() == file_count);
          // Maps local to "global_implicit"
          map_local_to_global_implicit(i32data.data(), file_count, nodeGlobalImplicitMap);
          out_data = i32data.data();
        }
        else {
          i64data.reserve(file_count);
          check_node_owning_processor_data(nodeOwningProcessor, file_count);
          map_nodeset_id_data(nodeOwningProcessor, nodesetOwnedNodes[ns], myProcessor,
                              reinterpret_cast<int64_t *>(data), num_to_get, i64data);
          assert(i64data.size() == file_count);
          map_local_to_global_implicit(i64data.data(), file_count, nodeGlobalImplicitMap);
          out_data = i64data.data();
        }
      }
      int ierr = ex_put_partial_set(get_file_pointer(), type, id, proc_offset + 1, file_count,
                                    out_data, nullptr);
      if (ierr < 0) {
        Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
      }
    }
    else if (field.get_name() == "orientation") {
      int ierr = ex_put_partial_set(get_file_pointer(), type, id, proc_offset + 1, file_count,
                                    nullptr, out_data);
      if (ierr < 0) {
        Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
      }
    }
    else if (field.get_name() == "distribution_factors") {
      int ierr = 0;
      if (type == EX_NODE_SET) {
        map_nodeset_data(nodesetOwnedNodes[ns], reinterpret_cast<double *>(data), dbldata);
        ierr = ex_put_partial_set_dist_fact(get_file_pointer(), type, id, proc_offset + 1,
                                            file_count, dbldata.data());
      }
      else {
        ierr = ex_put_partial_set_dist_fact(get_file_pointer(), type, id, proc_offset + 1,
                                            num_to_get, static_cast<double *>(out_data));
      }
      if (ierr < 0) {
        Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
      }
    }
    else {
      num_to_get = Ioss::Utils::field_warning(ns, field, "output");
    }
  }
  else if (role == Ioss::Field::TRANSIENT) {
    // Check if the specified field exists on this element block.
    // Note that 'higher-order' storage types (e.g. SYM_TENSOR)
    // exist on the database as scalars with the appropriate
    // extensions.

    // Transfer each component of the variable into 'data' and then
    // output.  Need temporary storage area of size 'number of
    // elements in this block.
    write_entity_transient_field(field, ns, entity_count, data);
  }
  else if (role == Ioss::Field::ATTRIBUTE) {
    num_to_get = write_attribute_field(field, ns, data);
  }
  else if (role == Ioss::Field::REDUCTION) {
    store_reduction_field(field, ns, data);
  }
  return num_to_get;
}

int64_t ParallelDatabaseIO::put_field_internal(const Ioss::NodeSet *ns, const Ioss::Field &field,
                                               void *data, size_t data_size) const
{
  return put_Xset_field_internal(ns, field, data, data_size);
}

int64_t ParallelDatabaseIO::put_field_internal(const Ioss::EdgeSet *ns, const Ioss::Field &field,
                                               void *data, size_t data_size) const
{
  return put_Xset_field_internal(ns, field, data, data_size);
}

int64_t ParallelDatabaseIO::put_field_internal(const Ioss::FaceSet *ns, const Ioss::Field &field,
                                               void *data, size_t data_size) const
{
  return put_Xset_field_internal(ns, field, data, data_size);
}

int64_t ParallelDatabaseIO::put_field_internal(const Ioss::ElementSet *ns, const Ioss::Field &field,
                                               void *data, size_t data_size) const
{
  return put_Xset_field_internal(ns, field, data, data_size);
}

int64_t ParallelDatabaseIO::put_field_internal(const Ioss::SideSet *ss, const Ioss::Field &field,
                                               void * /* data */, size_t data_size) const
{
  size_t num_to_get = field.verify(data_size);
  if (field.get_name() == "ids") {
    // Do nothing, just handles an idiosyncrasy of the GroupingEntity
  }
  else {
    num_to_get = Ioss::Utils::field_warning(ss, field, "output");
  }
  return num_to_get;
}

int64_t ParallelDatabaseIO::put_field_internal(const Ioss::CommSet * /*cs*/,
                                               const Ioss::Field &field, void * /*data*/,
                                               size_t             data_size) const
{
  size_t num_to_get = field.verify(data_size);
  return num_to_get;
}

int64_t ParallelDatabaseIO::put_field_internal(const Ioss::SideBlock *sb, const Ioss::Field &field,
                                               void *data, size_t data_size) const
{
  size_t  num_to_get = field.verify(data_size);
  int64_t id         = Ioex::get_id(sb, &ids_);

  size_t entity_count = sb->entity_count();
  size_t offset       = sb->get_property("set_offset").get_int();

  Ioss::Field::RoleType role = field.get_role();

  if (role == Ioss::Field::MESH) {
    if (field.get_name() == "side_ids" && sb->name() == "universal_sideset") {
      // The side ids are being stored as the distribution factor
      // field on the universal sideset.  There should be no other
      // side sets that request this field...  (Eventually,
      // create an id field to store this info.

      // Need to convert 'ints' to 'double' for storage on mesh...
      // FIX 64
      if (field.get_type() == Ioss::Field::INTEGER) {
        int                *ids = static_cast<int *>(data);
        std::vector<double> real_ids(num_to_get);
        for (size_t i = 0; i < num_to_get; i++) {
          real_ids[i] = static_cast<double>(ids[i]);
        }
        int ierr = ex_put_partial_set_dist_fact(get_file_pointer(), EX_SIDE_SET, id, offset + 1,
                                                entity_count, real_ids.data());
        if (ierr < 0) {
          Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
        }
      }
      else {
        int64_t            *ids = static_cast<int64_t *>(data);
        std::vector<double> real_ids(num_to_get);
        for (size_t i = 0; i < num_to_get; i++) {
          real_ids[i] = static_cast<double>(ids[i]);
        }
        int ierr = ex_put_partial_set_dist_fact(get_file_pointer(), EX_SIDE_SET, id, offset + 1,
                                                entity_count, real_ids.data());
        if (ierr < 0) {
          Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
        }
      }
    }

    else if (field.get_name() == "side_ids") {
    }

    else if (field.get_name() == "ids") {
      // =============================================================
      // NOTE: Code is currently commented out since we have
      // redundant ways of getting the data (element/side) out to
      // the database.  The 'ids' field method relies on a numbering
      // kluge, so for now trying the 'element_side' field...
      // =============================================================
    }

    else if (field.get_name() == "distribution_factors") {
      int    ierr;
      size_t df_offset      = sb->get_property("set_df_offset").get_int();
      size_t proc_df_offset = sb->get_property("processor_df_offset").get_int();
      size_t df_count       = sb->get_property("distribution_factor_count").get_int();
      ierr                  = ex_put_partial_set_dist_fact(get_file_pointer(), EX_SIDE_SET, id,
                                                           proc_df_offset + df_offset + 1, df_count,
                                                           static_cast<double *>(data));
      if (ierr < 0) {
        Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
      }
    }
    else if (field.get_name() == "element_side") {
      // In exodusII, the 'side block' is stored as a sideset.  A
      // sideset has a list of elements and a corresponding local
      // element side (1-based)

      // The 'data' passed into the function is stored as a
      // 2D vector e0,f0,e1,f1,... (e=element, f=side)

      // To avoid overwriting the passed in data, we allocate
      // two arrays to store the data for this sideset.

      // The element_id passed in is the global id; we need to
      // output the local id.

      // Allocate space for local side number and element numbers
      // numbers.
      // See if edges or faces...
      size_t side_offset = Ioss::Utils::get_side_offset(sb);

      size_t index = 0;

      size_t proc_offset = sb->get_optional_property("_processor_offset", 0);

      if (field.get_type() == Ioss::Field::INTEGER) {
        Ioss::IntVector element(num_to_get);
        Ioss::IntVector side(num_to_get);
        int            *el_side = reinterpret_cast<int *>(data);

        for (size_t i = 0; i < num_to_get; i++) {
          element[i] = elemMap.global_to_local(el_side[index++]);
          side[i]    = el_side[index++] + side_offset;
        }

        map_local_to_global_implicit(element.data(), num_to_get, elemGlobalImplicitMap);
        int ierr = ex_put_partial_set(get_file_pointer(), EX_SIDE_SET, id, proc_offset + offset + 1,
                                      num_to_get, element.data(), side.data());
        if (ierr < 0) {
          Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
        }
      }
      else {
        Ioss::Int64Vector element(num_to_get);
        Ioss::Int64Vector side(num_to_get);
        int64_t          *el_side = reinterpret_cast<int64_t *>(data);

        for (size_t i = 0; i < num_to_get; i++) {
          element[i] = elemMap.global_to_local(el_side[index++]);
          side[i]    = el_side[index++] + side_offset;
        }

        map_local_to_global_implicit(element.data(), num_to_get, elemGlobalImplicitMap);
        int ierr = ex_put_partial_set(get_file_pointer(), EX_SIDE_SET, id, proc_offset + offset + 1,
                                      num_to_get, element.data(), side.data());
        if (ierr < 0) {
          Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
        }
      }
    }
    else if (field.get_name() == "element_side_raw") {
      // In exodusII, the 'side block' is stored as a sideset.  A
      // sideset has a list of elements and a corresponding local
      // element side (1-based)

      // The 'data' passed into the function is stored as a
      // 2D vector e0,f0,e1,f1,... (e=element, f=side)

      // To avoid overwriting the passed in data, we allocate
      // two arrays to store the data for this sideset.

      // The element_id passed in is the local id.

      // See if edges or faces...
      size_t side_offset = Ioss::Utils::get_side_offset(sb);

      size_t index = 0;
      if (field.get_type() == Ioss::Field::INTEGER) {
        Ioss::IntVector element(num_to_get);
        Ioss::IntVector side(num_to_get);
        int            *el_side = reinterpret_cast<int *>(data);

        for (size_t i = 0; i < num_to_get; i++) {
          element[i] = el_side[index++];
          side[i]    = el_side[index++] + side_offset;
        }

        int ierr = ex_put_partial_set(get_file_pointer(), EX_SIDE_SET, id, offset + 1, entity_count,
                                      element.data(), side.data());
        if (ierr < 0) {
          Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
        }
      }
      else {
        Ioss::Int64Vector element(num_to_get);
        Ioss::Int64Vector side(num_to_get);
        int64_t          *el_side = reinterpret_cast<int64_t *>(data);

        for (size_t i = 0; i < num_to_get; i++) {
          element[i] = el_side[index++];
          side[i]    = el_side[index++] + side_offset;
        }

        int ierr = ex_put_partial_set(get_file_pointer(), EX_SIDE_SET, id, offset + 1, entity_count,
                                      element.data(), side.data());
        if (ierr < 0) {
          Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
        }
      }
    }
    else if (field.get_name() == "connectivity") {
      // Do nothing, just handles an idiosyncrasy of the GroupingEntity
    }
    else if (field.get_name() == "connectivity_raw") {
      // Do nothing, just handles an idiosyncrasy of the GroupingEntity
    }
    else {
      num_to_get = Ioss::Utils::field_warning(sb, field, "output");
    }
  }
  else if (role == Ioss::Field::TRANSIENT) {
    // Check if the specified field exists on this block.
    // Note that 'higher-order' storage types (e.g. SYM_TENSOR)
    // exist on the database as scalars with the appropriate
    // extensions.

    // Transfer each component of the variable into 'data' and then
    // output.  Need temporary storage area of size 'number of
    // entities in this block.
    write_entity_transient_field(field, sb, entity_count, data);
  }
  else if (role == Ioss::Field::ATTRIBUTE) {
    num_to_get = write_attribute_field(field, sb, data);
  }
  else if (role == Ioss::Field::REDUCTION) {
    store_reduction_field(field, sb, data);
  }
  return num_to_get;
}

void ParallelDatabaseIO::write_meta_data(Ioss::IfDatabaseExistsBehavior behavior)
{
  Ioss::Region *region = get_region();
  common_write_meta_data(behavior);

  char the_title[max_line_length + 1];

  // Title...
  if (region->property_exists("title")) {
    std::string title_str = region->get_property("title").get_string();
    Ioss::Utils::copy_string(the_title, title_str);
  }
  else {
    Ioss::Utils::copy_string(the_title, "IOSS Default Output Title");
  }

  bool       file_per_processor = false;
  Ioex::Mesh mesh(spatialDimension, the_title, util(), file_per_processor);
  mesh.populate(region);

  if (behavior != Ioss::DB_APPEND && behavior != Ioss::DB_MODIFY) {
    bool omit_qa = false;
    Ioss::Utils::check_set_bool_property(properties, "OMIT_QA_RECORDS", omit_qa);
    if (!omit_qa) {
      put_qa();
    }

    bool omit_info = false;
    Ioss::Utils::check_set_bool_property(properties, "OMIT_INFO_RECORDS", omit_info);
    if (!omit_info) {
      put_info();
    }

    // Write the metadata to the exodusII file...
    Ioex::Internals data(get_file_pointer(), maximumNameLength, util());
    mesh.comm.outputNemesis = false;

    int ierr = data.write_meta_data(mesh);

    if (ierr < 0) {
      Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
    }
  }

  metaDataWritten = true;

  // Set the processor offset property. Specifies where in the global list, the data from this
  // processor begins...
  update_processor_offset_property(region, mesh);

  if (behavior != Ioss::DB_APPEND && behavior != Ioss::DB_MODIFY) {
    output_node_map();
    output_other_meta_data();
  }
}

void ParallelDatabaseIO::create_implicit_global_map() const
{
  // If the node is locally owned, then its position is basically
  // determined by removing all shared nodes from the list and
  // then compressing the list. This location plus the proc_offset
  // gives its location in the global-implicit file.
  //
  // Do this over in the DecompositionData class since it has
  // several utilities in place for MPI communication.

  DecompositionData<int64_t> compose(Ioss::PropertyManager(), util().communicator());
  int64_t                    locally_owned_count = 0;
  int64_t                    processor_offset    = 0;
  compose.create_implicit_global_map(nodeOwningProcessor, nodeGlobalImplicitMap, nodeMap,
                                     &locally_owned_count, &processor_offset);

  nodeGlobalImplicitMapDefined                = true;
  const Ioss::NodeBlockContainer &node_blocks = get_region()->get_node_blocks();
  if (!node_blocks[0]->property_exists("locally_owned_count")) {
    node_blocks[0]->property_add(Ioss::Property("locally_owned_count", locally_owned_count));
  }
  if (!node_blocks[0]->property_exists("_processor_offset")) {
    node_blocks[0]->property_add(Ioss::Property("_processor_offset", processor_offset));
  }

  output_node_map();
}

void ParallelDatabaseIO::output_node_map() const
{
  // Write the partial nodemap to the database...  This is called
  // two times -- once from create_implicit_global_map() and once
  // from write_meta_data().  It will only output the map if
  // the metadata has been written to the output database AND if
  // the nodeMap.map and nodeGlobalImplicitMap are defined.

  if (metaDataWritten) {
    const Ioss::NodeBlockContainer &node_blocks = get_region()->get_node_blocks();
    if (node_blocks.empty()) {
      return;
    }
    assert(node_blocks[0]->property_exists("_processor_offset"));
    assert(node_blocks[0]->property_exists("locally_owned_count"));
    size_t processor_offset    = node_blocks[0]->get_property("_processor_offset").get_int();
    size_t locally_owned_count = node_blocks[0]->get_property("locally_owned_count").get_int();

    int ierr = 0;
    if (nodeMap.defined() && nodeGlobalImplicitMapDefined) {

      if (int_byte_size_api() == 4) {
        std::vector<int> file_ids;
        file_ids.reserve(locally_owned_count);
        check_node_owning_processor_data(nodeOwningProcessor, locally_owned_count);
        filter_owned_nodes(nodeOwningProcessor, myProcessor, &nodeMap.map()[1], file_ids);
        ierr = ex_put_partial_id_map(get_file_pointer(), EX_NODE_MAP, processor_offset + 1,
                                     locally_owned_count, file_ids.data());
      }
      else {
        std::vector<int64_t> file_ids;
        file_ids.reserve(locally_owned_count);
        check_node_owning_processor_data(nodeOwningProcessor, locally_owned_count);
        filter_owned_nodes(nodeOwningProcessor, myProcessor, &nodeMap.map()[1], file_ids);
        ierr = ex_put_partial_id_map(get_file_pointer(), EX_NODE_MAP, processor_offset + 1,
                                     locally_owned_count, file_ids.data());
      }
    }
    if (ierr < 0) {
      Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
    }
  }
}

void ParallelDatabaseIO::check_valid_values() const
{
  std::vector<int64_t> counts{nodeCount, elementCount, m_groupCount[EX_ELEM_BLOCK]};
  std::vector<int64_t> all_counts;
  util().all_gather(counts, all_counts);
  // Get minimum value in `all_counts`. If >0, then don't need to check further...
  auto min_val = *std::min_element(all_counts.begin(), all_counts.end());

  if (myProcessor == 0) {
    size_t proc_count = all_counts.size() / 3;

    if (min_val < 0) {
      static std::array<std::string, 3> label{"node", "element", "element block"};
      // Error on one or more of the counts...
      for (size_t j = 0; j < 3; j++) {
        std::vector<size_t> bad_proc;
        for (size_t i = 0; i < proc_count; i++) {
          if (all_counts[3 * i + j] < 0) {
            bad_proc.push_back(i);
          }
        }

        if (!bad_proc.empty()) {
          std::ostringstream errmsg;
          fmt::print(errmsg, "ERROR: Negative {} count on {} processor{}:\n\t{}\n\n", label[j],
                     bad_proc.size(), bad_proc.size() > 1 ? "s" : "",
                     Ioss::Utils::format_id_list(bad_proc, ":"));
          IOSS_ERROR(errmsg);
        }
      }
    }

    // Now check for warning (count == 0)
    if (min_val <= 0) {
      static std::array<std::string, 3> label{"nodes or elements", "elements", "element blocks"};
      // Possible warning on one or more of the counts...
      // Note that it is possible to have nodes on a processor with no elements,
      // but not possible to have elements if no nodes...
      for (size_t j = 0; j < 3; j++) {
        std::vector<size_t> bad_proc;
        for (size_t i = 0; i < proc_count; i++) {
          if (all_counts[3 * i + j] == 0) {
            bad_proc.push_back(i);
          }
        }

        if (!bad_proc.empty()) {
          fmt::print(Ioss::WarnOut(), "No {} on processor{}:\n\t{}\n\n", label[j],
                     bad_proc.size() > 1 ? "s" : "", Ioss::Utils::format_id_list(bad_proc, ":"));
          if (j == 0) {
            break;
          }
        }
      }
    }
  }
  else { // All other processors; need to abort if negative count
    if (min_val < 0) {
      std::ostringstream errmsg;
      IOSS_ERROR(errmsg);
    }
  }
}
} // namespace Ioex
#else
const char ioss_exodus_parallel_database_unused_symbol_dummy = '\0';
#endif
