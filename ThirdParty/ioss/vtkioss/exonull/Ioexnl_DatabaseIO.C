// Copyright(C) 1999-2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_CodeTypes.h"
#include "Ioss_ParallelUtils.h"
#include "Ioss_SmartAssert.h"
#include "Ioss_Utils.h"
#include "exonull/Ioexnl_DatabaseIO.h"
#include "exonull/Ioexnl_Internals.h"
#include "exonull/Ioexnl_Utils.h"
#include <array>
#include <cassert>
#include <ctime>
#include <vtk_exodusII.h>
#include "vtk_fmt.h"
#include VTK_FMT(fmt/ostream.h)

#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "Ioexnl_BaseDatabaseIO.h"
#include "Ioss_Assembly.h"
#include "Ioss_Blob.h"
#include "Ioss_CommSet.h"
#include "Ioss_DBUsage.h"
#include "Ioss_EdgeBlock.h"
#include "Ioss_EdgeSet.h"
#include "Ioss_ElementBlock.h"
#include "Ioss_ElementSet.h"
#include "Ioss_ElementTopology.h"
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
#include "Ioss_PropertyManager.h"
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

namespace Ioexnl {
  DatabaseIO::DatabaseIO(Ioss::Region *region, const std::string &filename,
                         Ioss::DatabaseUsage db_usage, Ioss_MPI_Comm communicator,
                         const Ioss::PropertyManager &props)
      : Ioexnl::BaseDatabaseIO(region, filename, db_usage, communicator, props)
  {
  }

  bool DatabaseIO::check_valid_file_ptr(bool, std::string *, int *, bool) const { return true; }

  bool DatabaseIO::handle_output_file(bool, std::string *, int *, bool, bool) const { return true; }

  int DatabaseIO::get_file_pointer() const { return 0; }

  void DatabaseIO::read_meta_data_nl() {}

  void DatabaseIO::read_region() {}

  void DatabaseIO::get_step_times_nl() {}

  int64_t DatabaseIO::write_attribute_field(const Ioss::Field          &field,
                                            const Ioss::GroupingEntity *ge, void *data) const
  {
    int64_t num_entity = ge->entity_count();
    int64_t fld_offset = field.get_index();

    int attribute_count = ge->get_property("attribute_count").get_int();
    assert(fld_offset > 0);
    assert(fld_offset - 1 + field.get_component_count(Ioss::Field::InOut::OUTPUT) <=
           attribute_count);

    Ioss::Field::BasicType ioss_type = field.get_type();
    assert(ioss_type == Ioss::Field::REAL || ioss_type == Ioss::Field::INTEGER ||
           ioss_type == Ioss::Field::INT64);

    if (ioss_type == Ioss::Field::INT64) {
      Ioss::Utils::check_int_to_real_overflow(field, (int64_t *)data, num_entity);
    }

    if (fld_offset == 1 &&
        field.get_component_count(Ioss::Field::InOut::OUTPUT) == attribute_count) {
      // Write all attributes in one big chunk...
      std::vector<double> temp;
      if (ioss_type == Ioss::Field::INTEGER) {
        int *idata = static_cast<int *>(data);
        extract_data(temp, idata, attribute_count * num_entity, 1, 0);
      }
      else if (ioss_type == Ioss::Field::INT64) {
        auto *idata = static_cast<int64_t *>(data);
        extract_data(temp, idata, attribute_count * num_entity, 1, 0);
      }
    }
    else {
      // Write a subset of the attributes.  If scalar, write one;
      // if higher-order (vector3d, ..) write each component.
      if (field.get_component_count(Ioss::Field::InOut::OUTPUT) == 1) {
        std::vector<double> temp;
        if (ioss_type == Ioss::Field::INTEGER) {
          int *idata = static_cast<int *>(data);
          extract_data(temp, idata, num_entity, 1, 0);
        }
        else if (ioss_type == Ioss::Field::INT64) {
          auto *idata = static_cast<int64_t *>(data);
          extract_data(temp, idata, num_entity, 1, 0);
        }
      }
      else {
        // Multi-component...  Need a local memory space to push
        // data into and then write that out to the file...
        std::vector<double> local_data(num_entity);
        int                 comp_count = field.get_component_count(Ioss::Field::InOut::OUTPUT);
        for (int i = 0; i < comp_count; i++) {
          size_t offset = i;
          if (ioss_type == Ioss::Field::REAL) {
            auto *rdata = static_cast<double *>(data);
            extract_data(local_data, rdata, num_entity, comp_count, offset);
          }
          else if (ioss_type == Ioss::Field::INTEGER) {
            int *idata = static_cast<int *>(data);
            extract_data(local_data, idata, num_entity, comp_count, offset);
          }
          else if (ioss_type == Ioss::Field::INT64) {
            auto *idata = static_cast<int64_t *>(data);
            extract_data(local_data, idata, num_entity, comp_count, offset);
          }
        }
      }
    }
    return num_entity;
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::Region *reg, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    return Ioexnl::BaseDatabaseIO::put_field_internal(reg, field, data, data_size);
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::NodeBlock *nb, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    {
      size_t num_to_get = field.verify(data_size);
      if (num_to_get > 0) {

        Ioss::Field::RoleType role = field.get_role();

        if (role == Ioss::Field::MESH) {
          if (field.get_name() == "mesh_model_coordinates_x") {}

          else if (field.get_name() == "mesh_model_coordinates_y") {
          }

          else if (field.get_name() == "mesh_model_coordinates_z") {
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
            auto *rdata = static_cast<double *>(data);

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
          write_nodal_transient_field(field, nb, num_to_get, data);
        }
        else if (role == Ioss::Field::REDUCTION) {
          store_reduction_field(field, nb, data);
        }
        else if (role == Ioss::Field::ATTRIBUTE) {
          num_to_get = write_attribute_field(field, nb, data);
        }
      }
      return num_to_get;
    }
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::Blob *blob, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    {
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

  int64_t DatabaseIO::put_field_internal(const Ioss::Assembly *assembly, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    {
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

  int64_t DatabaseIO::put_field_internal(const Ioss::ElementBlock *eb, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    {
      size_t num_to_get = field.verify(data_size);

      if (num_to_get > 0) {
        // Get the element block id and element count
        size_t                my_element_count = eb->entity_count();
        Ioss::Field::RoleType role             = field.get_role();

        if (role == Ioss::Field::MESH) {
          // Handle the MESH fields required for an Exodus file model.
          // (The 'genesis' portion)
          if (field.get_name() == "connectivity") {
            if (my_element_count > 0) {
              // Map element connectivity from global node id to local node id.
              int element_nodes = eb->topology()->number_nodes();
              nodeMap.reverse_map_data(data, field, num_to_get * element_nodes);
            }
          }
          else if (field.get_name() == "connectivity_edge") {
            if (my_element_count > 0) {
              // Map element connectivity from global edge id to local edge id.
              int element_edges = field.transformed_storage()->component_count();
              edgeMap.reverse_map_data(data, field, num_to_get * element_edges);
            }
          }
          else if (field.get_name() == "connectivity_face") {
            if (my_element_count > 0) {
              // Map element connectivity from global face id to local face id.
              int element_faces = field.transformed_storage()->component_count();
              faceMap.reverse_map_data(data, field, num_to_get * element_faces);
            }
          }
          else if (field.get_name() == "connectivity_raw") {
            if (my_element_count > 0) {
              // Element connectivity is already in local node id.
            }
          }
          else if (field.get_name() == "ids") {
            handle_element_ids(eb, data, num_to_get);
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
              int *comp32 = reinterpret_cast<int *>(Data(component));

              int index = comp;
              for (size_t i = 0; i < my_element_count; i++) {
                comp32[i] = data32[index];
                index += comp_count;
              }
            }
            else {
              auto *data64 = reinterpret_cast<int64_t *>(data);
              auto *comp64 = reinterpret_cast<int64_t *>(Data(component));

              int index = comp;
              for (size_t i = 0; i < my_element_count; i++) {
                comp64[i] = data64[index];
                index += comp_count;
              }
            }
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
          write_entity_transient_field(field, eb, my_element_count, data);
        }
        else if (role == Ioss::Field::REDUCTION) {
          store_reduction_field(field, eb, data);
        }
      }
      return num_to_get;
    }
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::FaceBlock *eb, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    {
      size_t num_to_get = field.verify(data_size);

      if (num_to_get > 0) {
        // Get the face block id and face count
        int64_t               my_face_count = eb->entity_count();
        Ioss::Field::RoleType role          = field.get_role();

        if (role == Ioss::Field::MESH) {
          // Handle the MESH fields required for an Exodus file model.
          // (The 'genesis' portion)
          if (field.get_name() == "connectivity") {
            if (my_face_count > 0) {
              // Map face connectivity from global node id to local node id.
              // Do it in 'data' ...
              int face_nodes = eb->topology()->number_nodes();
              nodeMap.reverse_map_data(data, field, num_to_get * face_nodes);
            }
          }
          else if (field.get_name() == "connectivity_edge") {
            if (my_face_count > 0) {
              // Map face connectivity from global edge id to local edge id.
              int face_edges = field.transformed_storage()->component_count();
              edgeMap.reverse_map_data(data, field, num_to_get * face_edges);
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
      }
      return num_to_get;
    }
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::EdgeBlock *eb, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    {
      size_t num_to_get = field.verify(data_size);

      if (num_to_get > 0) {
        // Get the edge block id and edge count
        int64_t               my_edge_count = eb->entity_count();
        Ioss::Field::RoleType role          = field.get_role();

        if (role == Ioss::Field::MESH) {
          // Handle the MESH fields required for an Exodus file model. (The 'genesis' portion)
          if (field.get_name() == "connectivity") {
            if (my_edge_count > 0) {
              // Map edge connectivity from global node id to local node id.
              // Do it in 'data' ...
              int edge_nodes = eb->topology()->number_nodes();
              nodeMap.reverse_map_data(data, field, num_to_get * edge_nodes);
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
     * use the following heuristics:
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
    SMART_ASSERT(num_to_get == nodeCount)(num_to_get)(nodeCount);

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

  void DatabaseIO::write_nodal_transient_field(const Ioss::Field &field,
                                               const Ioss::NodeBlock * /* ge */, int64_t count,
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
    int re_im      = 1;
    if (ioss_type == Ioss::Field::COMPLEX) {
      re_im = 2;
    }
    for (int complex_comp = 0; complex_comp < re_im; complex_comp++) {
      for (int i = 0; i < comp_count; i++) {
        std::string var_name = get_component_name(field, Ioss::Field::InOut::OUTPUT, i + 1);

        auto var_iter = m_variables[EX_NODE_BLOCK].find(var_name);
        if (var_iter == m_variables[EX_NODE_BLOCK].end()) {
          std::ostringstream errmsg;
          fmt::print(errmsg, "ERROR: Could not find nodal variable '{}'\n", var_name);
          IOSS_ERROR(errmsg);
        }
        int var_index = var_iter->second;

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
      }
    }
  }

  void DatabaseIO::write_entity_transient_field(const Ioss::Field          &field,
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
      const auto *elb = dynamic_cast<const Ioss::ElementBlock *>(ge);
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
    int comp_count = field.get_component_count(Ioss::Field::InOut::OUTPUT);

    // Handle quick easy, hopefully common case first...
    ex_entity_type type = Ioexnl::map_exodus_type(ge->type());
    if (comp_count == 1 && ioss_type == Ioss::Field::REAL && type != EX_SIDE_SET &&
        !map->reorders()) {
      // Simply output the variable...
      std::string var_name = get_component_name(field, Ioss::Field::InOut::OUTPUT, 1);
      auto        var_iter = m_variables[type].find(var_name);
      if (var_iter == m_variables[type].end()) {
        std::ostringstream errmsg;
        fmt::print(errmsg, "ERROR: Could not find field '{}'\n", var_name);
        IOSS_ERROR(errmsg);
      }
      return;
    }
    int re_im = 1;
    if (ioss_type == Ioss::Field::COMPLEX) {
      re_im = 2;
    }
    for (int complex_comp = 0; complex_comp < re_im; complex_comp++) {
      for (int i = 0; i < comp_count; i++) {
        std::string var_name = get_component_name(field, Ioss::Field::InOut::OUTPUT, i + 1);

        auto var_iter = m_variables[type].find(var_name);
        if (var_iter == m_variables[type].end()) {
          std::ostringstream errmsg;
          fmt::print(errmsg, "ERROR: Could not find field '{}'\n", var_name);
          IOSS_ERROR(errmsg);
        }
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
          map->map_field_to_db_scalar_order(static_cast<int *>(variables), temp, begin_offset,
                                            count, stride, eb_offset);
        }
        else if (ioss_type == Ioss::Field::INT64) {
          map->map_field_to_db_scalar_order(static_cast<int64_t *>(variables), temp, begin_offset,
                                            count, stride, eb_offset);
        }
      }
    }
  }

  int64_t DatabaseIO::put_Xset_field_internal(const Ioss::EntitySet *ns, const Ioss::Field &field,
                                              void *data, size_t data_size) const
  {
    {
      size_t entity_count = ns->entity_count();
      size_t num_to_get   = field.verify(data_size);
      if (num_to_get > 0) {

        Ioss::Field::RoleType role = field.get_role();

        if (role == Ioss::Field::MESH) {

          if (field.get_name() == "ids" || field.get_name() == "ids_raw") {
            // Map node id from global node id to local node id.
            // Do it in 'data' ...

            if (field.get_name() == "ids") {
              nodeMap.reverse_map_data(data, field, num_to_get);
            }
          }
          else if (field.get_name() == "orientation") {
          }
          else if (field.get_name() == "distribution_factors") {
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
      }
      return num_to_get;
    }
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::NodeSet *ns, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    return put_Xset_field_internal(ns, field, data, data_size);
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::EdgeSet *ns, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    return put_Xset_field_internal(ns, field, data, data_size);
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::FaceSet *ns, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    return put_Xset_field_internal(ns, field, data, data_size);
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::ElementSet *ns, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    return put_Xset_field_internal(ns, field, data, data_size);
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
        // Convert global node id to local node id and store in 'entities'
        if (int_byte_size_api() == 4) {
          int *entity_proc = static_cast<int *>(data);
          int *ent         = reinterpret_cast<int *>(Data(entities));
          int *pro         = reinterpret_cast<int *>(Data(procs));
          int  j           = 0;
          for (size_t i = 0; i < entity_count; i++) {
            int global_id = entity_proc[j++];
            ent[i]        = nodeMap.global_to_local(global_id, true);
            pro[i]        = entity_proc[j++];
          }
        }
        else {
          auto   *entity_proc = static_cast<int64_t *>(data);
          auto   *ent         = reinterpret_cast<int64_t *>(Data(entities));
          auto   *pro         = reinterpret_cast<int64_t *>(Data(procs));
          int64_t j           = 0;
          for (size_t i = 0; i < entity_count; i++) {
            int64_t global_id = entity_proc[j++];
            ent[i]            = nodeMap.global_to_local(global_id, true);
            pro[i]            = entity_proc[j++];
          }
        }

        if (commsetNodeCount > 0) {}

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
            compute_internal_border_maps(reinterpret_cast<int *>(Data(entities)),
                                         reinterpret_cast<int *>(Data(internal)), nodeCount,
                                         entity_count);
          }
          else {
            compute_internal_border_maps(reinterpret_cast<int64_t *>(Data(entities)),
                                         reinterpret_cast<int64_t *>(Data(internal)), nodeCount,
                                         entity_count);
          }
        }
      }
      else if (type == "side") {
        std::vector<char> sides(entity_count * int_byte_size_api());
        if (int_byte_size_api() == 4) {
          int *entity_proc = static_cast<int *>(data);
          int *ent         = reinterpret_cast<int *>(Data(entities));
          int *sid         = reinterpret_cast<int *>(Data(sides));
          int *pro         = reinterpret_cast<int *>(Data(procs));
          int  j           = 0;
          for (size_t i = 0; i < entity_count; i++) {
            ent[i] = elemMap.global_to_local(entity_proc[j++]);
            sid[i] = entity_proc[j++];
            pro[i] = entity_proc[j++];
          }
        }
        else {
          auto   *entity_proc = static_cast<int64_t *>(data);
          auto   *ent         = reinterpret_cast<int64_t *>(Data(entities));
          auto   *sid         = reinterpret_cast<int64_t *>(Data(sides));
          auto   *pro         = reinterpret_cast<int64_t *>(Data(procs));
          int64_t j           = 0;
          for (size_t i = 0; i < entity_count; i++) {
            ent[i] = elemMap.global_to_local(entity_proc[j++]);
            sid[i] = entity_proc[j++];
            pro[i] = entity_proc[j++];
          }
        }

        // Construct the element map (internal vs. border).
        // Border elements are those in the communication map (use entities array)
        // Internal elements are the rest.  Allocate array to hold all elements,
        // initialize all to '1', then zero out the elements in 'entities'.
        // Iterate through array again and consolidate all '1's
        std::vector<char> internal(elementCount * int_byte_size_api());
        if (int_byte_size_api() == 4) {
          compute_internal_border_maps(reinterpret_cast<int *>(Data(entities)),
                                       reinterpret_cast<int *>(Data(internal)), elementCount,
                                       entity_count);
        }
        else {
          compute_internal_border_maps(reinterpret_cast<int64_t *>(Data(entities)),
                                       reinterpret_cast<int64_t *>(Data(internal)), elementCount,
                                       entity_count);
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
                                         void * /* data */, size_t data_size) const
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
    size_t num_to_get = field.verify(data_size);
    if (num_to_get > 0) {

      size_t                entity_count = fb->entity_count();
      Ioss::Field::RoleType role         = field.get_role();

      if (role == Ioss::Field::MESH) {
        if (field.get_name() == "side_ids" && fb->name() == "universal_sideset") {}

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
            int            *el_side = reinterpret_cast<int *>(data);

            try {
              for (size_t i = 0; i < num_to_get; i++) {
                element[i] = elemMap.global_to_local(el_side[index++]);
                side[i]    = el_side[index++] + side_offset;
              }
            }
            catch (const std::runtime_error &x) {
              std::ostringstream errmsg;
              fmt::print(errmsg, "{}On SideBlock `{}` while outputting field `elem_side`\n",
                         x.what(), fb->name());
              IOSS_ERROR(errmsg);
            }
          }
          else {
            Ioss::Int64Vector element(num_to_get);
            Ioss::Int64Vector side(num_to_get);
            auto             *el_side = reinterpret_cast<int64_t *>(data);

            try {
              for (size_t i = 0; i < num_to_get; i++) {
                element[i] = elemMap.global_to_local(el_side[index++]);
                side[i]    = el_side[index++] + side_offset;
              }
            }
            catch (const std::runtime_error &x) {
              std::ostringstream errmsg;
              fmt::print(errmsg, "{}On SideBlock `{}` while outputting field `elem_side`\n",
                         x.what(), fb->name());
              IOSS_ERROR(errmsg);
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
            int            *el_side = reinterpret_cast<int *>(data);

            for (size_t i = 0; i < num_to_get; i++) {
              element[i] = el_side[index++];
              side[i]    = el_side[index++] + side_offset;
            }
          }
          else {
            Ioss::Int64Vector element(num_to_get);
            Ioss::Int64Vector side(num_to_get);
            auto             *el_side = reinterpret_cast<int64_t *>(data);

            for (size_t i = 0; i < num_to_get; i++) {
              element[i] = el_side[index++];
              side[i]    = el_side[index++] + side_offset;
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
        write_entity_transient_field(field, fb, entity_count, data);
      }
      else if (role == Ioss::Field::ATTRIBUTE) {
        num_to_get = write_attribute_field(field, fb, data);
      }
      else if (role == Ioss::Field::REDUCTION) {
        store_reduction_field(field, fb, data);
      }
    }
    return num_to_get;
  }

  void DatabaseIO::write_meta_data(Ioss::IfDatabaseExistsBehavior behavior)
  {
    Ioss::Region *region = get_region();
    common_write_metadata(behavior);

    char the_title[max_line_length + 1];

    // Title...
    if (region->property_exists("title")) {
      std::string title_str = region->get_property("title").get_string();
      Ioss::Utils::copy_string(the_title, title_str);
    }
    else {
      Ioss::Utils::copy_string(the_title, "IOSS Default Output Title");
    }

    bool         file_per_processor = true;
    Ioexnl::Mesh mesh(spatialDimension, the_title, util(), file_per_processor);
    {
      bool omit_maps = false;
      Ioss::Utils::check_set_bool_property(properties, "OMIT_EXODUS_NUM_MAPS", omit_maps);
      if (omit_maps) {
        // Used for special cases only -- typically very large meshes with *known* 1..count maps
        // and workarounds that avoid calling the "ids" put_field calls.
        mesh.use_node_map = false;
        mesh.use_elem_map = false;
        mesh.use_face_map = false;
        mesh.use_edge_map = false;
      }

      bool minimal_nemesis = false;
      Ioss::Utils::check_set_bool_property(properties, "MINIMAL_NEMESIS_DATA", minimal_nemesis);
      if (minimal_nemesis) {
        // Only output the node communication map data... This is all that stk/sierra needs
        mesh.full_nemesis_data = false;
      }

      mesh.populate(region);
      gather_communication_metadata(&mesh.comm);

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

        output_other_metadata();
      }
    }
  }

  void DatabaseIO::gather_communication_metadata(Ioexnl::CommunicationMetaData *meta)
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
      meta->globalNodes         = get_region()->get_optional_property("global_node_count", 1);
      meta->globalElements      = get_region()->get_optional_property("global_element_count", 1);
      meta->globalElementBlocks = get_region()->get_optional_property(
          "global_element_block_count", get_region()->get_element_blocks().size());
      meta->globalNodeSets = get_region()->get_optional_property(
          "global_node_set_count", get_region()->get_nodesets().size());
      meta->globalSideSets = get_region()->get_optional_property(
          "global_side_set_count", get_region()->get_sidesets().size());

      // ========================================================================
      // Load balance parameters (NEMESIS, p15)
      meta->nodesInternal = get_region()->get_optional_property("internal_node_count", nodeCount);
      meta->nodesBorder   = get_region()->get_optional_property("border_node_count", 0);
      meta->nodesExternal = 0; // Shadow nodes == 0 for now
      meta->elementsInternal =
          get_region()->get_optional_property("internal_element_count", elementCount);
      meta->elementsBorder = get_region()->get_optional_property("border_element_count", 0);

      const Ioss::CommSetContainer &comm_sets = get_region()->get_commsets();
      for (auto &cs : comm_sets) {
        std::string type  = cs->get_property("entity_type").get_string();
        size_t      count = cs->entity_count();
        int64_t     id    = Ioexnl::get_id(cs, &ids_);

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
} // namespace Ioexnl
