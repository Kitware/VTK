// Copyright(C) 1999-2020 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_CommSet.h"         // for CommSet
#include "Ioss_DBUsage.h"         // for DatabaseUsage
#include "Ioss_DatabaseIO.h"      // for DatabaseIO
#include "Ioss_ElementBlock.h"    // for ElementBlock
#include "Ioss_EntityType.h"      // for EntityType, etc
#include "Ioss_Field.h"           // for Field, etc
#include "Ioss_GroupingEntity.h"  // for GroupingEntity
#include "Ioss_IOFactory.h"       // for IOFactory
#include "Ioss_Map.h"             // for Map, MapContainer
#include "Ioss_NodeBlock.h"       // for NodeBlock
#include "Ioss_ParallelUtils.h"   // for ParallelUtils
#include "Ioss_Property.h"        // for Property
#include "Ioss_PropertyManager.h" // for PropertyManager
#include "Ioss_Region.h"          // for Region
#include "Ioss_SideSet.h"         // for SideSet
#include "Ioss_StructuredBlock.h"
#include "Ioss_VariableType.h" // for VariableType
#include <Ioss_CodeTypes.h>    // for Int64Vector, IntVector
#include <Ioss_SideBlock.h>    // for SideBlock
#include <Ioss_Utils.h>        // for Utils, IOSS_ERROR
#include <algorithm>           // for copy
#include <cassert>             // for assert
#include <cmath>               // for sqrt
#include <gen_struc/Iogs_DatabaseIO.h>
#include <gen_struc/Iogs_GeneratedMesh.h> // for GeneratedMesh
#include <iostream>                       // for ostringstream, operator<<, etc
#include <string>                         // for string, operator==, etc
#include <utility>                        // for pair
namespace Ioss {
  class EdgeBlock;
} // namespace Ioss
namespace Ioss {
  class EdgeSet;
} // namespace Ioss
namespace Ioss {
  class ElementSet;
} // namespace Ioss
namespace Ioss {
  class FaceBlock;
} // namespace Ioss
namespace Ioss {
  class FaceSet;
} // namespace Ioss

namespace {
  template <typename INT>
  void map_global_to_local(const Ioss::Map &map, size_t count, size_t stride, INT *data)
  {
    for (size_t i = 0; i < count; i += stride) {
      int64_t local = map.global_to_local(data[i], true);
      data[i]       = local;
    }
  }

  template <typename INT>
  void fill_transient_data(size_t component_count, double *data, INT *ids, size_t count,
                           double offset = 0.0)
  {
    if (component_count == 1) {
      for (size_t i = 0; i < count; i++) {
        data[i] = std::sqrt((double)ids[i]) + offset;
      }
    }
    else {
      for (size_t i = 0; i < count; i++) {
        for (size_t j = 0; j < component_count; j++) {
          data[i * component_count + j] = j + std::sqrt((double)ids[i]) + offset;
        }
      }
    }
  }

  void fill_transient_data(const Ioss::GroupingEntity *entity, const Ioss::Field &field, void *data,
                           void *id_data, size_t count, double offset = 0.0)
  {
    const Ioss::Field &ids = entity->get_fieldref("ids");
    if (ids.is_type(Ioss::Field::INTEGER)) {
      fill_transient_data(field.raw_storage()->component_count(), reinterpret_cast<double *>(data),
                          reinterpret_cast<int *>(id_data), count, offset);
    }
    else {
      fill_transient_data(field.raw_storage()->component_count(), reinterpret_cast<double *>(data),
                          reinterpret_cast<int64_t *>(id_data), count, offset);
    }
  }

  void fill_constant_data(const Ioss::Field &field, void *data, double value)
  {
    auto * rdata           = reinterpret_cast<double *>(data);
    size_t count           = field.raw_count();
    size_t component_count = field.raw_storage()->component_count();
    for (size_t i = 0; i < count * component_count; i++) {
      rdata[i] = value;
    }
  }
} // namespace
namespace Iogs {

  // ========================================================================
  const IOFactory *IOFactory::factory()
  {
    static IOFactory registerThis;
    return &registerThis;
  }

  IOFactory::IOFactory() : Ioss::IOFactory("gen_struc") {}

  Ioss::DatabaseIO *IOFactory::make_IO(const std::string &filename, Ioss::DatabaseUsage db_usage,
                                       MPI_Comm                     communicator,
                                       const Ioss::PropertyManager &props) const
  {
    return new DatabaseIO(nullptr, filename, db_usage, communicator, props);
  }

  // ========================================================================
  DatabaseIO::DatabaseIO(Ioss::Region *region, const std::string &filename,
                         Ioss::DatabaseUsage db_usage, MPI_Comm communicator,
                         const Ioss::PropertyManager &props)
      : Ioss::DatabaseIO(region, filename, db_usage, communicator, props)
  {
    if (is_input()) {
      dbState = Ioss::STATE_UNKNOWN;
    }
    else {
      std::ostringstream errmsg;
      errmsg << "ERROR: Structured Generated mesh option is only valid for input mesh.";
      IOSS_ERROR(errmsg);
    }
    if (props.exists("USE_CONSTANT_DF")) {
      m_useVariableDf = false;
    }
    if (util().parallel_size() > 1) {
      std::ostringstream errmsg;
      errmsg << "ERROR: Structured Generated mesh option is not valid for parallel yet.";
      IOSS_ERROR(errmsg);
    }
  }

  DatabaseIO::~DatabaseIO() { delete m_generatedMesh; }

  void DatabaseIO::read_meta_data__()
  {
    if (m_generatedMesh == nullptr) {
      if (get_filename() == "external") {
        std::ostringstream errmsg;
        errmsg << "ERROR: (gen_struc mesh) 'external' specified for mesh, but "
               << "getGeneratedMesh was not called to set the external mesh.\n";
        IOSS_ERROR(errmsg);
      }
      else {
        m_generatedMesh =
            new GeneratedMesh(get_filename(), util().parallel_size(), util().parallel_rank());
      }
    }

    assert(m_generatedMesh != nullptr);

    Ioss::Region *this_region = get_region();
    this_region->property_add(Ioss::Property("global_node_count", m_generatedMesh->node_count()));
    this_region->property_add(
        Ioss::Property("global_element_count", m_generatedMesh->element_count()));

    spatialDimension = 3;
    nodeCount        = m_generatedMesh->node_count_proc();
    elementCount     = m_generatedMesh->element_count_proc();

    get_step_times__();

    add_transient_fields(this_region);
    get_nodeblocks();
    get_structured_blocks();
    get_sidesets();

    this_region->property_add(
        Ioss::Property(std::string("title"), std::string("GeneratedMesh: ") += get_filename()));
  }

  bool DatabaseIO::begin__(Ioss::State /* state */) { return true; }

  bool DatabaseIO::end__(Ioss::State /* state */) { return true; }

  bool DatabaseIO::begin_state__(int /* state */, double time)
  {
    currentTime = time;
    return true;
  }

  int64_t DatabaseIO::get_field_internal(const Ioss::NodeBlock *nb, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    size_t num_to_get = field.verify(data_size);

    Ioss::Field::RoleType role = field.get_role();
    if (role == Ioss::Field::MESH) {
      if (field.get_name() == "mesh_model_coordinates") {
        // Cast 'data' to correct size -- double
        auto *rdata = static_cast<double *>(data);
        m_generatedMesh->coordinates(rdata);
      }

      // NOTE: The implicit_ids field is ONLY provided for backward-
      // compatibility and should not be used unless absolutely
      // required. For gen_struc mesh, the implicit_ids and ids are the same.
      else if (field.get_name() == "ids" || field.get_name() == "implicit_ids") {
        // Map the local ids in this node block
        // (1...node_count) to global node ids.
        get_node_map().map_implicit_data(data, field, num_to_get, 0);
      }
      else if (field.get_name() == "owning_processor") {
        int *owner = static_cast<int *>(data);
        m_generatedMesh->owning_processor(owner, num_to_get);
      }
      else if (field.get_name() == "connectivity") {
        // Do nothing, just handles an idiosyncrasy of the GroupingEntity
      }
      else if (field.get_name() == "connectivity_raw") {
        // Do nothing, just handles an idiosyncrasy of the GroupingEntity
      }
      else {
        num_to_get = Ioss::Utils::field_warning(nb, field, "input");
      }
      return num_to_get;
    }

    const Ioss::Field &id_fld = nb->get_fieldref("ids");
    std::vector<char>  ids(id_fld.get_size());
    get_field_internal(nb, id_fld, ids.data(), id_fld.get_size());
    fill_transient_data(nb, field, data, ids.data(), num_to_get, currentTime);

    return num_to_get;
  }

  int64_t DatabaseIO::get_field_internal(const Ioss::Region * /*region*/, const Ioss::Field &field,
                                         void *data, size_t /*data_size*/) const
  {
    Ioss::Field::RoleType role = field.get_role();
    if (role == Ioss::Field::TRANSIENT) {
      // Fill the field with arbitrary data...
      (reinterpret_cast<double *>(data))[0] = static_cast<double>(rand());
    }
    return 1;
  }

  int64_t DatabaseIO::get_field_internal(const Ioss::StructuredBlock *sb, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    Ioss::Field::RoleType role = field.get_role();
    int                   zone = sb->get_property("zone").get_int();

    int64_t num_to_get = field.verify(data_size);
    auto *  rdata      = static_cast<double *>(data);

    if (role == Ioss::Field::MESH) {
      if (field.get_name() == "mesh_model_coordinates_x") {
        m_generatedMesh->coordinates(1, zone, rdata);
      }

      else if (field.get_name() == "mesh_model_coordinates_y") {
        m_generatedMesh->coordinates(2, zone, rdata);
      }

      else if (field.get_name() == "mesh_model_coordinates_z") {
        m_generatedMesh->coordinates(3, zone, rdata);
      }

      else if (field.get_name() == "mesh_model_coordinates") {
        m_generatedMesh->coordinates(0, zone, rdata);
      }
      else if (field.get_name() == "cell_node_ids") {
        if (field.get_type() == Ioss::Field::INT64) {
          int64_t *idata = static_cast<int64_t *>(data);
          sb->get_cell_node_ids(idata, true);
        }
        else {
          assert(field.get_type() == Ioss::Field::INT32);
          int *idata = static_cast<int *>(data);
          sb->get_cell_node_ids(idata, true);
        }
      }
      else if (field.get_name() == "cell_ids") {
        if (field.get_type() == Ioss::Field::INT64) {
          int64_t *idata = static_cast<int64_t *>(data);
          sb->get_cell_ids(idata, true);
        }
        else {
          assert(field.get_type() == Ioss::Field::INT32);
          int *idata = static_cast<int *>(data);
          sb->get_cell_ids(idata, true);
        }
      }
      else {
        num_to_get = Ioss::Utils::field_warning(sb, field, "input");
      }
    }
    else if (role == Ioss::Field::TRANSIENT) {
    }
    else {
      num_to_get = Ioss::Utils::field_warning(sb, field, "input");
    }
    return num_to_get;
  }

  int64_t DatabaseIO::get_field_internal(const Ioss::SideBlock *ef_blk, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    size_t num_to_get = field.verify(data_size);

    int64_t id           = ef_blk->get_property("id").get_int();
    size_t  entity_count = ef_blk->entity_count();
    if (num_to_get != entity_count) {
      std::ostringstream errmsg;
      errmsg << "ERROR: Partial field input not implemented for side blocks";
      IOSS_ERROR(errmsg);
    }

    Ioss::Field::RoleType role = field.get_role();
    if (role == Ioss::Field::MESH) {

      if (field.get_name() == "ids") {
        // A sideset' is basically an exodus sideset.  A
        // sideset has a list of elements and a corresponding local
        // element side (1-based) The side id is: side_id =
        // 10*element_id + local_side_number This assumes that all
        // sides in a sideset are boundary sides.
        std::vector<int64_t> elem_side;
        m_generatedMesh->sideset_elem_sides(id, elem_side);
        if (field.is_type(Ioss::Field::INTEGER)) {
          int *ids = static_cast<int *>(data);
          for (size_t i = 0; i < num_to_get; i++) {
            ids[i] = 10 * elem_side[2 * i + 0] + elem_side[2 * i + 1] + 1;
          }
        }
        else {
          int64_t *ids = static_cast<int64_t *>(data);
          for (size_t i = 0; i < num_to_get; i++) {
            ids[i] = 10 * elem_side[2 * i + 0] + elem_side[2 * i + 1] + 1;
          }
        }
      }

      else if (field.get_name() == "element_side" || field.get_name() == "element_side_raw") {
        // Since we only have a single array, we need to allocate an extra
        // array to store all of the data.  Note also that the element_id
        // is the global id but only the local id is stored so we need to
        // map from local_to_global prior to generating the side id...

        std::vector<int64_t> elem_side;
        m_generatedMesh->sideset_elem_sides(id, elem_side);
        if (field.get_name() == "element_side_raw") {
          map_global_to_local(get_element_map(), elem_side.size(), 2, &elem_side[0]);
        }

        if (field.is_type(Ioss::Field::INTEGER)) {
          int *element_side = static_cast<int *>(data);
          for (size_t i = 0; i < num_to_get; i++) {
            element_side[2 * i + 0] = elem_side[2 * i + 0];
            element_side[2 * i + 1] = elem_side[2 * i + 1] + 1;
          }
        }
        else {
          int64_t *element_side = static_cast<int64_t *>(data);
          for (size_t i = 0; i < num_to_get; i++) {
            element_side[2 * i + 0] = elem_side[2 * i + 0];
            element_side[2 * i + 1] = elem_side[2 * i + 1] + 1;
          }
        }
      }

      else if (field.get_name() == "distribution_factors") {
        if (m_useVariableDf) {
          const Ioss::Field &id_fld = ef_blk->get_fieldref("ids");
          std::vector<char>  ids(id_fld.get_size());
          get_field_internal(ef_blk, id_fld, ids.data(), id_fld.get_size());
          fill_transient_data(ef_blk, field, data, ids.data(), num_to_get);
        }
        else {
          fill_constant_data(field, data, 1.0);
        }
      }

      else {
        num_to_get = Ioss::Utils::field_warning(ef_blk, field, "input");
      }
    }
    else if (role == Ioss::Field::TRANSIENT) {
      const Ioss::Field &id_fld = ef_blk->get_fieldref("ids");
      std::vector<char>  ids(id_fld.get_size());
      get_field_internal(ef_blk, id_fld, ids.data(), id_fld.get_size());
      fill_transient_data(ef_blk, field, data, ids.data(), num_to_get, currentTime);
    }
    return num_to_get;
  }

  int64_t DatabaseIO::get_field_internal(const Ioss::NodeSet * /* ns */,
                                         const Ioss::Field & /* field */, void * /* data */,
                                         size_t /* data_size */) const
  {
    return -1;
  }

  int64_t DatabaseIO::get_field_internal(const Ioss::EdgeBlock * /* fs */,
                                         const Ioss::Field & /* field */, void * /* data */,
                                         size_t /* data_size */) const
  {
    return -1;
  }
  int64_t DatabaseIO::get_field_internal(const Ioss::FaceBlock * /* fs */,
                                         const Ioss::Field & /* field */, void * /* data */,
                                         size_t /* data_size */) const
  {
    return -1;
  }
  int64_t DatabaseIO::get_field_internal(const Ioss::EdgeSet * /* fs */,
                                         const Ioss::Field & /* field */, void * /* data */,
                                         size_t /* data_size */) const
  {
    return -1;
  }
  int64_t DatabaseIO::get_field_internal(const Ioss::FaceSet * /* fs */,
                                         const Ioss::Field & /* field */, void * /* data */,
                                         size_t /* data_size */) const
  {
    return -1;
  }
  int64_t DatabaseIO::get_field_internal(const Ioss::ElementSet * /* fs */,
                                         const Ioss::Field & /* field */, void * /* data */,
                                         size_t /* data_size */) const
  {
    return -1;
  }

  int64_t DatabaseIO::get_field_internal(const Ioss::SideSet * /* fs */,
                                         const Ioss::Field & /* field */, void * /* data */,
                                         size_t /* data_size */) const
  {
    return -1;
  }
  int64_t DatabaseIO::get_field_internal(const Ioss::CommSet *cs, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    size_t num_to_get   = field.verify(data_size);
    size_t entity_count = cs->entity_count();
    assert(num_to_get == entity_count);

    // Return the <entity (node or face), processor> pair
    if (field.get_name() == "entity_processor" || field.get_name() == "entity_processor_raw") {

      // Check type -- node or face
      std::string type = cs->get_property("entity_type").get_string();

      if (type == "node") {
        // Allocate temporary storage space
        Ioss::Int64Vector entities(num_to_get);
        Ioss::IntVector   procs(num_to_get);
        m_generatedMesh->node_communication_map(entities, procs);

        // and store in 'data' ...
        if (field.is_type(Ioss::Field::INTEGER)) {
          int *entity_proc = static_cast<int *>(data);

          size_t j = 0;
          for (size_t i = 0; i < entity_count; i++) {
            assert(entities[i] > 0);
            entity_proc[j++] = entities[i];
            entity_proc[j++] = procs[i];
          }

          if (field.get_name() == "entity_processor_raw") {
            map_global_to_local(get_node_map(), 2 * entity_count, 2, entity_proc);
          }
        }
        else {
          int64_t *entity_proc = static_cast<int64_t *>(data);

          size_t j = 0;
          for (size_t i = 0; i < entity_count; i++) {
            assert(entities[i] > 0);
            entity_proc[j++] = entities[i];
            entity_proc[j++] = procs[i];
          }

          if (field.get_name() == "entity_processor_raw") {
            map_global_to_local(get_node_map(), 2 * entity_count, 2, entity_proc);
          }
        }
      }
      else {
        std::ostringstream errmsg;
        errmsg << "Invalid commset type " << type;
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

  // Input only database -- these will never be called...
  int64_t DatabaseIO::put_field_internal(const Ioss::Region * /*reg*/,
                                         const Ioss::Field & /*field*/, void * /*data*/,
                                         size_t /*data_size*/) const
  {
    return -1;
  }
  int64_t DatabaseIO::put_field_internal(const Ioss::ElementBlock * /*eb*/,
                                         const Ioss::Field & /*field*/, void * /*data*/,
                                         size_t /*data_size*/) const
  {
    return -1;
  }
  int64_t DatabaseIO::put_field_internal(const Ioss::FaceBlock * /*nb*/,
                                         const Ioss::Field & /*field*/, void * /*data*/,
                                         size_t /*data_size*/) const
  {
    return -1;
  }
  int64_t DatabaseIO::put_field_internal(const Ioss::EdgeBlock * /*nb*/,
                                         const Ioss::Field & /*field*/, void * /*data*/,
                                         size_t /*data_size*/) const
  {
    return -1;
  }
  int64_t DatabaseIO::put_field_internal(const Ioss::NodeBlock * /*nb*/,
                                         const Ioss::Field & /*field*/, void * /*data*/,
                                         size_t /*data_size*/) const
  {
    return -1;
  }
  int64_t DatabaseIO::put_field_internal(const Ioss::ElementSet * /*ns*/,
                                         const Ioss::Field & /*field*/, void * /*data*/,
                                         size_t /*data_size*/) const
  {
    return -1;
  }
  int64_t DatabaseIO::put_field_internal(const Ioss::FaceSet * /*ns*/,
                                         const Ioss::Field & /*field*/, void * /*data*/,
                                         size_t /*data_size*/) const
  {
    return -1;
  }
  int64_t DatabaseIO::put_field_internal(const Ioss::EdgeSet * /*ns*/,
                                         const Ioss::Field & /*field*/, void * /*data*/,
                                         size_t /*data_size*/) const
  {
    return -1;
  }
  int64_t DatabaseIO::put_field_internal(const Ioss::NodeSet * /*ns*/,
                                         const Ioss::Field & /*field*/, void * /*data*/,
                                         size_t /*data_size*/) const
  {
    return -1;
  }
  int64_t DatabaseIO::put_field_internal(const Ioss::SideSet * /*fs*/,
                                         const Ioss::Field & /*field*/, void * /*data*/,
                                         size_t /*data_size*/) const
  {
    return -1;
  }
  int64_t DatabaseIO::put_field_internal(const Ioss::SideBlock * /*fb*/,
                                         const Ioss::Field & /*field*/, void * /*data*/,
                                         size_t /*data_size*/) const
  {
    return -1;
  }
  int64_t DatabaseIO::put_field_internal(const Ioss::CommSet * /*cs*/,
                                         const Ioss::Field & /*field*/, void * /*data*/,
                                         size_t /*data_size*/) const
  {
    return -1;
  }

  const Ioss::Map &DatabaseIO::get_node_map() const
  {
    // Allocate space for node number map and read it in...
    // Can be called multiple times, allocate 1 time only
    if (nodeMap.map().empty()) {
      nodeMap.set_size(nodeCount);
      std::vector<int64_t> map;
      m_generatedMesh->node_map(map);
      nodeMap.set_map(map.data(), map.size(), 0, true);
    }
    return nodeMap;
  }

  const Ioss::Map &DatabaseIO::get_element_map() const
  {
    // Allocate space for element number map and read it in...
    // Can be called multiple times, allocate 1 time only
    if (elemMap.map().empty()) {
      elemMap.set_size(elementCount);
      std::vector<int64_t> map;
      m_generatedMesh->element_map(map);
      elemMap.set_map(map.data(), map.size(), 0, true);
    }
    return elemMap;
  }

  void DatabaseIO::get_nodeblocks()
  {
    std::string block_name = "nodeblock_1";
    auto block = new Ioss::NodeBlock(this, block_name, m_generatedMesh->node_count_proc(), 3);
    block->property_add(Ioss::Property("id", 1));
    block->property_add(Ioss::Property("guid", util().generate_guid(1)));
    get_region()->add(block);
    add_transient_fields(block);
  }

  void DatabaseIO::get_step_times__()
  {
    int time_step_count = m_generatedMesh->timestep_count();
    for (int i = 0; i < time_step_count; i++) {
      get_region()->add_state(i);
    }
  }

  void DatabaseIO::get_structured_blocks()
  {
    // Name, global range, local offset, local range.
    int block_count = m_generatedMesh->structured_block_count();
    for (int i = 0; i < block_count; i++) {
      std::string name   = Ioss::Utils::encode_entity_name("block", i + 1);
      Ioss::IJK_t global = m_generatedMesh->block_range(i + 1);
      auto        block = new Ioss::StructuredBlock(this, name, 3, global[0], global[1], global[2]);

      block->property_add(Ioss::Property("base", 1));
      block->property_add(Ioss::Property("zone", i + 1));
      block->property_add(Ioss::Property("id", i + 1));
      block->property_add(Ioss::Property("guid", i + 1));
      get_region()->add(block);
    }
  }

  void DatabaseIO::get_sidesets()
  {
    m_sideset_names.reserve(sidesetCount);
    for (int ifs = 0; ifs < sidesetCount; ifs++) {
      std::string name = Ioss::Utils::encode_entity_name("surface", ifs + 1);
      m_sideset_names.push_back(name);
      auto sideset = new Ioss::SideSet(this, name);
      sideset->property_add(Ioss::Property("id", ifs + 1));
      sideset->property_add(Ioss::Property("guid", util().generate_guid(ifs + 1)));
      get_region()->add(sideset);

      std::vector<std::string> touching_blocks = m_generatedMesh->sideset_touching_blocks(ifs + 1);
      if (touching_blocks.size() == 1) {
        std::string ef_block_name  = name + "_quad4";
        std::string side_topo_name = "quad4";
        std::string elem_topo_name = "unknown";
        int64_t     number_faces   = m_generatedMesh->sideset_side_count_proc(ifs + 1);

        auto ef_block =
            new Ioss::SideBlock(this, ef_block_name, side_topo_name, elem_topo_name, number_faces);
        sideset->add(ef_block);
        ef_block->property_add(Ioss::Property("id", ifs + 1));
        ef_block->property_add(Ioss::Property("guid", util().generate_guid(ifs + 1)));

        std::string storage = "Real[";
        storage += std::to_string(4);
        storage += "]";
        ef_block->field_add(Ioss::Field("distribution_factors", Ioss::Field::REAL, storage,
                                        Ioss::Field::MESH, number_faces));

        Ioss::ElementBlock *el_block = get_region()->get_element_block(touching_blocks[0]);
        ef_block->set_parent_element_block(el_block);
        add_transient_fields(ef_block);
      }
      else {
        for (auto &touching_block : touching_blocks) {
          std::string ef_block_name =
              "surface_" + touching_block + "_edge2_" + std::to_string(ifs + 1);
          std::string side_topo_name = "quad4";
          std::string elem_topo_name = "unknown";
          int64_t     number_faces   = m_generatedMesh->sideset_side_count_proc(ifs + 1);

          auto ef_block = new Ioss::SideBlock(this, ef_block_name, side_topo_name, elem_topo_name,
                                              number_faces);
          sideset->add(ef_block);
          ef_block->property_add(Ioss::Property("id", ifs + 1));
          ef_block->property_add(Ioss::Property("guid", util().generate_guid(ifs + 1)));

          std::string storage = "Real[";
          storage += std::to_string(4);
          storage += "]";
          ef_block->field_add(Ioss::Field("distribution_factors", Ioss::Field::REAL, storage,
                                          Ioss::Field::MESH, number_faces));

          Ioss::ElementBlock *el_block = get_region()->get_element_block(touching_block);
          ef_block->set_parent_element_block(el_block);
          add_transient_fields(ef_block);
        }
      }
    }
  }

  unsigned DatabaseIO::entity_field_support() const
  {
    return Ioss::NODEBLOCK | Ioss::STRUCTUREDBLOCK | Ioss::REGION | Ioss::SIDESET;
  }

  void DatabaseIO::add_transient_fields(Ioss::GroupingEntity *entity)
  {
    Ioss::EntityType type         = entity->type();
    size_t           entity_count = entity->entity_count();
    size_t           var_count    = m_generatedMesh->get_variable_count(type);
    for (size_t i = 0; i < var_count; i++) {
      std::string var_name = entity->type_string() + "_" + std::to_string(i + 1);
      entity->field_add(
          Ioss::Field(var_name, Ioss::Field::REAL, "scalar", Ioss::Field::TRANSIENT, entity_count));
    }
  }
} // namespace Iogs
