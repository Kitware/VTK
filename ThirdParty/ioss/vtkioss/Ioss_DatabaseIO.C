// Copyright(C) 1999-2022 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include <Ioss_BoundingBox.h>
#include <Ioss_CodeTypes.h>
#include <Ioss_CommSet.h>
#include <Ioss_DBUsage.h>
#include <Ioss_DatabaseIO.h>
#include <Ioss_ElementTopology.h>
#include <Ioss_EntityBlock.h>
#include <Ioss_Field.h>
#include <Ioss_FileInfo.h>
#include <Ioss_GroupingEntity.h>
#include <Ioss_NodeBlock.h>
#include <Ioss_ParallelUtils.h>
#include <Ioss_Property.h>
#include <Ioss_Region.h>
#include <Ioss_SerializeIO.h>
#include <Ioss_SideBlock.h>
#include <Ioss_SideSet.h>
#include <Ioss_Sort.h>
#include <Ioss_State.h>
#include <Ioss_StructuredBlock.h>
#include <Ioss_SurfaceSplit.h>
#include <Ioss_Utils.h>
#include <algorithm>
#include <cassert>
#include <cfloat>
#include <cstddef>
#include <cstring>
#include "vtk_fmt.h"
#include VTK_FMT(fmt/ostream.h)
#include <iomanip>
#include <iostream>
#include <iterator>
#include <set>
#include <string>
#include <tokenize.h>
#include <utility>
#include <vector>

#if defined SEACAS_HAVE_DATAWARP
extern "C" {
#include <datawarp.h>
}
#endif

namespace {
  auto initial_time = std::chrono::steady_clock::now();

  void log_time(std::chrono::time_point<std::chrono::steady_clock> &start,
                std::chrono::time_point<std::chrono::steady_clock> &finish, int current_state,
                double state_time, bool is_input, bool single_proc_only,
                const Ioss::ParallelUtils &util);

  void log_field(const char *symbol, const Ioss::GroupingEntity *entity, const Ioss::Field &field,
                 bool single_proc_only, const Ioss::ParallelUtils &util);

#ifndef NDEBUG
  bool internal_parallel_consistent(bool single_proc_only, const Ioss::GroupingEntity *ge,
                                    const Ioss::Field &field, int in_out,
                                    const Ioss::ParallelUtils &util)
  {
    if (single_proc_only) {
      return true;
    }

    const std::string &field_name = field.get_name();
    unsigned int       hash_code  = ge->hash() + Ioss::Utils::hash(field_name);
    unsigned int       max_hash   = util.global_minmax(hash_code, Ioss::ParallelUtils::DO_MAX);
    unsigned int       min_hash   = util.global_minmax(hash_code, Ioss::ParallelUtils::DO_MIN);
    if (max_hash != min_hash) {
      const std::string &ge_name = ge->name();
      fmt::print(Ioss::WARNING(),
                 "[{}] Parallel inconsistency detected for {} field '{}' on entity '{}'. (Hash: {} "
                 "{} {})\n",
                 in_out == 0 ? "writing" : "reading", util.parallel_rank(), field_name, ge_name,
                 hash_code, min_hash, max_hash);
      return false;
    }
    return true;
  }
#endif
  double my_min(double x1, double x2) { return x1 < x2 ? x1 : x2; }

  double my_max(double x1, double x2) { return x1 > x2 ? x1 : x2; }

  template <typename INT>
  void calc_bounding_box(size_t ndim, size_t node_count, std::vector<double> &coordinates,
                         std::vector<INT> &connectivity, double &xmin, double &ymin, double &zmin,
                         double &xmax, double &ymax, double &zmax)
  {
    std::vector<int> elem_block_nodes(node_count);
    for (auto &node : connectivity) {
      elem_block_nodes[node - 1] = 1;
    }

    xmin = DBL_MAX;
    ymin = DBL_MAX;
    zmin = DBL_MAX;

    xmax = -DBL_MAX;
    ymax = -DBL_MAX;
    zmax = -DBL_MAX;

    for (size_t i = 0; i < node_count; i++) {
      if (elem_block_nodes[i] == 1) {
        xmin = my_min(xmin, coordinates[ndim * i + 0]);
        xmax = my_max(xmax, coordinates[ndim * i + 0]);

        if (ndim > 1) {
          ymin = my_min(ymin, coordinates[ndim * i + 1]);
          ymax = my_max(ymax, coordinates[ndim * i + 1]);
        }

        if (ndim > 2) {
          zmin = my_min(zmin, coordinates[ndim * i + 2]);
          zmax = my_max(zmax, coordinates[ndim * i + 2]);
        }
      }
    }
    if (ndim < 3) {
      zmin = zmax = 0.0;
    }
    if (ndim < 2) {
      ymin = ymax = 0.0;
    }
  }

  void calc_bounding_box(size_t ndim, size_t node_count, std::vector<double> &coordinates,
                         double &xmin, double &ymin, double &zmin, double &xmax, double &ymax,
                         double &zmax)
  {
    xmin = DBL_MAX;
    ymin = DBL_MAX;
    zmin = DBL_MAX;

    xmax = -DBL_MAX;
    ymax = -DBL_MAX;
    zmax = -DBL_MAX;

    for (size_t i = 0; i < node_count; i++) {
      xmin = my_min(xmin, coordinates[ndim * i + 0]);
      xmax = my_max(xmax, coordinates[ndim * i + 0]);

      if (ndim > 1) {
        ymin = my_min(ymin, coordinates[ndim * i + 1]);
        ymax = my_max(ymax, coordinates[ndim * i + 1]);
      }

      if (ndim > 2) {
        zmin = my_min(zmin, coordinates[ndim * i + 2]);
        zmax = my_max(zmax, coordinates[ndim * i + 2]);
      }
    }
    if (ndim < 3) {
      zmin = zmax = 0.0;
    }
    if (ndim < 2) {
      ymin = ymax = 0.0;
    }
  }
} // namespace

namespace Ioss {
  DatabaseIO::DatabaseIO(Region *region, std::string filename, DatabaseUsage db_usage,
                         MPI_Comm communicator, const PropertyManager &props)
      : properties(props), DBFilename(std::move(filename)), dbUsage(db_usage),
        util_(db_usage == WRITE_HISTORY || db_usage == WRITE_HEARTBEAT ? MPI_COMM_SELF
                                                                       : communicator),
        region_(region), isInput(is_input_event(db_usage)),
        singleProcOnly(db_usage == WRITE_HISTORY || db_usage == WRITE_HEARTBEAT ||
                       SerializeIO::isEnabled())
  {
    isParallel  = util_.parallel_size() > 1;
    myProcessor = util_.parallel_rank();

    // Some operations modify DBFilename and there is a need to get
    // back to the original filename...
    originalDBFilename = DBFilename;

    // Check environment variable IOSS_PROPERTIES. If it exists, parse
    // the contents and add to the 'properties' map.
    util_.add_environment_properties(properties);

    Utils::check_set_bool_property(properties, "ENABLE_FIELD_RECOGNITION", enableFieldRecognition);

    if (properties.exists("FIELD_SUFFIX_SEPARATOR")) {
      std::string tmp         = properties.get("FIELD_SUFFIX_SEPARATOR").get_string();
      fieldSeparator          = tmp[0];
      fieldSeparatorSpecified = true;
    }

    // If `FIELD_SUFFIX_SEPARATOR` is empty and there are fields that
    // end with an underscore, then strip the underscore. This will
    // cause d_x, d_y, d_z to be a 3-component field 'd' and vx, vy,
    // vz to be a 3-component field 'v'.
    Utils::check_set_bool_property(properties, "FIELD_STRIP_TRAILING_UNDERSCORE",
                                   fieldStripTrailing_);

    if (properties.exists("INTEGER_SIZE_API")) {
      int isize = properties.get("INTEGER_SIZE_API").get_int();
      if (isize == 8) {
        set_int_byte_size_api(Ioss::USE_INT64_API);
      }
    }

    if (properties.exists("SERIALIZE_IO")) {
      int isize = properties.get("SERIALIZE_IO").get_int();
      Ioss::SerializeIO::setGroupFactor(isize);
      if (isize > 0) {
        singleProcOnly = true;
      }
    }

    if (properties.exists("CYCLE_COUNT")) {
      cycleCount = properties.get("CYCLE_COUNT").get_int();
    }

    if (properties.exists("OVERLAY_COUNT")) {
      overlayCount = properties.get("OVERLAY_COUNT").get_int();
    }

    Utils::check_set_bool_property(properties, "ENABLE_TRACING", m_enableTracing);
    Utils::check_set_bool_property(properties, "TIME_STATE_INPUT_OUTPUT", m_timeStateInOut);
    {
      bool logging;
      if (Utils::check_set_bool_property(properties, "LOGGING", logging)) {
        set_logging(logging);
      }
    }

    Utils::check_set_bool_property(properties, "LOWER_CASE_VARIABLE_NAMES", lowerCaseVariableNames);
    Utils::check_set_bool_property(properties, "USE_GENERIC_CANONICAL_NAMES",
                                   useGenericCanonicalName);
    Utils::check_set_bool_property(properties, "IGNORE_DATABASE_NAMES", ignoreDatabaseNames);

    {
      bool consistent;
      if (Utils::check_set_bool_property(properties, "PARALLEL_CONSISTENCY", consistent)) {
        set_parallel_consistency(consistent);
      }
    }

    check_setDW();

    if (!is_input()) {
      // Create full path to the output file at this point if it doesn't
      // exist...
      if (isParallel) {
        Ioss::FileInfo::create_path(DBFilename, util().communicator());
      }
      else {
        Ioss::FileInfo::create_path(DBFilename);
      }
    }
  }

  DatabaseIO::~DatabaseIO() = default;

  int DatabaseIO::int_byte_size_api() const
  {
    if (dbIntSizeAPI == USE_INT32_API) {
      return 4;
    }
    return 8;
  }

  /** \brief Set the number of bytes used to represent an integer.
   *
   *  \param[in] size The number of bytes. This is 4 for INT32 or 8 for INT64.
   */
  void DatabaseIO::set_int_byte_size_api(DataSize size) const
  {
    dbIntSizeAPI = size; // mutable
  }

  /** \brief Set the character used to separate a field suffix from the field basename
   *         when recognizing vector, tensor fields.
   *
   *  \param[in] separator The separator character.
   */
  void DatabaseIO::set_field_separator(const char separator)
  {
    if (properties.exists("FIELD_SUFFIX_SEPARATOR")) {
      properties.erase("FIELD_SUFFIX_SEPARATOR");
    }
    char tmp[2] = {separator, '\0'};
    properties.add(Property("FIELD_SUFFIX_SEPARATOR", tmp));
    fieldSeparator          = separator;
    fieldSeparatorSpecified = true;
  }

  std::string DatabaseIO::get_component_name(const Ioss::Field &field, Ioss::Field::InOut in_out,
                                             int component) const
  {
    // If the user has explicitly set the suffix separator for this database,
    // then use it for all fields.
    // If it was not explicity set, then use whatever the field has defined,
    // of if field also has nothing explicitly set, use '_'
    char suffix = fieldSeparatorSpecified ? get_field_separator() : 1;
    return field.get_component_name(component, in_out, suffix);
  }

  /**
   * Check whether user wants to use Cray DataWarp.  It will be enabled if:
   * the `DW_JOB_STRIPED` or `DW_JOB_PRIVATE` environment variable
   * is set by the queuing system during runtime and the IOSS property
   * `ENABLE_DATAWARP` set to `YES`.
   *
   * We currently only want output files to be directed to BB.
   */
  void DatabaseIO::check_setDW() const
  {
    if (!is_input()) {
      bool set_dw = false;
      Utils::check_set_bool_property(properties, "ENABLE_DATAWARP", set_dw);
      if (set_dw) {
        std::string bb_path;
        // Selected via `#DW jobdw type=scratch access_mode=striped`
        util().get_environment("DW_JOB_STRIPED", bb_path, isParallel);

        if (bb_path.empty()) { // See if using `private` mode...
          // Selected via `#DW jobdw type=scratch access_mode=private`
          util().get_environment("DW_JOB_PRIVATE", bb_path, isParallel);
        }
        if (!bb_path.empty()) {
          usingDataWarp = true;
          dwPath        = bb_path;
          if (myProcessor == 0) {
            fmt::print(Ioss::OUTPUT(), "\nDataWarp Burst Buffer Enabled.  Path = `{}`\n\n", dwPath);
          }
        }
        else {
          if (myProcessor == 0) {
            fmt::print(Ioss::WARNING(),
                       "DataWarp enabled via Ioss property `ENABLE_DATAWARP`, but\n"
                       "         burst buffer path was not specified via `DW_JOB_STRIPED` or "
                       "`DW_JOB_PRIVATE`\n"
                       "         environment variables (typically set by queuing system)\n"
                       "         DataWarp will *NOT* be enabled, but job will still run.\n\n");
          }
        }
      }
    }
  }

  /**
   * In this wrapper function we check if user intends to use Cray
   * DataWarp(aka DW), which provides ability to use NVMe based flash
   * storage available across all compute nodes accessible via high
   * speed NIC.
   */
  void DatabaseIO::openDW(const std::string &filename) const
  {
    set_pfsname(filename); // Name on permanent-file-store
    if (using_dw()) {      // We are about to write to a output database in BB
      Ioss::FileInfo path{filename};
      Ioss::FileInfo bb_file{get_dwPath() + path.tailname()};
      if (bb_file.exists() && !bb_file.is_writable()) {
        // already existing file which has been closed If we can't
        // write to the file on the BB, then it is a file which is
        // being staged by datawarp system over to the permanent
        // filesystem.  Wait until staging has finished...  stage wait
        // returns 0 = success, -ENOENT or -errno
#if defined SEACAS_HAVE_DATAWARP
#if IOSS_DEBUG_OUTPUT
        if (myProcessor == 0) {
          fmt::print(Ioss::DEBUG(), "DW: dw_wait_file_stage({});\n", bb_file.filename());
        }
#endif
        int dwret = dw_wait_file_stage(bb_file.filename().c_str());
        if (dwret < 0) {
          std::ostringstream errmsg;
          fmt::print(errmsg, "ERROR: failed waiting for file stage `{}`: {}\n", bb_file.filename(),
                     std::strerror(-dwret));
          IOSS_ERROR(errmsg);
        }
#else
        // Used to debug DataWarp logic on systems without DataWarp...
        fmt::print(Ioss::DEBUG(), "DW: (FAKE) dw_wait_file_stage({});\n", bb_file.filename());
#endif
      }
      set_dwname(bb_file.filename());
    }
    else {
      set_dwname(filename);
    }
  }

  /** \brief This function gets called inside closeDatabase__(), which checks if Cray Datawarp (DW)
   * is in use, if so, we want to call a stageout before actual close of this file.
   */
  void DatabaseIO::closeDW() const
  {
    if (using_dw()) {
      if (!using_parallel_io() || myProcessor == 0) {
#if defined SEACAS_HAVE_DATAWARP
        int complete = 0, pending = 0, deferred = 0, failed = 0;
        dw_query_file_stage(get_dwname().c_str(), &complete, &pending, &deferred, &failed);
#if IOSS_DEBUG_OUTPUT
        auto initial = std::chrono::steady_clock::now();
        fmt::print(Ioss::DEBUG(), "Query: {}, {}, {}, {}\n", complete, pending, deferred, failed);
#endif
        if (pending > 0) {
          int dwret = dw_wait_file_stage(get_dwname().c_str());
          if (dwret < 0) {
            std::ostringstream errmsg;
            fmt::print(errmsg, "ERROR: failed waiting for file stage `{}`: {}\n", get_dwname(),
                       std::strerror(-dwret));
            IOSS_ERROR(errmsg);
          }
#if IOSS_DEBUG_OUTPUT
          dw_query_file_stage(get_dwname().c_str(), &complete, &pending, &deferred, &failed);
          fmt::print(Ioss::DEBUG(), "Query: {}, {}, {}, {}\n", complete, pending, deferred, failed);
#endif
        }

#if IOSS_DEBUG_OUTPUT
        fmt::print(Ioss::DEBUG(), "\nDW: BEGIN dw_stage_file_out({}, {}, DW_STAGE_IMMEDIATE);\n",
                   get_dwname(), get_pfsname());
#endif
        int ret =
            dw_stage_file_out(get_dwname().c_str(), get_pfsname().c_str(), DW_STAGE_IMMEDIATE);

#if IOSS_DEBUG_OUTPUT
        auto                          time_now = std::chrono::steady_clock::now();
        std::chrono::duration<double> diff     = time_now - initial;
        fmt::print(Ioss::DEBUG(), "\nDW: END dw_stage_file_out({})\n", diff.count());
#endif
        if (ret < 0) {
          std::ostringstream errmsg;
          fmt::print(errmsg, "ERROR: file staging of `{}` to `{}` failed at close: {}\n",
                     get_dwname(), get_pfsname(), std::strerror(-ret));
          IOSS_ERROR(errmsg);
        }
#else
        fmt::print(Ioss::DEBUG(), "\nDW: (FAKE) dw_stage_file_out({}, {}, DW_STAGE_IMMEDIATE);\n",
                   get_dwname(), get_pfsname());
#endif
      }
      if (using_parallel_io()) {
        util().barrier();
      }
    }
  }

  void DatabaseIO::openDatabase__() const { openDW(get_filename()); }

  void DatabaseIO::closeDatabase__() const { closeDW(); }

  IfDatabaseExistsBehavior DatabaseIO::open_create_behavior() const
  {
    IfDatabaseExistsBehavior exists = DB_OVERWRITE;
    if (properties.exists("APPEND_OUTPUT")) {
      exists = static_cast<IfDatabaseExistsBehavior>(properties.get("APPEND_OUTPUT").get_int());
    }
    return exists;
  }

  const std::string &DatabaseIO::decoded_filename() const
  {
    if (decodedFilename.empty()) {
      if (isParallel) {
        decodedFilename = util().decode_filename(get_filename(), isParallel && !usingParallelIO);
      }
      else if (properties.exists("processor_count") && properties.exists("my_processor")) {
        int proc_count  = properties.get("processor_count").get_int();
        int my_proc     = properties.get("my_processor").get_int();
        decodedFilename = Ioss::Utils::decode_filename(get_filename(), my_proc, proc_count);
      }
      else {
        decodedFilename = get_filename();
      }

      openDW(decodedFilename);
      if (using_dw()) {
        // Note that if using_dw(), then we need to set the decodedFilename to the BB name.
        decodedFilename = get_dwname();
      }
    }
    return decodedFilename;
  }

  void DatabaseIO::verify_and_log(const GroupingEntity *ge, const Field &field, int in_out) const
  {
    if (ge != nullptr) {
      assert(!is_parallel_consistent() ||
             internal_parallel_consistent(singleProcOnly, ge, field, in_out, util_));
    }
    if (get_logging()) {
      log_field(in_out == 1 ? ">" : "<", ge, field, singleProcOnly, util_);
    }
  }

  bool DatabaseIO::begin_state(int state, double time)
  {
    IOSS_FUNC_ENTER(m_);
    if (m_timeStateInOut) {
      m_stateStart = std::chrono::steady_clock::now();
    }
    return begin_state__(state, time);
  }
  bool DatabaseIO::end_state(int state, double time)
  {
    IOSS_FUNC_ENTER(m_);
    bool res = end_state__(state, time);
    if (m_timeStateInOut) {
      auto finish = std::chrono::steady_clock::now();
      log_time(m_stateStart, finish, state, time, is_input(), singleProcOnly, util_);
    }
    return res;
  }

  // Default versions do nothing...
  bool DatabaseIO::begin_state__(int /* state */, double /* time */) { return true; }

  bool DatabaseIO::end_state__(int /* state */, double /* time */) { return true; }

  void DatabaseIO::handle_groups()
  {
    // Set Grouping requests are specified as properties...
    // See if the property exists and decode...
    // There is a property for each "type":
    // GROUP_SIDESET, GROUP_NODESET, GROUP_EDGESET, GROUP_FACESET,
    // GROUP_ELEMSET.
    // Within the property, the "value" consists of multiple groups separated by
    // ":"
    // Within the group, the names are "," separated:
    //
    // new_surf1,member1,member2,member3:new_surf2,mem1,mem2,mem3,mem4:new_surf3,....
    //
    // Currently does not check for duplicate entity membership in a set --
    // union
    // with duplicates
    //
    create_groups("GROUP_SIDESET", SIDESET, "side", (SideSet *)nullptr);
    create_groups("GROUP_NODESET", NODESET, "node", (NodeSet *)nullptr);
    create_groups("GROUP_EDGESET", EDGESET, "edge", (EdgeSet *)nullptr);
    create_groups("GROUP_FACESET", FACESET, "face", (FaceSet *)nullptr);
    create_groups("GROUP_ELEMSET", ELEMENTSET, "elem", (ElementSet *)nullptr);
  }

  template <typename T>
  void DatabaseIO::create_groups(const std::string &property_name, EntityType type,
                                 const std::string &type_name, const T *set_type)
  {
    if (!properties.exists(property_name)) {
      return;
    }

    std::string              prop   = properties.get(property_name).get_string();
    std::vector<std::string> groups = tokenize(prop, ":");
    for (auto &group : groups) {
      std::vector<std::string> group_spec = tokenize(group, ",");

      // group_spec should contain the name of the new group as
      // the first location and the members of the group as subsequent
      // locations.  OK to have a single member
      if (group_spec.size() < 2) {
        std::ostringstream errmsg;
        fmt::print(errmsg,
                   "ERROR: Invalid {} group specification '{}'\n"
                   "       Correct syntax is 'new_group,member1,...,memberN' and there must "
                   "       be at least 1 member of the group",
                   type_name, group);
        IOSS_ERROR(errmsg);
      }

      create_group(type, type_name, group_spec, set_type);
    }
  }

  template <typename T>
  void DatabaseIO::create_group(EntityType /*type*/, const std::string &type_name,
                                const std::vector<std::string> &group_spec, const T * /*set_type*/)
  {
    fmt::print(Ioss::WARNING(),
               "Grouping of {0} sets is not yet implemented.\n"
               "         Skipping the creation of {0} set '{1}'\n\n",
               type_name, group_spec[0]);
  }

  template <>
  void DatabaseIO::create_group(EntityType type, const std::string & /*type_name*/,
                                const std::vector<std::string> &group_spec,
                                const SideSet * /*set_type*/)
  {
    // Not generalized yet... This only works for T == SideSet
    if (type != SIDESET) {
      return;
    }

    int64_t entity_count = 0;
    int64_t df_count     = 0;

    // Create the new set...
    auto new_set = new SideSet(this, group_spec[0]);

    get_region()->add(new_set);

    // Find the member SideSets...
    for (size_t i = 1; i < group_spec.size(); i++) {
      SideSet *set = get_region()->get_sideset(group_spec[i]);
      if (set != nullptr) {
        const SideBlockContainer &side_blocks = set->get_side_blocks();
        for (auto &sbold : side_blocks) {
          size_t  side_count = sbold->entity_count();
          auto    sbnew      = new SideBlock(this, sbold->name(), sbold->topology()->name(),
                                             sbold->parent_element_topology()->name(), side_count);
          int64_t id         = sbold->get_property("id").get_int();
          sbnew->property_add(Property("set_offset", entity_count));
          sbnew->property_add(Property("set_df_offset", df_count));
          sbnew->property_add(Property("id", id));
          sbnew->property_add(Property("id", id));
          sbnew->property_add(Property("guid", util().generate_guid(id)));

          new_set->add(sbnew);

          size_t old_df_count = sbold->get_property("distribution_factor_count").get_int();
          if (old_df_count > 0) {
            std::string storage = "Real[";
            storage += std::to_string(sbnew->topology()->number_nodes());
            storage += "]";
            sbnew->field_add(
                Field("distribution_factors", Field::REAL, storage, Field::MESH, side_count));
          }
          entity_count += side_count;
          df_count += old_df_count;
        }
      }
      else {
        fmt::print(Ioss::WARNING(),
                   "While creating the grouped surface '{}', the surface '{}' does not exist. "
                   "This surface will skipped and not added to the group.\n\n",
                   group_spec[0], group_spec[i]);
      }
    }
  }

  // Utility function that may be used by derived classes.  Determines
  // whether all elements in the model have the same face topology.
  // This can be used to speed-up certain algorithms since they don't
  // have to check each face (or group of faces) individually.
  void DatabaseIO::set_common_side_topology() const
  {
    auto *new_this = const_cast<DatabaseIO *>(this);

    bool                         first          = true;
    const ElementBlockContainer &element_blocks = get_region()->get_element_blocks();
    for (auto block : element_blocks) {
      size_t element_count = block->entity_count();

      // Check face types.
      if (element_count > 0) {
        if (commonSideTopology != nullptr || first) {
          first                      = false;
          ElementTopology *side_type = block->topology()->boundary_type();
          if (commonSideTopology == nullptr) { // First block
            new_this->commonSideTopology = side_type;
          }
          if (commonSideTopology != side_type) { // Face topologies differ in mesh
            new_this->commonSideTopology = nullptr;
            return;
          }
        }
      }
    }
  }

  /** \brief Add multiple information records (informative strings) to the database.
   *
   *  \param[in] info The strings to add.
   */
  void DatabaseIO::add_information_records(const std::vector<std::string> &info)
  {
    informationRecords.reserve(informationRecords.size() + info.size());
    informationRecords.insert(informationRecords.end(), info.begin(), info.end());
  }

  /** \brief Add an information record (an informative string) to the database.
   *
   *  \param[in] info The string to add.
   */
  void DatabaseIO::add_information_record(const std::string &info)
  {
    informationRecords.push_back(info);
  }

  /** \brief Add a QA record, which consists of 4 strings, to the database
   *
   *  The 4 function parameters correspond to the 4 QA record strings.
   *
   *  \param[in] code A descriptive code name, such as the application that modified the database.
   *  \param[in] code_qa A descriptive string, such as the version of the application that modified
   * the database.
   *  \param[in] date A relevant date, such as the date the database was modified.
   *  \param[in] time A relevant time, such as the time the database was modified.
   */
  void DatabaseIO::add_qa_record(const std::string &code, const std::string &code_qa,
                                 const std::string &date, const std::string &time)
  {
    qaRecords.push_back(code);
    qaRecords.push_back(code_qa);
    qaRecords.push_back(date);
    qaRecords.push_back(time);
  }

  void DatabaseIO::set_block_omissions(const std::vector<std::string> &omissions,
                                       const std::vector<std::string> &inclusions)
  {
    if (!omissions.empty()) {
      blockOmissions.assign(omissions.cbegin(), omissions.cend());
      Ioss::sort(blockOmissions.begin(), blockOmissions.end());
    }
    if (!inclusions.empty()) {
      blockInclusions.assign(inclusions.cbegin(), inclusions.cend());
      Ioss::sort(blockInclusions.begin(), blockInclusions.end());
    }
  }

  // Check topology of all sides (face/edges) in model...
  void DatabaseIO::check_side_topology() const
  {
    // The following code creates the sideTopology sets which contain
    // a list of the side topologies in this model.
    //
    // If sideTopology.size() > 1 --> the model has sides with mixed
    // topology (i.e., quads and tris).
    //
    // If sideTopology.size() == 1 --> the model has homogeneous sides
    // and each side is of the topology type 'sideTopology[0]'
    //
    // This is used in other code speed up some tests.

    // Spheres and Circle have no faces/edges, so handle them special...

    if (sideTopology.empty()) {
      // Set contains (parent_element, boundary_topology) pairs...
      std::set<std::pair<const ElementTopology *, const ElementTopology *>> side_topo;

      const ElementBlockContainer &element_blocks = get_region()->get_element_blocks();

      bool all_sphere = true;
      for (auto &block : element_blocks) {
        const ElementTopology *elem_type = block->topology();
        const ElementTopology *side_type = elem_type->boundary_type();
        if (side_type == nullptr) {
          // heterogeneous sides.  Iterate through...
          int size = elem_type->number_boundaries();
          for (int i = 1; i <= size; i++) {
            side_type = elem_type->boundary_type(i);
            side_topo.insert(std::make_pair(elem_type, side_type));
            all_sphere = false;
          }
        }
        else {
          // homogeneous sides.
          side_topo.insert(std::make_pair(elem_type, side_type));
          all_sphere = false;
        }
      }
      if (all_sphere) {
        // If we end up here, the model either contains all spheres,
        // or there are no element blocks in the model...
        const ElementTopology *ftopo = ElementTopology::factory("unknown");
        if (element_blocks.empty()) {
          side_topo.insert(std::make_pair(ftopo, ftopo));
        }
        else {
          const ElementBlock *block = *element_blocks.cbegin();
          side_topo.insert(std::make_pair(block->topology(), ftopo));
        }
      }
      assert(!side_topo.empty());
      assert(sideTopology.empty());
      // Copy into the sideTopology container...
      auto *new_this = const_cast<DatabaseIO *>(this);
      std::copy(side_topo.cbegin(), side_topo.cend(), std::back_inserter(new_this->sideTopology));
    }
    assert(!sideTopology.empty());
  }

  void DatabaseIO::get_block_adjacencies__(const Ioss::ElementBlock *eb,
                                           std::vector<std::string> &block_adjacency) const
  {
    if (!blockAdjacenciesCalculated) {
      compute_block_adjacencies();
    }

    const Ioss::ElementBlockContainer &element_blocks = get_region()->get_element_blocks();
    assert(Ioss::Utils::check_block_order(element_blocks));

    // Extract the computed block adjacency information for this
    // element block:
    int blk_position = 0;
    if (eb->property_exists("original_block_order")) {
      blk_position = eb->get_property("original_block_order").get_int();
    }
    else {
      for (const auto &leb : element_blocks) {
        if (leb == eb) {
          break;
        }
        blk_position++;
      }
    }

    int lblk_position = -1;
    for (const auto &leb : element_blocks) {
      if (leb->property_exists("original_block_order")) {
        lblk_position = leb->get_property("original_block_order").get_int();
      }
      else {
        lblk_position++;
      }

      if (blk_position != lblk_position &&
          static_cast<int>(blockAdjacency[blk_position][lblk_position]) == 1) {
        block_adjacency.push_back(leb->name());
      }
    }
  }

  // common
  void DatabaseIO::compute_block_adjacencies() const
  {
    // Add a field to each element block specifying which other element
    // blocks the block is adjacent to (defined as sharing nodes).
    // This is only calculated on request...

    blockAdjacenciesCalculated = true;

    const Ioss::ElementBlockContainer &element_blocks = get_region()->get_element_blocks();
    assert(Ioss::Utils::check_block_order(element_blocks));

    if (element_blocks.size() == 1) {
      blockAdjacency.resize(1);
      blockAdjacency[0].resize(1);
      blockAdjacency[0][0] = false;
      return;
    }

    // Each processor first calculates the adjacencies on their own
    // processor...

    std::vector<int64_t>          node_used(nodeCount);
    std::vector<std::vector<int>> inv_con(nodeCount);

    {
      Ioss::SerializeIO serializeIO__(this);
      int               blk_position = -1;
      for (Ioss::ElementBlock *eb : element_blocks) {
        if (eb->property_exists("original_block_order")) {
          blk_position = eb->get_property("original_block_order").get_int();
        }
        else {
          blk_position++;
        }
        int64_t my_element_count = eb->entity_count();
        if (int_byte_size_api() == 8) {
          std::vector<int64_t> conn;
          eb->get_field_data("connectivity_raw", conn);
          for (auto &node : conn) {
            assert(node > 0 && node - 1 < nodeCount);
            node_used[node - 1] = blk_position + 1;
          }
        }
        else {
          std::vector<int> conn;
          eb->get_field_data("connectivity_raw", conn);
          for (auto &node : conn) {
            assert(node > 0 && node - 1 < nodeCount);
            node_used[node - 1] = blk_position + 1;
          }
        }

        if (my_element_count > 0) {
          for (int64_t i = 0; i < nodeCount; i++) {
            if (node_used[i] == blk_position + 1) {
              inv_con[i].push_back(blk_position);
            }
          }
        }
      }
    }

#ifdef SEACAS_HAVE_MPI
    if (isParallel) {
      // Get contributions from other processors...
      // Get the communication map...
      Ioss::CommSet *css = get_region()->get_commset("commset_node");
      Ioss::Utils::check_non_null(css, "communication map", "commset_node", __func__);
      std::vector<std::pair<int, int>> proc_node;
      {
        std::vector<int> entity_processor;
        css->get_field_data("entity_processor", entity_processor);
        proc_node.reserve(entity_processor.size() / 2);
        for (size_t i = 0; i < entity_processor.size(); i += 2) {
          proc_node.emplace_back(entity_processor[i + 1], entity_processor[i]);
        }
      }

      // Now sort by increasing processor number.
      Ioss::sort(proc_node.begin(), proc_node.end());

      // Pack the data: global_node_id, bits for each block, ...
      // Use 'int' as basic type...
      size_t                id_size   = 1;
      size_t                word_size = sizeof(int) * 8;
      size_t                bits_size = (element_blocks.size() + word_size - 1) / word_size;
      std::vector<unsigned> send(proc_node.size() * (id_size + bits_size));
      std::vector<unsigned> recv(proc_node.size() * (id_size + bits_size));

      std::vector<int> procs(util().parallel_size());
      size_t           offset = 0;
      for (const auto &pn : proc_node) {
        int64_t glob_id = pn.second;
        int     proc    = pn.first;
        procs[proc]++;
        send[offset++] = glob_id;
        int64_t loc_id = nodeMap.global_to_local(glob_id, true) - 1;
        for (int jblk : inv_con[loc_id]) {
          size_t wrd_off = jblk / word_size;
          size_t bit     = jblk % word_size;
          send[offset + wrd_off] |= (1 << bit);
        }
        offset += bits_size;
      }

      // Count nonzero entries in 'procs' array -- count of
      // sends/receives
      size_t non_zero = util().parallel_size() - std::count(procs.begin(), procs.end(), 0);

      // Post all receives...
      MPI_Request              request_null = MPI_REQUEST_NULL;
      std::vector<MPI_Request> request(non_zero, request_null);
      std::vector<MPI_Status>  status(non_zero);

      int    result  = MPI_SUCCESS;
      size_t req_cnt = 0;
      offset         = 0;
      for (int i = 0; result == MPI_SUCCESS && i < util().parallel_size(); i++) {
        if (procs[i] > 0) {
          const unsigned size     = procs[i] * (id_size + bits_size);
          void *const    recv_buf = &recv[offset];
          result = MPI_Irecv(recv_buf, size, MPI_INT, i, 10101, util().communicator(),
                             &request[req_cnt]);
          req_cnt++;
          offset += size;
        }
      }
      assert(result != MPI_SUCCESS || non_zero == req_cnt);

      if (result != MPI_SUCCESS) {
        std::ostringstream errmsg;
        fmt::print(errmsg, "ERROR: MPI_Irecv error on processor {} in {}", util().parallel_rank(),
                   __func__);
        IOSS_ERROR(errmsg);
      }

      int local_error  = (MPI_SUCCESS == result) ? 0 : 1;
      int global_error = util().global_minmax(local_error, Ioss::ParallelUtils::DO_MAX);

      if (global_error != 0) {
        std::ostringstream errmsg;
        fmt::print(errmsg, "ERROR: MPI_Irecv error on some processor in {}", __func__);
        IOSS_ERROR(errmsg);
      }

      result  = MPI_SUCCESS;
      req_cnt = 0;
      offset  = 0;
      for (int i = 0; result == MPI_SUCCESS && i < util().parallel_size(); i++) {
        if (procs[i] > 0) {
          const unsigned size     = procs[i] * (id_size + bits_size);
          void *const    send_buf = &send[offset];
          result = MPI_Rsend(send_buf, size, MPI_INT, i, 10101, util().communicator());
          req_cnt++;
          offset += size;
        }
      }
      assert(result != MPI_SUCCESS || non_zero == req_cnt);

      if (result != MPI_SUCCESS) {
        std::ostringstream errmsg;
        fmt::print(errmsg, "ERROR: MPI_Rsend error on processor {} in {}", util().parallel_rank(),
                   __func__);
        IOSS_ERROR(errmsg);
      }

      local_error  = (MPI_SUCCESS == result) ? 0 : 1;
      global_error = util().global_minmax(local_error, Ioss::ParallelUtils::DO_MAX);

      if (global_error != 0) {
        std::ostringstream errmsg;
        fmt::print(errmsg, "ERROR: MPI_Rsend error on some processor in {}", __func__);
        IOSS_ERROR(errmsg);
      }

      result = MPI_Waitall(req_cnt, request.data(), status.data());

      if (result != MPI_SUCCESS) {
        std::ostringstream errmsg;
        fmt::print(errmsg, "ERROR: MPI_Waitall error on processor {} in {}", util().parallel_rank(),
                   __func__);
        IOSS_ERROR(errmsg);
      }

      // Unpack the data and update the inv_con arrays for boundary
      // nodes...
      offset = 0;
      for (size_t i = 0; i < proc_node.size(); i++) {
        int64_t glob_id = recv[offset++];
        int64_t loc_id  = nodeMap.global_to_local(glob_id, true) - 1;
        for (size_t iblk = 0; iblk < element_blocks.size(); iblk++) {
          size_t wrd_off = iblk / word_size;
          size_t bit     = iblk % word_size;
          if (recv[offset + wrd_off] & (1 << bit)) {
            inv_con[loc_id].push_back(iblk); // May result in duplicates, but that is OK.
          }
        }
        offset += bits_size;
      }
    }
#endif

    // Convert from inv_con arrays to block adjacency...
    blockAdjacency.resize(element_blocks.size());
    for (auto &block : blockAdjacency) {
      block.resize(element_blocks.size());
    }

    for (int64_t i = 0; i < nodeCount; i++) {
      for (size_t j = 0; j < inv_con[i].size(); j++) {
        int jblk = inv_con[i][j];
        for (size_t k = j + 1; k < inv_con[i].size(); k++) {
          int kblk                   = inv_con[i][k];
          blockAdjacency[jblk][kblk] = true;
          blockAdjacency[kblk][jblk] = true;
        }
      }
    }

#ifdef SEACAS_HAVE_MPI
    if (isParallel) {
      // Sync across all processors...
      size_t word_size = sizeof(int) * 8;
      size_t bits_size = (element_blocks.size() + word_size - 1) / word_size;

      std::vector<unsigned> data(element_blocks.size() * bits_size);
      int64_t               offset = 0;
      for (size_t jblk = 0; jblk < element_blocks.size(); jblk++) {
        for (size_t iblk = 0; iblk < element_blocks.size(); iblk++) {
          if (blockAdjacency[jblk][iblk] == 1) {
            size_t wrd_off = iblk / word_size;
            size_t bit     = iblk % word_size;
            data[offset + wrd_off] |= (1 << bit);
          }
        }
        offset += bits_size;
      }

      std::vector<unsigned> out_data(element_blocks.size() * bits_size);
      MPI_Allreduce((void *)data.data(), out_data.data(), static_cast<int>(data.size()),
                    MPI_UNSIGNED, MPI_BOR, util().communicator());

      offset = 0;
      for (size_t jblk = 0; jblk < element_blocks.size(); jblk++) {
        for (size_t iblk = 0; iblk < element_blocks.size(); iblk++) {
          if (blockAdjacency[jblk][iblk] == 0) {
            size_t wrd_off = iblk / word_size;
            size_t bit     = iblk % word_size;
            if (out_data[offset + wrd_off] & (1 << bit)) {
              blockAdjacency[jblk][iblk] = 1;
            }
          }
        }
        offset += bits_size;
      }
    }
#endif

    // Make it symmetric... (TODO: this probably isn't needed...)
    for (size_t iblk = 0; iblk < element_blocks.size(); iblk++) {
      for (size_t jblk = iblk; jblk < element_blocks.size(); jblk++) {
        blockAdjacency[jblk][iblk] = blockAdjacency[iblk][jblk];
      }
    }
  }

  AxisAlignedBoundingBox DatabaseIO::get_bounding_box(const Ioss::ElementBlock *eb) const
  {
    if (elementBlockBoundingBoxes.empty()) {
      // Calculate the bounding boxes for all element blocks...
      std::vector<double> coordinates;
      Ioss::NodeBlock    *nb = get_region()->get_node_blocks()[0];
      nb->get_field_data("mesh_model_coordinates", coordinates);
      auto nnode = nb->entity_count();
      auto ndim  = nb->get_property("component_degree").get_int();

      const Ioss::ElementBlockContainer &element_blocks = get_region()->get_element_blocks();
      size_t                             nblock         = element_blocks.size();
      std::vector<double>                minmax;
      minmax.reserve(6 * nblock);

      for (auto &block : element_blocks) {
        double xmin, ymin, zmin, xmax, ymax, zmax;
        if (block->get_database()->int_byte_size_api() == 8) {
          std::vector<int64_t> connectivity;
          block->get_field_data("connectivity_raw", connectivity);
          calc_bounding_box(ndim, nnode, coordinates, connectivity, xmin, ymin, zmin, xmax, ymax,
                            zmax);
        }
        else {
          std::vector<int> connectivity;
          block->get_field_data("connectivity_raw", connectivity);
          calc_bounding_box(ndim, nnode, coordinates, connectivity, xmin, ymin, zmin, xmax, ymax,
                            zmax);
        }

        minmax.push_back(xmin);
        minmax.push_back(ymin);
        minmax.push_back(zmin);
        minmax.push_back(-xmax);
        minmax.push_back(-ymax);
        minmax.push_back(-zmax);
      }

      util().global_array_minmax(minmax, Ioss::ParallelUtils::DO_MIN);

      for (size_t i = 0; i < element_blocks.size(); i++) {
        Ioss::ElementBlock    *block = element_blocks[i];
        const std::string     &name  = block->name();
        AxisAlignedBoundingBox bbox(minmax[6 * i + 0], minmax[6 * i + 1], minmax[6 * i + 2],
                                    -minmax[6 * i + 3], -minmax[6 * i + 4], -minmax[6 * i + 5]);
        elementBlockBoundingBoxes[name] = bbox;
      }
    }
    return elementBlockBoundingBoxes[eb->name()];
  }

  AxisAlignedBoundingBox DatabaseIO::get_bounding_box(const Ioss::NodeBlock *nb) const
  {
    std::vector<double> coordinates;
    nb->get_field_data("mesh_model_coordinates", coordinates);
    auto nnode = nb->entity_count();
    auto ndim  = nb->get_property("component_degree").get_int();

    double xmin, ymin, zmin, xmax, ymax, zmax;
    calc_bounding_box(ndim, nnode, coordinates, xmin, ymin, zmin, xmax, ymax, zmax);

    std::vector<double> minmax;
    minmax.reserve(6);
    minmax.push_back(xmin);
    minmax.push_back(ymin);
    minmax.push_back(zmin);
    minmax.push_back(-xmax);
    minmax.push_back(-ymax);
    minmax.push_back(-zmax);

    util().global_array_minmax(minmax, Ioss::ParallelUtils::DO_MIN);

    AxisAlignedBoundingBox bbox(minmax[0], minmax[1], minmax[2], -minmax[3], -minmax[4],
                                -minmax[5]);
    return bbox;
  }

  AxisAlignedBoundingBox DatabaseIO::get_bounding_box(const Ioss::StructuredBlock *sb) const
  {
    auto ndim = sb->get_property("component_degree").get_int();

    std::pair<double, double> xx;
    std::pair<double, double> yy;
    std::pair<double, double> zz;

    std::vector<double> coordinates;
    sb->get_field_data("mesh_model_coordinates_x", coordinates);
    auto x = std::minmax_element(coordinates.cbegin(), coordinates.cend());
    xx     = std::make_pair(*(x.first), *(x.second));

    if (ndim > 1) {
      sb->get_field_data("mesh_model_coordinates_y", coordinates);
      auto y = std::minmax_element(coordinates.cbegin(), coordinates.cend());
      yy     = std::make_pair(*(y.first), *(y.second));
    }

    if (ndim > 2) {
      sb->get_field_data("mesh_model_coordinates_z", coordinates);
      auto z = std::minmax_element(coordinates.cbegin(), coordinates.cend());
      zz     = std::make_pair(*(z.first), *(z.second));
    }

    return {xx.first, yy.first, zz.first, xx.second, yy.second, zz.second};
  }
} // namespace Ioss

namespace {
  void log_time(std::chrono::time_point<std::chrono::steady_clock> &start,
                std::chrono::time_point<std::chrono::steady_clock> &finish, int current_state,
                double state_time, bool is_input, bool single_proc_only,
                const Ioss::ParallelUtils &util)
  {
    std::vector<double> all_times;
    double duration = std::chrono::duration<double, std::milli>(finish - start).count();
    if (single_proc_only) {
      all_times.push_back(duration);
    }
    else {
      util.gather(duration, all_times);
    }

    if (util.parallel_rank() == 0 || single_proc_only) {
      std::ostringstream strm;
      fmt::print(strm, "\nIOSS: Time to {} state {}, time {} is ", (is_input ? "read " : "write"),
                 current_state, state_time);

      double total = 0.0;
      for (auto &p_time : all_times) {
        total += p_time;
      }

      // Now append each processors time onto the stream...
      if (util.parallel_size() == 1) {
        fmt::print(strm, "{} (ms)\n", total);
      }
      else if (util.parallel_size() > 4) {
        Ioss::sort(all_times.begin(), all_times.end());
        fmt::print(strm, " Min: {}\tMax: {}\tMed: {}", all_times.front(), all_times.back(),
                   all_times[all_times.size() / 2]);
      }
      else {
        char sep = (util.parallel_size() > 1) ? ':' : ' ';
        for (auto &p_time : all_times) {
          fmt::print(strm, "{:8d}{}", p_time, sep);
        }
      }
      if (util.parallel_size() > 1) {
        fmt::print(strm, "\tTot: {} (ms)\n", total);
      }
      fmt::print(Ioss::DEBUG(), "{}", strm.str());
    }
  }

  void log_field(const char *symbol, const Ioss::GroupingEntity *entity, const Ioss::Field &field,
                 bool single_proc_only, const Ioss::ParallelUtils &util)
  {
    if (entity != nullptr) {
      std::vector<int64_t> all_sizes;
      if (single_proc_only) {
        all_sizes.push_back(field.get_size());
      }
      else {
        util.gather(static_cast<int64_t>(field.get_size()), all_sizes);
      }

      if (util.parallel_rank() == 0 || single_proc_only) {
        const std::string            &name = entity->name();
        std::ostringstream            strm;
        auto                          now  = std::chrono::steady_clock::now();
        std::chrono::duration<double> diff = now - initial_time;
        fmt::print(strm, "{} [{:.3f}]\t", symbol, diff.count());

        int64_t total = 0;
        for (auto &p_size : all_sizes) {
          total += p_size;
        }
        // Now append each processors size onto the stream...
        if (util.parallel_size() > 4) {
          auto min_max = std::minmax_element(all_sizes.cbegin(), all_sizes.cend());
          fmt::print(strm, " m: {:8d} M: {:8d} A: {:8d}", *min_max.first, *min_max.second,
                     total / all_sizes.size());
        }
        else {
          for (auto &p_size : all_sizes) {
            fmt::print(strm, "{:8d}:", p_size);
          }
        }
        if (util.parallel_size() > 1) {
          fmt::print(strm, " T:{:8d}", total);
        }
        fmt::print(strm, "\t{}/{}\n", name, field.get_name());
        fmt::print(Ioss::DEBUG(), "{}", strm.str());
      }
    }
    else {
      if (!single_proc_only) {
        util.barrier();
      }
      if (util.parallel_rank() == 0 || single_proc_only) {
        auto                          time_now = std::chrono::steady_clock::now();
        std::chrono::duration<double> diff     = time_now - initial_time;
        fmt::print("{} [{:.3f}]\n", symbol, diff.count());
      }
    }
  }
} // namespace
