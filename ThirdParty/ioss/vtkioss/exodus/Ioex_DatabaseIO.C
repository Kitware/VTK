// Copyright(C) 1999-2017, 2020 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//
//     * Neither the name of NTESS nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <Ioss_CodeTypes.h>
#include <Ioss_FileInfo.h>
#include <Ioss_ParallelUtils.h>
#include <Ioss_SerializeIO.h>
#include <Ioss_SurfaceSplit.h>
#include <Ioss_Utils.h>
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cfloat>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <exodus/Ioex_DatabaseIO.h>
#include <exodus/Ioex_Internals.h>
#include <exodus/Ioex_Utils.h>
#include <vtk_exodusII.h>
#include <fmt/ostream.h>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <numeric>
#include <set>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <tokenize.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <utility>
#include <vector>

#include "Ioss_Assembly.h"
#include "Ioss_Blob.h"
#include "Ioss_CommSet.h"
#include "Ioss_CoordinateFrame.h"
#include "Ioss_DBUsage.h"
#include "Ioss_DatabaseIO.h"
#include "Ioss_EdgeBlock.h"
#include "Ioss_EdgeSet.h"
#include "Ioss_ElementBlock.h"
#include "Ioss_ElementSet.h"
#include "Ioss_EntityBlock.h"
#include "Ioss_EntitySet.h"
#include "Ioss_EntityType.h"
#include "Ioss_FaceBlock.h"
#include "Ioss_FaceSet.h"
#include "Ioss_Field.h"
#include "Ioss_GroupingEntity.h"
#include "Ioss_Map.h"
#include "Ioss_NodeBlock.h"
#include "Ioss_NodeSet.h"
#include "Ioss_Property.h"
#include "Ioss_Region.h"
#include "Ioss_SideBlock.h"
#include "Ioss_SideSet.h"
#include "Ioss_State.h"
#include "Ioss_VariableType.h"

// ========================================================================
// Static internal helper functions
// ========================================================================
namespace {
  const size_t max_line_length = MAX_LINE_LENGTH;

  std::string SEP() { return std::string("@"); } // Separator for attribute offset storage
  const char *complex_suffix[] = {".re", ".im"};

  void get_connectivity_data(int exoid, void *data, ex_entity_type type, ex_entity_id id,
                             int position)
  {
    int ierr = 0;
    if ((ex_int64_status(exoid) & EX_BULK_INT64_API) != 0) {
      int64_t *conn[3];
      conn[0]        = nullptr;
      conn[1]        = nullptr;
      conn[2]        = nullptr;
      conn[position] = static_cast<int64_t *>(data);
      ierr           = ex_get_conn(exoid, type, id, conn[0], conn[1], conn[2]);
    }
    else {
      int *conn[3];
      conn[0]        = nullptr;
      conn[1]        = nullptr;
      conn[2]        = nullptr;
      conn[position] = static_cast<int *>(data);
      ierr           = ex_get_conn(exoid, type, id, conn[0], conn[1], conn[2]);
    }
    if (ierr < 0) {
      Ioex::exodus_error(exoid, __LINE__, __func__, __FILE__);
    }
  }

  template <typename T>
  void compute_internal_border_maps(T *entities, T *internal, size_t count, size_t entity_count)
  {
    // Construct the node/element map (internal vs. border).
    // Border nodes/elements are those in the communication map (use entities array)
    // Internal nodes/elements are the rest.  Allocate array to hold all nodes/elements,
    // initialize all to '1', then zero out the nodes/elements in 'entities'.
    // Iterate through array again and consolidate all '1's
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

  template <typename T>
  void extract_data(std::vector<double> &local_data, T *data, size_t num_entity, size_t comp_count,
                    size_t offset)
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

} // namespace

namespace Ioex {
  DatabaseIO::DatabaseIO(Ioss::Region *region, const std::string &filename,
                         Ioss::DatabaseUsage db_usage, MPI_Comm communicator,
                         const Ioss::PropertyManager &props)
      : Ioex::BaseDatabaseIO(region, filename, db_usage, communicator, props)
  {
    if (!is_input()) {
      // Check whether appending to existing file...
      if (open_create_behavior() == Ioss::DB_APPEND ||
          open_create_behavior() == Ioss::DB_APPEND_GROUP) {
        // Append to file if it already exists -- See if the file exists.
        Ioss::FileInfo file = Ioss::FileInfo(decoded_filename());
        fileExists          = file.exists();
      }
    }

    if (properties.exists("processor_count") && properties.exists("my_processor")) {
      if (!isParallel) {
        isSerialParallel = true;
      }
      else {
        std::ostringstream errmsg;
        fmt::print(
            errmsg,
            "ERROR: Processor id and processor count are specified via the "
            "'processor_count' and 'processor_id' properties which indicates that this "
            "database is "
            "being run in 'serial-parallel' mode, but the database constructor was passed an "
            "mpi communicator which has more than 1 processor. This is not allowed.\n");
        IOSS_ERROR(errmsg);
      }
    }
  }

  bool DatabaseIO::check_valid_file_ptr(bool write_message, std::string *error_msg, int *bad_count,
                                        bool abort_if_error) const
  {
    bool no_collective_calls = Ioss::SerializeIO::isEnabled();
    if (isParallel && no_collective_calls) {
      // Can't output a nice error message on processor 0 and throw a consistent error.
      // Have to just write message on processors that have issue and throw exception.
      if (exodusFilePtr < 0) {
        std::ostringstream errmsg;
        std::string        open_create = is_input() ? "open input" : "create output";
        fmt::print(errmsg, "ERROR: Unable to {} exodus decomposed database file '{}'\n",
                   open_create, decoded_filename());

        if (abort_if_error) {
          IOSS_ERROR(errmsg);
        }
        else {
          Ioss::WARNING() << errmsg.str();
        }
        return false;
      }
      return true; // At least on this processor...
    }

    // Check for valid exodus_file_ptr (valid >= 0; invalid < 0)
    int global_file_ptr = exodusFilePtr;
    if (isParallel) {
      global_file_ptr = util().global_minmax(exodusFilePtr, Ioss::ParallelUtils::DO_MIN);
    }

    if (global_file_ptr < 0) {
      if (write_message || error_msg != nullptr || bad_count != nullptr) {
        Ioss::IntVector status;
        if (isParallel) {
          util().all_gather(exodusFilePtr, status);
        }
        else {
          status.push_back(exodusFilePtr);
        }

        std::string open_create = is_input() ? "open input" : "create output";
        if (write_message || error_msg != nullptr) {
          // See which processors could not open/create the file...
          std::ostringstream errmsg;
          if (isParallel) {
            fmt::print(errmsg, "ERROR: Unable to {} exodus decomposed database files:\n",
                       open_create);
            for (int i = 0; i < util().parallel_size(); i++) {
              if (status[i] < 0) {
                fmt::print(errmsg, "\t{}\n",
                           Ioss::Utils::decode_filename(get_filename(), i, util().parallel_size()));
              }
            }
          }
          else {
            fmt::print(errmsg, "ERROR: Unable to {} database '{}' of type 'exodusII'", open_create,
                       get_filename());
          }
          fmt::print(errmsg, "\n");
          if (error_msg != nullptr) {
            *error_msg = errmsg.str();
          }
          if (write_message && myProcessor == 0) {
            Ioss::WARNING() << errmsg.str();
          }
        }
        if (bad_count != nullptr) {
          *bad_count = std::count_if(status.begin(), status.end(), [](int i) { return i < 0; });
        }
        if (abort_if_error) {
          std::ostringstream errmsg;
          fmt::print(errmsg, "ERROR: Cannot {} file '{}'\n", open_create, get_filename());
          IOSS_ERROR(errmsg);
        }
      }
      return false;
    }
    return true;
  }

  bool DatabaseIO::open_input_file(bool write_message, std::string *error_msg, int *bad_count,
                                   bool abort_if_error) const
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

    bool do_timer = false;
    Ioss::Utils::check_set_bool_property(properties, "IOSS_TIME_FILE_OPEN_CLOSE", do_timer);
    double t_begin = (do_timer ? Ioss::Utils::timer() : 0);

    int app_opt_val = ex_opts(EX_VERBOSE);
    exodusFilePtr   = ex_open(decoded_filename().c_str(), EX_READ | mode, &cpu_word_size,
                            &io_word_size, &version);

    if (do_timer) {
      double t_end    = Ioss::Utils::timer();
      double duration = t_end - t_begin;
      fmt::print(Ioss::DEBUG(), "Input File Open Time = {}\n", duration);
    }

    bool is_ok = check_valid_file_ptr(write_message, error_msg, bad_count, abort_if_error);

    if (is_ok) {
      finalize_file_open();
    }
    ex_opts(app_opt_val); // Reset back to what it was.
    return is_ok;
  }

  bool DatabaseIO::handle_output_file(bool write_message, std::string *error_msg, int *bad_count,
                                      bool overwrite, bool abort_if_error) const
  {
    // If 'overwrite' is false, we do not want to overwrite or clobber
    // the output file if it already exists since the app might be
    // reading the restart data from this file and then later
    // clobbering it and then writing restart data to the same
    // file. So, for output, we first check whether the file exists
    // and if it it and is writable, assume that we can later create a
    // new or append to existing file.

    // if 'overwrite' is true, then clobber/append
    bool is_ok = false;

    if (!overwrite) {
      // check if file exists and is writeable. If so, return true.
      Ioss::FileInfo file(decoded_filename());
      int            int_is_ok = file.exists() && file.is_writable() ? 1 : 0;

      // Check for consistency among all processors.
      // OK if *all* 0 or *all* 1
      int sum = util().global_minmax(int_is_ok, Ioss::ParallelUtils::DO_SUM);
      if (sum == util().parallel_size()) {
        // Note that at this point, we cannot totally guarantee that
        // we will be able to create the file when needed, but we have
        // a pretty good chance.  We can't guarantee creation without
        // creating and the app (or calling function) doesn't want us to overwrite...
        return true;
      }
      // File doesn't exist on any or all processors, so fall through and try to
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
    int app_opt_val = ex_opts(EX_VERBOSE);
    if (fileExists) {
      exodusFilePtr = ex_open(decoded_filename().c_str(), EX_WRITE | mode, &cpu_word_size,
                              &io_word_size, &version);
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
      exodusFilePtr = ex_create(decoded_filename().c_str(), mode, &cpu_word_size, &dbRealWordSize);
    }

    is_ok = check_valid_file_ptr(write_message, error_msg, bad_count, abort_if_error);

    if (is_ok) {
      ex_set_max_name_length(exodusFilePtr, maximumNameLength);

      // Check properties handled post-create/open...
      if (properties.exists("COMPRESSION_LEVEL")) {
        int comp_level = properties.get("COMPRESSION_LEVEL").get_int();
        ex_set_option(exodusFilePtr, EX_OPT_COMPRESSION_LEVEL, comp_level);
      }
      if (properties.exists("COMPRESSION_SHUFFLE")) {
        int shuffle = properties.get("COMPRESSION_SHUFFLE").get_int();
        ex_set_option(exodusFilePtr, EX_OPT_COMPRESSION_SHUFFLE, shuffle);
      }
    }
    ex_opts(app_opt_val); // Reset back to what it was.
    return is_ok;
  }

  int DatabaseIO::get_file_pointer() const
  {
    // Returns the file_pointer used to access the file on disk.
    // Checks that the file is open and if not, opens it first.
    if (Ioss::SerializeIO::isEnabled()) {
      if (!Ioss::SerializeIO::inBarrier()) {
        std::ostringstream errmsg;
        fmt::print(errmsg, "ERROR: Process {} is attempting to do I/O without serialized I/O",
                   Ioss::SerializeIO::getRank());
        IOSS_ERROR(errmsg);
      }

      if (!Ioss::SerializeIO::inMyGroup()) {
        std::ostringstream errmsg;
        fmt::print(errmsg, "ERROR: Process {} is attempting to do I/O while {} owns the token",
                   Ioss::SerializeIO::getRank(), Ioss::SerializeIO::getOwner());
        IOSS_ERROR(errmsg);
      }
    }

    return Ioex::BaseDatabaseIO::get_file_pointer();
  }

  void DatabaseIO::read_meta_data__()
  {
    // If this is a HISTORY file, there isn't really any metadata
    // Other than a single node and single element.  Just hardwire
    // it here (needed when appending to existing history file)
    if (dbUsage == Ioss::WRITE_HISTORY) {
      if (myProcessor == 0) {
        nodeCount           = 1;
        elementCount        = 1;
        Ioss::NodeBlock *nb = new Ioss::NodeBlock(this, "nodeblock_1", 1, 3);
        get_region()->add(nb);

        // Element Block
        Ioss::ElementBlock *eb = new Ioss::ElementBlock(this, "e1", "sphere", 1);
        eb->property_add(Ioss::Property("id", 1));
        eb->property_add(Ioss::Property("guid", util().generate_guid(1)));
        get_region()->add(eb);
        get_step_times__();
        add_region_fields();
      }
      return;
    }

    {
      Ioss::SerializeIO serializeIO__(this);

      if (isParallel) {
        Ioex::check_processor_info(get_file_pointer(), util().parallel_size(), myProcessor);
      }

      read_region();
      read_communication_metadata();
    }

    get_step_times__();

    get_nodeblocks();
    get_edgeblocks();
    get_faceblocks();
    get_elemblocks();

    check_side_topology();

    get_sidesets();
    get_nodesets();
    get_edgesets();
    get_facesets();
    get_elemsets();

    get_commsets();

    // Add assemblies now that all entities should be defined... consistent across processors
    // (metadata)
    get_assemblies();
    get_blobs();

    handle_groups();

    add_region_fields();

    if (!is_input() && open_create_behavior() == Ioss::DB_APPEND) {
      get_map(EX_NODE_BLOCK);
      get_map(EX_EDGE_BLOCK);
      get_map(EX_FACE_BLOCK);
      get_map(EX_ELEM_BLOCK);
    }
  }

  void DatabaseIO::read_region()
  {
    // Add properties and fields to the 'owning' region.
    // Also defines member variables of this class...
    ex_init_params info{};
    int            error = ex_get_init_ext(get_file_pointer(), &info);
    if (error < 0) {
      Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
    }

    spatialDimension = info.num_dim;
    nodeCount        = info.num_nodes;
    edgeCount        = info.num_edge;
    faceCount        = info.num_face;
    elementCount     = info.num_elem;

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

    if (nodeCount == 0) {
      fmt::print(Ioss::WARNING(), "No nodes were found in the model, file '{}'\n",
                 decoded_filename());
    }
    else if (nodeCount < 0) {
      // NOTE: Code will not continue past this call...
      std::ostringstream errmsg;
      fmt::print(errmsg,
                 "ERROR: Negative node count was found in the model\n"
                 "       File: '{}'.\n",
                 decoded_filename());
      IOSS_ERROR(errmsg);
    }

    if (elementCount == 0) {
      fmt::print(Ioss::WARNING(), "No elements were found in the model, file '{}'\n",
                 decoded_filename());
    }

    if (elementCount < 0) {
      // NOTE: Code will not continue past this call...
      std::ostringstream errmsg;
      fmt::print(errmsg,
                 "ERROR: Negative element count was found in the model\n"
                 "       File: '{}'.\n",
                 decoded_filename());
      IOSS_ERROR(errmsg);
    }

    if (elementCount > 0 && m_groupCount[EX_ELEM_BLOCK] <= 0) {
      // NOTE: Code will not continue past this call...
      std::ostringstream errmsg;
      fmt::print(errmsg,
                 "ERROR: No element blocks were found in the model\n"
                 "       File: '{}'.\n",
                 decoded_filename());
      IOSS_ERROR(errmsg);
    }

    Ioss::Region *this_region = get_region();

    // See if any coordinate frames exist on mesh.  If so, define them on region.
    Ioex::add_coordinate_frames(get_file_pointer(), this_region);

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

  void DatabaseIO::get_step_times__()
  {
    bool                exists         = false;
    double              last_time      = DBL_MAX;
    int                 timestep_count = 0;
    std::vector<double> tsteps(0);

    if (dbUsage == Ioss::WRITE_HISTORY) {
      if (myProcessor == 0) {
        timestep_count = ex_inquire_int(get_file_pointer(), EX_INQ_TIME);
        if (timestep_count <= 0) {
          return;
        }

        // For an exodus file, timesteps are global and are stored in the region.
        // A history file only stores that last time / step
        // Read the timesteps and add them to the region.
        // Since we can't access the Region's stateCount directly, we just add
        // all of the steps and assume the Region is dealing with them directly...
        tsteps.resize(timestep_count);
        int error = ex_get_all_times(get_file_pointer(), tsteps.data());
        if (error < 0) {
          Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
        }

        int max_step = timestep_count;
        if (properties.exists("APPEND_OUTPUT_AFTER_STEP")) {
          max_step = properties.get("APPEND_OUTPUT_AFTER_STEP").get_int();
        }
        if (max_step > timestep_count) {
          max_step = timestep_count;
        }

        double max_time = std::numeric_limits<double>::max();
        if (properties.exists("APPEND_OUTPUT_AFTER_TIME")) {
          max_time = properties.get("APPEND_OUTPUT_AFTER_TIME").get_real();
        }

        Ioss::Region *this_region = get_region();
        for (int i = 0; i < max_step; i++) {
          if (tsteps[i] <= max_time) {
            this_region->add_state__(tsteps[i] * timeScaleFactor);
          }
        }
      }
    }
    else {
      {
        Ioss::SerializeIO serializeIO__(this);
        timestep_count = ex_inquire_int(get_file_pointer(), EX_INQ_TIME);
        if (timestep_count <= 0) {
          return;
        }

        // For an exodus file, timesteps are global and are stored in the region.
        // Read the timesteps and add to the region
        tsteps.resize(timestep_count);
        int error = ex_get_all_times(get_file_pointer(), tsteps.data());
        if (error < 0) {
          Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
        }

        // See if the "last_written_time" attribute exists and if it
        // does, check that it matches the largest time in 'tsteps'.
        exists = Ioex::read_last_time_attribute(get_file_pointer(), &last_time);
      }
      if (exists && isParallel) {
        // Assume that if it exists on 1 processor, it exists on
        // all... Sync value among processors since could have a
        // corrupt step on only a single database.
        last_time = util().global_minmax(last_time, Ioss::ParallelUtils::DO_MIN);
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
      int max_step = timestep_count;
      if (properties.exists("APPEND_OUTPUT_AFTER_STEP")) {
        max_step = properties.get("APPEND_OUTPUT_AFTER_STEP").get_int();
      }
      if (max_step > timestep_count) {
        max_step = timestep_count;
      }

      double max_time = std::numeric_limits<double>::max();
      if (properties.exists("APPEND_OUTPUT_AFTER_TIME")) {
        max_time = properties.get("APPEND_OUTPUT_AFTER_TIME").get_real();
      }
      if (last_time > max_time) {
        last_time = max_time;
      }

      Ioss::Region *this_region = get_region();
      for (int i = 0; i < max_step; i++) {
        if (tsteps[i] <= last_time) {
          this_region->add_state__(tsteps[i] * timeScaleFactor);
        }
        else {
          if (myProcessor == 0 && max_time == std::numeric_limits<double>::max()) {
            // NOTE: Don't want to warn on all processors if there are
            // corrupt steps on all databases, but this will only print
            // a warning if there is a corrupt step on processor
            // 0... Need better warnings which won't overload in the
            // worst case...
            fmt::print(Ioss::WARNING(),
                       "Skipping step {:n} at time {} in database file\n\t{}.\n"
                       "\tThe data for that step is possibly corrupt since the last time written "
                       "successfully was {}.\n",
                       i + 1, tsteps[i], get_filename(), last_time);
          }
        }
      }
    }
  }

  void DatabaseIO::read_communication_metadata()
  {
    // Check that file is nemesis.
    int  num_proc;         // Number of processors file was decomposed for
    int  num_proc_in_file; // Number of processors this file has info for
    char file_type[2];     // "s" for scalar, "p" for parallel

    // Get global data (over all processors)
    int64_t global_nodes    = nodeCount;
    int64_t global_elements = elementCount;
    int64_t global_eblocks  = 0; // unused
    int64_t global_nsets    = 0; // unused
    int64_t global_ssets    = 0; // unused

    int64_t num_external_nodes; // unused
    int64_t num_elem_cmaps     = 0;
    int64_t num_node_cmaps     = 0;
    int64_t num_internal_nodes = nodeCount;
    int64_t num_border_nodes   = 0;
    int64_t num_internal_elems = elementCount;
    int64_t num_border_elems   = 0;

    bool nemesis_file = true;
    int  error = ex_get_init_info(get_file_pointer(), &num_proc, &num_proc_in_file, &file_type[0]);
    if (error < 0) {
      Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
    }

    if (num_proc == 1 && num_proc_in_file == 1) {
      // Not a nemesis file
      nemesis_file = false;
      if (isParallel && util().parallel_size() > 1) {
        std::ostringstream errmsg;
        fmt::print(errmsg, "ERROR: Exodus file does not contain nemesis information.\n");
        IOSS_ERROR(errmsg);
      }
      file_type[0] = 'p';
    }
    else {
      if (!isParallel) {
        // The file contains nemesis parallel information.
        // Even though we are running in serial, make the information
        // available to the application.
        isSerialParallel = true;
        get_region()->property_add(Ioss::Property("processor_count", num_proc));
      }
    }

    if (isParallel && num_proc != util().parallel_size() && util().parallel_size() > 1) {
      std::ostringstream errmsg;
      fmt::print(errmsg,
                 "ERROR: Exodus file was decomposed for {} processors; application is currently "
                 "being run on {} processors",
                 num_proc, util().parallel_size());
      IOSS_ERROR(errmsg);
    }
    if (num_proc_in_file != 1) {
      std::ostringstream errmsg;
      fmt::print(errmsg,
                 "ERROR: Exodus file contains data for {} processors; application requires 1 "
                 "processor per file.",
                 num_proc_in_file);
      IOSS_ERROR(errmsg);
    }
    if (file_type[0] != 'p') {
      std::ostringstream errmsg;
      fmt::print(errmsg,
                 "ERROR: Exodus file contains scalar nemesis data; application requires parallel "
                 "nemesis data.");
      IOSS_ERROR(errmsg);
    }

    if (nemesis_file) {
      if (int_byte_size_api() == 4) {
        int nin, nbn, nen, nie, nbe, nnc, nec;
        error = ex_get_loadbal_param(get_file_pointer(), &nin, &nbn, &nen, &nie, &nbe, &nnc, &nec,
                                     myProcessor);
        num_external_nodes = nen;
        num_elem_cmaps     = nec;
        num_node_cmaps     = nnc;
        num_internal_nodes = nin;
        num_border_nodes   = nbn;
        num_internal_elems = nie;
        num_border_elems   = nbe;
      }
      else {
        error = ex_get_loadbal_param(get_file_pointer(), &num_internal_nodes, &num_border_nodes,
                                     &num_external_nodes, &num_internal_elems, &num_border_elems,
                                     &num_node_cmaps, &num_elem_cmaps, myProcessor);
      }
      if (error < 0) {
        Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
      }

      // A nemesis file typically separates nodes into multiple
      // communication sets by processor.  (each set specifies
      // nodes/elements that communicate with only a single processor).
      // For Sierra, we want a single node commun. map and a single
      // element commun. map specifying all communications so we combine
      // all sets into a single set.

      if (int_byte_size_api() == 4) {
        int gn, ge, geb, gns, gss;
        error           = ex_get_init_global(get_file_pointer(), &gn, &ge, &geb, &gns, &gss);
        global_nodes    = gn;
        global_elements = ge;
        global_eblocks  = geb;
        global_nsets    = gns;
        global_ssets    = gss;
      }
      else {
        error = ex_get_init_global(get_file_pointer(), &global_nodes, &global_elements,
                                   &global_eblocks, &global_nsets, &global_ssets);
      }
      if (error < 0) {
        Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
      }
    }

    commsetNodeCount = num_node_cmaps;
    commsetElemCount = num_elem_cmaps;

    Ioss::Region *region = get_region();
    region->property_add(Ioss::Property("internal_node_count", num_internal_nodes));
    region->property_add(Ioss::Property("border_node_count", num_border_nodes));
    region->property_add(Ioss::Property("internal_element_count", num_internal_elems));
    region->property_add(Ioss::Property("border_element_count", num_border_elems));
    region->property_add(Ioss::Property("global_node_count", global_nodes));
    region->property_add(Ioss::Property("global_element_count", global_elements));
    region->property_add(Ioss::Property("global_element_block_count", global_eblocks));
    region->property_add(Ioss::Property("global_node_set_count", global_nsets));
    region->property_add(Ioss::Property("global_side_set_count", global_ssets));

    // Possibly, the following 4 fields should be nodesets and element
    // sets instead of fields on the region...
    region->field_add(Ioss::Field("internal_nodes", region->field_int_type(), IOSS_SCALAR(),
                                  Ioss::Field::COMMUNICATION, num_internal_nodes));
    region->field_add(Ioss::Field("border_nodes", region->field_int_type(), IOSS_SCALAR(),
                                  Ioss::Field::COMMUNICATION, num_border_nodes));
    region->field_add(Ioss::Field("internal_elements", region->field_int_type(), IOSS_SCALAR(),
                                  Ioss::Field::COMMUNICATION, num_internal_elems));
    region->field_add(Ioss::Field("border_elements", region->field_int_type(), IOSS_SCALAR(),
                                  Ioss::Field::COMMUNICATION, num_border_elems));

    assert(nodeCount == num_internal_nodes + num_border_nodes);
    assert(elementCount == num_internal_elems + num_border_elems);
  }

  const Ioss::Map &DatabaseIO::get_map(ex_entity_type type) const
  {
    switch (type) {
    case EX_NODE_BLOCK:
    case EX_NODE_SET: return get_map(nodeMap, nodeCount, EX_NODE_MAP, EX_INQ_NODE_MAP);

    case EX_ELEM_BLOCK:
    case EX_ELEM_SET: return get_map(elemMap, elementCount, EX_ELEM_MAP, EX_INQ_ELEM_MAP);

    case EX_FACE_BLOCK:
    case EX_FACE_SET: return get_map(faceMap, faceCount, EX_FACE_MAP, EX_INQ_FACE_MAP);

    case EX_EDGE_BLOCK:
    case EX_EDGE_SET: return get_map(edgeMap, edgeCount, EX_EDGE_MAP, EX_INQ_EDGE_MAP);

    default:
      std::ostringstream errmsg;
      fmt::print(errmsg, "INTERNAL ERROR: Invalid map type. "
                         "Something is wrong in the Ioex::DatabaseIO::get_map() function. "
                         "Please report.\n");
      IOSS_ERROR(errmsg);
    }
  }

  const Ioss::Map &DatabaseIO::get_map(Ioss::Map &entity_map, int64_t entity_count,
                                       ex_entity_type entity_type, ex_inquiry inquiry_type) const
  {
    // Allocate space for node number map and read it in...
    // Can be called multiple times, allocate 1 time only
    if (entity_map.map().empty()) {
      entity_map.set_size(entity_count);

      if (is_input() || open_create_behavior() == Ioss::DB_APPEND) {

        Ioss::SerializeIO serializeIO__(this);
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
            int error = 0;
            if ((ex_int64_status(get_file_pointer()) & EX_BULK_INT64_API) != 0) {
              Ioss::Int64Vector tmp_map(entity_map.size());
              error = ex_get_num_map(get_file_pointer(), entity_type, 1, tmp_map.data());
              if (error >= 0) {
                entity_map.set_map(tmp_map.data(), tmp_map.size(), 0, true);
                map_read = true;
              }
            }
            else {
              // Ioss stores as 64-bit, read as 32-bit and copy over...
              Ioss::IntVector tmp_map(entity_map.size());
              error = ex_get_num_map(get_file_pointer(), entity_type, 1, tmp_map.data());
              if (error >= 0) {
                entity_map.set_map(tmp_map.data(), tmp_map.size(), 0, true);
                map_read = true;
              }
            }
          }
          Ioss::Utils::delete_name_array(names, map_count);
        }

        if (!map_read) {
          int error = 0;
          if ((ex_int64_status(get_file_pointer()) & EX_BULK_INT64_API) != 0) {
            Ioss::Int64Vector tmp_map(entity_map.size());
            error = ex_get_id_map(get_file_pointer(), entity_type, tmp_map.data());
            if (error >= 0) {
              entity_map.set_map(tmp_map.data(), tmp_map.size(), 0, true);
            }
          }
          else {
            // Ioss stores as 64-bit, read as 32-bit and copy over...
            Ioss::IntVector tmp_map(entity_map.size());
            error = ex_get_id_map(get_file_pointer(), entity_type, tmp_map.data());
            if (error >= 0) {
              entity_map.set_map(tmp_map.data(), tmp_map.size(), 0, true);
            }
          }
          if (error < 0) {
            Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
          }
        }
      }
      else {
        // Output database; entity_map.map not set yet... Build a default map.
        entity_map.set_default(entity_count);
      }
    }
    return entity_map;
  }

  void DatabaseIO::get_elemblocks() { get_blocks(EX_ELEM_BLOCK, 0, "block"); }

  void DatabaseIO::get_faceblocks() { get_blocks(EX_FACE_BLOCK, 1, "faceblock"); }

  void DatabaseIO::get_edgeblocks() { get_blocks(EX_EDGE_BLOCK, 2, "edgeblock"); }

  void DatabaseIO::get_blocks(ex_entity_type entity_type, int rank_offset,
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

    // Get exodus X block metadata
    if (m_groupCount[entity_type] == 0) {
      return;
    }

    bool retain_empty_blocks = false;
    Ioss::Utils::check_set_bool_property(properties, "RETAIN_EMPTY_BLOCKS", retain_empty_blocks);

    Ioss::Int64Vector X_block_ids(m_groupCount[entity_type]);

    int error;
    {
      Ioss::SerializeIO serializeIO__(this);

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
    }

    size_t            all_X_type_length = m_groupCount[entity_type] * (MAX_STR_LENGTH + 1);
    std::vector<char> all_X_type(all_X_type_length);

    Ioss::Int64Vector counts(m_groupCount[entity_type] * 4);
    Ioss::Int64Vector local_X_count(m_groupCount[entity_type]);
    Ioss::Int64Vector global_X_count(m_groupCount[entity_type]);
    int               iblk;

    {
      Ioss::SerializeIO serializeIO__(this);

      for (iblk = 0; iblk < m_groupCount[entity_type]; iblk++) {
        int     index = 4 * iblk;
        int64_t id    = X_block_ids[iblk];

        char *const X_type = all_X_type.data() + iblk * (MAX_STR_LENGTH + 1);

        ex_block block{};
        block.id   = id;
        block.type = entity_type;
        error      = ex_get_block_param(get_file_pointer(), &block);
        if (error < 0) {
          Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
        }

        local_X_count[iblk] = block.num_entry;

        counts[index + 0] = block.num_nodes_per_entry;
        counts[index + 1] = block.num_edges_per_entry;
        counts[index + 2] = block.num_faces_per_entry;
        counts[index + 3] = block.num_attribute;

        if (block.num_entry == 0) {
          std::memset(X_type, 0, MAX_STR_LENGTH + 1);
        }
        else {
          Ioss::Utils::copy_string(X_type, block.topology, MAX_STR_LENGTH + 1);
        }
      }
    }

    // This is a collective call...
    util().attribute_reduction(all_X_type_length, all_X_type.data());

    // This is a collective call...
    util().global_array_minmax(counts, Ioss::ParallelUtils::DO_MAX);

    // Determine global X count for each X block....
    // Can also get this from an nemesis call, but the data may not always be there in all cases.
    util().global_count(local_X_count, global_X_count);

    // The 'offset' is used to map an X location within an X
    // block to the X 'file descriptor'.  For example, the file
    // descriptor of the 37th X in the 4th block is calculated by:
    // file_descriptor = offset of block 4 + 37 This can also be used to
    // determine which X block an X with a file_descriptor
    // maps into. An particular X block contains all Xs in
    // the range:
    //     offset < file_descriptor <= offset+number_Xs_per_block
    int64_t offset      = 0;
    int     used_blocks = 0;

    int nvar = std::numeric_limits<int>::max(); // Number of 'block' vars on database. Used to skip
                                                // querying if none.
    int nmap = std::numeric_limits<int>::max(); // Number of 'block' vars on database. Used to skip
                                                // querying if none.
    for (iblk = 0; iblk < m_groupCount[entity_type]; iblk++) {
      int     index       = 4 * iblk;
      int64_t nodes_per_X = counts[index + 0];
      int64_t edges_per_X = counts[index + 1];
      int64_t faces_per_X = counts[index + 2];
      int64_t attributes  = counts[index + 3];

      int64_t     id     = X_block_ids[iblk];
      std::string alias  = Ioss::Utils::encode_entity_name(basename, id);
      char *const X_type = all_X_type.data() + iblk * (MAX_STR_LENGTH + 1);

      bool        db_has_name = false;
      std::string block_name;
      if (ignore_database_names()) {
        block_name = alias;
      }
      else {
        Ioss::SerializeIO serializeIO__(this);
        block_name = Ioex::get_entity_name(get_file_pointer(), entity_type, id, basename,
                                           maximumNameLength, db_has_name);
      }
      if (get_use_generic_canonical_name()) {
        std::swap(block_name, alias);
      }

      std::string save_type = X_type;
      std::string type =
          Ioss::Utils::fixup_type(X_type, nodes_per_X, spatialDimension - rank_offset);
      if (local_X_count[iblk] == 0 && type == "") {
        // For an empty block, exodus does not store the X
        // type information and returns "nullptr" If there are no
        // Xs on any processors for this block, it will have
        // an empty type which is invalid and will throw an
        // exception in the XBlock constructor. Try to discern
        // the correct X type based on the block_name.
        std::vector<std::string> tokens = Ioss::tokenize(block_name, "_");
        if (tokens.size() >= 2) {
          // Check whether last token names an X topology type...
          const Ioss::ElementTopology *topology =
              Ioss::ElementTopology::factory(tokens.back(), true);
          if (topology != nullptr) {
            type = topology->name();
          }
        }
      }

      if (type == "null" || type == "") {
        // If we have no idea what the topology type for an empty
        // X block is, call it "unknown"
        type = "unknown";

        // If there are no Xs on any processor for this block and
        // we have no idea what the topology type is, skip it...
        if (!retain_empty_blocks && global_X_count[iblk] == 0) {
          continue;
        }
      }

      Ioss::EntityBlock *block = nullptr;
      if (entity_type == EX_ELEM_BLOCK) {
        auto eblock = new Ioss::ElementBlock(this, block_name, type, local_X_count[iblk]);
        block       = eblock;
        get_region()->add(eblock);
      }
      else if (entity_type == EX_FACE_BLOCK) {
        auto fblock = new Ioss::FaceBlock(this, block_name, type, local_X_count[iblk]);
        block       = fblock;
        get_region()->add(fblock);
      }
      else if (entity_type == EX_EDGE_BLOCK) {
        auto eblock = new Ioss::EdgeBlock(this, block_name, type, local_X_count[iblk]);
        block       = eblock;
        get_region()->add(eblock);
      }
      else {
        std::ostringstream errmsg;
        fmt::print(errmsg, "ERROR: Invalid type in get_blocks()");
        IOSS_ERROR(errmsg);
      }

      // See which connectivity options were defined for this block.
      // X -> Node is always defined.
      // X -> Face?
      if (faces_per_X > 0 && rank_offset < 1) {
        std::string storage = "Real[" + std::to_string(faces_per_X) + "]";
        block->field_add(
            Ioss::Field("connectivity_face", block->field_int_type(), storage, Ioss::Field::MESH));
      }
      // X -> Edge?
      if (edges_per_X > 0 && rank_offset < 2) {
        std::string storage = "Real[" + std::to_string(edges_per_X) + "]";
        block->field_add(
            Ioss::Field("connectivity_edge", block->field_int_type(), storage, Ioss::Field::MESH));
      }

      block->property_add(Ioss::Property("id", id)); // Do before adding for better error messages.
      block->property_add(Ioss::Property("guid", util().generate_guid(id)));
      if (db_has_name) {
        std::string *db_name = &block_name;
        if (get_use_generic_canonical_name()) {
          db_name = &alias;
        }
        if (alias != block_name) {
          block->property_add(Ioss::Property("db_name", *db_name));
        }
      }

      // Maintain block order on output database...
      block->property_add(Ioss::Property("original_block_order", used_blocks++));

      if (save_type != "null" && save_type != "") {
        block->property_update("original_topology_type", save_type);
      }

      block->property_add(Ioss::Property("global_entity_count", global_X_count[iblk]));

      offset += local_X_count[iblk];

      get_region()->add_alias(block_name, alias);

      // Check for additional variables.
      add_attribute_fields(entity_type, block, attributes, type);
      if (nvar > 0) {
        nvar = add_results_fields(entity_type, block, iblk);
      }
      add_reduction_results_fields(entity_type, block);
      add_mesh_reduction_fields(entity_type, id, block);

      if (entity_type == EX_ELEM_BLOCK) {
        Ioss::SerializeIO serializeIO__(this);
        if (nmap > 0) {
          Ioss::ElementBlock *elb = dynamic_cast<Ioss::ElementBlock *>(block);
          Ioss::Utils::check_dynamic_cast(elb);
          nmap =
              Ioex::add_map_fields(get_file_pointer(), elb, local_X_count[iblk], maximumNameLength);
        }
      }
    }
    m_groupCount[entity_type] = used_blocks;

    if (entity_type == EX_ELEM_BLOCK) {
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

  void DatabaseIO::compute_node_status() const
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
      int     element_nodes    = block->get_property("topology_node_count").get_int();
      int64_t my_element_count = block->entity_count();
      if (my_element_count > 0) {
        if ((ex_int64_status(get_file_pointer()) & EX_BULK_INT64_API) != 0) {
          std::vector<int64_t> conn(my_element_count * element_nodes);
          ex_get_conn(get_file_pointer(), EX_ELEM_BLOCK, id, conn.data(), nullptr, nullptr);
          for (int64_t j = 0; j < my_element_count * element_nodes; j++) {
            nodeConnectivityStatus[conn[j] - 1] |= status;
          }
        }
        else {
          std::vector<int> conn(my_element_count * element_nodes);
          ex_get_conn(get_file_pointer(), EX_ELEM_BLOCK, id, conn.data(), nullptr, nullptr);
          for (int64_t j = 0; j < my_element_count * element_nodes; j++) {
            nodeConnectivityStatus[conn[j] - 1] |= status;
          }
        }
      }
    }
    nodeConnectivityStatusCalculated = true;
  }

  void DatabaseIO::get_sidesets()
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

      // Get exodus sideset metadata

      // Get the names (may not exist) of all sidesets and see if they are actually
      // side "blocks" (perhaps written by IO system for a restart).  In that case,
      // they were split by a previous run and we need to reconstruct the side "set"
      // that may contain one or more of them.
      Ioex::SideSetMap fs_map;
      Ioex::SideSetSet fs_set;

      Ioss::Int64Vector side_set_ids(m_groupCount[EX_SIDE_SET]);
      {
        Ioss::SerializeIO serializeIO__(this);
        int               error;
        if ((ex_int64_status(get_file_pointer()) & EX_IDS_INT64_API) != 0) {
          error = ex_get_ids(get_file_pointer(), EX_SIDE_SET, side_set_ids.data());
        }
        else {
          Ioss::IntVector tmp_set_ids(side_set_ids.size());
          error = ex_get_ids(get_file_pointer(), EX_SIDE_SET, tmp_set_ids.data());
          if (error >= 0) {
            std::copy(tmp_set_ids.begin(), tmp_set_ids.end(), side_set_ids.begin());
          }
        }
        if (error < 0) {
          Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
        }

        for (const auto &id : side_set_ids) {
          std::vector<char> ss_name(maximumNameLength + 1);
          error = ex_get_name(get_file_pointer(), EX_SIDE_SET, id, ss_name.data());
          if (error < 0) {
            Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
          }
          if (ss_name[0] != '\0') {
            Ioss::Utils::fixup_name(ss_name.data());
            Ioex::decode_surface_name(fs_map, fs_set, ss_name.data());
          }
        }
      }

      // Create sidesets for each entry in the fs_set... These are the
      // sidesets which were probably written by a previous run of the
      // IO system and are already split into homogeneous pieces...
      {
        for (const auto &fs_name : fs_set) {
          auto side_set = new Ioss::SideSet(this, fs_name);
          get_region()->add(side_set);
          int64_t id = Ioex::extract_id(fs_name);
          if (id > 0) {
            side_set->property_add(Ioss::Property("id", id));
            side_set->property_add(Ioss::Property("guid", util().generate_guid(id)));
          }
        }
      }

      for (int iss = 0; iss < m_groupCount[EX_SIDE_SET]; iss++) {
        int64_t           id = side_set_ids[iss];
        std::string       sid;
        Ioex::TopologyMap topo_map;
        Ioex::TopologyMap side_map; // Used to determine side consistency

        Ioss::SurfaceSplitType split_type = splitType;
        std::string            side_set_name;
        Ioss::SideSet *        side_set = nullptr;

        {
          Ioss::SerializeIO serializeIO__(this);

          bool        db_has_name = false;
          std::string alias       = Ioss::Utils::encode_entity_name("surface", id);
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

          bool in_fs_map = false;
          auto FSM       = fs_map.find(side_set_name);
          if (FSM != fs_map.end()) {
            in_fs_map            = true;
            std::string efs_name = (*FSM).second;
            side_set             = get_region()->get_sideset(efs_name);
            Ioss::Utils::check_non_null(side_set, "sideset", efs_name, __func__);
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
              if (alias != side_set_name) {
                side_set->property_add(Ioss::Property("db_name", *db_name));
              }
            }

            get_region()->add(side_set);

            get_region()->add_alias(side_set_name, alias);
            get_region()->add_alias(side_set_name, Ioss::Utils::encode_entity_name("sideset", id));
          }

          //      split_type = SPLIT_BY_ELEMENT_BLOCK;
          //      split_type = SPLIT_BY_TOPOLOGIES;
          //      split_type = SPLIT_BY_DONT_SPLIT;

          // Determine how many side blocks compose this side set.
          ex_set set_param[1];
          set_param[0].id                       = id;
          set_param[0].type                     = EX_SIDE_SET;
          set_param[0].entry_list               = nullptr;
          set_param[0].extra_list               = nullptr;
          set_param[0].distribution_factor_list = nullptr;

          int error = ex_get_sets(get_file_pointer(), 1, set_param);
          if (error < 0) {
            Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
          }

          int64_t number_sides = set_param[0].num_entry;

          Ioss::Int64Vector element(number_sides);
          Ioss::Int64Vector sides(number_sides);

          // Easier below here if the element and sides are a known 64-bit size...
          // Kluge here to do that...
          if (int_byte_size_api() == 4) {
            Ioss::IntVector e32(number_sides);
            Ioss::IntVector s32(number_sides);
            int ierr = ex_get_set(get_file_pointer(), EX_SIDE_SET, id, e32.data(), s32.data());
            if (ierr < 0) {
              Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
            }
            std::copy(e32.begin(), e32.end(), element.begin());
            std::copy(s32.begin(), s32.end(), sides.begin());
          }
          else {
            int ierr =
                ex_get_set(get_file_pointer(), EX_SIDE_SET, id, element.data(), sides.data());
            if (ierr < 0) {
              Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
            }
          }

          if (!blockOmissions.empty() || !blockInclusions.empty()) {
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
          else if (in_fs_map) {
            std::vector<std::string> tokens = Ioss::tokenize(side_set_name, "_");
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
            sid = tokens.back();
          }
          else if (split_type == Ioss::SPLIT_BY_TOPOLOGIES) {
            // There are multiple side types in the model.
            // Iterate through the elements in the sideset, determine
            // their parent element block using the blocks element
            // topology and the side number, determine the side
            // type.

            for (const auto &elem : sideTopology) {
              topo_map[std::make_pair(elem.first->name(), elem.second)] = 0;
              side_map[std::make_pair(elem.first->name(), elem.second)] = 0;
            }

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
                std::string                  name         = block->name();
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
            Ioex::separate_surface_element_sides(element, sides, get_region(), topo_map, side_map,
                                                 split_type, side_set_name);
          }
        }

        // End of first step in splitting.  Check among all processors
        // to see which potential splits have sides in them...
        Ioss::Int64Vector global_side_counts(topo_map.size());
        {
          int64_t i = 0;
          for (const auto &topo : topo_map) {
            global_side_counts[i++] = topo.second;
          }

          // If splitting by element block, also sync the side_map
          // information which specifies whether the sideset has
          // consistent sides for all elements. Only really used for
          // shells, but easier to just set the value on all surfaces
          // in the element block split case.
          if (side_map.size() == topo_map.size()) {
            global_side_counts.resize(topo_map.size() + side_map.size());
            for (const auto &side : side_map) {
              global_side_counts[i++] = side.second;
            }
          }

          // See if any processor has non-zero count for the topo_map counts
          // For the side_map, need the max value.
          util().global_array_minmax(global_side_counts, Ioss::ParallelUtils::DO_MAX);
        }

        // Create Side Blocks

        int64_t i = 0;
        for (const auto &topo : topo_map) {
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
            if (side_set_name == "universal_sideset") {
              side_block_name = side_set_name;
            }
            else {
              if (sid == "") {
                side_block_name = Ioss::Utils::encode_entity_name(side_block_name, id);
              }
              else {
                side_block_name += "_";
                side_block_name += sid;
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
                           "INTERNAL ERROR: Could not find element block '{}' Something is wrong "
                           "in the Ioex::DatabaseIO class. Please report.\n",
                           topo_or_block_name);
                IOSS_ERROR(errmsg);
              }
              elem_topo = block->topology();
            }
            else if (split_type == Ioss::SPLIT_BY_DONT_SPLIT) {
              // Most likely this is "unknown", but can be a true
              // topology if there is only a single element block in
              // the model.
              elem_topo = Ioss::ElementTopology::factory(topo_or_block_name);
            }
            else {
              std::ostringstream errmsg;
              fmt::print(errmsg,
                         "INTERNAL ERROR: Invalid setting for `split_type` {}. Something is wrong "
                         "in the Ioex::DatabaseIO class. Please report.\n",
                         split_type);
              IOSS_ERROR(errmsg);
            }
            assert(elem_topo != nullptr);

            auto side_block = new Ioss::SideBlock(this, side_block_name, side_topo->name(),
                                                  elem_topo->name(), my_side_count);
            side_set->add(side_block);

            // Note that all sideblocks within a specific
            // sideset might have the same id.
            assert(side_block != nullptr);
            side_block->property_add(Ioss::Property("id", id));
            side_block->property_add(Ioss::Property("guid", util().generate_guid(id)));

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
              Ioss::SerializeIO serializeIO__(this);
              int ierr = ex_get_attr_param(get_file_pointer(), EX_SIDE_SET, 1, &num_attr);
              if (ierr < 0) {
                Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
              }
            }
            // Add additional fields
            add_attribute_fields(EX_SIDE_SET, side_block, num_attr, "");
            add_results_fields(EX_SIDE_SET, side_block, iss);
          }
        }
      }
    }
  }
} // namespace Ioex

template <typename T>
void DatabaseIO::get_sets(ex_entity_type type, int64_t count, const std::string &base,
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

  // Get exodus Xset metadata
  if (count > 0) {
    Ioss::Int64Vector Xset_ids(count);
    Ioss::IntVector   attributes(count);
    std::vector<T *>  Xsets(count);
    {
      Ioss::SerializeIO serializeIO__(this);
      if (ex_int64_status(get_file_pointer()) & EX_IDS_INT64_API) {
        int error = ex_get_ids(get_file_pointer(), type, Xset_ids.data());
        if (error < 0) {
          Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
        }
      }
      else {
        Ioss::IntVector tmp_set_ids(count);
        int             error = ex_get_ids(get_file_pointer(), type, tmp_set_ids.data());
        if (error < 0) {
          Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
        }
        std::copy(tmp_set_ids.begin(), tmp_set_ids.end(), Xset_ids.begin());
      }

      std::vector<ex_set> set_params(count);
      for (int ins = 0; ins < count; ins++) {
        set_params[ins].type                     = type;
        set_params[ins].id                       = Xset_ids[ins];
        set_params[ins].entry_list               = nullptr;
        set_params[ins].extra_list               = nullptr;
        set_params[ins].distribution_factor_list = nullptr;
      }

      int error = ex_get_sets(get_file_pointer(), count, set_params.data());
      if (error < 0) {
        Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
      }

      for (int ins = 0; ins < count; ins++) {
        int64_t id       = set_params[ins].id;
        int     num_attr = 0;
        int     ierr     = ex_get_attr_param(get_file_pointer(), type, id, &num_attr);
        if (ierr < 0) {
          Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
        }
        attributes[ins] = num_attr;

        bool        db_has_name = false;
        std::string alias       = Ioss::Utils::encode_entity_name(base + "list", id);
        std::string Xset_name;
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

        bool              filtered          = false;
        int64_t           original_set_size = set_params[ins].num_entry;
        Ioss::Int64Vector active_node_index;
        if ((!blockOmissions.empty() || !blockInclusions.empty()) && type == EX_NODE_SET) {
          active_node_index.resize(set_params[ins].num_entry);
          set_params[ins].entry_list = active_node_index.data();

          int old_status = ex_int64_status(get_file_pointer());
          ex_set_int64_status(get_file_pointer(), EX_BULK_INT64_API);
          error = ex_get_sets(get_file_pointer(), 1, &set_params[ins]);
          if (error < 0) {
            Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
          }
          ex_set_int64_status(get_file_pointer(), old_status);

          compute_node_status();
          filtered = Ioex::filter_node_list(active_node_index, nodeConnectivityStatus);
          set_params[ins].num_entry = active_node_index.size();
        }
        auto Xset  = new T(this, Xset_name, set_params[ins].num_entry);
        Xsets[ins] = Xset;
        Xset->property_add(Ioss::Property("id", id));
        Xset->property_add(Ioss::Property("guid", util().generate_guid(id)));
        if (db_has_name) {
          std::string *db_name = &Xset_name;
          if (get_use_generic_canonical_name()) {
            db_name = &alias;
          }
          if (alias != Xset_name) {
            Xset->property_add(Ioss::Property("db_name", *db_name));
          }
        }
        if (filtered && type == EX_NODE_SET) {
          Xset->property_add(Ioss::Property("filtered_db_set_size", original_set_size));
          activeNodeSetNodesIndex[Xset_name].swap(active_node_index);
        }
        get_region()->add(Xset);
        get_region()->add_alias(Xset_name, alias);
        get_region()->add_alias(Xset_name, Ioss::Utils::encode_entity_name(base + "set", id));
      }
    }

    // The attribute count will either be 0 if there are no
    // entities in the grouping entity on this processor, or it will be
    // the number of attributes (> 0). Therefore, if we take the 'max'
    // over all processors, each processor will then have the correct
    // attribute count...
    // This is a collective call...
    util().global_array_minmax(attributes, Ioss::ParallelUtils::DO_MAX);

    for (int ins = 0; ins < count; ins++) {
      add_attribute_fields(type, Xsets[ins], attributes[ins], "");
      add_results_fields(type, Xsets[ins], ins);
    }
  }
}

void DatabaseIO::get_nodesets()
{
  get_sets(EX_NODE_SET, m_groupCount[EX_NODE_SET], "node", (Ioss::NodeSet *)nullptr);
}

void DatabaseIO::get_edgesets()
{
  get_sets(EX_EDGE_SET, m_groupCount[EX_EDGE_SET], "edge", (Ioss::EdgeSet *)nullptr);
}

void DatabaseIO::get_facesets()
{
  get_sets(EX_FACE_SET, m_groupCount[EX_FACE_SET], "face", (Ioss::FaceSet *)nullptr);
}

void DatabaseIO::get_elemsets()
{
  get_sets(EX_ELEM_SET, m_groupCount[EX_ELEM_SET], "element", (Ioss::ElementSet *)nullptr);
}

void DatabaseIO::get_commsets()
{
  // Attributes of a commset are:
  // -- id (property)
  // -- name (property)
  // -- number of node--CPU pairs (field)

  // In a parallel execution, it is possible that a commset will have
  // no nodes on a particular processor...

  // If this is a serial execution, there will be no communication
  // nodesets, just return an empty container.

  if (isParallel || isSerialParallel) {
    Ioss::SerializeIO serializeIO__(this);
    // This is a parallel run. There should be communications data
    // Get nemesis commset metadata
    int64_t my_node_count = 0;
    int64_t elem_count    = 0;

    // NOTE: It is possible for a parallel run to have no
    // communications maps if the decomposition occurs along contact
    // surfaces.  In this case, we create empty node and element
    // communication maps.
    if (commsetNodeCount > 0 || commsetElemCount > 0) {
      if (commsetNodeCount > 0) {
        nodeCmapIds.resize(commsetNodeCount);
        nodeCmapNodeCnts.resize(commsetNodeCount);
      }
      if (commsetElemCount > 0) {
        elemCmapIds.resize(commsetElemCount);
        elemCmapElemCnts.resize(commsetElemCount);
      }

      int error;
      if (int_byte_size_api() == 4) {
        Ioss::IntVector nci(nodeCmapIds.size());
        Ioss::IntVector ncnc(nodeCmapNodeCnts.size());
        Ioss::IntVector eci(elemCmapIds.size());
        Ioss::IntVector ecec(elemCmapElemCnts.size());
        error = ex_get_cmap_params(get_file_pointer(), nci.data(), ncnc.data(), eci.data(),
                                   ecec.data(), myProcessor);
        if (error >= 0) {
          std::copy(nci.begin(), nci.end(), nodeCmapIds.begin());
          std::copy(ncnc.begin(), ncnc.end(), nodeCmapNodeCnts.begin());
          std::copy(eci.begin(), eci.end(), elemCmapIds.begin());
          std::copy(ecec.begin(), ecec.end(), elemCmapElemCnts.begin());
        }
      }
      else {
        error = ex_get_cmap_params(get_file_pointer(), nodeCmapIds.data(), nodeCmapNodeCnts.data(),
                                   elemCmapIds.data(), elemCmapElemCnts.data(), myProcessor);
      }
      if (error < 0) {
        Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
      }

      // Count nodes, elements, and convert counts to offsets.
      my_node_count +=
          std::accumulate(nodeCmapNodeCnts.begin(), nodeCmapNodeCnts.end(), int64_t(0));

      elem_count += std::accumulate(elemCmapElemCnts.begin(), elemCmapElemCnts.end(), int64_t(0));
    }
    // Create a single node commset and a single element commset
    Ioss::CommSet *commset = new Ioss::CommSet(this, "commset_node", "node", my_node_count);
    commset->property_add(Ioss::Property("id", 1));
    commset->property_add(Ioss::Property("guid", util().generate_guid(1)));
    get_region()->add(commset);

    commset = new Ioss::CommSet(this, "commset_side", "side", elem_count);
    commset->property_add(Ioss::Property("id", 1));
    commset->property_add(Ioss::Property("guid", util().generate_guid(1)));
    get_region()->add(commset);
  }
}

int64_t DatabaseIO::get_field_internal(const Ioss::Region *reg, const Ioss::Field &field,
                                       void *data, size_t data_size) const
{
  return Ioex::BaseDatabaseIO::get_field_internal(reg, field, data, data_size);
}

int64_t DatabaseIO::get_field_internal(const Ioss::NodeBlock *nb, const Ioss::Field &field,
                                       void *data, size_t data_size) const
{
  {
    Ioss::SerializeIO serializeIO__(this);

    size_t num_to_get = field.verify(data_size);
    if (num_to_get > 0) {

#ifndef NDEBUG
      int64_t my_node_count = field.raw_count();
      assert(my_node_count == nodeCount);
#endif

      Ioss::Field::RoleType role = field.get_role();
      if (role == Ioss::Field::MESH) {
        if (field.get_name() == "mesh_model_coordinates_x") {
          double *rdata = static_cast<double *>(data);
          int     ierr  = ex_get_coord(get_file_pointer(), rdata, nullptr, nullptr);
          if (ierr < 0) {
            Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
          }
        }

        else if (field.get_name() == "mesh_model_coordinates_y") {
          double *rdata = static_cast<double *>(data);
          int     ierr  = ex_get_coord(get_file_pointer(), nullptr, rdata, nullptr);
          if (ierr < 0) {
            Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
          }
        }

        else if (field.get_name() == "mesh_model_coordinates_z") {
          double *rdata = static_cast<double *>(data);
          int     ierr  = ex_get_coord(get_file_pointer(), nullptr, nullptr, rdata);
          if (ierr < 0) {
            Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
          }
        }

        else if (field.get_name() == "mesh_model_coordinates") {
          // Data required by upper classes store x0, y0, z0, ... xn,
          // yn, zn. Data stored in exodus file is x0, ..., xn, y0,
          // ..., yn, z0, ..., zn so we have to allocate some scratch
          // memory to read in the data and then map into supplied
          // 'data'
          std::vector<double> x(num_to_get);
          std::vector<double> y;
          if (spatialDimension > 1) {
            y.resize(num_to_get);
          }
          std::vector<double> z;
          if (spatialDimension == 3) {
            z.resize(num_to_get);
          }

          // Cast 'data' to correct size -- double
          double *rdata = static_cast<double *>(data);

          int ierr = ex_get_coord(get_file_pointer(), x.data(), y.data(), z.data());
          if (ierr < 0) {
            Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
          }

          size_t index = 0;
          for (size_t i = 0; i < num_to_get; i++) {
            rdata[index++] = x[i];
            if (spatialDimension > 1) {
              rdata[index++] = y[i];
            }
            if (spatialDimension == 3) {
              rdata[index++] = z[i];
            }
          }
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
          // If not parallel, then this is just 1..node_count
          // If parallel, then it is the data in the ex_get_id_map created by nem_spread.
          if (isParallel) {
            int error = ex_get_id_map(get_file_pointer(), EX_NODE_MAP, data);
            if (error < 0) {
              Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
            }
          }
          else {
            if ((ex_int64_status(get_file_pointer()) & EX_BULK_INT64_API) != 0) {
              int64_t *idata = static_cast<int64_t *>(data);
              for (int64_t i = 0; i < nodeCount; i++) {
                idata[i] = i + 1;
              }
            }
            else {
              int *idata = static_cast<int *>(data);
              for (int64_t i = 0; i < nodeCount; i++) {
                idata[i] = i + 1;
              }
            }
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
          // owning_processor field is always 32-bit.
          if (isParallel) {
            Ioss::CommSet *css   = get_region()->get_commset("commset_node");
            int *          idata = static_cast<int *>(data);
            for (int64_t i = 0; i < nodeCount; i++) {
              idata[i] = myProcessor;
            }

            if ((ex_int64_status(get_file_pointer()) & EX_BULK_INT64_API) != 0) {
              Ioss::Field          ep_field = css->get_field("entity_processor_raw");
              std::vector<int64_t> ent_proc(ep_field.raw_count() *
                                            ep_field.raw_storage()->component_count());
              size_t               ep_data_size = ent_proc.size() * sizeof(int64_t);
              get_field_internal(css, ep_field, ent_proc.data(), ep_data_size);
              for (size_t i = 0; i < ent_proc.size(); i += 2) {
                int64_t node = ent_proc[i + 0];
                int64_t proc = ent_proc[i + 1];
                if (proc < myProcessor) {
                  idata[node - 1] = proc;
                }
              }
            }
            else {
              Ioss::Field      ep_field = css->get_field("entity_processor_raw");
              std::vector<int> ent_proc(ep_field.raw_count() *
                                        ep_field.raw_storage()->component_count());
              size_t           ep_data_size = ent_proc.size() * sizeof(int);
              get_field_internal(css, ep_field, ent_proc.data(), ep_data_size);
              for (size_t i = 0; i < ent_proc.size(); i += 2) {
                int node = ent_proc[i + 0];
                int proc = ent_proc[i + 1];
                if (proc < myProcessor) {
                  idata[node - 1] = proc;
                }
              }
            }
          }
          else {
            // Serial case...
            int *idata = static_cast<int *>(data);
            for (int64_t i = 0; i < nodeCount; i++) {
              idata[i] = 0;
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
        num_to_get =
            read_transient_field(EX_NODE_BLOCK, m_variables[EX_NODE_BLOCK], field, nb, data);
      }
      else if (role == Ioss::Field::REDUCTION) {
        get_reduction_field(EX_NODE_BLOCK, field, nb, data);
      }
      else if (role == Ioss::Field::ATTRIBUTE) {
        num_to_get = read_attribute_field(EX_NODE_BLOCK, field, nb, data);
      }
    }
    return num_to_get;
  }
}

int64_t DatabaseIO::get_field_internal(const Ioss::Blob *blob, const Ioss::Field &field, void *data,
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
        num_to_get = read_transient_field(EX_BLOB, m_variables[EX_BLOB], field, blob, data);
      }
      else if (role == Ioss::Field::REDUCTION) {
        get_reduction_field(EX_BLOB, field, blob, data);
      }
      else if (role == Ioss::Field::ATTRIBUTE) {
        num_to_get = read_attribute_field(EX_BLOB, field, blob, data);
      }
    }
    return num_to_get;
  }
}

int64_t DatabaseIO::get_field_internal(const Ioss::Assembly *assembly, const Ioss::Field &field,
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
        num_to_get =
            read_transient_field(EX_ASSEMBLY, m_variables[EX_ASSEMBLY], field, assembly, data);
      }
      else if (role == Ioss::Field::REDUCTION) {
        get_reduction_field(EX_ASSEMBLY, field, assembly, data);
      }
      else if (role == Ioss::Field::ATTRIBUTE) {
        num_to_get = read_attribute_field(EX_ASSEMBLY, field, assembly, data);
      }
    }
    return num_to_get;
  }
}

int64_t DatabaseIO::get_field_internal(const Ioss::ElementBlock *eb, const Ioss::Field &field,
                                       void *data, size_t data_size) const
{
  {
    Ioss::SerializeIO serializeIO__(this);

    size_t num_to_get = field.verify(data_size);
    if (num_to_get > 0) {

      int64_t               id               = Ioex::get_id(eb, EX_ELEM_BLOCK, &ids_);
      size_t                my_element_count = eb->entity_count();
      Ioss::Field::RoleType role             = field.get_role();

      if (role == Ioss::Field::MESH) {
        // Handle the MESH fields required for an Exodus file model.
        // (The 'genesis' portion)

        if (field.get_name() == "connectivity") {
          int element_nodes = eb->get_property("topology_node_count").get_int();
          assert(field.raw_storage()->component_count() == element_nodes);

          // The connectivity is stored in a 1D array.
          // The element_node index varies fastet
          if (my_element_count > 0) {
            get_connectivity_data(get_file_pointer(), data, EX_ELEM_BLOCK, id, 0);
            get_map(EX_NODE_BLOCK).map_data(data, field, num_to_get * element_nodes);
          }
        }
        else if (field.get_name() == "connectivity_face") {
          int face_count = field.raw_storage()->component_count();

          // The connectivity is stored in a 1D array.
          // The element_face index varies fastest
          if (my_element_count > 0) {
            get_connectivity_data(get_file_pointer(), data, EX_ELEM_BLOCK, id, 2);
            get_map(EX_FACE_BLOCK).map_data(data, field, num_to_get * face_count);
          }
        }
        else if (field.get_name() == "connectivity_edge") {
          int edge_count = field.raw_storage()->component_count();

          // The connectivity is stored in a 1D array.
          // The element_edge index varies fastest
          if (my_element_count > 0) {
            get_connectivity_data(get_file_pointer(), data, EX_ELEM_BLOCK, id, 1);
            get_map(EX_EDGE_BLOCK).map_data(data, field, num_to_get * edge_count);
          }
        }
        else if (field.get_name() == "connectivity_raw") {
          // "connectivity_raw" has nodes in local id space (1-based)
          assert(field.raw_storage()->component_count() ==
                 eb->get_property("topology_node_count").get_int());

          // The connectivity is stored in a 1D array.
          // The element_node index varies fastest
          if (my_element_count > 0) {
            get_connectivity_data(get_file_pointer(), data, EX_ELEM_BLOCK, id, 0);
          }
        }
        else if (field.get_name() == "ids") {
          // Map the local ids in this element block
          // (eb_offset+1...eb_offset+1+my_element_count) to global element ids.
          get_map(EX_ELEM_BLOCK).map_implicit_data(data, field, num_to_get, eb->get_offset());
        }
        else if (field.get_name() == "implicit_ids") {
          // If not parallel, then this is just one..element_count
          // If parallel, then it is the data in the ex_get_id_map created by nem_spread.
          size_t eb_offset_plus_one = eb->get_offset() + 1;
          if (isParallel) {
            int error = ex_get_partial_id_map(get_file_pointer(), EX_ELEM_MAP, eb_offset_plus_one,
                                              my_element_count, data);
            if (error < 0) {
              Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
            }
          }
          else {
            if ((ex_int64_status(get_file_pointer()) & EX_BULK_INT64_API) != 0) {
              int64_t *idata = static_cast<int64_t *>(data);
              for (size_t i = 0; i < my_element_count; i++) {
                idata[i] = eb_offset_plus_one + i;
              }
            }
            else {
              int *idata = static_cast<int *>(data);
              for (size_t i = 0; i < my_element_count; i++) {
                idata[i] = eb_offset_plus_one + i;
              }
            }
          }
        }
        else if (field.get_name() == "skin") {
          // This is (currently) for the skinned body. It maps the
          // side element on the skin to the original element/local
          // side number.  It is a two component field, the first
          // component is the global id of the underlying element in
          // the initial mesh and its local side number (1-based).

          if (field.is_type(Ioss::Field::INTEGER)) {
            Ioss::IntVector element(my_element_count);
            Ioss::IntVector side(my_element_count);
            int *           el_side = reinterpret_cast<int *>(data);

            // FIX: Hardwired map ids....
            size_t eb_offset = eb->get_offset();
            ex_get_partial_num_map(get_file_pointer(), EX_ELEM_MAP, 1, eb_offset + 1,
                                   my_element_count, element.data());
            ex_get_partial_num_map(get_file_pointer(), EX_ELEM_MAP, 2, eb_offset + 1,
                                   my_element_count, side.data());

            int index = 0;
            for (size_t i = 0; i < my_element_count; i++) {
              el_side[index++] = element[i];
              el_side[index++] = side[i];
            }
          }
          else {
            Ioss::Int64Vector element(my_element_count);
            Ioss::Int64Vector side(my_element_count);
            int64_t *         el_side = reinterpret_cast<int64_t *>(data);

            // FIX: Hardwired map ids....
            size_t eb_offset = eb->get_offset();
            ex_get_partial_num_map(get_file_pointer(), EX_ELEM_MAP, 1, eb_offset + 1,
                                   my_element_count, element.data());
            ex_get_partial_num_map(get_file_pointer(), EX_ELEM_MAP, 2, eb_offset + 1,
                                   my_element_count, side.data());

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
      }
      else if (role == Ioss::Field::ATTRIBUTE) {
        num_to_get = read_attribute_field(EX_ELEM_BLOCK, field, eb, data);
      }
      else if (role == Ioss::Field::TRANSIENT) {
        // Check if the specified field exists on this element block.
        // Note that 'higher-order' storage types (e.g. SYM_TENSOR)
        // exist on the database as scalars with the appropriate
        // extensions.

        // Read in each component of the variable and transfer into
        // 'data'.  Need temporary storage area of size 'number of
        // elements in this block.
        num_to_get =
            read_transient_field(EX_ELEM_BLOCK, m_variables[EX_ELEM_BLOCK], field, eb, data);
      }
      else if (role == Ioss::Field::REDUCTION) {
        get_reduction_field(EX_ELEM_BLOCK, field, eb, data);
      }
    }
    return num_to_get;
  }
}

int64_t DatabaseIO::get_field_internal(const Ioss::FaceBlock *eb, const Ioss::Field &field,
                                       void *data, size_t data_size) const
{
  {
    Ioss::SerializeIO serializeIO__(this);

    size_t num_to_get = field.verify(data_size);
    if (num_to_get > 0) {

      int64_t               id            = Ioex::get_id(eb, EX_FACE_BLOCK, &ids_);
      size_t                my_face_count = eb->entity_count();
      Ioss::Field::RoleType role          = field.get_role();

      if (role == Ioss::Field::MESH) {
        // Handle the MESH fields required for an Exodus file model.
        // (The 'genesis' portion)

        if (field.get_name() == "connectivity") {
          int face_nodes = eb->get_property("topology_node_count").get_int();
          assert(field.raw_storage()->component_count() == face_nodes);

          // The connectivity is stored in a 1D array.
          // The face_node index varies fastet
          if (my_face_count > 0) {
            get_connectivity_data(get_file_pointer(), data, EX_FACE_BLOCK, id, 0);
            get_map(EX_NODE_BLOCK).map_data(data, field, num_to_get * face_nodes);
          }
        }
        else if (field.get_name() == "connectivity_edge") {
          int edge_count = field.raw_storage()->component_count();

          // The connectivity is stored in a 1D array.
          // The face_edge index varies fastest
          if (my_face_count > 0) {
            get_connectivity_data(get_file_pointer(), data, EX_FACE_BLOCK, id, 1);
            get_map(EX_EDGE_BLOCK).map_data(data, field, num_to_get * edge_count);
          }
        }
        else if (field.get_name() == "connectivity_raw") {
          // "connectivity_raw" has nodes in local id space (1-based)
          assert(field.raw_storage()->component_count() ==
                 eb->get_property("topology_node_count").get_int());

          // The connectivity is stored in a 1D array.
          // The face_node index varies fastet
          if (my_face_count > 0) {
            get_connectivity_data(get_file_pointer(), data, EX_FACE_BLOCK, id, 0);
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
        num_to_get = read_attribute_field(EX_FACE_BLOCK, field, eb, data);
      }
      else if (role == Ioss::Field::TRANSIENT) {
        // Check if the specified field exists on this element block.
        // Note that 'higher-order' storage types (e.g. SYM_TENSOR)
        // exist on the database as scalars with the appropriate
        // extensions.

        // Read in each component of the variable and transfer into
        // 'data'.  Need temporary storage area of size 'number of
        // elements in this block.
        num_to_get =
            read_transient_field(EX_FACE_BLOCK, m_variables[EX_FACE_BLOCK], field, eb, data);
      }
      else if (role == Ioss::Field::REDUCTION) {
        get_reduction_field(EX_FACE_BLOCK, field, eb, data);
      }
    }
    return num_to_get;
  }
}

int64_t DatabaseIO::get_field_internal(const Ioss::EdgeBlock *eb, const Ioss::Field &field,
                                       void *data, size_t data_size) const
{
  {
    Ioss::SerializeIO serializeIO__(this);

    size_t num_to_get = field.verify(data_size);
    if (num_to_get > 0) {

      int64_t               id            = Ioex::get_id(eb, EX_EDGE_BLOCK, &ids_);
      int64_t               my_edge_count = eb->entity_count();
      Ioss::Field::RoleType role          = field.get_role();

      if (role == Ioss::Field::MESH) {
        // Handle the MESH fields required for an Exodus file model.
        // (The 'genesis' portion)

        if (field.get_name() == "connectivity") {
          int edge_nodes = eb->get_property("topology_node_count").get_int();
          assert(field.raw_storage()->component_count() == edge_nodes);

          // The connectivity is stored in a 1D array.
          // The edge_node index varies fastet
          if (my_edge_count > 0) {
            get_connectivity_data(get_file_pointer(), data, EX_EDGE_BLOCK, id, 0);
            get_map(EX_NODE_BLOCK).map_data(data, field, num_to_get * edge_nodes);
          }
        }
        else if (field.get_name() == "connectivity_raw") {
          // "connectivity_raw" has nodes in local id space (1-based)
          assert(field.raw_storage()->component_count() ==
                 eb->get_property("topology_node_count").get_int());

          // The connectivity is stored in a 1D array.
          // The edge_node index varies fastet
          if (my_edge_count > 0) {
            get_connectivity_data(get_file_pointer(), data, EX_EDGE_BLOCK, id, 0);
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
        num_to_get = read_attribute_field(EX_EDGE_BLOCK, field, eb, data);
      }
      else if (role == Ioss::Field::TRANSIENT) {
        // Check if the specified field exists on this element block.
        // Note that 'higher-order' storage types (e.g. SYM_TENSOR)
        // exist on the database as scalars with the appropriate
        // extensions.

        // Read in each component of the variable and transfer into
        // 'data'.  Need temporary storage area of size 'number of
        // elements in this block.
        num_to_get =
            read_transient_field(EX_EDGE_BLOCK, m_variables[EX_EDGE_BLOCK], field, eb, data);
      }
      else if (role == Ioss::Field::REDUCTION) {
        get_reduction_field(EX_EDGE_BLOCK, field, eb, data);
      }
    }
    return num_to_get;
  }
}

int64_t DatabaseIO::get_Xset_field_internal(ex_entity_type type, const Ioss::EntitySet *ns,
                                            const Ioss::Field &field, void *data,
                                            size_t data_size) const
{
  {
    int               ierr;
    Ioss::SerializeIO serializeIO__(this);

    size_t num_to_get = field.verify(data_size);
    if (num_to_get > 0) {

      int64_t               id   = Ioex::get_id(ns, type, &ids_);
      Ioss::Field::RoleType role = field.get_role();
      if (role == Ioss::Field::MESH) {

        if (field.get_name() == "ids" || field.get_name() == "ids_raw") {
          if (field.get_type() == Ioss::Field::INTEGER) {
            ierr = ex_get_set(get_file_pointer(), type, id, static_cast<int *>(data), nullptr);
          }
          else {
            ierr = ex_get_set(get_file_pointer(), type, id, static_cast<int64_t *>(data), nullptr);
          }
          if (ierr < 0) {
            Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
          }

          if (field.get_name() == "ids") {
            // Convert the local node ids to global ids
            get_map(type).map_data(data, field, num_to_get);
          }
        }
        else if (field.get_name() == "orientation") {
          if (field.get_type() == Ioss::Field::INTEGER) {
            ierr = ex_get_set(get_file_pointer(), type, id, nullptr, static_cast<int *>(data));
          }
          else {
            ierr = ex_get_set(get_file_pointer(), type, id, nullptr, static_cast<int64_t *>(data));
          }
          if (ierr < 0) {
            Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
          }
        }
        else if (field.get_name() == "distribution_factors") {
          ex_set set_param[1];
          set_param[0].id                       = id;
          set_param[0].type                     = type;
          set_param[0].entry_list               = nullptr;
          set_param[0].extra_list               = nullptr;
          set_param[0].distribution_factor_list = nullptr;
          ierr                                  = ex_get_sets(get_file_pointer(), 1, set_param);
          if (ierr < 0) {
            Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
          }

          if (set_param[0].num_distribution_factor == 0) {
            double *rdata = static_cast<double *>(data);
            for (size_t i = 0; i < num_to_get; i++) {
              rdata[i] = 1.0;
            }
          }
          else {
            set_param[0].distribution_factor_list = static_cast<double *>(data);
            ierr                                  = ex_get_sets(get_file_pointer(), 1, set_param);
            if (ierr < 0) {
              Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
            }
          }
        }
        else {
          num_to_get = Ioss::Utils::field_warning(ns, field, "input");
        }
      }
      else if (role == Ioss::Field::ATTRIBUTE) {
        num_to_get = read_attribute_field(type, field, ns, data);
      }
      else if (role == Ioss::Field::REDUCTION) {
        get_reduction_field(type, field, ns, data);
      }
      else if (role == Ioss::Field::TRANSIENT) {
        // Check if the specified field exists on this node block.
        // Note that 'higher-order' storage types (e.g. SYM_TENSOR)
        // exist on the database as scalars with the appropriate
        // extensions.

        // Read in each component of the variable and transfer into
        // 'data'.  Need temporary storage area of size 'number of
        // nodes in this block.
        num_to_get = read_transient_field(type, m_variables[type], field, ns, data);
      }
    }
    return num_to_get;
  }
}

int64_t DatabaseIO::get_field_internal(const Ioss::NodeSet *ns, const Ioss::Field &field,
                                       void *data, size_t data_size) const
{
  if (!ns->property_exists("filtered_db_set_size")) {
    return get_Xset_field_internal(EX_NODE_SET, ns, field, data, data_size);
  }

  size_t db_size = ns->get_property("filtered_db_set_size").get_int();

  int               ierr;
  Ioss::SerializeIO serializeIO__(this);

  size_t num_to_get = field.verify(data_size);
  if (num_to_get > 0) {

    int64_t               id   = Ioex::get_id(ns, EX_NODE_SET, &ids_);
    Ioss::Field::RoleType role = field.get_role();
    if (role == Ioss::Field::MESH) {

      if (field.get_name() == "ids" || field.get_name() == "ids_raw") {
        if (field.get_type() == Ioss::Field::INTEGER) {
          Ioss::IntVector dbvals(db_size);
          ierr = ex_get_set(get_file_pointer(), EX_NODE_SET, id, dbvals.data(), nullptr);
          if (ierr >= 0) {
            Ioex::filter_node_list(static_cast<int *>(data), dbvals,
                                   activeNodeSetNodesIndex[ns->name()]);
          }
        }
        else {
          Ioss::Int64Vector dbvals(db_size);
          ierr = ex_get_set(get_file_pointer(), EX_NODE_SET, id, dbvals.data(), nullptr);
          if (ierr >= 0) {
            Ioex::filter_node_list(static_cast<int64_t *>(data), dbvals,
                                   activeNodeSetNodesIndex[ns->name()]);
          }
        }
        if (ierr < 0) {
          Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
        }

        if (field.get_name() == "ids") {
          // Convert the local node ids to global ids
          get_map(EX_NODE_SET).map_data(data, field, num_to_get);
        }
      }
      else if (field.get_name() == "distribution_factors") {
        ex_set set_param[1];
        set_param[0].id                       = id;
        set_param[0].type                     = EX_NODE_SET;
        set_param[0].entry_list               = nullptr;
        set_param[0].extra_list               = nullptr;
        set_param[0].distribution_factor_list = nullptr;
        ierr                                  = ex_get_sets(get_file_pointer(), 1, set_param);
        if (ierr < 0) {
          Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
        }

        if (set_param[0].num_distribution_factor == 0) {
          double *rdata = static_cast<double *>(data);
          for (size_t i = 0; i < num_to_get; i++) {
            rdata[i] = 1.0;
          }
        }
        else {
          std::vector<double> dbvals(db_size);
          set_param[0].distribution_factor_list = dbvals.data();
          ierr                                  = ex_get_sets(get_file_pointer(), 1, set_param);
          if (ierr < 0) {
            Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
          }
          Ioex::filter_node_list(static_cast<double *>(data), dbvals,
                                 activeNodeSetNodesIndex[ns->name()]);
          set_param[0].distribution_factor_list = nullptr;
        }
      }
      else {
        num_to_get = Ioss::Utils::field_warning(ns, field, "input");
      }
    }
    else if (role == Ioss::Field::ATTRIBUTE) {
      num_to_get = Ioss::Utils::field_warning(ns, field, "input");
    }
    else if (role == Ioss::Field::REDUCTION) {
      num_to_get = Ioss::Utils::field_warning(ns, field, "input");
    }
    else if (role == Ioss::Field::TRANSIENT) {
      // Filtered not currently implemented for transient or attributes....
    }
  }
  return num_to_get;
}

int64_t DatabaseIO::get_field_internal(const Ioss::EdgeSet *ns, const Ioss::Field &field,
                                       void *data, size_t data_size) const
{
  return get_Xset_field_internal(EX_EDGE_SET, ns, field, data, data_size);
}

int64_t DatabaseIO::get_field_internal(const Ioss::FaceSet *ns, const Ioss::Field &field,
                                       void *data, size_t data_size) const
{
  return get_Xset_field_internal(EX_FACE_SET, ns, field, data, data_size);
}

int64_t DatabaseIO::get_field_internal(const Ioss::ElementSet *ns, const Ioss::Field &field,
                                       void *data, size_t data_size) const
{
  return get_Xset_field_internal(EX_ELEM_SET, ns, field, data, data_size);
}

int64_t DatabaseIO::get_field_internal(const Ioss::SideSet *fs, const Ioss::Field &field,
                                       void * /* data */, size_t                   data_size) const
{
  size_t num_to_get = field.verify(data_size);
  if (field.get_name() == "ids") {
    // Do nothing, just handles an idiosyncrasy of the GroupingEntity
  }
  else {
    num_to_get = Ioss::Utils::field_warning(fs, field, "input");
  }
  return num_to_get;
}

int64_t DatabaseIO::get_field_internal(const Ioss::CommSet *cs, const Ioss::Field &field,
                                       void *data, size_t data_size) const
{
  {
    Ioss::SerializeIO serializeIO__(this);

    size_t num_to_get = field.verify(data_size);

    if (num_to_get > 0) {
      int64_t entity_count = cs->entity_count();

      // Return the <entity (node or side), processor> pair
      if (field.get_name() == "entity_processor" || field.get_name() == "entity_processor_raw") {

        // Check type -- node or side
        std::string type = cs->get_property("entity_type").get_string();

        // Allocate temporary storage space
        std::vector<char> entities(num_to_get * int_byte_size_api());
        std::vector<char> procs(num_to_get * int_byte_size_api());

        if (type == "node") {
          int64_t cm_offset = 0;

          for (int64_t i = 0; i < commsetNodeCount; i++) {
            int ierr = ex_get_node_cmap(get_file_pointer(), nodeCmapIds[i], &entities[cm_offset],
                                        &procs[cm_offset], myProcessor);
            if (ierr < 0) {
              Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
            }
            cm_offset += (nodeCmapNodeCnts[i] * int_byte_size_api());
          }
          assert(cm_offset == entity_count * int_byte_size_api());

          // Convert local node id to global node id and store in 'data'
          if (int_byte_size_api() == 4) {
            int *entity_proc = static_cast<int *>(data);
            int *ents        = reinterpret_cast<int *>(&entities[0]);
            int *pros        = reinterpret_cast<int *>(&procs[0]);

            size_t j = 0;
            if (field.get_name() == "entity_processor") {
              const Ioss::MapContainer &map = get_map(EX_NODE_BLOCK).map();

              for (int64_t i = 0; i < entity_count; i++) {
                int local_id     = ents[i];
                entity_proc[j++] = map[local_id];
                entity_proc[j++] = pros[i];
              }
            }
            else {
              for (int64_t i = 0; i < entity_count; i++) {
                entity_proc[j++] = ents[i];
                entity_proc[j++] = pros[i];
              }
            }
          }
          else {
            int64_t *entity_proc = static_cast<int64_t *>(data);
            int64_t *ents        = reinterpret_cast<int64_t *>(&entities[0]);
            int64_t *pros        = reinterpret_cast<int64_t *>(&procs[0]);

            size_t j = 0;
            if (field.get_name() == "entity_processor") {
              const Ioss::MapContainer &map = get_map(EX_NODE_BLOCK).map();

              for (int64_t i = 0; i < entity_count; i++) {
                int64_t local_id = ents[i];
                entity_proc[j++] = map[local_id];
                entity_proc[j++] = pros[i];
              }
            }
            else {
              for (int64_t i = 0; i < entity_count; i++) {
                entity_proc[j++] = ents[i];
                entity_proc[j++] = pros[i];
              }
            }
          }
        }
        else if (type == "side") {
          std::vector<char> sides(entity_count * int_byte_size_api());
          int64_t           cm_offset = 0;
          for (int64_t i = 0; i < commsetElemCount; i++) {
            int ierr = ex_get_elem_cmap(get_file_pointer(), elemCmapIds[i], &entities[cm_offset],
                                        &sides[cm_offset], &procs[cm_offset], myProcessor);
            if (ierr < 0) {
              Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
            }
            cm_offset += (elemCmapElemCnts[i] * int_byte_size_api());
          }
          assert(cm_offset == entity_count * int_byte_size_api());

          if (int_byte_size_api() == 4) {
            int *entity_proc = static_cast<int *>(data);
            int *ents        = reinterpret_cast<int *>(&entities[0]);
            int *pros        = reinterpret_cast<int *>(&procs[0]);
            int *sids        = reinterpret_cast<int *>(&sides[0]);

            size_t j = 0;
            if (field.get_name() == "entity_processor") {
              const Ioss::MapContainer &map = get_map(EX_ELEM_BLOCK).map();

              for (ssize_t i = 0; i < entity_count; i++) {
                entity_proc[j++] = map[ents[i]];
                entity_proc[j++] = sids[i];
                entity_proc[j++] = pros[i];
              }
            }
            else { // "entity_processor_raw"
              for (ssize_t i = 0; i < entity_count; i++) {
                entity_proc[j++] = ents[i];
                entity_proc[j++] = sids[i];
                entity_proc[j++] = pros[i];
              }
            }
          }
          else {
            int64_t *entity_proc = static_cast<int64_t *>(data);
            int64_t *ents        = reinterpret_cast<int64_t *>(&entities[0]);
            int64_t *pros        = reinterpret_cast<int64_t *>(&procs[0]);
            int64_t *sids        = reinterpret_cast<int64_t *>(&sides[0]);

            size_t j = 0;
            if (field.get_name() == "entity_processor") {
              const Ioss::MapContainer &map = get_map(EX_ELEM_BLOCK).map();

              for (ssize_t i = 0; i < entity_count; i++) {
                entity_proc[j++] = map[ents[i]];
                entity_proc[j++] = sids[i];
                entity_proc[j++] = pros[i];
              }
            }
            else { // "entity_processor_raw"
              for (ssize_t i = 0; i < entity_count; i++) {
                entity_proc[j++] = ents[i];
                entity_proc[j++] = sids[i];
                entity_proc[j++] = pros[i];
              }
            }
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
    }
    return num_to_get;
  }
}

int64_t DatabaseIO::get_field_internal(const Ioss::SideBlock *fb, const Ioss::Field &field,
                                       void *data, size_t data_size) const
{
  Ioss::SerializeIO serializeIO__(this);
  ssize_t           num_to_get = field.verify(data_size);
  if (num_to_get > 0) {

    int64_t id           = Ioex::get_id(fb, EX_SIDE_SET, &ids_);
    int64_t entity_count = fb->entity_count();
    if (num_to_get != entity_count) {
      std::ostringstream errmsg;
      fmt::print(errmsg, "ERROR: Partial field input not yet implemented for side blocks");
      IOSS_ERROR(errmsg);
    }

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

    int64_t number_sides                = set_param[0].num_entry;
    int64_t number_distribution_factors = set_param[0].num_distribution_factor;

    Ioss::Field::RoleType role = field.get_role();
    if (role == Ioss::Field::MESH) {

      // In exodus, we may have split the sideset into multiple
      // side blocks if there are multiple side topologies in the
      // sideset.  Because of this, the passed in 'data' may not be
      // large enough to hold the data residing in the sideset and we
      // may need to allocate a temporary array...  This can be checked
      // by comparing the size of the sideset with the 'side_count' of
      // the side block.

      // Get size of data stored on the file...
      // FIX 64: FIX THIS -- STORING INT IN DOUBLE WON'T WORK
      if (field.get_name() == "side_ids" && fb->name() == "universal_sideset") {
        // The side ids are being stored as the distribution factor
        // field on the universal sideset.  There should be no other
        // side sets that request this field...  (Eventually,
        // create an id field to store this info.

        if (number_distribution_factors == num_to_get) {
          std::vector<double> real_ids(num_to_get);
          set_param[0].distribution_factor_list = real_ids.data();
          ierr                                  = ex_get_sets(get_file_pointer(), 1, set_param);
          if (ierr < 0) {
            Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
          }
          set_param[0].distribution_factor_list = nullptr;

          if (field.get_type() == Ioss::Field::INTEGER) {
            // Need to convert 'double' to 'int' for Sierra use...
            int *ids = static_cast<int *>(data);
            for (ssize_t i = 0; i < num_to_get; i++) {
              ids[i] = static_cast<int>(real_ids[i]);
            }
          }
          else {
            // Need to convert 'double' to 'int' for Sierra use...
            int64_t *ids = static_cast<int64_t *>(data);
            for (ssize_t i = 0; i < num_to_get; i++) {
              ids[i] = static_cast<int64_t>(real_ids[i]);
            }
          }
        }
      }

      else if (field.get_name() == "side_ids") {
      }

      else if (field.get_name() == "ids") {
        // In exodus, the 'side set' is stored as a sideset.  A
        // sideset has a list of elements and a corresponding local
        // element side (1-based) The side id is: side_id =
        // 10*element_id + local_side_number This assumes that all
        // sides in a sideset are boundary sides.  Since we
        // only have a single array, we need to allocate an extra array
        // to store all of the data.  Note also that the element_id is
        // the global id but only the local id is stored so we need to
        // map from local_to_global prior to generating the side id...

        Ioss::Field       el_side = fb->get_field("element_side");
        std::vector<char> element_side(2 * number_sides * int_byte_size_api());
        get_field_internal(fb, el_side, element_side.data(), element_side.size());

        // At this point, have the 'element_side' data containing
        // the global element ids and the sides...  Iterate
        // through to generate the ids...
        if (int_byte_size_api() == 4) {
          int64_t int_max = std::numeric_limits<int>::max();
          int *   ids     = static_cast<int *>(data);
          int *   els     = reinterpret_cast<int *>(element_side.data());
          size_t  idx     = 0;
          for (ssize_t iel = 0; iel < 2 * entity_count; iel += 2) {
            int64_t new_id = static_cast<int64_t>(10) * els[iel] + els[iel + 1];
            if (new_id > int_max) {
              std::ostringstream errmsg;
              fmt::print(errmsg,
                         "ERROR: Process {} accessing the sideset field 'ids'\n"
                         "\t\thas exceeded the integer bounds for entity {}, local side id {}"
                         ".\n\t\tTry using 64-bit mode to read the file '{}'.\n",
                         Ioss::SerializeIO::getRank(), els[iel], els[iel + 1], decoded_filename());
              IOSS_ERROR(errmsg);
            }

            ids[idx++] = static_cast<int>(new_id);
          }
        }
        else {
          int64_t *ids = static_cast<int64_t *>(data);
          int64_t *els = reinterpret_cast<int64_t *>(element_side.data());
          size_t   idx = 0;
          for (ssize_t iel = 0; iel < 2 * entity_count; iel += 2) {
            int64_t new_id = 10 * els[iel] + els[iel + 1];
            ids[idx++]     = new_id;
          }
        }
      }
      else if (field.get_name() == "element_side") {
        // In exodus, the 'side set' is stored as a sideset.  A sideset
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
        int64_t side_offset = Ioss::Utils::get_side_offset(fb);

        std::vector<char> element(number_sides * int_byte_size_api());
        std::vector<char> sides(number_sides * int_byte_size_api());

        ierr = ex_get_set(get_file_pointer(), EX_SIDE_SET, id, element.data(), sides.data());
        if (ierr < 0) {
          Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
        }

        if (number_sides == entity_count) {
          ssize_t index = 0;
          if (int_byte_size_api() == 4) {
            int *element_side = static_cast<int *>(data);
            int *element32    = reinterpret_cast<int *>(element.data());
            int *sides32      = reinterpret_cast<int *>(sides.data());
            for (ssize_t iel = 0; iel < entity_count; iel++) {
              element_side[index++] = map[element32[iel]];
              element_side[index++] = sides32[iel] - side_offset;
            }
          }
          else {
            int64_t *element_side = static_cast<int64_t *>(data);
            int64_t *element64    = reinterpret_cast<int64_t *>(element.data());
            int64_t *sides64      = reinterpret_cast<int64_t *>(sides.data());
            for (ssize_t iel = 0; iel < entity_count; iel++) {
              element_side[index++] = map[element64[iel]];
              element_side[index++] = sides64[iel] - side_offset;
            }
          }
          assert(index / 2 == entity_count);
        }
        else {
          Ioss::IntVector is_valid_side;
          Ioss::Utils::calculate_sideblock_membership(is_valid_side, fb, int_byte_size_api(),
                                                      element.data(), sides.data(), number_sides,
                                                      get_region());

          ssize_t index = 0;
          if (int_byte_size_api() == 4) {
            int *element_side = static_cast<int *>(data);
            int *element32    = reinterpret_cast<int *>(element.data());
            int *sides32      = reinterpret_cast<int *>(sides.data());
            for (int64_t iel = 0; iel < number_sides; iel++) {
              if (is_valid_side[iel] == 1) {
                // This side  belongs in the side block
                element_side[index++] = map[element32[iel]];
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
                element_side[index++] = map[element64[iel]];
                element_side[index++] = sides64[iel] - side_offset;
              }
            }
          }
          assert(index / 2 == entity_count);
        }
      }
      else if (field.get_name() == "element_side_raw") {
        // In exodus, the 'side set' is stored as a sideset.  A sideset
        // has a list of elements and a corresponding local element side
        // (1-based)

        // Since we only have a single array, we need to allocate an extra
        // array to store all of the data.  Note also that the
        // element_id for the "raw" field is the local id, not the
        // global id.

        // See if edges or faces...
        int64_t side_offset = Ioss::Utils::get_side_offset(fb);

        std::vector<char> element(number_sides * int_byte_size_api());
        std::vector<char> sides(number_sides * int_byte_size_api());

        ierr = ex_get_set(get_file_pointer(), EX_SIDE_SET, id, element.data(), sides.data());
        if (ierr < 0) {
          Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
        }

        if (number_sides == entity_count) {
          ssize_t index = 0;
          if (int_byte_size_api() == 4) {
            int *element_side = static_cast<int *>(data);
            int *element32    = reinterpret_cast<int *>(element.data());
            int *sides32      = reinterpret_cast<int *>(sides.data());
            for (ssize_t iel = 0; iel < entity_count; iel++) {
              element_side[index++] = element32[iel];
              element_side[index++] = sides32[iel] - side_offset;
            }
          }
          else {
            int64_t *element_side = static_cast<int64_t *>(data);
            int64_t *element64    = reinterpret_cast<int64_t *>(element.data());
            int64_t *sides64      = reinterpret_cast<int64_t *>(sides.data());
            for (ssize_t iel = 0; iel < entity_count; iel++) {
              element_side[index++] = element64[iel];
              element_side[index++] = sides64[iel] - side_offset;
            }
          }
          assert(index / 2 == entity_count);
        }
        else {
          Ioss::IntVector is_valid_side;
          Ioss::Utils::calculate_sideblock_membership(is_valid_side, fb, int_byte_size_api(),
                                                      element.data(), sides.data(), number_sides,
                                                      get_region());

          ssize_t index = 0;
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
      }
      else if (field.get_name() == "connectivity") {
        // The side  connectivity needs to be generated 'on-the-fly' from
        // the element number and local side  of that element. A sideset
        // can span multiple element blocks, and contain multiple side
        // types; the side block contains side of similar topology.
        ierr = get_side_connectivity(fb, id, entity_count, data, true);
        if (ierr < 0) {
          Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
        }
      }
      else if (field.get_name() == "connectivity_raw") {
        // The side  connectivity needs to be generated 'on-the-fly' from
        // the element number and local side  of that element. A sideset
        // can span multiple element blocks, and contain multiple side
        // types; the side block contains side of similar topology.
        ierr = get_side_connectivity(fb, id, entity_count, data, false);
        if (ierr < 0) {
          Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
        }
      }
      else if (field.get_name() == "distribution_factors") {
        ierr = get_side_distributions(fb, id, entity_count, static_cast<double *>(data),
                                      data_size / sizeof(double));
        if (ierr < 0) {
          Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
        }
      }
      else {
        num_to_get = Ioss::Utils::field_warning(fb, field, "input");
      }
    }
    else if (role == Ioss::Field::TRANSIENT) {
      // Check if the specified field exists on this block.
      // Note that 'higher-order' storage types (e.g. SYM_TENSOR)
      // exist on the database as scalars with the appropriate
      // extensions.

      if (number_sides == entity_count) {
        num_to_get = read_transient_field(EX_SIDE_SET, m_variables[EX_SIDE_SET], field, fb, data);
      }
      else {
        // Need to read all values for the specified field and then
        // filter down to the elements actually in this side block.

        // Determine which sides are member of this block
        Ioss::IntVector is_valid_side;
        {
          //----
          std::vector<char> element(number_sides * int_byte_size_api());
          std::vector<char> sides(number_sides * int_byte_size_api());
          ierr = ex_get_set(get_file_pointer(), EX_SIDE_SET, id, element.data(), sides.data());
          if (ierr < 0) {
            Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
          }
          //----
          Ioss::Utils::calculate_sideblock_membership(is_valid_side, fb, int_byte_size_api(),
                                                      element.data(), sides.data(), number_sides,
                                                      get_region());
        }

        num_to_get = read_ss_transient_field(field, id, data, is_valid_side);
      }
    }
  }
  return num_to_get;
}

int64_t DatabaseIO::write_attribute_field(ex_entity_type type, const Ioss::Field &field,
                                          const Ioss::GroupingEntity *ge, void *data) const
{
  std::string att_name   = ge->name() + SEP() + field.get_name();
  ssize_t     num_entity = ge->entity_count();
  ssize_t     fld_offset = field.get_index();

  int64_t id              = Ioex::get_id(ge, type, &ids_);
  int     attribute_count = ge->get_property("attribute_count").get_int();
  assert(fld_offset > 0);
  assert(fld_offset - 1 + field.raw_storage()->component_count() <= attribute_count);

  Ioss::Field::BasicType ioss_type = field.get_type();
  assert(ioss_type == Ioss::Field::REAL || ioss_type == Ioss::Field::INTEGER ||
         ioss_type == Ioss::Field::INT64);

  if (fld_offset == 1 && field.raw_storage()->component_count() == attribute_count) {
    // Write all attributes in one big chunk...
    std::vector<double> temp;
    double *            rdata = nullptr;
    if (ioss_type == Ioss::Field::INTEGER) {
      int *idata = static_cast<int *>(data);
      extract_data(temp, idata, attribute_count * num_entity, 1, 0);
      rdata = temp.data();
    }
    else if (ioss_type == Ioss::Field::INT64) {
      int64_t *idata = static_cast<int64_t *>(data);
      extract_data(temp, idata, attribute_count * num_entity, 1, 0);
      rdata = temp.data();
    }
    else {
      rdata = static_cast<double *>(data);
    }

    int ierr = ex_put_attr(get_file_pointer(), type, id, rdata);
    if (ierr < 0) {
      Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
    }
  }
  else {
    // Write a subset of the attributes.  If scalar, write one;
    // if higher-order (vector3d, ..) write each component.
    if (field.raw_storage()->component_count() == 1) {
      std::vector<double> temp;
      double *            rdata = nullptr;
      if (ioss_type == Ioss::Field::INTEGER) {
        int *idata = static_cast<int *>(data);
        extract_data(temp, idata, num_entity, 1, 0);
        rdata = temp.data();
      }
      else if (ioss_type == Ioss::Field::INT64) {
        int64_t *idata = static_cast<int64_t *>(data);
        extract_data(temp, idata, num_entity, 1, 0);
        rdata = temp.data();
      }
      else {
        rdata = static_cast<double *>(data);
      }

      int ierr = ex_put_one_attr(get_file_pointer(), type, id, fld_offset, rdata);
      if (ierr < 0) {
        Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
      }
    }
    else {
      // Multi-component...  Need a local memory space to push
      // data into and then write that out to the file...
      std::vector<double> local_data(num_entity);
      int                 comp_count = field.raw_storage()->component_count();
      for (int i = 0; i < comp_count; i++) {
        size_t offset = i;
        if (ioss_type == Ioss::Field::REAL) {
          double *rdata = static_cast<double *>(data);
          extract_data(local_data, rdata, num_entity, comp_count, offset);
        }
        else if (ioss_type == Ioss::Field::INTEGER) {
          int *idata = static_cast<int *>(data);
          extract_data(local_data, idata, num_entity, comp_count, offset);
        }
        else if (ioss_type == Ioss::Field::INT64) {
          int64_t *idata = static_cast<int64_t *>(data);
          extract_data(local_data, idata, num_entity, comp_count, offset);
        }

        int ierr = ex_put_one_attr(get_file_pointer(), type, id, fld_offset + i, local_data.data());
        if (ierr < 0) {
          Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
        }
      }
    }
  }
  return num_entity;
}

int64_t DatabaseIO::read_attribute_field(ex_entity_type type, const Ioss::Field &field,
                                         const Ioss::GroupingEntity *ge, void *data) const
{
  // TODO: Handle INTEGER fields...

  int64_t num_entity = ge->entity_count();
  if (num_entity == 0) {
    return 0;
  }

  Ioss::Field::BasicType ioss_type = field.get_type();
  if (ioss_type == Ioss::Field::INTEGER || ioss_type == Ioss::Field::INT64) {
    std::ostringstream errmsg;
    fmt::print(errmsg, "INTERNAL ERROR: Integer attribute fields are not yet handled for read. "
                       "Please report.\n");
    IOSS_ERROR(errmsg);
  }

  int     attribute_count = ge->get_property("attribute_count").get_int();
  int64_t id              = Ioex::get_id(ge, type, &ids_);

  std::string att_name = ge->name() + SEP() + field.get_name();
  ssize_t     offset   = field.get_index();
  assert(offset - 1 + field.raw_storage()->component_count() <= attribute_count);
  if (offset == 1 && field.raw_storage()->component_count() == attribute_count) {
    // Read all attributes in one big chunk...
    int ierr = ex_get_attr(get_file_pointer(), type, id, static_cast<double *>(data));
    if (ierr < 0) {
      Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
    }
  }
  else {
    // Read a subset of the attributes.  If scalar, read one;
    // if higher-order (vector3d, ..) read each component and
    // put into correct location...
    if (field.raw_storage()->component_count() == 1) {
      int ierr = ex_get_one_attr(get_file_pointer(), type, id, offset, static_cast<double *>(data));
      if (ierr < 0) {
        Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
      }
    }
    else {
      // Multi-component...
      // Need a local memory space to read data into and
      // then push that into the user-supplied data block...
      std::vector<double> local_data(num_entity);
      int                 comp_count = field.raw_storage()->component_count();
      double *            rdata      = static_cast<double *>(data);
      for (int i = 0; i < comp_count; i++) {
        int ierr = ex_get_one_attr(get_file_pointer(), type, id, offset + i, local_data.data());
        if (ierr < 0) {
          Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
        }

        size_t k = i;
        for (ssize_t j = 0; j < num_entity; j++) {
          rdata[k] = local_data[j];
          k += comp_count;
        }
      }
    }
  }
  return num_entity;
}

int64_t DatabaseIO::read_transient_field(ex_entity_type               type,
                                         const Ioex::VariableNameMap &variables,
                                         const Ioss::Field &field, const Ioss::GroupingEntity *ge,
                                         void *data) const
{
  const Ioss::VariableType *var_type = field.raw_storage();

  // Read into a double variable since that is all Exodus can store...
  size_t              num_entity = ge->entity_count();
  std::vector<double> temp(num_entity);

  size_t step = get_current_state();

  // get number of components, cycle through each component
  // and add suffix to base 'field_name'.  Look up index
  // of this name in 'nodeVariables' map
  size_t comp_count = var_type->component_count();
  size_t var_index  = 0;

  char field_suffix_separator = get_field_separator();
  if (comp_count == 1 && field.get_type() == Ioss::Field::REAL) {
    std::string var_name = var_type->label_name(field.get_name(), 1, field_suffix_separator);

    // Read the variable...
    int64_t id   = Ioex::get_id(ge, type, &ids_);
    int     ierr = 0;
    var_index    = variables.find(var_name)->second;
    assert(var_index > 0);
    ierr = ex_get_var(get_file_pointer(), step, type, var_index, id, num_entity, data);
    if (ierr < 0) {
      Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
    }
  }
  else {
    for (size_t i = 0; i < comp_count; i++) {
      std::string var_name = var_type->label_name(field.get_name(), i + 1, field_suffix_separator);

      // Read the variable...
      int64_t id   = Ioex::get_id(ge, type, &ids_);
      int     ierr = 0;
      var_index    = variables.find(var_name)->second;
      assert(var_index > 0);
      ierr = ex_get_var(get_file_pointer(), step, type, var_index, id, num_entity, temp.data());
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
  }
  return num_entity;
}

int64_t DatabaseIO::read_ss_transient_field(const Ioss::Field &field, int64_t id, void *variables,
                                            Ioss::IntVector &is_valid_side) const
{
  size_t                    num_valid_sides = 0;
  const Ioss::VariableType *var_type        = field.raw_storage();
  size_t                    my_side_count   = is_valid_side.size();
  std::vector<double>       temp(my_side_count);

  size_t step = get_current_state();

  // get number of components, cycle through each component
  // and add suffix to base 'field_name'.  Look up index
  // of this name in 'nodeVariables' map
  size_t comp_count = var_type->component_count();
  size_t var_index  = 0;

  char field_suffix_separator = get_field_separator();
  for (size_t i = 0; i < comp_count; i++) {
    std::string var_name = var_type->label_name(field.get_name(), i + 1, field_suffix_separator);

    // Read the variable...
    int ierr  = 0;
    var_index = m_variables[EX_SIDE_SET].find(var_name)->second;
    assert(var_index > 0);
    ierr = ex_get_var(get_file_pointer(), step, EX_SIDE_SET, var_index, id, my_side_count,
                      temp.data());
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

template <typename INT>
int64_t DatabaseIO::get_side_connectivity_internal(const Ioss::SideBlock *fb, int64_t id,
                                                   int64_t /*unused*/, INT *          fconnect,
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
  //----
  std::vector<INT> element(number_sides);
  std::vector<INT> side(number_sides);

  set_param[0].entry_list = element.data();
  set_param[0].extra_list = side.data();
  ierr                    = ex_get_sets(get_file_pointer(), 1, set_param);
  if (ierr < 0) {
    Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
  }
  //----

  Ioss::IntVector is_valid_side;
  Ioss::Utils::calculate_sideblock_membership(is_valid_side, fb, int_byte_size_api(),
                                              (void *)element.data(), (void *)side.data(),
                                              number_sides, get_region());

  std::vector<INT>    elconnect;
  int64_t             elconsize  = 0;       // Size of currently allocated connectivity block
  Ioss::ElementBlock *conn_block = nullptr; // Block that we currently
  // have connectivity for

  Ioss::ElementBlock *block = nullptr;
  Ioss::IntVector     side_elem_map; // Maps the side into the elements

  // connectivity array
  int64_t current_side = -1;
  int     nelnode      = 0;
  int     nfnodes      = 0;
  int     ieb          = 0;
  size_t  offset       = 0;
  for (ssize_t iel = 0; iel < number_sides; iel++) {
    if (is_valid_side[iel] == 1) {

      int64_t elem_id = element[iel];

      // ensure we have correct connectivity
      block = get_region()->get_element_block(elem_id);
      assert(block != nullptr);
      if (conn_block != block) {
        ssize_t nelem = block->entity_count();
        nelnode       = block->topology()->number_nodes();
        // Used to map element number into position in connectivity array.
        // E.g., element 97 is the (97-offset)th element in this block and
        // is stored in array index (97-offset-1).
        offset = block->get_offset() + 1;
        if (elconsize < nelem * nelnode) {
          elconsize = nelem * nelnode;
          elconnect.resize(elconsize);
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
      int64_t side_id = side[iel];

      if (current_side != side_id) {
        side_elem_map = block->topology()->boundary_connectivity(side_id);
        current_side  = side_id;
        nfnodes       = block->topology()->boundary_type(side_id)->number_nodes();
      }
      for (int inode = 0; inode < nfnodes; inode++) {
        size_t index    = (elem_id - offset) * nelnode + side_elem_map[inode];
        fconnect[ieb++] = elconnect[index];
      }
    }
  }
  return ierr;
}

int64_t DatabaseIO::get_side_connectivity(const Ioss::SideBlock *fb, int64_t id,
                                          int64_t my_side_count, void *fconnect, bool map_ids) const
{
  if (int_byte_size_api() == 4) {
    return get_side_connectivity_internal(fb, id, my_side_count, reinterpret_cast<int *>(fconnect),
                                          map_ids);
  }
  return get_side_connectivity_internal(fb, id, my_side_count,
                                        reinterpret_cast<int64_t *>(fconnect), map_ids);
}

// Get distribution factors for the specified side block
int64_t DatabaseIO::get_side_distributions(const Ioss::SideBlock *fb, int64_t id,
                                           int64_t my_side_count, double *dist_fact,
                                           size_t /* data_size */) const
{
  // Allocate space for elements and local side numbers
  // Get size of data stored on the file...
  ex_set set_param[1];
  set_param[0].id                       = id;
  set_param[0].type                     = EX_SIDE_SET;
  set_param[0].entry_list               = nullptr;
  set_param[0].extra_list               = nullptr;
  set_param[0].distribution_factor_list = nullptr;

  int error = ex_get_sets(get_file_pointer(), 1, set_param);
  if (error < 0) {
    Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
  }
  int64_t number_sides                = set_param[0].num_entry;
  int64_t number_distribution_factors = set_param[0].num_distribution_factor;

  const Ioss::ElementTopology *ftopo   = fb->topology();
  int                          nfnodes = ftopo->number_nodes();

  if (number_distribution_factors == 0) {
    // Fill in the array with '1.0'...
    for (int64_t i = 0; i < nfnodes * my_side_count; i++) {
      dist_fact[i] = 1.0;
    }
    return 0;
  }

  // Take care of the easy situation -- If 'side_count' ==
  // 'number_sides' then the sideset is stored in a single sideblock
  // and all distribution factors on the database are transferred
  // 1-to-1 into 'dist_fact' array.
  if (my_side_count == number_sides) {
    // Verify that number_distribution_factors is sane...
    if (number_sides * nfnodes != number_distribution_factors &&
        number_sides != number_distribution_factors) {
      std::ostringstream errmsg;
      fmt::print(errmsg,
                 "ERROR: SideBlock '{}' in file '{}'\n"
                 "\thas incorrect distribution factor count.\n"
                 "\tThere are {} '{}' sides with "
                 "{} nodes per side, but there are {} distribution factors which is not correct.\n"
                 "\tThere should be either {} or {} distribution factors.\n",
                 fb->name(), get_filename(), number_sides, ftopo->name(), nfnodes,
                 number_distribution_factors, number_sides, number_sides * nfnodes);
      IOSS_ERROR(errmsg);
    }
    return ex_get_set_dist_fact(get_file_pointer(), EX_SIDE_SET, id, dist_fact);
  }

  // Allocate space for distribution factors.
  std::vector<double> dist(number_distribution_factors);
  int                 ierr = ex_get_set_dist_fact(get_file_pointer(), EX_SIDE_SET, id, dist.data());
  if (ierr < 0) {
    Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
  }

  // Another easy situation (and common for exodus) is if the input
  // distribution factors are all the same value (typically 1).  In
  // that case, we only have to fill in the output array with that
  // value.
  {
    double value    = dist[0];
    bool   constant = true;
    for (int64_t i = 1; i < number_distribution_factors; i++) {
      if (dist[i] != value) {
        constant = false;
        break;
      }
    }
    if (constant) {
      if (value == 0.0) {
        value = 1.0; // Take care of some buggy mesh generators
      }
      for (ssize_t j = 0; j < my_side_count * nfnodes; j++) {
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
  std::vector<char> side(number_sides * int_byte_size_api());

  ierr = ex_get_set(get_file_pointer(), EX_SIDE_SET, id, element.data(), side.data());
  if (ierr < 0) {
    Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
  }
  //----

  Ioss::IntVector is_valid_side;
  Ioss::Utils::calculate_sideblock_membership(is_valid_side, fb, int_byte_size_api(),
                                              element.data(), side.data(), number_sides,
                                              get_region());

  int64_t             ieb   = 0; // counter for distribution factors in this sideblock
  int64_t             idb   = 0; // counter for distribution factors read from database
  Ioss::ElementBlock *block = nullptr;

  int *    element32 = nullptr;
  int64_t *element64 = nullptr;
  int *    side32    = nullptr;
  int64_t *side64    = nullptr;

  if (int_byte_size_api() == 4) {
    element32 = reinterpret_cast<int *>(element.data());
    side32    = reinterpret_cast<int *>(side.data());
  }
  else {
    element64 = reinterpret_cast<int64_t *>(element.data());
    side64    = reinterpret_cast<int64_t *>(side.data());
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
                 "INTERNAL ERROR: Could not find element block containing element with id {}. "
                 "Something is wrong in the Ioex::DatabaseIO class. Please report.\n",
                 elem_id);
      IOSS_ERROR(errmsg);
    }

    const Ioss::ElementTopology *topo = block->topology()->boundary_type(side_id);

    if (topo == nullptr) {
      std::ostringstream errmsg;
      fmt::print(errmsg, "INTERNAL ERROR: Could not find topology of element block boundary. "
                         "Something is wrong in the Ioex::DatabaseIO class. Please report.\n");
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

int64_t DatabaseIO::put_field_internal(const Ioss::Region *reg, const Ioss::Field &field,
                                       void *data, size_t data_size) const
{
  return Ioex::BaseDatabaseIO::put_field_internal(reg, field, data, data_size);
}

int64_t DatabaseIO::put_field_internal(const Ioss::NodeBlock *nb, const Ioss::Field &field,
                                       void *data, size_t data_size) const
{
  {
    Ioss::SerializeIO serializeIO__(this);

    size_t num_to_get = field.verify(data_size);
    if (num_to_get > 0) {

      Ioss::Field::RoleType role = field.get_role();

      if (role == Ioss::Field::MESH) {
        if (field.get_name() == "mesh_model_coordinates_x") {
          double *rdata = static_cast<double *>(data);
          int     ierr  = ex_put_coord(get_file_pointer(), rdata, nullptr, nullptr);
          if (ierr < 0) {
            Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
          }
        }

        else if (field.get_name() == "mesh_model_coordinates_y") {
          double *rdata = static_cast<double *>(data);
          int     ierr  = ex_put_coord(get_file_pointer(), nullptr, rdata, nullptr);
          if (ierr < 0) {
            Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
          }
        }

        else if (field.get_name() == "mesh_model_coordinates_z") {
          double *rdata = static_cast<double *>(data);
          int     ierr  = ex_put_coord(get_file_pointer(), nullptr, nullptr, rdata);
          if (ierr < 0) {
            Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
          }
        }

        else if (field.get_name() == "mesh_model_coordinates") {
          // Data required by upper classes store x0, y0, z0, ... xn, yn, zn
          // Data stored in exodus file is x0, ..., xn, y0, ..., yn, z0, ..., zn
          // so we have to allocate some scratch memory to read in the data
          // and then map into supplied 'data'
          std::vector<double> x;
          x.reserve(num_to_get);
          std::vector<double> y;
          if (spatialDimension > 1) {
            y.reserve(num_to_get);
          }
          std::vector<double> z;
          if (spatialDimension == 3) {
            z.reserve(num_to_get);
          }

          // Cast 'data' to correct size -- double
          double *rdata = static_cast<double *>(data);

          size_t index = 0;
          for (size_t i = 0; i < num_to_get; i++) {
            x.push_back(rdata[index++]);
            if (spatialDimension > 1) {
              y.push_back(rdata[index++]);
            }
            if (spatialDimension == 3) {
              z.push_back(rdata[index++]);
            }
          }
          int ierr = ex_put_coord(get_file_pointer(), x.data(), y.data(), z.data());
          if (ierr < 0) {
            Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
          }
        }
        else if (field.get_name() == "ids") {
          // The ids coming in are the global ids; their position is the
          // local id -1 (That is, data[0] contains the global id of local
          // node 1)
          handle_node_ids(data, num_to_get);
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
        write_nodal_transient_field(EX_NODE_BLOCK, field, nb, num_to_get, data);
      }
      else if (role == Ioss::Field::REDUCTION) {
        store_reduction_field(EX_NODE_BLOCK, field, nb, data);
      }
      else if (role == Ioss::Field::ATTRIBUTE) {
        num_to_get = write_attribute_field(EX_NODE_BLOCK, field, nb, data);
      }
    }
    return num_to_get;
  }
}

int64_t DatabaseIO::put_field_internal(const Ioss::Blob *blob, const Ioss::Field &field, void *data,
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
        write_entity_transient_field(EX_BLOB, field, blob, num_to_get, data);
      }
      else if (role == Ioss::Field::REDUCTION) {
        store_reduction_field(EX_BLOB, field, blob, data);
      }
      else if (role == Ioss::Field::ATTRIBUTE) {
        num_to_get = write_attribute_field(EX_BLOB, field, blob, data);
      }
    }
    return num_to_get;
  }
}

int64_t DatabaseIO::put_field_internal(const Ioss::Assembly *assembly, const Ioss::Field &field,
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
        write_entity_transient_field(EX_ASSEMBLY, field, assembly, num_to_get, data);
      }
      else if (role == Ioss::Field::REDUCTION) {
        store_reduction_field(EX_ASSEMBLY, field, assembly, data);
      }
      else if (role == Ioss::Field::ATTRIBUTE) {
        num_to_get = write_attribute_field(EX_ASSEMBLY, field, assembly, data);
      }
    }
    return num_to_get;
  }
}

int64_t DatabaseIO::put_field_internal(const Ioss::ElementBlock *eb, const Ioss::Field &field,
                                       void *data, size_t data_size) const
{
  {
    Ioss::SerializeIO serializeIO__(this);

    size_t num_to_get = field.verify(data_size);

    if (num_to_get > 0) {
      int ierr = 0;

      // Get the element block id and element count
      int64_t               id               = Ioex::get_id(eb, EX_ELEM_BLOCK, &ids_);
      size_t                my_element_count = eb->entity_count();
      Ioss::Field::RoleType role             = field.get_role();

      if (role == Ioss::Field::MESH) {
        // Handle the MESH fields required for an Exodus file model.
        // (The 'genesis' portion)
        if (field.get_name() == "connectivity") {
          if (my_element_count > 0) {
            // Map element connectivity from global node id to local node id.
            int element_nodes = eb->get_property("topology_node_count").get_int();
            nodeMap.reverse_map_data(data, field, num_to_get * element_nodes);
            ierr = ex_put_conn(get_file_pointer(), EX_ELEM_BLOCK, id, data, nullptr, nullptr);
            if (ierr < 0) {
              Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
            }
          }
        }
        else if (field.get_name() == "connectivity_edge") {
          if (my_element_count > 0) {
            // Map element connectivity from global edge id to local edge id.
            int element_edges = field.transformed_storage()->component_count();
            edgeMap.reverse_map_data(data, field, num_to_get * element_edges);
            ierr = ex_put_conn(get_file_pointer(), EX_ELEM_BLOCK, id, nullptr, data, nullptr);
            if (ierr < 0) {
              Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
            }
          }
        }
        else if (field.get_name() == "connectivity_face") {
          if (my_element_count > 0) {
            // Map element connectivity from global face id to local face id.
            int element_faces = field.transformed_storage()->component_count();
            faceMap.reverse_map_data(data, field, num_to_get * element_faces);
            ierr = ex_put_conn(get_file_pointer(), EX_ELEM_BLOCK, id, nullptr, nullptr, data);
            if (ierr < 0) {
              Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
            }
          }
        }
        else if (field.get_name() == "connectivity_raw") {
          if (my_element_count > 0) {
            // Element connectivity is already in local node id.
            ierr = ex_put_conn(get_file_pointer(), EX_ELEM_BLOCK, id, data, nullptr, nullptr);
            if (ierr < 0) {
              Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
            }
          }
        }
        else if (field.get_name() == "ids") {
          handle_element_ids(eb, data, num_to_get);
        }
        else if (field.get_name() == "implicit_ids") {
          // Do nothing, input only field.
        }
        else if (field.get_name() == "skin") {
          // This is (currently) for the skinned body. It maps the
          // side element on the skin to the original element/local
          // side number.  It is a two component field, the first
          // component is the global id of the underlying element in
          // the initial mesh and its local side number (1-based).

          // FIX: Hardwired map ids....
          int map_count = ex_inquire_int(get_file_pointer(), EX_INQ_ELEM_MAP);
          if (map_count == 0) {
            // This needs to be fixed... Currently hardwired....
            ierr = ex_put_map_param(get_file_pointer(), 0, 2);
            if (ierr < 0) {
              Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
            }
          }

          std::vector<char> element(my_element_count * int_byte_size_api());
          std::vector<char> side(my_element_count * int_byte_size_api());

          if (int_byte_size_api() == 4) {
            int *el_side   = reinterpret_cast<int *>(data);
            int *element32 = reinterpret_cast<int *>(element.data());
            int *side32    = reinterpret_cast<int *>(side.data());

            int index = 0;
            for (size_t i = 0; i < my_element_count; i++) {
              element32[i] = el_side[index++];
              side32[i]    = el_side[index++];
            }
          }
          else {
            int64_t *el_side   = reinterpret_cast<int64_t *>(data);
            int64_t *element64 = reinterpret_cast<int64_t *>(element.data());
            int64_t *side64    = reinterpret_cast<int64_t *>(side.data());

            int64_t index = 0;
            for (size_t i = 0; i < my_element_count; i++) {
              element64[i] = el_side[index++];
              side64[i]    = el_side[index++];
            }
          }

          size_t eb_offset = eb->get_offset();
          ierr = ex_put_partial_num_map(get_file_pointer(), EX_ELEM_MAP, 1, eb_offset + 1,
                                        my_element_count, element.data());
          if (ierr < 0) {
            Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
          }

          ierr = ex_put_partial_num_map(get_file_pointer(), EX_ELEM_MAP, 2, eb_offset + 1,
                                        my_element_count, side.data());
          if (ierr < 0) {
            Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
          }

          if (map_count == 0) {
            // NOTE: ex_put_*num_map must be called prior to defining the name...
            ierr = ex_put_name(get_file_pointer(), EX_ELEM_MAP, 1, "skin:parent_element_id");
            if (ierr < 0) {
              Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
            }

            ierr =
                ex_put_name(get_file_pointer(), EX_ELEM_MAP, 2, "skin:parent_element_side_number");
            if (ierr < 0) {
              Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
            }
          }
        }
        else {
          num_to_get = Ioss::Utils::field_warning(eb, field, "mesh output");
        }
      }
      else if (role == Ioss::Field::ATTRIBUTE) {
        num_to_get = write_attribute_field(EX_ELEM_BLOCK, field, eb, data);
      }
      else if (role == Ioss::Field::TRANSIENT) {
        // Check if the specified field exists on this element block.
        // Note that 'higher-order' storage types (e.g. SYM_TENSOR)
        // exist on the database as scalars with the appropriate
        // extensions.

        // Transfer each component of the variable into 'data' and then
        // output.  Need temporary storage area of size 'number of
        // elements in this block.
        write_entity_transient_field(EX_ELEM_BLOCK, field, eb, my_element_count, data);
      }
      else if (role == Ioss::Field::REDUCTION) {
        store_reduction_field(EX_ELEM_BLOCK, field, eb, data);
      }
    }
    return num_to_get;
  }
}

int64_t DatabaseIO::put_field_internal(const Ioss::FaceBlock *eb, const Ioss::Field &field,
                                       void *data, size_t data_size) const
{
  {
    Ioss::SerializeIO serializeIO__(this);

    size_t num_to_get = field.verify(data_size);

    if (num_to_get > 0) {
      int ierr = 0;

      // Get the face block id and face count
      int64_t               id            = Ioex::get_id(eb, EX_FACE_BLOCK, &ids_);
      int64_t               my_face_count = eb->entity_count();
      Ioss::Field::RoleType role          = field.get_role();

      if (role == Ioss::Field::MESH) {
        // Handle the MESH fields required for an Exodus file model.
        // (The 'genesis' portion)
        if (field.get_name() == "connectivity") {
          if (my_face_count > 0) {
            // Map face connectivity from global node id to local node id.
            // Do it in 'data' ...
            int face_nodes = eb->get_property("topology_node_count").get_int();
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
            int face_edges = field.transformed_storage()->component_count();
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
        num_to_get = write_attribute_field(EX_FACE_BLOCK, field, eb, data);
      }
      else if (role == Ioss::Field::TRANSIENT) {
        // Check if the specified field exists on this face block.
        // Note that 'higher-order' storage types (e.g. SYM_TENSOR)
        // exist on the database as scalars with the appropriate
        // extensions.

        // Transfer each component of the variable into 'data' and then
        // output.  Need temporary storage area of size 'number of
        // faces in this block.
        write_entity_transient_field(EX_FACE_BLOCK, field, eb, my_face_count, data);
      }
      else if (role == Ioss::Field::REDUCTION) {
        store_reduction_field(EX_FACE_BLOCK, field, eb, data);
      }
    }
    return num_to_get;
  }
}

int64_t DatabaseIO::put_field_internal(const Ioss::EdgeBlock *eb, const Ioss::Field &field,
                                       void *data, size_t data_size) const
{
  {
    Ioss::SerializeIO serializeIO__(this);

    size_t num_to_get = field.verify(data_size);

    if (num_to_get > 0) {
      int ierr = 0;

      // Get the edge block id and edge count
      int64_t               id            = Ioex::get_id(eb, EX_EDGE_BLOCK, &ids_);
      int64_t               my_edge_count = eb->entity_count();
      Ioss::Field::RoleType role          = field.get_role();

      if (role == Ioss::Field::MESH) {
        // Handle the MESH fields required for an Exodus file model. (The 'genesis' portion)
        if (field.get_name() == "connectivity") {
          if (my_edge_count > 0) {
            // Map edge connectivity from global node id to local node id.
            // Do it in 'data' ...
            int edge_nodes = eb->get_property("topology_node_count").get_int();
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
        num_to_get = write_attribute_field(EX_EDGE_BLOCK, field, eb, data);
      }
      else if (role == Ioss::Field::TRANSIENT) {
        // Check if the specified field exists on this edge block.
        // Note that 'higher-order' storage types (e.g. SYM_TENSOR)
        // exist on the database as scalars with the appropriate
        // extensions.

        // Transfer each component of the variable into 'data' and then
        // output.  Need temporary storage area of size 'number of
        // edges in this block.
        write_entity_transient_field(EX_EDGE_BLOCK, field, eb, my_edge_count, data);
      }
      else if (role == Ioss::Field::REDUCTION) {
        store_reduction_field(EX_EDGE_BLOCK, field, eb, data);
      }
    }
    return num_to_get;
  }
}

int64_t DatabaseIO::handle_node_ids(void *ids, int64_t num_to_get) const
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
  assert(num_to_get == nodeCount);

  nodeMap.set_size(nodeCount);

  bool in_define = (dbState == Ioss::STATE_MODEL) || (dbState == Ioss::STATE_DEFINE_MODEL);
  if (int_byte_size_api() == 4) {
    nodeMap.set_map(static_cast<int *>(ids), num_to_get, 0, in_define);
  }
  else {
    nodeMap.set_map(static_cast<int64_t *>(ids), num_to_get, 0, in_define);
  }

  if (in_define) {
    // Only a single nodeblock and all set
    assert(get_region()->get_property("node_block_count").get_int() == 1);

    // Write to the database...
    if (ex_put_id_map(get_file_pointer(), EX_NODE_MAP, ids) < 0) {
      Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
    }
  }
  return num_to_get;
}

int64_t DatabaseIO::handle_element_ids(const Ioss::ElementBlock *eb, void *ids,
                                       size_t num_to_get) const
{
  elemMap.set_size(elementCount);
  size_t offset = eb->get_offset();
  return handle_block_ids(eb, EX_ELEM_MAP, elemMap, ids, num_to_get, offset);
}

int64_t DatabaseIO::handle_face_ids(const Ioss::FaceBlock *eb, void *ids, size_t num_to_get) const
{
  faceMap.set_size(faceCount);
  size_t offset = eb->get_offset();
  return handle_block_ids(eb, EX_FACE_MAP, faceMap, ids, num_to_get, offset);
}

int64_t DatabaseIO::handle_edge_ids(const Ioss::EdgeBlock *eb, void *ids, size_t num_to_get) const
{
  edgeMap.set_size(edgeCount);
  size_t offset = eb->get_offset();
  return handle_block_ids(eb, EX_EDGE_MAP, edgeMap, ids, num_to_get, offset);
}

void DatabaseIO::write_nodal_transient_field(ex_entity_type /* type */, const Ioss::Field &field,
                                             const Ioss::NodeBlock * /* ge */, int64_t     count,
                                             void *variables) const
{
  Ioss::Field::BasicType ioss_type = field.get_type();
  assert(ioss_type == Ioss::Field::REAL || ioss_type == Ioss::Field::INTEGER ||
         ioss_type == Ioss::Field::INT64 || ioss_type == Ioss::Field::COMPLEX);

  // Note that if the field's basic type is COMPLEX, then each component of
  // the VariableType is a complex variable consisting of a real and
  // imaginary part.  Since exodus cannot handle complex variables,
  // we have to output a (real and imaginary) X (number of
  // components) fields. For example, if V is a 3d vector of complex
  // data, the data in the 'variables' array are v_x, v.im_x, v_y,
  // v.im_y, v_z, v.im_z which need to be output in six separate
  // exodus fields.  These fields were already defined in
  // "write_results_metadata".

  const Ioss::VariableType *var_type = field.transformed_storage();
  std::vector<double>       temp(count);

  int step = get_current_state();
  step     = get_database_step(step);

  // get number of components, cycle through each component
  // and add suffix to base 'field_name'.  Look up index
  // of this name in 'm_variables[EX_NODE_BLOCK]' map
  int comp_count = var_type->component_count();
  int var_index  = 0;

  int re_im = 1;
  if (ioss_type == Ioss::Field::COMPLEX) {
    re_im = 2;
  }
  for (int complex_comp = 0; complex_comp < re_im; complex_comp++) {
    std::string field_name = field.get_name();
    if (re_im == 2) {
      field_name += complex_suffix[complex_comp];
    }

    char field_suffix_separator = get_field_separator();
    for (int i = 0; i < comp_count; i++) {
      std::string var_name = var_type->label_name(field_name, i + 1, field_suffix_separator);

      auto var_iter = m_variables[EX_NODE_BLOCK].find(var_name);
      if (var_iter == m_variables[EX_NODE_BLOCK].end()) {
        std::ostringstream errmsg;
        fmt::print(errmsg, "ERROR: Could not find nodal variable '{}'\n", var_name);
        IOSS_ERROR(errmsg);
      }

      var_index = var_iter->second;

      size_t  begin_offset = (re_im * i) + complex_comp;
      size_t  stride       = re_im * comp_count;
      int64_t num_out      = 0;

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

      if (num_out != nodeCount) {
        std::ostringstream errmsg;
        fmt::print(errmsg,
                   "ERROR: Problem outputting nodal variable '{}' with index = {} to file '{}'\n"
                   "Should have output {} values, but instead only output {} values.\n",
                   var_name, var_index, decoded_filename(), nodeCount, num_out);
        IOSS_ERROR(errmsg);
      }

      // Write the variable...
      int ierr =
          ex_put_var(get_file_pointer(), step, EX_NODE_BLOCK, var_index, 0, num_out, temp.data());
      if (ierr < 0) {
        std::ostringstream errmsg;
        fmt::print(errmsg, "Problem outputting nodal variable '{}' with index = {}\n", var_name,
                   var_index);
        Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__, errmsg.str());
      }
    }
  }
}

void DatabaseIO::write_entity_transient_field(ex_entity_type type, const Ioss::Field &field,
                                              const Ioss::GroupingEntity *ge, int64_t count,
                                              void *variables) const
{
  static Ioss::Map non_element_map; // Used as an empty map for ge->type() != element block.
  const Ioss::VariableType *var_type = field.transformed_storage();
  std::vector<double>       temp(count);

  int step = get_current_state();
  step     = get_database_step(step);

  Ioss::Map *map       = nullptr;
  ssize_t    eb_offset = 0;
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
  int comp_count = var_type->component_count();
  int var_index  = 0;

  // Handle quick easy, hopefully common case first...
  if (comp_count == 1 && ioss_type == Ioss::Field::REAL && type != EX_SIDE_SET &&
      !map->reorders()) {
    // Simply output the variable...
    int64_t     id       = Ioex::get_id(ge, type, &ids_);
    std::string var_name = var_type->label_name(field.get_name(), 1);
    var_index            = m_variables[type].find(var_name)->second;
    assert(var_index > 0);
    int ierr = ex_put_var(get_file_pointer(), step, type, var_index, id, count, variables);

    if (ierr < 0) {
      std::ostringstream extra_info;
      fmt::print(extra_info, "Outputting field {} at step {:n} on {} {}.", field.get_name(), step,
                 ge->type_string(), ge->name());
      Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__, extra_info.str());
    }
    return;
  }
  int re_im = 1;
  if (ioss_type == Ioss::Field::COMPLEX) {
    re_im = 2;
  }
  for (int complex_comp = 0; complex_comp < re_im; complex_comp++) {
    std::string field_name = field.get_name();
    if (re_im == 2) {
      field_name += complex_suffix[complex_comp];
    }

    char field_suffix_separator = get_field_separator();
    for (int i = 0; i < comp_count; i++) {
      std::string var_name = var_type->label_name(field_name, i + 1, field_suffix_separator);

      var_index = m_variables[type].find(var_name)->second;
      assert(var_index > 0);

      // var is a [count,comp,re_im] array;  re_im = 1(real) or 2(complex)
      // beg_offset = (re_im*i)+complex_comp
      // number_values = count
      // stride = re_im*comp_count
      ssize_t begin_offset = (re_im * i) + complex_comp;
      ssize_t stride       = re_im * comp_count;

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
      int64_t id = Ioex::get_id(ge, type, &ids_);
      int     ierr;
      if (type == EX_SIDE_SET) {
        size_t offset = ge->get_property("set_offset").get_int();
        ierr = ex_put_partial_var(get_file_pointer(), step, type, var_index, id, offset + 1, count,
                                  temp.data());
      }
      else {
        ierr = ex_put_var(get_file_pointer(), step, type, var_index, id, count, temp.data());
      }

      if (ierr < 0) {
        std::ostringstream extra_info;
        fmt::print(extra_info, "Outputting component {} of field {} at step {:n} on {} {}.", i,
                   field_name, step, ge->type_string(), ge->name());
        Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__, extra_info.str());
      }
    }
  }
}

int64_t DatabaseIO::put_Xset_field_internal(ex_entity_type type, const Ioss::EntitySet *ns,
                                            const Ioss::Field &field, void *data,
                                            size_t data_size) const
{
  {
    Ioss::SerializeIO serializeIO__(this);
    //    ex_update(get_file_pointer());

    size_t entity_count = ns->entity_count();
    size_t num_to_get   = field.verify(data_size);
    if (num_to_get > 0) {

      int64_t               id   = Ioex::get_id(ns, type, &ids_);
      Ioss::Field::RoleType role = field.get_role();

      if (role == Ioss::Field::MESH) {

        if (field.get_name() == "ids" || field.get_name() == "ids_raw") {
          // Map node id from global node id to local node id.
          // Do it in 'data' ...

          if (field.get_name() == "ids") {
            nodeMap.reverse_map_data(data, field, num_to_get);
          }
          int ierr = ex_put_set(get_file_pointer(), type, id, data, nullptr);
          if (ierr < 0) {
            Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
          }
        }
        else if (field.get_name() == "orientation") {
          int ierr = ex_put_set(get_file_pointer(), type, id, nullptr, data);
          if (ierr < 0) {
            Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
          }
        }
        else if (field.get_name() == "distribution_factors") {
          int ierr =
              ex_put_set_dist_fact(get_file_pointer(), type, id, static_cast<double *>(data));
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
        write_entity_transient_field(type, field, ns, entity_count, data);
      }
      else if (role == Ioss::Field::ATTRIBUTE) {
        num_to_get = write_attribute_field(type, field, ns, data);
      }
      else if (role == Ioss::Field::REDUCTION) {
        store_reduction_field(type, field, ns, data);
      }
    }
    return num_to_get;
  }
}

int64_t DatabaseIO::put_field_internal(const Ioss::NodeSet *ns, const Ioss::Field &field,
                                       void *data, size_t data_size) const
{
  return put_Xset_field_internal(EX_NODE_SET, ns, field, data, data_size);
}

int64_t DatabaseIO::put_field_internal(const Ioss::EdgeSet *ns, const Ioss::Field &field,
                                       void *data, size_t data_size) const
{
  return put_Xset_field_internal(EX_EDGE_SET, ns, field, data, data_size);
}

int64_t DatabaseIO::put_field_internal(const Ioss::FaceSet *ns, const Ioss::Field &field,
                                       void *data, size_t data_size) const
{
  return put_Xset_field_internal(EX_FACE_SET, ns, field, data, data_size);
}

int64_t DatabaseIO::put_field_internal(const Ioss::ElementSet *ns, const Ioss::Field &field,
                                       void *data, size_t data_size) const
{
  return put_Xset_field_internal(EX_ELEM_SET, ns, field, data, data_size);
}

int64_t DatabaseIO::put_field_internal(const Ioss::CommSet *cs, const Ioss::Field &field,
                                       void *data, size_t data_size) const
{
  size_t num_to_get   = field.verify(data_size);
  size_t entity_count = cs->entity_count();

  assert(num_to_get == entity_count);
  if (num_to_get == 0) {
    return 0;
  }

  // Return the <entity (node or side), processor> pair
  if (field.get_name() == "entity_processor") {

    // Check type -- node or side
    std::string type = cs->get_property("entity_type").get_string();

    // Allocate temporary storage space
    std::vector<char> entities(entity_count * int_byte_size_api());
    std::vector<char> procs(entity_count * int_byte_size_api());

    if (type == "node") {
      Ioss::SerializeIO serializeIO__(this);
      // Convert global node id to local node id and store in 'entities'
      if (int_byte_size_api() == 4) {
        int *entity_proc = static_cast<int *>(data);
        int *ent         = reinterpret_cast<int *>(&entities[0]);
        int *pro         = reinterpret_cast<int *>(&procs[0]);
        int  j           = 0;
        for (size_t i = 0; i < entity_count; i++) {
          int global_id = entity_proc[j++];
          ent[i]        = nodeMap.global_to_local(global_id, true);
          pro[i]        = entity_proc[j++];
        }
      }
      else {
        int64_t *entity_proc = static_cast<int64_t *>(data);
        int64_t *ent         = reinterpret_cast<int64_t *>(&entities[0]);
        int64_t *pro         = reinterpret_cast<int64_t *>(&procs[0]);
        int64_t  j           = 0;
        for (size_t i = 0; i < entity_count; i++) {
          int64_t global_id = entity_proc[j++];
          ent[i]            = nodeMap.global_to_local(global_id, true);
          pro[i]            = entity_proc[j++];
        }
      }

      if (commsetNodeCount > 0) {
        int ierr = ex_put_node_cmap(get_file_pointer(),
                                    Ioex::get_id(cs, static_cast<ex_entity_type>(0), &ids_),
                                    entities.data(), procs.data(), myProcessor);
        if (ierr < 0) {
          Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
        }
      }

      if (commsetNodeCount == 1) {
        // NOTE: The internal and border node maps must be output in one call.
        //       In this routine, we only have one commset at a time and can't
        //       construct the entire map at one time.  This is not really needed,
        //       so for now we just skip if there is more than one commset.  If
        //       this information is really needed, need to cache the information
        //       until all commsets have been processed.  Also need to change
        //       write_communication_metada() [Maybe, unless client sets correct
        //       properties.]

        // Construct the node map (internal vs. border).
        // Border nodes are those in the communication map (use entities array)
        // Internal nodes are the rest.  Allocate array to hold all nodes,
        // initialize all to '1', then zero out the nodes in 'entities'.
        // Iterate through array again and consolidate all '1's

        std::vector<char> internal(nodeCount * int_byte_size_api());
        if (int_byte_size_api() == 4) {
          compute_internal_border_maps(reinterpret_cast<int *>(&entities[0]),
                                       reinterpret_cast<int *>(&internal[0]), nodeCount,
                                       entity_count);
        }
        else {
          compute_internal_border_maps(reinterpret_cast<int64_t *>(&entities[0]),
                                       reinterpret_cast<int64_t *>(&internal[0]), nodeCount,
                                       entity_count);
        }

        int ierr = ex_put_processor_node_maps(get_file_pointer(), internal.data(), entities.data(),
                                              nullptr, myProcessor);
        if (ierr < 0) {
          Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
        }
      }
    }
    else if (type == "side") {
      Ioss::SerializeIO serializeIO__(this);
      std::vector<char> sides(entity_count * int_byte_size_api());
      if (int_byte_size_api() == 4) {
        int *entity_proc = static_cast<int *>(data);
        int *ent         = reinterpret_cast<int *>(&entities[0]);
        int *sid         = reinterpret_cast<int *>(&sides[0]);
        int *pro         = reinterpret_cast<int *>(&procs[0]);
        int  j           = 0;
        for (size_t i = 0; i < entity_count; i++) {
          ent[i] = elemMap.global_to_local(entity_proc[j++]);
          sid[i] = entity_proc[j++];
          pro[i] = entity_proc[j++];
        }
      }
      else {
        int64_t *entity_proc = static_cast<int64_t *>(data);
        int64_t *ent         = reinterpret_cast<int64_t *>(&entities[0]);
        int64_t *sid         = reinterpret_cast<int64_t *>(&sides[0]);
        int64_t *pro         = reinterpret_cast<int64_t *>(&procs[0]);
        int64_t  j           = 0;
        for (size_t i = 0; i < entity_count; i++) {
          ent[i] = elemMap.global_to_local(entity_proc[j++]);
          sid[i] = entity_proc[j++];
          pro[i] = entity_proc[j++];
        }
      }

      int ierr = ex_put_elem_cmap(get_file_pointer(),
                                  Ioex::get_id(cs, static_cast<ex_entity_type>(0), &ids_),
                                  entities.data(), sides.data(), procs.data(), myProcessor);
      if (ierr < 0) {
        Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
      }

      // Construct the element map (internal vs. border).
      // Border elements are those in the communication map (use entities array)
      // Internal elements are the rest.  Allocate array to hold all elements,
      // initialize all to '1', then zero out the elements in 'entities'.
      // Iterate through array again and consolidate all '1's
      std::vector<char> internal(elementCount * int_byte_size_api());
      if (int_byte_size_api() == 4) {
        compute_internal_border_maps(reinterpret_cast<int *>(&entities[0]),
                                     reinterpret_cast<int *>(&internal[0]), elementCount,
                                     entity_count);
      }
      else {
        compute_internal_border_maps(reinterpret_cast<int64_t *>(&entities[0]),
                                     reinterpret_cast<int64_t *>(&internal[0]), elementCount,
                                     entity_count);
      }

      ierr = ex_put_processor_elem_maps(get_file_pointer(), internal.data(), entities.data(),
                                        myProcessor);
      if (ierr < 0) {
        Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
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
    num_to_get = Ioss::Utils::field_warning(cs, field, "output");
  }
  return num_to_get;
}

int64_t DatabaseIO::put_field_internal(const Ioss::SideSet *fs, const Ioss::Field &field,
                                       void * /* data */, size_t                   data_size) const
{
  size_t num_to_get = field.verify(data_size);
  if (field.get_name() == "ids") {
    // Do nothing, just handles an idiosyncrasy of the GroupingEntity
  }
  else {
    num_to_get = Ioss::Utils::field_warning(fs, field, "output");
  }
  return num_to_get;
}

int64_t DatabaseIO::put_field_internal(const Ioss::SideBlock *fb, const Ioss::Field &field,
                                       void *data, size_t data_size) const
{
  Ioss::SerializeIO serializeIO__(this);
  size_t            num_to_get = field.verify(data_size);
  if (num_to_get > 0) {

    int64_t id = Ioex::get_id(fb, EX_SIDE_SET, &ids_);

    size_t entity_count = fb->entity_count();
    size_t offset       = fb->get_property("set_offset").get_int();

    Ioss::Field::RoleType role = field.get_role();

    if (role == Ioss::Field::MESH) {
      if (field.get_name() == "side_ids" && fb->name() == "universal_sideset") {
        // The side ids are being stored as the distribution factor
        // field on the universal sideset.  There should be no other
        // side sets that request this field...  (Eventually,
        // create an id field to store this info.

        // Need to convert 'ints' to 'double' for storage on mesh...
        // FIX 64
        if (field.get_type() == Ioss::Field::INTEGER) {
          int *               ids = static_cast<int *>(data);
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
          int64_t *           ids = static_cast<int64_t *>(data);
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
        size_t df_offset = fb->get_property("set_df_offset").get_int();
        size_t df_count  = fb->get_property("distribution_factor_count").get_int();
        ierr = ex_put_partial_set_dist_fact(get_file_pointer(), EX_SIDE_SET, id, df_offset + 1,
                                            df_count, static_cast<double *>(data));
        if (ierr < 0) {
          Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
        }
      }
      else if (field.get_name() == "element_side") {
        // In exodus, the 'side block' is stored as a sideset.  A
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
        size_t side_offset = Ioss::Utils::get_side_offset(fb);

        size_t index = 0;

        if (field.get_type() == Ioss::Field::INTEGER) {
          Ioss::IntVector element(num_to_get);
          Ioss::IntVector side(num_to_get);
          int *           el_side = reinterpret_cast<int *>(data);

          for (size_t i = 0; i < num_to_get; i++) {
            element[i] = elemMap.global_to_local(el_side[index++]);
            side[i]    = el_side[index++] + side_offset;
          }

          int ierr = ex_put_partial_set(get_file_pointer(), EX_SIDE_SET, id, offset + 1,
                                        entity_count, element.data(), side.data());
          if (ierr < 0) {
            Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
          }
        }
        else {
          Ioss::Int64Vector element(num_to_get);
          Ioss::Int64Vector side(num_to_get);
          int64_t *         el_side = reinterpret_cast<int64_t *>(data);

          for (size_t i = 0; i < num_to_get; i++) {
            element[i] = elemMap.global_to_local(el_side[index++]);
            side[i]    = el_side[index++] + side_offset;
          }

          int ierr = ex_put_partial_set(get_file_pointer(), EX_SIDE_SET, id, offset + 1,
                                        entity_count, element.data(), side.data());
          if (ierr < 0) {
            Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
          }
        }
      }
      else if (field.get_name() == "element_side_raw") {
        // In exodus, the 'side block' is stored as a sideset.  A
        // sideset has a list of elements and a corresponding local
        // element side (1-based)

        // The 'data' passed into the function is stored as a
        // 2D vector e0,f0,e1,f1,... (e=element, f=side)

        // To avoid overwriting the passed in data, we allocate
        // two arrays to store the data for this sideset.

        // The element_id passed in is the local id.

        // See if edges or faces...
        size_t side_offset = Ioss::Utils::get_side_offset(fb);

        size_t index = 0;
        if (field.get_type() == Ioss::Field::INTEGER) {
          Ioss::IntVector element(num_to_get);
          Ioss::IntVector side(num_to_get);
          int *           el_side = reinterpret_cast<int *>(data);

          for (size_t i = 0; i < num_to_get; i++) {
            element[i] = el_side[index++];
            side[i]    = el_side[index++] + side_offset;
          }

          int ierr = ex_put_partial_set(get_file_pointer(), EX_SIDE_SET, id, offset + 1,
                                        entity_count, element.data(), side.data());
          if (ierr < 0) {
            Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
          }
        }
        else {
          Ioss::Int64Vector element(num_to_get);
          Ioss::Int64Vector side(num_to_get);
          int64_t *         el_side = reinterpret_cast<int64_t *>(data);

          for (size_t i = 0; i < num_to_get; i++) {
            element[i] = el_side[index++];
            side[i]    = el_side[index++] + side_offset;
          }

          int ierr = ex_put_partial_set(get_file_pointer(), EX_SIDE_SET, id, offset + 1,
                                        entity_count, element.data(), side.data());
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
        num_to_get = Ioss::Utils::field_warning(fb, field, "output");
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
      write_entity_transient_field(EX_SIDE_SET, field, fb, entity_count, data);
    }
    else if (role == Ioss::Field::ATTRIBUTE) {
      num_to_get = write_attribute_field(EX_SIDE_SET, field, fb, data);
    }
    else if (role == Ioss::Field::REDUCTION) {
      store_reduction_field(EX_SIDE_SET, field, fb, data);
    }
  }
  return num_to_get;
}

void DatabaseIO::write_meta_data()
{
  Ioss::Region *region = get_region();

  const Ioss::NodeBlockContainer &node_blocks = region->get_node_blocks();
  assert(node_blocks.size() == 1);
  nodeCount        = node_blocks[0]->entity_count();
  spatialDimension = node_blocks[0]->get_property("component_degree").get_int();

  char the_title[max_line_length + 1];

  // Title...
  if (region->property_exists("title")) {
    std::string title_str = region->get_property("title").get_string();
    Ioss::Utils::copy_string(the_title, title_str);
  }
  else {
    Ioss::Utils::copy_string(the_title, "IOSS Default Output Title");
  }

  Ioex::get_id(node_blocks[0], EX_NODE_BLOCK, &ids_);

  // Assemblies --
  {
    const auto &assemblies = region->get_assemblies();
    // Set ids of all entities that have "id" property...
    for (auto &assem : assemblies) {
      Ioex::set_id(assem, EX_ASSEMBLY, &ids_);
    }

    for (auto &assem : assemblies) {
      Ioex::get_id(assem, EX_ASSEMBLY, &ids_);
    }
    m_groupCount[EX_ASSEMBLY] = assemblies.size();
  }

  // Blobs --
  {
    const auto &blobs = region->get_blobs();
    // Set ids of all entities that have "id" property...
    for (auto &blob : blobs) {
      Ioex::set_id(blob, EX_BLOB, &ids_);
    }

    for (auto &blob : blobs) {
      Ioex::get_id(blob, EX_BLOB, &ids_);
    }
    m_groupCount[EX_BLOB] = blobs.size();
  }

  // Edge Blocks --
  {
    const Ioss::EdgeBlockContainer &edge_blocks = region->get_edge_blocks();
    assert(Ioss::Utils::check_block_order(edge_blocks));
    // Set ids of all entities that have "id" property...
    for (auto &edge_block : edge_blocks) {
      Ioex::set_id(edge_block, EX_EDGE_BLOCK, &ids_);
    }

    edgeCount = 0;
    for (auto &edge_block : edge_blocks) {
      edgeCount += edge_block->entity_count();
      // Set ids of all entities that do not have "id" property...
      Ioex::get_id(edge_block, EX_EDGE_BLOCK, &ids_);
    }
    m_groupCount[EX_EDGE_BLOCK] = edge_blocks.size();
  }

  // Face Blocks --
  {
    const Ioss::FaceBlockContainer &face_blocks = region->get_face_blocks();
    assert(Ioss::Utils::check_block_order(face_blocks));
    // Set ids of all entities that have "id" property...
    for (auto &face_block : face_blocks) {
      Ioex::set_id(face_block, EX_FACE_BLOCK, &ids_);
    }

    faceCount = 0;
    for (auto &face_block : face_blocks) {
      faceCount += face_block->entity_count();
      // Set ids of all entities that do not have "id" property...
      Ioex::get_id(face_block, EX_FACE_BLOCK, &ids_);
    }
    m_groupCount[EX_FACE_BLOCK] = face_blocks.size();
  }

  // Element Blocks --
  {
    const Ioss::ElementBlockContainer &element_blocks = region->get_element_blocks();
    assert(Ioss::Utils::check_block_order(element_blocks));
    // Set ids of all entities that have "id" property...
    for (auto &element_block : element_blocks) {
      Ioex::set_id(element_block, EX_ELEM_BLOCK, &ids_);
    }

    elementCount = 0;
    for (auto &element_block : element_blocks) {
      elementCount += element_block->entity_count();
      // Set ids of all entities that do not have "id" property...
      Ioex::get_id(element_block, EX_ELEM_BLOCK, &ids_);
    }
    m_groupCount[EX_ELEM_BLOCK] = element_blocks.size();
  }

  // NodeSets ...
  {
    const Ioss::NodeSetContainer &nodesets = region->get_nodesets();
    for (auto &nodeset : nodesets) {
      Ioex::set_id(nodeset, EX_NODE_SET, &ids_);
    }

    for (auto &nodeset : nodesets) {
      Ioex::get_id(nodeset, EX_NODE_SET, &ids_);
    }
    m_groupCount[EX_NODE_SET] = nodesets.size();
  }

  // EdgeSets ...
  {
    const Ioss::EdgeSetContainer &edgesets = region->get_edgesets();
    for (auto &edgeset : edgesets) {
      Ioex::set_id(edgeset, EX_EDGE_SET, &ids_);
    }

    for (auto &edgeset : edgesets) {
      Ioex::get_id(edgeset, EX_EDGE_SET, &ids_);
    }
    m_groupCount[EX_EDGE_SET] = edgesets.size();
  }

  // FaceSets ...
  {
    const Ioss::FaceSetContainer &facesets = region->get_facesets();
    for (auto &faceset : facesets) {
      Ioex::set_id(faceset, EX_FACE_SET, &ids_);
    }

    for (auto &faceset : facesets) {
      Ioex::get_id(faceset, EX_FACE_SET, &ids_);
    }
    m_groupCount[EX_FACE_SET] = facesets.size();
  }

  // ElementSets ...
  {
    const Ioss::ElementSetContainer &elementsets = region->get_elementsets();
    for (auto &elementset : elementsets) {
      Ioex::set_id(elementset, EX_ELEM_SET, &ids_);
    }

    for (auto &elementset : elementsets) {
      Ioex::get_id(elementset, EX_ELEM_SET, &ids_);
    }
    m_groupCount[EX_ELEM_SET] = elementsets.size();
  }

  // SideSets ...
  const Ioss::SideSetContainer &ssets = region->get_sidesets();
  for (auto &sset : ssets) {
    Ioex::set_id(sset, EX_SIDE_SET, &ids_);
  }

  // Get entity counts for all face sets... Create SideSets.
  for (auto &sset : ssets) {
    Ioex::get_id(sset, EX_SIDE_SET, &ids_);
    int64_t id           = sset->get_property("id").get_int();
    int64_t entity_count = 0;
    int64_t df_count     = 0;

    const Ioss::SideBlockContainer &side_blocks = sset->get_side_blocks();
    for (auto &side_block : side_blocks) {
      // Add  "*_offset" properties to specify at what offset
      // the data for this block appears in the containing set.
      Ioss::SideBlock *new_block = const_cast<Ioss::SideBlock *>(side_block);
      new_block->property_add(Ioss::Property("set_offset", entity_count));
      new_block->property_add(Ioss::Property("set_df_offset", df_count));

      // If combining sideblocks into sidesets on output, then
      // the id of the sideblock must be the same as the sideset
      // id.
      new_block->property_update("id", id);
      new_block->property_update("guid", util().generate_guid(1));

      entity_count += side_block->entity_count();
      df_count += side_block->get_property("distribution_factor_count").get_int();
    }
    Ioss::SideSet *new_entity = const_cast<Ioss::SideSet *>(sset);
    new_entity->property_add(Ioss::Property("entity_count", entity_count));
    new_entity->property_add(Ioss::Property("distribution_factor_count", df_count));
  }

  m_groupCount[EX_SIDE_SET] = ssets.size();

  {
    Ioss::SerializeIO serializeIO__(this);
    if (!properties.exists("OMIT_QA_RECORDS")) {
      put_qa();
    }
    if (!properties.exists("OMIT_INFO_RECORDS")) {
      put_info();
    }

    bool       file_per_processor = true;
    Ioex::Mesh mesh(spatialDimension, the_title, file_per_processor);
    mesh.populate(region);
    gather_communication_metadata(&mesh.comm);

    // Write the metadata to the exodus file...
    Ioex::Internals data(get_file_pointer(), maximumNameLength, util());
    int             ierr = data.write_meta_data(mesh);

    if (ierr < 0) {
      Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
    }

    output_other_meta_data();
  }
}

void DatabaseIO::gather_communication_metadata(Ioex::CommunicationMetaData *meta)
{
  // It's possible that we are a serial program outputting information
  // for later use by a parallel program.

  meta->processorCount = 0;
  meta->processorId    = 0;
  meta->outputNemesis  = false;

  if (isParallel) {
    meta->processorCount = util().parallel_size();
    meta->processorId    = myProcessor;
    meta->outputNemesis  = true;
  }
  else {
    if (properties.exists("processor_count")) {
      meta->processorCount = properties.get("processor_count").get_int();
    }
    else if (get_region()->property_exists("processor_count")) {
      meta->processorCount = get_region()->get_property("processor_count").get_int();
    }

    if (properties.exists("my_processor")) {
      meta->processorId = properties.get("my_processor").get_int();
    }
    else if (get_region()->property_exists("my_processor")) {
      meta->processorId = get_region()->get_property("my_processor").get_int();
    }

    if (!get_region()->get_commsets().empty()) {
      isSerialParallel    = true;
      meta->outputNemesis = true;
    }
  }

  if (isSerialParallel || meta->processorCount > 0) {
    meta->globalNodes    = 1; // Just need a nonzero value.
    meta->globalElements = 1; // Just need a nonzero value.

    if (get_region()->property_exists("global_node_count")) {
      meta->globalNodes = get_region()->get_property("global_node_count").get_int();
    }

    if (get_region()->property_exists("global_element_count")) {
      meta->globalElements = get_region()->get_property("global_element_count").get_int();
    }

    if (get_region()->property_exists("global_element_block_count")) {
      meta->globalElementBlocks =
          get_region()->get_property("global_element_block_count").get_int();
    }
    else {
      meta->globalElementBlocks = get_region()->get_element_blocks().size();
    }

    if (get_region()->property_exists("global_node_set_count")) {
      meta->globalNodeSets = get_region()->get_property("global_node_set_count").get_int();
    }
    else {
      meta->globalNodeSets = get_region()->get_nodesets().size();
    }

    if (get_region()->property_exists("global_side_set_count")) {
      meta->globalSideSets = get_region()->get_property("global_side_set_count").get_int();
    }
    else {
      meta->globalSideSets = get_region()->get_sidesets().size();
    }

    // ========================================================================
    // Load balance parameters (NEMESIS, p15)
    meta->nodesInternal    = nodeCount;
    meta->nodesBorder      = 0;
    meta->nodesExternal    = 0; // Shadow nodes == 0 for now
    meta->elementsInternal = elementCount;
    meta->elementsBorder   = 0;

    // Now, see if any of the above are redefined by a property...
    if (get_region()->property_exists("internal_node_count")) {
      meta->nodesInternal = get_region()->get_property("internal_node_count").get_int();
    }

    if (get_region()->property_exists("border_node_count")) {
      meta->nodesBorder = get_region()->get_property("border_node_count").get_int();
    }

    if (get_region()->property_exists("internal_element_count")) {
      meta->elementsInternal = get_region()->get_property("internal_element_count").get_int();
    }

    if (get_region()->property_exists("border_element_count")) {
      meta->elementsBorder = get_region()->get_property("border_element_count").get_int();
    }

    const Ioss::CommSetContainer &comm_sets = get_region()->get_commsets();
    for (auto &cs : comm_sets) {
      std::string type  = cs->get_property("entity_type").get_string();
      size_t      count = cs->entity_count();
      int64_t     id    = Ioex::get_id(cs, static_cast<ex_entity_type>(0), &ids_);

      if (type == "node") {
        meta->nodeMap.emplace_back(id, count, 'n');
      }
      else if (type == "side") {
        meta->elementMap.emplace_back(id, count, 'e');
      }
      else {
        std::ostringstream errmsg;
        fmt::print(errmsg, "Internal Program Error...");
        IOSS_ERROR(errmsg);
      }
    }
  }
  commsetNodeCount = meta->nodeMap.size();
  commsetElemCount = meta->elementMap.size();
}
} // namespace Ioex
