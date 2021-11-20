// Copyright(C) 1999-2021 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include <tokenize.h>

#include "Ioss_CommSet.h"         // for CommSet
#include "Ioss_DBUsage.h"         // for DatabaseUsage, etc
#include "Ioss_DatabaseIO.h"      // for DatabaseIO
#include "Ioss_EdgeBlock.h"       // for EdgeBlock
#include "Ioss_EdgeSet.h"         // for EdgeSet
#include "Ioss_ElementBlock.h"    // for ElementBlock
#include "Ioss_ElementSet.h"      // for ElementSet
#include "Ioss_EntityType.h"      // for EntityType::ELEMENTBLOCK
#include "Ioss_FaceBlock.h"       // for FaceBlock
#include "Ioss_FaceSet.h"         // for FaceSet
#include "Ioss_FileInfo.h"        // for FileInfo
#include "Ioss_Map.h"             // for Map, MapContainer
#include "Ioss_NodeBlock.h"       // for NodeBlock
#include "Ioss_NodeSet.h"         // for NodeSet
#include "Ioss_Property.h"        // for Property
#include "Ioss_SideBlock.h"       // for SideBlock
#include <Ioss_Assembly.h>        // for Assembly
#include <Ioss_CodeTypes.h>       // for HAVE_MPI
#include <Ioss_ElementTopology.h> // for NameList
#include <Ioss_ParallelUtils.h>   // for ParallelUtils, etc
#include <Ioss_Region.h>          // for Region
#include <Ioss_SerializeIO.h>     // for SerializeIO
#include <Ioss_StructuredBlock.h> // for StructuredBlock

#include <Ioss_Utils.h> // for Utils, IOSS_ERROR, etc
#include <climits>
#include <fmt/ostream.h>

#include <catalyst.hpp>
#include <catalyst/Iocatalyst_DatabaseIO.h>

namespace Ioss {
  class PropertyManager;
}

namespace Iocatalyst {

  namespace detail {

    class FieldNonExistent
    {
    };

    template <typename GroupingEntityT>
    GroupingEntityT *createEntityGroup(const conduit_cpp::Node &node, Ioss::DatabaseIO *dbase);

    template <>
    Ioss::NodeBlock *createEntityGroup<Ioss::NodeBlock>(const conduit_cpp::Node &node,
                                                        Ioss::DatabaseIO *       dbase)
    {
      const auto name = node.name();
      return new Ioss::NodeBlock(dbase, name, node["properties/entity_count/value"].as_int64(),
                                 node["properties/component_degree/value"].as_int64());
    }

    template <>
    Ioss::ElementBlock *createEntityGroup<Ioss::ElementBlock>(const conduit_cpp::Node &node,
                                                              Ioss::DatabaseIO *       dbase)
    {
      const auto name = node.name();
      return new Ioss::ElementBlock(dbase, name,
                                    node["properties/original_topology_type/value"].as_string(),
                                    node["properties/entity_count/value"].as_int64());
    }

    template <>
    Ioss::NodeSet *createEntityGroup<Ioss::NodeSet>(const conduit_cpp::Node &node,
                                                    Ioss::DatabaseIO *       dbase)
    {
      const auto name = node.name();
      return new Ioss::NodeSet(dbase, name, node["properties/entity_count/value"].as_int64());
    }

    template <>
    Ioss::SideSet *createEntityGroup<Ioss::SideSet>(const conduit_cpp::Node &node,
                                                    Ioss::DatabaseIO *       dbase)
    {
      const auto name = node.name();
      return new Ioss::SideSet(dbase, name);
    }

  } // namespace detail

  class DatabaseIO::ImplementationT
  {
    conduit_cpp::Node Root;
    conduit_cpp::Node DBNode;
    mutable Ioss::Map NodeMap;

  public:
    conduit_cpp::Node &databaseNode() { return this->DBNode; }
    void               setDatabaseNode(conduit_node *c_node)
    {
      this->DBNode = conduit_cpp::Node();
      conduit_node_set_external_node(conduit_cpp::c_node(&this->DBNode), c_node);
    }
    conduit_cpp::Node &root() { return this->Root; }

    void print()
    {
      auto &node = this->DBNode;
      node.print_detailed();
    }

    bool defineModel(Ioss::Region *region)
    {
      assert(region->model_defined());

      auto &node = this->DBNode;
      node       = conduit_cpp::Node();

      node["database/int_byte_size_api"].set_int8(region->get_database()->int_byte_size_api());

      this->defineEntityGroup(node["node_blocks"], region->get_node_blocks());
      this->defineEntityGroup(node["edge_blocks"], region->get_edge_blocks());
      this->defineEntityGroup(node["face_blocks"], region->get_face_blocks());
      this->defineEntityGroup(node["element_blocks"], region->get_element_blocks());
      // TODO: need to handle side-blocks.
      this->defineEntityGroup(node["sidesets"], region->get_sidesets());
      this->defineEntityGroup(node["nodesets"], region->get_nodesets());
      this->defineEntityGroup(node["edgesets"], region->get_edgesets());
      this->defineEntityGroup(node["facesets"], region->get_facesets());
      this->defineEntityGroup(node["elementsets"], region->get_elementsets());
      this->defineEntityGroup(node["structured_blocks"], region->get_structured_blocks());
      this->defineEntityGroup(node["assemblies"], region->get_assemblies());
      return true;
    }

    bool readModel(Ioss::Region *region)
    {
      auto &node = this->DBNode;
      region->get_database()->set_int_byte_size_api(
          static_cast<Ioss::DataSize>(node["database/int_byte_size_api"].as_int8()));
      this->readEntityGroup<Ioss::NodeBlock>(node["node_blocks"], region);
      this->readEntityGroup<Ioss::ElementBlock>(node["element_blocks"], region);
      // this->readEntityGroup<Ioss::EdgeBlock>(node["edge_blocks"], region);
      // this->readEntityGroup<Ioss::FaceBlock>(node["face_blocks"], region);
      // this->readEntityGroup<Ioss::SideSet>(node["sidesets"], region);
      // this->readEntityGroup<Ioss::NodeSet>(node["nodesets"], region);
      // this->readEntityGroup<Ioss::EdgeSet>(node["edgesets"], region);
      // this->readEntityGroup<Ioss::FaceSet>(node["facesets"], region);
      // this->readEntityGroup<Ioss::ElementSet>(node["elementsets"], region);
      // this->readEntityGroup<Ioss::StructuredBlock>(node["structured_blocks"], region);
      // this->readEntityGroup<Ioss::Assembly>(node["assemblies"], region);

      return this->readTime(region);
    }

    bool readTime(Ioss::Region *region)
    {
      auto &     node = this->DBNode;
      const auto time = node["state_time"].as_float64();
      region->add_state(time);
      return true;
    }

    int64_t putField(const std::string &containerName, const Ioss::GroupingEntity *entityGroup,
                     const Ioss::Field &field, void *data, size_t data_size, bool deep_copy)
    {
      const auto groupName      = entityGroup->generic_name();
      const auto num_to_get     = field.verify(data_size);
      const auto num_components = field.raw_storage()->component_count();
      if (num_to_get > 0) {
        auto &&node = this->DBNode[containerName + "/" + groupName + "/fields/" + field.get_name()];
        node["role"].set(static_cast<std::int8_t>(field.get_role()));
        node["type"].set(static_cast<std::int8_t>(field.get_type()));
        node["count"].set(static_cast<std::int64_t>(field.verify(data_size)));
        node["index"].set(static_cast<std::int64_t>(field.get_index()));
        node["component_count"].set(static_cast<std::int64_t>(num_components));
        node["storage"].set(field.raw_storage()->name());
        switch (field.get_type()) {
        case Ioss::Field::BasicType::DOUBLE:
          if (deep_copy) {
            node["value"].set(static_cast<double *>(data), num_to_get * num_components);
          }
          else {
            node["value"].set_external(static_cast<double *>(data), num_to_get * num_components);
          }
          break;
        case Ioss::Field::BasicType::INT32:
          if (deep_copy) {
            node["value"].set(static_cast<std::int32_t *>(data), num_to_get * num_components);
          }
          else {
            node["value"].set_external(static_cast<std::int32_t *>(data),
                                       num_to_get * num_components);
          }
          break;
        case Ioss::Field::BasicType::INT64:
          if (deep_copy) {
            node["value"].set(static_cast<std::int64_t *>(data), num_to_get * num_components);
          }
          else {
            node["value"].set_external(static_cast<std::int64_t *>(data),
                                       num_to_get * num_components);
          }
          break;
        case Ioss::Field::BasicType::CHARACTER:
          if (deep_copy) {
            node["value"].set(static_cast<std::int8_t *>(data), num_to_get * num_components);
          }
          else {
            node["value"].set_external(static_cast<std::int8_t *>(data),
                                       num_to_get * num_components);
          }
          break;
        default:
          fmt::print(stderr, "ERROR in {} {}: {} ({}), unsupported field type: {}\n", __func__,
                     containerName, field.get_name(), num_to_get, field.type_string());
        }
      }
      fmt::print(stderr, "put_field {}: {} ({})\n", containerName, field.get_name(), num_to_get);
      return num_to_get;
    }

    int64_t getField(const std::string &containerName, const Ioss::GroupingEntity *entityGroup,
                     const Ioss::Field &field, void *data, size_t data_size)
    {
      const auto groupName      = entityGroup->generic_name();
      auto       num_to_get     = field.verify(data_size);
      const auto num_components = field.raw_storage()->component_count();
      if (num_to_get > 0) {
        auto path = containerName + "/" + groupName + "/fields/" + field.get_name() + "/value";
        if (!this->DBNode.has_path(path)) {
          throw detail::FieldNonExistent();
          ;
        }
        const auto &&node = this->DBNode[path];
        switch (field.get_type()) {
        case Ioss::Field::BasicType::DOUBLE:
          std::copy_n(reinterpret_cast<const double *>(node.element_ptr(0)),
                      num_to_get * num_components, reinterpret_cast<double *>(data));
          break;

        case Ioss::Field::BasicType::INT32:
          std::copy_n(reinterpret_cast<const int32_t *>(node.element_ptr(0)),
                      num_to_get * num_components, reinterpret_cast<int32_t *>(data));
          break;

        case Ioss::Field::BasicType::INT64:
          std::copy_n(reinterpret_cast<const int64_t *>(node.element_ptr(0)),
                      num_to_get * num_components, reinterpret_cast<int64_t *>(data));
          break;

        case Ioss::Field::BasicType::CHARACTER:
          std::copy_n(reinterpret_cast<const char *>(node.element_ptr(0)),
                      num_to_get * num_components, reinterpret_cast<char *>(data));
          break;
        default:
          fmt::print(stderr, "ERROR in {} {}: {} ({}), unsupported field type: {}\n", __func__,
                     containerName, field.get_name(), num_to_get, field.type_string());
        }
      }
      fmt::print(stderr, "get_field {}: {} ({})\n", containerName, field.get_name(), num_to_get);
      return num_to_get;
    }

    Ioss::Map &get_node_map(const Ioss::DatabaseIO *dbase) const
    {
      if (this->NodeMap.defined()) {
        return this->NodeMap;
      }

      auto &&idsNode  = this->DBNode["node_blocks/nodeblock_1/fields/ids"];
      auto   node_ids = const_cast<void *>(idsNode["value"].element_ptr(0));
      this->NodeMap.set_size(idsNode["count"].as_int64());
      if (idsNode["type"].as_int8() == Ioss::Field::BasicType::INT32) {
        this->NodeMap.set_map(reinterpret_cast<int32_t *>(node_ids), idsNode["count"].as_int64(),
                              0);
      }
      if (idsNode["type"].as_int8() == Ioss::Field::BasicType::INT64) {
        this->NodeMap.set_map(reinterpret_cast<int64_t *>(node_ids), idsNode["count"].as_int64(),
                              0);
      }

      this->NodeMap.set_defined(true);
      return this->NodeMap;
    }

  private:
    template <typename GroupingEntityT>
    bool defineEntityGroup(conduit_cpp::Node                     parent,
                           const std::vector<GroupingEntityT *> &container)
    {
      for (auto group : container) {
        this->addProperties(parent[group->generic_name()], group);
      }
      return true;
    }

    template <typename GroupingEntityT>
    bool addProperties(conduit_cpp::Node parent, GroupingEntityT *entityGroup)
    {
      Ioss::NameList names;
      // skip implicit properties.
      entityGroup->property_describe(Ioss::Property::INTERNAL, &names);
      entityGroup->property_describe(Ioss::Property::EXTERNAL, &names);
      entityGroup->property_describe(Ioss::Property::ATTRIBUTE, &names);

      auto &&propertiesNode = parent["properties"];
      for (const auto &name : names) {
        auto property = entityGroup->get_property(name);

        auto &&node = propertiesNode[name];
        node["type"].set(static_cast<std::int8_t>(property.get_type()));
        node["origin"].set(static_cast<std::int8_t>(property.get_origin()));
        switch (property.get_type()) {
        case Ioss::Property::BasicType::REAL: node["value"].set(property.get_real()); break;

        case Ioss::Property::BasicType::INTEGER: node["value"].set(property.get_int()); break;

        case Ioss::Property::BasicType::STRING: node["value"].set(property.get_string()); break;

        case Ioss::Property::BasicType::VEC_INTEGER:
          node["value"].set(property.get_vec_int());
          break;

        case Ioss::Property::BasicType::VEC_DOUBLE:
          node["value"].set(property.get_vec_double());
          break;

        case Ioss::Property::BasicType::POINTER:
        case Ioss::Property::BasicType::INVALID:
        default: return false;
        }
      }
      return true;
    }

    template <typename GroupingEntityT>
    bool readEntityGroup(conduit_cpp::Node &&parent, Ioss::Region *region)
    {
      for (conduit_index_t idx = 0, max = parent.number_of_children(); idx < max; ++idx) {
        auto &&child = parent[idx];
        auto   block = detail::createEntityGroup<GroupingEntityT>(child, region->get_database());
        region->add(block);
        this->readProperties(child["properties"], block);

        // read fields (meta-data only)
        this->readFields(child["fields"], block);
      }
      return true;
    }

    template <typename GroupingEntityT>
    bool readProperties(const conduit_cpp::Node &&parent, GroupingEntityT *block) const
    {
      for (conduit_index_t idx = 0, max = parent.number_of_children(); idx < max; ++idx) {
        auto &&    child  = parent[idx];
        const auto name   = child.name();
        const auto origin = static_cast<Ioss::Property::Origin>(child["origin"].as_int8());
        switch (child["type"].as_int8()) {
        // TODO: missing origin
        case Ioss::Property::BasicType::REAL:
          block->property_add(Ioss::Property(name, child["value"].as_float64(), origin));
          break;

        case Ioss::Property::BasicType::INTEGER:
          block->property_add(Ioss::Property(name, child["value"].as_int64(), origin));
          break;

        case Ioss::Property::BasicType::STRING:
          block->property_add(Ioss::Property(name, child["value"].as_string(), origin));
          break;

        case Ioss::Property::BasicType::VEC_INTEGER:
        case Ioss::Property::BasicType::VEC_DOUBLE: abort(); // TODO:
        }
      }
      return true;
    }

    template <typename GroupingEntityT>
    bool readFields(const conduit_cpp::Node &&parent, GroupingEntityT *block) const
    {
      for (conduit_index_t idx = 0, max = parent.number_of_children(); idx < max; ++idx) {
        auto &&    child   = parent[idx];
        const auto name    = child.name();
        const auto type    = static_cast<Ioss::Field::BasicType>(child["type"].as_int8());
        const auto role    = static_cast<Ioss::Field::RoleType>(child["role"].as_int8());
        const auto count   = child["count"].as_int64();
        const auto index   = child["index"].as_int64();
        const auto storage = child["storage"].as_string();
        if (!block->field_exists(name)) {
          block->field_add(Ioss::Field(name, type, storage, role, count, index));
        }
        else {
          // TODO verify field details.
          assert(block->get_field(name).get_type() == type);
        }
      }

      return true;
    }
  };

  DatabaseIO::DatabaseIO(Ioss::Region *region, const std::string &filename,
                         Ioss::DatabaseUsage db_usage, MPI_Comm communicator,
                         const Ioss::PropertyManager &props)
      : Ioss::DatabaseIO(region, filename, db_usage, communicator, props),
        Impl(new DatabaseIO::ImplementationT()), useDeepCopy(true)

  {
    dbState = Ioss::STATE_UNKNOWN;
    //// Always 64 bits
    // dbIntSizeAPI = Ioss::USE_INT64_API;
    dbIntSizeAPI = Ioss::USE_INT32_API;

    bool shallowCopy = false;
    if (Ioss::Utils::check_set_bool_property(properties, "SHALLOW_COPY_FIELDS", shallowCopy)) {
      this->useDeepCopy = !shallowCopy;
    }

    if (is_input()) {
      auto &pm = get_property_manager();
      if (pm.exists("CATALYST_CONDUIT_NODE")) {
        auto c_node_ptr = reinterpret_cast<conduit_node *>(
            get_property_manager().get("CATALYST_CONDUIT_NODE").get_pointer());
        this->Impl->setDatabaseNode(c_node_ptr);
      }
      else {
        // we'll use filename as the location for the data dumps and read those.
        std::ostringstream path;
        path << filename << "/execute_invc43_params.conduit_bin." << util().parallel_size() << "."
             << util().parallel_rank();
        auto &root  = this->Impl->root();
        auto &dbase = this->Impl->databaseNode();
        conduit_node_load(conduit_cpp::c_node(&root), path.str().c_str(), "conduit_bin");
        conduit_node_set_external_node(
            conduit_cpp::c_node(&dbase),
            conduit_node_fetch(conduit_cpp::c_node(&root), "catalyst/channels/dataset/data"));
      }
    }
    else {
      // in output-mode, we're pass data on to Catalyst.
      conduit_cpp::Node node;

#if defined(SEACAS_HAVE_MPI)
      // pass communicator is using MPI.
      // TODO: skip for now.
      // node["catalyst"]["mpi_comm"].set(MPI_Comm_c2f(communicator));
#endif

      // TODO: Here, we need to pass pipeline scripts to execute to the Catalyst
      // implementation. ParaView Catalyst supports multiple scripts with args.
      // We could support that via the property manager. There are several
      // options:
      //
      // 1. the simulation can add bunch of properties that indicate which scripts
      //    to run and we use those.
      // 2. we could use the filename passed to the database as the
      //    configuration file that provides the scripts to runs.
      // 3. we could simply treat the filename passed to the database as the
      //    pipeline script.
      //
      // Here, I am using the simplest option #3. I do that since that makes it
      // easy for me to use existing tools like `io_shell` to test this database
      node["catalyst"]["scripts"]["sample0"]["filename"].set(filename);
      catalyst_initialize(conduit_cpp::c_node(&node));
    }
  }

  DatabaseIO::~DatabaseIO()
  {
    if (!is_input()) {
      conduit_cpp::Node node;
      catalyst_finalize(conduit_cpp::c_node(&node));
    }
  }

  bool DatabaseIO::begin__(Ioss::State state)
  {
    this->dbState = state;
    return true;
  }

  bool DatabaseIO::end__(Ioss::State state)
  {
    assert(this->dbState == state);

    if (!is_input()) {
      auto region = this->get_region();
      assert(region != nullptr);

      auto &impl = (*this->Impl.get());
      switch (state) {
      case Ioss::STATE_DEFINE_MODEL:
        // here the basic structure for the model is defined i.e.
        // number of blocks/sets/names etc.
        if (!impl.defineModel(region)) {
          return false;
        }
        break;

      case Ioss::STATE_MODEL:
        // here the model has meshdata e.g mesh fields, ids, coordinates etc.
        // impl.print();
        break;

      case Ioss::STATE_DEFINE_TRANSIENT: break;

      case Ioss::STATE_TRANSIENT: break;

      case Ioss::STATE_LAST_ENTRY: break;
      case Ioss::STATE_UNKNOWN:
      case Ioss::STATE_INVALID:
      case Ioss::STATE_READONLY:
      case Ioss::STATE_CLOSED: break;
      }
    }

    dbState = Ioss::STATE_UNKNOWN;
    return true;
  }

  bool DatabaseIO::begin_state__(int state, double time) { return true; }

  // common
  bool DatabaseIO::end_state__(int state, double time)
  {
    if (this->is_input()) {
    }
    else {
      // invoke catalyst.
      auto &impl = (*this->Impl.get());

      // state is 1-based, need to offset by 1 to make it 0-based.
      // timesteps start with 0.
      conduit_cpp::Node node;
      node["catalyst/state/timestep"].set(state - 1);
      node["catalyst/state/cycle"].set(state - 1);
      node["catalyst/state/time"].set(time);
      node["catalyst/channels/dataset/type"].set(std::string("ioss"));
      node["catalyst/channels/dataset/data"].set_external(impl.databaseNode());
      node["catalyst/channels/dataset/data/state_time"].set(time);
      catalyst_execute(conduit_cpp::c_node(&node));
    }
    return true;
  }

  unsigned DatabaseIO::entity_field_support() const
  {
    return Ioss::NODEBLOCK | Ioss::EDGEBLOCK | Ioss::FACEBLOCK | Ioss::ELEMENTBLOCK |
           Ioss::NODESET | Ioss::EDGESET | Ioss::FACESET | Ioss::ELEMENTSET | Ioss::SIDESET |
           Ioss::SIDEBLOCK | Ioss::REGION;
  }

  void DatabaseIO::read_meta_data__()
  {
    auto region = this->get_region();
    assert(region != nullptr);

    auto &impl = (*this->Impl.get());
    impl.readModel(region);
  }

  void DatabaseIO::get_step_times__()
  {
    auto region = this->get_region();
    assert(region != nullptr);

    auto &impl = (*this->Impl.get());
    impl.readTime(region);
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::NodeBlock *nb, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.putField("node_blocks", nb, field, data, data_size, this->deep_copy());
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::EdgeBlock *eb, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.putField("edge_blocks", eb, field, data, data_size, this->deep_copy());
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::FaceBlock *fb, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.putField("face_blocks", fb, field, data, data_size, this->deep_copy());
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::ElementBlock *eb, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.putField("element_blocks", eb, field, data, data_size, this->deep_copy());
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::SideBlock *sb, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.putField("sideblocks", sb, field, data, data_size, this->deep_copy());
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::NodeSet *ns, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.putField("nodesets", ns, field, data, data_size, this->deep_copy());
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::EdgeSet *es, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.putField("edgesets", es, field, data, data_size, this->deep_copy());
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::FaceSet *fs, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.putField("facesets", fs, field, data, data_size, this->deep_copy());
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::ElementSet *es, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.putField("elementsets", es, field, data, data_size, this->deep_copy());
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::SideSet *ss, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.putField("sidesets", ss, field, data, data_size, this->deep_copy());
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::CommSet *cs, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    return -1;
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::Assembly * /*as*/,
                                         const Ioss::Field & /*field*/, void * /*data*/,
                                         size_t /*data_size*/) const
  {
    return -1;
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::Blob * /*bl*/, const Ioss::Field & /*field*/,
                                         void * /*data*/, size_t /*data_size*/) const
  {
    return -1;
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::StructuredBlock *sb, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.putField("structured_blocks", sb, field, data, data_size, this->deep_copy());
  }

  int64_t DatabaseIO::get_field_internal(const Ioss::NodeBlock *nb, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.getField("node_blocks", nb, field, data, data_size);
  }

  int64_t DatabaseIO::get_field_internal(const Ioss::ElementBlock *eb, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    auto &impl = (*this->Impl.get());
    try {
      return impl.getField("element_blocks", eb, field, data, data_size);
    }
    catch (detail::FieldNonExistent &) {
      if (field.get_name() == "connectivity_raw") {
        // maybe the data has 'connectivity' provided, so we convert it to 'connectivity_raw'.
        auto count = this->get_field_internal(eb, eb->get_field("connectivity"), data, data_size);
        if (count <= 0) {
          return count;
        }

        impl.get_node_map(this).reverse_map_data(
            data, field, field.verify(data_size) * field.raw_storage()->component_count());
        return count;
      }
      else if (field.get_name() == "connectivity") {
        // maybe the data has 'connectivity_raw' is provided, so we convert it to 'connectivity.
        auto count =
            this->get_field_internal(eb, eb->get_field("connectivity_raw"), data, data_size);
        if (count <= 0) {
          return count;
        }

        impl.get_node_map(this).map_data(
            data, field, field.verify(data_size) * field.raw_storage()->component_count());
        return count;
      }
      return -1;
    }
  }

  int64_t DatabaseIO::get_field_internal(const Ioss::EdgeBlock *eb, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.getField("edge_blocks", eb, field, data, data_size);
  }

  int64_t DatabaseIO::get_field_internal(const Ioss::FaceBlock *fb, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.getField("face_blocks", fb, field, data, data_size);
  }
  int64_t DatabaseIO::get_field_internal(const Ioss::SideBlock *fb, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    return -1;
  }
  int64_t DatabaseIO::get_field_internal(const Ioss::NodeSet *ns, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.getField("nodesets", ns, field, data, data_size);
  }
  int64_t DatabaseIO::get_field_internal(const Ioss::EdgeSet *ns, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.getField("edgesets", ns, field, data, data_size);
  }
  int64_t DatabaseIO::get_field_internal(const Ioss::FaceSet *ns, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.getField("facesets", ns, field, data, data_size);
  }
  int64_t DatabaseIO::get_field_internal(const Ioss::ElementSet *ns, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.getField("elementsets", ns, field, data, data_size);
  }
  int64_t DatabaseIO::get_field_internal(const Ioss::SideSet *fs, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.getField("sidesets", fs, field, data, data_size);
  }
  int64_t DatabaseIO::get_field_internal(const Ioss::CommSet *cs, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    return -1;
  }
  int64_t DatabaseIO::get_field_internal(const Ioss::Assembly * /*as*/,
                                         const Ioss::Field & /*field*/, void * /*data*/,
                                         size_t /*data_size*/) const
  {
    return -1;
  }
  int64_t DatabaseIO::get_field_internal(const Ioss::Blob * /*bl*/, const Ioss::Field & /*field*/,
                                         void * /*data*/, size_t /*data_size*/) const
  {
    return -1;
  }
  int64_t DatabaseIO::get_field_internal(const Ioss::StructuredBlock *sb, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.getField("structured_blocks", sb, field, data, data_size);
  }

} // namespace Iocatalyst
