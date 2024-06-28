// Copyright(C) 1999-2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include <cassert>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <vtk_exodusII.h>
#include "vtk_fmt.h"
#include VTK_FMT(fmt/core.h)
#include VTK_FMT(fmt/ostream.h)
#include <map>
#include <sstream>
#include <string>
#include <tokenize.h>
#include <vector>

#include "Ioex_Utils.h"
#include "Ioss_Assembly.h"
#include "Ioss_BasisVariableType.h"
#include "Ioss_Blob.h"
#include "Ioss_CodeTypes.h"
#include "Ioss_ComposedVariableType.h"
#include "Ioss_CompositeVariableType.h"
#include "Ioss_DBUsage.h"
#include "Ioss_DatabaseIO.h"
#include "Ioss_EdgeBlock.h"
#include "Ioss_EdgeSet.h"
#include "Ioss_ElementBlock.h"
#include "Ioss_ElementSet.h"
#include "Ioss_EntityBlock.h"
#include "Ioss_EntityType.h"
#include "Ioss_Enumerate.h"
#include "Ioss_FaceBlock.h"
#include "Ioss_FaceSet.h"
#include "Ioss_Field.h"
#include "Ioss_FileInfo.h"
#include "Ioss_GroupingEntity.h"
#include "Ioss_IOFactory.h"
#include "Ioss_Map.h"
#include "Ioss_MeshType.h"
#include "Ioss_NamedSuffixVariableType.h"
#include "Ioss_NodeBlock.h"
#include "Ioss_NodeSet.h"
#include "Ioss_ParallelUtils.h"
#include "Ioss_Property.h"
#include "Ioss_PropertyManager.h"
#include "Ioss_QuadratureVariableType.h"
#include "Ioss_Region.h"
#include "Ioss_SerializeIO.h"
#include "Ioss_SideBlock.h"
#include "Ioss_SideSet.h"
#include "Ioss_SmartAssert.h"
#include "Ioss_State.h"
#include "Ioss_Utils.h"

#include "exodus/Ioex_BaseDatabaseIO.h"
#include "exodus/Ioex_Internals.h"

// Transitioning from treating global variables as Ioss::Field::TRANSIENT
// to Ioss::Field::REDUCTION.  To get the old behavior, define the value
// below to '1'.
#define GLOBALS_ARE_TRANSIENT 0

// ========================================================================
// Static internal helper functions
// ========================================================================
namespace {
  bool sixty_four_bit_message_output = false;

  std::vector<ex_entity_type> exodus_types({EX_GLOBAL, EX_BLOB, EX_ASSEMBLY, EX_NODE_BLOCK,
                                            EX_EDGE_BLOCK, EX_FACE_BLOCK, EX_ELEM_BLOCK,
                                            EX_NODE_SET, EX_EDGE_SET, EX_FACE_SET, EX_ELEM_SET,
                                            EX_SIDE_SET});

  const size_t max_line_length = MAX_LINE_LENGTH;

  void check_variable_consistency(const ex_var_params &exo_params, int my_processor,
                                  const std::string &filename, const Ioss::ParallelUtils &util);

  void check_attribute_index_order(Ioss::GroupingEntity *block);

  template <typename T>
  void write_attribute_names(int exoid, ex_entity_type type, const std::vector<T *> &entities);

  class AssemblyTreeFilter
  {
  public:
    AssemblyTreeFilter()                           = delete;
    AssemblyTreeFilter(const AssemblyTreeFilter &) = delete;

    AssemblyTreeFilter(Ioss::Region *region, const Ioss::EntityType filterType,
                       const std::vector<ex_assembly> &assemblies)
        : m_region(region), m_type(filterType), m_assemblies(assemblies),
          m_visitedAssemblies(assemblies.size(), false)
    {
    }

    void update_list_from_assembly_tree(size_t assemblyIndex, Ioss::NameList &list)
    {
      // Walk the tree without cyclic dependency
      if (assemblyIndex < m_assemblies.size()) {
        if (!m_visitedAssemblies[assemblyIndex]) {
          m_visitedAssemblies[assemblyIndex] = true;

          const auto            &assembly     = m_assemblies[assemblyIndex];
          const Ioss::EntityType assemblyType = Ioex::map_exodus_type(assembly.type);
          if (m_type == assemblyType) {
            for (int j = 0; j < assembly.entity_count; j++) {
              Ioss::GroupingEntity *ge = m_region->get_entity(assembly.entity_list[j], m_type);
              if (nullptr != ge) {
                list.push_back(ge->name());
              }
            }
          }

          if (Ioss::ASSEMBLY == assemblyType) {
            for (int i = 0; i < assembly.entity_count; i++) {
              // Find the sub assembly with the same id
              int64_t subAssemblyId = assembly.entity_list[i];
              bool    found         = false;
              for (size_t j = 0; j < m_assemblies.size(); j++) {
                if (m_assemblies[j].id == subAssemblyId) {
                  found = true;
                  update_list_from_assembly_tree(j, list);
                  break;
                }
              }

              if (!found) {
                std::ostringstream errmsg;
                fmt::print(errmsg, "ERROR: Could not find sub-assembly with id: {} and name: {}",
                           assembly.id, assembly.name);
                IOSS_ERROR(errmsg);
              }
            }
          }
        }
      }
    }

    void update_assembly_filter_list(Ioss::NameList &assemblyFilterList)
    {
      for (size_t i = 0; i < m_assemblies.size(); ++i) {
        if (m_visitedAssemblies[i]) {
          assemblyFilterList.emplace_back(m_assemblies[i].name);
        }
      }

      std::sort(assemblyFilterList.begin(), assemblyFilterList.end(), std::less<std::string>());
      auto endIter = std::unique(assemblyFilterList.begin(), assemblyFilterList.end());
      assemblyFilterList.resize(endIter - assemblyFilterList.begin());
    }

  private:
    Ioss::Region                   *m_region = nullptr;
    Ioss::EntityType                m_type   = Ioss::INVALID_TYPE;
    const std::vector<ex_assembly> &m_assemblies;
    mutable std::vector<bool>       m_visitedAssemblies;
  };

  std::vector<ex_assembly> get_exodus_assemblies(int exoid)
  {
    std::vector<ex_assembly> assemblies;
    int                      nassem = ex_inquire_int(exoid, EX_INQ_ASSEMBLY);
    if (nassem > 0) {
      assemblies.resize(nassem);

      int max_name_length = ex_inquire_int(exoid, EX_INQ_DB_MAX_USED_NAME_LENGTH);
      for (auto &assembly : assemblies) {
        assembly.name = new char[max_name_length + 1];
      }

      int ierr = ex_get_assemblies(exoid, Data(assemblies));
      if (ierr < 0) {
        Ioex::exodus_error(exoid, __LINE__, __func__, __FILE__);
      }

      // Now allocate space for member list and get assemblies again...
      for (auto &assembly : assemblies) {
        assembly.entity_list = new int64_t[assembly.entity_count];
      }

      ierr = ex_get_assemblies(exoid, Data(assemblies));
      if (ierr < 0) {
        Ioex::exodus_error(exoid, __LINE__, __func__, __FILE__);
      }
    }
    return assemblies;
  }

  void cleanup_exodus_assembly_vector(std::vector<ex_assembly> &assemblies)
  {
    for (const auto &assembly : assemblies) {
      delete[] assembly.entity_list;
      delete[] assembly.name;
    }
  }

} // namespace

namespace Ioex {
  BaseDatabaseIO::BaseDatabaseIO(Ioss::Region *region, const std::string &filename,
                                 Ioss::DatabaseUsage db_usage, Ioss_MPI_Comm communicator,
                                 const Ioss::PropertyManager &props)
      : Ioss::DatabaseIO(region, filename, db_usage, communicator, props)
  {
    m_groupCount[EX_GLOBAL]     = 1; // To make some common code work more cleanly.
    m_groupCount[EX_NODE_BLOCK] = 1; // To make some common code work more cleanly.

    // A history file is only written on processor 0...
    if (db_usage == Ioss::WRITE_HISTORY) {
      isParallel = false;
    }

    timeLastFlush = time(nullptr);

    dbState = Ioss::STATE_UNKNOWN;

    // Set exodusII warning level.
    if (util().get_environment("EX_DEBUG", isParallel)) {
      fmt::print(
          Ioss::DebugOut(),
          "IOEX: Setting EX_VERBOSE|EX_DEBUG because EX_DEBUG environment variable is set.\n");
      ex_opts(EX_VERBOSE | EX_DEBUG);
    }

    if (!is_input()) {
      if (util().get_environment("EX_MODE", exodusMode, isParallel)) {
        fmt::print(
            Ioss::OUTPUT(),
            "IOEX: Exodus create mode set to {} from value of EX_MODE environment variable.\n",
            exodusMode);
      }

      if (util().get_environment("EX_MINIMIZE_OPEN_FILES", isParallel)) {
        fmt::print(Ioss::OUTPUT(),
                   "IOEX: Minimizing open files because EX_MINIMIZE_OPEN_FILES environment "
                   "variable is set.\n");
        minimizeOpenFiles = true;
      }
      else {
        Ioss::Utils::check_set_bool_property(properties, "MINIMIZE_OPEN_FILES", minimizeOpenFiles);
      }

      {
        bool file_per_state = false;
        Ioss::Utils::check_set_bool_property(properties, "FILE_PER_STATE", file_per_state);
        if (file_per_state) {
          set_file_per_state(true);
        }
      }
    }

    // See if there are any properties that need to (or can) be
    // handled prior to opening/creating database...
    bool compress = ((properties.exists("COMPRESSION_LEVEL") &&
                      properties.get("COMPRESSION_LEVEL").get_int() > 0) ||
                     (properties.exists("COMPRESSION_SHUFFLE") &&
                      properties.get("COMPRESSION_SHUFFLE").get_int() > 0));

    if (compress) {
      exodusMode |= EX_NETCDF4;
    }

    if (properties.exists("FILE_TYPE")) {
      std::string type = properties.get("FILE_TYPE").get_string();
      if (type == "netcdf3" || type == "netcdf-3") {
        exodusMode = EX_CLOBBER; // Reset back to default...
      }
      if (type == "netcdf4" || type == "netcdf-4" || type == "hdf5") {
        exodusMode |= EX_NETCDF4;
      }
      else if (type == "netcdf5" || type == "netcdf-5" || type == "cdf5") {
        exodusMode |= EX_64BIT_DATA;
      }
    }

    if (properties.exists("ENABLE_FILE_GROUPS")) {
      exodusMode |= EX_NETCDF4;
      exodusMode |= EX_NOCLASSIC;
    }

    if (properties.exists("MAXIMUM_NAME_LENGTH")) {
      maximumNameLength = properties.get("MAXIMUM_NAME_LENGTH").get_int();
    }

    if (properties.exists("REAL_SIZE_DB")) {
      int rsize = properties.get("REAL_SIZE_DB").get_int();
      if (rsize == 4) {
        dbRealWordSize = 4; // Only used for file create...
      }
    }

    if (properties.exists("INTEGER_SIZE_DB")) {
      int isize = properties.get("INTEGER_SIZE_DB").get_int();
      if (isize == 8) {
        exodusMode |= EX_ALL_INT64_DB;
      }
    }

    if (properties.exists("INTEGER_SIZE_API")) {
      int isize = properties.get("INTEGER_SIZE_API").get_int();
      if (isize == 8) {
        set_int_byte_size_api(Ioss::USE_INT64_API);
      }
    }

    if (!is_input()) {
      if (properties.exists("FLUSH_INTERVAL")) {
        int interval  = properties.get("FLUSH_INTERVAL").get_int();
        flushInterval = interval;
      }
    }

    // Don't open output files until they are actually going to be
    // written to.  This is needed for proper support of the topology
    // files and auto restart so we don't overwrite a file with data we
    // need to save...
  }

  void BaseDatabaseIO::set_int_byte_size_api(Ioss::DataSize size) const
  {
    if (m_exodusFilePtr > 0) {
      int old_status = ex_int64_status(get_file_pointer());
      if (size == 8) {
        ex_set_int64_status(get_file_pointer(), EX_ALL_INT64_API | old_status);
      }
      else {
        // Need to clear EX_ALL_INT64_API if set...
        if ((old_status & EX_ALL_INT64_API) != 0) {
          old_status &= ~EX_ALL_INT64_API;
          assert(!(old_status & EX_ALL_INT64_API));
          ex_set_int64_status(m_exodusFilePtr, old_status);
        }
      }
    }
    else {
      if (size == 8) {
        exodusMode |= EX_ALL_INT64_API;
      }
      else {
        exodusMode &= ~EX_ALL_INT64_API;
      }
    }
    dbIntSizeAPI = size; // mutable
  }

  // Returns byte size of integers stored on the database...
  int BaseDatabaseIO::int_byte_size_db() const
  {
    int status = ex_int64_status(get_file_pointer());
    if (status & EX_MAPS_INT64_DB || status & EX_IDS_INT64_DB || status & EX_BULK_INT64_DB) {
      return 8;
    }
    else {
      return 4;
    }
  }

  // common
  BaseDatabaseIO::~BaseDatabaseIO()
  {
    try {
      free_file_pointer();
    }
    catch (...) {
    }
  }

  // common
  unsigned BaseDatabaseIO::entity_field_support() const
  {
    return Ioss::NODEBLOCK | Ioss::EDGEBLOCK | Ioss::FACEBLOCK | Ioss::ELEMENTBLOCK |
           Ioss::NODESET | Ioss::EDGESET | Ioss::FACESET | Ioss::ELEMENTSET | Ioss::SIDESET |
           Ioss::SIDEBLOCK | Ioss::REGION | Ioss::SUPERELEMENT;
  }

  // common
  int BaseDatabaseIO::get_file_pointer() const
  {
    // Returns the file_pointer used to access the file on disk.
    // Checks that the file is open and if not, opens it first.
    if (m_exodusFilePtr < 0) {
      bool write_message  = true;
      bool abort_if_error = true;
      if (is_input()) {
        open_input_file(write_message, nullptr, nullptr, abort_if_error);
      }
      else {
        bool overwrite = true;
        handle_output_file(write_message, nullptr, nullptr, overwrite, abort_if_error);
      }

      if (!m_groupName.empty()) {
        ex_get_group_id(m_exodusFilePtr, m_groupName.c_str(), &m_exodusFilePtr);
      }
    }
    assert(m_exodusFilePtr >= 0);
    fileExists = true;
    return m_exodusFilePtr;
  }

  int BaseDatabaseIO::free_file_pointer() const
  {
    if (m_exodusFilePtr != -1) {
      bool do_timer = false;
      if (isParallel) {
        Ioss::Utils::check_set_bool_property(properties, "IOSS_TIME_FILE_OPEN_CLOSE", do_timer);
      }
      double t_begin = (do_timer ? Ioss::Utils::timer() : 0);

      ex_close(m_exodusFilePtr);
      close_dw();
      if (do_timer && isParallel) {
        double t_end    = Ioss::Utils::timer();
        double duration = util().global_minmax(t_end - t_begin, Ioss::ParallelUtils::DO_MAX);
        if (myProcessor == 0) {
          fmt::print(Ioss::DebugOut(), "File Close Time = {} ({})\n", duration, get_filename());
        }
      }
    }
    m_exodusFilePtr = -1;

    return m_exodusFilePtr;
  }

  bool BaseDatabaseIO::ok_nl(bool write_message, std::string *error_message, int *bad_count) const
  {
    // For input, we try to open the existing file.

    // For output, we do not want to overwrite or clobber the output
    // file if it already exists since the app might be reading the restart
    // data from this file and then later clobbering it and then writing
    // restart data to the same file. So, for output, we first check
    // whether the file exists and if it it and is writable, assume
    // that we can later create a new or append to existing file.

    // Returns the number of processors on which this file is *NOT* ok in 'bad_count' if not null.
    // Will return 'true' only if file ok on all processors.

    if (fileExists) {
      // File has already been opened at least once...
      return dbState != Ioss::STATE_INVALID;
    }

    bool abort_if_error = false;
    bool is_ok;
    if (is_input()) {
      is_ok = open_input_file(write_message, error_message, bad_count, abort_if_error);
    }
    else {
      // See if file exists... Don't overwrite (yet) it it exists.
      bool overwrite = false;
      is_ok =
          handle_output_file(write_message, error_message, bad_count, overwrite, abort_if_error);
      // Close all open files...
      if (m_exodusFilePtr >= 0) {
        ex_close(m_exodusFilePtr);
        m_exodusFilePtr = -1;
      }
    }
    return is_ok;
  }

  void BaseDatabaseIO::finalize_file_open() const
  {
    assert(m_exodusFilePtr >= 0);
    // Check byte-size of integers stored on the database...
    if ((ex_int64_status(m_exodusFilePtr) & EX_ALL_INT64_DB) != 0) {
      if (myProcessor == 0 && !sixty_four_bit_message_output) {
        fmt::print(Ioss::DebugOut(),
                   "IOSS: Input database contains 8-byte integers. Setting Ioss to use "
                   "8-byte integers.\n");
        sixty_four_bit_message_output = true;
      }
      ex_set_int64_status(m_exodusFilePtr, EX_ALL_INT64_API);
      set_int_byte_size_api(Ioss::USE_INT64_API);
    }

    // Check for maximum name length used on the input file.
    int max_name_length = ex_inquire_int(m_exodusFilePtr, EX_INQ_DB_MAX_USED_NAME_LENGTH);
    if (max_name_length > maximumNameLength) {
      maximumNameLength = max_name_length;
    }

    ex_set_max_name_length(m_exodusFilePtr, maximumNameLength);
  }

  bool BaseDatabaseIO::open_group_nl(const std::string &group_name)
  {
    // Get existing file pointer...
    bool success = false;

    int exoid = get_file_pointer();

    m_groupName = group_name;
    ex_get_group_id(exoid, m_groupName.c_str(), &m_exodusFilePtr);

    if (m_exodusFilePtr < 0) {
      std::ostringstream errmsg;
      fmt::print(errmsg, "ERROR: Could not open group named '{}' in file '{}'.\n", m_groupName,
                 get_filename());
      IOSS_ERROR(errmsg);
    }
    success = true;
    return success;
  }

  bool BaseDatabaseIO::create_subgroup_nl(const std::string &group_name)
  {
    bool success = false;
    if (!is_input()) {
      // Get existing file pointer...
      int exoid = get_file_pointer();

      // Check name for '/' which is not allowed since it is the
      // separator character in a full group path
      if (group_name.find('/') != std::string::npos) {
        std::ostringstream errmsg;
        fmt::print(errmsg, "ERROR: Invalid group name '{}' contains a '/' which is not allowed.\n",
                   m_groupName);
        IOSS_ERROR(errmsg);
      }

      m_groupName = group_name;
      exoid       = ex_create_group(exoid, m_groupName.c_str());
      if (exoid < 0) {
        std::ostringstream errmsg;
        fmt::print(errmsg, "ERROR: Could not create group named '{}' in file '{}'.\n", m_groupName,
                   get_filename());
        IOSS_ERROR(errmsg);
      }
      m_exodusFilePtr = exoid;
      success         = true;
    }
    return success;
  }

  // common
  void BaseDatabaseIO::put_qa()
  {
    struct qa_element
    {
      char *qa_record[1][4];
    };

    size_t num_qa_records = qaRecords.size() / 4;

    if (using_parallel_io() && myProcessor != 0) {
      // This call only sets the `num_qa_records` metadata on the other ranks...
      ex_put_qa(get_file_pointer(), num_qa_records + 1, nullptr);
    }
    else {
      std::vector<qa_element> qa(num_qa_records + 1);
      for (size_t i = 0; i < num_qa_records + 1; i++) {
        for (int j = 0; j < 4; j++) {
          qa[i].qa_record[0][j] = new char[MAX_STR_LENGTH + 1];
        }
      }

      {
        int j = 0;
        for (size_t i = 0; i < num_qa_records; i++) {
          Ioss::Utils::copy_string(qa[i].qa_record[0][0], qaRecords[j++], MAX_STR_LENGTH + 1);
          Ioss::Utils::copy_string(qa[i].qa_record[0][1], qaRecords[j++], MAX_STR_LENGTH + 1);
          Ioss::Utils::copy_string(qa[i].qa_record[0][2], qaRecords[j++], MAX_STR_LENGTH + 1);
          Ioss::Utils::copy_string(qa[i].qa_record[0][3], qaRecords[j++], MAX_STR_LENGTH + 1);
        }
      }

      Ioss::Utils::time_and_date(qa[num_qa_records].qa_record[0][3],
                                 qa[num_qa_records].qa_record[0][2], MAX_STR_LENGTH);

      std::string codename = "unknown";
      std::string version  = "unknown";

      if (get_region()->property_exists("code_name")) {
        codename = get_region()->get_property("code_name").get_string();
      }
      if (get_region()->property_exists("code_version")) {
        version = get_region()->get_property("code_version").get_string();
      }

      Ioss::Utils::copy_string(qa[num_qa_records].qa_record[0][0], codename, MAX_STR_LENGTH + 1);
      Ioss::Utils::copy_string(qa[num_qa_records].qa_record[0][1], version, MAX_STR_LENGTH + 1);

      int ierr = 0;
      ierr     = ex_put_qa(get_file_pointer(), num_qa_records + 1, qa[0].qa_record);
      if (ierr < 0) {
        Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
      }

      for (size_t i = 0; i < num_qa_records + 1; i++) {
        for (int j = 0; j < 4; j++) {
          delete[] qa[i].qa_record[0][j];
        }
      }
    }
  }

  // common
  void BaseDatabaseIO::put_info()
  {
    int    total_lines = 0;
    char **info        = nullptr;

    if (!using_parallel_io() || myProcessor == 0) {
      // dump info records, include the product_registry
      // See if the input file was specified as a property on the database...
      Ioss::NameList input_lines;
      if (get_region()->property_exists("input_file_name")) {
        std::string filename = get_region()->get_property("input_file_name").get_string();
        // Determine size of input file so can embed it in info records...
        Ioss::Utils::input_file(filename, &input_lines, max_line_length);
      }

      // Get configuration information for IOSS library.
      // Split into strings and remove empty lines...
      std::string config = Ioss::IOFactory::show_configuration();
      std::replace(std::begin(config), std::end(config), '\t', ' ');
      auto lines = Ioss::tokenize(config, "\n");
      lines.erase(std::remove_if(lines.begin(), lines.end(),
                                 [](const std::string &line) { return line.empty(); }),
                  lines.end());

      // See if the client added any "information_records"
      size_t info_rec_size = informationRecords.size();
      size_t in_lines      = input_lines.size();
      size_t qa_lines      = 1; // Platform info
      size_t config_lines  = lines.size();

      total_lines = in_lines + qa_lines + info_rec_size + config_lines;

      // 'total_lines' pointers to char buffers
      info = Ioex::get_name_array(total_lines, max_line_length);

      int i = 0;
      Ioss::Utils::copy_string(info[i++], Ioss::Utils::platform_information(), max_line_length + 1);

      // Copy input file lines into 'info' array...
      for (size_t j = 0; j < input_lines.size(); j++, i++) {
        Ioss::Utils::copy_string(info[i], input_lines[j], max_line_length + 1);
      }

      // Copy "information_records" property data ...
      for (size_t j = 0; j < informationRecords.size(); j++, i++) {
        Ioss::Utils::copy_string(info[i], informationRecords[j], max_line_length + 1);
      }

      for (size_t j = 0; j < lines.size(); j++, i++) {
        Ioss::Utils::copy_string(info[i], lines[j], max_line_length + 1);
      }
    }

    if (using_parallel_io()) {
      util().broadcast(total_lines);
    }

    int ierr = 0;
    if (!using_parallel_io() || myProcessor == 0) {
      ierr = ex_put_info(get_file_pointer(), total_lines, info);
      Ioex::delete_name_array(info, total_lines);
    }
    else {
      // This call only sets the `total_lines` metadata on the other ranks...
      ierr = ex_put_info(get_file_pointer(), total_lines, nullptr);
    }
    if (ierr < 0) {
      Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
    }
  }

  // common
  int BaseDatabaseIO::get_current_state() const
  {
    int step = get_region()->get_current_state();

    if (step <= 0) {
      std::ostringstream errmsg;
      fmt::print(errmsg,
                 "ERROR: No currently active state.  The calling code must call "
                 "Ioss::Region::begin_state(int step)\n"
                 "       to set the database timestep from which to read the transient data.\n"
                 "       [{}]\n",
                 get_filename());
      IOSS_ERROR(errmsg);
    }
    return step;
  }

  void BaseDatabaseIO::update_block_omissions_from_assemblies()
  {
    Ioss::SerializeIO serializeIO_(this);

    // Query number of assemblies...
    auto assemblies = get_exodus_assemblies(get_file_pointer());
    if (!assemblies.empty()) {
      Ioss::NameList exclusions;
      Ioss::NameList inclusions;

      AssemblyTreeFilter inclusionFilter(get_region(), Ioss::ELEMENTBLOCK, assemblies);
      AssemblyTreeFilter exclusionFilter(get_region(), Ioss::ELEMENTBLOCK, assemblies);

      for (size_t i = 0; i < assemblies.size(); ++i) {
        const auto &assembly = assemblies[i];

        bool omitAssembly =
            std::binary_search(assemblyOmissions.begin(), assemblyOmissions.end(), assembly.name);
        bool includeAssembly =
            std::binary_search(assemblyInclusions.begin(), assemblyInclusions.end(), assembly.name);

        if (omitAssembly) {
          exclusionFilter.update_list_from_assembly_tree(i, exclusions);
        }

        if (includeAssembly) {
          inclusionFilter.update_list_from_assembly_tree(i, inclusions);
        }
      }

      exclusionFilter.update_assembly_filter_list(assemblyOmissions);
      inclusionFilter.update_assembly_filter_list(assemblyInclusions);

      cleanup_exodus_assembly_vector(assemblies);

      Ioss::Utils::insert_sort_and_unique(exclusions, blockOmissions);
      Ioss::Utils::insert_sort_and_unique(inclusions, blockInclusions);
    }
  }

  void BaseDatabaseIO::get_assemblies()
  {
    Ioss::SerializeIO serializeIO_(this);

    auto assemblies = get_exodus_assemblies(get_file_pointer());
    if (!assemblies.empty()) {
      for (const auto &assembly : assemblies) {
        auto *assem = new Ioss::Assembly(get_region()->get_database(), assembly.name);
        assem->property_add(Ioss::Property("id", assembly.id));
        get_region()->add(assem);
      }

      // Now iterate again and populate member lists...
      for (const auto &assembly : assemblies) {
        Ioss::Assembly *assem = get_region()->get_assembly(assembly.name);
        assert(assem != nullptr);
        auto   type               = Ioex::map_exodus_type(assembly.type);
        size_t num_added_entities = 0;

        for (int j = 0; j < assembly.entity_count; j++) {
          auto *ge = get_region()->get_entity(assembly.entity_list[j], type);
          if (ge == nullptr) {
            fmt::print(Ioss::WarnOut(),
                       "Error: Failed to find entity of type {} with id {} for assembly {}.\n",
                       Ioss::Utils::entity_type_to_string(type), assembly.entity_list[j],
                       assem->name());
            continue;
          }

          if (!Ioss::Utils::block_is_omitted(ge)) {
            assem->add(ge);
            num_added_entities++;
          }
        }
        SMART_ASSERT(assem->member_count() == num_added_entities)
        (assem->member_count())(num_added_entities);

        add_mesh_reduction_fields(assembly.id, assem);
        // Check for additional variables.
        int attribute_count = assem->get_property("attribute_count").get_int();
        add_attribute_fields(assem, attribute_count, "Assembly");
        add_reduction_results_fields(assem);
      }

      // If there are any reduction results fields ("REDUCTION"), then need to
      // allocate space for the values to be stored on each timestep...
      if (!m_reductionVariables[EX_ASSEMBLY].empty()) {
        size_t size = m_reductionVariables[EX_ASSEMBLY].size();
        for (const auto &assembly : assemblies) {
          m_reductionValues[EX_ASSEMBLY][assembly.id].resize(size);
        }
      }
      cleanup_exodus_assembly_vector(assemblies);

      assert(assemblyOmissions.empty() || assemblyInclusions.empty()); // Only one can be non-empty

      // Handle all assembly omissions or inclusions...
      if (!assemblyOmissions.empty()) {
        for (const auto &name : assemblyOmissions) {
          auto assembly = get_region()->get_assembly(name);
          if (assembly != nullptr) {
            assembly->property_add(Ioss::Property(std::string("omitted"), 1));
          }
        }
      }

      if (!assemblyInclusions.empty()) {
        const auto &ioss_assemblies = get_region()->get_assemblies();
        for (auto &assembly : ioss_assemblies) {
          assembly->property_add(Ioss::Property(std::string("omitted"), 1));
        }

        // Now, erase the property on any assemblies in the inclusion list...
        for (const auto &name : assemblyInclusions) {
          auto assembly = get_region()->get_assembly(name);
          if (assembly != nullptr) {
            assembly->property_erase("omitted");
          }
        }
      }
    }
  }

  void BaseDatabaseIO::get_blobs()
  {
    Ioss::SerializeIO serializeIO_(this);
    // Query number of blobs...
    int nblob = ex_inquire_int(get_file_pointer(), EX_INQ_BLOB);

    if (nblob > 0) {
      std::vector<ex_blob> blobs(nblob);
      int max_name_length = ex_inquire_int(get_file_pointer(), EX_INQ_DB_MAX_USED_NAME_LENGTH);
      for (auto &bl : blobs) {
        bl.name = new char[max_name_length + 1];
      }

      int ierr = ex_get_blobs(get_file_pointer(), Data(blobs));
      if (ierr < 0) {
        Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
      }

      for (const auto &bl : blobs) {
#ifdef SEACAS_HAVE_MPI
        // Each blob is spread across all processors (should support a minimum size...)
        // Determine size of blob on each rank and offset from beginning of blob.
        size_t per_proc = bl.num_entry / parallel_size();
        size_t extra    = bl.num_entry % parallel_size();
        size_t count    = per_proc + (myProcessor < (int)extra ? 1 : 0);

        size_t offset = 0;
        if (myProcessor < (int)extra) {
          offset = (per_proc + 1) * myProcessor;
        }
        else {
          offset = (per_proc + 1) * extra + per_proc * (myProcessor - extra);
        }
        Ioss::Blob *blob = new Ioss::Blob(get_region()->get_database(), bl.name, count);
        blob->property_add(Ioss::Property("_processor_offset", (int64_t)offset));
        blob->property_add(Ioss::Property("global_size", (int64_t)bl.num_entry));
#else
        auto *blob = new Ioss::Blob(get_region()->get_database(), bl.name, bl.num_entry);
#endif
        blob->property_add(Ioss::Property("id", bl.id));
        get_region()->add(blob);
      }

      // Now iterate again and populate member lists...
      int iblk = 0;
      for (const auto &bl : blobs) {
        Ioss::Blob *blob = get_region()->get_blob(bl.name);
        assert(blob != nullptr);

        add_mesh_reduction_fields(bl.id, blob);
        // Check for additional variables.
        int attribute_count = blob->get_property("attribute_count").get_int();
        add_attribute_fields(blob, attribute_count, "Blob");
        add_reduction_results_fields(blob);
        add_results_fields(blob, iblk++);
      }

      // If there are any reduction results fields ("REDUCTION"), then need to
      // allocate space for the values to be stored on each timestep...
      if (!m_reductionVariables[EX_BLOB].empty()) {
        size_t size = m_reductionVariables[EX_BLOB].size();
        for (const auto &blob : blobs) {
          m_reductionValues[EX_BLOB][blob.id].resize(size);
        }
      }

      for (const auto &bl : blobs) {
        delete[] bl.name;
      }
    }
  }

  // common
  void BaseDatabaseIO::get_nodeblocks()
  {
    // For exodusII, there is only a single node block which contains
    // all of the nodes.
    // The default id assigned is '1' and the name is 'nodeblock_1'

    std::string block_name = "nodeblock_1";
    auto        block      = new Ioss::NodeBlock(this, block_name, nodeCount, spatialDimension);
    block->property_add(Ioss::Property("id", 1));
    block->property_add(Ioss::Property("guid", util().generate_guid(1)));
    // Check for results variables.

    int num_attr = 0;
    {
      Ioss::SerializeIO serializeIO_(this);
      int               ierr = ex_get_attr_param(get_file_pointer(), EX_NODE_BLOCK, 1, &num_attr);
      if (ierr < 0) {
        Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
      }
    }

    add_attribute_fields(block, num_attr, "");
    // Not supported on nodeblocks at this time
    // add_reduction_results_fields(block);
    add_results_fields(block);

    // If there are any reduction results fields ("REDUCTION"), then need to
    // allocate space for the values to be stored on each timestep...
    if (!m_reductionVariables[EX_NODE_BLOCK].empty()) {
      size_t size = m_reductionVariables[EX_NODE_BLOCK].size();
      m_reductionValues[EX_NODE_BLOCK][1].resize(size);
    }

    bool added = get_region()->add(block);
    if (!added) {
      delete block;
    }
  }

  // common
  // common
  size_t BaseDatabaseIO::handle_block_ids(const Ioss::EntityBlock *eb, ex_entity_type map_type,
                                          Ioss::Map &entity_map, void *ids, size_t num_to_get,
                                          size_t offset) const
  {
    /*!
     * NOTE: "element" is generic for "element", "face", or "edge"
     *
     * There are two modes we need to support in this routine:
     * 1. Initial definition of element map (local->global) and
     * elemMap.reverse (global->local).
     * 2. Redefinition of element map via 'reordering' of the original
     * map when the elements on this processor are the same, but their
     * order is changed.
     *
     * So, there will be two maps the 'elemMap.map' map is a 'direct lookup'
     * map which maps current local position to global id and the
     * 'elemMap.reverse' is an associative lookup which maps the
     * global id to 'original local'.  There is also a
     * 'elemMap.reorder' which is direct lookup and maps current local
     * position to original local.

     * The ids coming in are the global ids; their position is the
     * local id -1 (That is, data[0] contains the global id of local
     * element 1 in this element block).  The 'model-local' id is
     * given by eb_offset + 1 + position:
     *
     * int local_position = elemMap.reverse[ElementMap[i+1]]
     * (the elemMap.map and elemMap.reverse are 1-based)
     *
     * But, this assumes 1..numel elements are being output at the same
     * time; we are actually outputting a blocks worth of elements at a
     * time, so we need to consider the block offsets.
     * So... local-in-block position 'i' is index 'eb_offset+i' in
     * 'elemMap.map' and the 'local_position' within the element
     * blocks data arrays is 'local_position-eb_offset'.  With this, the
     * position within the data array of this element block is:
     *
     * int eb_position =
     * elemMap.reverse[elemMap.map[eb_offset+i+1]]-eb_offset-1
     *
     * To determine which map to update on a call to this function, we
     * use the following heuristics:
     * -- If the database state is 'Ioss::STATE_MODEL:', then update the
     *    'elemMap.reverse'.
     * -- If the database state is not Ioss::STATE_MODEL, then leave
     *    the 'elemMap.reverse' alone since it corresponds to the
     *    information already written to the database. [May want to add
     *    a Ioss::STATE_REDEFINE_MODEL]
     * -- Always update elemMap.map to match the passed in 'ids'
     *    array.
     *
     * NOTE: the maps are built an element block at a time...
     * NOTE: The mapping is done on TRANSIENT fields only; MODEL fields
     *       should be in the original order...
     */

    // Overwrite this portion of the 'elemMap.map', but keep other
    // parts as they were.  We are adding elements starting at position
    // 'eb_offset+offset' and ending at
    // 'eb_offset+offset+num_to_get'. If the entire block is being
    // processed, this reduces to the range 'eb_offset..eb_offset+my_element_count'

    bool    in_define = (dbState == Ioss::STATE_MODEL) || (dbState == Ioss::STATE_DEFINE_MODEL);
    int64_t eb_offset = eb->get_offset();
    if (int_byte_size_api() == 4) {
      entity_map.set_map(static_cast<int *>(ids), num_to_get, eb_offset, in_define);
    }
    else {
      entity_map.set_map(static_cast<int64_t *>(ids), num_to_get, eb_offset, in_define);
    }

    // Now, if the state is Ioss::STATE_MODEL, output this portion of
    // the entity number map...
    if (in_define) {
      if (ex_put_partial_id_map(get_file_pointer(), map_type, offset + 1, num_to_get, ids) < 0) {
        Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
      }
    }
    return num_to_get;
  }

  // common
  void BaseDatabaseIO::compute_block_membership_nl(Ioss::SideBlock *efblock,
                                                   Ioss::NameList  &block_membership) const
  {
    const Ioss::ElementBlockContainer &element_blocks = get_region()->get_element_blocks();
    assert(Ioss::Utils::check_block_order(element_blocks));

    Ioss::Int64Vector block_ids(element_blocks.size());
    if (block_ids.size() == 1) {
      block_ids[0] = 1;
    }
    else {
      Ioss::Int64Vector element_side;
      if (int_byte_size_api() == 4) {
        Ioss::IntVector es32;
        efblock->get_field_data("element_side", es32);
        element_side.resize(es32.size());
        std::copy(es32.begin(), es32.end(), element_side.begin());
      }
      else {
        efblock->get_field_data("element_side", element_side);
      }

      size_t              number_sides = element_side.size() / 2;
      Ioss::ElementBlock *block        = nullptr;
      for (size_t iel = 0; iel < number_sides; iel++) {
        int64_t elem_id = element_side[2 * iel]; // Vector contains both element and side.
        elem_id         = elemMap.global_to_local(elem_id);
        if (block == nullptr || !block->contains(elem_id)) {
          block = get_region()->get_element_block(elem_id);
          assert(block != nullptr);
          size_t block_order = block->get_property("original_block_order").get_int();
          assert(block_order < block_ids.size());
          block_ids[block_order] = 1;
        }
      }
    }

    // Synchronize among all processors....
    if (isParallel) {
      util().global_array_minmax(block_ids, Ioss::ParallelUtils::DO_MAX);
    }

    for (const auto &block : element_blocks) {
      size_t block_order = block->get_property("original_block_order").get_int();
      assert(block_order < block_ids.size());
      if (block_ids[block_order] == 1) {
        if (!Ioss::Utils::block_is_omitted(block)) {
          block_membership.push_back(block->name());
        }
      }
    }
  }

  // common
  int64_t BaseDatabaseIO::get_field_internal(const Ioss::Region * /* region */,
                                             const Ioss::Field &field, void *data,
                                             size_t data_size) const
  {
    // For now, assume that all TRANSIENT fields on a region
    // are REDUCTION fields (1 value).  We need to gather these
    // and output them all at one time.  The storage location is a
    // 'globalVariables' array
    {
      size_t            num_to_get = field.verify(data_size);
      Ioss::SerializeIO serializeIO_(this);

      Ioss::Field::RoleType role = field.get_role();

      if (role == Ioss::Field::TRANSIENT || role == Ioss::Field::REDUCTION) {
        get_reduction_field(field, get_region(), data);
      }
      else {
        std::ostringstream errmsg;
        fmt::print(errmsg,
                   "ERROR: Can not handle non-TRANSIENT or non-REDUCTION fields on regions");
        IOSS_ERROR(errmsg);
      }
      return num_to_get;
    }
  }

  // common
  int64_t BaseDatabaseIO::put_field_internal(const Ioss::Region * /* region */,
                                             const Ioss::Field &field, void *data,
                                             size_t data_size) const
  {
    // For now, assume that all TRANSIENT fields on a region
    // are REDUCTION fields (1 value).  We need to gather these
    // and output them all at one time.  The storage location is a
    // 'globalVariables' array
    {
      Ioss::SerializeIO serializeIO_(this);

      Ioss::Field::RoleType role       = field.get_role();
      size_t                num_to_get = field.verify(data_size);

      if ((role == Ioss::Field::TRANSIENT || role == Ioss::Field::REDUCTION) && num_to_get == 1) {
        store_reduction_field(field, get_region(), data);
      }
      else if (num_to_get != 1) {
        // There should have been a warning/error message printed to the
        // log file earlier for this, so we won't print anything else
        // here since it would be printed for each and every timestep....
        ;
      }
      else {
        std::ostringstream errmsg;
        fmt::print(
            errmsg,
            "ERROR: The variable named '{}' is of the wrong type. A region variable must be of type"
            " TRANSIENT or REDUCTION.\n"
            "This is probably an internal error; please notify gdsjaar@sandia.gov",
            field.get_name());
        IOSS_ERROR(errmsg);
      }
      return num_to_get;
    }
  }

  namespace {
    // common
    template <typename T>
    void generate_block_truth_table(const VariableNameMap &variables, Ioss::IntVector &truth_table,
                                    std::vector<T *> &blocks, char field_suffix_separator)
    {
      size_t block_count = blocks.size();
      size_t var_count   = variables.size();

      if (var_count == 0 || block_count == 0) {
        return;
      }

      truth_table.resize(block_count * var_count);

      // Fill in the truth table.  It is conceptually a two-dimensional array of
      // the form 'array[num_blocks][num_element_var]'.  In C++,
      // the values for the first block are first, followed by
      // next block, ...
      size_t offset = 0;
      for (const auto &block : blocks) {
        // Get names of all transient and reduction fields...
        Ioss::NameList results_fields = block->field_describe(Ioss::Field::TRANSIENT);
        block->field_describe(Ioss::Field::REDUCTION, &results_fields);

        for (const auto &fn : results_fields) {
          Ioss::Field            field     = block->get_field(fn);
          Ioss::Field::BasicType ioss_type = field.get_type();

          int re_im = 1;
          if (ioss_type == Ioss::Field::COMPLEX) {
            re_im = 2;
          }
          for (int complex_comp = 0; complex_comp < re_im; complex_comp++) {
            for (int i = 1; i <= field.get_component_count(Ioss::Field::InOut::INPUT); i++) {
              std::string var_string =
                  field.get_component_name(i, Ioss::Field::InOut::INPUT, field_suffix_separator);
              // Find position of 'var_string' in 'variables'
              auto VN = variables.find(var_string);
              if (VN != variables.end()) {
                // Index '(*VN).second' is 1-based...
                truth_table[offset + (*VN).second - 1] = 1;
              }
            }
          }
        }
        offset += var_count;
      }
      assert(offset == var_count * block_count);
    }
  } // namespace
  // common
  void BaseDatabaseIO::store_reduction_field(const Ioss::Field          &field,
                                             const Ioss::GroupingEntity *ge, void *variables) const
  {
    Ioss::Field::BasicType ioss_type = field.get_type();
    assert(ioss_type == Ioss::Field::REAL || ioss_type == Ioss::Field::INTEGER ||
           ioss_type == Ioss::Field::INT64 || ioss_type == Ioss::Field::COMPLEX);
    auto *rvar   = static_cast<double *>(variables);
    auto *ivar   = static_cast<int *>(variables);
    auto *ivar64 = static_cast<int64_t *>(variables);

    auto id = ge->get_optional_property("id", 0);

    auto type = Ioex::map_exodus_type(ge->type());

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
    // of this name in 'm_variables[EX_GLOBAL]' map
    int comp_count = field.get_component_count(Ioss::Field::InOut::OUTPUT);
    int var_index  = 0;

    int re_im = 1;
    if (field.get_type() == Ioss::Field::COMPLEX) {
      re_im = 2;
    }
    for (int complex_comp = 0; complex_comp < re_im; complex_comp++) {
      for (int i = 0; i < comp_count; i++) {
        std::string var_name = get_component_name(field, Ioss::Field::InOut::OUTPUT, i + 1);

#if GLOBALS_ARE_TRANSIENT
        if (type == EX_GLOBAL) {
          SMART_ASSERT(m_variables[type].find(var_name) != m_variables[type].end())(type)(var_name);
          var_index = m_variables[type].find(var_name)->second;
        }
        else {
          SMART_ASSERT(m_reductionVariables[type].find(var_name) !=
                       m_reductionVariables[type].end())
          (type)(var_name);
          var_index = m_reductionVariables[type].find(var_name)->second;
        }
#else
        SMART_ASSERT(m_reductionVariables[type].find(var_name) != m_reductionVariables[type].end())
        (type)(var_name);
        var_index = m_reductionVariables[type].find(var_name)->second;
#endif

        SMART_ASSERT(static_cast<int>(m_reductionValues[type][id].size()) >= var_index)
        (id)(m_reductionValues[type][id].size())(var_index);

        // Transfer from 'variables' array.
        if (ioss_type == Ioss::Field::REAL || ioss_type == Ioss::Field::COMPLEX) {
          m_reductionValues[type][id][var_index - 1] = rvar[i];
        }
        else if (ioss_type == Ioss::Field::INTEGER) {
          m_reductionValues[type][id][var_index - 1] = ivar[i];
        }
        else if (ioss_type == Ioss::Field::INT64) {
          m_reductionValues[type][id][var_index - 1] = ivar64[i]; // FIX 64 UNSAFE
        }
      }
    }
  }

  // common
  void BaseDatabaseIO::get_reduction_field(const Ioss::Field &field, const Ioss::GroupingEntity *ge,
                                           void *variables) const
  {
    auto id   = ge->get_optional_property("id", 0);
    auto type = Ioex::map_exodus_type(ge->type());

    Ioss::Field::BasicType ioss_type = field.get_type();
    assert(ioss_type == Ioss::Field::REAL || ioss_type == Ioss::Field::INTEGER ||
           ioss_type == Ioss::Field::INT64);
    auto *rvar   = static_cast<double *>(variables);
    auto *ivar   = static_cast<int *>(variables);
    auto *i64var = static_cast<int64_t *>(variables);

    // get number of components, cycle through each component
    // and add suffix to base 'field_name'.  Look up index
    // of this name in 'm_variables[type]' map

    int comp_count = field.get_component_count(Ioss::Field::InOut::INPUT);
    for (int i = 0; i < comp_count; i++) {
      int         var_index = 0;
      std::string var_name  = get_component_name(field, Ioss::Field::InOut::INPUT, i + 1);

#if GLOBALS_ARE_TRANSIENT
      if (type == EX_GLOBAL) {
        assert(m_variables[type].find(var_name) != m_variables[type].end());
        var_index = m_variables[type].find(var_name)->second;
      }
      else {
        assert(m_reductionVariables[type].find(var_name) != m_reductionVariables[type].end());
        var_index = m_reductionVariables[type].find(var_name)->second;
      }

      assert(static_cast<int>(m_reductionValues[type][id].size()) >= var_index);
#else
      SMART_ASSERT(m_reductionVariables[type].find(var_name) != m_reductionVariables[type].end())
      (type)(var_name);
      var_index = m_reductionVariables[type].find(var_name)->second;
      SMART_ASSERT(static_cast<int>(m_reductionValues[type][id].size()) >= var_index);
#endif
      // Transfer to 'variables' array.
      if (ioss_type == Ioss::Field::REAL) {
        rvar[i] = m_reductionValues[type][id][var_index - 1];
      }
      else if (ioss_type == Ioss::Field::INT64) {
        i64var[i] = static_cast<int64_t>(m_reductionValues[type][id][var_index - 1]);
      }
      else if (ioss_type == Ioss::Field::INTEGER) {
        ivar[i] = static_cast<int>(m_reductionValues[type][id][var_index - 1]);
      }
    }
  }

  // common
  void BaseDatabaseIO::write_reduction_fields() const
  {
    int step = get_current_state();
    step     = get_database_step(step);
    for (const auto &type : exodus_types) {
      const auto &id_values = m_reductionValues[type];
      for (const auto &values : id_values) {
        int64_t id    = values.first;
        auto   &vals  = values.second;
        size_t  count = vals.size();
        if (count > 0) {
          int ierr = ex_put_reduction_vars(get_file_pointer(), step, type, id, count, Data(vals));
          if (ierr < 0) {
            Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
          }
        }
      }
    }
  }

  // common
  void BaseDatabaseIO::read_reduction_fields() const
  {
    int step = get_current_state();

    for (const auto &type : exodus_types) {
      const auto &id_values = m_reductionValues[type];
      for (const auto &values : id_values) {
        int64_t id    = values.first;
        auto   &vals  = values.second;
        size_t  count = vals.size();
        if (count > 0) {
          int ierr = ex_get_reduction_vars(get_file_pointer(), step, type, id, count,
                                           (double *)Data(vals));
          if (ierr < 0) {
            Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
          }
        }
      }
    }
  }

  // common
  bool BaseDatabaseIO::begin_nl(Ioss::State state)
  {
    dbState = state;
    return true;
  }

  // common
  bool BaseDatabaseIO::end_nl(Ioss::State state)
  {
    // Transitioning out of state 'state'
    assert(state == dbState);
    switch (state) {
    case Ioss::STATE_DEFINE_MODEL:
      if (!is_input()) {
        write_meta_data(open_create_behavior());
      }
      break;
    case Ioss::STATE_DEFINE_TRANSIENT:
      if (!is_input()) {
        write_results_metadata(true, open_create_behavior());
      }
      break;
    default: // ignore everything else...
      break;
    }

    {
      Ioss::SerializeIO serializeIO_(this);

      if (!is_input()) {
        ex_update(get_file_pointer());
        if (minimizeOpenFiles) {
          free_file_pointer();
        }
      }
      dbState = Ioss::STATE_UNKNOWN;
    }

    return true;
  }

  void BaseDatabaseIO::open_state_file(int state)
  {
    // Close current file...
    free_file_pointer();

    // Update filename to append state count...
    decodedFilename.clear();

    Ioss::FileInfo db(originalDBFilename);
    std::string    new_filename;
    if (!db.pathname().empty()) {
      new_filename += db.pathname() + "/";
    }

    if (get_cycle_count() >= 1) {
      static const std::string suffix{"ABCDEFGHIJKLMNOPQRSTUVWXYZ"};
      int                      index = (state - 1) % get_cycle_count();
      new_filename += db.basename() + "-state-" + suffix[index] + "." + db.extension();
    }
    else {
      new_filename += db.basename() + "-state-" + std::to_string(state) + "." + db.extension();
    }

    DBFilename = new_filename;
    fileExists = false;

    ex_var_params exo_params{};
#if GLOBALS_ARE_TRANSIENT
    exo_params.num_glob = m_variables[EX_GLOBAL].size();
#else
    exo_params.num_glob = m_reductionVariables[EX_GLOBAL].size();
#endif
    exo_params.num_node  = m_variables[EX_NODE_BLOCK].size();
    exo_params.num_edge  = m_variables[EX_EDGE_BLOCK].size();
    exo_params.num_face  = m_variables[EX_FACE_BLOCK].size();
    exo_params.num_elem  = m_variables[EX_ELEM_BLOCK].size();
    exo_params.num_nset  = m_variables[EX_NODE_SET].size();
    exo_params.num_eset  = m_variables[EX_EDGE_SET].size();
    exo_params.num_fset  = m_variables[EX_FACE_SET].size();
    exo_params.num_sset  = m_variables[EX_SIDE_SET].size();
    exo_params.num_elset = m_variables[EX_ELEM_SET].size();

    char the_title[max_line_length + 1];

    // Title...
    if (get_region()->property_exists("title")) {
      std::string title_str = get_region()->get_property("title").get_string();
      Ioss::Utils::copy_string(the_title, title_str);
    }
    else {
      Ioss::Utils::copy_string(the_title, "IOSS Default Output Title");
    }

    Ioex::Mesh mesh(spatialDimension, the_title, util(), !usingParallelIO);
    mesh.populate(get_region());

    // Write the metadata to the exodus file...
    Ioex::Internals data(get_file_pointer(), maximumNameLength, util());
    int             ierr = data.initialize_state_file(mesh, exo_params, originalDBFilename);

    if (ierr < 0) {
      Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
    }
  }

  bool BaseDatabaseIO::begin_state_nl(int state, double time)
  {
    Ioss::SerializeIO serializeIO_(this);

    time /= timeScaleFactor;

    if (!is_input()) {
      if (get_file_per_state()) {
        // Close current file; create new file and output transient metadata...
        open_state_file(state);
        write_results_metadata(false, open_create_behavior());
      }
      int ierr = ex_put_time(get_file_pointer(), get_database_step(state), &time);
      if (ierr < 0) {
        Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
      }

      // Zero global variable array...
      for (const auto &type : exodus_types) {
        auto &id_values = m_reductionValues[type];
        for (auto &values : id_values) {
          auto &vals = values.second;
          std::fill(vals.begin(), vals.end(), 0.0);
        }
      }
    }
    else {
      // Store reduction variables
      read_reduction_fields();
    }
    return true;
  }

  // common
  bool BaseDatabaseIO::end_state_nl(int state, double time)
  {
    Ioss::SerializeIO serializeIO_(this);

    if (!is_input()) {
      write_reduction_fields();
      time /= timeScaleFactor;
      finalize_write(state, time);
      if (minimizeOpenFiles) {
        free_file_pointer();
      }
    }
    return true;
  }

  // common
  void BaseDatabaseIO::add_region_fields()
  {
#if GLOBALS_ARE_TRANSIENT
    int field_count = add_results_fields(get_region());
#else
    int field_count = add_reduction_results_fields(get_region());
#endif
    m_reductionValues[EX_GLOBAL][0].resize(field_count);
    add_mesh_reduction_fields(0, get_region());
  }

  namespace {
    // Memory allocated in `ex_get_attributes`, this makes deletion cleaner...
    class EX_attribute : public ex_attribute
    {
    public:
      EX_attribute() { values = nullptr; }
      ~EX_attribute() { free(values); }
    };
  } // namespace

  void BaseDatabaseIO::add_mesh_reduction_fields(int64_t id, Ioss::GroupingEntity *entity)
  {
    // Get "global attributes"
    // These are single key-value per grouping entity
    // Stored as Ioss::Property with origin of ATTRIBUTE
    Ioss::SerializeIO serializeIO_(this);
    auto              type      = Ioex::map_exodus_type(entity->type());
    int               att_count = ex_get_attribute_count(get_file_pointer(), type, id);

    if (att_count > 0) {
      std::vector<EX_attribute> attr(att_count);
      ex_get_attribute_param(get_file_pointer(), type, id, Data(attr));
      ex_get_attributes(get_file_pointer(), att_count, Data(attr));

      // Create a property on `entity` for each `attribute`
      for (const auto &att : attr) {
        if (att.value_count == 0) {
          // Just an attribute name.  Give it an empty value...
          entity->property_add(Ioss::Property(att.name, "", Ioss::Property::ATTRIBUTE));
          continue;
        }
        assert(att.values != nullptr);

        switch (att.type) {
        case EX_INTEGER: {
          const auto *idata = static_cast<int *>(att.values);
          if (att.value_count == 1) {
            entity->property_add(Ioss::Property(att.name, *idata, Ioss::Property::ATTRIBUTE));
          }
          else {
            std::vector<int> tmp(att.value_count);
            std::copy(idata, idata + att.value_count, tmp.begin());
            entity->property_add(Ioss::Property(att.name, tmp, Ioss::Property::ATTRIBUTE));
          }
        } break;
        case EX_DOUBLE: {
          const auto *ddata = static_cast<double *>(att.values);
          if (att.value_count == 1) {
            entity->property_add(Ioss::Property(att.name, *ddata, Ioss::Property::ATTRIBUTE));
          }
          else {
            std::vector<double> tmp(att.value_count);
            std::copy(ddata, ddata + att.value_count, tmp.begin());
            entity->property_add(Ioss::Property(att.name, tmp, Ioss::Property::ATTRIBUTE));
          }
        } break;
        case EX_CHAR: {
          const auto *cdata = static_cast<char *>(att.values);
          entity->property_add(Ioss::Property(att.name, cdata, Ioss::Property::ATTRIBUTE));
        } break;
        }
      }
    }
  }

  // common
  int64_t BaseDatabaseIO::add_results_fields(Ioss::GroupingEntity *entity, int64_t position)
  {
    ex_entity_type type = Ioex::map_exodus_type(entity->type());
    return internal_add_results_fields(type, entity, position, m_groupCount[type],
                                       m_truthTable[type], m_variables[type]);
  }

  // common
  int64_t BaseDatabaseIO::internal_add_results_fields(ex_entity_type        type,
                                                      Ioss::GroupingEntity *entity,
                                                      int64_t position, int64_t block_count,
                                                      Ioss::IntVector       &truth_table,
                                                      Ioex::VariableNameMap &variables)
  {
    int nvar = 0;
    {
      Ioss::SerializeIO serializeIO_(this);

      int ierr = ex_get_variable_param(get_file_pointer(), type, &nvar);
      if (ierr < 0) {
        Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
      }
    }

    if (nvar > 0) {

      if (truth_table.empty()) {
        truth_table.resize(block_count * nvar);

        // Read and store the truth table (Should be there since we only
        // get to this routine if there are variables...)

        if (type == EX_NODE_BLOCK || type == EX_GLOBAL || type == EX_ASSEMBLY) {
          // These types don't have a truth table in the exodus api...
          // They do in Ioss just for some consistency...
          std::fill(truth_table.begin(), truth_table.end(), 1);
        }
        else {
          Ioss::SerializeIO serializeIO_(this);
          int               ierr =
              ex_get_truth_table(get_file_pointer(), type, block_count, nvar, Data(truth_table));
          if (ierr < 0) {
            Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
          }
        }

        // If parallel, then synchronize the truth table among all
        // processors...  Need to know that block_X has variable_Y
        // even if block_X is empty on a specific processor...  The
        // truth table contains 0 if the variable doesn't exist and 1
        // if it does, so we just take the maximum at each location...
        // This is a collective call... Make sure not in Serialize
        if (isParallel) {
          util().global_array_minmax(truth_table, Ioss::ParallelUtils::DO_MAX);
        }
      }

      // Get the variable names and add as fields. Need to decode these
      // into vector/tensor/... eventually, for now store all as
      // scalars.

      // Read the names...
      // (Currently, names are read for every block.  We could save them...)
      Ioss::NameList names;
      {
        Ioss::SerializeIO serializeIO_(this);
        names = Ioex::get_variable_names(nvar, maximumNameLength, get_file_pointer(), type);
      }

      // Add to VariableNameMap so can determine exodusII index given a
      // Sierra field name.  exodusII index is just 'i+1'
      for (int i = 0; i < nvar; i++) {
        std::string var = names[i];
        if (lowerCaseVariableNames) {
          Ioss::Utils::fixup_name(var);
        }
        variables.insert(VNMValuePair(var, i + 1));
      }

      int  offset      = position * nvar;
      int *local_truth = nullptr;
      if (!truth_table.empty()) {
        local_truth = &truth_table[offset];
      }

      // If the file contains field metadata, define fields via that mechanism...
      auto fields = get_fields_via_field_metadata(entity, type, names);

      // Now, either get all fields via suffix matching, or if the file did not
      // specify all fields in the field metadata, get the rest...
      Ioss::Utils::get_fields(entity->entity_count(), names, Ioss::Field::TRANSIENT, this,
                              local_truth, fields);

      for (auto &field : fields) {
        if (lowerCaseVariableNames) {
          Ioss::Utils::fixup_name(field.get_name());
        }
        entity->field_add(field);
      }

      for (const auto &nameTuple : Ioss::enumerate(names)) {
        const auto &i = std::get<0>(nameTuple);
        const auto &name = std::get<1>(nameTuple);
        // Verify that all names were used for a field...
        SMART_ASSERT(name.empty() || (local_truth && local_truth[i] == 0))(i)(name);
      }
    }
    return nvar;
  }

  std::vector<Ioss::Field>
  Ioex::BaseDatabaseIO::get_fields_via_field_metadata(Ioss::GroupingEntity *entity,
                                                      ex_entity_type type, Ioss::NameList &names)
  {
    std::vector<Ioss::Field> fields;
    if (!entity->get_database()->get_field_recognition()) {
      return fields;
    }
    // See if this entity is using enhanced field attributes...
    auto id = entity->get_optional_property("id", 0);
    int  enhanced_fld_cnt;
    {
      Ioss::SerializeIO serializeIO_(this);
      enhanced_fld_cnt = ex_get_field_metadata_count(get_file_pointer(), type, id);
    }

    if (enhanced_fld_cnt > 0) {
      std::vector<ex_field> exo_fields(enhanced_fld_cnt);
      for (auto &field : exo_fields) {
        field.entity_type = type;
        field.entity_id   = id;
      }

      {
        Ioss::SerializeIO serializeIO_(this);
        int               ierr = ex_get_field_metadata(get_file_pointer(), Data(exo_fields));
        if (ierr < 0) {
          Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
        }
      }

      for (const auto &exo_field : exo_fields) {
        std::string ios_field_type{};

        auto type_names = Ioss::tokenize(exo_field.type_name, ",", true);
        if (exo_field.type[0] == EX_FIELD_TYPE_SEQUENCE) {
          ios_field_type = fmt::format("Real[{}]", exo_field.cardinality[0]);
        }
        else if (exo_field.type[0] == EX_FIELD_TYPE_USER_DEFINED) {
          auto suffices = Ioss::tokenize(exo_field.suffices, ",");
          Ioss::VariableType::create_named_suffix_type(exo_field.name, suffices);
          ios_field_type = exo_field.name;
        }
        else if (exo_field.type[0] == EX_BASIS || exo_field.type[0] == EX_QUADRATURE) {
          ios_field_type = Ioss::Utils::lowercase(type_names[0]);
        }
        else {
          ios_field_type = Ioex::map_ioss_field_type(exo_field.type[0]);
        }

        int         num_copies = 1;
        std::string secondary_field_type{};

        if (exo_field.nesting == 2) {
          // For IOSS, the nesting is basically N copies of the field
          // at nesting level 1, so we just need to verify that the
          // field type is `EX_FIELD_TYPE_SEQUENCE`, `EX_BASIS`, or
          // `EX_QUADRATURE` and then get the cardinality...
          if (exo_field.type[1] == EX_FIELD_TYPE_SEQUENCE) {
            num_copies = exo_field.cardinality[1];
          }
          else if (exo_field.type[1] == EX_BASIS || exo_field.type[1] == EX_QUADRATURE) {
            secondary_field_type = Ioss::Utils::lowercase(type_names[1]);
          }
          else {
            fmt::print("ERROR: Unrecognized field type for nested field.\n");
          }
        }
        if (secondary_field_type.empty()) {
          fields.emplace_back(exo_field.name, Ioss::Field::REAL, ios_field_type, num_copies,
                              Ioss::Field::TRANSIENT, entity->entity_count());
        }
        else {
          fields.emplace_back(exo_field.name, Ioss::Field::REAL, ios_field_type,
                              secondary_field_type, Ioss::Field::TRANSIENT, entity->entity_count());
        }

        auto &field = fields.back();
        if (exo_field.nesting == 1) {
          field.set_suffix_separator(exo_field.component_separator[0]);
        }
        else {
          field.set_suffix_separator(exo_field.component_separator[0],
                                     exo_field.component_separator[1]);
        }

        if (lowerCaseVariableNames) {
          field.set_suffices_uppercase(false);
        }

        // Now remove the used field+component names from `names` to verify that we found all
        // fields on this entity... (Also set suffices_uppercase...)
        for (int i = 0; i < field.get_component_count(Ioss::Field::InOut::INPUT); i++) {
          auto comp_name = field.get_component_name(i + 1, Ioss::Field::InOut::INPUT);
          // Find `comp_name` in `names`...
          for (size_t j = 0; j < names.size(); j++) {
            if (Ioss::Utils::str_equal(comp_name, names[j])) {
              if (!lowerCaseVariableNames &&
                  field.get_component_count(Ioss::Field::InOut::INPUT) > 1 && i == 0) {
                // Find the last-most alphabetic character...
                auto len = names[j].length();
                for (size_t k = 0; k < len; k++) {
                  if (isalpha(names[j][len - k - 1])) {
                    field.set_suffices_uppercase(isupper(names[j][len - k - 1]));
                    break;
                  }
                }
              }
              names[j] = "";
              break;
            }
          }
        }

#if IOSS_DEBUG_OUTPUT
        if (myProcessor == 0) {
          fmt::print(Ioss::DebugOut(), "Enhanced Field:  Adding to {} {}:\n\t{}\n",
                     entity->type_string(), entity->name(), field);
        }
#endif
      }
    }
    return fields;
  }

  // common
  int64_t BaseDatabaseIO::add_reduction_results_fields(Ioss::GroupingEntity *entity)
  {
    ex_entity_type type = Ioex::map_exodus_type(entity->type());
    int            nvar = 0;
    {
      Ioss::SerializeIO serializeIO_(this);

      int ierr = ex_get_reduction_variable_param(get_file_pointer(), type, &nvar);
      if (ierr < 0) {
        Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
      }
    }

    if (nvar > 0) {
      // Get the variable names and add as fields. Need to decode these
      // into vector/tensor/... eventually, for now store all as
      // scalars.
      Ioss::NameList names;
      {
        Ioss::SerializeIO serializeIO_(this);
        names =
            Ioex::get_reduction_variable_names(nvar, maximumNameLength, get_file_pointer(), type);
      }

      // Add to VariableNameMap so can determine exodusII index given a
      // Sierra field name.  exodusII index is just 'i+1'
      auto &variables = m_reductionVariables[type];
      for (int i = 0; i < nvar; i++) {
        if (lowerCaseVariableNames) {
          Ioss::Utils::fixup_name(names[i]);
        }
        variables.insert(VNMValuePair(std::string(names[i]), i + 1));
      }

      int *local_truth = nullptr;

      std::vector<Ioss::Field> fields;
      int64_t                  count = 1;
      Ioss::Utils::get_fields(count, names, Ioss::Field::REDUCTION, this, local_truth, fields);

      for (const auto &field : fields) {
        entity->field_add(field);
      }

      for (int i = 0; i < nvar; i++) {
        // Verify that all names were used for a field...
        assert(names[i].empty() || (local_truth && local_truth[i] == 0));
      }
    }
    return nvar;
  }

  // common
  void BaseDatabaseIO::write_results_metadata(bool                           gather_data,
                                              Ioss::IfDatabaseExistsBehavior behavior)
  {
    if (gather_data) {
      int glob_index = 0;
#if GLOBALS_ARE_TRANSIENT
      glob_index = gather_names(m_variables[EX_GLOBAL], get_region(), glob_index, true);
#else
      glob_index = gather_names(m_reductionVariables[EX_GLOBAL], get_region(), glob_index, true);
#endif
      m_reductionValues[EX_GLOBAL][0].resize(glob_index);

      const Ioss::NodeBlockContainer &node_blocks = get_region()->get_node_blocks();
      assert(node_blocks.size() <= 1);
      internal_gather_results_metadata(EX_NODE_BLOCK, node_blocks);

      const Ioss::EdgeBlockContainer &edge_blocks = get_region()->get_edge_blocks();
      internal_gather_results_metadata(EX_EDGE_BLOCK, edge_blocks);

      const Ioss::FaceBlockContainer &face_blocks = get_region()->get_face_blocks();
      internal_gather_results_metadata(EX_FACE_BLOCK, face_blocks);

      const Ioss::ElementBlockContainer &element_blocks = get_region()->get_element_blocks();
      internal_gather_results_metadata(EX_ELEM_BLOCK, element_blocks);

      const Ioss::NodeSetContainer &nodesets = get_region()->get_nodesets();
      internal_gather_results_metadata(EX_NODE_SET, nodesets);

      const Ioss::EdgeSetContainer &edgesets = get_region()->get_edgesets();
      internal_gather_results_metadata(EX_EDGE_SET, edgesets);

      const Ioss::FaceSetContainer &facesets = get_region()->get_facesets();
      internal_gather_results_metadata(EX_FACE_SET, facesets);

      const Ioss::ElementSetContainer &elementsets = get_region()->get_elementsets();
      internal_gather_results_metadata(EX_ELEM_SET, elementsets);

      const auto &blobs = get_region()->get_blobs();
      internal_gather_results_metadata(EX_BLOB, blobs);

      const auto &assemblies = get_region()->get_assemblies();
      internal_gather_results_metadata(EX_ASSEMBLY, assemblies);

      {
        int                           index    = 0;
        const Ioss::SideSetContainer &sidesets = get_region()->get_sidesets();
        for (const auto &sideset : sidesets) {
          const Ioss::SideBlockContainer &side_blocks = sideset->get_side_blocks();
          for (const auto &block : side_blocks) {
            glob_index = gather_names(m_reductionVariables[EX_SIDE_SET], block, glob_index, true);
            index      = gather_names(m_variables[EX_SIDE_SET], block, index, false);
          }
        }
        generate_sideset_truth_table();
      }
    }

    if (behavior != Ioss::DB_APPEND && behavior != Ioss::DB_MODIFY) {
      ex_var_params exo_params{};
#if GLOBALS_ARE_TRANSIENT
      exo_params.num_glob = m_variables[EX_GLOBAL].size();
#else
      exo_params.num_glob = m_reductionVariables[EX_GLOBAL].size();
#endif
      exo_params.num_node  = m_variables[EX_NODE_BLOCK].size();
      exo_params.num_edge  = m_variables[EX_EDGE_BLOCK].size();
      exo_params.num_face  = m_variables[EX_FACE_BLOCK].size();
      exo_params.num_elem  = m_variables[EX_ELEM_BLOCK].size();
      exo_params.num_nset  = m_variables[EX_NODE_SET].size();
      exo_params.num_eset  = m_variables[EX_EDGE_SET].size();
      exo_params.num_fset  = m_variables[EX_FACE_SET].size();
      exo_params.num_sset  = m_variables[EX_SIDE_SET].size();
      exo_params.num_elset = m_variables[EX_ELEM_SET].size();

      exo_params.edge_var_tab  = Data(m_truthTable[EX_EDGE_BLOCK]);
      exo_params.face_var_tab  = Data(m_truthTable[EX_FACE_BLOCK]);
      exo_params.elem_var_tab  = Data(m_truthTable[EX_ELEM_BLOCK]);
      exo_params.nset_var_tab  = Data(m_truthTable[EX_NODE_SET]);
      exo_params.eset_var_tab  = Data(m_truthTable[EX_EDGE_SET]);
      exo_params.fset_var_tab  = Data(m_truthTable[EX_FACE_SET]);
      exo_params.sset_var_tab  = Data(m_truthTable[EX_SIDE_SET]);
      exo_params.elset_var_tab = Data(m_truthTable[EX_ELEM_SET]);

      if (isParallel) {
        // Check consistency among all processors.  They should all
        // have the same number of each variable type...
        // The called function will throw an exception if the counts differ.
        check_variable_consistency(exo_params, myProcessor, get_filename(), util());
      }

      {
        Ioss::SerializeIO serializeIO_(this);

        int ierr = ex_put_all_var_param_ext(get_file_pointer(), &exo_params);
        if (ierr < 0) {
          Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
        }

        // Blob and Assembly not supported in ex_put_all_var_param_ext...
        if (!m_variables[EX_BLOB].empty()) {
          ierr = ex_put_variable_param(get_file_pointer(), EX_BLOB, m_variables[EX_BLOB].size());
          if (ierr < 0) {
            Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
          }
        }
        if (!m_variables[EX_ASSEMBLY].empty()) {
          ierr = ex_put_variable_param(get_file_pointer(), EX_ASSEMBLY,
                                       m_variables[EX_ASSEMBLY].size());
          if (ierr < 0) {
            Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
          }
        }

        for (const auto &type : exodus_types) {
          output_results_names(type, m_variables[type], false);
          output_results_names(type, m_reductionVariables[type], true);
        }
      }

      // Output field metadata
      output_field_metadata();
    }
  }

  namespace {
    void internal_output_field_metadata(int exoid, ex_entity_type type,
                                        Ioss::GroupingEntity *entity)
    {
      // Get all transient fields on this entity...
      char default_separator = entity->get_database()->get_field_separator();
      auto results_fields    = entity->field_describe(Ioss::Field::TRANSIENT);
      for (const auto &field_name : results_fields) {
        const auto &field = entity->get_fieldref(field_name);

        ex_field exo_field{};
        Ioss::Utils::copy_string(exo_field.name, field_name);
        exo_field.entity_type = type;
        exo_field.entity_id   = entity->get_optional_property("id", 0);

        auto *storage      = field.transformed_storage();
        auto  storage_type = storage->type();

        if (storage_type == Ioss::VariableType::Type::COMPOSED) {
          exo_field.nesting = 2;

          const auto *composed = dynamic_cast<const Ioss::ComposedVariableType *>(storage);
          assert(composed != nullptr);
          exo_field.type[0]                = Ioex::map_ioss_field_type(composed->get_base_type());
          exo_field.cardinality[0]         = composed->get_base_type()->component_count();
          char separator0                  = field.get_suffix_separator();
          exo_field.component_separator[0] = separator0 == 1 ? default_separator : separator0;

          if (exo_field.type[0] == EX_FIELD_TYPE_USER_DEFINED) {
            assert(composed->get_base_type()->type() == Ioss::VariableType::Type::NAMED_SUFFIX);
            auto nsvt =
                dynamic_cast<const Ioss::NamedSuffixVariableType *>(composed->get_base_type());
            assert(nsvt != nullptr);
            std::string suffices{};
            for (int i = 0; i < nsvt->component_count(); i++) {
              if (i > 0) {
                suffices += ",";
              }
              suffices += nsvt->label(i + 1, 0);
            }
            Ioss::Utils::copy_string(exo_field.suffices, suffices.c_str(), EX_MAX_NAME + 1);
          }

          exo_field.type[1]        = Ioex::map_ioss_field_type(composed->get_secondary_type());
          exo_field.cardinality[1] = composed->get_secondary_type()->component_count();
          char separator1          = field.get_suffix_separator(1);
          exo_field.component_separator[1] = separator1 == 1 ? default_separator : separator1;
          if (exo_field.type[1] == EX_BASIS || exo_field.type[1] == EX_QUADRATURE) {
            exo_field.type_name[0] = ',';
            Ioss::Utils::copy_string(&exo_field.type_name[1],
                                     composed->get_secondary_type()->name(), EX_MAX_NAME);
          }
        }
        else if (storage_type == Ioss::VariableType::Type::COMPOSITE) {
          exo_field.nesting = 2;

          const auto *composite = dynamic_cast<const Ioss::CompositeVariableType *>(storage);
          assert(composite != nullptr);
          exo_field.type[0]                = Ioex::map_ioss_field_type(composite->get_base_type());
          exo_field.cardinality[0]         = composite->get_base_type()->component_count();
          char separator0                  = field.get_suffix_separator();
          exo_field.component_separator[0] = separator0 == 1 ? default_separator : separator0;

          exo_field.type[1]                = EX_FIELD_TYPE_SEQUENCE;
          exo_field.cardinality[1]         = composite->get_num_copies();
          char separator1                  = field.get_suffix_separator(1);
          exo_field.component_separator[1] = separator1 == 1 ? default_separator : separator1;
        }
        else {
          exo_field.nesting = 1;
          exo_field.type[0] = Ioex::map_ioss_field_type(storage);
          if (exo_field.type[0] == EX_FIELD_TYPE_SEQUENCE) {
            exo_field.cardinality[0] = storage->component_count();
          }
          if (exo_field.type[0] == EX_BASIS) {
            assert(storage->type() == Ioss::VariableType::Type::BASIS);
            const auto *basis = dynamic_cast<const Ioss::BasisVariableType *>(storage);
            assert(basis != nullptr);
            exo_field.cardinality[0] = storage->component_count();
            Ioss::Utils::copy_string(exo_field.type_name, basis->name());
          }
          if (exo_field.type[0] == EX_QUADRATURE) {
            assert(storage->type() == Ioss::VariableType::Type::QUADRATURE);
            const auto *quad = dynamic_cast<const Ioss::QuadratureVariableType *>(storage);
            assert(quad != nullptr);
            exo_field.cardinality[0] = storage->component_count();
            Ioss::Utils::copy_string(exo_field.type_name, quad->name());
          }
          if (exo_field.type[0] == EX_FIELD_TYPE_USER_DEFINED) {
            assert(storage->type() == Ioss::VariableType::Type::NAMED_SUFFIX);
            auto nsvt = dynamic_cast<const Ioss::NamedSuffixVariableType *>(storage);
            assert(nsvt != nullptr);
            exo_field.cardinality[0] = nsvt->component_count();
            std::string suffices{};
            for (int i = 0; i < nsvt->component_count(); i++) {
              if (i > 0) {
                suffices += ",";
              }
              suffices += nsvt->label(i + 1, 0);
            }
            Ioss::Utils::copy_string(exo_field.suffices, suffices.c_str(), EX_MAX_NAME + 1);
          }
          char separator                   = field.get_suffix_separator();
          exo_field.component_separator[0] = separator == 1 ? default_separator : separator;
        }

        if (exo_field.type[0] != EX_SCALAR) {
          ex_put_field_metadata(exoid, exo_field);
          if (exo_field.type[0] == EX_FIELD_TYPE_USER_DEFINED) {
            ex_put_field_suffices(exoid, exo_field, exo_field.suffices);
          }
        }
      }
    }

    void output_basis(int exoid, const Ioss::VariableType *var)
    {
      const auto *basis = dynamic_cast<const Ioss::BasisVariableType *>(var);
      assert(basis != nullptr);
      ex_basis exo_basis{};
      exo_basis.cardinality = basis->component_count();
      ex_initialize_basis_struct(&exo_basis, 1, 1);
      Ioss::Utils::copy_string(exo_basis.name, basis->name(), EX_MAX_NAME);
      for (int i = 0; i < basis->component_count(); i++) {
        const auto &component         = basis->get_basis_component(i + 1);
        exo_basis.subc_dim[i]         = component.subc_dim;
        exo_basis.subc_ordinal[i]     = component.subc_ordinal;
        exo_basis.subc_dof_ordinal[i] = component.subc_dof_ordinal;
        exo_basis.subc_num_dof[i]     = component.subc_num_dof;
        exo_basis.xi[i]               = component.xi;
        exo_basis.eta[i]              = component.eta;
        exo_basis.zeta[i]             = component.zeta;
      }
      ex_put_basis(exoid, exo_basis);
      ex_initialize_basis_struct(&exo_basis, 1, -1);
    }

    void output_quad(int exoid, const Ioss::VariableType *var)
    {
      const auto *quadrature = dynamic_cast<const Ioss::QuadratureVariableType *>(var);
      assert(quadrature != nullptr);
      ex_quadrature exo_quadrature{};
      exo_quadrature.cardinality = quadrature->component_count();
      ex_initialize_quadrature_struct(&exo_quadrature, 1, 1);
      Ioss::Utils::copy_string(exo_quadrature.name, quadrature->name(), EX_MAX_NAME);
      const auto &quad = quadrature->get_quadrature();
      for (const auto &quadTuple: Ioss::enumerate(quad)) {
        const auto &i = std::get<0>(quadTuple);
        const auto &component = std::get<1>(quadTuple);
        exo_quadrature.xi[i]     = component.xi;
        exo_quadrature.eta[i]    = component.eta;
        exo_quadrature.zeta[i]   = component.zeta;
        exo_quadrature.weight[i] = component.weight;
      }
      ex_put_quadrature(exoid, exo_quadrature);
      ex_initialize_quadrature_struct(&exo_quadrature, 1, -1);
    }

    void output_type_metadata(int exoid)
    {
      // Iterate the list and output the `quadrature` and `basis` types...
      auto basis_list = Ioss::VariableType::external_types(Ioss::VariableType::Type::BASIS);
      for (auto &var : basis_list) {
        output_basis(exoid, var);
      }

      auto quad_list = Ioss::VariableType::external_types(Ioss::VariableType::Type::QUADRATURE);
      for (auto &var : quad_list) {
        output_quad(exoid, var);
      }
    }

    template <typename T>
    void internal_output_field_metadata(int exoid, ex_entity_type type, std::vector<T *> entities)
    {
      for (const auto &entity : entities) {
        internal_output_field_metadata(exoid, type, entity);
      }
    }
  } // namespace

  void BaseDatabaseIO::output_field_metadata()
  {
    Ioss::SerializeIO serializeIO_(this);
    // Output the 'basis' and 'quadrature' type metadata...
    output_type_metadata(get_file_pointer());

    const Ioss::NodeBlockContainer &node_blocks = get_region()->get_node_blocks();
    assert(node_blocks.size() <= 1);
    internal_output_field_metadata(get_file_pointer(), EX_NODE_BLOCK, node_blocks);

    const Ioss::EdgeBlockContainer &edge_blocks = get_region()->get_edge_blocks();
    internal_output_field_metadata(get_file_pointer(), EX_EDGE_BLOCK, edge_blocks);

    const Ioss::FaceBlockContainer &face_blocks = get_region()->get_face_blocks();
    internal_output_field_metadata(get_file_pointer(), EX_FACE_BLOCK, face_blocks);

    const Ioss::ElementBlockContainer &element_blocks = get_region()->get_element_blocks();
    internal_output_field_metadata(get_file_pointer(), EX_ELEM_BLOCK, element_blocks);

    const Ioss::NodeSetContainer &nodesets = get_region()->get_nodesets();
    internal_output_field_metadata(get_file_pointer(), EX_NODE_SET, nodesets);

    const Ioss::EdgeSetContainer &edgesets = get_region()->get_edgesets();
    internal_output_field_metadata(get_file_pointer(), EX_EDGE_SET, edgesets);

    const Ioss::FaceSetContainer &facesets = get_region()->get_facesets();
    internal_output_field_metadata(get_file_pointer(), EX_FACE_SET, facesets);

    const Ioss::ElementSetContainer &elementsets = get_region()->get_elementsets();
    internal_output_field_metadata(get_file_pointer(), EX_ELEM_SET, elementsets);

    const auto &blobs = get_region()->get_blobs();
    internal_output_field_metadata(get_file_pointer(), EX_BLOB, blobs);

    const auto &assemblies = get_region()->get_assemblies();
    internal_output_field_metadata(get_file_pointer(), EX_ASSEMBLY, assemblies);

    const Ioss::SideSetContainer &sidesets = get_region()->get_sidesets();
    internal_output_field_metadata(get_file_pointer(), EX_SIDE_SET, sidesets);
  }

  // common
  template <typename T>
  void BaseDatabaseIO::internal_gather_results_metadata(ex_entity_type   type,
                                                        std::vector<T *> entities)
  {
    int index     = 0;
    int red_index = 0;
    for (const auto &entity : entities) {
      red_index = gather_names(m_reductionVariables[type], entity, red_index, true);
      index     = gather_names(m_variables[type], entity, index, false);
    }

#if GLOBALS_ARE_TRANSIENT
    size_t value_size =
        type == EX_GLOBAL ? m_variables[type].size() : m_reductionVariables[type].size();
#else
    size_t value_size = m_reductionVariables[type].size();
#endif
    for (const auto &entity : entities) {
      auto id = entity->get_optional_property("id", 0);
      m_reductionValues[type][id].resize(value_size);
    }

    generate_block_truth_table(m_variables[type], m_truthTable[type], entities,
                               get_field_separator());
  }

  // common
  int BaseDatabaseIO::gather_names(VariableNameMap &variables, const Ioss::GroupingEntity *ge,
                                   int index, bool reduction)
  {
    int new_index = index;

    auto type   = Ioex::map_exodus_type(ge->type());
    bool nblock = (type == EX_NODE_BLOCK);

    // Get names of all transient and reduction fields...
    Ioss::NameList results_fields;
    if (reduction) {
      ge->field_describe(Ioss::Field::REDUCTION, &results_fields);
    }

    if (!reduction || type == EX_GLOBAL) {
      ge->field_describe(Ioss::Field::TRANSIENT, &results_fields);
    }

    // Some applications will set the index on the field to get a specific
    // ordering of the fields. For exodus, we typically use that to get the
    // same output ordering as the input ordering. The output from `field_describe`
    // comes back sorted on field names.  Lets check whether any of the fields
    // have an index set and if so, then sort the fields based on the index...
    std::vector<Ioss::Field> fields;
    fields.reserve(results_fields.size());
    for (const auto &name : results_fields) {
      fields.push_back(ge->get_field(name));
    }
    std::stable_sort(fields.begin(), fields.end(), [](const Ioss::Field &a, const Ioss::Field &b) {
      return a.get_index() < b.get_index();
    });

    // NOTE: For exodusII, the convention is that the displacement
    //       fields are the first 'ndim' fields in the file.
    //       Try to find a likely displacement field
    std::string disp_name;
    bool        has_disp = false;
    if (!reduction && nblock && new_index == 0) {
      has_disp = find_displacement_field(results_fields, ge, spatialDimension, &disp_name);
      if (has_disp) {
        new_index += spatialDimension;
      }
    }

    int save_index = 0;
    for (const auto &field : fields) {
      if (has_disp && field.get_name() == disp_name && new_index != 0) {
        save_index = new_index;
        new_index  = 0;
      }

      int re_im = 1;
      if (field.get_type() == Ioss::Field::COMPLEX) {
        re_im = 2;
      }
      for (int complex_comp = 0; complex_comp < re_im; complex_comp++) {
        for (int i = 1; i <= field.get_component_count(Ioss::Field::InOut::OUTPUT); i++) {
          std::string var_string = get_component_name(field, Ioss::Field::InOut::OUTPUT, i);

          if (variables.find(var_string) == variables.end()) {
            variables.insert(VNMValuePair(var_string, ++new_index));
          }
        }
      }
      if (has_disp && field.get_name() == disp_name) {
        new_index = save_index;
      }
    }
    return new_index;
  }

  // common
  void BaseDatabaseIO::generate_sideset_truth_table()
  {
    size_t var_count = m_variables[EX_SIDE_SET].size();

    if (var_count == 0 || m_groupCount[EX_SIDE_SET] == 0) {
      return;
    }

    // Member variable.  Will be deleted in destructor...
    m_truthTable[EX_SIDE_SET].resize(m_groupCount[EX_SIDE_SET] * var_count);

    // Fill in the truth table.  It is conceptually a two-dimensional array of
    // the form 'array[num_blocks][num_var]'.  In C++,
    // the values for the first block are first, followed by
    // next block, ...
    size_t offset = 0;

    const Ioss::SideSetContainer &sidesets = get_region()->get_sidesets();
    for (const auto &sideset : sidesets) {

      const Ioss::SideBlockContainer &side_blocks = sideset->get_side_blocks();
      for (const auto &block : side_blocks) {
        // See if this sideblock has a corresponding entry in the sideset list.
        if (block->property_exists("invalid")) {
          continue;
        }

        // Get names of all transient and reduction fields...
        Ioss::NameList results_fields = block->field_describe(Ioss::Field::TRANSIENT);
        block->field_describe(Ioss::Field::REDUCTION, &results_fields);

        for (const auto &fn : results_fields) {
          Ioss::Field            field     = block->get_field(fn);
          Ioss::Field::BasicType ioss_type = field.get_type();

          int re_im = 1;
          if (ioss_type == Ioss::Field::COMPLEX) {
            re_im = 2;
          }
          for (int complex_comp = 0; complex_comp < re_im; complex_comp++) {
            for (int i = 1; i <= field.get_component_count(Ioss::Field::InOut::OUTPUT); i++) {
              std::string var_string = get_component_name(field, Ioss::Field::InOut::OUTPUT, i);
              // Find position of 'var_string' in 'm_variables[]'
              auto VN = m_variables[EX_SIDE_SET].find(var_string);
              if (VN != m_variables[EX_SIDE_SET].end()) {
                // Index '(*VN).second' is 1-based...
                m_truthTable[EX_SIDE_SET][offset + (*VN).second - 1] = 1;
              }
            }
          }
        }
      }
      offset += var_count;
    }
    assert(offset == var_count * m_groupCount[EX_SIDE_SET]);
  }

  // common
  void BaseDatabaseIO::output_results_names(ex_entity_type type, VariableNameMap &variables,
                                            bool reduction) const
  {
    bool lowercase_names =
        (properties.exists("VARIABLE_NAME_CASE") &&
         Ioss::Utils::lowercase(properties.get("VARIABLE_NAME_CASE").get_string()) == "lower");
    bool uppercase_names =
        (properties.exists("VARIABLE_NAME_CASE") &&
         Ioss::Utils::lowercase(properties.get("VARIABLE_NAME_CASE").get_string()) == "upper");

    size_t var_count = variables.size();

    if (var_count > 0) {
      size_t name_length = 0;
      // Push into a char** array...
      std::vector<char *> var_names(var_count);
      Ioss::NameList      variable_names(var_count);
      for (const auto &variable : variables) {
        size_t index = variable.second;
        assert(index > 0 && index <= var_count);
        variable_names[index - 1] = variable.first;
        if (uppercase_names) {
          variable_names[index - 1] = Ioss::Utils::uppercase(variable_names[index - 1]);
        }
        else if (lowercase_names) {
          variable_names[index - 1] = Ioss::Utils::lowercase(variable_names[index - 1]);
        }
        var_names[index - 1] = const_cast<char *>(variable_names[index - 1].c_str());
        size_t name_len      = variable_names[index - 1].length();
        name_length          = name_len > name_length ? name_len : name_length;
      }

      // Should handle this automatically, but by the time we get to defining transient fields, we
      // have already created the output database and populated the set/block names. At this point,
      // it is too late to change the size of the names stored on the output database... (I think...
      // try changing DIM_STR_NAME value and see if works...)
      if (name_length > static_cast<size_t>(maximumNameLength)) {
        if (myProcessor == 0) {
          fmt::print(Ioss::WarnOut(),
                     "There are variables names whose name length ({0}) exceeds the current "
                     "maximum name length ({1})\n         set for this database ({2}).\n"
                     "         You should either reduce the length of the variable name, or "
                     "set the 'MAXIMUM_NAME_LENGTH' property\n"
                     "         to at least {0}.\n         Contact gdsjaar@sandia.gov for more "
                     "information.\n\n",
                     name_length, maximumNameLength, get_filename());
        }
      }
      int ierr = 0;
      if (reduction) {
        ierr =
            ex_put_reduction_variable_names(get_file_pointer(), type, var_count, Data(var_names));
      }
      else {
        ierr = ex_put_variable_names(get_file_pointer(), type, var_count, Data(var_names));
      }
      if (ierr < 0) {
        Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
      }
    }
  }

  // common
  // Handle special output time requests -- primarily restart (cycle, overwrite)
  // Given the global region step, return the step on the database...
  int BaseDatabaseIO::get_database_step(int global_step) const
  {
    if (get_file_per_state()) {
      return 1;
    }

    assert(overlayCount >= 0 && cycleCount >= 0);
    if (overlayCount == 0 && cycleCount == 0) {
      return global_step;
    }

    int local_step = global_step - 1;
    local_step /= (overlayCount + 1);
    if (cycleCount > 0) {
      local_step %= cycleCount;
    }
    return local_step + 1;
  }

  // common
  void BaseDatabaseIO::flush_database_nl() const
  {
    if (!is_input()) {
      if (isParallel || myProcessor == 0) {
        ex_update(get_file_pointer());
      }
    }
  }

  void BaseDatabaseIO::finalize_write(int state, double sim_time)
  {
    // Attempt to ensure that all data written up to this point has
    // actually made it out to disk.  We also write a special attribute
    // to the file to indicate that the current timestep should be
    // complete on the disk.
    // The attribute is a GLOBAL attribute named "last_written_time"
    // which is a double value which can be compared to the values in
    // the time array to make sure they match.  If they don't, then
    // hopefully the "last_written_time" is smaller than the time
    // array value and indicates that the last step is corrupt.

    // Update the attribute.
    Ioex::update_last_time_attribute(get_file_pointer(), sim_time);

    // Flush the files buffer to disk...
    // If:
    //  flushInterval == -1 (default) -- flush if there is more
    // than 10 seconds since the last flush to avoid
    // the flush eating up cpu time for small fast jobs...
    //
    //  flushInterval == 0 -- do not flush until file is closed.
    //
    //  flushInterval == 1 -- flush every step
    //
    //  flushInterval > 1 -- flush if step % flushInterval == 0

    bool do_flush = true;
    if (flushInterval == 1) {
      do_flush = true;
    }
    else if (flushInterval == 0) {
      do_flush = false;
    }
    else if (flushInterval < 0) {
      time_t cur_time = time(nullptr);
      if (cur_time - timeLastFlush >= 10) {
        timeLastFlush = cur_time;
        do_flush      = true;
      }
      else {
        do_flush = false;
      }
#ifdef SEACAS_HAVE_MPI
      if (isParallel) {
        int iflush = do_flush ? 1 : 0;
        util().broadcast(iflush);
        do_flush = iflush == 1;
      }
#endif
    }
    else if (flushInterval > 1) {
      if (state % flushInterval == 0) {
        do_flush = true;
      }
    }

    if (do_flush) {
      flush_database_nl();
    }
  }

  // common
  void Ioex::BaseDatabaseIO::add_attribute_fields(Ioss::GroupingEntity *block, int attribute_count,
                                                  const std::string &type)
  {
    /// \todo REFACTOR Some of the attribute knowledge should be at
    /// the Ioss::ElementTopology level instead of here. That would
    /// make it easier for an application to register a new element
    /// type and its attributes.

    // Attribute "Conventions" to be used if there are no attribute names on the database:
    // from Table 1 in ExodusII manual
    //
    // Circle     1     Radius [Volume]
    // Sphere     1     Radius [Volume]
    // Truss      1     Area
    // 2D Beam    3     Area, I, J
    // 3D Beam    7     Area, I1, I2, J, V1, V2, V3 (V will be a 3D vector named "reference_axis")
    // Shell      1     Thickness
    //
    // Additional conventions not defined in ExodusII manual:
    // * If a "beam" has 1 attribute, call it "area"
    // * Treat "bar" and "rod" as aliases for "truss"
    // * Treat "trishell" as alias for "shell"
    // * All "shell" or "trishell" elements -- If #attributes == #node/element, the
    //                                         attribute is "nodal_thickness"
    //
    // If there are attribute names on the database, use those names.
    // Always create a variable "attribute" which contains a single
    // field for all attributes...

    assert(block != nullptr);
    if (attribute_count > 0) {
      // Some older applications do not want to used named
      // attributes; in this case, just create a field for each
      // attribute named "attribute_1", "attribute_2", ..., "attribute_#"
      // This is controlled by the database property
      // "IGNORE_ATTRIBUTE_NAMES"
      bool attributes_named = true; // Possibly reset below; note that even if ignoring
      // attribute names, they are still 'named'

      size_t         my_element_count = block->entity_count();
      Ioss::NameList names;
      names.reserve(attribute_count);
      if (properties.exists("IGNORE_ATTRIBUTE_NAMES")) {
        for (int i = 0; i < attribute_count; i++) {
          names.emplace_back(fmt::format("attribute_{}", i + 1));
        }
      }
      else {
        // Use attribute names if they exist.

        // Get the attribute names. May not exist or may be blank...
        char  **cnames = Ioex::get_name_array(attribute_count, maximumNameLength);
        int64_t id     = block->get_property("id").get_int();

        {
          Ioss::SerializeIO serializeIO_(this);
          if (block->entity_count() != 0) {
            ex_entity_type entity_type = Ioex::map_exodus_type(block->type());
            int ierr = ex_get_attr_names(get_file_pointer(), entity_type, id, &cnames[0]);
            if (ierr < 0) {
              Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
            }
          }
        }

        // Sync names across processors...
        if (isParallel) {
          std::vector<char> cname(attribute_count * (maximumNameLength + 1));
          if (block->entity_count() != 0) {
            for (int i = 0; i < attribute_count; i++) {
              std::memcpy(&cname[i * (maximumNameLength + 1)], cnames[i], maximumNameLength + 1);
            }
          }
          util().attribute_reduction(attribute_count * (maximumNameLength + 1), Data(cname));
          for (int i = 0; i < attribute_count; i++) {
            std::memcpy(cnames[i], &cname[i * (maximumNameLength + 1)], maximumNameLength + 1);
          }
        }

        // Convert to lowercase.
        attributes_named = true;
        for (int i = 0; i < attribute_count; i++) {
          if (cnames[i][0] == '\0' || (!(std::isalnum(cnames[i][0]) || cnames[i][0] == '_'))) {
            attributes_named = false;
          }
        }
        if (attributes_named) {
          for (int i = 0; i < attribute_count; i++) {
            fix_bad_name(cnames[i]);
            Ioss::Utils::fixup_name(cnames[i]);
            names.emplace_back(cnames[i]);
          }
        }
        // Release memory...
        Ioex::delete_name_array(cnames, attribute_count);
      }

      if (attributes_named) {
        std::vector<Ioss::Field> attributes;
        Ioss::Utils::get_fields(my_element_count, names, Ioss::Field::ATTRIBUTE, this, nullptr,
                                attributes);
        int offset = 1;
        for (const auto &field : attributes) {
          if (block->field_exists(field.get_name())) {
            std::ostringstream errmsg;
            fmt::print(errmsg,
                       "ERROR: In block '{}', attribute '{}' is defined multiple times which is "
                       "not allowed.\n",
                       block->name(), field.get_name());
            IOSS_ERROR(errmsg);
          }
          block->field_add(field);
          const Ioss::Field &tmp_field = block->get_fieldref(field.get_name());
          tmp_field.set_index(offset);
          offset += field.get_component_count(Ioss::Field::InOut::INPUT);
        }
      }
      else {
        // Attributes are not named....
        // Try to assign some meaningful names based on conventions...
        int unknown_attributes = 0;

        if (type_match(type, "shell") || type_match(type, "trishell")) {
          if (attribute_count == block->get_property("topology_node_count").get_int()) {

            std::string att_name = "nodal_thickness";

            std::string storage = fmt::format("Real[{}]", attribute_count);
            block->field_add(Ioss::Field(std::move(att_name), Ioss::Field::REAL, storage,
                                         Ioss::Field::ATTRIBUTE, my_element_count, 1));
          }
          else {
            std::string att_name = "thickness";
            block->field_add(Ioss::Field(std::move(att_name), Ioss::Field::REAL, IOSS_SCALAR(),
                                         Ioss::Field::ATTRIBUTE, my_element_count, 1));
            unknown_attributes = attribute_count - 1;
          }
        }

        // NOTE: This must appear before the "sphere" check since
        // sphere is substring of "sphere-mass"
        // Want an exact match here, not substring match...
        else if (Ioss::Utils::str_equal(type, "sphere-mass")) {
          if (attribute_count != 10) {
            if (myProcessor == 0) {
              fmt::print(Ioss::WarnOut(),
                         "For element block '{}' of type '{}' there were {} attributes instead of "
                         "the expected 10 attributes "
                         "known to the IO Subsystem. "
                         " The attributes can be accessed as the field named 'attribute'",
                         block->name(), type, attribute_count);
            }
          }
          else {
            // First attribute is concentrated mass...
            size_t offset = 1;
            block->field_add(Ioss::Field("mass", Ioss::Field::REAL, IOSS_SCALAR(),
                                         Ioss::Field::ATTRIBUTE, my_element_count, offset));
            offset += 1;

            // Next six attributes are moment of inertia -- symmetric tensor
            block->field_add(Ioss::Field("inertia", Ioss::Field::REAL, IOSS_SYM_TENSOR(),
                                         Ioss::Field::ATTRIBUTE, my_element_count, offset));
            offset += 6;

            // Next three attributes are offset from node to CG
            block->field_add(Ioss::Field("offset", Ioss::Field::REAL, IOSS_VECTOR_3D(),
                                         Ioss::Field::ATTRIBUTE, my_element_count, offset));
          }
        }

        else if (type_match(type, "circle") || type_match(type, "sphere")) {
          std::string att_name = "radius";
          size_t      offset   = 1;
          block->field_add(Ioss::Field(att_name, Ioss::Field::REAL, IOSS_SCALAR(),
                                       Ioss::Field::ATTRIBUTE, my_element_count, offset++));
          if (attribute_count > 1) {
            // Default second attribute (from sphgen3d) is "volume"
            // which is the volume of the cube which would surround a
            // sphere of the given radius.
            att_name = "volume";
            block->field_add(Ioss::Field(std::move(att_name), Ioss::Field::REAL, IOSS_SCALAR(),
                                         Ioss::Field::ATTRIBUTE, my_element_count, offset++));
          }
          unknown_attributes = attribute_count - 2;
        }

        else if (type_match(type, "truss") || type_match(type, "bar") || type_match(type, "beam") ||
                 type_match(type, "rod")) {
          // Technically, truss, bar, rod should all only have 1 attribute; however,
          // there are some mesh generation codes that treat all of these types the
          // same and put "beam-type" attributes on bars...
          int         index    = 1;
          std::string att_name = "area";
          block->field_add(Ioss::Field(std::move(att_name), Ioss::Field::REAL, IOSS_SCALAR(),
                                       Ioss::Field::ATTRIBUTE, my_element_count, index++));

          if (spatialDimension == 2 && attribute_count >= 3) {
            block->field_add(Ioss::Field("i", Ioss::Field::REAL, IOSS_SCALAR(),
                                         Ioss::Field::ATTRIBUTE, my_element_count, index++));
            block->field_add(Ioss::Field("j", Ioss::Field::REAL, IOSS_SCALAR(),
                                         Ioss::Field::ATTRIBUTE, my_element_count, index++));
          }
          else if (spatialDimension == 3 && attribute_count >= 7) {
            block->field_add(Ioss::Field("i1", Ioss::Field::REAL, IOSS_SCALAR(),
                                         Ioss::Field::ATTRIBUTE, my_element_count, index++));
            block->field_add(Ioss::Field("i2", Ioss::Field::REAL, IOSS_SCALAR(),
                                         Ioss::Field::ATTRIBUTE, my_element_count, index++));
            block->field_add(Ioss::Field("j", Ioss::Field::REAL, IOSS_SCALAR(),
                                         Ioss::Field::ATTRIBUTE, my_element_count, index++));
            block->field_add(Ioss::Field("reference_axis", Ioss::Field::REAL, IOSS_VECTOR_3D(),
                                         Ioss::Field::ATTRIBUTE, my_element_count, index));
            index += 3;
            if (attribute_count >= 10) {
              // Next three attributes would (hopefully) be offset vector...
              // This is typically from a NASGEN model.
              block->field_add(Ioss::Field("offset", Ioss::Field::REAL, IOSS_VECTOR_3D(),
                                           Ioss::Field::ATTRIBUTE, my_element_count, index));
              index += 3;
            }
          }
          unknown_attributes = attribute_count - (index - 1);
        }

        else {
          unknown_attributes = attribute_count;
        }

        if (unknown_attributes > 0) {
          std::string att_name = "extra_attribute_";
          att_name += std::to_string(unknown_attributes);
          std::string storage = fmt::format("Real[{}]", unknown_attributes);
          size_t      index   = attribute_count - unknown_attributes + 1;
          block->field_add(Ioss::Field(std::move(att_name), Ioss::Field::REAL, storage,
                                       Ioss::Field::ATTRIBUTE, my_element_count, index));
        }
      }

      // Always create a field called "attribute" containing data
      // for all attributes on the mesh
      std::string att_name = "attribute"; // Default
      std::string storage  = fmt::format("Real[{}]", attribute_count);

      block->field_add(Ioss::Field(std::move(att_name), Ioss::Field::REAL, storage,
                                   Ioss::Field::ATTRIBUTE, my_element_count, 1));
    }
  }

  void BaseDatabaseIO::common_write_metadata(Ioss::IfDatabaseExistsBehavior behavior)
  {
    Ioss::Region *region = get_region();

    // Verify that exodus supports the mesh_type...
    if (region->mesh_type() != Ioss::MeshType::UNSTRUCTURED) {
      std::ostringstream errmsg;
      fmt::print(errmsg,
                 "ERROR: The mesh type is '{}' which Exodus does not support.\n"
                 "       Only 'Unstructured' is supported at this time.\n",
                 region->mesh_type_string());
      IOSS_ERROR(errmsg);
    }

    const Ioss::NodeBlockContainer &node_blocks = region->get_node_blocks();
    assert(node_blocks.size() <= 1);
    if (!node_blocks.empty()) {
      Ioex::get_id(node_blocks[0], &ids_);
      nodeCount        = node_blocks[0]->entity_count();
      spatialDimension = node_blocks[0]->get_property("component_degree").get_int();
    }
    else {
      spatialDimension = 1;
    }

    // Assemblies --
    {
      const auto &assemblies = region->get_assemblies();
      if (behavior != Ioss::DB_MODIFY) {
        // Set ids of all entities that have "id" property...
        for (const auto &assem : assemblies) {
          Ioex::set_id(assem, &ids_);
        }

        for (const auto &assem : assemblies) {
          Ioex::get_id(assem, &ids_);
        }
      }
      m_groupCount[EX_ASSEMBLY] = assemblies.size();
    }

    // Blobs --
    {
      const auto &blobs = region->get_blobs();
      // Set ids of all entities that have "id" property...
      if (behavior != Ioss::DB_MODIFY) {
        for (const auto &blob : blobs) {
          Ioex::set_id(blob, &ids_);
        }

        for (const auto &blob : blobs) {
          Ioex::get_id(blob, &ids_);
        }
      }
      m_groupCount[EX_BLOB] = blobs.size();
    }

    // Edge Blocks --
    {
      const Ioss::EdgeBlockContainer &edge_blocks = region->get_edge_blocks();
      assert(Ioss::Utils::check_block_order(edge_blocks));
      // Set ids of all entities that have "id" property...
      if (behavior != Ioss::DB_MODIFY) {
        for (const auto &edge_block : edge_blocks) {
          Ioex::set_id(edge_block, &ids_);
        }

        edgeCount = 0;
        for (const auto &edge_block : edge_blocks) {
          edgeCount += edge_block->entity_count();
          // Set ids of all entities that do not have "id" property...
          Ioex::get_id(edge_block, &ids_);
        }
      }
      m_groupCount[EX_EDGE_BLOCK] = edge_blocks.size();
    }

    // Face Blocks --
    {
      const Ioss::FaceBlockContainer &face_blocks = region->get_face_blocks();
      assert(Ioss::Utils::check_block_order(face_blocks));
      // Set ids of all entities that have "id" property...
      if (behavior != Ioss::DB_MODIFY) {
        for (const auto &face_block : face_blocks) {
          Ioex::set_id(face_block, &ids_);
        }

        faceCount = 0;
        for (auto &face_block : face_blocks) {
          faceCount += face_block->entity_count();
          // Set ids of all entities that do not have "id" property...
          Ioex::get_id(face_block, &ids_);
        }
      }
      m_groupCount[EX_FACE_BLOCK] = face_blocks.size();
    }

    // Element Blocks --
    {
      const Ioss::ElementBlockContainer &element_blocks = region->get_element_blocks();
      assert(Ioss::Utils::check_block_order(element_blocks));
      if (behavior != Ioss::DB_MODIFY) {
        // Set ids of all entities that have "id" property...
        for (const auto &element_block : element_blocks) {
          Ioex::set_id(element_block, &ids_);
        }
      }
      elementCount = 0;
      Ioss::Int64Vector element_counts;
      element_counts.reserve(element_blocks.size());
      for (const auto &element_block : element_blocks) {
        elementCount += element_block->entity_count();
        element_counts.push_back(element_block->entity_count());
        // Set ids of all entities that do not have "id" property...
        if (behavior != Ioss::DB_MODIFY) {
          Ioex::get_id(element_block, &ids_);
        }
      }
      m_groupCount[EX_ELEM_BLOCK] = element_blocks.size();

      if (isParallel) {
        // Set "global_entity_count" property on all blocks.
        // Used to skip output on "globally" empty blocks.
        Ioss::Int64Vector global_counts(element_counts.size());
        util().global_count(element_counts, global_counts);
        size_t idx = 0;
        for (const auto &element_block : element_blocks) {
          element_block->property_add(Ioss::Property("global_entity_count", global_counts[idx++]));
        }
      }
    }

    // NodeSets ...
    {
      const Ioss::NodeSetContainer &nodesets = region->get_nodesets();
      if (behavior != Ioss::DB_MODIFY) {
        for (const auto &set : nodesets) {
          Ioex::set_id(set, &ids_);
        }

        for (const auto &set : nodesets) {
          Ioex::get_id(set, &ids_);
        }
      }
      m_groupCount[EX_NODE_SET] = nodesets.size();
    }

    // EdgeSets ...
    {
      const Ioss::EdgeSetContainer &edgesets = region->get_edgesets();
      if (behavior != Ioss::DB_MODIFY) {
        for (const auto &set : edgesets) {
          Ioex::set_id(set, &ids_);
        }

        for (const auto &set : edgesets) {
          Ioex::get_id(set, &ids_);
        }
      }
      m_groupCount[EX_EDGE_SET] = edgesets.size();
    }

    // FaceSets ...
    {
      const Ioss::FaceSetContainer &facesets = region->get_facesets();
      if (behavior != Ioss::DB_MODIFY) {
        for (const auto &set : facesets) {
          Ioex::set_id(set, &ids_);
        }

        for (const auto &set : facesets) {
          Ioex::get_id(set, &ids_);
        }
      }
      m_groupCount[EX_FACE_SET] = facesets.size();
    }

    // ElementSets ...
    {
      const Ioss::ElementSetContainer &elementsets = region->get_elementsets();
      if (behavior != Ioss::DB_MODIFY) {
        for (const auto &set : elementsets) {
          Ioex::set_id(set, &ids_);
        }

        for (const auto &set : elementsets) {
          Ioex::get_id(set, &ids_);
        }
      }
      m_groupCount[EX_ELEM_SET] = elementsets.size();
    }

    // SideSets ...
    {
      const Ioss::SideSetContainer &ssets = region->get_sidesets();
      if (behavior != Ioss::DB_MODIFY) {
        for (const auto &set : ssets) {
          Ioex::set_id(set, &ids_);
        }
      }
      // Get entity counts for all face sets... Create SideSets.
      for (const auto &set : ssets) {
        if (behavior != Ioss::DB_MODIFY) {
          Ioex::get_id(set, &ids_);
        }
        int64_t id           = set->get_property("id").get_int();
        int64_t entity_count = 0;
        int64_t df_count     = 0;

        const Ioss::SideBlockContainer &side_blocks = set->get_side_blocks();
        for (const auto &block : side_blocks) {
          // Add  "*_offset" properties to specify at what offset
          // the data for this block appears in the containing set.
          auto *new_block = const_cast<Ioss::SideBlock *>(block);
          new_block->property_add(Ioss::Property("set_offset", entity_count));
          new_block->property_add(Ioss::Property("set_df_offset", df_count));

          // If combining sideblocks into sidesets on output, then
          // the id of the sideblock must be the same as the sideset
          // id.
          new_block->property_update("id", id);
          new_block->property_update("guid", util().generate_guid(id));

          entity_count += block->entity_count();
          df_count += block->get_property("distribution_factor_count").get_int();
        }
        auto *new_entity = const_cast<Ioss::SideSet *>(set);
        new_entity->property_add(Ioss::Property("entity_count", entity_count));
        new_entity->property_add(Ioss::Property("distribution_factor_count", df_count));
      }
      m_groupCount[EX_SIDE_SET] = ssets.size();
    }
  }

  void BaseDatabaseIO::output_other_metadata()
  {
    // Write attribute names (if any)...
    write_attribute_names(get_file_pointer(), EX_NODE_SET, get_region()->get_nodesets());
    write_attribute_names(get_file_pointer(), EX_EDGE_SET, get_region()->get_edgesets());
    write_attribute_names(get_file_pointer(), EX_FACE_SET, get_region()->get_facesets());
    write_attribute_names(get_file_pointer(), EX_ELEM_SET, get_region()->get_elementsets());
    write_attribute_names(get_file_pointer(), EX_NODE_BLOCK, get_region()->get_node_blocks());
    write_attribute_names(get_file_pointer(), EX_EDGE_BLOCK, get_region()->get_edge_blocks());
    write_attribute_names(get_file_pointer(), EX_FACE_BLOCK, get_region()->get_face_blocks());
    write_attribute_names(get_file_pointer(), EX_ELEM_BLOCK, get_region()->get_element_blocks());
    write_attribute_names(get_file_pointer(), EX_ASSEMBLY, get_region()->get_assemblies());
    write_attribute_names(get_file_pointer(), EX_BLOB, get_region()->get_blobs());

    // Write "reduction" attributes...
    std::vector<Ioss::Region *> regions;
    regions.push_back(get_region());
    Ioex::write_reduction_attributes(get_file_pointer(), regions);
    Ioex::write_reduction_attributes(get_file_pointer(), get_region()->get_nodesets());
    Ioex::write_reduction_attributes(get_file_pointer(), get_region()->get_nodesets());
    Ioex::write_reduction_attributes(get_file_pointer(), get_region()->get_edgesets());
    Ioex::write_reduction_attributes(get_file_pointer(), get_region()->get_facesets());
    Ioex::write_reduction_attributes(get_file_pointer(), get_region()->get_elementsets());
    Ioex::write_reduction_attributes(get_file_pointer(), get_region()->get_node_blocks());
    Ioex::write_reduction_attributes(get_file_pointer(), get_region()->get_edge_blocks());
    Ioex::write_reduction_attributes(get_file_pointer(), get_region()->get_face_blocks());
    Ioex::write_reduction_attributes(get_file_pointer(), get_region()->get_element_blocks());
    Ioex::write_reduction_attributes(get_file_pointer(), get_region()->get_assemblies());
    Ioex::write_reduction_attributes(get_file_pointer(), get_region()->get_blobs());

    // Write coordinate names...
    if (!get_region()->get_node_blocks().empty()) {
      char const *labels[3];
      labels[0] = "x";
      labels[1] = "y";
      labels[2] = "z";
      int ierr  = ex_put_coord_names(get_file_pointer(), (char **)labels);
      if (ierr < 0) {
        Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
      }
    }

    // Determine number of node, element maps (client-specified)
    // Set the index/order of the maps for later output.
    // Note that some fields have more than a single component and each component maps to a
    // different map
    size_t node_map_cnt = 0;
    if (get_region()->get_property("node_block_count").get_int() > 0) {
      auto *node_block      = get_region()->get_node_blocks()[0];
      auto  node_map_fields = node_block->field_describe(Ioss::Field::MAP);
      for (const auto &field_name : node_map_fields) {
        const auto &field = node_block->get_fieldref(field_name);
        if (field.get_index() == 0) {
          field.set_index(node_map_cnt + 1);
        }
        node_map_cnt += field.get_component_count(Ioss::Field::InOut::OUTPUT);
      }
    }

    Ioss::NameList elem_map_fields;
    const auto    &blocks = get_region()->get_element_blocks();
    for (const auto &block : blocks) {
      block->field_describe(Ioss::Field::MAP, &elem_map_fields);
    }

    Ioss::Utils::uniquify(elem_map_fields);

    // Now need to set the map index on any element map fields...
    // Note that not all blocks will potentially have all maps...
    size_t elem_map_cnt = 0;
    for (const auto &field_name : elem_map_fields) {
      int comp_count = 0;
      for (const auto &block : blocks) {
        if (block->field_exists(field_name)) {
          auto &field = block->get_fieldref(field_name);
          if (field.get_index() == 0) {
            field.set_index(elem_map_cnt + 1);
          }
          // Assumes all maps of a type have same component count
          comp_count = field.get_component_count(Ioss::Field::InOut::OUTPUT);
        }
      }
      elem_map_cnt += comp_count;
    }

    int ierr = ex_put_map_param(get_file_pointer(), node_map_cnt, elem_map_cnt);
    if (ierr < 0) {
      Ioex::exodus_error(get_file_pointer(), __LINE__, __func__, __FILE__);
    }

    if (node_map_cnt > 0) {
      char **names = Ioex::get_name_array(node_map_cnt, maximumNameLength);
      auto  *node_block =
          get_region()->get_node_blocks()[0]; // If there are node_maps, then there is a node_block
      auto node_map_fields = node_block->field_describe(Ioss::Field::MAP);
      for (const auto &field_name : node_map_fields) {
        const auto &field           = node_block->get_fieldref(field_name);
        int         component_count = field.get_component_count(Ioss::Field::InOut::OUTPUT);
        if (component_count == 1) {
          Ioss::Utils::copy_string(names[field.get_index() - 1], field_name, maximumNameLength + 1);
        }
        else {
          for (int i = 0; i < component_count; i++) {
            auto name = fmt::format("{}:{}", field_name, i + 1);
            Ioss::Utils::copy_string(names[field.get_index() + i - 1], name, maximumNameLength + 1);
          }
        }
      }
      ex_put_names(get_file_pointer(), EX_NODE_MAP, names);
      Ioex::delete_name_array(names, node_map_cnt);
    }

    if (elem_map_cnt > 0) {
      char **names = Ioex::get_name_array(elem_map_cnt, maximumNameLength);
      for (const auto &field_name : elem_map_fields) {
        // Now, we need to find an element block that has this field...
        for (const auto &block : blocks) {
          if (block->field_exists(field_name)) {
            const auto &field           = block->get_fieldref(field_name);
            int         component_count = field.get_component_count(Ioss::Field::InOut::OUTPUT);
            if (component_count == 1) {
              Ioss::Utils::copy_string(names[field.get_index() - 1], field_name,
                                       maximumNameLength + 1);
            }
            else {
              for (int i = 0; i < component_count; i++) {
                auto name = fmt::format("{}:{}", field_name, i + 1);
                if (field_name == "skin") {
                  name = i == 0 ? "skin:parent_element_id" : "skin:parent_element_side_number";
                }
                else if (field_name == "chain") {
                  name = i == 0 ? "chain:root_element_id" : "chain:depth_from_root";
                }
                Ioss::Utils::copy_string(names[field.get_index() + i - 1], name,
                                         maximumNameLength + 1);
              }
            }
            break;
          }
        }
      }
      ex_put_names(get_file_pointer(), EX_ELEM_MAP, names);
      Ioex::delete_name_array(names, elem_map_cnt);
    }

    // Write coordinate frame data...
    write_coordinate_frames(get_file_pointer(), get_region()->get_coordinate_frames());
  }
} // namespace Ioex

namespace {
  template <typename T>
  void write_attribute_names(int exoid, ex_entity_type type, const std::vector<T *> &entities)
  {
    // For the entity, determine the attribute fields and the correct
    // order. Write the names of these fields.  However, be aware that
    // the field "attribute" always exists to contain all attributes
    // and its name should not be used even if it is the only
    // attribute field.
    for (const auto &ge : entities) {
      int attribute_count = ge->get_property("attribute_count").get_int();
      if (attribute_count > 0) {

        check_attribute_index_order(ge);

        std::vector<char *> names(attribute_count);
        Ioss::NameList      names_str(attribute_count);

        // Get the attribute fields...
        Ioss::NameList results_fields = ge->field_describe(Ioss::Field::ATTRIBUTE);

        for (const auto &field_name : results_fields) {
          const Ioss::Field &field = ge->get_fieldref(field_name);
          assert(field.get_index() != 0);

          if (field_name == "attribute") {
            field.set_index(1);
            continue;
          }

          int comp_count   = field.get_component_count(Ioss::Field::InOut::OUTPUT);
          int field_offset = field.get_index();
          for (int i = 0; i < comp_count; i++) {
            names_str[field_offset - 1 + i] =
                ge->get_database()->get_component_name(field, Ioss::Field::InOut::OUTPUT, i + 1);
            names[field_offset - 1 + i] =
                const_cast<char *>(names_str[field_offset - 1 + i].c_str());
          }
        }
        size_t ge_id = ge->get_property("id").get_int();
        int    ierr  = ex_put_attr_names(exoid, type, ge_id, Data(names));
        if (ierr < 0) {
          Ioex::exodus_error(exoid, __LINE__, __func__, __FILE__);
        }
      }
    }
  }

  // common
  void check_attribute_index_order(Ioss::GroupingEntity *block)
  {
    int attribute_count = block->get_property("attribute_count").get_int();
    if (attribute_count == 0) {
      return;
    }
    int component_sum = 0;

    std::vector<int> attributes(attribute_count + 1);

    // Get the attribute fields...
    Ioss::NameList results_fields = block->field_describe(Ioss::Field::ATTRIBUTE);

    bool all_attributes_indexed  = true;
    bool some_attributes_indexed = false;

    for (const auto &field_name : results_fields) {
      const Ioss::Field &field = block->get_fieldref(field_name);

      if (field_name == "attribute") {
        field.set_index(1);
        if (results_fields.size() == 1) {
          return;
        }
        continue;
      }

      int field_offset = field.get_index();
      if (field_offset == 0) {
        all_attributes_indexed = false;
      }
      else {
        some_attributes_indexed = true;
      }

      int comp_count = field.get_component_count(Ioss::Field::InOut::OUTPUT);
      component_sum += comp_count;

      if (field_offset == 0) {
        continue;
      }

      if (field_offset + comp_count - 1 > attribute_count) {
        std::ostringstream errmsg;
        fmt::print(
            errmsg,
            "INTERNAL ERROR: For block '{}', attribute '{}', the indexing is incorrect.\n"
            "Something is wrong in the Ioex::BaseDatabaseIO class, function {}. Please report.\n",
            block->name(), field_name, __func__);
        IOSS_ERROR(errmsg);
      }

      for (int i = field_offset; i < field_offset + comp_count; i++) {
        if (attributes[i] != 0) {
          std::ostringstream errmsg;
          fmt::print(
              errmsg,
              "INTERNAL ERROR: For block '{}', attribute '{}', indexes into the same location as a "
              "previous attribute.\n"
              "Something is wrong in the Ioex::BaseDatabaseIO class, function {}. Please report.\n",
              block->name(), field_name, __func__);
          IOSS_ERROR(errmsg);
        }
        attributes[i] = 1;
      }
    }

    if (component_sum > attribute_count) {
      std::ostringstream errmsg;
      fmt::print(
          errmsg,
          "INTERNAL ERROR: Block '{}' is supposed to have {} attributes, but {} attributes "
          "were counted.\n"
          "Something is wrong in the Ioex::BaseDatabaseIO class, function {}. Please report.\n",
          block->name(), attribute_count, component_sum, __func__);
      IOSS_ERROR(errmsg);
    }

    // Take care of the easy cases first...
    if (all_attributes_indexed) {
      // Check that all attributes are defined.  This should have
      // caught above in the duplicate index check.
      for (int i = 1; i <= attribute_count; i++) {
        if (attributes[i] == 0) {
          std::ostringstream errmsg;
          fmt::print(
              errmsg,
              "INTERNAL ERROR: Block '{}' has an incomplete set of attributes.\n"
              "Something is wrong in the Ioex::BaseDatabaseIO class, function {}. Please report.\n",
              block->name(), __func__);
          IOSS_ERROR(errmsg);
        }
      }
      return;
    }

    if (!some_attributes_indexed) {
      // Index was not set for any of the attributes; set them all...
      size_t offset = 1;
      for (const auto &field_name : results_fields) {
        const Ioss::Field &field = block->get_fieldref(field_name);

        if (field_name == "attribute") {
          field.set_index(1);
          continue;
        }

        int comp_count = field.get_component_count(Ioss::Field::InOut::OUTPUT);

        assert(field.get_index() == 0);
        field.set_index(offset);
        offset += comp_count;
      }
      assert((int)offset == attribute_count + 1);
      return;
    }

    // At this point, we have a partially indexed set of attributes.  Some have an index and some
    // don't
    // The easy case is if the missing indices are at the end of the list...
    assert(!all_attributes_indexed && some_attributes_indexed);
    int last_defined = 0;
    for (int i = 1; i < attribute_count + 1; i++) {
      if (attributes[i] != 0) {
        last_defined = i;
      }
    }
    int first_undefined = attribute_count;
    for (int i = attribute_count; i > 0; i--) {
      if (attributes[i] == 0) {
        first_undefined = i;
      }
    }
    if (last_defined < first_undefined) {
      for (const auto &field_name : results_fields) {
        const Ioss::Field &field = block->get_fieldref(field_name);

        if (field_name == "attribute") {
          field.set_index(1);
          continue;
        }

        if (field.get_index() == 0) {
          field.set_index(first_undefined);
          int comp_count = field.get_component_count(Ioss::Field::InOut::OUTPUT);
          first_undefined += comp_count;
        }
      }
      assert(first_undefined == attribute_count + 1);
      return;
    }

    // Take the easy way out... Just reindex all attributes.
    size_t offset = 1;
    for (const auto &field_name : results_fields) {
      const Ioss::Field &field = block->get_fieldref(field_name);

      if (field_name == "attribute") {
        field.set_index(1);
        continue;
      }

      int comp_count = field.get_component_count(Ioss::Field::InOut::OUTPUT);

      assert(field.get_index() == 0);
      field.set_index(offset);
      offset += comp_count;
    }
    assert((int)offset == attribute_count + 1);
  }

  void check_variable_consistency(IOSS_MAYBE_UNUSED const ex_var_params &exo_params,
                                  IOSS_MAYBE_UNUSED int                  my_processor,
                                  IOSS_MAYBE_UNUSED const std::string &filename,
                                  IOSS_MAYBE_UNUSED const Ioss::ParallelUtils &util)
  {
    IOSS_PAR_UNUSED(exo_params);
    IOSS_PAR_UNUSED(my_processor);
    IOSS_PAR_UNUSED(filename);
    IOSS_PAR_UNUSED(util);
#ifdef SEACAS_HAVE_MPI
    const int        num_types = 10;
    std::vector<int> var_counts(num_types);
    var_counts[0] = exo_params.num_glob;
    var_counts[1] = exo_params.num_node;
    var_counts[2] = exo_params.num_edge;
    var_counts[3] = exo_params.num_face;
    var_counts[4] = exo_params.num_elem;
    var_counts[5] = exo_params.num_nset;
    var_counts[6] = exo_params.num_eset;
    var_counts[7] = exo_params.num_fset;
    var_counts[8] = exo_params.num_sset;
    var_counts[9] = exo_params.num_elset;

    Ioss::IntVector all_counts;
    util.gather(var_counts, all_counts);

    bool               any_diff = false;
    std::ostringstream errmsg;
    if (my_processor == 0) {
      bool diff[num_types];
      // See if any differ...
      for (int iv = 0; iv < 10; iv++) {
        diff[iv] = false;
        std::string type;
        switch (iv) {
        case 0: type = "global"; break;
        case 1: type = "nodal"; break;
        case 2: type = "edge"; break;
        case 3: type = "face"; break;
        case 4: type = "element"; break;
        case 5: type = "nodeset"; break;
        case 6: type = "edgeset"; break;
        case 7: type = "faceset"; break;
        case 8: type = "sideset"; break;
        case 9: type = "elementset"; break;
        }

        for (int ip = 1; ip < util.parallel_size(); ip++) {
          if (var_counts[iv] != all_counts[ip * num_types + iv]) {
            any_diff = true;
            if (!diff[iv]) {
              Ioss::FileInfo db(filename);
              diff[iv] = true;
              fmt::print(errmsg,
                         "\nERROR: Number of {} variables is not consistent on all processors.\n"
                         "       Database: '{}'\n"
                         "\tProcessor 0 count = {}\n",
                         type, db.tailname(), var_counts[iv]);
            }
            fmt::print(errmsg, "\tProcessor {} count = {}\n", ip, all_counts[ip * num_types + iv]);
          }
        }
      }
    }
    else {
      // Give the other processors something to say...
      fmt::print(errmsg, "ERROR: Variable type counts are inconsistent. See processor 0 output for "
                         "more details.\n");
    }
    int idiff = any_diff ? 1 : 0;
    util.broadcast(idiff);
    any_diff = idiff == 1;

    if (any_diff) {
      std::runtime_error x(errmsg.str());
      throw x;
    }
#endif
  }
} // namespace
