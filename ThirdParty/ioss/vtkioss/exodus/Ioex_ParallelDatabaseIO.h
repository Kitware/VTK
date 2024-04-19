// Copyright(C) 1999-2022 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

// -*- Mode: c++ -*-
#pragma once

#include "ioex_export.h"

#include "vtk_ioss_mangle.h"

#include <vtk_exodusII.h>
#if defined PARALLEL_AWARE_EXODUS
#include <Ioss_CodeTypes.h>
#include <Ioss_DBUsage.h>               // for DatabaseUsage
#include <Ioss_Map.h>                   // for Map
#include <Ioss_State.h>                 // for State
#include <exodus/Ioex_BaseDatabaseIO.h> // for DatabaseIO
#include <functional>                   // for less
#include <map>                          // for map, map<>::value_compare
#include <memory>
#include <set>      // for set
#include <stddef.h> // for size_t
#include <stdint.h> // for int64_t
#include <string>   // for string, operator<
#include <time.h>   // for nullptr, time_t
#include <utility>  // for pair
#include <vector>   // for vector
namespace Ioex {
  class DecompositionDataBase;
}
namespace Ioex {
  template <typename INT> class DecompositionData;
}

namespace Ioss {
  class Assembly;
  class Blob;
  class EntityBlock;
  class ElementTopology;
  class CommSet;
  class EdgeBlock;
  class EdgeSet;
  class ElementBlock;
  class ElementSet;
  class EntitySet;
  class FaceBlock;
  class FaceSet;
  class Field;
  class GroupingEntity;
  class NodeBlock;
  class NodeSet;
  class PropertyManager;
  class Region;
  class SideBlock;
  class SideSet;
  class StructuredBlock;
} // namespace Ioss

/** \brief A namespace for the decompose-on-the-fly version of the
 *  parallel exodus database format.
 */
namespace Ioex {
  class IOEX_EXPORT ParallelDatabaseIO : public Ioex::BaseDatabaseIO
  {
  public:
    ParallelDatabaseIO(Ioss::Region *region, const std::string &filename,
                       Ioss::DatabaseUsage db_usage, Ioss_MPI_Comm communicator,
                       const Ioss::PropertyManager &properties);
    ParallelDatabaseIO(const ParallelDatabaseIO &from)            = delete;
    ParallelDatabaseIO &operator=(const ParallelDatabaseIO &from) = delete;
    ~ParallelDatabaseIO();

    int  get_file_pointer() const override; // Open file and set exodusFilePtr.
    bool needs_shared_node_information() const override { return true; }

  private:
    void compute_node_status() const;

    void release_memory__() override;

    void get_step_times__() override;

    bool open_input_file(bool write_message, std::string *error_msg, int *bad_count,
                         bool abort_if_error) const override;
    bool handle_output_file(bool write_message, std::string *error_msg, int *bad_count,
                            bool overwrite, bool abort_if_error) const override;
    bool check_valid_file_ptr(bool write_message, std::string *error_msg, int *bad_count,
                              bool abort_if_error) const;

    int64_t get_field_internal(const Ioss::Region *reg, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t get_field_internal(const Ioss::NodeBlock *nb, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t get_field_internal(const Ioss::EdgeBlock *eb, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t get_field_internal(const Ioss::FaceBlock *eb, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t get_field_internal(const Ioss::ElementBlock *eb, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t get_field_internal(const Ioss::StructuredBlock * /* sb */,
                               const Ioss::Field & /* field */, void * /* data */,
                               size_t /* data_size */) const override
    {
      return -1;
    }
    int64_t get_field_internal(const Ioss::SideBlock *sb, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t get_field_internal(const Ioss::NodeSet *ns, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t get_field_internal(const Ioss::EdgeSet *ns, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t get_field_internal(const Ioss::FaceSet *ns, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t get_field_internal(const Ioss::ElementSet *ns, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t get_field_internal(const Ioss::SideSet *fs, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t get_field_internal(const Ioss::CommSet *cs, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t get_field_internal(const Ioss::Assembly *sb, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t get_field_internal(const Ioss::Blob *sb, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t put_field_internal(const Ioss::Region *reg, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t put_field_internal(const Ioss::NodeBlock *nb, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t put_field_internal(const Ioss::EdgeBlock *eb, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t put_field_internal(const Ioss::FaceBlock *eb, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t put_field_internal(const Ioss::ElementBlock *eb, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t put_field_internal(const Ioss::SideBlock *sb, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t put_field_internal(const Ioss::NodeSet *ns, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t put_field_internal(const Ioss::EdgeSet *ns, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t put_field_internal(const Ioss::FaceSet *ns, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t put_field_internal(const Ioss::ElementSet *ns, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t put_field_internal(const Ioss::SideSet *ss, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t put_field_internal(const Ioss::CommSet *cs, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t put_field_internal(const Ioss::Assembly *sb, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t put_field_internal(const Ioss::Blob *sb, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t put_field_internal(const Ioss::StructuredBlock * /* sb */,
                               const Ioss::Field & /* field */, void * /* data */,
                               size_t /* data_size */) const override
    {
      return -1;
    }

    int64_t put_Xset_field_internal(const Ioss::EntitySet *ns, const Ioss::Field &field, void *data,
                                    size_t data_size) const;
    int64_t get_Xset_field_internal(const Ioss::EntitySet *ns, const Ioss::Field &field, void *data,
                                    size_t data_size) const;

    int free_file_pointer() const override;

    int64_t read_nodal_coordinates();
    void    read_elements(const Ioss::ElementBlock &block);

    void create_implicit_global_map() const;
    void output_node_map() const;

    // Metadata-related functions.
    void read_meta_data__() override;

    int64_t read_transient_field(const Ioex::VariableNameMap &variables, const Ioss::Field &field,
                                 const Ioss::GroupingEntity *ge, void *data) const;

    int64_t read_attribute_field(const Ioss::Field &field, const Ioss::GroupingEntity *ge,
                                 void *data) const;

    int64_t write_attribute_field(const Ioss::Field &field, const Ioss::GroupingEntity *ge,
                                  void *data) const;

    // Handles subsetting of side blocks.
    int64_t read_ss_transient_field(const Ioss::Field &field, int64_t id, void *variables,
                                    std::vector<int> &is_valid_side) const;

    // Should be made more generic again so can rejoin with write_element_transient field
    void write_nodal_transient_field(const Ioss::Field &field, const Ioss::NodeBlock *nb,
                                     int64_t count, void *variables) const;
    // Should be made more generic again so can rejoin with write_nodal_transient field
    void write_entity_transient_field(const Ioss::Field &field, const Ioss::GroupingEntity *ge,
                                      int64_t count, void *variables) const;
    void write_meta_data(Ioss::IfDatabaseExistsBehavior behavior) override;

    // Read related metadata and store it in the region...
    void read_region();
    void get_edgeblocks();
    void get_faceblocks();
    void get_elemblocks();
    void get_blocks(ex_entity_type entity_type, int rank_offset, const std::string &basename);

    void get_sidesets();

    template <typename T>
    void get_sets(ex_entity_type type, int64_t count, const std::string &base, const T *);
    void get_nodesets();
    void get_edgesets();
    void get_facesets();
    void get_elemsets();

    void get_commsets();

    void check_valid_values() const;

    // ID Mapping functions.
    const Ioss::Map &get_map(ex_entity_type type) const;
    const Ioss::Map &get_map(Ioss::Map &entity_map, int64_t entityCount, int64_t file_offset,
                             int64_t file_count, ex_entity_type entity_type,
                             ex_inquiry inquiry_type) const;

    // Internal data handling
    int64_t handle_node_ids(void *ids, int64_t num_to_get, size_t offset, size_t count) const;
    int64_t handle_element_ids(const Ioss::ElementBlock *eb, void *ids, size_t num_to_get,
                               size_t offset, size_t count) const;
    int64_t handle_face_ids(const Ioss::FaceBlock *eb, void *ids, size_t num_to_get) const;
    int64_t handle_edge_ids(const Ioss::EdgeBlock *eb, void *ids, size_t num_to_get) const;

    int64_t get_side_connectivity(const Ioss::SideBlock *sd_blk, int64_t id, int64_t side_count,
                                  void *fconnect, bool map_ids) const;
    int64_t get_side_distributions(const Ioss::SideBlock *sd_blk, int64_t id, int64_t my_side_count,
                                   double *dist_fact, size_t data_size) const;

    int64_t get_side_field(const Ioss::SideBlock *sd_blk, const Ioss::Field &field, void *data,
                           size_t data_size) const;
    int64_t put_side_field(const Ioss::SideBlock *sd_blk, const Ioss::Field &field, void *data,
                           size_t data_size) const;

    // Private member data...
    mutable std::unique_ptr<DecompositionDataBase> decomp;

    mutable Ioss::IntVector nodeOwningProcessor; // Processor that owns each node on this processor
    mutable Ioss::Int64Vector
        nodeGlobalImplicitMap; // Position of this node in the global-implicit ordering
    mutable Ioss::Int64Vector
        elemGlobalImplicitMap; // Position of this element in the global-implicit ordering

    // Contains the indices of all owned nodes in each nodeset on this processor to pull data
    // from the global list down to the file list.
    // NOTE: Even though map type is GroupingEntity*, it is only valid
    // for a GroupingEntity* which is a NodeSet*
    mutable std::map<const Ioss::GroupingEntity *, Ioss::Int64Vector> nodesetOwnedNodes;

    mutable bool metaDataWritten{false};
    mutable bool nodeGlobalImplicitMapDefined{false};
    mutable bool elemGlobalImplicitMapDefined{false};
  };
} // namespace Ioex
#endif
