// Copyright(C) 1999-2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "Ioss_CoordinateFrame.h" // for CoordinateFrame
#include "Ioss_DBUsage.h"
#include "Ioss_DatabaseIO.h" // for DatabaseIO
#include "Ioss_DynamicTopology.h"
#include "Ioss_DynamicTopologyObserver.h"
#include "Ioss_EntityType.h" // for EntityType, etc
#include "Ioss_Field.h"
#include "Ioss_GroupingEntity.h" // for GroupingEntity
#include "Ioss_MeshType.h"
#include "Ioss_Property.h" // for Property
#include "Ioss_State.h"    // for State
#include <assert.h>
#include <cstddef> // for size_t, nullptr
#include <cstdint> // for int64_t

#include "Ioss_CodeTypes.h"
#include "Ioss_Utils.h"
#include "Ioss_VariableType.h"
#include "ioss_export.h"
#include "vtk_ioss_mangle.h"
#include <functional> // for less
#include <iosfwd>     // for ostream
#include <map>        // for map, map<>::value_compare
#include <memory>
#include <sstream>
#include <string> // for string, operator<
#include <tuple>
#include <utility> // for pair
#include <vector>  // for vector

namespace Ioss {
  class Assembly;
  class Blob;
  class CommSet;
  class EdgeBlock;
  class EdgeSet;
  class ElementBlock;
  class ElementSet;
  class FaceBlock;
  class FaceSet;
  class Field;
  class NodeBlock;
  class NodeSet;
  class SideBlock;
  class SideSet;
  class StructuredBlock;
} // namespace Ioss
// Needed for node_global_to_local inline function.

namespace Ioss {

  class CoordinateFrame;

  enum class MeshType;

  using AssemblyContainer = std::vector<Ioss::Assembly *>;
  using BlobContainer     = std::vector<Ioss::Blob *>;

  using NodeBlockContainer    = std::vector<NodeBlock *>;
  using EdgeBlockContainer    = std::vector<EdgeBlock *>;
  using FaceBlockContainer    = std::vector<FaceBlock *>;
  using ElementBlockContainer = std::vector<ElementBlock *>;

  using NodeSetContainer    = std::vector<NodeSet *>;
  using EdgeSetContainer    = std::vector<EdgeSet *>;
  using FaceSetContainer    = std::vector<FaceSet *>;
  using ElementSetContainer = std::vector<ElementSet *>;

  using SideSetContainer         = std::vector<SideSet *>;
  using StructuredBlockContainer = std::vector<StructuredBlock *>;
  using CommSetContainer         = std::vector<CommSet *>;
  using StateTimeContainer       = std::vector<double>;

  using CoordinateFrameContainer = std::vector<CoordinateFrame>;

  using AliasMap = std::map<std::string, std::string, std::less<>>;

  /** \brief A grouping entity that contains other grouping entities.
   *
   * Maintains a list of NodeBlocks, ElementBlocks, NodeLists, CommLists and Surfaces.
   * [Similar to the "Composite Pattern" in Design Patterns]  All interface to
   * GroupingEntities is through the Region class; clients of the IO subsystem have no direct
   * access to the underlying GroupingEntities (other than the Region).
   */
  class IOSS_EXPORT Region : public GroupingEntity
  {
  public:
    explicit Region(DatabaseIO *iodatabase = nullptr, const std::string &my_name = "");

    ~Region() override;

    IOSS_NODISCARD std::string type_string() const override { return "Region"; }
    IOSS_NODISCARD std::string short_type_string() const override { return "region"; }
    IOSS_NODISCARD std::string contains_string() const override { return "Entities"; }
    IOSS_NODISCARD EntityType  type() const override { return REGION; }

    IOSS_NODISCARD MeshType mesh_type() const;
    IOSS_NODISCARD std::string mesh_type_string() const;
    IOSS_NODISCARD bool        node_major() const;

    void output_summary(std::ostream &strm, bool do_transient = true) const;

    IOSS_NODISCARD bool supports_field_type(Ioss::EntityType fld_type) const;

    // Helper function...
    IOSS_NODISCARD int64_t node_global_to_local(int64_t global, bool must_exist = true) const;

    bool begin_mode(State new_state);
    bool end_mode(State current_state);

    // Add a new state at this time, return state number
    virtual int add_state(double time)
    {
      IOSS_FUNC_ENTER(m_);
      return add_state_nl(time);
    }
    virtual int add_state_nl(double time);

    // Get time corresponding to specified state

    IOSS_NODISCARD virtual double get_state_time(int state = -1) const;
    IOSS_NODISCARD int            get_current_state() const;
    double                        begin_state(int state);
    double                        end_state(int state);

    /** \brief Determine whether the metadata defining the model (nontransient,
     *         geometry, and toploloty) has been set.
     *
     *  \returns True if the metadata defining the model has been set.
     */
    IOSS_NODISCARD bool model_defined() const { return modelDefined; }

    /** \brief Determine whether the metadata related to the transient
     *         data has been set.
     *
     *  \returns True if the metadata related to the transient data has been set.
     */
    IOSS_NODISCARD bool transient_defined() const { return transientDefined; }

    /** \brief Remove all fields of the specified `role` from all entities in the region
     */
    void erase_fields(Field::RoleType role);

    // Return a pair consisting of the step (1-based) corresponding to
    // the maximum time on the database and the corresponding maximum
    // time value. Note that this may not necessarily be the last step
    // on the database if cycle and overlay are being used.
    IOSS_NODISCARD std::pair<int, double> get_max_time() const;

    // Return a tuple consisting of the step (1-based) corresponding to
    // the maximum time across all change sets on the database, the corresponding
    // maximum time value and the corresponding set.
    IOSS_NODISCARD std::tuple<std::string, int, double> get_db_max_time() const;

    // Return a pair consisting of the step (1-based) corresponding to
    // the minimum time on the database and the corresponding minimum
    // time value. Note that this may not necessarily be the first step
    // on the database if cycle and overlay are being used.
    IOSS_NODISCARD std::pair<int, double> get_min_time() const;

    // Return a tuple consisting of the step (1-based) corresponding to
    // the minimum time across all change sets on the database, the corresponding
    // minimum time value and the corresponding set.
    IOSS_NODISCARD std::tuple<std::string, int, double> get_db_min_time() const;

    // Functions for an output region...
    bool add(NodeBlock *node_block);
    bool add(EdgeBlock *edge_block);
    bool add(FaceBlock *face_block);
    bool add(ElementBlock *element_block);
    bool add(SideSet *sideset);
    bool add(NodeSet *nodeset);
    bool add(EdgeSet *edgeset);
    bool add(FaceSet *faceset);
    bool add(ElementSet *elementset);
    bool add(CommSet *commset);
    bool add(StructuredBlock *structured_block);
    bool add(Assembly *assembly);
    bool add(Blob *blob);
    bool add(const CoordinateFrame &frame);

    // Special purpose...
    bool remove(Assembly *removal);

    IOSS_NODISCARD const NodeBlockContainer       &get_node_blocks() const;
    IOSS_NODISCARD const EdgeBlockContainer       &get_edge_blocks() const;
    IOSS_NODISCARD const FaceBlockContainer       &get_face_blocks() const;
    IOSS_NODISCARD const ElementBlockContainer    &get_element_blocks() const;
    IOSS_NODISCARD const SideSetContainer         &get_sidesets() const;
    IOSS_NODISCARD const NodeSetContainer         &get_nodesets() const;
    IOSS_NODISCARD const EdgeSetContainer         &get_edgesets() const;
    IOSS_NODISCARD const FaceSetContainer         &get_facesets() const;
    IOSS_NODISCARD const ElementSetContainer      &get_elementsets() const;
    IOSS_NODISCARD const CommSetContainer         &get_commsets() const;
    IOSS_NODISCARD const StructuredBlockContainer &get_structured_blocks() const;
    IOSS_NODISCARD const AssemblyContainer        &get_assemblies() const;
    IOSS_NODISCARD const BlobContainer            &get_blobs() const;
    IOSS_NODISCARD const CoordinateFrameContainer &get_coordinate_frames() const;

    // Retrieve the Grouping Entity with the specified name.
    // Returns nullptr if the entity does not exist
    IOSS_NODISCARD GroupingEntity *get_entity(const std::string &my_name, EntityType io_type) const;
    IOSS_NODISCARD GroupingEntity *get_entity(const std::string &my_name) const;
    IOSS_NODISCARD NodeBlock      *get_node_block(const std::string &my_name) const;
    IOSS_NODISCARD EdgeBlock      *get_edge_block(const std::string &my_name) const;
    IOSS_NODISCARD FaceBlock      *get_face_block(const std::string &my_name) const;
    IOSS_NODISCARD ElementBlock   *get_element_block(const std::string &my_name) const;
    IOSS_NODISCARD SideSet        *get_sideset(const std::string &my_name) const;
    IOSS_NODISCARD SideBlock      *get_sideblock(const std::string &my_name) const;
    IOSS_NODISCARD NodeSet        *get_nodeset(const std::string &my_name) const;
    IOSS_NODISCARD EdgeSet        *get_edgeset(const std::string &my_name) const;
    IOSS_NODISCARD FaceSet        *get_faceset(const std::string &my_name) const;
    IOSS_NODISCARD ElementSet     *get_elementset(const std::string &my_name) const;
    IOSS_NODISCARD CommSet        *get_commset(const std::string &my_name) const;
    IOSS_NODISCARD StructuredBlock *get_structured_block(const std::string &my_name) const;
    IOSS_NODISCARD Assembly        *get_assembly(const std::string &my_name) const;
    IOSS_NODISCARD Blob            *get_blob(const std::string &my_name) const;

    // Not guaranteed to be efficient...
    // Note that not all GroupingEntity's are guaranteed to have an 'id'...
    IOSS_NODISCARD GroupingEntity *get_entity(int64_t id, EntityType io_type) const;

    IOSS_NODISCARD const CoordinateFrame &get_coordinate_frame(int64_t id) const;

    // Add the name 'alias' as an alias for the database entity of
    // type 'type' with the name 'db_name'. Returns true if alias
    // added; false if problems adding alias.
    bool           add_alias(const std::string &db_name, const std::string &alias, EntityType type);
    bool           add_alias(const std::string &db_name, const std::string &alias);
    bool           add_alias(const GroupingEntity *ge);
    IOSS_NODISCARD std::string get_alias(const std::string &alias, EntityType type) const;
    IOSS_NODISCARD std::string get_alias_nl(const std::string &alias,
                                            EntityType         type) const; // Not locked by mutex

    IOSS_NODISCARD const AliasMap &get_alias_map(EntityType entity_type) const;

    /// Get a map containing all aliases defined for the entity with basename 'my_name'
    int get_aliases(const std::string &my_name, EntityType type, Ioss::NameList &aliases) const;

    // This routine transfers all relevant aliases from the 'this'
    // region and applies them to the 'to' file.
    void transfer_mesh_aliases(Region *to) const;

    // Ensure that the 'this' region has the same ids and names as the 'from' region.
    void synchronize_id_and_name(const Region *from, bool sync_attribute_field_names = false);

    // Returns true if the passed in name refers to a known Entity
    // defined on this region.  If true, then 'type' (if non-nullptr) is
    // filled in with the type of the entity; if false, then type (if
    // non-nullptr) is set to 'INVALID' This function is defined to
    // consolidate several distinct implementations of this code in
    // client code. Because of this, the 'type' used in the client
    // code is repeated here instead of something more generic.
    IOSS_NODISCARD bool is_valid_io_entity(const std::string &my_name, unsigned int io_type,
                                           std::string *my_type = nullptr) const;

    void check_for_duplicate_names(const Ioss::GroupingEntity *entity) const;

    // Retrieve the element block that contains the specified element
    // The 'local_id' is the local database id (1-based), not the global id.
    // returns nullptr if no element block contains this element (local_id <= 0
    // or greater than number of elements in database)
    IOSS_NODISCARD ElementBlock *get_element_block(size_t local_id) const;

    // Retrieve the structured block that contains the specified node
    // The 'global_offset' is the global offset (0-based)
    // returns nullptr if no structured block contains this node (local_id <= 0
    // or greater than number of cell-nodes in database)
    IOSS_NODISCARD StructuredBlock *get_structured_block(size_t global_offset) const;

    // Handle implicit properties -- These are calculated from data stored
    // in the grouping entity instead of having an explicit value assigned.
    // An example would be 'element_block_count' for a region.
    IOSS_NODISCARD Property get_implicit_property(const std::string &my_name) const override;

    IOSS_NODISCARD const Ioss::NameList &get_information_records() const;
    void                                 add_information_records(const Ioss::NameList &info);
    void                                 add_information_record(const std::string &info);

    IOSS_NODISCARD const Ioss::NameList &get_qa_records() const;
    void add_qa_record(const std::string &code, const std::string &code_qa,
                       const std::string &date = "", const std::string &time = "");

    template <typename T, typename U>
    std::vector<size_t> get_entity_field_data(const std::string      &field_name,
                                              const std::vector<T *> &entity_container,
                                              std::vector<U>         &field_data) const;

    void register_mesh_modification_observer(std::shared_ptr<DynamicTopologyObserver> observer);
    IOSS_NODISCARD std::shared_ptr<DynamicTopologyObserver> get_mesh_modification_observer() const
    {
      return topologyObserver;
    }

    void                        reset_topology_modification();
    void                        set_topology_modification(unsigned int type);
    IOSS_NODISCARD unsigned int get_topology_modification() const;

    void start_new_output_database_entry(int steps = 0);

    void set_topology_change_count(unsigned int new_count) { dbChangeCount = new_count; }
    IOSS_NODISCARD unsigned int get_topology_change_count() const { return dbChangeCount; }

    void set_file_cyclic_count(unsigned int new_count) { fileCyclicCount = new_count; }
    IOSS_NODISCARD unsigned int get_file_cyclic_count() const { return fileCyclicCount; }

    void set_if_database_exists_behavior(IfDatabaseExistsBehavior if_exists)
    {
      ifDatabaseExists = if_exists;
    }
    IOSS_NODISCARD IfDatabaseExistsBehavior get_if_database_exists_behavior() const
    {
      return ifDatabaseExists;
    }

    IOSS_NODISCARD bool model_is_written() const { return modelWritten; }
    IOSS_NODISCARD bool transient_is_written() const { return transientWritten; }

    IOSS_NODISCARD bool load_internal_change_set_mesh(const std::string &set_name);
    IOSS_NODISCARD bool load_internal_change_set_mesh(const int set_index);

    IOSS_NODISCARD std::tuple<std::string, int, double> locate_db_state(double targetTime) const;

    // Reinitialize region data structures
    void reset_region();

  protected:
    std::string get_internal_change_set_name() const;
    void        update_dynamic_topology();
    void        clone_and_replace_output_database(int steps = 0);
    void        add_output_database_change_set(int steps = 0, bool force_addition = false);

    int64_t internal_get_field_data(const Field &field, void *data,
                                    size_t data_size = 0) const override;

    int64_t internal_put_field_data(const Field &field, void *data,
                                    size_t data_size = 0) const override;

    int64_t internal_get_zc_field_data(const Field &field, void **data,
                                       size_t *data_size) const override;

  private:
    template <typename T>
    std::vector<size_t> internal_get_entity_field_data(const std::string      &field_name,
                                                       const std::vector<T *> &entity_container,
                                                       void *data, size_t data_size = 0) const;

    // Add the name 'alias' as an alias for the database entity with the
    // name 'db_name'. Returns true if alias added; false if problems
    // adding alias. Not protected by mutex -- call internally only.
    bool add_alias_nl(const std::string &db_name, const std::string &alias, EntityType type);
    bool add_alias_nl(const GroupingEntity *ge);

    bool begin_mode_nl(State new_state);
    bool end_mode_nl(State current_state);

    void delete_database() override;

    mutable std::map<EntityType, AliasMap> aliases_; ///< Stores alias mappings

    // Containers for all grouping entities
    NodeBlockContainer    nodeBlocks;
    EdgeBlockContainer    edgeBlocks;
    FaceBlockContainer    faceBlocks;
    ElementBlockContainer elementBlocks;

    NodeSetContainer    nodeSets;
    EdgeSetContainer    edgeSets;
    FaceSetContainer    faceSets;
    ElementSetContainer elementSets;

    SideSetContainer           sideSets;
    CommSetContainer           commSets;
    CoordinateFrameContainer   coordinateFrames;
    StructuredBlockContainer   structuredBlocks;
    AssemblyContainer          assemblies;
    BlobContainer              blobs;
    mutable StateTimeContainer stateTimes;

    int         currentState{-1};
    mutable int stateCount{0};
    bool        modelDefined{false};
    bool        transientDefined{false};

    std::shared_ptr<DynamicTopologyObserver> topologyObserver;

    unsigned int dbChangeCount{1}; //!< Used to track number of topology changes.
    unsigned int fileCyclicCount{
        0}; //!< For cycling file-A, file-B, file-C, ..., File-A, typically restart only.
    IfDatabaseExistsBehavior ifDatabaseExists{DB_OVERWRITE};

    bool modelWritten{false};
    bool transientWritten{false};
    bool fileGroupsStarted{false};
  };
} // namespace Ioss

/** \brief Get the index (1-based) of the currently-active state.
 *
 *  \returns The index.
 */
inline int Ioss::Region::get_current_state() const { return currentState; }

inline bool Ioss::Region::supports_field_type(Ioss::EntityType fld_type) const
{
  return static_cast<unsigned int>((get_database()->entity_field_support() & fld_type) != 0U) != 0U;
}

inline int64_t Ioss::Region::node_global_to_local(int64_t global, bool must_exist) const
{
  return get_database()->node_global_to_local(global, must_exist);
}

/** \brief Get all information records (informative strings) for the region's database.
 *
 *  \returns The informative strings.
 */
inline const Ioss::NameList &Ioss::Region::get_information_records() const
{
  IOSS_FUNC_ENTER(m_);
  return get_database()->get_information_records();
}

/** \brief Add multiple information records (informative strings) to the region's database.
 *
 *  \param[in] info The strings to add.
 */
inline void Ioss::Region::add_information_records(const Ioss::NameList &info)
{
  IOSS_FUNC_ENTER(m_);
  return get_database()->add_information_records(info);
}

/** \brief Add an information record (an informative string) to the region's database.
 *
 *  \param[in] info The string to add.
 */
inline void Ioss::Region::add_information_record(const std::string &info)
{
  IOSS_FUNC_ENTER(m_);
  return get_database()->add_information_record(info);
}

/** \brief Add a QA record, which consists of 4 strings, to the region's database
 *
 *  The 4 function parameters correspond to the 4 QA record strings.
 *
 *  \param[in] code A descriptive code name, such as the application that modified the database.
 *  \param[in] code_qa A descriptive string, such as the version of the application that modified
 * the database.
 *  \param[in] date A relevant date, such as the date the database was modified.
 *  \param[in] time A relevant time, such as the time the database was modified.
 */
inline void Ioss::Region::add_qa_record(const std::string &code, const std::string &code_qa,
                                        const std::string &date, const std::string &time)
{
  IOSS_FUNC_ENTER(m_);
  return get_database()->add_qa_record(code, code_qa, date, time);
}

/** \brief Get all QA records, each of which consists of 4 strings, from the region's database.
 *
 *  The 4 strings that make up a database QA record are:
 *
 *  1. A descriptive code name, such as the application that modified the database.
 *
 *  2. A descriptive string, such as the version of the application that modified the database.
 *
 *  3. A relevant date, such as the date the database was modified.
 *
 *  4. A relevant time, such as the time the database was modified.
 *
 *  \returns All QA records in a single vector. Every 4 consecutive elements of the
 *           vector make up a single QA record.
 */
inline const Ioss::NameList &Ioss::Region::get_qa_records() const
{
  IOSS_FUNC_ENTER(m_);
  return get_database()->get_qa_records();
}

namespace Ioss {

  template <typename T>
  bool verify_field_exists_in_entity_group(const std::string      &field_name,
                                           const std::vector<T *> &entity_container)
  {
    bool                  found = false;
    Ioss::Field::RoleType role  = Ioss::Field::RoleType::INTERNAL;

    for (const T *entity : entity_container) {
      if (entity->field_exists(field_name)) {
        Ioss::Field field = entity->get_field(field_name);

        if (found && field.get_role() != role) {
          std::ostringstream errmsg;
          // Would be nice to use fmt:: here, but we need to avoid using fmt includes in public
          // headers...
          errmsg << "ERROR: Field " << field.get_name() << " with role " << field.role_string()
                 << " on entity " << entity->name() << " does not match previously found role "
                 << Ioss::Field::role_string(role) << ".\n";
          IOSS_ERROR(errmsg);
        }

        found = true;
        role  = field.get_role();
      }
    }

    return found;
  }

  namespace impl {
    template <typename T>
    size_t get_field_data_count_for_entities(const std::string      &field_name,
                                             const std::vector<T *> &entity_container)
    {
      size_t count = 0;

      for (const T *entity : entity_container) {
        if (entity->field_exists(field_name)) {
          Ioss::Field field = entity->get_field(field_name);

          count += entity->entity_count() * field.raw_storage()->component_count();
        }
      }

      return count;
    }
  } // namespace impl
} // namespace Ioss

template <typename T, typename U>
std::vector<size_t> Ioss::Region::get_entity_field_data(const std::string      &field_name,
                                                        const std::vector<T *> &entity_container,
                                                        std::vector<U>         &field_data) const
{
  bool field_exists = verify_field_exists_in_entity_group(field_name, entity_container);

  if (!field_exists) {
    return std::vector<size_t>(entity_container.size() + 1, 0);
  }

  size_t field_count = impl::get_field_data_count_for_entities(field_name, entity_container);

  field_data.resize(field_count);
  size_t data_size = field_count * sizeof(U);

  std::vector<size_t> offsets =
      internal_get_entity_field_data(field_name, entity_container, Data(field_data), data_size);

  assert(offsets.size() == (entity_container.size() + 1));
  assert(offsets[entity_container.size()] == field_count);

  return offsets;
}

template <typename T>
std::vector<size_t>
Ioss::Region::internal_get_entity_field_data(const std::string      &field_name,
                                             const std::vector<T *> &entity_container, void *data,
                                             size_t data_size) const
{
  return get_database()->get_entity_field_data(field_name, entity_container, data, data_size);
}
