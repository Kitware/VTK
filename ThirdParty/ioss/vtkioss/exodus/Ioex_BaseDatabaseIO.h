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

// -*- Mode: c++ -*-
#ifndef IOSS_Ioex_BaseDatabaseIO_h
#define IOSS_Ioex_BaseDatabaseIO_h

#include "vtk_ioss_mangle.h"

#include <Ioss_DBUsage.h>
#include <Ioss_DatabaseIO.h>
#include <Ioss_Field.h>
#include <Ioss_Map.h>
#include <Ioss_Utils.h>

#include <vtk_exodusII.h>

#include <algorithm>
#include <cstdint>
#include <ctime>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace Ioss {
  class Assembly;
  class Blob;
  class GroupingEntity;
  class Region;
  class EntityBlock;
  class NodeBlock;
  class EdgeBlock;
  class FaceBlock;
  class ElementBlock;
  class EntitySet;
  class NodeSet;
  class EdgeSet;
  class FaceSet;
  class ElementSet;
  class SideBlock;
  class SideSet;
  class StructuredBlock;
  class CommSet;
  class ElementTopology;
} // namespace Ioss

/** \brief A namespace for the exodus database format.
 */
namespace Ioex {
  struct CommunicationMetaData;

  // Used for variable name index mapping
  using VariableNameMap = std::map<std::string, int, std::less<std::string>>;
  using VNMValuePair    = VariableNameMap::value_type;

  // Used to store reduction variables
  using ValueContainer = std::vector<double>;

  // Used for persistent entity IDs
  // The set contains a pair of <ex_entity_type, int>.
  // The ex_entity_type is the exodus entity type defined in
  // exodus's exodusII.h. A couple examples are:
  // EX_ELEM_BLOCK element block and EX_NODE_SET nodeset.
  //
  // The 'int' is the entity id.  The set is used for output databases
  // to ensure that there are no id collisions.
  using EntityIdSet = std::set<std::pair<int64_t, int64_t>>;

  class BaseDatabaseIO : public Ioss::DatabaseIO
  {
  public:
    BaseDatabaseIO(Ioss::Region *region, const std::string &filename, Ioss::DatabaseUsage db_usage,
                   MPI_Comm communicator, const Ioss::PropertyManager &props);
    BaseDatabaseIO(const BaseDatabaseIO &from) = delete;
    BaseDatabaseIO &operator=(const BaseDatabaseIO &from) = delete;

    ~BaseDatabaseIO() override;

    const std::string get_format() const override { return "Exodus"; }

    // Check capabilities of input/output database...  Returns an
    // unsigned int with the supported Ioss::EntityTypes or'ed
    // together. If "return_value & Ioss::EntityType" is set, then the
    // database supports that type (e.g. return_value & Ioss::FACESET)
    unsigned entity_field_support() const override;

  protected:
    // Check to see if database state is ok...
    // If 'write_message' true, then output a warning message indicating the problem.
    // If 'error_message' non-null, then put the warning message into the string and return it.
    // If 'bad_count' non-null, it counts the number of processors where the file does not exist.
    //    if ok returns false, but *bad_count==0, then the routine does not support this argument.
    bool ok__(bool write_message = false, std::string *error_message = nullptr,
              int *bad_count = nullptr) const override;

    bool open_group__(const std::string &group_name) override;
    bool create_subgroup__(const std::string &group_name) override;

    bool begin__(Ioss::State state) override;
    bool end__(Ioss::State state) override;

    void open_state_file(int state);

    bool begin_state__(int state, double time) override;
    bool end_state__(int state, double time) override;
    void get_step_times__() override = 0;

    int maximum_symbol_length() const override { return maximumNameLength; }

    // NOTE: If this is called after write_meta_data, it will have no affect.
    //       Also, it only affects output databases, not input.
    void set_maximum_symbol_length(int requested_symbol_size) override
    {
      if (!is_input()) {
        maximumNameLength = requested_symbol_size;
      }
    }

    size_t handle_block_ids(const Ioss::EntityBlock *eb, ex_entity_type map_type,
                            Ioss::Map &entity_map, void *ids, size_t num_to_get,
                            size_t offset) const;

    void compute_block_membership__(Ioss::SideBlock *         efblock,
                                    std::vector<std::string> &block_membership) const override;

    int  int_byte_size_db() const override;
    void set_int_byte_size_api(Ioss::DataSize size) const override;

  protected:
    int64_t get_field_internal(const Ioss::Region *reg, const Ioss::Field &field, void *data,
                               size_t data_size) const override             = 0;
    int64_t get_field_internal(const Ioss::NodeBlock *nb, const Ioss::Field &field, void *data,
                               size_t data_size) const override             = 0;
    int64_t get_field_internal(const Ioss::EdgeBlock *nb, const Ioss::Field &field, void *data,
                               size_t data_size) const override             = 0;
    int64_t get_field_internal(const Ioss::FaceBlock *nb, const Ioss::Field &field, void *data,
                               size_t data_size) const override             = 0;
    int64_t get_field_internal(const Ioss::ElementBlock *eb, const Ioss::Field &field, void *data,
                               size_t data_size) const override             = 0;
    int64_t get_field_internal(const Ioss::StructuredBlock *sb, const Ioss::Field &field,
                               void *data, size_t data_size) const override = 0;
    int64_t get_field_internal(const Ioss::SideBlock *fb, const Ioss::Field &field, void *data,
                               size_t data_size) const override             = 0;
    int64_t get_field_internal(const Ioss::NodeSet *ns, const Ioss::Field &field, void *data,
                               size_t data_size) const override             = 0;
    int64_t get_field_internal(const Ioss::EdgeSet *ns, const Ioss::Field &field, void *data,
                               size_t data_size) const override             = 0;
    int64_t get_field_internal(const Ioss::FaceSet *ns, const Ioss::Field &field, void *data,
                               size_t data_size) const override             = 0;
    int64_t get_field_internal(const Ioss::ElementSet *ns, const Ioss::Field &field, void *data,
                               size_t data_size) const override             = 0;
    int64_t get_field_internal(const Ioss::SideSet *fs, const Ioss::Field &field, void *data,
                               size_t data_size) const override             = 0;
    int64_t get_field_internal(const Ioss::CommSet *cs, const Ioss::Field &field, void *data,
                               size_t data_size) const override             = 0;
    int64_t get_field_internal(const Ioss::Assembly *as, const Ioss::Field &field, void *data,
                               size_t data_size) const override             = 0;
    int64_t get_field_internal(const Ioss::Blob *blob, const Ioss::Field &field, void *data,
                               size_t data_size) const override             = 0;

    int64_t put_field_internal(const Ioss::Region *reg, const Ioss::Field &field, void *data,
                               size_t data_size) const override             = 0;
    int64_t put_field_internal(const Ioss::NodeBlock *nb, const Ioss::Field &field, void *data,
                               size_t data_size) const override             = 0;
    int64_t put_field_internal(const Ioss::EdgeBlock *nb, const Ioss::Field &field, void *data,
                               size_t data_size) const override             = 0;
    int64_t put_field_internal(const Ioss::FaceBlock *nb, const Ioss::Field &field, void *data,
                               size_t data_size) const override             = 0;
    int64_t put_field_internal(const Ioss::ElementBlock *eb, const Ioss::Field &field, void *data,
                               size_t data_size) const override             = 0;
    int64_t put_field_internal(const Ioss::StructuredBlock *sb, const Ioss::Field &field,
                               void *data, size_t data_size) const override = 0;
    int64_t put_field_internal(const Ioss::SideBlock *fb, const Ioss::Field &field, void *data,
                               size_t data_size) const override             = 0;
    int64_t put_field_internal(const Ioss::NodeSet *ns, const Ioss::Field &field, void *data,
                               size_t data_size) const override             = 0;
    int64_t put_field_internal(const Ioss::EdgeSet *ns, const Ioss::Field &field, void *data,
                               size_t data_size) const override             = 0;
    int64_t put_field_internal(const Ioss::FaceSet *ns, const Ioss::Field &field, void *data,
                               size_t data_size) const override             = 0;
    int64_t put_field_internal(const Ioss::ElementSet *ns, const Ioss::Field &field, void *data,
                               size_t data_size) const override             = 0;
    int64_t put_field_internal(const Ioss::SideSet *fs, const Ioss::Field &field, void *data,
                               size_t data_size) const override             = 0;
    int64_t put_field_internal(const Ioss::CommSet *cs, const Ioss::Field &field, void *data,
                               size_t data_size) const override             = 0;
    int64_t put_field_internal(const Ioss::Assembly *as, const Ioss::Field &field, void *data,
                               size_t data_size) const override             = 0;
    int64_t put_field_internal(const Ioss::Blob *blob, const Ioss::Field &field, void *data,
                               size_t data_size) const override             = 0;

    virtual void write_meta_data() = 0;
    void         write_results_metadata(bool gather_data = true);

    void openDatabase__() const override { get_file_pointer(); }

    void closeDatabase__() const override
    {
      free_file_pointer();
      closeDW();
    }

    int get_file_pointer() const override = 0; // Open file and set exodusFilePtr.

    virtual int free_file_pointer() const; // Close file and set exodusFilePtr.

    virtual bool open_input_file(bool write_message, std::string *error_msg, int *bad_count,
                                 bool abort_if_error) const                    = 0;
    virtual bool handle_output_file(bool write_message, std::string *error_msg, int *bad_count,
                                    bool overwrite, bool abort_if_error) const = 0;
    void         finalize_file_open() const;

    int  get_current_state() const; // Get current state with error checks and usage message.
    void put_qa();
    void put_info();

    template <typename T>
    void internal_write_results_metadata(ex_entity_type type, std::vector<T *> entities,
                                         int &glob_index);

    void generate_sideset_truth_table();

    void output_results_names(ex_entity_type type, VariableNameMap &variables,
                              bool reduction) const;
    int  gather_names(ex_entity_type type, VariableNameMap &variables,
                      const Ioss::GroupingEntity *ge, int index, bool reduction);

    void get_nodeblocks();
    void get_assemblies();
    void get_blobs();

    void add_attribute_fields(ex_entity_type entity_type, Ioss::GroupingEntity *block,
                              int attribute_count, const std::string &type);

    void output_other_meta_data();

    int64_t internal_add_results_fields(ex_entity_type type, Ioss::GroupingEntity *entity,
                                        int64_t position, int64_t block_count,
                                        Ioss::IntVector &      truth_table,
                                        Ioex::VariableNameMap &variables);
    int64_t add_results_fields(ex_entity_type type, Ioss::GroupingEntity *entity,
                               int64_t position = 0);
    int64_t add_reduction_results_fields(ex_entity_type type, Ioss::GroupingEntity *entity);
    void add_mesh_reduction_fields(ex_entity_type type, int64_t id, Ioss::GroupingEntity *entity);

    void add_region_fields();
    void store_reduction_field(ex_entity_type type, const Ioss::Field &field,
                               const Ioss::GroupingEntity *ge, void *variables) const;

    void get_reduction_field(ex_entity_type type, const Ioss::Field &field,
                             const Ioss::GroupingEntity *ge, void *variables) const;
    void write_reduction_fields() const;
    void read_reduction_fields() const;

    // Handle special output time requests -- primarily restart (cycle, keep, overwrite)
    // Given the global region step, return the step on the database...
    int get_database_step(int global_step) const;

    void flush_database__() const override;
    void finalize_write(int state, double sim_time);

    // Private member data...
  protected:
    mutable int         exodusFilePtr{-1};
    mutable std::string m_groupName;

    mutable EntityIdSet ids_;

    mutable int exodusMode{EX_CLOBBER};
    mutable int dbRealWordSize{8};

    mutable int maximumNameLength{32};
    int         spatialDimension{0};

    int64_t edgeCount{0};
    int64_t faceCount{0};

    mutable std::map<ex_entity_type, int> m_groupCount;

    // Communication Set Data
    Ioss::Int64Vector nodeCmapIds;
    Ioss::Int64Vector nodeCmapNodeCnts;
    Ioss::Int64Vector elemCmapIds;
    Ioss::Int64Vector elemCmapElemCnts;
    int64_t           commsetNodeCount{0};
    int64_t           commsetElemCount{0};

    // --- Nodal/Element/Attribute Variable Names -- Maps from sierra
    // field names to index of nodal/element/attribute variable in
    // exodusII. Note that the component suffix of the field is added on
    // prior to searching the map for the index.  For example, given the
    // Sierra field 'displ' which is a VECTOR_3D, the names stored in
    // 'elementMap' would be 'displ_x', 'displ_y' and 'displ_z'.  All
    // names are converted to lowercase.

    mutable std::map<ex_entity_type, Ioss::IntVector> m_truthTable;
    mutable std::map<ex_entity_type, VariableNameMap> m_variables;
    mutable std::map<ex_entity_type, VariableNameMap> m_reductionVariables;

    mutable std::map<ex_entity_type, std::map<int64_t, ValueContainer>> m_reductionValues;

    mutable std::vector<unsigned char> nodeConnectivityStatus;

    // For a database with omitted blocks, this map contains the indices of the
    // active nodes for each nodeset.  If the nodeset is not reduced in size,
    // the map's vector will be empty for that nodeset. If the vector is not
    // empty, then some nodes on that nodeset are only connected to omitted elements.
    mutable std::map<std::string, Ioss::Int64Vector> activeNodeSetNodesIndex;

    time_t timeLastFlush{0};
    int    flushInterval{-1};

    mutable bool fileExists{false}; // False if file has never been opened/created
    mutable bool minimizeOpenFiles{false};

    mutable bool blockAdjacenciesCalculated{false}; // True if the lazy creation of
    // block adjacencies has been calculated.
    mutable bool nodeConnectivityStatusCalculated{
        false}; // True if the lazy creation of
                // nodeConnectivityStatus has been calculated.
  };
} // namespace Ioex
#endif
