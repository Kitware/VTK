// Copyright(C) 1999-2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "exodus/Ioex_Internals.h" // for Internals, ElemBlock, etc
#include "exodus/Ioex_Utils.h"
#include "vtk_fmt.h"
#include VTK_FMT(fmt/core.h)

#include "Ioss_ElementTopology.h"
#include "Ioss_GroupingEntity.h"
#include "vtk_exodusII.h" // for ex_err, ex_opts, etc

extern "C" {
#include <exodusII_int.h>
}

#include <array>
#include <cstdlib>  // for exit, EXIT_FAILURE
#include <cstring>  // for strlen
#include <vtk_netcdf.h> // for NC_NOERR, nc_def_var, etc
#include <string>   // for string, operator==, etc
#include <vector>   // for vector

#include "Ioss_Assembly.h"
#include "Ioss_Blob.h"
#include "Ioss_EdgeBlock.h"
#include "Ioss_EdgeSet.h"
#include "Ioss_ElementBlock.h"
#include "Ioss_ElementSet.h"
#include "Ioss_FaceBlock.h"
#include "Ioss_FaceSet.h"
#include "Ioss_Field.h"
#include "Ioss_NodeBlock.h"
#include "Ioss_NodeSet.h"
#include "Ioss_Property.h"
#include "Ioss_Region.h"
#include "Ioss_SideBlock.h"
#include "Ioss_SideSet.h"
#include "Ioss_SmartAssert.h"
#include "Ioss_Utils.h"
#include "Ioss_VariableType.h"

using namespace Ioex;

namespace {
  nc_type get_type(int exoid, unsigned int type)
  {
    if ((ex_int64_status(exoid) & type) != 0U) {
      return NC_INT64;
    }

    return NC_INT;
  }
  int define_netcdf_vars(int exoid, const char *type, size_t count, const char *dim_num,
                         const char *stat_var, const char *id_var, const char *name_var);
  int define_variable(int exodusFilePtr, int64_t size, const char *dim, const char *var,
                      nc_type type);
  int define_variables(int exodusFilePtr, int64_t size, const char *dim, const char *var[],
                       const nc_type *types);
  int conditional_define_variable(int exodusFilePtr, const char *var, int dimid, int *varid,
                                  nc_type type);

  int put_int_array(int exoid, const char *var_type, const std::vector<int> &array);
  int put_int_array(int exoid, const char *var_type, const std::vector<int64_t> &array);

  int put_id_array(int exoid, const char *var_type, const std::vector<entity_id> &ids);
  int define_coordinate_vars(int exodusFilePtr, int64_t nodes, int node_dim, int dimension,
                             int dim_dim, int str_dim);
  template <typename T>
  int output_names(const std::vector<T> &entities, int exoid, ex_entity_type ent_type);
  template <typename T> int get_max_name_length(const std::vector<T> &entities, int old_max);
} // namespace

Redefine::Redefine(int exoid) : exodusFilePtr(exoid)
{
  // Enter define mode...
  int status = nc_redef(exodusFilePtr);
  if (status != NC_NOERR) {
    ex_opts(EX_VERBOSE);
    std::string errmsg =
        fmt::format("Error: failed to put file id {} into define mode", exodusFilePtr);
    ex_err_fn(exoid, __func__, errmsg.c_str(), status);
    exit(EXIT_FAILURE);
  }
}

Redefine::~Redefine()
{
  try {
    int status = nc_enddef(exodusFilePtr);
    if (status != NC_NOERR) {
      ex_opts(EX_VERBOSE);
      std::string errmsg = fmt::format(
          "Error: failed to complete variable definitions in file id {}", exodusFilePtr);
      ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      exit(EXIT_FAILURE);
    }
  }
  catch (...) {
  }
}

Assembly::Assembly(const Ioss::Assembly &other)
{
  if (other.property_exists("db_name")) {
    name = other.get_property("db_name").get_string();
  }
  else {
    name = other.name();
  }

  id             = other.get_optional_property("id", 1);
  entityCount    = other.member_count();
  attributeCount = other.get_property("attribute_count").get_int();
  type           = Ioex::map_exodus_type(other.get_member_type());

  const auto &members = other.get_members();
  for (const auto &member : members) {
    SMART_ASSERT(member->property_exists("id"));
    memberIdList.push_back(member->get_property("id").get_int());
  }
}

Blob::Blob(const Ioss::Blob &other)
{
  if (other.property_exists("db_name")) {
    name = other.get_property("db_name").get_string();
  }
  else {
    name = other.name();
  }

  id             = other.get_optional_property("id", 1);
  entityCount    = other.entity_count();
  attributeCount = other.get_property("attribute_count").get_int();
}

NodeBlock::NodeBlock(const Ioss::NodeBlock &other)
{
  if (other.property_exists("db_name")) {
    name = other.get_property("db_name").get_string();
  }
  else {
    name = other.name();
  }

  id              = other.get_optional_property("id", 1);
  entityCount     = other.entity_count();
  localOwnedCount = other.get_optional_property("locally_owned_count", entityCount);
  attributeCount  = other.get_property("attribute_count").get_int();
  procOffset      = 0;
}

EdgeBlock::EdgeBlock(const Ioss::EdgeBlock &other)
{
  if (other.property_exists("db_name")) {
    name = other.get_property("db_name").get_string();
  }
  else {
    name = other.name();
  }

  id             = other.get_property("id").get_int();
  entityCount    = other.entity_count();
  nodesPerEntity = other.topology()->number_nodes();
  attributeCount = other.get_property("attribute_count").get_int();

  std::string el_type = other.topology()->name();
  if (other.property_exists("original_topology_type")) {
    el_type = other.get_property("original_topology_type").get_string();
  }

  Ioss::Utils::copy_string(elType, el_type);
  procOffset = 0;
}

FaceBlock::FaceBlock(const Ioss::FaceBlock &other)
{
  if (other.property_exists("db_name")) {
    name = other.get_property("db_name").get_string();
  }
  else {
    name = other.name();
  }

  id             = other.get_property("id").get_int();
  entityCount    = other.entity_count();
  nodesPerEntity = other.topology()->number_nodes();
  if (other.field_exists("connectivty_edge")) {
    edgesPerEntity = other.get_field("connectivity_edge").raw_storage()->component_count();
  }
  else {
    edgesPerEntity = 0;
  }
  attributeCount = other.get_property("attribute_count").get_int();

  std::string el_type = other.topology()->name();
  if (other.property_exists("original_topology_type")) {
    el_type = other.get_property("original_topology_type").get_string();
  }

  Ioss::Utils::copy_string(elType, el_type);
  procOffset = 0;
}

ElemBlock::ElemBlock(const Ioss::ElementBlock &other)
{
  if (other.property_exists("db_name")) {
    name = other.get_property("db_name").get_string();
  }
  else {
    name = other.name();
  }

  id                = other.get_property("id").get_int();
  entityCount       = other.entity_count();
  globalEntityCount = other.get_optional_property("global_entity_count", 0);
  nodesPerEntity    = other.topology()->number_nodes();

  if (other.field_exists("connectivity_edge")) {
    edgesPerEntity = other.get_field("connectivity_edge").raw_storage()->component_count();
  }
  else {
    edgesPerEntity = 0;
  }

  if (other.field_exists("connectivity_face")) {
    facesPerEntity = other.get_field("connectivity_face").raw_storage()->component_count();
  }
  else {
    facesPerEntity = 0;
  }

  attributeCount = other.get_property("attribute_count").get_int();
  offset_        = other.get_offset();
  std::string el_type =
      other.get_optional_property("original_topology_type", other.topology()->name());

  Ioss::Utils::copy_string(elType, el_type);

  // Fixup an exodusII kluge.  For triangular elements, the same
  // name is used for 2D elements and 3D shell elements.  Convert
  // to unambiguous names for the IO Subsystem.  The 2D name
  // stays the same, the 3D name becomes 'trishell#'
  // Here, we need to map back to the 'triangle' name...
  if (std::strncmp(elType, "trishell", 8) == 0) {
    Ioss::Utils::copy_string(elType, "triangle");
  }
  procOffset = 0;
}

NodeSet::NodeSet(const Ioss::NodeSet &other)
{
  if (other.property_exists("db_name")) {
    name = other.get_property("db_name").get_string();
  }
  else {
    name = other.name();
  }

  id                = other.get_property("id").get_int();
  entityCount       = other.entity_count();
  globalEntityCount = other.get_optional_property("global_entity_count", 0);
  localOwnedCount   = other.get_optional_property("locally_owned_count", entityCount);
  attributeCount    = other.get_property("attribute_count").get_int();
  dfCount           = other.get_property("distribution_factor_count").get_int();
  if (dfCount > 0 && dfCount != entityCount) {
    dfCount = entityCount;
  }
  procOffset = 0;
}

EdgeSet::EdgeSet(const Ioss::EdgeSet &other)
{
  if (other.property_exists("db_name")) {
    name = other.get_property("db_name").get_string();
  }
  else {
    name = other.name();
  }

  id             = other.get_property("id").get_int();
  entityCount    = other.entity_count();
  attributeCount = other.get_property("attribute_count").get_int();
  dfCount        = other.get_property("distribution_factor_count").get_int();
  procOffset     = 0;
}

FaceSet::FaceSet(const Ioss::FaceSet &other)
{
  if (other.property_exists("db_name")) {
    name = other.get_property("db_name").get_string();
  }
  else {
    name = other.name();
  }

  id             = other.get_property("id").get_int();
  entityCount    = other.entity_count();
  attributeCount = other.get_property("attribute_count").get_int();
  dfCount        = other.get_property("distribution_factor_count").get_int();
  procOffset     = 0;
}

ElemSet::ElemSet(const Ioss::ElementSet &other)
{
  if (other.property_exists("db_name")) {
    name = other.get_property("db_name").get_string();
  }
  else {
    name = other.name();
  }

  id             = other.get_property("id").get_int();
  entityCount    = other.entity_count();
  attributeCount = other.get_property("attribute_count").get_int();
  dfCount        = other.get_property("distribution_factor_count").get_int();
  procOffset     = 0;
}

SideSet::SideSet(const Ioss::SideBlock &other)
{
  if (other.property_exists("db_name")) {
    name = other.get_property("db_name").get_string();
  }
  else {
    name = other.name();
  }

  id                         = other.get_property("id").get_int();
  entityCount                = other.entity_count();
  globalEntityCount          = other.get_optional_property("global_entity_count", 0);
  dfCount                    = other.get_property("distribution_factor_count").get_int();
  const std::string &io_name = other.name();

  // KLUGE: universal_sideset has side dfCount...
  if (io_name == "universal_sideset") {
    dfCount = entityCount;
  }
  procOffset   = 0;
  dfProcOffset = 0;
}

SideSet::SideSet(const Ioss::SideSet &other)
{
  if (other.property_exists("db_name")) {
    name = other.get_property("db_name").get_string();
  }
  else {
    name = other.name();
  }

  id                         = other.get_property("id").get_int();
  entityCount                = other.entity_count();
  globalEntityCount          = other.get_optional_property("global_entity_count", 0);
  dfCount                    = other.get_property("distribution_factor_count").get_int();
  const std::string &io_name = other.name();

  // KLUGE: universal_sideset has side dfCount...
  if (io_name == "universal_sideset") {
    dfCount = entityCount;
  }
  procOffset   = 0;
  dfProcOffset = 0;
}

Internals::Internals(int exoid, int maximum_name_length, const Ioss::ParallelUtils &util)
    : exodusFilePtr(exoid), maximumNameLength(maximum_name_length), parallelUtil(util)
{
}

int Internals::initialize_state_file(Mesh &mesh, const ex_var_params &var_params,
                                     const std::string &base_filename)
{
  // Determine global counts...
  if (!mesh.file_per_processor) {
    mesh.get_global_counts();
  }

  int ierr = 0;
  {
    Redefine the_database(exodusFilePtr);
    int      old_fill = 0;

    int status = nc_set_fill(exodusFilePtr, NC_NOFILL, &old_fill);
    if (status != EX_NOERR) {
      return EX_FATAL;
    }

    status = nc_put_att_text(exodusFilePtr, NC_GLOBAL, "base_database",
                             static_cast<int>(base_filename.length()) + 1, base_filename.c_str());

    if (status != NC_NOERR) {
      ex_opts(EX_VERBOSE);
      std::string errmsg = fmt::format(
          "Error: failed to define 'base_database' attribute to file id {}", exodusFilePtr);
      ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      return EX_FATAL;
    }

    // Time Dimension...
    int timedim;
    if ((status = nc_def_dim(exodusFilePtr, DIM_TIME, NC_UNLIMITED, &timedim)) != NC_NOERR) {
      std::string errmsg =
          fmt::format("Error: failed to define time dimension in file id {}", exodusFilePtr);
      ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      return EX_FATAL;
    }

    // Name String Length...
    int namestrdim;
    status = nc_def_dim(exodusFilePtr, DIM_STR_NAME, maximumNameLength + 1, &namestrdim);
    if (status != NC_NOERR) {
      ex_opts(EX_VERBOSE);
      std::string errmsg =
          fmt::format("Error: failed to define 'name string length' in file id {}", exodusFilePtr);
      ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      return EX_FATAL;
    }

    // Nodes (Node Block) ...
    if (var_params.num_node > 0) {
      int numnoddim;
      status = nc_def_dim(exodusFilePtr, DIM_NUM_NODES, mesh.nodeblocks[0].entityCount, &numnoddim);
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        std::string errmsg =
            fmt::format("Error: failed to define number of nodes in file id {}", exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        return EX_FATAL;
      }
    }

    // ========================================================================
    // Blocks...
    size_t elem_count = 0;
    for (const auto &elem : mesh.elemblocks) {
      elem_count += elem.entityCount;
    }

    if (elem_count > 0 && var_params.num_elem > 0) {
      int numelemdim;
      status = nc_def_dim(exodusFilePtr, DIM_NUM_ELEM, elem_count, &numelemdim);
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        std::string errmsg =
            fmt::format("Error: failed to define number of elements in file id {}", exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        return EX_FATAL;
      }
      if (define_netcdf_vars(exodusFilePtr, "element block", mesh.elemblocks.size(), DIM_NUM_EL_BLK,
                             VAR_STAT_EL_BLK, VAR_ID_EL_BLK, VAR_NAME_EL_BLK) != EX_NOERR) {
        return EX_FATAL;
      }
    }

    size_t face_count = 0;
    for (const auto &face : mesh.faceblocks) {
      face_count += face.entityCount;
    }

    if (face_count > 0 && var_params.num_face > 0) {
      int numfacedim;
      status = nc_def_dim(exodusFilePtr, DIM_NUM_FACE, face_count, &numfacedim);
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        std::string errmsg =
            fmt::format("Error: failed to define number of faces in file id {}", exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        return EX_FATAL;
      }
      if (define_netcdf_vars(exodusFilePtr, "face block", mesh.faceblocks.size(), DIM_NUM_FA_BLK,
                             VAR_STAT_FA_BLK, VAR_ID_FA_BLK, VAR_NAME_FA_BLK) != EX_NOERR) {
        return EX_FATAL;
      }
    }

    size_t edge_count = 0;
    for (const auto &edge : mesh.edgeblocks) {
      edge_count += edge.entityCount;
    }

    if (edge_count > 0 && var_params.num_edge > 0) {
      int numedgedim;
      status = nc_def_dim(exodusFilePtr, DIM_NUM_EDGE, edge_count, &numedgedim);
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        std::string errmsg =
            fmt::format("Error: failed to define number of edges in file id {}", exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        return EX_FATAL;
      }
      if (define_netcdf_vars(exodusFilePtr, "edge block", mesh.edgeblocks.size(), DIM_NUM_ED_BLK,
                             VAR_STAT_ED_BLK, VAR_ID_ED_BLK, VAR_NAME_ED_BLK) != EX_NOERR) {
        return EX_FATAL;
      }
    }

    // ========================================================================
    // Sets...
    if (var_params.num_nset > 0) {
      if (define_netcdf_vars(exodusFilePtr, "node set", mesh.nodesets.size(), DIM_NUM_NS,
                             VAR_NS_STAT, VAR_NS_IDS, VAR_NAME_NS) != EX_NOERR) {
        return EX_FATAL;
      }
    }

    if (var_params.num_eset > 0) {
      if (define_netcdf_vars(exodusFilePtr, "edge set", mesh.edgesets.size(), DIM_NUM_ES,
                             VAR_ES_STAT, VAR_ES_IDS, VAR_NAME_ES) != EX_NOERR) {
        return EX_FATAL;
      }
    }

    if (var_params.num_fset > 0) {
      if (define_netcdf_vars(exodusFilePtr, "face set", mesh.facesets.size(), DIM_NUM_FS,
                             VAR_FS_STAT, VAR_FS_IDS, VAR_NAME_FS) != EX_NOERR) {
        return EX_FATAL;
      }
    }

    if (var_params.num_elset > 0) {
      if (define_netcdf_vars(exodusFilePtr, "element set", mesh.elemsets.size(), DIM_NUM_ELS,
                             VAR_ELS_STAT, VAR_ELS_IDS, VAR_NAME_ELS) != EX_NOERR) {
        return EX_FATAL;
      }
    }

    // ========================================================================
    // side sets...
    if (var_params.num_sset > 0) {
      if (define_netcdf_vars(exodusFilePtr, "side set", mesh.sidesets.size(), DIM_NUM_SS,
                             VAR_SS_STAT, VAR_SS_IDS, VAR_NAME_SS) != EX_NOERR) {
        return EX_FATAL;
      }
    }

    if (var_params.num_edge > 0) {
      if ((ierr = put_metadata(mesh.edgeblocks, true)) != EX_NOERR) {
        EX_FUNC_LEAVE(ierr);
      }
    }

    if (var_params.num_face > 0) {
      if ((ierr = put_metadata(mesh.faceblocks, true)) != EX_NOERR) {
        EX_FUNC_LEAVE(ierr);
      }
    }

    if (var_params.num_elem > 0) {
      if ((ierr = put_metadata(mesh.elemblocks, true)) != EX_NOERR) {
        EX_FUNC_LEAVE(ierr);
      }
    }

    if (var_params.num_nset > 0) {
      if ((ierr = put_metadata(mesh.nodesets, true)) != EX_NOERR) {
        EX_FUNC_LEAVE(ierr);
      }
    }

    if (var_params.num_eset > 0) {
      if ((ierr = put_metadata(mesh.edgesets, true)) != EX_NOERR) {
        EX_FUNC_LEAVE(ierr);
      }
    }

    if (var_params.num_fset > 0) {
      if ((ierr = put_metadata(mesh.facesets, true)) != EX_NOERR) {
        EX_FUNC_LEAVE(ierr);
      }
    }

    if (var_params.num_elset > 0) {
      if ((ierr = put_metadata(mesh.elemsets, true)) != EX_NOERR) {
        EX_FUNC_LEAVE(ierr);
      }
    }

    if (var_params.num_sset > 0) {
      if ((ierr = put_metadata(mesh.sidesets, true)) != EX_NOERR) {
        EX_FUNC_LEAVE(ierr);
      }
    }

    int        varid;
    std::array dim{timedim};
    if ((status = nc_def_var(exodusFilePtr, VAR_WHOLE_TIME, nc_flt_code(exodusFilePtr), 1,
                             Data(dim), &varid)) != NC_NOERR) {
      std::string errmsg = fmt::format(
          "Error: failed to define whole time step variable in file id {}", exodusFilePtr);
      ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      return EX_FATAL;
    }

    struct exi_file_item *file = exi_find_file_item(exodusFilePtr);
    if (file) {
      file->time_varid = varid;
    }

    exi_compress_variable(exodusFilePtr, varid, -2); /* don't compress, but do set collective io */
  } // Exit redefine mode

  bool output_global_data = (mesh.comm.outputNemesis && mesh.comm.processorCount > 1);
  if (var_params.num_edge > 0) {
    if ((ierr = put_non_define_data(mesh.edgeblocks)) != EX_NOERR) {
      EX_FUNC_LEAVE(ierr);
    }
    output_names(mesh.edgeblocks, exodusFilePtr, EX_EDGE_BLOCK);
  }

  if (var_params.num_face > 0) {
    if ((ierr = put_non_define_data(mesh.faceblocks)) != EX_NOERR) {
      EX_FUNC_LEAVE(ierr);
    }
    output_names(mesh.faceblocks, exodusFilePtr, EX_FACE_BLOCK);
  }

  if (var_params.num_elem > 0) {
    if ((ierr = put_non_define_data(mesh.elemblocks, output_global_data)) != EX_NOERR) {
      EX_FUNC_LEAVE(ierr);
    }
    output_names(mesh.elemblocks, exodusFilePtr, EX_ELEM_BLOCK);
  }

  if (var_params.num_nset > 0) {
    if ((ierr = put_non_define_data(mesh.nodesets, output_global_data)) != EX_NOERR) {
      EX_FUNC_LEAVE(ierr);
    }
    output_names(mesh.nodesets, exodusFilePtr, EX_NODE_SET);
  }

  if (var_params.num_eset > 0) {
    if ((ierr = put_non_define_data(mesh.edgesets)) != EX_NOERR) {
      EX_FUNC_LEAVE(ierr);
    }
    output_names(mesh.edgesets, exodusFilePtr, EX_EDGE_SET);
  }

  if (var_params.num_fset > 0) {
    if ((ierr = put_non_define_data(mesh.facesets)) != EX_NOERR) {
      EX_FUNC_LEAVE(ierr);
    }
    output_names(mesh.facesets, exodusFilePtr, EX_FACE_SET);
  }

  if (var_params.num_elset > 0) {
    if ((ierr = put_non_define_data(mesh.elemsets)) != EX_NOERR) {
      EX_FUNC_LEAVE(ierr);
    }
    output_names(mesh.elemsets, exodusFilePtr, EX_ELEM_SET);
  }

  if (var_params.num_sset > 0) {
    if ((ierr = put_non_define_data(mesh.sidesets, output_global_data)) != EX_NOERR) {
      EX_FUNC_LEAVE(ierr);
    }
    output_names(mesh.sidesets, exodusFilePtr, EX_SIDE_SET);
  }

  return EX_NOERR;
}

void Mesh::populate(Ioss::Region *region)
{
  {
    const auto &node_blocks = region->get_node_blocks();
    if (!node_blocks.empty()) {
      Ioex::NodeBlock N(*node_blocks[0]);
      nodeblocks.push_back(N);
    }
  }

  // Assemblies --
  {
    const auto &assem = region->get_assemblies();
    for (auto &assembly : assem) {
      Ioex::Assembly T(*(assembly));
      assemblies.push_back(T);
    }
  }

  // Blobs --
  {
    const auto &blbs = region->get_blobs();
    for (auto &blob : blbs) {
      Ioex::Blob T(*(blob));
      blobs.push_back(T);
    }
  }

  // Edge Blocks --
  {
    const Ioss::EdgeBlockContainer &edge_blocks = region->get_edge_blocks();
    for (auto &edge_block : edge_blocks) {
      Ioex::EdgeBlock T(*(edge_block));
      edgeblocks.push_back(T);
    }
  }

  // Face Blocks --
  {
    const Ioss::FaceBlockContainer &face_blocks = region->get_face_blocks();
    for (auto &face_block : face_blocks) {
      Ioex::FaceBlock T(*(face_block));
      faceblocks.push_back(T);
    }
  }

  // Element Blocks --
  {
    const Ioss::ElementBlockContainer &element_blocks = region->get_element_blocks();
    for (auto &element_block : element_blocks) {
      Ioex::ElemBlock T(*(element_block));
      elemblocks.push_back(T);
    }
  }

  // NodeSets ...
  {
    const Ioss::NodeSetContainer &node_sets = region->get_nodesets();
    for (auto &set : node_sets) {
      const Ioex::NodeSet T(*(set));
      nodesets.push_back(T);
    }
  }

  // EdgeSets ...
  {
    const Ioss::EdgeSetContainer &edge_sets = region->get_edgesets();
    for (auto &set : edge_sets) {
      const Ioex::EdgeSet T(*(set));
      edgesets.push_back(T);
    }
  }

  // FaceSets ...
  {
    const Ioss::FaceSetContainer &face_sets = region->get_facesets();
    for (auto &set : face_sets) {
      const Ioex::FaceSet T(*(set));
      facesets.push_back(T);
    }
  }

  // ElementSets ...
  {
    const Ioss::ElementSetContainer &element_sets = region->get_elementsets();
    for (auto &set : element_sets) {
      const Ioex::ElemSet T(*(set));
      elemsets.push_back(T);
    }
  }

  // SideSets ...
  {
    const Ioss::SideSetContainer &ssets = region->get_sidesets();
    for (auto &set : ssets) {
      // Add a SideSet corresponding to this SideSet/SideBlock
      Ioex::SideSet T(*set);
      sidesets.push_back(T);
    }
  }

  // Determine global counts...
  if (!file_per_processor) {
    get_global_counts();
  }
}

void Mesh::get_global_counts()
{
#if defined(SEACAS_HAVE_MPI)
  std::vector<int64_t> counts;
  std::vector<int64_t> global_counts;

  for (auto &nodeblock : nodeblocks) {
    counts.push_back(nodeblock.localOwnedCount);
  }
  for (auto &edgeblock : edgeblocks) {
    counts.push_back(edgeblock.entityCount);
  }
  for (auto &faceblock : faceblocks) {
    counts.push_back(faceblock.entityCount);
  }
  for (auto &elemblock : elemblocks) {
    counts.push_back(elemblock.entityCount);
  }
  for (auto &nodeset : nodesets) {
    counts.push_back(nodeset.localOwnedCount);
    counts.push_back(nodeset.dfCount);
  }
  for (auto &edgeset : edgesets) {
    counts.push_back(edgeset.entityCount);
    counts.push_back(edgeset.dfCount);
  }
  for (auto &faceset : facesets) {
    counts.push_back(faceset.entityCount);
    counts.push_back(faceset.dfCount);
  }
  for (auto &elemset : elemsets) {
    counts.push_back(elemset.entityCount);
    counts.push_back(elemset.dfCount);
  }
  for (auto &sideset : sidesets) {
    counts.push_back(sideset.entityCount);
    counts.push_back(sideset.dfCount);
  }
  for (auto &blob : blobs) {
    counts.push_back(blob.entityCount);
  }

  // Now gather this information on each processor so
  // they can determine the offsets and totals...
  global_counts.resize(counts.size() * parallelUtil.parallel_size());

  MPI_Allgather(Data(counts), counts.size(), MPI_LONG_LONG_INT, Data(global_counts), counts.size(),
                MPI_LONG_LONG_INT, parallelUtil.communicator());

  std::vector<int64_t> offsets(counts.size());

  size_t my_proc    = parallelUtil.parallel_rank();
  size_t proc_count = parallelUtil.parallel_size();

  // Calculate offsets for each entity on each processor
  for (size_t j = 0; j < offsets.size(); j++) {
    for (size_t i = 0; i < my_proc; i++) {
      offsets[j] += global_counts[i * offsets.size() + j];
    }
  }

  // Now calculate the total count of entities over all processors
  for (size_t j = 0; j < offsets.size(); j++) {
    for (size_t i = 1; i < proc_count; i++) {
      global_counts[j] += global_counts[i * offsets.size() + j];
    }
  }

  size_t j = 0;
  for (auto &nodeblock : nodeblocks) {
    nodeblock.procOffset  = offsets[j];
    nodeblock.entityCount = global_counts[j++];
  }
  for (auto &edgeblock : edgeblocks) {
    edgeblock.procOffset  = offsets[j];
    edgeblock.entityCount = global_counts[j++];
  }
  for (auto &faceblock : faceblocks) {
    faceblock.procOffset  = offsets[j];
    faceblock.entityCount = global_counts[j++];
  }
  for (auto &elemblock : elemblocks) {
    elemblock.procOffset  = offsets[j];
    elemblock.entityCount = global_counts[j++];
  }
  for (auto &nodeset : nodesets) {
    nodeset.procOffset  = offsets[j];
    nodeset.entityCount = global_counts[j++];
    nodeset.dfCount     = global_counts[j++];
    if (nodeset.dfCount != 0) {
      // Need to adjust for locally-owned only in the auto-join output.
      nodeset.dfCount = nodeset.entityCount;
    }
  }
  for (auto &edgeset : edgesets) {
    edgeset.procOffset  = offsets[j];
    edgeset.entityCount = global_counts[j++];
    edgeset.dfCount     = global_counts[j++];
  }
  for (auto &faceset : facesets) {
    faceset.procOffset  = offsets[j];
    faceset.entityCount = global_counts[j++];
    faceset.dfCount     = global_counts[j++];
  }
  for (auto &elemset : elemsets) {
    elemset.procOffset  = offsets[j];
    elemset.entityCount = global_counts[j++];
    elemset.dfCount     = global_counts[j++];
  }
  for (auto &sideset : sidesets) {
    sideset.procOffset   = offsets[j];
    sideset.entityCount  = global_counts[j++];
    sideset.dfProcOffset = offsets[j];
    sideset.dfCount      = global_counts[j++];
  }
  for (auto &blob : blobs) {
    blob.procOffset  = offsets[j];
    blob.entityCount = global_counts[j++];
  }
#endif
}

int Internals::write_meta_data(Mesh &mesh)
{
  EX_FUNC_ENTER();
  int ierr;
  {
    // Determine length of longest name... Reduces calls to put_att
    maximumNameLength = get_max_name_length(mesh.edgeblocks, maximumNameLength);
    maximumNameLength = get_max_name_length(mesh.faceblocks, maximumNameLength);
    maximumNameLength = get_max_name_length(mesh.elemblocks, maximumNameLength);
    maximumNameLength = get_max_name_length(mesh.nodesets, maximumNameLength);
    maximumNameLength = get_max_name_length(mesh.edgesets, maximumNameLength);
    maximumNameLength = get_max_name_length(mesh.facesets, maximumNameLength);
    maximumNameLength = get_max_name_length(mesh.elemsets, maximumNameLength);
    maximumNameLength = get_max_name_length(mesh.sidesets, maximumNameLength);
    maximumNameLength = get_max_name_length(mesh.blobs, maximumNameLength);
    maximumNameLength = get_max_name_length(mesh.assemblies, maximumNameLength);

    Redefine the_database(exodusFilePtr);
    // Set the database to NOFILL mode.  Only writes values we want written...
    int old_fill = 0;

    if ((ierr = nc_set_fill(exodusFilePtr, NC_NOFILL, &old_fill)) != EX_NOERR) {
      EX_FUNC_LEAVE(ierr);
    }

    if ((ierr = put_metadata(mesh, mesh.comm)) != EX_NOERR) {
      EX_FUNC_LEAVE(ierr);
    }

    if ((ierr = put_metadata(mesh.edgeblocks)) != EX_NOERR) {
      EX_FUNC_LEAVE(ierr);
    }

    if ((ierr = put_metadata(mesh.faceblocks)) != EX_NOERR) {
      EX_FUNC_LEAVE(ierr);
    }

    if ((ierr = put_metadata(mesh.elemblocks)) != EX_NOERR) {
      EX_FUNC_LEAVE(ierr);
    }

    if ((ierr = put_metadata(mesh.nodesets)) != EX_NOERR) {
      EX_FUNC_LEAVE(ierr);
    }

    if ((ierr = put_metadata(mesh.edgesets)) != EX_NOERR) {
      EX_FUNC_LEAVE(ierr);
    }

    if ((ierr = put_metadata(mesh.facesets)) != EX_NOERR) {
      EX_FUNC_LEAVE(ierr);
    }

    if ((ierr = put_metadata(mesh.elemsets)) != EX_NOERR) {
      EX_FUNC_LEAVE(ierr);
    }

    if ((ierr = put_metadata(mesh.sidesets)) != EX_NOERR) {
      EX_FUNC_LEAVE(ierr);
    }

    if ((ierr = put_metadata(mesh.blobs)) != EX_NOERR) {
      EX_FUNC_LEAVE(ierr);
    }

    if ((ierr = put_metadata(mesh.assemblies)) != EX_NOERR) {
      EX_FUNC_LEAVE(ierr);
    }
  }

  // NON-Define mode output...
  bool output_global_data = (mesh.comm.outputNemesis && mesh.comm.processorCount > 1);

  if ((ierr = put_non_define_data(mesh.comm, mesh.full_nemesis_data)) != EX_NOERR) {
    EX_FUNC_LEAVE(ierr);
  }

  if ((ierr = put_non_define_data(mesh.edgeblocks)) != EX_NOERR) {
    EX_FUNC_LEAVE(ierr);
  }

  if ((ierr = put_non_define_data(mesh.faceblocks)) != EX_NOERR) {
    EX_FUNC_LEAVE(ierr);
  }

  if ((ierr = put_non_define_data(mesh.elemblocks, output_global_data)) != EX_NOERR) {
    EX_FUNC_LEAVE(ierr);
  }

  if ((ierr = put_non_define_data(mesh.nodesets, output_global_data)) != EX_NOERR) {
    EX_FUNC_LEAVE(ierr);
  }

  if ((ierr = put_non_define_data(mesh.edgesets)) != EX_NOERR) {
    EX_FUNC_LEAVE(ierr);
  }

  if ((ierr = put_non_define_data(mesh.facesets)) != EX_NOERR) {
    EX_FUNC_LEAVE(ierr);
  }

  if ((ierr = put_non_define_data(mesh.elemsets)) != EX_NOERR) {
    EX_FUNC_LEAVE(ierr);
  }

  if ((ierr = put_non_define_data(mesh.sidesets, output_global_data)) != EX_NOERR) {
    EX_FUNC_LEAVE(ierr);
  }

  if ((ierr = put_non_define_data(mesh.blobs)) != EX_NOERR) {
    EX_FUNC_LEAVE(ierr);
  }

  if ((ierr = put_non_define_data(mesh.assemblies)) != EX_NOERR) {
    EX_FUNC_LEAVE(ierr);
  }

  // For now, put entity names using the ExodusII api...
  output_names(mesh.edgeblocks, exodusFilePtr, EX_EDGE_BLOCK);
  output_names(mesh.faceblocks, exodusFilePtr, EX_FACE_BLOCK);
  output_names(mesh.elemblocks, exodusFilePtr, EX_ELEM_BLOCK);
  output_names(mesh.nodesets, exodusFilePtr, EX_NODE_SET);
  output_names(mesh.edgesets, exodusFilePtr, EX_EDGE_SET);
  output_names(mesh.facesets, exodusFilePtr, EX_FACE_SET);
  output_names(mesh.elemsets, exodusFilePtr, EX_ELEM_SET);
  output_names(mesh.sidesets, exodusFilePtr, EX_SIDE_SET);

  EX_FUNC_LEAVE(EX_NOERR);
}

void Internals::copy_database(int in_file, int out_file, bool transient_also)
{
  ex_copy(in_file, out_file);
  if (transient_also) {
    ex_copy_transient(in_file, out_file);
  }
}

void Internals::update_assembly_data(int exoid, std::vector<Assembly> &assemblies, int stage)
{
  Ioss::ParallelUtils pm;
  Internals           internal{exoid, 0, pm};

  if (stage == 0 || stage == 1) {
    Redefine the_database(exoid);
    internal.put_metadata(assemblies);
  }

  if (stage == 0 || stage == 2) {
    internal.put_non_define_data(assemblies);
  }
}

int Internals::put_metadata(const Mesh &mesh, const CommunicationMetaData &comm)
{
  int numdimdim  = 0;
  int numnoddim  = 0;
  int namestrdim = 0;
  int varid      = 0;
  int timedim    = 0;

  int map_type  = get_type(exodusFilePtr, EX_MAPS_INT64_DB);
  int bulk_type = get_type(exodusFilePtr, EX_BULK_INT64_DB);
  int ids_type  = get_type(exodusFilePtr, EX_IDS_INT64_DB);

  int rootid = static_cast<unsigned>(exodusFilePtr) & EX_FILE_ID_MASK;

  if (rootid == exodusFilePtr && nc_inq_dimid(exodusFilePtr, DIM_NUM_DIM, &numdimdim) == NC_NOERR) {
    std::string errmsg =
        fmt::format("Error: initialization already done for file id {}", exodusFilePtr);
    ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), EX_MSG);
    return EX_FATAL;
  }

  if (rootid == exodusFilePtr) {
    // We are creating a grouped file, the title and other attributes have
    // already been defined when the root group was created; don't redo now.
    int status = nc_put_att_text(rootid, NC_GLOBAL, ATT_TITLE,
                                 static_cast<int>(std::strlen(mesh.title)) + 1, mesh.title);

    // define some attributes...
    if (status != NC_NOERR) {
      ex_opts(EX_VERBOSE);
      std::string errmsg =
          fmt::format("Error: failed to define title attribute to file id {}", exodusFilePtr);
      ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      return EX_FATAL;
    }

    // For use later as a consistency check, define the number of processors and
    // the current processor id as an attribute of the file...
    if (comm.outputNemesis && comm.processorCount > 1) {
      std::array ltempsv{comm.processorCount, comm.processorId};
      status = nc_put_att_int(rootid, NC_GLOBAL, "processor_info", NC_INT, 2, Data(ltempsv));
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        std::string errmsg = fmt::format(
            "Error: failed to define processor info attribute to file id {}", exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        return EX_FATAL;
      }
    }

    // For use later to determine whether a timestep is corrupt, we define an attribute
    // containing the last written time...
    {
      double fake_time = -1.0e38;
      status = nc_put_att_double(rootid, NC_GLOBAL, "last_written_time", NC_DOUBLE, 1, &fake_time);
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        std::string errmsg = fmt::format(
            "Error: failed to define 'last_written_time' attribute to file id {}", exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        return EX_FATAL;
      }
    }

    // For use later to help readers know how much memory to allocate
    // for name storage, we define an attribute containing the maximum
    // size of any name.
    {
      int current_len = 0;
      status = nc_put_att_int(rootid, NC_GLOBAL, ATT_MAX_NAME_LENGTH, NC_INT, 1, &current_len);
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        std::string errmsg = fmt::format(
            "Error: failed to define ATT_MAX_NAME_LENGTH attribute to file id {}", exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        return EX_FATAL;
      }
    }
  }

  /* create name string length dimension */
  if (maximumNameLength < 32) {
    maximumNameLength = 32;
  }
  if (nc_inq_dimid(rootid, DIM_STR_NAME, &namestrdim) != NC_NOERR) {
    int status = nc_def_dim(rootid, DIM_STR_NAME, maximumNameLength + 1, &namestrdim);
    if (status != NC_NOERR) {
      ex_opts(EX_VERBOSE);
      std::string errmsg =
          fmt::format("Error: failed to define name string length in file id {}", exodusFilePtr);
      ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      return EX_FATAL;
    }
  }

  int status = nc_def_dim(exodusFilePtr, DIM_NUM_DIM, mesh.dimensionality, &numdimdim);
  if (status != NC_NOERR) {
    ex_opts(EX_VERBOSE);
    std::string errmsg =
        fmt::format("Error: failed to define number of dimensions in file id {}", exodusFilePtr);
    ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
    return EX_FATAL;
  }

  if ((status = nc_def_dim(exodusFilePtr, DIM_TIME, NC_UNLIMITED, &timedim)) != NC_NOERR) {
    std::string errmsg =
        fmt::format("Error: failed to define time dimension in file id {}", exodusFilePtr);
    ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
    return EX_FATAL;
  }

  std::array dim{timedim};
  if ((status = nc_def_var(exodusFilePtr, VAR_WHOLE_TIME, nc_flt_code(exodusFilePtr), 1, Data(dim),
                           &varid)) != NC_NOERR) {
    std::string errmsg = fmt::format(
        "Error: failed to define whole time step variable in file id {}", exodusFilePtr);
    ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
    return EX_FATAL;
  }
  {
    struct exi_file_item *file = exi_find_file_item(exodusFilePtr);
    if (file != nullptr) {
      file->time_varid = varid;
    }
  }
  exi_compress_variable(exodusFilePtr, varid, -2);

  if (!mesh.nodeblocks.empty() && mesh.nodeblocks[0].entityCount > 0) {
    status = nc_def_dim(exodusFilePtr, DIM_NUM_NODES, mesh.nodeblocks[0].entityCount, &numnoddim);
    if (status != NC_NOERR) {
      ex_opts(EX_VERBOSE);
      std::string errmsg =
          fmt::format("Error: failed to define number of nodes in file id {}", exodusFilePtr);
      ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      return EX_FATAL;
    }

    // Define the node map here to avoid a later redefine call
    if (mesh.use_node_map) {
      std::array dims1{numnoddim};
      status = nc_def_var(exodusFilePtr, VAR_NODE_NUM_MAP, map_type, 1, Data(dims1), &varid);
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        if (status == NC_ENAMEINUSE) {
          std::string errmsg =
              fmt::format("Error: node numbering map already exists in file id {}", exodusFilePtr);
          ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        }
        else {
          std::string errmsg = fmt::format(
              "Error: failed to create node numbering map array in file id {}", exodusFilePtr);
          ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        }
        return EX_FATAL;
      }
      exi_compress_variable(exodusFilePtr, varid, 1);
    }
  }

  if (!mesh.nodeblocks.empty() && mesh.nodeblocks[0].attributeCount > 0) {
    int numattrdim;
    status = nc_def_dim(exodusFilePtr, DIM_NUM_ATT_IN_NBLK, mesh.nodeblocks[0].attributeCount,
                        &numattrdim);
    if (status != NC_NOERR) {
      ex_opts(EX_VERBOSE);
      std::string errmsg =
          fmt::format("Error: failed to define number of attributes in node block {}"
                      " in file id {}",
                      static_cast<entity_id>(mesh.nodeblocks[0].id), exodusFilePtr);
      ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      return EX_FATAL;
    }

    std::array dims{numnoddim, numattrdim};
    status =
        nc_def_var(exodusFilePtr, VAR_NATTRIB, nc_flt_code(exodusFilePtr), 2, Data(dims), &varid);
    if (status != NC_NOERR) {
      ex_opts(EX_VERBOSE);
      std::string errmsg =
          fmt::format("Error:  failed to define attributes for node block {} in file id {}",
                      mesh.nodeblocks[0].id, exodusFilePtr);
      ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      return EX_FATAL;
    }
    exi_compress_variable(exodusFilePtr, varid, 2);

    // Attribute name array...
    dims[0] = numattrdim;
    dims[1] = namestrdim;

    status = nc_def_var(exodusFilePtr, VAR_NAME_NATTRIB, NC_CHAR, 2, Data(dims), &varid);
    if (status != NC_NOERR) {
      ex_opts(EX_VERBOSE);
      std::string errmsg =
          fmt::format("Error: failed to define attribute name array for node block {}"
                      " in file id {}",
                      mesh.nodeblocks[0].id, exodusFilePtr);
      ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      return EX_FATAL;
    }
    exi_set_compact_storage(exodusFilePtr, varid);
  }

  size_t elem_count = 0;
  for (const auto &elem : mesh.elemblocks) {
    elem_count += elem.entityCount;
  }

  if (elem_count > 0) {
    int numelemdim;
    status = nc_def_dim(exodusFilePtr, DIM_NUM_ELEM, elem_count, &numelemdim);
    if (status != NC_NOERR) {
      ex_opts(EX_VERBOSE);
      std::string errmsg =
          fmt::format("Error: failed to define number of elements in file id {}", exodusFilePtr);
      ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      return EX_FATAL;
    }

    // Define the element map here to avoid a later redefine call
    if (mesh.use_elem_map) {
      std::array dims{numelemdim};
      varid  = 0;
      status = nc_def_var(exodusFilePtr, VAR_ELEM_NUM_MAP, map_type, 1, Data(dims), &varid);
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        if (status == NC_ENAMEINUSE) {
          std::string errmsg = fmt::format(
              "Error: element numbering map already exists in file id {}", exodusFilePtr);
          ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        }
        else {
          std::string errmsg = fmt::format(
              "Error: failed to create element numbering map in file id {}", exodusFilePtr);
          ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        }
        return EX_FATAL;
      }
      exi_compress_variable(exodusFilePtr, varid, 1);
    }
  }

  size_t face_count = 0;
  for (const auto &elem : mesh.faceblocks) {
    face_count += elem.entityCount;
  }

  if (face_count > 0) {
    int numfacedim;
    status = nc_def_dim(exodusFilePtr, DIM_NUM_FACE, face_count, &numfacedim);
    if (status != NC_NOERR) {
      ex_opts(EX_VERBOSE);
      std::string errmsg =
          fmt::format("Error: failed to define number of faces in file id {}", exodusFilePtr);
      ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      return EX_FATAL;
    }

    // Define the face map here to avoid a later redefine call
    if (mesh.use_face_map) {
      std::array dims{numfacedim};
      varid  = 0;
      status = nc_def_var(exodusFilePtr, VAR_FACE_NUM_MAP, map_type, 1, Data(dims), &varid);
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        if (status == NC_ENAMEINUSE) {
          std::string errmsg =
              fmt::format("Error: face numbering map already exists in file id {}", exodusFilePtr);
          ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        }
        else {
          std::string errmsg = fmt::format(
              "Error: failed to create face numbering map in file id {}", exodusFilePtr);
          ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        }
        return EX_FATAL;
      }
      exi_compress_variable(exodusFilePtr, varid, 1);
    }
  }

  size_t edge_count = 0;
  for (const auto &elem : mesh.edgeblocks) {
    edge_count += elem.entityCount;
  }

  if (edge_count > 0) {
    int numedgedim;
    status = nc_def_dim(exodusFilePtr, DIM_NUM_EDGE, edge_count, &numedgedim);
    if (status != NC_NOERR) {
      ex_opts(EX_VERBOSE);
      std::string errmsg =
          fmt::format("Error: failed to define number of edges in file id {}", exodusFilePtr);
      ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      return EX_FATAL;
    }

    // Define the edge map here to avoid a later redefine call
    if (mesh.use_edge_map) {
      std::array dims{numedgedim};
      varid  = 0;
      status = nc_def_var(exodusFilePtr, VAR_EDGE_NUM_MAP, map_type, 1, Data(dims), &varid);
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        if (status == NC_ENAMEINUSE) {
          std::string errmsg =
              fmt::format("Error: edge numbering map already exists in file id {}", exodusFilePtr);
          ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        }
        else {
          std::string errmsg = fmt::format(
              "Error: failed to create edge numbering map in file id {}", exodusFilePtr);
          ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        }
        return EX_FATAL;
      }
      exi_compress_variable(exodusFilePtr, varid, 1);
    }
  }

  // ========================================================================
  // Blocks...
  if (define_netcdf_vars(exodusFilePtr, "edge block", mesh.edgeblocks.size(), DIM_NUM_ED_BLK,
                         VAR_STAT_ED_BLK, VAR_ID_ED_BLK, VAR_NAME_ED_BLK) != EX_NOERR) {
    return EX_FATAL;
  }

  if (define_netcdf_vars(exodusFilePtr, "face block", mesh.faceblocks.size(), DIM_NUM_FA_BLK,
                         VAR_STAT_FA_BLK, VAR_ID_FA_BLK, VAR_NAME_FA_BLK) != EX_NOERR) {
    return EX_FATAL;
  }

  if (define_netcdf_vars(exodusFilePtr, "element block", mesh.elemblocks.size(), DIM_NUM_EL_BLK,
                         VAR_STAT_EL_BLK, VAR_ID_EL_BLK, VAR_NAME_EL_BLK) != EX_NOERR) {
    return EX_FATAL;
  }

  // ========================================================================
  // Sets...
  if (define_netcdf_vars(exodusFilePtr, "node set", mesh.nodesets.size(), DIM_NUM_NS, VAR_NS_STAT,
                         VAR_NS_IDS, VAR_NAME_NS) != EX_NOERR) {
    return EX_FATAL;
  }

  if (define_netcdf_vars(exodusFilePtr, "edge set", mesh.edgesets.size(), DIM_NUM_ES, VAR_ES_STAT,
                         VAR_ES_IDS, VAR_NAME_ES) != EX_NOERR) {
    return EX_FATAL;
  }

  if (define_netcdf_vars(exodusFilePtr, "face set", mesh.facesets.size(), DIM_NUM_FS, VAR_FS_STAT,
                         VAR_FS_IDS, VAR_NAME_FS) != EX_NOERR) {
    return EX_FATAL;
  }

  if (define_netcdf_vars(exodusFilePtr, "element set", mesh.elemsets.size(), DIM_NUM_ELS,
                         VAR_ELS_STAT, VAR_ELS_IDS, VAR_NAME_ELS) != EX_NOERR) {
    return EX_FATAL;
  }

  // ========================================================================
  // side sets...
  if (define_netcdf_vars(exodusFilePtr, "side set", mesh.sidesets.size(), DIM_NUM_SS, VAR_SS_STAT,
                         VAR_SS_IDS, VAR_NAME_SS) != EX_NOERR) {
    return EX_FATAL;
  }

  // ========================================================================
  if (!mesh.nodeblocks.empty()) {
    if (define_coordinate_vars(exodusFilePtr, mesh.nodeblocks[0].entityCount, numnoddim,
                               mesh.dimensionality, numdimdim, namestrdim) != EX_NOERR) {
      return EX_FATAL;
    }
  }

  // Define dimension for the number of processors
  if (comm.outputNemesis) {
    if (comm.processorCount > 0) {
      int procdim;
      status = nc_inq_dimid(exodusFilePtr, DIM_NUM_PROCS, &procdim);
      if (status != NC_NOERR) {
        int ltempsv = comm.processorCount;
        status      = nc_def_dim(exodusFilePtr, DIM_NUM_PROCS, ltempsv, &procdim);
        if (status != NC_NOERR) {
          ex_opts(EX_VERBOSE);
          std::string errmsg = fmt::format("Error: failed to dimension \"{}\" in file ID {}",
                                           DIM_NUM_PROCS, exodusFilePtr);
          ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
          return EX_FATAL;
        }
      }
    }

    // If this is a parallel file then the status vectors are size 1
    int dimid_npf;
    status = nc_inq_dimid(exodusFilePtr, DIM_NUM_PROCS_F, &dimid_npf);
    if ((status) != NC_NOERR) {
      int ltempsv = 1; // 1 processor per file...
      status      = nc_def_dim(exodusFilePtr, DIM_NUM_PROCS_F, ltempsv, &dimid_npf);
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        std::string errmsg = fmt::format("Error: failed to dimension \"{}\" in file ID {}",
                                         DIM_NUM_PROCS_F, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        return EX_FATAL;
      }
    }

    // Define the file type variable...
    status = nc_inq_varid(exodusFilePtr, VAR_FILE_TYPE, &varid);
    if (status != NC_NOERR) {
      status = nc_def_var(exodusFilePtr, VAR_FILE_TYPE, NC_INT, 0, nullptr, &varid);
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        std::string errmsg =
            fmt::format("Error: failed to define file type in file ID {}", exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        return EX_FATAL;
      }
    }
    exi_set_compact_storage(exodusFilePtr, varid);

    // Output the file version
    int ierr = exi_put_nemesis_version(exodusFilePtr);
    if (ierr < 0) {
      return (ierr);
    }

    if (comm.globalNodes > 0) {
      // Define dimension for number of global nodes
      size_t ltempsv   = comm.globalNodes;
      int    glonoddim = 0;
      status           = nc_def_dim(exodusFilePtr, DIM_NUM_NODES_GLOBAL, ltempsv, &glonoddim);
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        std::string errmsg = fmt::format("Error: failed to dimension \"{}\" in file ID {}",
                                         DIM_NUM_NODES_GLOBAL, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        return EX_FATAL;
      }
    }

    if (comm.globalElements > 0) {
      // Define dimension for number of global elements
      size_t ltempsv    = comm.globalElements;
      int    gloelemdim = 0;
      status            = nc_def_dim(exodusFilePtr, DIM_NUM_ELEMS_GLOBAL, ltempsv, &gloelemdim);
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        std::string errmsg = fmt::format("Error: failed to dimension \"{}\" in file ID {}",
                                         DIM_NUM_ELEMS_GLOBAL, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        return EX_FATAL;
      }
    }

    // Output the number of global element blocks. This is output as a
    // dimension since the vector of global element block IDs is sized
    // by this quantity.
    {
      std::array<const char *, 3> vars{VAR_ELBLK_IDS_GLOBAL, VAR_ELBLK_CNT_GLOBAL, nullptr};
      const std::array            types{ids_type, bulk_type};

      status = define_variables(exodusFilePtr, static_cast<int>(comm.globalElementBlocks),
                                DIM_NUM_ELBLK_GLOBAL, Data(vars), Data(types));
      if (status != EX_NOERR) {
        return EX_FATAL;
      }
    }

    // Output the number of global node sets. This is output as a
    // dimension since the vector of global element block IDs is sized
    // by this quantity.
    {
      std::array<const char *, 4> vars{VAR_NS_IDS_GLOBAL, VAR_NS_NODE_CNT_GLOBAL,
                                       VAR_NS_DF_CNT_GLOBAL, nullptr};
      const std::array            types{ids_type, bulk_type, bulk_type};

      status = define_variables(exodusFilePtr, static_cast<int>(comm.globalNodeSets),
                                DIM_NUM_NS_GLOBAL, Data(vars), Data(types));
      if (status != EX_NOERR) {
        return EX_FATAL;
      }
    }

    // Output the number of global side sets. This is output as a
    // dimension since the vector of global element block IDs is sized
    // by this quantity.
    {
      std::array<const char *, 4> vars{VAR_SS_IDS_GLOBAL, VAR_SS_SIDE_CNT_GLOBAL,
                                       VAR_SS_DF_CNT_GLOBAL, nullptr};
      const std::array            types{ids_type, bulk_type, bulk_type};

      status = define_variables(exodusFilePtr, static_cast<int>(comm.globalSideSets),
                                DIM_NUM_SS_GLOBAL, Data(vars), Data(types));
      if (status != EX_NOERR) {
        return EX_FATAL;
      }
    }

    // Internal Node status
    if (mesh.full_nemesis_data) {
      status = conditional_define_variable(exodusFilePtr, VAR_INT_N_STAT, dimid_npf,
                                           &nodeMapVarID[0], NC_INT);
      if (status != EX_NOERR) {
        return EX_FATAL;
      }

      // Border node status
      status = conditional_define_variable(exodusFilePtr, VAR_BOR_N_STAT, dimid_npf,
                                           &nodeMapVarID[1], NC_INT);
      if (status != EX_NOERR) {
        return EX_FATAL;
      }

      // External Node status
      status = conditional_define_variable(exodusFilePtr, VAR_EXT_N_STAT, dimid_npf,
                                           &nodeMapVarID[2], NC_INT);
      if (status != EX_NOERR) {
        return EX_FATAL;
      }

      // Define the variable IDs for the elemental status vectors
      // Internal elements
      status = conditional_define_variable(exodusFilePtr, VAR_INT_E_STAT, dimid_npf,
                                           &elementMapVarID[0], NC_INT);
      if (status != EX_NOERR) {
        return EX_FATAL;
      }

      // Border elements
      status = conditional_define_variable(exodusFilePtr, VAR_BOR_E_STAT, dimid_npf,
                                           &elementMapVarID[1], NC_INT);
      if (status != EX_NOERR) {
        return EX_FATAL;
      }

      // Define variable for the internal element information
      status = define_variable(exodusFilePtr, comm.elementsInternal, DIM_NUM_INT_ELEMS,
                               VAR_ELEM_MAP_INT, bulk_type);
      if (status != EX_NOERR) {
        return EX_FATAL;
      }

      // Define variable for the border element information
      status = define_variable(exodusFilePtr, comm.elementsBorder, DIM_NUM_BOR_ELEMS,
                               VAR_ELEM_MAP_BOR, bulk_type);
      if (status != EX_NOERR) {
        return EX_FATAL;
      }

      // Define variable for vector of internal FEM node IDs
      status = define_variable(exodusFilePtr, comm.nodesInternal, DIM_NUM_INT_NODES,
                               VAR_NODE_MAP_INT, bulk_type);
      if (status != EX_NOERR) {
        return EX_FATAL;
      }

      // Define variable for vector of border FEM node IDs
      status = define_variable(exodusFilePtr, comm.nodesBorder, DIM_NUM_BOR_NODES, VAR_NODE_MAP_BOR,
                               bulk_type);
      if (status != EX_NOERR) {
        return EX_FATAL;
      }

      // Define dimension for vector of external FEM node IDs
      status = define_variable(exodusFilePtr, comm.nodesExternal, DIM_NUM_EXT_NODES,
                               VAR_NODE_MAP_EXT, bulk_type);
      if (status != EX_NOERR) {
        return EX_FATAL;
      }
    }

    // Add the nodal communication map count

    size_t ncnt_cmap = 0;
    for (const auto &elem : comm.nodeMap) {
      ncnt_cmap += elem.entityCount;
    }

    {
      std::array<const char *, 4> vars{VAR_N_COMM_IDS, VAR_N_COMM_STAT, VAR_N_COMM_DATA_IDX,
                                       nullptr};
      const std::array            types{ids_type, NC_INT, bulk_type};

      status = define_variables(exodusFilePtr, static_cast<int>(comm.nodeMap.size()),
                                DIM_NUM_N_CMAPS, Data(vars), Data(types));
      if (status != EX_NOERR) {
        return EX_FATAL;
      }
    }
    {
      std::array<const char *, 3> vars{VAR_N_COMM_NIDS, VAR_N_COMM_PROC, nullptr};
      const std::array            types{ids_type, NC_INT};

      // Add dimensions for all of the nodal communication maps
      status = define_variables(exodusFilePtr, ncnt_cmap, DIM_NCNT_CMAP, Data(vars), Data(types));
      if (status != EX_NOERR) {
        return EX_FATAL;
      }
    }

    // Add the nodal communication map count
    size_t ecnt_cmap = 0;
    for (const auto &elem : comm.elementMap) {
      ecnt_cmap += elem.entityCount;
    }

    if (mesh.full_nemesis_data) {
      {
        std::array<const char *, 4> vars{VAR_E_COMM_IDS, VAR_E_COMM_STAT, VAR_E_COMM_DATA_IDX,
                                         nullptr};
        const std::array            types{ids_type, NC_INT, bulk_type};

        status = define_variables(exodusFilePtr, static_cast<int>(comm.elementMap.size()),
                                  DIM_NUM_E_CMAPS, Data(vars), Data(types));
        if (status != EX_NOERR) {
          return EX_FATAL;
        }
      }
      {
        std::array<const char *, 4> vars{VAR_E_COMM_EIDS, VAR_E_COMM_PROC, VAR_E_COMM_SIDS,
                                         nullptr};
        const std::array            types{ids_type, NC_INT, bulk_type};
        status = define_variables(exodusFilePtr, ecnt_cmap, DIM_ECNT_CMAP, Data(vars), Data(types));
        if (status != EX_NOERR) {
          return EX_FATAL;
        }
      }
    }
  }
  return EX_NOERR;
}

int Internals::put_metadata(const std::vector<Assembly> &assemblies)
{
  if (assemblies.empty()) {
    return EX_NOERR;
  }
  int status;
  if ((status = exi_check_valid_file_id(exodusFilePtr, __func__)) != EX_NOERR) {
    std::string errmsg = fmt::format("Error: Invalid exodus file handle: {}", exodusFilePtr);
    ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
    return EX_FATAL;
  }

  int int_type = NC_INT;
  if (ex_int64_status(exodusFilePtr) & EX_IDS_INT64_DB) {
    int_type = NC_INT64;
  }

  for (const auto &assembly : assemblies) {
    char *numentryptr = DIM_NUM_ENTITY_ASSEMBLY(assembly.id);

    /* define dimensions and variables */
    int dimid;
    status = nc_def_dim(exodusFilePtr, numentryptr, assembly.entityCount, &dimid);
    if (status != NC_NOERR) {
      ex_opts(EX_VERBOSE);
      std::string errmsg = fmt::format(
          "Error: failed to define number of entities in assembly in file id {}", exodusFilePtr);
      ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      return EX_FATAL;
    }

    /* create variable array in which to store the entry lists */
    int        entlst_id;
    std::array dims{dimid};
    if ((status = nc_def_var(exodusFilePtr, VAR_ENTITY_ASSEMBLY(assembly.id), int_type, 1,
                             Data(dims), &entlst_id)) != NC_NOERR) {
      std::string errmsg = fmt::format(
          "Error: failed to define entity assembly variable in file id {}", exodusFilePtr);
      ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      return EX_FATAL;
    }
    exi_compress_variable(exodusFilePtr, entlst_id, 1);

    if (ex_int64_status(exodusFilePtr) & EX_IDS_INT64_DB) {
      long long tmp = assembly.id;
      status = nc_put_att_longlong(exodusFilePtr, entlst_id, EX_ATTRIBUTE_ID, NC_INT64, 1, &tmp);
    }
    else {
      int id = assembly.id;
      status = nc_put_att_int(exodusFilePtr, entlst_id, EX_ATTRIBUTE_ID, NC_INT, 1, &id);
    }
    if (status != NC_NOERR) {
      ex_opts(EX_VERBOSE);
      std::string errmsg = fmt::format("Error: failed to define '{}' attribute to file id {}",
                                       EX_ATTRIBUTE_ID, exodusFilePtr);
      ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      return EX_FATAL;
    }

    int type = assembly.type;
    status   = nc_put_att_int(exodusFilePtr, entlst_id, EX_ATTRIBUTE_TYPE, NC_INT, 1, &type);
    if (status != NC_NOERR) {
      ex_opts(EX_VERBOSE);
      std::string errmsg = fmt::format("Error: failed to define '{}' attribute to file id {}",
                                       EX_ATTRIBUTE_TYPE, exodusFilePtr);
      ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      return EX_FATAL;
    }

    status = nc_put_att_text(exodusFilePtr, entlst_id, EX_ATTRIBUTE_NAME, assembly.name.size() + 1,
                             assembly.name.c_str());
    if (status != NC_NOERR) {
      ex_opts(EX_VERBOSE);
      std::string errmsg = fmt::format("Error: failed to define '{}' attribute to file id {}",
                                       EX_ATTRIBUTE_NAME, exodusFilePtr);
      ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      return EX_FATAL;
    }

    {
      char *contains = ex_name_of_object(assembly.type);
      status         = nc_put_att_text(exodusFilePtr, entlst_id, EX_ATTRIBUTE_TYPENAME,
                                       strlen(contains) + 1, contains);
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        std::string errmsg = fmt::format("Error: failed to define '{}' attribute to file id {}",
                                         EX_ATTRIBUTE_TYPENAME, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        return EX_FATAL;
      }
    }

    /* Increment assembly count */
    struct exi_file_item *file = exi_find_file_item(exodusFilePtr);
    if (file != nullptr) {
      file->assembly_count++;
    }
  }
  return EX_NOERR;
}

int Internals::put_metadata(const std::vector<Blob> &blobs)
{
  if (blobs.empty()) {
    return EX_NOERR;
  }

  int status;

  int n1dim;
  if ((status = nc_def_dim(exodusFilePtr, DIM_N1, 1L, &n1dim)) != NC_NOERR) {
    ex_opts(EX_VERBOSE);
    std::string errmsg =
        fmt::format("Error: failed to define number \"1\" dimension in file id {}", exodusFilePtr);
    ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
    return EX_FATAL;
  }

  for (const auto &blob : blobs) {
    char *numentryptr = DIM_NUM_VALUES_BLOB(blob.id);

    // define dimensions and variables
    int dimid;
    if ((status = nc_def_dim(exodusFilePtr, numentryptr, blob.entityCount, &dimid)) != NC_NOERR) {
      ex_opts(EX_VERBOSE);
      std::string errmsg =
          fmt::format("Error: failed to define number of entries in blob {} in file id {}", blob.id,
                      exodusFilePtr);
      ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      return EX_FATAL;
    }

    // create a variable just as a way to have a blob and its attributes; values not used for
    // anything
    std::array dims{n1dim};
    int        entlst;
    if ((status = nc_def_var(exodusFilePtr, VAR_ENTITY_BLOB(blob.id), NC_INT, 1, Data(dims),
                             &entlst)) != NC_NOERR) {
      std::string errmsg = fmt::format("Error: failed to create entity for blob {} in file id {}",
                                       blob.id, exodusFilePtr);
      ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      return EX_FATAL;
    }
    exi_set_compact_storage(exodusFilePtr, entlst);

    if (ex_int64_status(exodusFilePtr) & EX_IDS_INT64_DB) {
      long long tmp = blob.id;
      status = nc_put_att_longlong(exodusFilePtr, entlst, EX_ATTRIBUTE_ID, NC_INT64, 1, &tmp);
    }
    else {
      int id = blob.id;
      status = nc_put_att_int(exodusFilePtr, entlst, EX_ATTRIBUTE_ID, NC_INT, 1, &id);
    }
    if (status != NC_NOERR) {
      std::string errmsg =
          fmt::format("Error: failed to store blob id {} in file id {}", blob.id, exodusFilePtr);
      ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      return EX_FATAL;
    }

    if ((status = nc_put_att_text(exodusFilePtr, entlst, EX_ATTRIBUTE_NAME, blob.name.length() + 1,
                                  blob.name.c_str())) != NC_NOERR) {
      std::string errmsg = fmt::format("Error: failed to store blob name {} in file id {}",
                                       blob.name, exodusFilePtr);
      ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      return EX_FATAL;
    }
  }
  return EX_NOERR;
}

int Internals::put_metadata(const std::vector<ElemBlock> &blocks, bool count_only)
{
  int status = 0; // clear error code

  if (blocks.empty()) {
    return EX_NOERR;
  }

  int bulk_type = get_type(exodusFilePtr, EX_BULK_INT64_DB);

  // Get number of element blocks defined for this file
  int    dimid;
  size_t num_elem_blk = 0;
  status              = nc_inq_dimid(exodusFilePtr, DIM_NUM_EL_BLK, &dimid);
  if (status != NC_NOERR) {
    ex_opts(EX_VERBOSE);
    std::string errmsg =
        fmt::format("Error: no element blocks defined in file id {}", exodusFilePtr);
    ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
    return EX_FATAL;
  }

  int namestrdim;
  status = nc_inq_dimid(exodusFilePtr, DIM_STR_NAME, &namestrdim);
  if (status != NC_NOERR) {
    ex_opts(EX_VERBOSE);
    std::string errmsg =
        fmt::format("Error: failed to get string length in file id {}", exodusFilePtr);
    ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
    return EX_FATAL;
  }

  status = nc_inq_dimlen(exodusFilePtr, dimid, &num_elem_blk);
  if (status != NC_NOERR) {
    ex_opts(EX_VERBOSE);
    std::string errmsg =
        fmt::format("Error: failed to get number of element blocks in file id {}", exodusFilePtr);
    ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
    return EX_FATAL;
  }

  SMART_ASSERT(blocks.size() == num_elem_blk);

  // Iterate over blocks ...
  for (size_t iblk = 0; iblk < num_elem_blk; iblk++) {
    exi_inc_file_item(exodusFilePtr, exi_get_counter_list(EX_ELEM_BLOCK));

    if (blocks[iblk].entityCount == 0) {
      continue;
    }

    // define some dimensions and variables
    int numelbdim;
    status = nc_def_dim(exodusFilePtr, DIM_NUM_EL_IN_BLK(iblk + 1), blocks[iblk].entityCount,
                        &numelbdim);
    if (status != NC_NOERR) {
      if (status == NC_ENAMEINUSE) { // duplicate entry
        ex_opts(EX_VERBOSE);
        std::string errmsg = fmt::format("Error: element block {} already defined in file id {}",
                                         blocks[iblk].id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      }
      else {
        ex_opts(EX_VERBOSE);
        std::string errmsg =
            fmt::format("Error: failed to define number of elements/block for block {}"
                        " file id {}",
                        blocks[iblk].id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      }
      return EX_FATAL;
    }
    if (count_only) {
      continue;
    }

    int nelnoddim;
    status = nc_def_dim(exodusFilePtr, DIM_NUM_NOD_PER_EL(iblk + 1), blocks[iblk].nodesPerEntity,
                        &nelnoddim);
    if (status != NC_NOERR) {
      ex_opts(EX_VERBOSE);
      std::string errmsg =
          fmt::format("Error: failed to define number of nodes/element for block {}"
                      " in file id {}",
                      blocks[iblk].id, exodusFilePtr);
      ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      return EX_FATAL;
    }

    // element connectivity array
    {
      std::array dims{numelbdim, nelnoddim};
      int        connid = 0;
      status = nc_def_var(exodusFilePtr, VAR_CONN(iblk + 1), bulk_type, 2, Data(dims), &connid);
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        std::string errmsg = fmt::format("Error: failed to create connectivity array for block {}"
                                         " in file id {}",
                                         blocks[iblk].id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        return EX_FATAL;
      }
      exi_compress_variable(exodusFilePtr, connid, 1);

      // store element type as attribute of connectivity variable
      status = nc_put_att_text(exodusFilePtr, connid, ATT_NAME_ELB,
                               static_cast<int>(std::strlen(blocks[iblk].elType)) + 1,
                               blocks[iblk].elType);
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        std::string errmsg =
            fmt::format("Error: failed to store element type name {} in file id {}",
                        blocks[iblk].elType, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        return EX_FATAL;
      }
    }

    if (blocks[iblk].edgesPerEntity > 0) {
      int neledgdim;
      status = nc_def_dim(exodusFilePtr, DIM_NUM_EDG_PER_EL(iblk + 1), blocks[iblk].edgesPerEntity,
                          &neledgdim);
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        std::string errmsg =
            fmt::format("Error: failed to define number of edges/element for block {}"
                        " in file id {}",
                        blocks[iblk].id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        return EX_FATAL;
      }

      // element->edge connectivity array
      std::array dims{numelbdim, neledgdim};

      int connid = 0;
      status = nc_def_var(exodusFilePtr, VAR_ECONN(iblk + 1), bulk_type, 2, Data(dims), &connid);
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        std::string errmsg =
            fmt::format("Error: failed to create element->edge connectivity array for block {}"
                        " in file id {}",
                        blocks[iblk].id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        return EX_FATAL;
      }
      exi_compress_variable(exodusFilePtr, connid, 1);
    }

    if (blocks[iblk].facesPerEntity > 0) {
      int nelfacdim;
      status = nc_def_dim(exodusFilePtr, DIM_NUM_FAC_PER_EL(iblk + 1), blocks[iblk].facesPerEntity,
                          &nelfacdim);
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        std::string errmsg =
            fmt::format("Error: failed to define number of faces/element for block {}"
                        " in file id {}",
                        blocks[iblk].id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        return EX_FATAL;
      }

      // element->face connectivity array
      std::array dims{numelbdim, nelfacdim};

      int connid = 0;
      status = nc_def_var(exodusFilePtr, VAR_FCONN(iblk + 1), bulk_type, 2, Data(dims), &connid);
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        std::string errmsg =
            fmt::format("Error: failed to create element->edge connectivity array for block {}"
                        " in file id {}",
                        blocks[iblk].id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        return EX_FATAL;
      }
      exi_compress_variable(exodusFilePtr, connid, 1);
    }

    // element attribute array
    if (blocks[iblk].attributeCount > 0) {
      int numattrdim;
      status = nc_def_dim(exodusFilePtr, DIM_NUM_ATT_IN_BLK(iblk + 1), blocks[iblk].attributeCount,
                          &numattrdim);
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        std::string errmsg = fmt::format("Error: failed to define number of attributes in block {}"
                                         " in file id {}",
                                         blocks[iblk].id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        return EX_FATAL;
      }

      {
        std::array dims{numelbdim, numattrdim};
        int        varid = 0;
        status = nc_def_var(exodusFilePtr, VAR_ATTRIB(iblk + 1), nc_flt_code(exodusFilePtr), 2,
                            Data(dims), &varid);
        if (status != NC_NOERR) {
          ex_opts(EX_VERBOSE);
          std::string errmsg =
              fmt::format("Error:  failed to define attributes for element block {}"
                          " in file id {}",
                          blocks[iblk].id, exodusFilePtr);
          ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
          return EX_FATAL;
        }
        exi_compress_variable(exodusFilePtr, varid, 2);

#if defined(PARALLEL_AWARE_EXODUS)
        // There is currently a bug in netcdf-4.5.1-devel and earlier
        // for partial parallel output of strided arrays in collective
        // mode for netcdf-4-based output.  If the number of attributes >
        // 1 and in parallel mode, set the mode to independent.
        if (blocks[iblk].attributeCount > 1) {
          struct exi_file_item *file = exi_find_file_item(exodusFilePtr);
          if (file && file->is_parallel && file->is_hdf5) {
            nc_var_par_access(exodusFilePtr, varid, NC_INDEPENDENT);
          }
        }
#endif
      }

      {
        // Attribute name array...
        std::array dims{numattrdim, namestrdim};
        int        varid = 0;
        status =
            nc_def_var(exodusFilePtr, VAR_NAME_ATTRIB(iblk + 1), NC_CHAR, 2, Data(dims), &varid);
        if (status != NC_NOERR) {
          ex_opts(EX_VERBOSE);
          std::string errmsg =
              fmt::format("Error: failed to define attribute name array for element block {}"
                          " in file id {}",
                          blocks[iblk].id, exodusFilePtr);
          ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
          return EX_FATAL;
        }
        exi_set_compact_storage(exodusFilePtr, varid);
      }
    }
  }
  return EX_NOERR;
}

int Internals::put_metadata(const std::vector<FaceBlock> &blocks, bool count_only)
{
  int status = 0; // clear error code

  int bulk_type = get_type(exodusFilePtr, EX_BULK_INT64_DB);

  if (blocks.empty()) {
    return EX_NOERR;
  }

  // Get number of face blocks defined for this file
  int    dimid;
  size_t num_face_blk = 0;
  status              = nc_inq_dimid(exodusFilePtr, DIM_NUM_FA_BLK, &dimid);
  if (status != NC_NOERR) {
    ex_opts(EX_VERBOSE);
    std::string errmsg = fmt::format("Error: no face blocks defined in file id {}", exodusFilePtr);
    ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
    return EX_FATAL;
  }

  int namestrdim;
  status = nc_inq_dimid(exodusFilePtr, DIM_STR_NAME, &namestrdim);
  if (status != NC_NOERR) {
    ex_opts(EX_VERBOSE);
    std::string errmsg =
        fmt::format("Error: failed to get string length in file id {}", exodusFilePtr);
    ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
    return EX_FATAL;
  }

  status = nc_inq_dimlen(exodusFilePtr, dimid, &num_face_blk);
  if (status != NC_NOERR) {
    ex_opts(EX_VERBOSE);
    std::string errmsg =
        fmt::format("Error: failed to get number of face blocks in file id {}", exodusFilePtr);
    ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
    return EX_FATAL;
  }

  SMART_ASSERT(blocks.size() == num_face_blk);

  // Iterate over blocks ...
  for (size_t iblk = 0; iblk < num_face_blk; iblk++) {
    exi_inc_file_item(exodusFilePtr, exi_get_counter_list(EX_FACE_BLOCK));

    if (blocks[iblk].entityCount == 0) {
      continue;
    }

    // define some dimensions and variables
    int numelbdim;
    status = nc_def_dim(exodusFilePtr, DIM_NUM_FA_IN_FBLK(iblk + 1), blocks[iblk].entityCount,
                        &numelbdim);
    if (status != NC_NOERR) {
      if (status == NC_ENAMEINUSE) { // duplicate entry
        ex_opts(EX_VERBOSE);
        std::string errmsg = fmt::format("Error: face block {} already defined in file id {}",
                                         blocks[iblk].id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      }
      else {
        ex_opts(EX_VERBOSE);
        std::string errmsg =
            fmt::format("Error: failed to define number of faces/block for block {}"
                        " file id {}",
                        blocks[iblk].id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      }
      return EX_FATAL;
    }
    if (count_only) {
      continue;
    }

    int nelnoddim;
    status = nc_def_dim(exodusFilePtr, DIM_NUM_NOD_PER_FA(iblk + 1), blocks[iblk].nodesPerEntity,
                        &nelnoddim);
    if (status != NC_NOERR) {
      ex_opts(EX_VERBOSE);
      std::string errmsg = fmt::format("Error: failed to define number of nodes/face for block {}"
                                       " in file id {}",
                                       blocks[iblk].id, exodusFilePtr);
      ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      return EX_FATAL;
    }

    // face attribute array
    if (blocks[iblk].attributeCount > 0) {
      int numattrdim;
      status = nc_def_dim(exodusFilePtr, DIM_NUM_ATT_IN_FBLK(iblk + 1), blocks[iblk].attributeCount,
                          &numattrdim);
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        std::string errmsg = fmt::format("Error: failed to define number of attributes in block {}"
                                         " in file id {}",
                                         blocks[iblk].id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        return EX_FATAL;
      }

      {
        std::array dims{numelbdim, numattrdim};
        int        varid = 0;
        status = nc_def_var(exodusFilePtr, VAR_FATTRIB(iblk + 1), nc_flt_code(exodusFilePtr), 2,
                            Data(dims), &varid);
        if (status != NC_NOERR) {
          ex_opts(EX_VERBOSE);
          std::string errmsg =
              fmt::format("Error:  failed to define attributes for face block {} in file id {}",
                          blocks[iblk].id, exodusFilePtr);
          ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
          return EX_FATAL;
        }
        exi_compress_variable(exodusFilePtr, varid, 2);
      }
      {
        // Attribute name array...
        std::array dims{numattrdim, namestrdim};
        int        varid = 0;

        status =
            nc_def_var(exodusFilePtr, VAR_NAME_FATTRIB(iblk + 1), NC_CHAR, 2, Data(dims), &varid);
        if (status != NC_NOERR) {
          ex_opts(EX_VERBOSE);
          std::string errmsg =
              fmt::format("Error: failed to define attribute name array for face block {}"
                          " in file id {}",
                          blocks[iblk].id, exodusFilePtr);
          ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
          return EX_FATAL;
        }
        exi_set_compact_storage(exodusFilePtr, varid);
      }
    }

    {
      // face connectivity array
      std::array dims{numelbdim, nelnoddim};
      int        connid = 0;
      status = nc_def_var(exodusFilePtr, VAR_FBCONN(iblk + 1), bulk_type, 2, Data(dims), &connid);
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        std::string errmsg =
            fmt::format("Error: failed to create connectivity array for block {} in file id {}",
                        blocks[iblk].id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        return EX_FATAL;
      }
      exi_compress_variable(exodusFilePtr, connid, 1);

      // store element type as attribute of connectivity variable
      status = nc_put_att_text(exodusFilePtr, connid, ATT_NAME_ELB,
                               static_cast<int>(std::strlen(blocks[iblk].elType)) + 1,
                               blocks[iblk].elType);
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        std::string errmsg =
            fmt::format("Error: failed to store element type name {} in file id {}",
                        blocks[iblk].elType, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        return EX_FATAL;
      }
    }
  }
  return EX_NOERR;
}

int Internals::put_metadata(const std::vector<EdgeBlock> &blocks, bool count_only)
{
  if (blocks.empty()) {
    return EX_NOERR;
  }

  // Get number of edge blocks defined for this file
  int    dimid;
  size_t num_edge_blk = 0;
  int    status       = nc_inq_dimid(exodusFilePtr, DIM_NUM_ED_BLK, &dimid);
  if (status != NC_NOERR) {
    ex_opts(EX_VERBOSE);
    std::string errmsg = fmt::format("Error: no edge blocks defined in file id {}", exodusFilePtr);
    ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
    return EX_FATAL;
  }

  int namestrdim;
  status = nc_inq_dimid(exodusFilePtr, DIM_STR_NAME, &namestrdim);
  if (status != NC_NOERR) {
    ex_opts(EX_VERBOSE);
    std::string errmsg =
        fmt::format("Error: failed to get string length in file id {}", exodusFilePtr);
    ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
    return EX_FATAL;
  }

  status = nc_inq_dimlen(exodusFilePtr, dimid, &num_edge_blk);
  if (status != NC_NOERR) {
    ex_opts(EX_VERBOSE);
    std::string errmsg =
        fmt::format("Error: failed to get number of edge blocks in file id {}", exodusFilePtr);
    ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
    return EX_FATAL;
  }

  SMART_ASSERT(blocks.size() == num_edge_blk);

  // Iterate over blocks ...
  for (size_t iblk = 0; iblk < num_edge_blk; iblk++) {
    exi_inc_file_item(exodusFilePtr, exi_get_counter_list(EX_EDGE_BLOCK));

    if (blocks[iblk].entityCount == 0) {
      continue;
    }

    // define some dimensions and variables
    int numelbdim;
    status = nc_def_dim(exodusFilePtr, DIM_NUM_ED_IN_EBLK(iblk + 1), blocks[iblk].entityCount,
                        &numelbdim);
    if (status != NC_NOERR) {
      if (status == NC_ENAMEINUSE) { // duplicate entry
        ex_opts(EX_VERBOSE);
        std::string errmsg = fmt::format("Error: edge block {} already defined in file id {}",
                                         blocks[iblk].id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      }
      else {
        ex_opts(EX_VERBOSE);
        std::string errmsg =
            fmt::format("Error: failed to define number of edges/block for block {}"
                        " file id {}",
                        blocks[iblk].id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      }
      return EX_FATAL;
    }
    if (count_only) {
      continue;
    }

    int nelnoddim;
    status = nc_def_dim(exodusFilePtr, DIM_NUM_NOD_PER_ED(iblk + 1), blocks[iblk].nodesPerEntity,
                        &nelnoddim);
    if (status != NC_NOERR) {
      ex_opts(EX_VERBOSE);
      std::string errmsg =
          fmt::format("Error: failed to define number of nodes/edge ({}) for block {}"
                      " in file id {}",
                      blocks[iblk].nodesPerEntity, blocks[iblk].id, exodusFilePtr);
      ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      return EX_FATAL;
    }

    // edge attribute array
    if (blocks[iblk].attributeCount > 0) {
      int numattrdim;
      status = nc_def_dim(exodusFilePtr, DIM_NUM_ATT_IN_EBLK(iblk + 1), blocks[iblk].attributeCount,
                          &numattrdim);
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        std::string errmsg = fmt::format("Error: failed to define number of attributes in block {}"
                                         " in file id {}",
                                         blocks[iblk].id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        return EX_FATAL;
      }

      std::array dims{numelbdim, numattrdim};
      int        varid = 0;
      status = nc_def_var(exodusFilePtr, VAR_EATTRIB(iblk + 1), nc_flt_code(exodusFilePtr), 2,
                          Data(dims), &varid);
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        std::string errmsg =
            fmt::format("Error:  failed to define attributes for edge block {} in file id {}",
                        blocks[iblk].id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        return EX_FATAL;
      }
      exi_compress_variable(exodusFilePtr, varid, 2);

      // Attribute name array...
      dims[0] = numattrdim;
      dims[1] = namestrdim;

      status =
          nc_def_var(exodusFilePtr, VAR_NAME_EATTRIB(iblk + 1), NC_CHAR, 2, Data(dims), &varid);
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        std::string errmsg =
            fmt::format("Error: failed to define attribute name array for edge block {}"
                        " in file id {}",
                        blocks[iblk].id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        return EX_FATAL;
      }
      exi_set_compact_storage(exodusFilePtr, varid);
    }

    // edge connectivity array
    std::array dims{numelbdim, nelnoddim};

    int connid    = 0;
    int bulk_type = get_type(exodusFilePtr, EX_BULK_INT64_DB);
    status = nc_def_var(exodusFilePtr, VAR_EBCONN(iblk + 1), bulk_type, 2, Data(dims), &connid);
    if (status != NC_NOERR) {
      ex_opts(EX_VERBOSE);
      std::string errmsg =
          fmt::format("Error: failed to create connectivity array for block {} in file id {}",
                      blocks[iblk].id, exodusFilePtr);
      ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      return EX_FATAL;
    }
    exi_compress_variable(exodusFilePtr, connid, 1);

    // store element type as attribute of connectivity variable
    status = nc_put_att_text(exodusFilePtr, connid, ATT_NAME_ELB,
                             static_cast<int>(std::strlen(blocks[iblk].elType)) + 1,
                             blocks[iblk].elType);
    if (status != NC_NOERR) {
      ex_opts(EX_VERBOSE);
      std::string errmsg = fmt::format("Error: failed to store element type name {} in file id {}",
                                       blocks[iblk].elType, exodusFilePtr);
      ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      return EX_FATAL;
    }
  }
  return EX_NOERR;
}

int Internals::put_non_define_data(const CommunicationMetaData &comm, bool full_nemesis_data)
{
  // Metadata that must be written outside of define mode...
  if (comm.outputNemesis) {
    int status = 0;

    // Output the file type
    int varid;
    status = nc_inq_varid(exodusFilePtr, VAR_FILE_TYPE, &varid);
    if (status != NC_NOERR) {
      ex_opts(EX_VERBOSE);
      std::string errmsg =
          fmt::format("Error: failed to locate file type in file ID {}", exodusFilePtr);
      ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      return EX_FATAL;
    }

    int lftype = 0; // Parallel file...
    status     = nc_put_var1_int(exodusFilePtr, varid, nullptr, &lftype);
    if (status != NC_NOERR) {
      ex_opts(EX_VERBOSE);
      std::string errmsg =
          fmt::format("Error: unable to output file type variable in file ID {}", exodusFilePtr);
      ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      return EX_FATAL;
    }

    if (full_nemesis_data) {
      int nmstat = comm.nodesInternal == 0 ? 0 : 1;
      status     = nc_put_var_int(exodusFilePtr, nodeMapVarID[0], &nmstat);
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        std::string errmsg = fmt::format(
            "Error: failed to output status for internal node map in file ID {}", exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        return EX_FATAL;
      }

      nmstat = comm.nodesBorder == 0 ? 0 : 1;
      status = nc_put_var_int(exodusFilePtr, nodeMapVarID[1], &nmstat);
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        std::string errmsg = fmt::format(
            "Error: failed to output status for border node map in file ID {}", exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        return EX_FATAL;
      }

      nmstat = comm.nodesExternal == 0 ? 0 : 1;
      status = nc_put_var_int(exodusFilePtr, nodeMapVarID[2], &nmstat);
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        std::string errmsg = fmt::format(
            "Error: failed to output status for external node map in file ID {}", exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        return EX_FATAL;
      }

      nmstat = comm.elementsInternal == 0 ? 0 : 1;
      status = nc_put_var_int(exodusFilePtr, elementMapVarID[0], &nmstat);
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        std::string errmsg = fmt::format(
            "Error: failed to output status for internal elem map in file ID {}", exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        return EX_FATAL;
      }

      nmstat = comm.elementsBorder == 0 ? 0 : 1;
      status = nc_put_var_int(exodusFilePtr, elementMapVarID[1], &nmstat);
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        std::string errmsg = fmt::format(
            "Error: failed to output status for border elem map in file ID {}", exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        return EX_FATAL;
      }
    }

    size_t ncnt_cmap = 0;
    for (const auto &nmap : comm.nodeMap) {
      ncnt_cmap += nmap.entityCount;
    }

    if (!comm.nodeMap.empty() && ncnt_cmap > 0) {
      int n_varid;
      status = nc_inq_varid(exodusFilePtr, VAR_N_COMM_STAT, &n_varid);
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        std::string errmsg =
            fmt::format("Error: failed to find variable ID for \"{}\" in file ID {}",
                        VAR_N_COMM_STAT, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        return EX_FATAL;
      }

      long long nl_ncnt_cmap = 0;
      for (size_t icm = 0; icm < comm.nodeMap.size(); icm++) {

        std::array start{icm};
        int        nmstat = comm.nodeMap[icm].entityCount > 0 ? 1 : 0;
        status            = nc_put_var1_int(exodusFilePtr, n_varid, Data(start), &nmstat);
        if (status != NC_NOERR) {
          ex_opts(EX_VERBOSE);
          std::string errmsg =
              fmt::format("Error: unable to output variable in file ID {}", exodusFilePtr);
          ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
          return EX_FATAL;
        }

        // increment to the next starting position
        nl_ncnt_cmap += comm.nodeMap[icm].entityCount;

        // fill the cmap data index
        status = nc_inq_varid(exodusFilePtr, VAR_N_COMM_DATA_IDX, &commIndexVar);
        if (status != NC_NOERR) {
          ex_opts(EX_VERBOSE);
          std::string errmsg = fmt::format(
              "Error: failed to locate node communication map in file id {}", exodusFilePtr);
          ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
          return EX_FATAL;
        }
        status = nc_put_var1_longlong(exodusFilePtr, commIndexVar, Data(start), &nl_ncnt_cmap);

        if (status != NC_NOERR) {
          ex_opts(EX_VERBOSE);
          std::string errmsg = fmt::format(
              "Error: failed to output node communication map index in file ID {}", exodusFilePtr);
          ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
          return EX_FATAL;
        }
      } // End "for(icm=0; icm < num_n_comm_maps; icm++)"

      // Put Communication set ids...
      std::vector<entity_id> node_cmap_ids(comm.nodeMap.size());
      for (size_t i = 0; i < comm.nodeMap.size(); i++) {
        node_cmap_ids[i] = comm.nodeMap[i].id;
      }
      if (put_id_array(exodusFilePtr, VAR_N_COMM_IDS, node_cmap_ids) != NC_NOERR) {
        return EX_FATAL;
      }
    }
    // Set the status of the elemental communication maps
    long long ecnt_cmap = 0;
    for (const auto &elem : comm.elementMap) {
      ecnt_cmap += elem.entityCount;
    }

    if (!comm.elementMap.empty() && ecnt_cmap > 0) {

      // Get variable ID for elemental status vector
      int e_varid;
      status = nc_inq_varid(exodusFilePtr, VAR_E_COMM_STAT, &e_varid);
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        std::string errmsg =
            fmt::format("Error: failed to find variable ID for \"{}\" in file ID {}",
                        VAR_E_COMM_STAT, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        return EX_FATAL;
      }

      long long nl_ecnt_cmap = 0; // reset this for index
      for (size_t icm = 0; icm < comm.elementMap.size(); icm++) {

        std::array start{icm};
        auto       nmstat = comm.elementMap[icm].entityCount > 0 ? 1 : 0;

        status = nc_put_var1_int(exodusFilePtr, e_varid, Data(start), &nmstat);
        if (status != NC_NOERR) {
          ex_opts(EX_VERBOSE);
          std::string errmsg =
              fmt::format("Error: unable to output variable in file ID {}", exodusFilePtr);
          ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
          return EX_FATAL;
        }

        // increment to the next starting position
        nl_ecnt_cmap += comm.elementMap[icm].entityCount;

        // fill the cmap data index
        status = nc_inq_varid(exodusFilePtr, VAR_E_COMM_DATA_IDX, &elemCommIndexVar);
        if (status != NC_NOERR) {
          ex_opts(EX_VERBOSE);
          std::string errmsg = fmt::format(
              "Error: failed to locate element communication map in file id {}", exodusFilePtr);
          ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
          return EX_FATAL;
        }
        status = nc_put_var1_longlong(exodusFilePtr, elemCommIndexVar, Data(start), &nl_ecnt_cmap);
        if (status != NC_NOERR) {
          ex_opts(EX_VERBOSE);
          std::string errmsg = fmt::format(
              "Error: failed to output int elem map index in file ID {}", exodusFilePtr);
          ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
          return EX_FATAL;
        }
      } // End "for(icm=0; icm < num_e_comm_maps; icm++)"

      // Get the variable ID for the elemental comm map IDs vector
      std::vector<entity_id> elem_cmap_ids(comm.elementMap.size());
      for (size_t i = 0; i < comm.elementMap.size(); i++) {
        elem_cmap_ids[i] = comm.elementMap[i].id;
      }
      if (put_id_array(exodusFilePtr, VAR_E_COMM_IDS, elem_cmap_ids) != NC_NOERR) {
        return EX_FATAL;
      }
    }
  }
  return EX_NOERR;
}

int Internals::put_non_define_data(const std::vector<Blob> &blobs)
{
  int status;
  int entlst_id;

  size_t name_length = 0;
  for (const auto &blob : blobs) {
    name_length = std::max(name_length, blob.name.length());
    if ((status = nc_inq_varid(exodusFilePtr, VAR_ENTITY_BLOB(blob.id), &entlst_id)) != NC_NOERR) {
      std::string errmsg =
          fmt::format("Error: failed to locate entity list array for blob {} in file id {}",
                      blob.id, exodusFilePtr);
      ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      return EX_FATAL;
    }

    long dummy = 0;
    if ((status = nc_put_var_long(exodusFilePtr, entlst_id, &dummy)) != EX_NOERR) {
      std::string errmsg = fmt::format(
          "Error: failed to output dummy value for blob {} in file id {}", blob.id, exodusFilePtr);
      ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      return EX_FATAL;
    }
  }
  exi_update_max_name_length(exodusFilePtr, name_length);
  return EX_NOERR;
}

int Internals::put_non_define_data(const std::vector<Assembly> &assemblies)
{
  size_t name_length = 0;
  for (const auto &assembly : assemblies) {
    int status  = EX_NOERR;
    name_length = std::max(name_length, assembly.name.length());

    if (!assembly.memberIdList.empty()) {
      int entlst_id = 0;
      if ((status = nc_inq_varid(exodusFilePtr, VAR_ENTITY_ASSEMBLY(assembly.id), &entlst_id)) !=
          EX_NOERR) {
        std::string errmsg =
            fmt::format("Error: failed to locate entity list for assembly {} in file id {}",
                        assembly.id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        return EX_FATAL;
      }
      if ((status = nc_put_var_longlong(exodusFilePtr, entlst_id,
                                        (long long int *)Data(assembly.memberIdList))) !=
          EX_NOERR) {
        std::string errmsg =
            fmt::format("Error: failed to output entity list for assembly {} in file {}",
                        assembly.id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        return EX_FATAL;
      }
    }
  }
  exi_update_max_name_length(exodusFilePtr, name_length);
  return EX_NOERR;
}

int Internals::put_non_define_data(const std::vector<ElemBlock> &blocks, bool output_global_data)
{
  int num_elem_blk = static_cast<int>(blocks.size()); // Verified via SMART_ASSERT earlier...

  if (num_elem_blk > 0) {
    // first get id of element block ids array variable
    std::vector<entity_id> elem_blk_id(num_elem_blk);
    for (int iblk = 0; iblk < num_elem_blk; iblk++) {
      elem_blk_id[iblk] = blocks[iblk].id;
    }

    if (put_id_array(exodusFilePtr, VAR_ID_EL_BLK, elem_blk_id) != NC_NOERR) {
      return EX_FATAL;
    }

    if (output_global_data) {
      if (put_id_array(exodusFilePtr, VAR_ELBLK_IDS_GLOBAL, elem_blk_id) != NC_NOERR) {
        return EX_FATAL;
      }

      std::vector<int64_t> counts(num_elem_blk);
      for (int iblk = 0; iblk < num_elem_blk; iblk++) {
        counts[iblk] = blocks[iblk].globalEntityCount;
      }
      if (put_int_array(exodusFilePtr, VAR_ELBLK_CNT_GLOBAL, counts) != NC_NOERR) {
        return EX_FATAL;
      }
    }

    // Now, write the element block status array
    std::vector<int> elem_blk_status(num_elem_blk);
    for (int iblk = 0; iblk < num_elem_blk; iblk++) {
      elem_blk_status[iblk] = blocks[iblk].entityCount > 0 ? 1 : 0;
    }

    if (put_int_array(exodusFilePtr, VAR_STAT_EL_BLK, elem_blk_status) != NC_NOERR) {
      return EX_FATAL;
    }

    // TODO: Is this code correct?  `text` is never set...
    std::string           text;
    std::array<size_t, 2> start{0, 0};
    std::array<size_t, 2> count{1, text.size() + 1};

    for (int iblk = 0; iblk < num_elem_blk; iblk++) {
      if (blocks[iblk].attributeCount > 0 && blocks[iblk].entityCount > 0) {
        int varid;
        int status = nc_inq_varid(exodusFilePtr, VAR_NAME_ATTRIB(iblk + 1), &varid);
        if (status != NC_NOERR) {
          ex_opts(EX_VERBOSE);
          std::string errmsg = fmt::format(
              "Error: failed to locate variable name attribute in file id {}", exodusFilePtr);
          ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
          return EX_FATAL;
        }

        for (int i = 0; i < blocks[iblk].attributeCount; i++) {
          start[0] = i;
          nc_put_vara_text(exodusFilePtr, varid, Data(start), Data(count), text.c_str());
        }
      }
    }
  }
  return EX_NOERR;
}

int Internals::put_non_define_data(const std::vector<FaceBlock> &blocks)
{
  int num_face_blk = static_cast<int>(blocks.size()); // Verified via SMART_ASSERT earlier...

  if (num_face_blk > 0) {
    // first get id of face block ids array variable
    std::vector<entity_id> face_blk_id(num_face_blk);
    for (int iblk = 0; iblk < num_face_blk; iblk++) {
      face_blk_id[iblk] = blocks[iblk].id;
    }

    if (put_id_array(exodusFilePtr, VAR_ID_FA_BLK, face_blk_id) != NC_NOERR) {
      return EX_FATAL;
    }

    // Now, write the face block status array
    std::vector<int> face_blk_status(num_face_blk);
    for (int iblk = 0; iblk < num_face_blk; iblk++) {
      face_blk_status[iblk] = blocks[iblk].entityCount > 0 ? 1 : 0;
    }

    if (put_int_array(exodusFilePtr, VAR_STAT_FA_BLK, face_blk_status) != NC_NOERR) {
      return EX_FATAL;
    }

    // TODO: Is this code correct?  `text` is never set...
    std::string           text;
    std::array<size_t, 2> start{0, 0};
    std::array<size_t, 2> count{1, text.size() + 1};

    for (int iblk = 0; iblk < num_face_blk; iblk++) {
      if (blocks[iblk].attributeCount > 0 && blocks[iblk].entityCount > 0) {
        int varid;
        int status = nc_inq_varid(exodusFilePtr, VAR_NAME_FATTRIB(iblk + 1), &varid);
        if (status != NC_NOERR) {
          ex_opts(EX_VERBOSE);
          std::string errmsg = fmt::format(
              "Error: failed to locate face variable name attribute in file id {}", exodusFilePtr);
          ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
          return EX_FATAL;
        }

        for (int i = 0; i < blocks[iblk].attributeCount; i++) {
          start[0] = i;
          nc_put_vara_text(exodusFilePtr, varid, Data(start), Data(count), text.c_str());
        }
      }
    }
  }
  return EX_NOERR;
}

int Internals::put_non_define_data(const std::vector<EdgeBlock> &blocks)
{
  int num_edge_blk = static_cast<int>(blocks.size()); // Verified via SMART_ASSERT earlier...

  if (num_edge_blk > 0) {
    // first get id of edge block ids array variable
    std::vector<entity_id> edge_blk_id(num_edge_blk);
    for (int iblk = 0; iblk < num_edge_blk; iblk++) {
      edge_blk_id[iblk] = blocks[iblk].id;
    }

    if (put_id_array(exodusFilePtr, VAR_ID_ED_BLK, edge_blk_id) != NC_NOERR) {
      return EX_FATAL;
    }

    // Now, write the edge block status array
    std::vector<int> edge_blk_status(num_edge_blk);
    for (int iblk = 0; iblk < num_edge_blk; iblk++) {
      edge_blk_status[iblk] = blocks[iblk].entityCount > 0 ? 1 : 0;
    }

    if (put_int_array(exodusFilePtr, VAR_STAT_ED_BLK, edge_blk_status) != NC_NOERR) {
      return EX_FATAL;
    }

    // TODO: Is this code correct?  `text` is never set...
    std::string           text;
    std::array<size_t, 2> start{0, 0};
    std::array<size_t, 2> count{1, text.size() + 1};
    for (int iblk = 0; iblk < num_edge_blk; iblk++) {
      if (blocks[iblk].attributeCount > 0 && blocks[iblk].entityCount > 0) {
        int varid;
        int status = nc_inq_varid(exodusFilePtr, VAR_NAME_EATTRIB(iblk + 1), &varid);
        if (status != NC_NOERR) {
          ex_opts(EX_VERBOSE);
          std::string errmsg =
              fmt::format("Error: failed to locate element variable name attribute in file id {}",
                          exodusFilePtr);
          ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
          return EX_FATAL;
        }

        for (int i = 0; i < blocks[iblk].attributeCount; i++) {
          start[0] = i;
          nc_put_vara_text(exodusFilePtr, varid, Data(start), Data(count), text.c_str());
        }
      }
    }
  }
  return EX_NOERR;
}

// ========================================================================
int Internals::put_metadata(const std::vector<NodeSet> &nodesets, bool count_only)
{
  if (nodesets.empty()) {
    return EX_NOERR;
  }

  int bulk_type = get_type(exodusFilePtr, EX_BULK_INT64_DB);

  // Get number of node sets defined for this file
  int dimid;
  int num_node_sets = 0;
  int status        = nc_inq_dimid(exodusFilePtr, DIM_NUM_NS, &dimid);
  if (status != NC_NOERR) {
    ex_opts(EX_VERBOSE);
    if (status == NC_EBADDIM) {
      std::string errmsg = fmt::format("Error: no node sets defined for file id {}", exodusFilePtr);
      ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
    }
    else {
      std::string errmsg =
          fmt::format("Error: failed to locate node sets defined in file id {}", exodusFilePtr);
      ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
    }
    return EX_FATAL;
  }

  // inquire how many node sets are to be stored
  num_node_sets = ex_inquire_int(exodusFilePtr, EX_INQ_NODE_SETS);

  int namestrdim;
  status = nc_inq_dimid(exodusFilePtr, DIM_STR_NAME, &namestrdim);
  if (status != NC_NOERR) {
    ex_opts(EX_VERBOSE);
    std::string errmsg =
        fmt::format("Error: failed to get string length in file id {}", exodusFilePtr);
    ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
    return EX_FATAL;
  }

  SMART_ASSERT(static_cast<int>(nodesets.size()) == num_node_sets);

  for (int i = 0; i < num_node_sets; i++) {

    //  NOTE: exi_inc_file_item is used to find the number of node sets
    // for a specific file and returns that value incremented.
    int cur_num_node_sets = exi_inc_file_item(exodusFilePtr, exi_get_counter_list(EX_NODE_SET));

    if (nodesets[i].entityCount == 0) {
      continue;
    }

    status = nc_def_dim(exodusFilePtr, DIM_NUM_NOD_NS(cur_num_node_sets + 1),
                        nodesets[i].entityCount, &dimid);
    if (status != NC_NOERR) {
      ex_opts(EX_VERBOSE);
      if (status == NC_ENAMEINUSE) {
        std::string errmsg = fmt::format("Error: node set {} already defined in file id {}",
                                         nodesets[i].id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      }
      else {
        std::string errmsg =
            fmt::format("Error: failed to define number of nodes for set {} in file id {}",
                        nodesets[i].id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      }
      return EX_FATAL;
    }
    if (count_only) {
      continue;
    }

    // define variable to store node set node list here instead of in expns
    std::array dims1{dimid};
    int        varid;
    status = nc_def_var(exodusFilePtr, VAR_NODE_NS(cur_num_node_sets + 1), bulk_type, 1,
                        Data(dims1), &varid);
    if (status != NC_NOERR) {
      ex_opts(EX_VERBOSE);
      if (status == NC_ENAMEINUSE) {
        std::string errmsg =
            fmt::format("Error: node set {} node list already defined in file id {}",
                        nodesets[i].id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      }
      else {
        std::string errmsg =
            fmt::format("Error: failed to create node set {} node list in file id {}",
                        nodesets[i].id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      }
      return EX_FATAL;
    }
    exi_compress_variable(exodusFilePtr, varid, 1);

    // Create variable for distribution factors if required
    if (nodesets[i].dfCount > 0) {
      // num_dist_per_set should equal num_nodes_per_set
      if (nodesets[i].dfCount != nodesets[i].entityCount) {
        status = EX_FATAL;
        ex_opts(EX_VERBOSE);
        std::string errmsg = fmt::format("Error: # dist fact ({}) not equal to # nodes ({}) "
                                         "in node set {} file id {}",
                                         nodesets[i].dfCount, nodesets[i].entityCount,
                                         nodesets[i].id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        return EX_FATAL;
      }
      // create variable for distribution factors
      status = nc_def_var(exodusFilePtr, VAR_FACT_NS(cur_num_node_sets + 1),
                          nc_flt_code(exodusFilePtr), 1, Data(dims1), &varid);
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        if (status == NC_ENAMEINUSE) {
          std::string errmsg =
              fmt::format("Error: node set {} dist factors already exist in file id {}",
                          nodesets[i].id, exodusFilePtr);
          ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        }
        else {
          std::string errmsg =
              fmt::format("Error: failed to create node set {} dist factors in file id {}",
                          nodesets[i].id, exodusFilePtr);
          ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        }
        return EX_FATAL;
      }
      exi_compress_variable(exodusFilePtr, varid, 2);
    }

    if (nodesets[i].attributeCount > 0) {
      int numattrdim;
      status = nc_def_dim(exodusFilePtr, DIM_NUM_ATT_IN_NS(cur_num_node_sets + 1),
                          nodesets[i].attributeCount, &numattrdim);
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        std::string errmsg =
            fmt::format("Error: failed to define number of attributes in nodeset {}"
                        " in file id {}",
                        nodesets[i].id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        return EX_FATAL;
      }

      {
        std::array dims{dimid, numattrdim};
        status = nc_def_var(exodusFilePtr, VAR_NSATTRIB(cur_num_node_sets + 1),
                            nc_flt_code(exodusFilePtr), 2, Data(dims), &varid);
        if (status != NC_NOERR) {
          ex_opts(EX_VERBOSE);
          std::string errmsg =
              fmt::format("Error:  failed to define attributes for element nodeset {}"
                          " in file id {}",
                          nodesets[i].id, exodusFilePtr);
          ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
          return EX_FATAL;
        }
        exi_compress_variable(exodusFilePtr, varid, 2);
      }

      {
        // Attribute name array...
        std::array dims{numattrdim, namestrdim};

        status = nc_def_var(exodusFilePtr, VAR_NAME_NSATTRIB(cur_num_node_sets + 1), NC_CHAR, 2,
                            Data(dims), &varid);
        if (status != NC_NOERR) {
          ex_opts(EX_VERBOSE);
          std::string errmsg =
              fmt::format("Error: failed to define attribute name array for nodeset {}"
                          " in file id {}",
                          nodesets[i].id, exodusFilePtr);
          ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
          return EX_FATAL;
        }
        exi_set_compact_storage(exodusFilePtr, varid);
      }
    }
  }
  return EX_NOERR;
}

// ========================================================================
int Internals::put_metadata(const std::vector<EdgeSet> &edgesets, bool count_only)
{
  if (edgesets.empty()) {
    return EX_NOERR;
  }
  int bulk_type = get_type(exodusFilePtr, EX_BULK_INT64_DB);

  // Get number of edge sets defined for this file
  int dimid;
  int num_edge_sets = 0;
  int status        = nc_inq_dimid(exodusFilePtr, DIM_NUM_ES, &dimid);
  if (status != NC_NOERR) {
    ex_opts(EX_VERBOSE);
    if (status == NC_EBADDIM) {
      std::string errmsg = fmt::format("Error: no edge sets defined for file id {}", exodusFilePtr);
      ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
    }
    else {
      std::string errmsg =
          fmt::format("Error: failed to locate edge sets defined in file id {}", exodusFilePtr);
      ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
    }
    return EX_FATAL;
  }

  // inquire how many edge sets are to be stored
  num_edge_sets = ex_inquire_int(exodusFilePtr, EX_INQ_EDGE_SETS);

  SMART_ASSERT(static_cast<int>(edgesets.size()) == num_edge_sets);

  int namestrdim;
  status = nc_inq_dimid(exodusFilePtr, DIM_STR_NAME, &namestrdim);
  if (status != NC_NOERR) {
    ex_opts(EX_VERBOSE);
    std::string errmsg =
        fmt::format("Error: failed to get string length in file id {}", exodusFilePtr);
    ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
    return EX_FATAL;
  }

  for (int i = 0; i < num_edge_sets; i++) {

    //  NOTE: exi_inc_file_item is used to find the number of edge sets
    // for a specific file and returns that value incremented.
    int cur_num_edge_sets = exi_inc_file_item(exodusFilePtr, exi_get_counter_list(EX_EDGE_SET));

    if (edgesets[i].entityCount == 0) {
      continue;
    }

    status = nc_def_dim(exodusFilePtr, DIM_NUM_EDGE_ES(cur_num_edge_sets + 1),
                        edgesets[i].entityCount, &dimid);
    if (status != NC_NOERR) {
      ex_opts(EX_VERBOSE);
      if (status == NC_ENAMEINUSE) {
        std::string errmsg = fmt::format("Error: edge set {} already defined in file id {}",
                                         edgesets[i].id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      }
      else {
        std::string errmsg =
            fmt::format("Error: failed to define number of edges for set {} in file id {}",
                        edgesets[i].id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      }
      return EX_FATAL;
    }
    if (count_only) {
      continue;
    }

    // define variable to store edge set edge list here instead of in expns
    std::array dims1{dimid};
    int        varid;
    status = nc_def_var(exodusFilePtr, VAR_EDGE_ES(cur_num_edge_sets + 1), bulk_type, 1,
                        Data(dims1), &varid);
    if (status != NC_NOERR) {
      ex_opts(EX_VERBOSE);
      if (status == NC_ENAMEINUSE) {
        std::string errmsg =
            fmt::format("Error: edge set {} edge list already defined in file id {}",
                        edgesets[i].id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      }
      else {
        std::string errmsg =
            fmt::format("Error: failed to create edge set {} edge list in file id {}",
                        edgesets[i].id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      }
      return EX_FATAL;
    }
    exi_compress_variable(exodusFilePtr, varid, 1);

    // Orientation variable
    status = nc_def_var(exodusFilePtr, VAR_ORNT_ES(cur_num_edge_sets + 1), bulk_type, 1,
                        Data(dims1), &varid);
    if (status != NC_NOERR) {
      ex_opts(EX_VERBOSE);
      if (status == NC_ENAMEINUSE) {
        std::string errmsg =
            fmt::format("Error: extra list already exists for edge set {} in file id {}",
                        edgesets[i].id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      }
      else {
        std::string errmsg =
            fmt::format("Error: failed to create extra list for edge set {} in file id {}",
                        edgesets[i].id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      }
      return EX_FATAL;
    }
    exi_compress_variable(exodusFilePtr, varid, 1);

    // Create variable for distribution factors if required
    if (edgesets[i].dfCount > 0) {
      // num_dist_per_set should equal num_edges_per_set
      if (edgesets[i].dfCount != edgesets[i].entityCount) {
        status = EX_FATAL;
        ex_opts(EX_VERBOSE);
        std::string errmsg = fmt::format("Error: # dist fact ({}) not equal to # edges ({}) "
                                         "in edge set {} file id {}",
                                         edgesets[i].dfCount, edgesets[i].entityCount,
                                         edgesets[i].id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        return EX_FATAL;
      }
      // create variable for distribution factors
      status = nc_def_var(exodusFilePtr, VAR_FACT_ES(cur_num_edge_sets + 1),
                          nc_flt_code(exodusFilePtr), 1, Data(dims1), &varid);
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        if (status == NC_ENAMEINUSE) {
          std::string errmsg =
              fmt::format("Error: edge set {} dist factors already exist in file id {}",
                          edgesets[i].id, exodusFilePtr);
          ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        }
        else {
          std::string errmsg =
              fmt::format("Error: failed to create edge set {} dist factors in file id {}",
                          edgesets[i].id, exodusFilePtr);
          ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        }
        return EX_FATAL;
      }
      exi_compress_variable(exodusFilePtr, varid, 2);
    }
    if (edgesets[i].attributeCount > 0) {
      int numattrdim;
      status = nc_def_dim(exodusFilePtr, DIM_NUM_ATT_IN_ES(cur_num_edge_sets + 1),
                          edgesets[i].attributeCount, &numattrdim);
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        std::string errmsg =
            fmt::format("Error: failed to define number of attributes in edgeset {}"
                        " in file id {}",
                        edgesets[i].id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        return EX_FATAL;
      }

      {
        std::array dims{dimid, numattrdim};
        status = nc_def_var(exodusFilePtr, VAR_ESATTRIB(cur_num_edge_sets + 1),
                            nc_flt_code(exodusFilePtr), 2, Data(dims), &varid);
        if (status != NC_NOERR) {
          ex_opts(EX_VERBOSE);
          std::string errmsg =
              fmt::format("Error:  failed to define attributes for element edgeset {}"
                          " in file id {}",
                          edgesets[i].id, exodusFilePtr);
          ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
          return EX_FATAL;
        }
        exi_compress_variable(exodusFilePtr, varid, 2);
      }

      {
        // Attribute name array...
        std::array dims{numattrdim, namestrdim};

        status = nc_def_var(exodusFilePtr, VAR_NAME_ESATTRIB(cur_num_edge_sets + 1), NC_CHAR, 2,
                            Data(dims), &varid);
        if (status != NC_NOERR) {
          ex_opts(EX_VERBOSE);
          std::string errmsg =
              fmt::format("Error: failed to define attribute name array for edgeset {}"
                          " in file id {}",
                          edgesets[i].id, exodusFilePtr);
          ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
          return EX_FATAL;
        }
        exi_set_compact_storage(exodusFilePtr, varid);
      }
    }
  }
  return EX_NOERR;
}

// ========================================================================
int Internals::put_metadata(const std::vector<FaceSet> &facesets, bool count_only)
{
  if (facesets.empty()) {
    return EX_NOERR;
  }
  int bulk_type = get_type(exodusFilePtr, EX_BULK_INT64_DB);

  // Get number of face sets defined for this file
  int dimid;
  int num_face_sets = 0;
  int status        = nc_inq_dimid(exodusFilePtr, DIM_NUM_FS, &dimid);
  if (status != NC_NOERR) {
    ex_opts(EX_VERBOSE);
    if (status == NC_EBADDIM) {
      std::string errmsg = fmt::format("Error: no face sets defined for file id {}", exodusFilePtr);
      ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
    }
    else {
      std::string errmsg =
          fmt::format("Error: failed to locate face sets defined in file id {}", exodusFilePtr);
      ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
    }
    return EX_FATAL;
  }

  // inquire how many face sets are to be stored
  num_face_sets = ex_inquire_int(exodusFilePtr, EX_INQ_FACE_SETS);

  SMART_ASSERT(static_cast<int>(facesets.size()) == num_face_sets);

  int namestrdim;
  status = nc_inq_dimid(exodusFilePtr, DIM_STR_NAME, &namestrdim);
  if (status != NC_NOERR) {
    ex_opts(EX_VERBOSE);
    std::string errmsg =
        fmt::format("Error: failed to get string length in file id {}", exodusFilePtr);
    ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
    return EX_FATAL;
  }

  for (int i = 0; i < num_face_sets; i++) {

    //  NOTE: exi_inc_file_item is used to find the number of face sets
    // for a specific file and returns that value incremented.
    int cur_num_face_sets = exi_inc_file_item(exodusFilePtr, exi_get_counter_list(EX_FACE_SET));

    if (facesets[i].entityCount == 0) {
      continue;
    }

    status = nc_def_dim(exodusFilePtr, DIM_NUM_FACE_FS(cur_num_face_sets + 1),
                        facesets[i].entityCount, &dimid);
    if (status != NC_NOERR) {
      ex_opts(EX_VERBOSE);
      if (status == NC_ENAMEINUSE) {
        std::string errmsg = fmt::format("Error: face set {} already defined in file id {}",
                                         facesets[i].id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      }
      else {
        std::string errmsg =
            fmt::format("Error: failed to define number of faces for set {} in file id {}",
                        facesets[i].id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      }
      return EX_FATAL;
    }
    if (count_only) {
      continue;
    }

    // define variable to store face set face list here instead of in expns
    std::array dims1{dimid};
    int        varid;
    status = nc_def_var(exodusFilePtr, VAR_FACE_FS(cur_num_face_sets + 1), bulk_type, 1,
                        Data(dims1), &varid);
    if (status != NC_NOERR) {
      ex_opts(EX_VERBOSE);
      if (status == NC_ENAMEINUSE) {
        std::string errmsg =
            fmt::format("Error: face set {} face list already defined in file id {}",
                        facesets[i].id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      }
      else {
        std::string errmsg =
            fmt::format("Error: failed to create face set {} face list in file id {}",
                        facesets[i].id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      }
      return EX_FATAL;
    }
    exi_compress_variable(exodusFilePtr, varid, 1);

    // Orientation variable
    status = nc_def_var(exodusFilePtr, VAR_ORNT_FS(cur_num_face_sets + 1), bulk_type, 1,
                        Data(dims1), &varid);
    if (status != NC_NOERR) {
      ex_opts(EX_VERBOSE);
      if (status == NC_ENAMEINUSE) {
        std::string errmsg =
            fmt::format("Error: extra list already exists for face set {} in file id {}",
                        facesets[i].id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      }
      else {
        std::string errmsg =
            fmt::format("Error: failed to create extra list for face set {} in file id {}",
                        facesets[i].id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      }
      return EX_FATAL;
    }
    exi_compress_variable(exodusFilePtr, varid, 1);

    // Create variable for distribution factors if required
    if (facesets[i].dfCount > 0) {
      // num_dist_per_set should equal num_faces_per_set
      if (facesets[i].dfCount != facesets[i].entityCount) {
        status = EX_FATAL;
        ex_opts(EX_VERBOSE);
        std::string errmsg = fmt::format("Error: # dist fact ({}) not equal to # faces ({}) "
                                         "in face set {} file id {}",
                                         facesets[i].dfCount, facesets[i].entityCount,
                                         facesets[i].id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        return EX_FATAL;
      }
      // create variable for distribution factors
      status = nc_def_var(exodusFilePtr, VAR_FACT_FS(cur_num_face_sets + 1),
                          nc_flt_code(exodusFilePtr), 1, Data(dims1), &varid);
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        if (status == NC_ENAMEINUSE) {
          std::string errmsg =
              fmt::format("Error: face set {} dist factors already exist in file id {}",
                          facesets[i].id, exodusFilePtr);
          ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        }
        else {
          std::string errmsg =
              fmt::format("Error: failed to create face set {} dist factors in file id {}",
                          facesets[i].id, exodusFilePtr);
          ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        }
        return EX_FATAL;
      }
      exi_compress_variable(exodusFilePtr, varid, 2);
    }
    if (facesets[i].attributeCount > 0) {
      int numattrdim;
      status = nc_def_dim(exodusFilePtr, DIM_NUM_ATT_IN_FS(cur_num_face_sets + 1),
                          facesets[i].attributeCount, &numattrdim);
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        std::string errmsg =
            fmt::format("Error: failed to define number of attributes in faceset {}"
                        " in file id {}",
                        facesets[i].id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        return EX_FATAL;
      }

      {
        std::array dims{dimid, numattrdim};
        status = nc_def_var(exodusFilePtr, VAR_FSATTRIB(cur_num_face_sets + 1),
                            nc_flt_code(exodusFilePtr), 2, Data(dims), &varid);
        if (status != NC_NOERR) {
          ex_opts(EX_VERBOSE);
          std::string errmsg =
              fmt::format("Error:  failed to define attributes for element faceset {}"
                          " in file id {}",
                          facesets[i].id, exodusFilePtr);
          ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
          return EX_FATAL;
        }
        exi_compress_variable(exodusFilePtr, varid, 2);
      }

      {
        // Attribute name array...
        std::array dims{numattrdim, namestrdim};

        status = nc_def_var(exodusFilePtr, VAR_NAME_FSATTRIB(cur_num_face_sets + 1), NC_CHAR, 2,
                            Data(dims), &varid);
        if (status != NC_NOERR) {
          ex_opts(EX_VERBOSE);
          std::string errmsg =
              fmt::format("Error: failed to define attribute name array for faceset {}"
                          " in file id {}",
                          facesets[i].id, exodusFilePtr);
          ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
          return EX_FATAL;
        }
        exi_set_compact_storage(exodusFilePtr, varid);
      }
    }
  }
  return EX_NOERR;
}

// ========================================================================
int Internals::put_metadata(const std::vector<ElemSet> &elemsets, bool count_only)
{
  if (elemsets.empty()) {
    return EX_NOERR;
  }
  // Get number of element sets defined for this file
  int dimid;
  int num_elem_sets = 0;
  int status        = nc_inq_dimid(exodusFilePtr, DIM_NUM_ELS, &dimid);
  if (status != NC_NOERR) {
    ex_opts(EX_VERBOSE);
    if (status == NC_EBADDIM) {
      std::string errmsg =
          fmt::format("Error: no element sets defined for file id {}", exodusFilePtr);
      ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
    }
    else {
      std::string errmsg =
          fmt::format("Error: failed to locate element sets defined in file id {}", exodusFilePtr);
      ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
    }
    return EX_FATAL;
  }

  // inquire how many element sets are to be stored
  num_elem_sets = ex_inquire_int(exodusFilePtr, EX_INQ_ELEM_SETS);

  SMART_ASSERT(static_cast<int>(elemsets.size()) == num_elem_sets);

  int namestrdim;
  status = nc_inq_dimid(exodusFilePtr, DIM_STR_NAME, &namestrdim);
  if (status != NC_NOERR) {
    ex_opts(EX_VERBOSE);
    std::string errmsg =
        fmt::format("Error: failed to get string length in file id {}", exodusFilePtr);
    ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
    return EX_FATAL;
  }

  for (int i = 0; i < num_elem_sets; i++) {

    //  NOTE: exi_inc_file_item is used to find the number of elem sets
    // for a specific file and returns that value incremented.
    int cur_num_elem_sets = exi_inc_file_item(exodusFilePtr, exi_get_counter_list(EX_ELEM_SET));

    if (elemsets[i].entityCount == 0) {
      continue;
    }

    status = nc_def_dim(exodusFilePtr, DIM_NUM_ELE_ELS(cur_num_elem_sets + 1),
                        elemsets[i].entityCount, &dimid);
    if (status != NC_NOERR) {
      ex_opts(EX_VERBOSE);
      if (status == NC_ENAMEINUSE) {
        std::string errmsg = fmt::format("Error: elem set {} already defined in file id {}",
                                         elemsets[i].id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      }
      else {
        std::string errmsg =
            fmt::format("Error: failed to define number of elems for set {} in file id {}",
                        elemsets[i].id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      }
      return EX_FATAL;
    }
    if (count_only) {
      continue;
    }

    // define variable to store element set element list here instead of in expns
    std::array dims1{dimid};
    int        varid;
    status = nc_def_var(exodusFilePtr, VAR_ELEM_ELS(cur_num_elem_sets + 1),
                        get_type(exodusFilePtr, EX_BULK_INT64_DB), 1, Data(dims1), &varid);

    if (status != NC_NOERR) {
      ex_opts(EX_VERBOSE);
      if (status == NC_ENAMEINUSE) {
        std::string errmsg =
            fmt::format("Error: element set {} element list already defined in file id {}",
                        elemsets[i].id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      }
      else {
        std::string errmsg =
            fmt::format("Error: failed to create element set {} element list in file id {}",
                        elemsets[i].id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      }
      return EX_FATAL;
    }
    exi_compress_variable(exodusFilePtr, varid, 1);

    // Create variable for distribution factors if required
    if (elemsets[i].dfCount > 0) {
      // num_dist_per_set should equal num_elems_per_set
      if (elemsets[i].dfCount != elemsets[i].entityCount) {
        status = EX_FATAL;
        ex_opts(EX_VERBOSE);
        std::string errmsg = fmt::format("Error: # dist fact ({}) not equal to # elements ({}) "
                                         "in element set {} file id {}",
                                         elemsets[i].dfCount, elemsets[i].entityCount,
                                         elemsets[i].id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        return EX_FATAL;
      }
      // create variable for distribution factors
      status = nc_def_var(exodusFilePtr, VAR_FACT_ELS(cur_num_elem_sets + 1),
                          nc_flt_code(exodusFilePtr), 1, Data(dims1), &varid);
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        if (status == NC_ENAMEINUSE) {
          std::string errmsg =
              fmt::format("Error: element set {} dist factors already exist in file id {}",
                          elemsets[i].id, exodusFilePtr);
          ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        }
        else {
          std::string errmsg =
              fmt::format("Error: failed to create element set {} dist factors in file id {}",
                          elemsets[i].id, exodusFilePtr);
          ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        }
        return EX_FATAL;
      }
      exi_compress_variable(exodusFilePtr, varid, 2);
    }
    if (elemsets[i].attributeCount > 0) {
      int numattrdim;
      status = nc_def_dim(exodusFilePtr, DIM_NUM_ATT_IN_ES(cur_num_elem_sets + 1),
                          elemsets[i].attributeCount, &numattrdim);
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        std::string errmsg =
            fmt::format("Error: failed to define number of attributes in elemset {}"
                        " in file id {}",
                        elemsets[i].id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        return EX_FATAL;
      }

      {
        std::array dims{dimid, numattrdim};
        status = nc_def_var(exodusFilePtr, VAR_ESATTRIB(cur_num_elem_sets + 1),
                            nc_flt_code(exodusFilePtr), 2, Data(dims), &varid);
        if (status != NC_NOERR) {
          ex_opts(EX_VERBOSE);
          std::string errmsg =
              fmt::format("Error:  failed to define attributes for element elemset {}"
                          " in file id {}",
                          elemsets[i].id, exodusFilePtr);
          ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
          return EX_FATAL;
        }
        exi_compress_variable(exodusFilePtr, varid, 2);
      }

      {
        // Attribute name array...
        std::array dims{numattrdim, namestrdim};

        status = nc_def_var(exodusFilePtr, VAR_NAME_ESATTRIB(cur_num_elem_sets + 1), NC_CHAR, 2,
                            Data(dims), &varid);
        if (status != NC_NOERR) {
          ex_opts(EX_VERBOSE);
          std::string errmsg =
              fmt::format("Error: failed to define attribute name array for elemset {}"
                          " in file id {}",
                          elemsets[i].id, exodusFilePtr);
          ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
          return EX_FATAL;
        }
        exi_set_compact_storage(exodusFilePtr, varid);
      }
    }
  }
  return EX_NOERR;
}

int Internals::put_non_define_data(const std::vector<NodeSet> &nodesets, bool output_global_data)
{
  if (nodesets.empty()) {
    return EX_NOERR;
  }

  // Output nodeset ids...
  size_t                 num_nodesets = nodesets.size();
  std::vector<entity_id> nodeset_id(num_nodesets);
  for (size_t i = 0; i < num_nodesets; i++) {
    nodeset_id[i] = nodesets[i].id;
  }

  if (put_id_array(exodusFilePtr, VAR_NS_IDS, nodeset_id) != NC_NOERR) {
    return EX_FATAL;
  }

  if (output_global_data) {
    if (put_id_array(exodusFilePtr, VAR_NS_IDS_GLOBAL, nodeset_id) != NC_NOERR) {
      return EX_FATAL;
    }

    std::vector<int64_t> counts(num_nodesets);
    for (size_t iset = 0; iset < num_nodesets; iset++) {
      counts[iset] = nodesets[iset].globalEntityCount;
    }
    if (put_int_array(exodusFilePtr, VAR_NS_NODE_CNT_GLOBAL, counts) != NC_NOERR) {
      return EX_FATAL;
    }
  }

  // Now, write the status array
  std::vector<int> status(num_nodesets);
  for (size_t i = 0; i < num_nodesets; i++) {
    status[i] = nodesets[i].entityCount > 0 ? 1 : 0;
  }

  if (put_int_array(exodusFilePtr, VAR_NS_STAT, status) != NC_NOERR) {
    return EX_FATAL;
  }

  return EX_NOERR;
}

int Internals::put_non_define_data(const std::vector<EdgeSet> &edgesets)
{
  if (edgesets.empty()) {
    return EX_NOERR;
  }

  // Output edgeset ids...
  size_t                 num_edgesets = edgesets.size();
  std::vector<entity_id> edgeset_id(num_edgesets);
  for (size_t i = 0; i < num_edgesets; i++) {
    edgeset_id[i] = edgesets[i].id;
  }

  if (put_id_array(exodusFilePtr, VAR_ES_IDS, edgeset_id) != NC_NOERR) {
    return EX_FATAL;
  }

  // Now, write the status array
  std::vector<int> status(num_edgesets);
  for (size_t i = 0; i < num_edgesets; i++) {
    status[i] = edgesets[i].entityCount > 0 ? 1 : 0;
  }

  if (put_int_array(exodusFilePtr, VAR_ES_STAT, status) != NC_NOERR) {
    return EX_FATAL;
  }

  return EX_NOERR;
}

int Internals::put_non_define_data(const std::vector<FaceSet> &facesets)
{
  if (facesets.empty()) {
    return EX_NOERR;
  }

  // Output faceset ids...
  size_t                 num_facesets = facesets.size();
  std::vector<entity_id> faceset_id(num_facesets);
  for (size_t i = 0; i < num_facesets; i++) {
    faceset_id[i] = facesets[i].id;
  }

  if (put_id_array(exodusFilePtr, VAR_FS_IDS, faceset_id) != NC_NOERR) {
    return EX_FATAL;
  }

  // Now, write the status array
  std::vector<int> status(num_facesets);
  for (size_t i = 0; i < num_facesets; i++) {
    status[i] = facesets[i].entityCount > 0 ? 1 : 0;
  }

  if (put_int_array(exodusFilePtr, VAR_FS_STAT, status) != NC_NOERR) {
    return EX_FATAL;
  }

  return EX_NOERR;
}

int Internals::put_non_define_data(const std::vector<ElemSet> &elemsets)
{
  if (elemsets.empty()) {
    return EX_NOERR;
  }

  // Output elemset ids...
  size_t                 num_elemsets = elemsets.size();
  std::vector<entity_id> elemset_id(num_elemsets);
  for (size_t i = 0; i < num_elemsets; i++) {
    elemset_id[i] = elemsets[i].id;
  }

  if (put_id_array(exodusFilePtr, VAR_ELS_IDS, elemset_id) != NC_NOERR) {
    return EX_FATAL;
  }

  // Now, write the status array
  std::vector<int> status(num_elemsets);
  for (size_t i = 0; i < num_elemsets; i++) {
    status[i] = elemsets[i].entityCount > 0 ? 1 : 0;
  }

  if (put_int_array(exodusFilePtr, VAR_ELS_STAT, status) != NC_NOERR) {
    return EX_FATAL;
  }

  return EX_NOERR;
}

// ========================================================================
int Internals::put_metadata(const std::vector<SideSet> &sidesets, bool count_only)
{
  if (sidesets.empty()) {
    return EX_NOERR;
  }
  int bulk_type = get_type(exodusFilePtr, EX_BULK_INT64_DB);

  // Get number of side sets defined for this file
  int dimid;
  int num_side_sets = 0;
  int status        = nc_inq_dimid(exodusFilePtr, DIM_NUM_SS, &dimid);
  if (status != NC_NOERR) {
    ex_opts(EX_VERBOSE);
    if (status == NC_EBADDIM) {
      std::string errmsg = fmt::format("Error: no side sets defined for file id {}", exodusFilePtr);
      ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
    }
    else {
      std::string errmsg =
          fmt::format("Error: failed to locate side sets defined in file id {}", exodusFilePtr);
      ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
    }
    return EX_FATAL;
  }

  // inquire how many side sets are to be stored
  num_side_sets = ex_inquire_int(exodusFilePtr, EX_INQ_SIDE_SETS);

  SMART_ASSERT(static_cast<int>(sidesets.size()) == num_side_sets);

  for (int i = 0; i < num_side_sets; i++) {

    //  NOTE: exi_inc_file_item is used to find the number of side sets
    // for a specific file and returns that value incremented.
    int cur_num_side_sets = exi_inc_file_item(exodusFilePtr, exi_get_counter_list(EX_SIDE_SET));

    if (sidesets[i].entityCount == 0) {
      continue;
    }

    status = nc_def_dim(exodusFilePtr, DIM_NUM_SIDE_SS(cur_num_side_sets + 1),
                        sidesets[i].entityCount, &dimid);
    if (status != NC_NOERR) {
      ex_opts(EX_VERBOSE);
      if (status == NC_ENAMEINUSE) {
        std::string errmsg = fmt::format("Error: side set {} already defined in file id {}",
                                         sidesets[i].id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      }
      else {
        std::string errmsg =
            fmt::format("Error: failed to define number of sides for set {} in file id {}",
                        sidesets[i].id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      }
      return EX_FATAL;
    }
    if (count_only) {
      continue;
    }

    std::array dims{dimid};
    int        varid = 0;
    status = nc_def_var(exodusFilePtr, VAR_ELEM_SS(cur_num_side_sets + 1), bulk_type, 1, Data(dims),
                        &varid);
    if (status != NC_NOERR) {
      ex_opts(EX_VERBOSE);
      if (status == NC_ENAMEINUSE) {
        std::string errmsg =
            fmt::format("Error: side set {} element list already defined in file id {}",
                        sidesets[i].id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      }
      else {
        std::string errmsg =
            fmt::format("Error: failed to create side set {} element list in file id {}",
                        sidesets[i].id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      }
      return EX_FATAL;
    }
    exi_compress_variable(exodusFilePtr, varid, 1);

    // create side list variable for side set
    status = nc_def_var(exodusFilePtr, VAR_SIDE_SS(cur_num_side_sets + 1), bulk_type, 1, Data(dims),
                        &varid);
    if (status != NC_NOERR) {
      ex_opts(EX_VERBOSE);
      if (status == NC_ENAMEINUSE) {
        std::string errmsg =
            fmt::format("Error: side list already exists for side set {} in file id {}",
                        sidesets[i].id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      }
      else {
        std::string errmsg =
            fmt::format("Error: failed to create side list for side set {} in file id {}",
                        sidesets[i].id, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      }
      return EX_FATAL;
    }
    exi_compress_variable(exodusFilePtr, varid, 1);

    // Create variable for distribution factors if required
    if (sidesets[i].dfCount > 0) {
      status = nc_def_dim(exodusFilePtr, DIM_NUM_DF_SS(cur_num_side_sets + 1), sidesets[i].dfCount,
                          &dimid);
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        if (status == NC_ENAMEINUSE) {
          std::string errmsg =
              fmt::format("Error: side set df count {} already defined in file id {}",
                          sidesets[i].id, exodusFilePtr);
          ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        }
        else {
          std::string errmsg =
              fmt::format("Error: failed to define side set df count for set {} in file id {}",
                          sidesets[i].id, exodusFilePtr);
          ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        }
        return EX_FATAL;
      }

      // create distribution factor list variable for side set
      dims[0] = dimid;
      status  = nc_def_var(exodusFilePtr, VAR_FACT_SS(cur_num_side_sets + 1),
                           nc_flt_code(exodusFilePtr), 1, Data(dims), &varid);
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        if (status == NC_ENAMEINUSE) {
          std::string errmsg = fmt::format("Error: dist factor list already exists for side set {}"
                                           " in file id {}",
                                           sidesets[i].id, exodusFilePtr);
          ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        }
        else {
          std::string errmsg =
              fmt::format("Error: failed to create dist factor list for side set {}"
                          " in file id {}",
                          sidesets[i].id, exodusFilePtr);
          ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        }
        return EX_FATAL;
      }
    }
    exi_compress_variable(exodusFilePtr, varid, 2);
  }
  return EX_NOERR;
}

int Internals::put_non_define_data(const std::vector<SideSet> &sidesets, bool output_global_data)
{
  if (sidesets.empty()) {
    return EX_NOERR;
  }

  // Output sideset ids...
  int                    num_sidesets = static_cast<int>(sidesets.size());
  std::vector<entity_id> sideset_id(num_sidesets);
  for (int i = 0; i < num_sidesets; i++) {
    sideset_id[i] = sidesets[i].id;
  }

  if (put_id_array(exodusFilePtr, VAR_SS_IDS, sideset_id) != NC_NOERR) {
    return EX_FATAL;
  }

  if (output_global_data) {
    if (put_id_array(exodusFilePtr, VAR_SS_IDS_GLOBAL, sideset_id) != NC_NOERR) {
      return EX_FATAL;
    }

    std::vector<int64_t> counts(num_sidesets);
    for (int iset = 0; iset < num_sidesets; iset++) {
      counts[iset] = sidesets[iset].globalEntityCount;
    }
    if (put_int_array(exodusFilePtr, VAR_SS_SIDE_CNT_GLOBAL, counts) != NC_NOERR) {
      return EX_FATAL;
    }
  }

  // Now, write the status array
  std::vector<int> status(num_sidesets);
  for (int i = 0; i < num_sidesets; i++) {
    status[i] = sidesets[i].entityCount > 0 ? 1 : 0;
  }

  if (put_int_array(exodusFilePtr, VAR_SS_STAT, status) != NC_NOERR) {
    return EX_FATAL;
  }

  return EX_NOERR;
}

namespace {
  template <typename T> int get_max_name_length(const std::vector<T> &entities, int old_max)
  {
    for (const auto &entity : entities) {
      old_max = std::max(old_max, static_cast<int>(entity.name.size()));
    }
    return (old_max);
  }

  template <typename T>
  int output_names(const std::vector<T> &entities, int exoid, ex_entity_type ent_type)
  {
    if (!entities.empty()) {
      std::vector<char *> names;

      names.resize(entities.size());
      for (size_t i = 0; i < entities.size(); i++) {
        names[i] = (char *)entities[i].name.c_str();
      }
      return (ex_put_names(exoid, ent_type, Data(names)));
    }
    return EX_NOERR;
  }

  int conditional_define_variable(int exodusFilePtr, const char *var, int dimid, int *varid,
                                  nc_type type)
  {
    int status = nc_inq_varid(exodusFilePtr, var, varid);
    if (status != NC_NOERR) {
      status = nc_def_var(exodusFilePtr, var, type, 1, &dimid, varid);
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        std::string errmsg = fmt::format("Error: Failed to define variable \"{}\" in file ID {}",
                                         var, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        return EX_FATAL;
      }
    }
    exi_compress_variable(exodusFilePtr, *varid, 1);
    return EX_NOERR;
  }

  int define_variable(int exodusFilePtr, int64_t size, const char *dim, const char *var,
                      nc_type type)
  {

    if (size > 0) {
      std::array<int, 1> dimid;
      int                status = nc_def_dim(exodusFilePtr, dim, size, Data(dimid));
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        std::string errmsg = fmt::format("Error: failed to dimension \"{}\" in file id {}",
                                         DIM_NUM_BOR_ELEMS, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        return EX_FATAL;
      }

      int varid;
      status = nc_def_var(exodusFilePtr, var, type, 1, Data(dimid), &varid);
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        std::string errmsg = fmt::format("Error: failed to define variable \"{}\" in file ID {}",
                                         VAR_ELEM_MAP_BOR, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        return EX_FATAL;
      }
      exi_compress_variable(exodusFilePtr, varid, 1);
    }
    return EX_NOERR;
  }

  int define_variables(int exodusFilePtr, int64_t size, const char *dim, const char *var[],
                       const nc_type *types)
  {
    if (size > 0) {
      std::array<int, 1> dimid;
      int                status = nc_def_dim(exodusFilePtr, dim, size, Data(dimid));
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        std::string errmsg = fmt::format("Error: failed to dimension \"{}\" in file id {}",
                                         DIM_NUM_BOR_ELEMS, exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        return EX_FATAL;
      }

      int i = 0;
      while (var[i] != nullptr) {
        int varid;
        status = nc_def_var(exodusFilePtr, var[i], types[i], 1, Data(dimid), &varid);
        if (status != NC_NOERR) {
          ex_opts(EX_VERBOSE);
          std::string errmsg = fmt::format("Error: failed to define variable \"{}\" in file ID {}",
                                           var[i], exodusFilePtr);
          ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
          return EX_FATAL;
        }
        exi_set_compact_storage(exodusFilePtr, varid);
        i++;
      }
    }
    return EX_NOERR;
  }

  int put_int_array(int exoid, const char *var_type, const std::vector<int> &array)
  {
    int var_id;
    int status = nc_inq_varid(exoid, var_type, &var_id);
    if (status != NC_NOERR) {
      ex_opts(EX_VERBOSE);
      std::string errmsg = fmt::format("Error: failed to locate {} in file id {}", var_type, exoid);
      ex_err_fn(exoid, __func__, errmsg.c_str(), status);
      return EX_FATAL;
    }

    status = nc_put_var_int(exoid, var_id, Data(array));
    if (status != NC_NOERR) {
      ex_opts(EX_VERBOSE);
      std::string errmsg =
          fmt::format("Error: failed to write {} array in file id {}", var_type, exoid);
      ex_err_fn(exoid, __func__, errmsg.c_str(), status);
      return EX_FATAL;
    }
    return EX_NOERR;
  }

  int put_int_array(int exoid, const char *var_type, const std::vector<int64_t> &array)
  {
    int var_id;
    int status = nc_inq_varid(exoid, var_type, &var_id);
    if (status != NC_NOERR) {
      ex_opts(EX_VERBOSE);
      std::string errmsg = fmt::format("Error: failed to locate {} in file id {}", var_type, exoid);
      ex_err_fn(exoid, __func__, errmsg.c_str(), status);
      return EX_FATAL;
    }

    status = nc_put_var_longlong(exoid, var_id, (long long *)Data(array));
    if (status != NC_NOERR) {
      ex_opts(EX_VERBOSE);
      std::string errmsg =
          fmt::format("Error: failed to write {} array in file id {}", var_type, exoid);
      ex_err_fn(exoid, __func__, errmsg.c_str(), status);
      return EX_FATAL;
    }
    return EX_NOERR;
  }

  int put_id_array(int exoid, const char *var_type, const std::vector<entity_id> &ids)
  {
    int var_id;
    int status = nc_inq_varid(exoid, var_type, &var_id);
    if (status != NC_NOERR) {
      ex_opts(EX_VERBOSE);
      std::string errmsg = fmt::format("Error: failed to locate {} in file id {}", var_type, exoid);
      ex_err_fn(exoid, __func__, errmsg.c_str(), status);
      return EX_FATAL;
    }

    int id_type = get_type(exoid, EX_IDS_INT64_API);

    if (id_type == NC_INT64) {
      status = nc_put_var_longlong(exoid, var_id, (long long int *)Data(ids));
    }
    else {
      // Have entity_id (long long), need ints...
      std::vector<int> int_ids(ids.size());
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4244)
#endif
      int_ids.assign(ids.begin(), ids.end());
#ifdef _MSC_VER
#pragma warning(pop)
#endif
      status = nc_put_var_int(exoid, var_id, Data(int_ids));
    }

    if (status != NC_NOERR) {
      ex_opts(EX_VERBOSE);
      std::string errmsg =
          fmt::format("Error: failed to write {} array in file id {}", var_type, exoid);
      ex_err_fn(exoid, __func__, errmsg.c_str(), status);
      return EX_FATAL;
    }
    return EX_NOERR;
  }

  int define_coordinate_vars(int exodusFilePtr, int64_t nodes, int node_dim, int dimension,
                             int dim_dim, int str_dim)
  {
    int varid;

    if (nodes > 0) {
      // node coordinate arrays -- separate storage...
      std::array dim{node_dim};
      if (dimension > 0) {
        int status = nc_def_var(exodusFilePtr, VAR_COORD_X, nc_flt_code(exodusFilePtr), 1,
                                Data(dim), &varid);
        if (status != NC_NOERR) {
          ex_opts(EX_VERBOSE);
          std::string errmsg = fmt::format(
              "Error: failed to define node x coordinate array in file id {}", exodusFilePtr);
          ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
          return EX_FATAL;
        }
        exi_compress_variable(exodusFilePtr, varid, 2);
      }

      if (dimension > 1) {
        int status = nc_def_var(exodusFilePtr, VAR_COORD_Y, nc_flt_code(exodusFilePtr), 1,
                                Data(dim), &varid);
        if (status != NC_NOERR) {
          ex_opts(EX_VERBOSE);
          std::string errmsg = fmt::format(
              "Error: failed to define node y coordinate array in file id {}", exodusFilePtr);
          ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
          return EX_FATAL;
        }
        exi_compress_variable(exodusFilePtr, varid, 2);
      }

      if (dimension > 2) {
        int status = nc_def_var(exodusFilePtr, VAR_COORD_Z, nc_flt_code(exodusFilePtr), 1,
                                Data(dim), &varid);
        if (status != NC_NOERR) {
          ex_opts(EX_VERBOSE);
          std::string errmsg = fmt::format(
              "Error: failed to define node z coordinate array in file id {}", exodusFilePtr);
          ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
          return EX_FATAL;
        }
        exi_compress_variable(exodusFilePtr, varid, 2);
      }
    }

    // coordinate names array
    std::array dim{dim_dim, str_dim};

    int status = nc_def_var(exodusFilePtr, VAR_NAME_COOR, NC_CHAR, 2, Data(dim), &varid);
    if (status != NC_NOERR) {
      ex_opts(EX_VERBOSE);
      std::string errmsg =
          fmt::format("Error: failed to define coordinate name array in file id {}", exodusFilePtr);
      ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      return EX_FATAL;
    }
    exi_set_compact_storage(exodusFilePtr, varid);
    return EX_NOERR;
  }

  int define_netcdf_vars(int exoid, const char *type, size_t count, const char *dim_num,
                         const char *stat_var, const char *id_var, const char *name_var)
  {
    if (count == 0) {
      return EX_NOERR;
    }

    size_t sixty_four_kb = 64 * 1024;
    int    dimid         = 0;
    int    varid         = 0;
    int    namestrdim    = 0;

    int status = nc_inq_dimid(exoid, DIM_STR_NAME, &namestrdim);
    if (status != NC_NOERR) {
      std::string errmsg = fmt::format("Error: failed to get string length in file id {}", exoid);
      ex_err_fn(exoid, __func__, errmsg.c_str(), status);
      return EX_FATAL;
    }

    status = nc_def_dim(exoid, dim_num, count, &dimid);
    if (status != NC_NOERR) {
      ex_opts(EX_VERBOSE);
      std::string errmsg =
          fmt::format("Error: failed to define number of {}s in file id {}", type, exoid);
      ex_err_fn(exoid, __func__, errmsg.c_str(), status);
      return EX_FATAL;
    }

    // id status array:
    std::array dim1{dimid};
    status = nc_def_var(exoid, stat_var, NC_INT, 1, Data(dim1), &varid);
    if (status != NC_NOERR) {
      ex_opts(EX_VERBOSE);
      std::string errmsg =
          fmt::format("Error: failed to define side {} status in file id {}", type, exoid);
      ex_err_fn(exoid, __func__, errmsg.c_str(), status);
      return EX_FATAL;
    }
    if (count * 4 < sixty_four_kb) {
      exi_set_compact_storage(exoid, varid);
    }

    // id array:
    int ids_type = get_type(exoid, EX_IDS_INT64_DB);
    int ids_size = ids_type == NC_INT ? 4 : 8;
    status       = nc_def_var(exoid, id_var, ids_type, 1, Data(dim1), &varid);
    if (status != NC_NOERR) {
      ex_opts(EX_VERBOSE);
      std::string errmsg =
          fmt::format("Error: failed to define {} property in file id {}", type, exoid);
      ex_err_fn(exoid, __func__, errmsg.c_str(), status);
      return EX_FATAL;
    }
    if (count * ids_size < sixty_four_kb) {
      exi_set_compact_storage(exoid, varid);
    }

    // store property name as attribute of property array variable
    status = nc_put_att_text(exoid, varid, ATT_PROP_NAME, 3, "ID");
    if (status != NC_NOERR) {
      ex_opts(EX_VERBOSE);
      std::string errmsg = fmt::format("Error: failed to store {} property name {} in file id {}",
                                       type, "ID", exoid);
      ex_err_fn(exoid, __func__, errmsg.c_str(), status);
      return EX_FATAL;
    }

    if (name_var != nullptr) {
      std::array dim{dimid, namestrdim};
      status = nc_def_var(exoid, name_var, NC_CHAR, 2, Data(dim), &varid);
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        std::string errmsg =
            fmt::format("Error: failed to define {} name array in file id {}", type, exoid);
        ex_err_fn(exoid, __func__, errmsg.c_str(), status);
        return EX_FATAL;
      }
      exi_set_compact_storage(exoid, varid);
    }
    return EX_NOERR;
  }
} // namespace
