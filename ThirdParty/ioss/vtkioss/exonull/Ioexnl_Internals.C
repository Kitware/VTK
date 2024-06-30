// Copyright(C) 1999-2023 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "exonull/Ioexnl_Internals.h" // for Internals, ElemBlock, etc
#include "exonull/Ioexnl_Utils.h"

extern "C" {
}

#include <cassert> // for assert
#include <cstring> // for strlen
#include <string>  // for string, operator==, etc
#include <vector>  // for vector

#include "Ioss_Assembly.h"
#include "Ioss_Blob.h"
#include "Ioss_EdgeBlock.h"
#include "Ioss_EdgeSet.h"
#include "Ioss_ElementBlock.h"
#include "Ioss_ElementSet.h"
#include "Ioss_ElementTopology.h"
#include "Ioss_FaceBlock.h"
#include "Ioss_FaceSet.h"
#include "Ioss_Field.h"
#include "Ioss_GroupingEntity.h"
#include "Ioss_NodeBlock.h"
#include "Ioss_NodeSet.h"
#include "Ioss_Property.h"
#include "Ioss_Region.h"
#include "Ioss_SideBlock.h"
#include "Ioss_SideSet.h"
#include "Ioss_Utils.h"
#include "Ioss_VariableType.h"

using namespace Ioexnl;

namespace {
  template <typename T> int get_max_name_length(const std::vector<T> &entities, int old_max);
} // namespace

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
  type           = Ioexnl::map_exodus_type(other.get_member_type());

  const auto &members = other.get_members();
  for (const auto &member : members) {
    assert(member->property_exists("id"));
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

void Mesh::populate(Ioss::Region *region)
{
  {
    const auto &node_blocks = region->get_node_blocks();
    if (!node_blocks.empty()) {
      Ioexnl::NodeBlock N(*node_blocks[0]);
      nodeblocks.push_back(N);
    }
  }

  // Assemblies --
  {
    const auto &assem = region->get_assemblies();
    for (const auto &assembly : assem) {
      Ioexnl::Assembly T(*(assembly));
      assemblies.push_back(T);
    }
  }

  // Blobs --
  {
    const auto &blbs = region->get_blobs();
    for (const auto &blob : blbs) {
      Ioexnl::Blob T(*(blob));
      blobs.push_back(T);
    }
  }

  // Edge Blocks --
  {
    const Ioss::EdgeBlockContainer &edge_blocks = region->get_edge_blocks();
    for (const auto &edge_block : edge_blocks) {
      Ioexnl::EdgeBlock T(*(edge_block));
      edgeblocks.push_back(T);
    }
  }

  // Face Blocks --
  {
    const Ioss::FaceBlockContainer &face_blocks = region->get_face_blocks();
    for (const auto &face_block : face_blocks) {
      Ioexnl::FaceBlock T(*(face_block));
      faceblocks.push_back(T);
    }
  }

  // Element Blocks --
  {
    const Ioss::ElementBlockContainer &element_blocks = region->get_element_blocks();
    for (const auto &element_block : element_blocks) {
      Ioexnl::ElemBlock T(*(element_block));
      elemblocks.push_back(T);
    }
  }

  // NodeSets ...
  {
    const Ioss::NodeSetContainer &node_sets = region->get_nodesets();
    for (const auto &set : node_sets) {
      const Ioexnl::NodeSet T(*(set));
      nodesets.push_back(T);
    }
  }

  // EdgeSets ...
  {
    const Ioss::EdgeSetContainer &edge_sets = region->get_edgesets();
    for (const auto &set : edge_sets) {
      const Ioexnl::EdgeSet T(*(set));
      edgesets.push_back(T);
    }
  }

  // FaceSets ...
  {
    const Ioss::FaceSetContainer &face_sets = region->get_facesets();
    for (const auto &set : face_sets) {
      const Ioexnl::FaceSet T(*(set));
      facesets.push_back(T);
    }
  }

  // ElementSets ...
  {
    const Ioss::ElementSetContainer &element_sets = region->get_elementsets();
    for (const auto &set : element_sets) {
      const Ioexnl::ElemSet T(*(set));
      elemsets.push_back(T);
    }
  }

  // SideSets ...
  {
    const Ioss::SideSetContainer &ssets = region->get_sidesets();
    for (const auto &set : ssets) {
      // Add a SideSet corresponding to this SideSet/SideBlock
      Ioexnl::SideSet T(*set);
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

  for (const auto &nodeblock : nodeblocks) {
    counts.push_back(nodeblock.localOwnedCount);
  }
  for (const auto &edgeblock : edgeblocks) {
    counts.push_back(edgeblock.entityCount);
  }
  for (const auto &faceblock : faceblocks) {
    counts.push_back(faceblock.entityCount);
  }
  for (const auto &elemblock : elemblocks) {
    counts.push_back(elemblock.entityCount);
  }
  for (const auto &nodeset : nodesets) {
    counts.push_back(nodeset.localOwnedCount);
    counts.push_back(nodeset.dfCount);
  }
  for (const auto &edgeset : edgesets) {
    counts.push_back(edgeset.entityCount);
    counts.push_back(edgeset.dfCount);
  }
  for (const auto &faceset : facesets) {
    counts.push_back(faceset.entityCount);
    counts.push_back(faceset.dfCount);
  }
  for (const auto &elemset : elemsets) {
    counts.push_back(elemset.entityCount);
    counts.push_back(elemset.dfCount);
  }
  for (const auto &sideset : sidesets) {
    counts.push_back(sideset.entityCount);
    counts.push_back(sideset.dfCount);
  }
  for (const auto &blob : blobs) {
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

namespace {
  template <typename T> int get_max_name_length(const std::vector<T> &entities, int old_max)
  {
    for (const auto &entity : entities) {
      old_max = std::max(old_max, static_cast<int>(entity.name.size()));
    }
    return (old_max);
  }
} // namespace
