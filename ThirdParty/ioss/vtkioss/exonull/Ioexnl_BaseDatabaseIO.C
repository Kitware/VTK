// Copyright(C) 1999-2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_CodeTypes.h"
#include "Ioss_IOFactory.h"
#include "Ioss_ParallelUtils.h"
#include "Ioss_Utils.h"
#include "exonull/Ioexnl_BaseDatabaseIO.h"
#include <cassert>
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

#include "Ioexnl_Utils.h"
#include "Ioss_Assembly.h"
#include "Ioss_Blob.h"
#include "Ioss_DBUsage.h"
#include "Ioss_DatabaseIO.h"
#include "Ioss_EdgeBlock.h"
#include "Ioss_EdgeSet.h"
#include "Ioss_ElementBlock.h"
#include "Ioss_ElementSet.h"
#include "Ioss_EntityBlock.h"
#include "Ioss_EntityType.h"
#include "Ioss_FaceBlock.h"
#include "Ioss_FaceSet.h"
#include "Ioss_Field.h"
#include "Ioss_FileInfo.h"
#include "Ioss_GroupingEntity.h"
#include "Ioss_Map.h"
#include "Ioss_MeshType.h"
#include "Ioss_NodeBlock.h"
#include "Ioss_NodeSet.h"
#include "Ioss_Property.h"
#include "Ioss_PropertyManager.h"
#include "Ioss_Region.h"
#include "Ioss_SideBlock.h"
#include "Ioss_SideSet.h"
#include "Ioss_SmartAssert.h"
#include "Ioss_State.h"

// Transitioning from treating global variables as Ioss::Field::TRANSIENT
// to Ioss::Field::REDUCTION.  To get the old behavior, define the value
// below to '1'.
#define GLOBALS_ARE_TRANSIENT 0

// ========================================================================
// Static internal helper functions
// ========================================================================
namespace {
  std::vector<ex_entity_type> exodus_types({EX_GLOBAL, EX_BLOB, EX_ASSEMBLY, EX_NODE_BLOCK,
                                            EX_EDGE_BLOCK, EX_FACE_BLOCK, EX_ELEM_BLOCK,
                                            EX_NODE_SET, EX_EDGE_SET, EX_FACE_SET, EX_ELEM_SET,
                                            EX_SIDE_SET});

  void check_variable_consistency(const ex_var_params &exo_params, int my_processor,
                                  const std::string &filename, const Ioss::ParallelUtils &util);

  void check_attribute_index_order(Ioss::GroupingEntity *block);

  template <typename T>
  void write_attribute_names(int exoid, ex_entity_type type, const std::vector<T *> &entities);

  char **get_name_array(size_t count, int size)
  {
    auto *names = new char *[count];
    for (size_t i = 0; i < count; i++) {
      names[i] = new char[size + 1];
      std::memset(names[i], '\0', size + 1);
    }
    return names;
  }

  void delete_name_array(char **names, int count)
  {
    for (int i = 0; i < count; i++) {
      delete[] names[i];
    }
    delete[] names;
  }

} // namespace

namespace Ioexnl {
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
    dbState       = Ioss::STATE_UNKNOWN;

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
    dbIntSizeAPI = size; // mutable
  }

  // Returns byte size of integers stored on the database...
  int BaseDatabaseIO::int_byte_size_db() const { return 8; }

  // common
  unsigned BaseDatabaseIO::entity_field_support() const
  {
    return Ioss::NODEBLOCK | Ioss::EDGEBLOCK | Ioss::FACEBLOCK | Ioss::ELEMENTBLOCK |
           Ioss::NODESET | Ioss::EDGESET | Ioss::FACESET | Ioss::ELEMENTSET | Ioss::SIDESET |
           Ioss::SIDEBLOCK | Ioss::REGION | Ioss::SUPERELEMENT;
  }

  // common
  int BaseDatabaseIO::get_file_pointer() const { return 0; }

  int BaseDatabaseIO::free_file_pointer() const { return 0; }

  bool BaseDatabaseIO::ok_nl(bool, std::string *, int *) const { return true; }

  // common
  void BaseDatabaseIO::put_qa()
  {
    struct qa_element
    {
      char *qa_record[1][4];
    };

    size_t num_qa_records = qaRecords.size() / 4;

    if (using_parallel_io() && myProcessor != 0) {}
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

      for (size_t i = 0; i < num_qa_records + 1; i++) {
        for (int j = 0; j < 4; j++) {
          delete[] qa[i].qa_record[0][j];
        }
      }
    }
  }

  // common
  void BaseDatabaseIO::put_info() {}

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

  size_t BaseDatabaseIO::handle_block_ids(const Ioss::EntityBlock *eb, ex_entity_type,
                                          Ioss::Map &entity_map, void *ids, size_t num_to_get,
                                          size_t) const
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

  int64_t BaseDatabaseIO::put_field_internal(const Ioss::Region * /* region */,
                                             const Ioss::Field &field, void *data,
                                             size_t data_size) const
  {
    // For now, assume that all TRANSIENT fields on a region
    // are REDUCTION fields (1 value).  We need to gather these
    // and output them all at one time.  The storage location is a
    // 'globalVariables' array
    {
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

    auto type = Ioexnl::map_exodus_type(ge->type());

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
    auto type = Ioexnl::map_exodus_type(ge->type());

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
  void BaseDatabaseIO::write_reduction_fields() const {}

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
      if (!is_input()) {
        if (minimizeOpenFiles) {
          free_file_pointer();
        }
      }
      dbState = Ioss::STATE_UNKNOWN;
    }

    return true;
  }

  bool BaseDatabaseIO::begin_state_nl(int, double)
  {
    if (!is_input()) {
      // Zero global variable array...
      for (const auto &type : exodus_types) {
        auto &id_values = m_reductionValues[type];
        for (auto &values : id_values) {
          auto &vals = values.second;
          std::fill(vals.begin(), vals.end(), 0.0);
        }
      }
    }
    return true;
  }

  // common
  bool BaseDatabaseIO::end_state_nl(int state, double time)
  {
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
        for (const auto &type : exodus_types) {
          output_results_names(type, m_variables[type], false);
          output_results_names(type, m_reductionVariables[type], true);
        }
      }
    }
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

    auto type   = Ioexnl::map_exodus_type(ge->type());
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
  void BaseDatabaseIO::output_results_names(ex_entity_type, VariableNameMap &, bool) const {}

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
  void BaseDatabaseIO::flush_database_nl() const {}

  void BaseDatabaseIO::finalize_write(int, double) {}

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
      Ioexnl::get_id(node_blocks[0], &ids_);
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
          Ioexnl::set_id(assem, &ids_);
        }

        for (const auto &assem : assemblies) {
          Ioexnl::get_id(assem, &ids_);
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
          Ioexnl::set_id(blob, &ids_);
        }

        for (const auto &blob : blobs) {
          Ioexnl::get_id(blob, &ids_);
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
          Ioexnl::set_id(edge_block, &ids_);
        }

        edgeCount = 0;
        for (const auto &edge_block : edge_blocks) {
          edgeCount += edge_block->entity_count();
          // Set ids of all entities that do not have "id" property...
          Ioexnl::get_id(edge_block, &ids_);
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
          Ioexnl::set_id(face_block, &ids_);
        }

        faceCount = 0;
        for (auto &face_block : face_blocks) {
          faceCount += face_block->entity_count();
          // Set ids of all entities that do not have "id" property...
          Ioexnl::get_id(face_block, &ids_);
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
          Ioexnl::set_id(element_block, &ids_);
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
          Ioexnl::get_id(element_block, &ids_);
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
          Ioexnl::set_id(set, &ids_);
        }

        for (const auto &set : nodesets) {
          Ioexnl::get_id(set, &ids_);
        }
      }
      m_groupCount[EX_NODE_SET] = nodesets.size();
    }

    // EdgeSets ...
    {
      const Ioss::EdgeSetContainer &edgesets = region->get_edgesets();
      if (behavior != Ioss::DB_MODIFY) {
        for (const auto &set : edgesets) {
          Ioexnl::set_id(set, &ids_);
        }

        for (const auto &set : edgesets) {
          Ioexnl::get_id(set, &ids_);
        }
      }
      m_groupCount[EX_EDGE_SET] = edgesets.size();
    }

    // FaceSets ...
    {
      const Ioss::FaceSetContainer &facesets = region->get_facesets();
      if (behavior != Ioss::DB_MODIFY) {
        for (const auto &set : facesets) {
          Ioexnl::set_id(set, &ids_);
        }

        for (const auto &set : facesets) {
          Ioexnl::get_id(set, &ids_);
        }
      }
      m_groupCount[EX_FACE_SET] = facesets.size();
    }

    // ElementSets ...
    {
      const Ioss::ElementSetContainer &elementsets = region->get_elementsets();
      if (behavior != Ioss::DB_MODIFY) {
        for (const auto &set : elementsets) {
          Ioexnl::set_id(set, &ids_);
        }

        for (const auto &set : elementsets) {
          Ioexnl::get_id(set, &ids_);
        }
      }
      m_groupCount[EX_ELEM_SET] = elementsets.size();
    }

    // SideSets ...
    {
      const Ioss::SideSetContainer &ssets = region->get_sidesets();
      if (behavior != Ioss::DB_MODIFY) {
        for (const auto &set : ssets) {
          Ioexnl::set_id(set, &ids_);
        }
      }
      // Get entity counts for all face sets... Create SideSets.
      for (const auto &set : ssets) {
        if (behavior != Ioss::DB_MODIFY) {
          Ioexnl::get_id(set, &ids_);
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
    Ioexnl::write_reduction_attributes(get_file_pointer(), regions);
    Ioexnl::write_reduction_attributes(get_file_pointer(), get_region()->get_nodesets());
    Ioexnl::write_reduction_attributes(get_file_pointer(), get_region()->get_nodesets());
    Ioexnl::write_reduction_attributes(get_file_pointer(), get_region()->get_edgesets());
    Ioexnl::write_reduction_attributes(get_file_pointer(), get_region()->get_facesets());
    Ioexnl::write_reduction_attributes(get_file_pointer(), get_region()->get_elementsets());
    Ioexnl::write_reduction_attributes(get_file_pointer(), get_region()->get_node_blocks());
    Ioexnl::write_reduction_attributes(get_file_pointer(), get_region()->get_edge_blocks());
    Ioexnl::write_reduction_attributes(get_file_pointer(), get_region()->get_face_blocks());
    Ioexnl::write_reduction_attributes(get_file_pointer(), get_region()->get_element_blocks());
    Ioexnl::write_reduction_attributes(get_file_pointer(), get_region()->get_assemblies());
    Ioexnl::write_reduction_attributes(get_file_pointer(), get_region()->get_blobs());

    // Write coordinate names...

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

    if (node_map_cnt > 0) {
      char **names = get_name_array(node_map_cnt, maximumNameLength);
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
      delete_name_array(names, node_map_cnt);
    }

    if (elem_map_cnt > 0) {
      char **names = get_name_array(elem_map_cnt, maximumNameLength);
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
      delete_name_array(names, elem_map_cnt);
    }

    // Write coordinate frame data...
    write_coordinate_frames(get_file_pointer(), get_region()->get_coordinate_frames());
  }
} // namespace Ioexnl

namespace {
  template <typename T>
  void write_attribute_names(int, ex_entity_type, const std::vector<T *> &entities)
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
            "Something is wrong in the Ioexnl::BaseDatabaseIO class, function {}. Please report.\n",
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
              "Something is wrong in the Ioexnl::BaseDatabaseIO class, function {}. Please "
              "report.\n",
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
          "Something is wrong in the Ioexnl::BaseDatabaseIO class, function {}. Please report.\n",
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
          fmt::print(errmsg,
                     "INTERNAL ERROR: Block '{}' has an incomplete set of attributes.\n"
                     "Something is wrong in the Ioexnl::BaseDatabaseIO class, function {}. Please "
                     "report.\n",
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
