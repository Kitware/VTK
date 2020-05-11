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

#include <Ioss_Assembly.h>
#include <Ioss_Blob.h>
#include <Ioss_CommSet.h>
#include <Ioss_CoordinateFrame.h>
#include <Ioss_DBUsage.h>
#include <Ioss_DatabaseIO.h>
#include <Ioss_EdgeBlock.h>
#include <Ioss_EdgeSet.h>
#include <Ioss_ElementBlock.h>
#include <Ioss_ElementSet.h>
#include <Ioss_ElementTopology.h>
#include <Ioss_EntityBlock.h>
#include <Ioss_EntityType.h>
#include <Ioss_FaceBlock.h>
#include <Ioss_FaceSet.h>
#include <Ioss_Field.h>
#include <Ioss_GroupingEntity.h>
#include <Ioss_NodeBlock.h>
#include <Ioss_NodeSet.h>
#include <Ioss_Property.h>
#include <Ioss_PropertyManager.h>
#include <Ioss_Region.h>
#include <Ioss_SideBlock.h>
#include <Ioss_SideSet.h>
#include <Ioss_SmartAssert.h>
#include <Ioss_State.h>
#include <Ioss_StructuredBlock.h>

#include <algorithm>
#include <cctype>
#include <climits>
#include <cstddef>
#include <fmt/ostream.h>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <utility>
#include <vector>

namespace {
  std::string id_str() { return std::string("id"); }
  std::string db_name_str() { return std::string("db_name"); }
  std::string orig_topo_str() { return std::string("original_topology_type"); }
  std::string orig_block_order() { return std::string("original_block_order"); }

  template <typename T>
  Ioss::GroupingEntity *get_entity_internal(int64_t id, const std::vector<T> &entities)
  {
    for (auto ent : entities) {
      if (ent->property_exists(id_str())) {
        if (id == ent->get_property(id_str()).get_int()) {
          return ent;
        }
      }
    }
    return nullptr;
  }

  template <typename T> size_t get_variable_count(const std::vector<T> &entities)
  {
    Ioss::NameList names;
    for (auto ent : entities) {
      ent->field_describe(Ioss::Field::TRANSIENT, &names);
    }
    Ioss::Utils::uniquify(names);
    return names.size();
  }

  template <typename T> size_t get_reduction_variable_count(const std::vector<T> &entities)
  {
    Ioss::NameList names;
    for (auto ent : entities) {
      ent->field_describe(Ioss::Field::REDUCTION, &names);
    }
    Ioss::Utils::uniquify(names);
    return names.size();
  }

  template <typename T> int64_t get_entity_count(const std::vector<T> &entities)
  {
    int64_t count = 0;
    for (auto ent : entities) {
      count += ent->entity_count();
    }
    return count;
  }

  void update_database(const Ioss::Region *region, Ioss::GroupingEntity *entity)
  {
    entity->reset_database(region->get_database());
  }

  void update_database(const Ioss::Region *region, Ioss::SideSet *sset)
  {
    sset->reset_database(region->get_database());
    const auto &blocks = sset->get_side_blocks();
    for (const auto &block : blocks) {
      block->reset_database(region->get_database());
    }
  }

  void check_for_duplicate_names(const Ioss::Region *region, const Ioss::GroupingEntity *entity)
  {
    const std::string &name = entity->name();

    // See if any alias with this name...
    std::string alias = region->get_alias__(name);

    if (!alias.empty()) {
      // There is an entity with this name...
      const Ioss::GroupingEntity *old_ge = region->get_entity(name);

      if (old_ge != nullptr &&
          !(old_ge->type() == Ioss::SIDEBLOCK || old_ge->type() == Ioss::SIDESET)) {
        std::string        filename = region->get_database()->get_filename();
        std::ostringstream errmsg;
        int64_t            id1 = 0;
        int64_t            id2 = 0;
        if (entity->property_exists(id_str())) {
          id1 = entity->get_property(id_str()).get_int();
        }
        if (old_ge->property_exists(id_str())) {
          id2 = old_ge->get_property(id_str()).get_int();
        }
        fmt::print(errmsg,
                   "ERROR: There are multiple blocks or sets with the same name defined in the "
                   "exodus file '{}'.\n"
                   "\tBoth {} {} and {} {} are named '{}'.  All names must be unique.",
                   filename, entity->type_string(), id1, old_ge->type_string(), id2, name);
        IOSS_ERROR(errmsg);
      }
    }
  }

  constexpr unsigned numberOfBits(unsigned x) { return x < 2 ? x : 1 + numberOfBits(x >> 1); }

  size_t compute_hash(Ioss::GroupingEntity *entity, size_t which)
  {
    // Can add more properties and or fields later.  For now just do
    // name and optional id.
    size_t hash = entity->hash();
    if (entity->property_exists(id_str())) {
      hash += which * entity->get_property(id_str()).get_int();
    }
    return hash;
  }

  template <typename T>
  void compute_hashes(const std::vector<T> &                     entities,
                      std::array<size_t, Ioss::entityTypeCount> &hashes, Ioss::EntityType type)
  {
    auto index = numberOfBits(type) - 1;
    SMART_ASSERT(index < hashes.size())(type)(index)(hashes.size());

    size_t which = 1;
    for (const auto &entity : entities) {
      hashes[index] += compute_hash(entity, which++);
    }
  }

  bool check_hashes(const std::vector<size_t> &min_hash, const std::vector<size_t> &max_hash,
                    Ioss::EntityType type)
  {
    auto index = numberOfBits(type) - 1;
    SMART_ASSERT(index < min_hash.size())(type)(index)(min_hash.size());
    return (min_hash[index] == max_hash[index]);
  }

  template <typename T>
  void report_inconsistency(const std::vector<T> &entities, Ioss::ParallelUtils &util)
  {
    // Know that there is some mismatch in name or (optional)id.  Let user know where...
    std::vector<size_t> hashes;

    size_t which = 1;
    for (const auto &entity : entities) {
      hashes.push_back(compute_hash(entity, which++));
    }

    std::ostringstream errmsg;
    fmt::print(errmsg, "IOSS: ERROR: Parallel Consistency Error.\n\t\t");

    auto min_hash = hashes;
    auto max_hash = hashes;
    // Now find mismatched location...
    util.global_array_minmax(min_hash, Ioss::ParallelUtils::DO_MIN);
    util.global_array_minmax(max_hash, Ioss::ParallelUtils::DO_MAX);

    if (util.parallel_rank() == 0) {
      int count = 0;
      for (size_t i = 0; i < hashes.size(); i++) {
        if (min_hash[i] != max_hash[i]) {
          auto ge = entities[i];
          if (count == 0) {
            fmt::print(errmsg, "{}(s) ", ge->type_string());
          }
          else {
            fmt::print(errmsg, ", ");
          }
          fmt::print(errmsg, "'{}'", ge->name());
          count++;
        }
      }
      fmt::print(errmsg,
                 " {} not consistently defined on all processors.\n\t\t"
                 "Check that name and id matches across processors.\n",
                 (count == 1 ? "is" : "are"));
      IOSS_ERROR(errmsg);
    }
  }

  bool check_parallel_consistency(const Ioss::Region &region)
  {
    if (!region.get_database()->is_parallel()) {
      return true;
    }

    // Want a good approximate test that the grouping entity lists on
    // all processor contain the same entities in the same order.
    // We will say an entity is the same if the name and optional id match.
    //
    // Hash the name and multiply it by position in list and add id+1.
    // Do this for each type separately...  Then verify that they
    // match on all processors...
    std::array<size_t, Ioss::entityTypeCount> hashes{};

    compute_hashes(region.get_node_blocks(), hashes, Ioss::NODEBLOCK);
    compute_hashes(region.get_edge_blocks(), hashes, Ioss::EDGEBLOCK);
    compute_hashes(region.get_face_blocks(), hashes, Ioss::FACEBLOCK);
    compute_hashes(region.get_element_blocks(), hashes, Ioss::ELEMENTBLOCK);
    compute_hashes(region.get_nodesets(), hashes, Ioss::NODESET);
    compute_hashes(region.get_edgesets(), hashes, Ioss::EDGESET);
    compute_hashes(region.get_facesets(), hashes, Ioss::FACESET);
    compute_hashes(region.get_elementsets(), hashes, Ioss::ELEMENTSET);
    compute_hashes(region.get_sidesets(), hashes, Ioss::SIDESET);
    compute_hashes(region.get_commsets(), hashes, Ioss::COMMSET);
    compute_hashes(region.get_structured_blocks(), hashes, Ioss::STRUCTUREDBLOCK);
    compute_hashes(region.get_assemblies(), hashes, Ioss::ASSEMBLY);
    compute_hashes(region.get_blobs(), hashes, Ioss::BLOB);

    auto                util = region.get_database()->util();
    std::vector<size_t> min_hash(hashes.begin(), hashes.end());
    std::vector<size_t> max_hash(hashes.begin(), hashes.end());
    util.global_array_minmax(min_hash, Ioss::ParallelUtils::DO_MIN);
    util.global_array_minmax(max_hash, Ioss::ParallelUtils::DO_MAX);

    bool differ = false;
    if (!check_hashes(min_hash, max_hash, Ioss::NODEBLOCK)) {
      report_inconsistency(region.get_node_blocks(), util);
      differ = true;
    }
    if (!check_hashes(min_hash, max_hash, Ioss::EDGEBLOCK)) {
      report_inconsistency(region.get_edge_blocks(), util);
      differ = true;
    }
    if (!check_hashes(min_hash, max_hash, Ioss::FACEBLOCK)) {
      report_inconsistency(region.get_face_blocks(), util);
      differ = true;
    }
    if (!check_hashes(min_hash, max_hash, Ioss::ELEMENTBLOCK)) {
      report_inconsistency(region.get_element_blocks(), util);
      differ = true;
    }
    if (!check_hashes(min_hash, max_hash, Ioss::NODESET)) {
      report_inconsistency(region.get_nodesets(), util);
      differ = true;
    }
    if (!check_hashes(min_hash, max_hash, Ioss::EDGESET)) {
      report_inconsistency(region.get_edgesets(), util);
      differ = true;
    }
    if (!check_hashes(min_hash, max_hash, Ioss::FACESET)) {
      report_inconsistency(region.get_facesets(), util);
      differ = true;
    }
    if (!check_hashes(min_hash, max_hash, Ioss::ELEMENTSET)) {
      report_inconsistency(region.get_elementsets(), util);
      differ = true;
    }
    if (!check_hashes(min_hash, max_hash, Ioss::SIDESET)) {
      report_inconsistency(region.get_sidesets(), util);
      differ = true;
    }
    if (!check_hashes(min_hash, max_hash, Ioss::COMMSET)) {
      report_inconsistency(region.get_commsets(), util);
      differ = true;
    }
    if (!check_hashes(min_hash, max_hash, Ioss::STRUCTUREDBLOCK)) {
      report_inconsistency(region.get_structured_blocks(), util);
      differ = true;
    }
    if (!check_hashes(min_hash, max_hash, Ioss::ASSEMBLY)) {
      report_inconsistency(region.get_assemblies(), util);
      differ = true;
    }
    if (!check_hashes(min_hash, max_hash, Ioss::BLOB)) {
      report_inconsistency(region.get_blobs(), util);
      differ = true;
    }
    return !differ;
  }

  bool is_input_or_appending_output(const Ioss::DatabaseIO *iodatabase)
  {
    return iodatabase->is_input() || iodatabase->open_create_behavior() == Ioss::DB_APPEND;
  }
} // namespace

namespace Ioss {

  /** \brief Constructor reads in all metadata from disk.
   *
   *  This constructor connects this region to the database, opens the
   *  underlying file, reads all the metadata in the file into Region
   *  and its subentities, and closes the underlying file. Region properties,
   *  such as spatial_dimension, element_block_count, element_count, etc, are
   *  also added to the Region's property manager.
   *
   *  \param[in] iodatabase The name of the database associated with the Region.
   *  \param[in] my_name The name of the Region.
   *
   */
  Region::Region(DatabaseIO *iodatabase, const std::string &my_name)
      : GroupingEntity(iodatabase, my_name, 1), currentState(-1), stateCount(0),
        modelDefined(false), transientDefined(false)
  {
    SMART_ASSERT(iodatabase != nullptr);
    iodatabase->set_region(this);

    if (iodatabase->usage() != Ioss::WRITE_HEARTBEAT &&
        (is_input_or_appending_output(iodatabase))) {
      // Read metadata -- populates GroupingEntity lists and transient data
      Region::begin_mode(STATE_DEFINE_MODEL);
      iodatabase->read_meta_data();
      modelDefined     = true;
      transientDefined = true;
      Region::end_mode(STATE_DEFINE_MODEL);
      if (iodatabase->open_create_behavior() != Ioss::DB_APPEND) {
        Region::begin_mode(STATE_READONLY);
      }
    }

    properties.add(Property(this, "spatial_dimension", Property::INTEGER));
    properties.add(Property(this, "node_block_count", Property::INTEGER));
    properties.add(Property(this, "edge_block_count", Property::INTEGER));
    properties.add(Property(this, "face_block_count", Property::INTEGER));
    properties.add(Property(this, "element_block_count", Property::INTEGER));
    properties.add(Property(this, "structured_block_count", Property::INTEGER));
    properties.add(Property(this, "assembly_count", Property::INTEGER));
    properties.add(Property(this, "blob_count", Property::INTEGER));
    properties.add(Property(this, "side_set_count", Property::INTEGER));
    properties.add(Property(this, "node_set_count", Property::INTEGER));
    properties.add(Property(this, "edge_set_count", Property::INTEGER));
    properties.add(Property(this, "face_set_count", Property::INTEGER));
    properties.add(Property(this, "element_set_count", Property::INTEGER));
    properties.add(Property(this, "comm_set_count", Property::INTEGER));
    properties.add(Property(this, "node_count", Property::INTEGER));
    properties.add(Property(this, "edge_count", Property::INTEGER));
    properties.add(Property(this, "face_count", Property::INTEGER));
    properties.add(Property(this, "element_count", Property::INTEGER));
    properties.add(Property(this, "coordinate_frame_count", Property::INTEGER));
    properties.add(Property(this, "state_count", Property::INTEGER));
    properties.add(Property(this, "current_state", Property::INTEGER));
    properties.add(Property(this, "database_name", Property::STRING));
  }

  Region::~Region()
  {
    // Do anything to the database to make it consistent prior to closing and destructing...
    get_database()->finalize_database();

    // Region owns all sub-grouping entities it contains...
    try {
      IOSS_FUNC_ENTER(m_);
      for (auto nb : nodeBlocks) {
        delete (nb);
      }

      for (auto eb : edgeBlocks) {
        delete (eb);
      }

      for (auto fb : faceBlocks) {
        delete (fb);
      }

      for (auto eb : elementBlocks) {
        delete (eb);
      }

      for (auto sb : structuredBlocks) {
        delete (sb);
      }

      for (auto ss : sideSets) {
        delete (ss);
      }

      for (auto ns : nodeSets) {
        delete (ns);
      }

      for (auto es : edgeSets) {
        delete (es);
      }

      for (auto fs : faceSets) {
        delete (fs);
      }

      for (auto es : elementSets) {
        delete (es);
      }

      for (auto cs : commSets) {
        delete (cs);
      }

      for (auto as : assemblies) {
        delete (as);
      }

      for (auto bl : blobs) {
        delete (bl);
      }

      // Region owns the database pointer even though other entities use it.
      GroupingEntity::really_delete_database();
    }
    catch (...) {
    }
  }

  void Region::delete_database() { GroupingEntity::really_delete_database(); }

  bool Region::node_major() const { return get_database()->node_major(); }

  MeshType Region::mesh_type() const
  {
    if (elementBlocks.empty() && structuredBlocks.empty()) {
      return MeshType::UNSTRUCTURED;
    }
    if (!elementBlocks.empty() && !structuredBlocks.empty()) {
      return MeshType::HYBRID;
    }
    if (!structuredBlocks.empty()) {
      return MeshType::STRUCTURED;
    }
    SMART_ASSERT(!elementBlocks.empty());
    return MeshType::UNSTRUCTURED;
  }

  const std::string Region::mesh_type_string() const
  {
    switch (mesh_type()) {
    case MeshType::UNKNOWN: return "Unknown";
    case MeshType::HYBRID: return "Hybrid";
    case MeshType::STRUCTURED: return "Structured";
    case MeshType::UNSTRUCTURED: return "Unstructured";
    }
    SMART_ASSERT(1 == 0 && "Program Error");
    return "Invalid";
  }

  /** \brief Print a summary of entities in the region.
   *
   *  \param[in,out] strm The output stream to use for printing.
   *  \param[in]     do_transient deprecated and ignored
   */
  void Region::output_summary(std::ostream &strm, bool /* do_transient */) const
  {
    int64_t total_cells       = get_entity_count(get_structured_blocks());
    int64_t total_fs_faces    = get_entity_count(get_facesets());
    int64_t total_ns_nodes    = get_entity_count(get_nodesets());
    int64_t total_es_edges    = get_entity_count(get_edgesets());
    int64_t total_es_elements = get_entity_count(get_elementsets());

    int64_t                       total_sides = 0;
    const Ioss::SideSetContainer &sss         = get_sidesets();
    for (auto fs : sss) {
      total_sides += get_entity_count(fs->get_side_blocks());
    }

    int64_t total_nodes    = get_property("node_count").get_int();
    int64_t total_elements = get_property("element_count").get_int();
    auto    max_entity = std::max({total_sides, total_es_elements, total_fs_faces, total_es_edges,
                                total_ns_nodes, total_cells, total_nodes, total_elements});

    int64_t num_ts = get_property("state_count").get_int();
    auto    max_sb = std::max(
        {get_property("spatial_dimension").get_int(), get_property("node_block_count").get_int(),
         get_property("edge_block_count").get_int(), get_property("face_block_count").get_int(),
         get_property("element_block_count").get_int(),
         get_property("structured_block_count").get_int(), get_property("node_set_count").get_int(),
         get_property("edge_set_count").get_int(), get_property("face_set_count").get_int(),
         get_property("element_set_count").get_int(), get_property("side_set_count").get_int(),
         get_property("assembly_count").get_int(), get_property("blob_count").get_int(), num_ts});

    // Global variables transitioning from TRANSIENT to REDUCTION..
    size_t num_glo_vars  = field_count(Ioss::Field::TRANSIENT);
    size_t num_nod_vars  = get_variable_count(get_node_blocks());
    size_t num_edg_vars  = get_variable_count(get_edge_blocks());
    size_t num_fac_vars  = get_variable_count(get_face_blocks());
    size_t num_ele_vars  = get_variable_count(get_element_blocks());
    size_t num_str_vars  = get_variable_count(get_structured_blocks());
    size_t num_ns_vars   = get_variable_count(get_nodesets());
    size_t num_es_vars   = get_variable_count(get_edgesets());
    size_t num_fs_vars   = get_variable_count(get_facesets());
    size_t num_els_vars  = get_variable_count(get_elementsets());
    size_t num_asm_vars  = get_variable_count(get_assemblies());
    size_t num_blob_vars = get_variable_count(get_blobs());

    size_t num_glo_red_vars  = field_count(Ioss::Field::REDUCTION);
    size_t num_nod_red_vars  = get_variable_count(get_node_blocks());
    size_t num_edg_red_vars  = get_variable_count(get_edge_blocks());
    size_t num_fac_red_vars  = get_variable_count(get_face_blocks());
    size_t num_ele_red_vars  = get_variable_count(get_element_blocks());
    size_t num_str_red_vars  = get_variable_count(get_structured_blocks());
    size_t num_ns_red_vars   = get_variable_count(get_nodesets());
    size_t num_es_red_vars   = get_variable_count(get_edgesets());
    size_t num_fs_red_vars   = get_variable_count(get_facesets());
    size_t num_els_red_vars  = get_variable_count(get_elementsets());
    size_t num_asm_red_vars  = get_reduction_variable_count(get_assemblies());
    size_t num_blob_red_vars = get_variable_count(get_blobs());

    size_t                       num_ss_vars = 0;
    const Ioss::SideSetContainer fss         = get_sidesets();
    for (auto fs : fss) {
      num_ss_vars += get_variable_count(fs->get_side_blocks());
    }

    auto max_vr    = std::max({num_glo_vars,     num_nod_vars,     num_ele_vars,     num_str_vars,
                            num_ns_vars,      num_ss_vars,      num_edg_vars,     num_fac_vars,
                            num_es_vars,      num_fs_vars,      num_els_vars,     num_blob_vars,
                            num_asm_vars,     num_glo_red_vars, num_nod_red_vars, num_edg_red_vars,
                            num_fac_red_vars, num_ele_red_vars, num_str_red_vars, num_ns_red_vars,
                            num_es_red_vars,  num_fs_red_vars,  num_els_red_vars, num_asm_red_vars,
                            num_blob_red_vars});
    int  vr_width  = Ioss::Utils::number_width(max_vr, true) + 2;
    int  num_width = Ioss::Utils::number_width(max_entity, true) + 2;
    int  sb_width  = Ioss::Utils::number_width(max_sb, true) + 2;

    // clang-format off
    fmt::print(
        strm,
        "\n Database: {0}\n"
        " Mesh Type = {1}, {39}\n\n"
        "                      {38:{24}s}\t                 {38:{23}s}\t Variables : Transient / Reduction\n"
        " Spatial dimensions = {2:{24}n}\t                 {38:{23}s}\t Global     = {26:{25}n}\t{44:{25}n}\n"
        " Node blocks        = {7:{24}n}\t Nodes         = {3:{23}n}\t Nodal      = {27:{25}n}\t{45:{25}n}\n"
        " Edge blocks        = {8:{24}n}\t Edges         = {4:{23}n}\t Edge       = {33:{25}n}\t{46:{25}n}\n"
        " Face blocks        = {9:{24}n}\t Faces         = {5:{23}n}\t Face       = {34:{25}n}\t{47:{25}n}\n"
        " Element blocks     = {10:{24}n}\t Elements      = {6:{23}n}\t Element    = {28:{25}n}\t{48:{25}n}\n"
        " Structured blocks  = {11:{24}n}\t Cells         = {17:{23}n}\t Structured = {29:{25}n}\t{49:{25}n}\n"
        " Node sets          = {12:{24}n}\t Node list     = {18:{23}n}\t Nodeset    = {30:{25}n}\t{50:{25}n}\n"
        " Edge sets          = {13:{24}n}\t Edge list     = {19:{23}n}\t Edgeset    = {35:{25}n}\t{51:{25}n}\n"
        " Face sets          = {14:{24}n}\t Face list     = {20:{23}n}\t Faceset    = {36:{25}n}\t{52:{25}n}\n"
        " Element sets       = {15:{24}n}\t Element list  = {21:{23}n}\t Elementset = {37:{25}n}\t{53:{25}n}\n"
        " Element side sets  = {16:{24}n}\t Element sides = {22:{23}n}\t Sideset    = {31:{25}n}\n"
        " Assemblies         = {40:{24}n}\t                 {38:{23}s}\t Assembly   = {41:{25}n}\t{54:{25}n}\n"
        " Blobs              = {42:{24}n}\t                 {38:{23}s}\t Blob       = {43:{25}n}\t{55:{25}n}\n\n"
        " Time steps         = {32:{24}n}\n",
        get_database()->get_filename(), mesh_type_string(),
        get_property("spatial_dimension").get_int(), get_property("node_count").get_int(),
        get_property("edge_count").get_int(), get_property("face_count").get_int(),
        get_property("element_count").get_int(), get_property("node_block_count").get_int(),
        get_property("edge_block_count").get_int(), get_property("face_block_count").get_int(),
        get_property("element_block_count").get_int(),
        get_property("structured_block_count").get_int(), get_property("node_set_count").get_int(),
        get_property("edge_set_count").get_int(), get_property("face_set_count").get_int(),
        get_property("element_set_count").get_int(), get_property("side_set_count").get_int(),
        total_cells, total_ns_nodes, total_es_edges, total_fs_faces, total_es_elements, total_sides,
        num_width, sb_width, vr_width, num_glo_vars, num_nod_vars, num_ele_vars, num_str_vars,
        num_ns_vars, num_ss_vars, num_ts, num_edg_vars, num_fac_vars, num_es_vars, num_fs_vars,
        num_els_vars, " ", get_database()->get_format(), get_property("assembly_count").get_int(),
        num_asm_vars, get_property("blob_count").get_int(), num_blob_vars, num_glo_red_vars,
        num_nod_red_vars, num_edg_red_vars, num_fac_red_vars, num_ele_red_vars, num_str_red_vars,
        num_ns_red_vars, num_es_red_vars, num_fs_red_vars, num_els_red_vars, num_asm_red_vars,
        num_blob_red_vars);
    // clang-format on
  }

  /** \brief Set the Region and the associated DatabaseIO to the given State.
   *
   *  All transitions must begin from the 'STATE_CLOSED' state or be to
   *  the 'STATE_CLOSED' state (There are no nested begin/end pairs at
   *  this time.)
   *
   *  \param[in] new_state The new State to which the Region and DatabaseIO should be set.
   *  \returns True if successful.
   *
   */
  bool Region::begin_mode(State new_state)
  {
    bool success = false;
    {
      IOSS_FUNC_ENTER(m_);
      success = begin_mode__(new_state);
    }
    // Pass the 'begin state' message on to the database so it can do any
    // cleanup/data checking/manipulations it needs to do.
    if (success) {
      DatabaseIO *db = get_database();

      if (new_state == STATE_DEFINE_TRANSIENT && db->usage() == Ioss::WRITE_HISTORY &&
          !(is_input_or_appending_output(db))) {
        set_state(STATE_CLOSED);
        Ioss::Utils::generate_history_mesh(this);
        set_state(new_state);
      }

      IOSS_FUNC_ENTER(m_);
      success = db->begin(new_state);
    }
    return success;
  }

  bool Region::begin_mode__(State new_state)
  {
    bool success = false;
    if (new_state == STATE_CLOSED) {
      success = set_state(new_state);
    }
    else {
      switch (get_state()) {
      case STATE_CLOSED:
        // Make sure we can go to the specified state.
        switch (new_state) {
        default: success = set_state(new_state);
        }
        break;

      // For the invalid transitions; provide a more meaningful
      // message in certain cases...
      case STATE_READONLY: {
        std::ostringstream errmsg;
        fmt::print(errmsg, "Cannot change state of an input (readonly) database in {}",
                   get_database()->get_filename());
        IOSS_ERROR(errmsg);
      }

      break;
      default: {
        std::ostringstream errmsg;
        fmt::print(errmsg, "Invalid nesting of begin/end pairs in {}",
                   get_database()->get_filename());
        IOSS_ERROR(errmsg);
      }
      }
    }
    return success;
  }

  /** \brief Return the Region and the associated DatabaseIO to STATE_CLOSED.
   *
   *  \param[in] current_state The State to end.
   *  \returns True if successful.
   *
   */
  bool Region::end_mode(State current_state)
  {
    IOSS_FUNC_ENTER(m_);
    return end_mode__(current_state);
  }

  bool Region::end_mode__(State current_state)
  {
    // Check that 'current_state' matches the current state of the
    // Region (that is, we are leaving the state we are in).
    if (get_state() != current_state) {
      std::ostringstream errmsg;
      fmt::print(errmsg,
                 "ERROR: Specified end state does not match currently open state\n"
                 "       [{}]\n",
                 get_database()->get_filename());
      IOSS_ERROR(errmsg);
    }

    if (current_state == STATE_DEFINE_MODEL) {
      if (is_input_or_appending_output(get_database())) {
        auto sortName = [](const Ioss::EntityBlock *b1, const Ioss::EntityBlock *b2) {
          return (b1->name() < b2->name());
        };
        std::sort(structuredBlocks.begin(), structuredBlocks.end(), sortName);
      }
      else {
        // Sort the element blocks based on the idOffset field, followed by
        // name...
        auto lessOffset = [](const Ioss::EntityBlock *b1, const Ioss::EntityBlock *b2) {
          SMART_ASSERT(b1->property_exists(orig_block_order()));
          SMART_ASSERT(b2->property_exists(orig_block_order()));
          int64_t b1_orderInt = b1->get_property(orig_block_order()).get_int();
          int64_t b2_orderInt = b2->get_property(orig_block_order()).get_int();
          return ((b1_orderInt == b2_orderInt) ? (b1->name() < b2->name())
                                               : (b1_orderInt < b2_orderInt));
        };

        std::sort(elementBlocks.begin(), elementBlocks.end(), lessOffset);
        std::sort(faceBlocks.begin(), faceBlocks.end(), lessOffset);
        std::sort(edgeBlocks.begin(), edgeBlocks.end(), lessOffset);

        // Now update the block offsets based on this new order...
        {
          int64_t offset = 0;
          for (auto eb : elementBlocks) {
            eb->set_offset(offset);
            offset += eb->entity_count();
          }
        }
        {
          int64_t offset = 0;
          for (auto fb : faceBlocks) {
            fb->set_offset(offset);
            offset += fb->entity_count();
          }
        }
        {
          int64_t offset = 0;
          for (auto eb : edgeBlocks) {
            eb->set_offset(offset);
            offset += eb->entity_count();
          }
        }
      }

      // GroupingEntity consistency check:
      // -- debug and parallel     -- default to true; can disable via environment variable
      // -- non-debug and parallel -- default to false; can enable via environment variable
#ifndef NDEBUG
      bool check_consistency = true;
#else
      bool check_consistency = false;
#endif
      Ioss::Utils::check_set_bool_property(get_database()->get_property_manager(),
                                           "CHECK_PARALLEL_CONSISTENCY", check_consistency);
      if (check_consistency) {
        bool ok = check_parallel_consistency(*this);
        if (!ok) {
          std::ostringstream errmsg;
          fmt::print(errmsg, "ERROR: Parallel Consistency Failure for {} database '{}'.",
                     (get_database()->is_input() ? "input" : "output"),
                     get_database()->get_filename());
          IOSS_ERROR(errmsg);
        }
      }

      modelDefined = true;
    }
    else if (current_state == STATE_DEFINE_TRANSIENT) {
      transientDefined = true;
    }

    // Pass the 'end state' message on to the database so it can do any
    // cleanup/data checking/manipulations it needs to do.
    DatabaseIO *db      = get_database();
    bool        success = db->end(current_state);

    begin_mode__(STATE_CLOSED);

    return success;
  }

  /** \brief Add a state for a specified time.
   *
   *  The states in the region will be 1-based.
   *
   *  \param[in] time The time at the new state.
   *  \returns The state index (1-based).
   */
  int Region::add_state__(double time)
  {
    static bool warning_output = false;

    // NOTE:  For restart input databases, it is possible that the time
    //        is not monotonically increasing...
    if (!get_database()->is_input() && !stateTimes.empty() && time <= stateTimes.back()) {
      // Check that time is increasing...
      if (!warning_output) {
        fmt::print(Ioss::WARNING(),
                   "Current time {} is not greater than previous time {} in\n\t{}.\n"
                   "This may cause problems in applications that assume monotonically increasing "
                   "time values.\n",
                   time, stateTimes.back(), get_database()->get_filename());
        warning_output = true;
      }
    }

    if (get_database()->is_input() || get_database()->usage() == WRITE_RESULTS ||
        get_database()->usage() == WRITE_RESTART) {
      stateTimes.push_back(time);
      SMART_ASSERT((int)stateTimes.size() == stateCount + 1)(stateTimes.size())(stateCount);
    }
    else {

      // Keep only the last time in the vector... This is to avoid
      // memory growth for output databases that write lots of steps
      // (heartbeat, history).  There is no need to keep a list of
      // times that have been written since they are just streamed out
      // and never read We do sometimes need the list of times written
      // to restart or results files though...
      if (stateTimes.empty()) {
        stateTimes.push_back(time);
      }
      else {
        stateTimes[0] = time;
      }
    }
    return ++stateCount;
    ;
  }

  /** \brief Get the time corresponding to the specified state or the currently active state.
   *
   *  \param[in] state The state index (1-based) or -1 for the currently active state.
   *  \returns The time at the specified state or the currently active state.
   */
  double Region::get_state_time(int state) const
  {
    IOSS_FUNC_ENTER(m_);
    double time = 0.0;
    if (state == -1) {
      if (get_database()->is_input() || get_database()->usage() == WRITE_RESULTS ||
          get_database()->usage() == WRITE_RESTART) {
        if (currentState == -1) {
          std::ostringstream errmsg;
          fmt::print(errmsg, "ERROR: No currently active state.\n       [{}]\n",
                     get_database()->get_filename());
          IOSS_ERROR(errmsg);
        }
        else {
          SMART_ASSERT((int)stateTimes.size() >= currentState)(stateTimes.size())(currentState);
          time = stateTimes[currentState - 1];
        }
      }
      else {
        SMART_ASSERT(!stateTimes.empty());
        time = stateTimes[0];
      }
    }
    else if (state <= 0 || state > stateCount) {
      std::ostringstream errmsg;
      fmt::print(errmsg,
                 "ERROR: Requested state ({}) is invalid. State must be between 1 and {}.\n"
                 "       [{}]\n",
                 state, stateCount, get_database()->get_filename());
      IOSS_ERROR(errmsg);
    }
    else {
      if (get_database()->is_input() || get_database()->usage() == WRITE_RESULTS ||
          get_database()->usage() == WRITE_RESTART) {
        SMART_ASSERT((int)stateTimes.size() >= state)(stateTimes.size())(state);
        time = stateTimes[state - 1];
      }
      else {
        SMART_ASSERT(!stateTimes.empty());
        time = stateTimes[0];
      }
    }
    return time;
  }

  /** \brief Get the maximum time step index (1-based) and time for the region.
   *
   *  \returns A pair consisting of the step (1-based) corresponding to
   *           the maximum time on the database and the corresponding maximum
   *           time value. Note that this may not necessarily be the last step
   *           on the database if cycle and overlay are being used.
   */
  std::pair<int, double> Region::get_max_time() const
  {
    IOSS_FUNC_ENTER(m_);
    if (!get_database()->is_input() && get_database()->usage() != WRITE_RESULTS &&
        get_database()->usage() != WRITE_RESTART) {
      return std::make_pair(currentState, stateTimes[0]);
    }
    // Cleanout the stateTimes vector and reload with current data in
    // case the database is being read and written at the same time.
    // This is rare, but is a supported use case.
    stateCount = 0;
    Ioss::Utils::clear(stateTimes);
    DatabaseIO *db = get_database();
    db->get_step_times();

    int    step     = -1;
    double max_time = -1.0;
    for (int i = 0; i < static_cast<int>(stateTimes.size()); i++) {
      if (stateTimes[i] > max_time) {
        step     = i;
        max_time = stateTimes[i];
      }
    }
    return std::make_pair(step + 1, max_time);
  }

  /** \brief Get the minimum time step index (1-based) and time for the region.
   *
   *  \returns A pair consisting of the step (1-based) corresponding to
   *           the minimum time on the database and the corresponding minimum
   *           time value. Note that this may not necessarily be the first step
   *           on the database if cycle and overlay are being used.
   */
  std::pair<int, double> Region::get_min_time() const
  {
    IOSS_FUNC_ENTER(m_);
    if (!get_database()->is_input() && get_database()->usage() != WRITE_RESULTS &&
        get_database()->usage() != WRITE_RESTART) {
      return std::make_pair(currentState, stateTimes[0]);
    }
    // Cleanout the stateTimes vector and reload with current data in
    // case the database is being read and written at the same time.
    // This is rare, but is a supported use case.
    stateCount = 0;
    Ioss::Utils::clear(stateTimes);
    DatabaseIO *db = get_database();
    db->get_step_times();

    int    step     = 0;
    double min_time = stateTimes[0];
    for (int i = 1; i < static_cast<int>(stateTimes.size()); i++) {
      if (stateTimes[i] < min_time) {
        step     = i;
        min_time = stateTimes[i];
      }
    }
    return std::make_pair(step + 1, min_time);
  }

  /** \brief Begin a state (moment in time).
   *
   *  \param[in] state The state index (1-based).
   *  \returns The time of this state.
   */
  double Region::begin_state(int state)
  {
    double time = 0.0;
    if (get_database()->is_input() && stateCount == 0) {
      std::ostringstream errmsg;
      fmt::print(errmsg,
                 "ERROR: There are no states (time steps) on the input database.\n"
                 "       [{}]\n",
                 get_database()->get_filename());
      IOSS_ERROR(errmsg);
    }
    if (state <= 0 || state > stateCount) {
      std::ostringstream errmsg;
      fmt::print(errmsg,
                 "ERROR: Requested state ({}) is invalid. State must be between 1 and {}.\n"
                 "       [{}]\n",
                 state, stateCount, get_database()->get_filename());
      IOSS_ERROR(errmsg);
    }
    else if (currentState != -1 && !get_database()->is_input()) {
      std::ostringstream errmsg;
      fmt::print(errmsg, "ERROR: State {} was not ended. Can not begin new state.\n       [{}]\n",
                 currentState, get_database()->get_filename());
      IOSS_ERROR(errmsg);
    }
    else {
      {
        IOSS_FUNC_ENTER(m_);

        SMART_ASSERT(state <= stateCount)(state)(stateCount);
        if (get_database()->is_input() || get_database()->usage() == WRITE_RESULTS ||
            get_database()->usage() == WRITE_RESTART) {
          SMART_ASSERT((int)stateTimes.size() >= state)(stateTimes.size())(state);
          time = stateTimes[state - 1];
        }
        else {
          SMART_ASSERT(!stateTimes.empty());
          time = stateTimes[0];
        }
        currentState = state;
      }
      DatabaseIO *db = get_database();
      db->begin_state(state, time);
    }
    return time;
  }

  /** \brief End a state (moment in time).
   *
   *  \param[in] state The state index (1-based).
   *  \returns The time of this state.
   */
  double Region::end_state(int state)
  {
    if (state != currentState) {
      std::ostringstream errmsg;
      fmt::print(errmsg,
                 "ERROR: The current database state ({}) does not match the ending state ({}).\n"
                 "       [{}]\n",
                 currentState, state, get_database()->get_filename());
      IOSS_ERROR(errmsg);
    }
    DatabaseIO *db   = get_database();
    double      time = 0.0;
    {
      IOSS_FUNC_ENTER(m_);
      if (get_database()->is_input() || get_database()->usage() == WRITE_RESULTS ||
          get_database()->usage() == WRITE_RESTART) {
        SMART_ASSERT((int)stateTimes.size() >= state)(stateTimes.size())(state);
        time = stateTimes[state - 1];
      }
      else {
        SMART_ASSERT(!stateTimes.empty());
        time = stateTimes[0];
      }
    }
    db->end_state(state, time);
    currentState = -1;
    return time;
  }

  /** \brief Add a structured block to the region.
   *
   *  \param[in] structured_block The structured block to add
   *  \returns True if successful.
   */
  bool Region::add(StructuredBlock *structured_block)
  {
    check_for_duplicate_names(this, structured_block);
    update_database(this, structured_block);
    IOSS_FUNC_ENTER(m_);

    // Check that region is in correct state for adding entities
    if (get_state() == STATE_DEFINE_MODEL) {
      // Add node and cell offsets based on the node_count and
      // cell_count of the previous block.  Only add if there is more
      // than one block; use default for first block (or user-defined
      // values)
      if (!structuredBlocks.empty()) {
        auto   prev_block = structuredBlocks.back();
        size_t num_node   = prev_block->get_property("node_count").get_int();
        size_t num_cell   = prev_block->get_property("cell_count").get_int();
        num_node += prev_block->get_node_offset();
        num_cell += prev_block->get_cell_offset();

        structured_block->set_node_offset(num_node);
        structured_block->set_cell_offset(num_cell);

        size_t global_num_node = prev_block->get_property("global_node_count").get_int();
        size_t global_num_cell = prev_block->get_property("global_cell_count").get_int();
        global_num_node += prev_block->get_node_global_offset();
        global_num_cell += prev_block->get_cell_global_offset();

        structured_block->set_node_global_offset(global_num_node);
        structured_block->set_cell_global_offset(global_num_cell);
      }

      structured_block->property_add(
          Ioss::Property(orig_block_order(), (int)structuredBlocks.size()));
      structuredBlocks.push_back(structured_block);
      // Add name as alias to itself to simplify later uses...
      add_alias__(structured_block);
      return true;
    }
    return false;
  }

  /** \brief Add a node block to the region.
   *
   *  \param[in] node_block The node block to add
   *  \returns True if successful.
   */
  bool Region::add(NodeBlock *node_block)
  {
    check_for_duplicate_names(this, node_block);
    update_database(this, node_block);
    IOSS_FUNC_ENTER(m_);

    // Check that region is in correct state for adding entities
    if (get_state() == STATE_DEFINE_MODEL) {
      nodeBlocks.push_back(node_block);
      // Add name as alias to itself to simplify later uses...
      add_alias__(node_block);

      return true;
    }
    return false;
  }

  /** \brief Remove an assembly to the region.
   *
   *  \param[in] removal The assembly to remove
   *  \returns True if successful.
   *
   *  Checks other assemblies for uses of this assembly and removes
   * if from their member lists.
   */
  bool Region::remove(Assembly *removal)
  {
    IOSS_FUNC_ENTER(m_);

    bool changed = false;
    // Check that region is in correct state for modifying entities
    if (get_state() == STATE_DEFINE_MODEL) {
      // Check all existing assemblies to see if any contain this assembly:
      for (auto *assembly : assemblies) {
        bool removed = assembly->remove(removal);
        if (removed) {
          changed = true;
        }
      }

      // Now remove this assembly...
      for (size_t i = 0; i < assemblies.size(); i++) {
        if (assemblies[i] == removal) {
          assemblies.erase(assemblies.begin() + i);
          changed = true;
        }
      }
    }
    return changed;
  }

  /** \brief Add an assembly to the region.
   *
   *  \param[in] assembly The assembly to add
   *  \returns True if successful.
   */
  bool Region::add(Assembly *assembly)
  {
    check_for_duplicate_names(this, assembly);
    update_database(this, assembly);
    IOSS_FUNC_ENTER(m_);

    // Check that region is in correct state for adding entities
    if (get_state() == STATE_DEFINE_MODEL) {
      assemblies.push_back(assembly);
      // Add name as alias to itself to simplify later uses...
      add_alias__(assembly);

      return true;
    }
    return false;
  }

  /** \brief Add an blob to the region.
   *
   *  \param[in] blob The blob to add
   *  \returns True if successful.
   */
  bool Region::add(Blob *blob)
  {
    check_for_duplicate_names(this, blob);
    update_database(this, blob);
    IOSS_FUNC_ENTER(m_);

    // Check that region is in correct state for adding entities
    if (get_state() == STATE_DEFINE_MODEL) {
      blobs.push_back(blob);
      // Add name as alias to itself to simplify later uses...
      add_alias__(blob);

      return true;
    }
    return false;
  }

  /** \brief Add a coordinate frame to the region.
   *
   *  \param[in] frame The coordinate frame to add
   *  \returns True if successful.
   */
  bool Region::add(const CoordinateFrame &frame)
  {
    IOSS_FUNC_ENTER(m_);
    // Check that region is in correct state for adding entities
    if (get_state() == STATE_DEFINE_MODEL) {
      coordinateFrames.push_back(frame);
      return true;
    }
    return false;
  }

  /** \brief Add an element block to the region.
   *
   *  \param[in] element_block The element block to add
   *  \returns True if successful.
   */
  bool Region::add(ElementBlock *element_block)
  {
    check_for_duplicate_names(this, element_block);
    update_database(this, element_block);
    IOSS_FUNC_ENTER(m_);

    // Check that region is in correct state for adding entities
    if (get_state() == STATE_DEFINE_MODEL) {
      // Add name as alias to itself to simplify later uses...
      add_alias__(element_block);

      // An input database defines these in the order matching the order
      // on the "file".  For output, we need to order based on the
      // "original_block_order" property and calculate the offset at that
      // point.  This is done in "end".

      if (is_input_or_appending_output(get_database())) {
        size_t  nblocks = elementBlocks.size();
        int64_t offset  = 0;
        if (nblocks > 0) {
          offset =
              elementBlocks[nblocks - 1]->get_offset() + elementBlocks[nblocks - 1]->entity_count();
        }
        SMART_ASSERT(offset >= 0)(offset);
        element_block->set_offset(offset);
      }
#if 0
      // Would like to use this, but gives issue in legacy contact...
      // If this is enabled, then remove all settings of
      // "orig_block_order()" from individual DatabaseIO classes.
      element_block->property_add(Ioss::Property(orig_block_order(), (int)elementBlocks.size()));
#else
      else {
        // Check whether the "original_block_order" property exists on
        // this element block. If it isn't there, then add it with a
        // large value. If this is an element block read from the
        // input mesh, then the value will be updated during the
        // 'synchronize_id_and_name' function; if it is a block
        // created by the application during execution, then this
        // value will persist.  Add the property with a very large
        // number such that it will later be sorted after all
        // "original" blocks.  Note that it doesn't matter if two of
        // the "new" blocks have the same value since there is no
        // ordering of new blocks that must be preserved. (Use
        // int_MAX/2 just to avoid some paranoia about strange issue
        // that might arise from int_MAX)
        if (!element_block->property_exists(orig_block_order())) {
          element_block->property_add(Property(orig_block_order(), INT_MAX / 2));
        }
      }
#endif
      elementBlocks.push_back(element_block);
      return true;
    }
    return false;
  }

  /** \brief Add a face block to the region.
   *
   *  \param[in] face_block The face block to add
   *  \returns True if successful.
   */
  bool Region::add(FaceBlock *face_block)
  {
    check_for_duplicate_names(this, face_block);
    update_database(this, face_block);
    IOSS_FUNC_ENTER(m_);

    // Check that region is in correct state for adding entities
    if (get_state() == STATE_DEFINE_MODEL) {
      // Add name as alias to itself to simplify later uses...
      add_alias__(face_block);

      // An input database defines these in the order matching the order
      // on the "file".  For output, we need to order based on the
      // "original_block_order" property and calculate the offset at that
      // point.  This is done in "end".

      if (is_input_or_appending_output(get_database())) {
        size_t  nblocks = faceBlocks.size();
        int64_t offset  = 0;
        if (nblocks > 0) {
          offset = faceBlocks[nblocks - 1]->get_offset() + faceBlocks[nblocks - 1]->entity_count();
        }
        face_block->set_offset(offset);
      }
      face_block->property_add(Ioss::Property(orig_block_order(), (int)faceBlocks.size()));
      faceBlocks.push_back(face_block);
      return true;
    }
    return false;
  }

  /** \brief Add an edge block to the region.
   *
   *  \param[in] edge_block The edge block to add
   *  \returns True if successful.
   */
  bool Region::add(EdgeBlock *edge_block)
  {
    check_for_duplicate_names(this, edge_block);
    update_database(this, edge_block);
    IOSS_FUNC_ENTER(m_);

    // Check that region is in correct state for adding entities
    if (get_state() == STATE_DEFINE_MODEL) {
      // Add name as alias to itself to simplify later uses...
      add_alias__(edge_block);

      // An input database defines these in the order matching the order
      // on the "file".  For output, we need to order based on the
      // "original_block_order" property and calculate the offset at that
      // point.  This is done in "end".

      if (is_input_or_appending_output(get_database())) {
        size_t  nblocks = edgeBlocks.size();
        int64_t offset  = 0;
        if (nblocks > 0) {
          offset = edgeBlocks[nblocks - 1]->get_offset() + edgeBlocks[nblocks - 1]->entity_count();
        }
        edge_block->set_offset(offset);
      }
      edge_block->property_add(Ioss::Property(orig_block_order(), (int)edgeBlocks.size()));
      edgeBlocks.push_back(edge_block);
      return true;
    }
    return false;
  }

  /** \brief Add a side set to the region.
   *
   *  \param[in] sideset The side set to add
   *  \returns True if successful.
   */
  bool Region::add(SideSet *sideset)
  {
    check_for_duplicate_names(this, sideset);
    update_database(this, sideset);
    IOSS_FUNC_ENTER(m_);
    // Check that region is in correct state for adding entities
    if (get_state() == STATE_DEFINE_MODEL) {
      // Add name as alias to itself to simplify later uses...
      add_alias__(sideset);
      sideSets.push_back(sideset);
      return true;
    }
    return false;
  }

  /** \brief Add a node set to the region.
   *
   *  \param[in] nodeset The node set to add
   *  \returns True if successful.
   */
  bool Region::add(NodeSet *nodeset)
  {
    check_for_duplicate_names(this, nodeset);
    update_database(this, nodeset);
    IOSS_FUNC_ENTER(m_);
    // Check that region is in correct state for adding entities
    if (get_state() == STATE_DEFINE_MODEL) {
      // Add name as alias to itself to simplify later uses...
      add_alias__(nodeset);
      nodeSets.push_back(nodeset);
      return true;
    }
    return false;
  }

  /** \brief Add an edge set to the region.
   *
   *  \param[in] edgeset The edge set to add
   *  \returns True if successful.
   */
  bool Region::add(EdgeSet *edgeset)
  {
    check_for_duplicate_names(this, edgeset);
    update_database(this, edgeset);
    IOSS_FUNC_ENTER(m_);
    // Check that region is in correct state for adding entities
    if (get_state() == STATE_DEFINE_MODEL) {
      // Add name as alias to itself to simplify later uses...
      add_alias__(edgeset);
      edgeSets.push_back(edgeset);
      return true;
    }
    return false;
  }

  /** \brief Add a face set to the region.
   *
   *  \param[in] faceset The face set to add
   *  \returns True if successful.
   */
  bool Region::add(FaceSet *faceset)
  {
    check_for_duplicate_names(this, faceset);
    update_database(this, faceset);
    IOSS_FUNC_ENTER(m_);
    // Check that region is in correct state for adding entities
    if (get_state() == STATE_DEFINE_MODEL) {
      // Add name as alias to itself to simplify later uses...
      add_alias__(faceset);
      faceSets.push_back(faceset);
      return true;
    }
    return false;
  }

  /** \brief Add an element set to the region.
   *
   *  \param[in] elementset The element set to add
   *  \returns True if successful.
   */
  bool Region::add(ElementSet *elementset)
  {
    check_for_duplicate_names(this, elementset);
    update_database(this, elementset);
    IOSS_FUNC_ENTER(m_);
    // Check that region is in correct state for adding entities
    if (get_state() == STATE_DEFINE_MODEL) {
      // Add name as alias to itself to simplify later uses...
      add_alias__(elementset);
      elementSets.push_back(elementset);
      return true;
    }
    return false;
  }

  /** \brief Add a comm set to the region.
   *
   *  \param[in] commset The comm set to add
   *  \returns True if successful.
   */
  bool Region::add(CommSet *commset)
  {
    check_for_duplicate_names(this, commset);
    update_database(this, commset);
    IOSS_FUNC_ENTER(m_);
    // Check that region is in correct state for adding entities
    if (get_state() == STATE_DEFINE_MODEL) {
      // Add name as alias to itself to simplify later uses...
      add_alias__(commset);
      commSets.push_back(commset);
      return true;
    }
    return false;
  }

  /** \brief Get all the region's Assembly objects.
   *
   *  \returns A vector of all the region's Assembly objects.
   */
  const AssemblyContainer &Region::get_assemblies() const { return assemblies; }

  /** \brief Get all the region's Blob objects.
   *
   *  \returns A vector of all the region's Blob objects.
   */
  const BlobContainer &Region::get_blobs() const { return blobs; }

  /** \brief Get all the region's NodeBlock objects.
   *
   *  \returns A vector of all the region's NodeBlock objects.
   */
  const NodeBlockContainer &Region::get_node_blocks() const { return nodeBlocks; }

  /** \brief Get all the region's EdgeBlock objects.
   *
   *  \returns A vector of all the region's EdgeBlock objects.
   */
  const EdgeBlockContainer &Region::get_edge_blocks() const { return edgeBlocks; }

  /** \brief Get all the region's FaceBlock objects.
   *
   *  \returns A vector of all the region's FaceBlock objects.
   */
  const FaceBlockContainer &Region::get_face_blocks() const { return faceBlocks; }

  /** \brief Get all the region's ElementBlock objects.
   *
   *  \returns A vector of all the region's ElementBlock objects.
   */
  const ElementBlockContainer &Region::get_element_blocks() const { return elementBlocks; }

  /** \brief Get all the region's StructuredBlock objects.
   *
   *  \returns A vector of all the region's StructuredBlock objects.
   */
  const StructuredBlockContainer &Region::get_structured_blocks() const { return structuredBlocks; }

  /** \brief Get all the region's SideSet objects.
   *
   *  \returns A vector of all the region's SideSet objects.
   */
  const SideSetContainer &Region::get_sidesets() const { return sideSets; }

  /** \brief Get all the region's NodeSet objects.
   *
   *  \returns A vector of all the region's NodeSet objects.
   */
  const NodeSetContainer &Region::get_nodesets() const { return nodeSets; }

  /** \brief Get all the region's EdgeSet objects.
   *
   *  \returns A vector of all the region's EdgeSet objects.
   */
  const EdgeSetContainer &Region::get_edgesets() const { return edgeSets; }

  /** \brief Get all the region's FaceSet objects.
   *
   *  \returns A vector of all the region's FaceSet objects.
   */
  const FaceSetContainer &Region::get_facesets() const { return faceSets; }

  /** \brief Get all the region's ElementSet objects.
   *
   *  \returns A vector of all the region's ElementSet objects.
   */
  const ElementSetContainer &Region::get_elementsets() const { return elementSets; }

  /** \brief Get all the region's CommSet objects.
   *
   *  \returns A vector of all the region's CommSet objects.
   */
  const CommSetContainer &Region::get_commsets() const { return commSets; }

  /** \brief Get all the region's CoordinateFrame objects.
   *
   *  \returns A vector of all the region's CoordinateFrame objects.
   */
  const CoordinateFrameContainer &Region::get_coordinate_frames() const { return coordinateFrames; }

  /** \brief Add a grouping entity's name as an alias for itself.
   *
   *  \param[in] ge The grouping entity.
   *  \returns True if successful
   */
  bool Region::add_alias(const GroupingEntity *ge)
  {
    IOSS_FUNC_ENTER(m_);
    return add_alias__(ge);
  }

  bool Region::add_alias__(const GroupingEntity *ge)
  {
    // See if an entity with this name already exists...
    const std::string &db_name = ge->name();
    std::string        alias   = get_alias__(db_name);

    if (!alias.empty()) {
      const GroupingEntity *old_ge = get_entity(db_name);
      if (old_ge != nullptr && ge != old_ge) {
        if (!((old_ge->type() == SIDEBLOCK && ge->type() == SIDESET) ||
              (ge->type() == SIDEBLOCK && old_ge->type() == SIDESET))) {
          ssize_t old_id = -1;
          ssize_t new_id = -1;
          if (old_ge->property_exists(id_str())) {
            old_id = old_ge->get_property(id_str()).get_int();
          }
          if (ge->property_exists(id_str())) {
            new_id = ge->get_property(id_str()).get_int();
          }

          std::ostringstream errmsg;
          fmt::print(errmsg,
                     "\n\nERROR: Duplicate names detected.\n"
                     "       The name '{}' was found for both {} {} and {} {}.\n"
                     "       Names must be unique over all types in a finite element model.\n\n",
                     db_name, old_ge->type_string(), old_id, ge->type_string(), new_id);
          IOSS_ERROR(errmsg);
        }
      }
    }
    bool success = add_alias__(db_name, db_name);

    // "db_name" property is used with the canonical name setting.
    if (success && ge->property_exists("db_name")) {
      std::string canon_name = ge->get_property("db_name").get_string();
      if (canon_name != db_name) {
        success = add_alias__(db_name, canon_name);
      }
    }

    return success;
  }

  /** \brief Add an alias for a name in a region.
   *
   *  For use with the USTRING type in Sierra, create an uppercase
   *  version of all aliases...
   *
   *  \param[in] db_name The original name.
   *  \param[in] alias the alias
   *  \returns True if successful
   */
  bool Region::add_alias(const std::string &db_name, const std::string &alias)
  {
    IOSS_FUNC_ENTER(m_);
    return add_alias__(db_name, alias);
  }

  bool Region::add_alias__(const std::string &db_name, const std::string &alias)
  {
    // Possible that 'db_name' is itself an alias, resolve down to "canonical"
    // name...
    std::string canon = db_name;
    if (db_name != alias) {
      canon = get_alias__(db_name);
    }

    if (!canon.empty()) {
      std::string uname = Ioss::Utils::uppercase(alias);
      if (uname != alias) {
        aliases_.insert(std::make_pair(uname, canon));
      }

      bool result;
      std::tie(std::ignore, result) = aliases_.insert(std::make_pair(alias, canon));
      return result;
    }
    std::ostringstream errmsg;
    fmt::print(errmsg,
               "\n\nERROR: The entity named '{}' which is being aliased to '{}' does not exist in "
               "region '{}'.\n",
               db_name, alias, name());
    IOSS_ERROR(errmsg);
  }

  /** \brief Get the original name for an alias.
   *
   *  \param[in] alias The alias name.
   *  \returns The original name.
   */
  std::string Region::get_alias(const std::string &alias) const
  {
    IOSS_FUNC_ENTER(m_);
    return get_alias__(alias);
  }

  std::string Region::get_alias__(const std::string &alias) const
  {
    std::string ci_alias = Ioss::Utils::uppercase(alias);
    auto        I        = aliases_.find(ci_alias);
    if (I == aliases_.end()) {
      return "";
    }
    return (*I).second;
  }

  /** \brief Get all aliases for a name in the region.
   *
   *  \param[in] my_name The original name.
   *  \param[in,out] aliases On input, any vector of strings.
   *                         On output, all aliases for my_name are appended.
   *  \returns The number of aliases that were appended.
   *
   */
  int Region::get_aliases(const std::string &my_name, std::vector<std::string> &aliases) const
  {
    IOSS_FUNC_ENTER(m_);
    size_t size = aliases.size();
    for (auto alias_pair : aliases_) {
      std::string alias = alias_pair.first;
      std::string base  = alias_pair.second;
      if (base == my_name) {
        aliases.push_back(alias);
      }
    }
    return static_cast<int>(aliases.size() - size);
  }

  /** \brief Get all original name / alias pairs for the region.
   *
   *  \returns All original name / alias pairs for the region.
   */
  const AliasMap &Region::get_alias_map() const { return aliases_; }

  /** \brief Get an entity of a known EntityType
   *
   *  \param[in] my_name The name of the entity to get
   *  \param[in] io_type The known type of the entity.
   *  \returns The entity with the given name of the given type, or nullptr if not found.
   */
  GroupingEntity *Region::get_entity(const std::string &my_name, EntityType io_type) const
  {
    if (io_type == NODEBLOCK) {
      return get_node_block(my_name);
    }
    if (io_type == ELEMENTBLOCK) {
      return get_element_block(my_name);
    }
    if (io_type == STRUCTUREDBLOCK) {
      return get_structured_block(my_name);
    }
    if (io_type == FACEBLOCK) {
      return get_face_block(my_name);
    }
    if (io_type == EDGEBLOCK) {
      return get_edge_block(my_name);
    }
    if (io_type == SIDESET) {
      return get_sideset(my_name);
    }
    if (io_type == NODESET) {
      return get_nodeset(my_name);
    }
    else if (io_type == EDGESET) {
      return get_edgeset(my_name);
    }
    else if (io_type == FACESET) {
      return get_faceset(my_name);
    }
    else if (io_type == ELEMENTSET) {
      return get_elementset(my_name);
    }
    else if (io_type == COMMSET) {
      return get_commset(my_name);
    }
    else if (io_type == SIDEBLOCK) {
      return get_sideblock(my_name);
    }
    else if (io_type == ASSEMBLY) {
      return get_assembly(my_name);
    }
    else if (io_type == BLOB) {
      return get_blob(my_name);
    }
    return nullptr;
  }

  /** \brief Get an entity of a unknown EntityType
   *
   *  Searches for an entity with the given name in a fixed order of
   *  entity types. First NODEBLOCK entities are searched. Then ELEMENTBLOCK
   *  entities, etc.
   *
   *  \param[in] my_name The name of the entity to get
   *  \returns The entity with the given name, or nullptr if not found.
   */
  GroupingEntity *Region::get_entity(const std::string &my_name) const
  {
    GroupingEntity *entity = get_node_block(my_name);
    if (entity != nullptr) {
      return entity;
    }
    entity = get_element_block(my_name);
    if (entity != nullptr) {
      return entity;
    }
    entity = get_structured_block(my_name);
    if (entity != nullptr) {
      return entity;
    }
    entity = get_face_block(my_name);
    if (entity != nullptr) {
      return entity;
    }
    entity = get_edge_block(my_name);
    if (entity != nullptr) {
      return entity;
    }
    entity = get_sideset(my_name);
    if (entity != nullptr) {
      return entity;
    }
    entity = get_nodeset(my_name);
    if (entity != nullptr) {
      return entity;
    }
    entity = get_edgeset(my_name);
    if (entity != nullptr) {
      return entity;
    }
    entity = get_faceset(my_name);
    if (entity != nullptr) {
      return entity;
    }
    entity = get_elementset(my_name);
    if (entity != nullptr) {
      return entity;
    }
    entity = get_commset(my_name);
    if (entity != nullptr) {
      return entity;
    }
    entity = get_sideblock(my_name);
    if (entity != nullptr) {
      return entity;
    }
    entity = get_assembly(my_name);
    if (entity != nullptr) {
      return entity;
    }
    entity = get_blob(my_name);
    if (entity != nullptr) {
      return entity;
    }

    return entity;
  }

  /** \brief Get an entity of a known EntityType and specified id
   *
   *  \param[in] id The id of the entity to get
   *  \param[in] io_type The known type of the entity.
   *  \returns The entity with the given id of the given type, or nullptr if not found or if ids not
   * supported.
   */
  GroupingEntity *Region::get_entity(int64_t id, EntityType io_type) const
  {
    if (io_type == NODEBLOCK) {
      return get_entity_internal(id, get_node_blocks());
    }
    if (io_type == ELEMENTBLOCK) {
      return get_entity_internal(id, get_element_blocks());
    }
    if (io_type == STRUCTUREDBLOCK) {
      return get_entity_internal(id, get_structured_blocks());
    }
    if (io_type == FACEBLOCK) {
      return get_entity_internal(id, get_face_blocks());
    }
    if (io_type == EDGEBLOCK) {
      return get_entity_internal(id, get_edge_blocks());
    }
    if (io_type == SIDESET) {
      return get_entity_internal(id, get_sidesets());
    }
    if (io_type == NODESET) {
      return get_entity_internal(id, get_nodesets());
    }
    else if (io_type == EDGESET) {
      return get_entity_internal(id, get_edgesets());
    }
    else if (io_type == FACESET) {
      return get_entity_internal(id, get_facesets());
    }
    else if (io_type == ELEMENTSET) {
      return get_entity_internal(id, get_elementsets());
    }
    else if (io_type == COMMSET) {
      return get_entity_internal(id, get_commsets());
    }
    else if (io_type == ASSEMBLY) {
      return get_entity_internal(id, get_assemblies());
    }
    else if (io_type == BLOB) {
      return get_entity_internal(id, get_blobs());
    }
    return nullptr;
  }

  /** \brief Get the assembly with the given name.
   *
   *  \param[in] my_name The name of the assembly to get.
   *  \returns The assembly, or nullptr if not found.
   */
  Assembly *Region::get_assembly(const std::string &my_name) const
  {
    IOSS_FUNC_ENTER(m_);
    const std::string db_name = get_alias__(my_name);
    unsigned int      db_hash = Ioss::Utils::hash(db_name);

    Assembly *ge = nullptr;
    for (auto as : assemblies) {
      if (db_hash == as->hash() && as->name() == db_name) {
        ge = as;
        break;
      }
    }
    return ge;
  }

  /** \brief Get the blob with the given name.
   *
   *  \param[in] my_name The name of the blob to get.
   *  \returns The blob, or nullptr if not found.
   */
  Blob *Region::get_blob(const std::string &my_name) const
  {
    IOSS_FUNC_ENTER(m_);
    const std::string db_name = get_alias__(my_name);
    unsigned int      db_hash = Ioss::Utils::hash(db_name);

    Blob *ge = nullptr;
    for (auto bl : blobs) {
      if (db_hash == bl->hash() && bl->name() == db_name) {
        ge = bl;
        break;
      }
    }
    return ge;
  }

  /** \brief Get the node block with the given name.
   *
   *  \param[in] my_name The name of the node block to get.
   *  \returns The node block, or nullptr if not found.
   */
  NodeBlock *Region::get_node_block(const std::string &my_name) const
  {
    IOSS_FUNC_ENTER(m_);
    const std::string db_name = get_alias__(my_name);
    unsigned int      db_hash = Ioss::Utils::hash(db_name);

    NodeBlock *ge = nullptr;
    for (auto nb : nodeBlocks) {
      if (db_hash == nb->hash() && nb->name() == db_name) {
        ge = nb;
        break;
      }
    }
    return ge;
  }

  /** \brief Get the edge block with the given name.
   *
   *  \param[in] my_name The name of the edge block to get.
   *  \returns The edge block, or nullptr if not found.
   */
  EdgeBlock *Region::get_edge_block(const std::string &my_name) const
  {
    IOSS_FUNC_ENTER(m_);
    const std::string db_name = get_alias__(my_name);
    unsigned int      db_hash = Ioss::Utils::hash(db_name);

    EdgeBlock *ge = nullptr;
    for (auto eb : edgeBlocks) {
      if (db_hash == eb->hash() && eb->name() == db_name) {
        ge = eb;
        break;
      }
    }
    return ge;
  }

  /** \brief Get the face block with the given name.
   *
   *  \param[in] my_name The name of the face block to get.
   *  \returns The face block, or nullptr if not found.
   */
  FaceBlock *Region::get_face_block(const std::string &my_name) const
  {
    IOSS_FUNC_ENTER(m_);
    const std::string db_name = get_alias__(my_name);
    unsigned int      db_hash = Ioss::Utils::hash(db_name);

    FaceBlock *ge = nullptr;
    for (auto fb : faceBlocks) {
      if (db_hash == fb->hash() && fb->name() == db_name) {
        ge = fb;
        break;
      }
    }
    return ge;
  }

  /** \brief Get the element block with the given name.
   *
   *  \param[in] my_name The name of the element block to get.
   *  \returns The element block, or nullptr if not found.
   */
  ElementBlock *Region::get_element_block(const std::string &my_name) const
  {
    IOSS_FUNC_ENTER(m_);
    const std::string db_name = get_alias__(my_name);
    unsigned int      db_hash = Ioss::Utils::hash(db_name);

    ElementBlock *ge = nullptr;
    for (auto eb : elementBlocks) {
      if (db_hash == eb->hash() && eb->name() == db_name) {
        ge = eb;
        break;
      }
    }
    return ge;
  }

  /** \brief Get the structured block with the given name.
   *
   *  \param[in] my_name The name of the structured block to get.
   *  \returns The structured block, or nullptr if not found.
   */
  StructuredBlock *Region::get_structured_block(const std::string &my_name) const
  {
    IOSS_FUNC_ENTER(m_);
    const std::string db_name = get_alias__(my_name);
    unsigned int      db_hash = Ioss::Utils::hash(db_name);

    StructuredBlock *ge = nullptr;
    for (auto sb : structuredBlocks) {
      if (db_hash == sb->hash() && sb->name() == db_name) {
        ge = sb;
        break;
      }
    }
    return ge;
  }

  /** \brief Get the side set with the given name.
   *
   *  \param[in] my_name The name of the side set to get.
   *  \returns The side set, or nullptr if not found.
   */
  SideSet *Region::get_sideset(const std::string &my_name) const
  {
    IOSS_FUNC_ENTER(m_);
    const std::string db_name = get_alias__(my_name);
    unsigned int      db_hash = Ioss::Utils::hash(db_name);

    SideSet *ge = nullptr;
    for (auto ss : sideSets) {
      if (db_hash == ss->hash() && ss->name() == db_name) {
        ge = ss;
        break;
      }
    }
    return ge;
  }

  /** \brief Get the side block with the given name.
   *
   *  \param[in] my_name The name of the side block to get.
   *  \returns The side block, or nullptr if not found.
   */
  SideBlock *Region::get_sideblock(const std::string &my_name) const
  {
    IOSS_FUNC_ENTER(m_);
    SideBlock *ge = nullptr;
    for (auto ss : sideSets) {
      ge = ss->get_side_block(my_name);
      if (ge != nullptr) {
        break;
      }
    }
    return ge;
  }

  /** \brief Get the node set with the given name.
   *
   *  \param[in] my_name The name of the node set to get.
   *  \returns The node set, or nullptr if not found.
   */
  NodeSet *Region::get_nodeset(const std::string &my_name) const
  {
    IOSS_FUNC_ENTER(m_);
    const std::string db_name = get_alias__(my_name);
    unsigned int      db_hash = Ioss::Utils::hash(db_name);

    NodeSet *ge = nullptr;
    for (auto ns : nodeSets) {
      if (db_hash == ns->hash() && ns->name() == db_name) {
        ge = ns;
        break;
      }
    }
    return ge;
  }

  /** \brief Get the edge set with the given name.
   *
   *  \param[in] my_name The name of the edge set to get.
   *  \returns The edge set, or nullptr if not found.
   */
  EdgeSet *Region::get_edgeset(const std::string &my_name) const
  {
    IOSS_FUNC_ENTER(m_);
    const std::string db_name = get_alias__(my_name);
    unsigned int      db_hash = Ioss::Utils::hash(db_name);

    EdgeSet *ge = nullptr;
    for (auto es : edgeSets) {
      if (db_hash == es->hash() && es->name() == db_name) {
        ge = es;
        break;
      }
    }
    return ge;
  }

  /** \brief Get the face set with the given name.
   *
   *  \param[in] my_name The name of the face set to get.
   *  \returns The face set, or nullptr if not found.
   */
  FaceSet *Region::get_faceset(const std::string &my_name) const
  {
    IOSS_FUNC_ENTER(m_);
    const std::string db_name = get_alias__(my_name);
    unsigned int      db_hash = Ioss::Utils::hash(db_name);

    FaceSet *ge = nullptr;
    for (auto fs : faceSets) {
      if (db_hash == fs->hash() && fs->name() == db_name) {
        ge = fs;
        break;
      }
    }
    return ge;
  }

  /** \brief Get the element set with the given name.
   *
   *  \param[in] my_name The name of the element set to get.
   *  \returns The element set, or nullptr if not found.
   */
  ElementSet *Region::get_elementset(const std::string &my_name) const
  {
    IOSS_FUNC_ENTER(m_);
    const std::string db_name = get_alias__(my_name);
    unsigned int      db_hash = Ioss::Utils::hash(db_name);

    ElementSet *ge = nullptr;
    for (auto es : elementSets) {
      if (db_hash == es->hash() && es->name() == db_name) {
        ge = es;
        break;
      }
    }
    return ge;
  }

  /** \brief Get the comm set with the given name.
   *
   *  \param[in] my_name The name of the comm set to get.
   *  \returns The comm set, or nullptr if not found.
   */
  CommSet *Region::get_commset(const std::string &my_name) const
  {
    IOSS_FUNC_ENTER(m_);
    const std::string db_name = get_alias__(my_name);
    unsigned int      db_hash = Ioss::Utils::hash(db_name);

    CommSet *ge = nullptr;
    for (auto cs : commSets) {
      if (db_hash == cs->hash() && cs->name() == db_name) {
        ge = cs;
        break;
      }
    }
    return ge;
  }

  /** \brief Get the coordinate frame with the given id
   *
   *  \param[in] id The id of the coordinate frame to get.
   *  \returns The coordinate frame, or nullptr if not found.
   */
  const CoordinateFrame &Region::get_coordinate_frame(int64_t id) const
  {
    IOSS_FUNC_ENTER(m_);
    for (auto &coor_frame : coordinateFrames) {
      if (coor_frame.id() == id) {
        return coor_frame;
      }
    }
    std::ostringstream errmsg;
    fmt::print(errmsg, "Error: Invalid id {} specified for coordinate frame.", id);
    IOSS_ERROR(errmsg);
  }

  /** \brief Determine whether the entity with the given name and type exists.
   *
   *  \param[in] my_name The name of the entity to search for.
   *  \param[in] io_type The type of the entity.
   *  \param[out] my_type A string representing the type if the entity is found.
   *                      "INVALID" if the entity is not found or the type is invalid.
   *  \returns True if the type is valid and the entity is found.
   */
  bool Region::is_valid_io_entity(const std::string &my_name, unsigned int io_type,
                                  std::string *my_type) const
  {
    // Search all entities defined on this region for the name 'my_name'.
    // If found, then set 'type' (if non-nullptr) to the type of the entity
    // (the 'type' values are from client code that was developed prior
    // to this function, so they are somewhat exodusII specific...).
    if (((io_type & NODEBLOCK) != 0u) && get_node_block(my_name) != nullptr) {
      if (my_type != nullptr) {
        *my_type = "NODE_BLOCK";
      }
      return true;
    }
    if (((io_type & ASSEMBLY) != 0u) && get_assembly(my_name) != nullptr) {
      if (my_type != nullptr) {
        *my_type = "ASSEMBLY";
      }
      return true;
    }
    if (((io_type & BLOB) != 0u) && get_blob(my_name) != nullptr) {
      if (my_type != nullptr) {
        *my_type = "BLOB";
      }
      return true;
    }
    if (((io_type & EDGEBLOCK) != 0u) && get_edge_block(my_name) != nullptr) {
      if (my_type != nullptr) {
        *my_type = "EDGE_BLOCK";
      }
      return true;
    }
    if (((io_type & FACEBLOCK) != 0u) && get_face_block(my_name) != nullptr) {
      if (my_type != nullptr) {
        *my_type = "FACE_BLOCK";
      }
      return true;
    }
    if (((io_type & ELEMENTBLOCK) != 0u) && get_element_block(my_name) != nullptr) {
      if (my_type != nullptr) {
        *my_type = "ELEMENT_BLOCK";
      }
      return true;
    }
    if (((io_type & STRUCTUREDBLOCK) != 0u) && get_structured_block(my_name) != nullptr) {
      if (my_type != nullptr) {
        *my_type = "STRUCTURED_BLOCK";
      }
      return true;
    }
    if (((io_type & SIDESET) != 0u) && get_sideset(my_name) != nullptr) {
      if (my_type != nullptr) {
        *my_type = "SURFACE";
      }
      return true;
    }
    else if (((io_type & NODESET) != 0u) && get_nodeset(my_name) != nullptr) {
      if (my_type != nullptr) {
        *my_type = "NODESET";
      }
      return true;
    }
    else if (((io_type & EDGESET) != 0u) && get_edgeset(my_name) != nullptr) {
      if (my_type != nullptr) {
        *my_type = "EDGESET";
      }
      return true;
    }
    else if (((io_type & FACESET) != 0u) && get_faceset(my_name) != nullptr) {
      if (my_type != nullptr) {
        *my_type = "FACESET";
      }
      return true;
    }
    else if (((io_type & ELEMENTSET) != 0u) && get_elementset(my_name) != nullptr) {
      if (my_type != nullptr) {
        *my_type = "ELEMENTSET";
      }
      return true;
    }
    else if (((io_type & COMMSET) != 0u) && get_commset(my_name) != nullptr) {
      if (my_type != nullptr) {
        *my_type = "COMMSET";
      }
      return true;
    }
    if (my_type != nullptr) {
      *my_type = "INVALID";
    }
    return false;
  }

  /** \brief Get the element block containing a specified element.
   *
   *  \param[in] local_id The local database id (1-based), not the global id.
   *  \returns The element block, or nullptr if no element block contains this
   *           element (local_id <= 0 or greater than number of elements in database)
   */
  ElementBlock *Region::get_element_block(size_t local_id) const
  {
    IOSS_FUNC_ENTER(m_);
    for (auto eb : elementBlocks) {
      if (eb->contains(local_id)) {
        return eb;
      }
    }
    // Should not reach this point...
    std::ostringstream errmsg;
    fmt::print(errmsg,
               "ERROR: In Ioss::Region::get_element_block, an invalid local_id of {} is specified. "
               " The valid range is 1 to {}",
               local_id, get_implicit_property("element_count").get_int());
    IOSS_ERROR(errmsg);
  }

  /** \brief Get the structured block containing a specified global-offset-node.
   *
   *  \param[in] global_offset The offset of cell-nodes for all blocks; 0-based.
   *  \returns The structured block, or nullptr if no structured block contains this
   *           node (local_id <= 0 or greater than number of cell-nodes in database)
   */
  StructuredBlock *Region::get_structured_block(size_t global_offset) const
  {
    IOSS_FUNC_ENTER(m_);
    for (auto sb : structuredBlocks) {
      if (sb->contains(global_offset)) {
        return sb;
      }
    }
    // Should not reach this point...
    std::ostringstream errmsg;
    fmt::print(errmsg,
               "ERROR: In Ioss::Region::get_structured_block, an invalid global_offset of {} is "
               "specified.",
               global_offset);
    IOSS_ERROR(errmsg);
  }

  /** \brief Get an implicit property -- These are calcuated from data stored
   *         in the grouping entity instead of having an explicit value assigned.
   *
   *  An example would be 'element_block_count' for a region.
   *
   *  \param[in] my_name The property name.
   *  \returns The property.
   */
  Property Region::get_implicit_property(const std::string &my_name) const
  {
    if (my_name == "spatial_dimension") {
      if (!nodeBlocks.empty()) {
        return nodeBlocks[0]->get_property("component_degree");
      }

      return Property(my_name, 0);
    }

    if (my_name == "node_block_count") {
      return Property(my_name, static_cast<int>(nodeBlocks.size()));
    }

    if (my_name == "edge_block_count") {
      return Property(my_name, static_cast<int>(edgeBlocks.size()));
    }

    if (my_name == "face_block_count") {
      return Property(my_name, static_cast<int>(faceBlocks.size()));
    }

    if (my_name == "element_block_count") {
      return Property(my_name, static_cast<int>(elementBlocks.size()));
    }

    if (my_name == "structured_block_count") {
      return Property(my_name, static_cast<int>(structuredBlocks.size()));
    }

    if (my_name == "assembly_count") {
      return Property(my_name, static_cast<int>(assemblies.size()));
    }

    if (my_name == "blob_count") {
      return Property(my_name, static_cast<int>(blobs.size()));
    }

    if (my_name == "side_set_count") {
      return Property(my_name, static_cast<int>(sideSets.size()));
    }

    if (my_name == "node_set_count") {
      return Property(my_name, static_cast<int>(nodeSets.size()));
    }

    if (my_name == "edge_set_count") {
      return Property(my_name, static_cast<int>(edgeSets.size()));
    }

    if (my_name == "face_set_count") {
      return Property(my_name, static_cast<int>(faceSets.size()));
    }

    if (my_name == "element_set_count") {
      return Property(my_name, static_cast<int>(elementSets.size()));
    }

    if (my_name == "comm_set_count") {
      return Property(my_name, static_cast<int>(commSets.size()));
    }

    if (my_name == "coordinate_frame_count") {
      return Property(my_name, static_cast<int>(coordinateFrames.size()));
    }

    if (my_name == "state_count") {
      return Property(my_name, stateCount);
    }

    if (my_name == "current_state") {
      return Property(my_name, currentState);
    }

    if (my_name == "element_count") {
      int64_t count = 0;
      for (auto eb : elementBlocks) {
        count += eb->entity_count();
      }
      return Property(my_name, count);
    }

    if (my_name == "cell_count") {
      int64_t count = 0;
      for (auto eb : structuredBlocks) {
        count += eb->get_property("cell_count").get_int();
      }
      return Property(my_name, count);
    }

    if (my_name == "face_count") {
      int64_t count = 0;
      for (auto fb : faceBlocks) {
        count += fb->entity_count();
      }
      return Property(my_name, count);
    }

    if (my_name == "edge_count") {
      int64_t count = 0;
      for (auto eb : edgeBlocks) {
        count += eb->entity_count();
      }
      return Property(my_name, count);
    }

    if (my_name == "node_count") {
      int64_t count = 0;
      for (auto nb : nodeBlocks) {
        count += nb->entity_count();
      }
      return Property(my_name, count);
    }

    if (my_name == "database_name") {
      std::string filename = get_database()->get_filename();
      return Property(my_name, filename);
    }

    {
      return GroupingEntity::get_implicit_property(my_name);
    }
  }

  int64_t Region::internal_get_field_data(const Field &field, void *data, size_t data_size) const
  {
    return get_database()->get_field(this, field, data, data_size);
  }

  int64_t Region::internal_put_field_data(const Field &field, void *data, size_t data_size) const
  {
    return get_database()->put_field(this, field, data, data_size);
  }

  /** \brief Transfer all relevant aliases from this region to another region
   *
   *  \param[in] to The region to which the aliases are to be transferred.
   */
  void Region::transfer_mesh_aliases(Region *to) const
  {
    IOSS_FUNC_ENTER(m_);
    // Iterate through list, [ returns <alias, base_entity_name> ], if
    // 'base_entity_name' is defined on the restart file, add 'alias' as
    // an alias for it...
    for (auto alias_pair : aliases_) {
      std::string alias = alias_pair.first;
      std::string base  = alias_pair.second;
      if (alias != base && to->get_entity(base) != nullptr) {
        to->add_alias__(base, alias);
      }
    }
  }

  /** \brief Ensure that the restart and results files have the same ids.
   *
   *  There is very little connection between an input (mesh) database
   *  and an output (results/restart) database.  Basically, the entity
   *  names are the same between the two files.  This works fine in the
   *  case that an input database has 'generated' entity names of the
   *  form 'block_10' or 'surface_32' since then the output database
   *  can de-generate or decode the name and infer that the block
   *  should have an id of 10 and the surface an id of 32.
   *
   *  However, if alias or other renaming happens, then the output
   *  block may have a name of the form 'fireset' and the underlying
   *  database cannot infer that the id of the block should be 10.
   *  Instead, it sets the id to an arbitrary number (1,2,...).  This
   *  is annoying in the case of the results file since there is no
   *  correspondence between the mesh numbering and the results
   *  numbering. In the case of the restart output file, it can be
   *  disastrous since when the file is used to restart the analysis,
   *  there is no match between the mesh blocks and those found on the
   *  restart file and the restart fails.
   *
   *  So... We need to somehow ensure that the restart (and results)
   *  files have the same ids.  To do this, we do the following:
   *
   *  1. The mesh database will set the property 'id' on input.
   *
   *  2. The results/restart files will have a 'name' based on either
   *     the true name or an alias name.  Whichever, that alias will
   *     appear on the mesh database also, so we can query the mesh
   *     database aliases to get the entity.
   *
   *  3. Once we have the entity, we can query the 'id' property and
   *     add the same 'id' property to the results/restart database.
   *  4. Also set the 'name' property to the base 'name' on the output file.
   *
   *  5. Note that a property may already exist and must be removed
   *    before the 'correct' value is set.
   */
  void Region::synchronize_id_and_name(const Region *from, bool sync_attribute_field_names)
  {
    for (auto alias_pair : aliases_) {
      std::string alias = alias_pair.first;
      std::string base  = alias_pair.second;

      if (alias == base) {

        // Query the 'from' database to get the entity (if any) referred
        // to by the 'alias'
        GroupingEntity *ge = from->get_entity(base);

        if (ge != nullptr) {
          // Get the entity from this region... Must be non-nullptr
          GroupingEntity *this_ge = get_entity(base);
          if (this_ge == nullptr) {
            std::ostringstream errmsg;
            fmt::print(errmsg,
                       "INTERNAL ERROR: Could not find entity '{}' in synchronize_id_and_name() "
                       "                [{}]\n",
                       base, get_database()->get_filename());
            IOSS_ERROR(errmsg);
          }

          // See if there is an 'id' property...
          if (ge->property_exists(id_str())) {
            int64_t id = ge->get_property(id_str()).get_int();
            this_ge->property_update(id_str(), id);
          }
          else {
            // No id, make sure the base name matches in both databases...
            // There is always a 'name' property on an entity
            if (this_ge->name() != base) {
              this_ge->set_name(base);
            }
          }

          // See if there is an 'db_name' property...
          if (ge->property_exists(db_name_str())) {
            std::string db_name = ge->get_property(db_name_str()).get_string();
            // Set the new property
            this_ge->property_update(db_name_str(), db_name);
          }

          // See if there is a 'original_topology_type' property...
          if (ge->property_exists(orig_topo_str())) {
            std::string oes = ge->get_property(orig_topo_str()).get_string();
            this_ge->property_update(orig_topo_str(), oes);
          }

          // Specific to entity blocks. Transfer the "original_block_order"
          // property.
          if (ge->property_exists(orig_block_order())) {
            int64_t offset = ge->get_property(orig_block_order()).get_int();
            this_ge->property_update(orig_block_order(), offset);
          }

          if (sync_attribute_field_names) {
            // If there are any attribute fields, then copy those over
            // to the new entity in order to maintain the same order
            // since some codes access attributes by implicit order and
            // not name... (typically, element blocks only)
            size_t count = this_ge->entity_count();

            Ioss::NameList attr_fields;
            ge->field_describe(Ioss::Field::ATTRIBUTE, &attr_fields);
            for (auto &field_name : attr_fields) {
              const Ioss::Field &field = ge->get_fieldref(field_name);
              if (this_ge->field_exists(field_name)) {
                // If the field is already defined on the entity, make
                // sure that the attribute index matches...
                size_t             index      = field.get_index();
                const Ioss::Field &this_field = this_ge->get_fieldref(field_name);
                this_field.set_index(index);
              }
              else {
                // If the field does not already exist, add it to the
                // output node block
                if (field.raw_count() != count) {
                  Ioss::Field new_field(field);
                  new_field.reset_count(count);
                  this_ge->field_add(new_field);
                }
                else {
                  this_ge->field_add(field);
                }
              }
            }
          }
        }
      }
    }

    for (auto alias_pair : aliases_) {
      std::string alias = alias_pair.first;
      std::string base  = alias_pair.second;

      if (alias != base) {
        GroupingEntity *ge = get_entity(base);
        if (ge != nullptr) {
          add_alias__(base, alias);
        }
      }
    }
  }
} // namespace Ioss
