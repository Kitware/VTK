// Copyright(C) 1999-2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_Assembly.h"
#include "Ioss_Beam2.h"
#include "Ioss_Beam3.h"
#include "Ioss_CodeTypes.h"
#include "Ioss_FaceGenerator.h"
#include "Ioss_Hex20.h"
#include "Ioss_Hex27.h"
#include "Ioss_Hex8.h"
#include "Ioss_IOFactory.h"
#include "Ioss_Node.h"
#include "Ioss_Pyramid13.h"
#include "Ioss_Pyramid14.h"
#include "Ioss_Pyramid5.h"
#include "Ioss_Quad4.h"
#include "Ioss_Quad8.h"
#include "Ioss_Quad9.h"
#include "Ioss_Sort.h"
#include "Ioss_Spring2.h"
#include "Ioss_Spring3.h"
#include "Ioss_StructuredBlock.h"
#include "Ioss_Tet10.h"
#include "Ioss_Tet4.h"
#include "Ioss_Tri3.h"
#include "Ioss_Tri6.h"
#include "Ioss_Unknown.h"
#include "Ioss_Utils.h"
#include "Ioss_Wedge15.h"
#include "Ioss_Wedge18.h"
#include "Ioss_Wedge6.h"
#include <assert.h>
#include "vtk_fmt.h"
#include VTK_FMT(fmt/chrono.h)
#include VTK_FMT(fmt/core.h)
#include VTK_FMT(fmt/format.h)
#include <stdint.h>
#include <stdlib.h>
#if !defined __NVCC__
#include VTK_FMT(fmt/color.h)
#endif
#include "cgns/Iocgns_StructuredZoneData.h"
#include "cgns/Iocgns_Utils.h"
#include "vtk_cgns.h"
#include VTK_CGNS(cgnsconfig.h)
#include VTK_CGNS(cgnstypes.h)
#include <cmath>
#include <cstring>
#include <ctime>
#include VTK_FMT(fmt/ostream.h)
#include <limits>
#include <numeric>
#include <ostream>
#include <set>
#include <string>
#include <tokenize.h>

#include "Ioss_DatabaseIO.h"
#include "Ioss_ElementBlock.h"
#include "Ioss_ElementTopology.h"
#include "Ioss_EntityBlock.h"
#include "Ioss_EntityType.h"
#include "Ioss_GroupingEntity.h"
#include "Ioss_MeshType.h"
#include "Ioss_NodeBlock.h"
#include "Ioss_ParallelUtils.h"
#include "Ioss_Property.h"
#include "Ioss_Region.h"
#include "Ioss_SideBlock.h"
#include "Ioss_SideSet.h"
#include "Ioss_VariableType.h"
#include "Ioss_ZoneConnectivity.h"
#include "robin_hash.h"
#include "robin_set.h"

#include <vtk_cgns.h> // xxx(kitware)
#include VTK_CGNS(cgnsconfig.h)
#include VTK_CGNS(cgnstypes.h)
#if CG_BUILD_PARALLEL
#include VTK_CGNS(pcgnslib.h)
#else
#include VTK_CGNS(cgnslib.h)
#endif

#include "cgns/Iocgns_Defines.h"

#ifdef CG_BUILD_HDF5
#include "vtk_hdf5.h"
#endif

#define CGERR(funcall)                                                                             \
  do {                                                                                             \
    if ((funcall) != CG_OK) {                                                                      \
      Iocgns::Utils::cgns_error(file_ptr, __FILE__, __func__, __LINE__, -1);                       \
    }                                                                                              \
  } while (0)

namespace {
#if defined(__IOSS_WINDOWS__) || defined(__CYGWIN__)
  const char *strcasestr(const char *haystack, const char *needle)
  {
    std::string lneedle(Ioss::Utils::lowercase(needle));
    std::string lhaystack(Ioss::Utils::lowercase(haystack));

    auto pos = lhaystack.find(lneedle);
    return pos != std::string::npos ? haystack + pos : nullptr;
  }
#endif

  int power_2(int count)
  {
    // Return the maximum power of two which is less than or equal to 'count'
    // count = 15 -> returns 8
    // count = 16 -> returns 16
    // count = 17 -> returns 16

    // Use brute force...
    int pow2 = 1;
    while (pow2 <= count) {
      pow2 *= 2;
    }
    if (pow2 > count) {
      pow2 /= 2;
    }
    return pow2;
  }

  struct Range
  {
    Range(int a, int b) : m_beg(a < b ? a : b), m_end(a < b ? b : a), m_reversed(b < a) {}

    int  m_beg;
    int  m_end;
    bool m_reversed;
  };

  bool overlaps(const Range &a, const Range &b) { return a.m_beg <= b.m_end && b.m_beg <= a.m_end; }

  Range subset_range(const Range &a, const Range &b)
  {
    Range ret(std::max(a.m_beg, b.m_beg), std::min(a.m_end, b.m_end));
    ret.m_reversed = a.m_reversed || b.m_reversed;
    return ret;
  }

  void bc_subset_range(const Ioss::StructuredBlock *block, Ioss::BoundaryCondition &bc)
  {
    Ioss::IJK_t ordinal;
    ordinal[0] = block->get_property("ni").get_int();
    ordinal[1] = block->get_property("nj").get_int();
    ordinal[2] = block->get_property("nk").get_int();

    Ioss::IJK_t offset;
    offset[0] = block->get_property("offset_i").get_int();
    offset[1] = block->get_property("offset_j").get_int();
    offset[2] = block->get_property("offset_k").get_int();

    // NOTE: Updates the range in bc

    // Note that block range is nodes and m_ordinal[] is cells, so need to add 1 to range.
    Range z_i(1 + offset[0], ordinal[0] + offset[0] + 1);
    Range z_j(1 + offset[1], ordinal[1] + offset[1] + 1);
    Range z_k(1 + offset[2], ordinal[2] + offset[2] + 1);

    Range gc_i(bc.m_rangeBeg[0], bc.m_rangeEnd[0]);
    Range gc_j(bc.m_rangeBeg[1], bc.m_rangeEnd[1]);
    Range gc_k(bc.m_rangeBeg[2], bc.m_rangeEnd[2]);

    Range gc_ii = subset_range(z_i, gc_i);
    Range gc_jj = subset_range(z_j, gc_j);
    Range gc_kk = subset_range(z_k, gc_k);

    if (overlaps(z_i, gc_i) && overlaps(z_j, gc_j) && overlaps(z_k, gc_k)) {
      bc.m_rangeBeg[0] = gc_ii.m_reversed ? gc_ii.m_end : gc_ii.m_beg;
      bc.m_rangeEnd[0] = gc_ii.m_reversed ? gc_ii.m_beg : gc_ii.m_end;
      bc.m_rangeBeg[1] = gc_jj.m_reversed ? gc_jj.m_end : gc_jj.m_beg;
      bc.m_rangeEnd[1] = gc_jj.m_reversed ? gc_jj.m_beg : gc_jj.m_end;
      bc.m_rangeBeg[2] = gc_kk.m_reversed ? gc_kk.m_end : gc_kk.m_beg;
      bc.m_rangeEnd[2] = gc_kk.m_reversed ? gc_kk.m_beg : gc_kk.m_end;
    }
    else {
      bc.m_rangeBeg = {{0, 0, 0}};
      bc.m_rangeEnd = {{0, 0, 0}};
    }
  }

  int extract_trailing_int(const char *name)
  {
    // 'name' consists of an arbitrary number of characters followed by
    // zero or more digits.  Return the integer value of the contiguous
    // set of trailing digits.
    // Example: Name42 returns 42;  Name_52or_perhaps_3_43 returns 43.

    size_t len = std::strlen(name);
    int    val = 0;
    int    mul = 1;
    for (size_t d = len; d > 0; d--) {
      if (isdigit(name[d - 1])) {
        val += mul * (name[d - 1] - '0');
        mul *= 10;
      }
      else {
        break;
      }
    }
    return val;
  }

  int proc_with_minimum_work(Iocgns::StructuredZoneData *zone, const std::vector<size_t> &work,
                             std::set<std::pair<int, int>> &proc_adam_map)
  {
    size_t min_work = std::numeric_limits<size_t>::max();
    int    min_proc = -1;
    for (int i = 0; i < static_cast<int>(work.size()); i++) {
      if (work[i] < min_work &&
          proc_adam_map.find(std::make_pair(zone->m_adam->m_zone, i)) == proc_adam_map.end()) {
        min_work = work[i];
        min_proc = i;
        if (min_work == 0) {
          break;
        }
      }
    }
    return min_proc;
  }
  void add_bc_to_block(Ioss::StructuredBlock *block, const std::string &boco_name,
                       const std::string &fam_name, int ibc, cgsize_t *range,
                       CGNS_ENUMT(BCType_t) bocotype, bool is_parallel_io)
  {
    Ioss::SideSet *sset = block->get_database()->get_region()->get_sideset(fam_name);
    if (sset == nullptr) {
      if (block->get_database()->parallel_rank() == 0) {
        fmt::print(Ioss::WarnOut(),
                   "On block '{}', found the boundary condition named '{}' in family '{}'.\n"
                   "         This family was not previously defined at the top-level of the file"
                   " which is not normal.\n"
                   "         Check your file to make sure this does not "
                   "indicate a problem with the mesh.\n",
                   block->name(), boco_name, fam_name);
      }

      // Need to create a new sideset since didn't see this earlier.
      auto *db = block->get_database();
      sset     = new Ioss::SideSet(db, fam_name);

      // Get all previous sidesets to make sure we set a unique id...
      int64_t     max_id   = 0;
      const auto &sidesets = db->get_region()->get_sidesets();
      for (const auto &ss : sidesets) {
        if (ss->property_exists("id")) {
          auto id = ss->get_property("id").get_int();
          max_id  = (id > max_id) ? id : max_id;
        }
      }
      sset->property_add(Ioss::Property("id", max_id + 10));
      sset->property_add(Ioss::Property("guid", db->util().generate_guid(max_id + 10)));
      db->get_region()->add(sset);
    }

    assert(sset != nullptr);

    Ioss::IJK_t range_beg{{(int)std::min(range[0], range[3]), (int)std::min(range[1], range[4]),
                           (int)std::min(range[2], range[5])}};

    Ioss::IJK_t range_end{{(int)std::max(range[0], range[3]), (int)std::max(range[1], range[4]),
                           (int)std::max(range[2], range[5])}};

    // Determine overlap of surface with block (in parallel, a block may
    // be split among multiple processors and the block face this is applied
    // to may not exist on this decomposed block)
    auto        bc   = Ioss::BoundaryCondition(boco_name, fam_name, range_beg, range_end);
    std::string name = std::string(boco_name) + "/" + block->name();

    bc_subset_range(block, bc);
    if (!is_parallel_io && !bc.is_valid()) {
      bc.m_rangeBeg = {{0, 0, 0}};
      bc.m_rangeEnd = {{0, 0, 0}};
    }
    block->m_boundaryConditions.push_back(bc);
    auto sb = new Ioss::SideBlock(block->get_database(), name, Ioss::Quad4::name, Ioss::Hex8::name,
                                  block->m_boundaryConditions.back().get_face_count());
    sb->set_parent_block(block);
    sset->add(sb);

    int base = block->get_property("base").get_int();
    int zone = block->get_property("zone").get_int();
    sb->property_add(Ioss::Property("base", base));
    sb->property_add(Ioss::Property("zone", zone));
    sb->property_add(Ioss::Property("section", ibc + 1));
    sb->property_add(Ioss::Property("id", sset->get_property("id").get_int()));
    sb->property_add(Ioss::Property(
        "guid", block->get_database()->util().generate_guid(sset->get_property("id").get_int())));

    // Set a property on the sideset specifying the boundary condition type (bocotype)
    // In CGNS, the bocotype is an enum; we store it as the integer value of the enum.
    if (sset->property_exists("bc_type")) {
      // Check that the 'bocotype' value matches the value of the property.
      auto old_bocotype = sset->get_property("bc_type").get_int();
      if (old_bocotype != bocotype && bocotype != CGNS_ENUMV(FamilySpecified)) {
        fmt::print(Ioss::WarnOut(),
                   "On sideset '{}', the boundary condition type was previously set to {}"
                   " which does not match the current value of {}. It will keep the old value.\n",
                   sset->name(), old_bocotype, static_cast<int>(bocotype));
      }
    }
    else {
      sset->property_add(Ioss::Property("bc_type", (int)bocotype));
    }
  }

  void sync_transient_variables_fpp(Ioss::Region *region)
  {
    // With an fpp read, certain blocks may only be on certain
    // processors -- This consistency is addressed elsewhere; however,
    // if a block is not on a processor, then that block will not have
    // any transient fields.  Need to sync across all processors such
    // that a block has the same fields on all processors.
    //
    // ASSUME: A block will have the same fields in the same order on
    // all processors on which it exists.
    //
    // Do the gather all metadata to proc 0; consolidate and then
    // broadcast back...
    // Need: 'name' and 'VariableType'.  Assume all are double and the
    // size will be processor dependent.
    auto            &sblocks = region->get_structured_blocks();
    std::vector<int> fld_count;
    fld_count.reserve(2 * sblocks.size());
    for (const auto &block : sblocks) {
      fld_count.push_back(block->field_count(Ioss::Field::TRANSIENT));
      const auto &nb = block->get_node_block();
      fld_count.push_back(nb.field_count(Ioss::Field::TRANSIENT));
    }
    auto par = region->get_database()->util();
    par.global_array_minmax(fld_count, Ioss::ParallelUtils::DO_MAX);

    // Determine total number of fields on all blocks...
    int tot_fld = std::accumulate(fld_count.begin(), fld_count.end(), 0);
    // Assuming fields are the same on all processors that have fields...
    std::vector<char> fld_names(tot_fld * 2 * (CGNS_MAX_NAME_LENGTH + 1), 0);

    size_t offset = 0;
    for (size_t i = 0; i < sblocks.size(); i++) {
      const auto    &block  = sblocks[i];
      Ioss::NameList fields = block->field_describe(Ioss::Field::TRANSIENT);
      if (!fields.empty()) {
        for (const auto &field_name : fields) {
          const Ioss::Field &field = block->get_fieldref(field_name);
          std::string        type  = field.raw_storage()->name();
          Ioss::Utils::copy_string(&fld_names[offset], field_name, CGNS_MAX_NAME_LENGTH + 1);
          offset += CGNS_MAX_NAME_LENGTH + 1;
          Ioss::Utils::copy_string(&fld_names[offset], type, CGNS_MAX_NAME_LENGTH + 1);
          offset += CGNS_MAX_NAME_LENGTH + 1;
        }
      }
      else {
        offset += (CGNS_MAX_NAME_LENGTH + 1) * 2 * fld_count[2 * i];
      }
      const auto    &nb          = block->get_node_block();
      Ioss::NameList node_fields = nb.field_describe(Ioss::Field::TRANSIENT);
      if (!node_fields.empty()) {
        for (const auto &field_name : node_fields) {
          const Ioss::Field &field = nb.get_fieldref(field_name);
          std::string        type  = field.raw_storage()->name();
          Ioss::Utils::copy_string(&fld_names[offset], field_name, CGNS_MAX_NAME_LENGTH + 1);
          offset += CGNS_MAX_NAME_LENGTH + 1;
          Ioss::Utils::copy_string(&fld_names[offset], type, CGNS_MAX_NAME_LENGTH + 1);
          offset += CGNS_MAX_NAME_LENGTH + 1;
        }
      }
      else {
        offset += (CGNS_MAX_NAME_LENGTH + 1) * 2 * fld_count[2 * i + 1];
      }
    }

    par.global_array_minmax(fld_names, Ioss::ParallelUtils::DO_MAX);

    // Each processor now should have a consistent list of the field
    // names.  Now need to add the missing fields to the blocks that
    // are not 'native' to this processor...
    //
    offset = 0;
    for (size_t i = 0; i < sblocks.size(); i++) {
      auto &block = sblocks[i];
      if (block->field_count(Ioss::Field::TRANSIENT) != (size_t)fld_count[2 * i]) {
        // Verify that either has 0 or correct number of fields...
        assert(block->field_count(Ioss::Field::TRANSIENT) == 0);

        // Extract the field name and storage type...
        for (int nf = 0; nf < fld_count[2 * i]; nf++) {
          std::string fld_name(&fld_names[offset]);
          offset += CGNS_MAX_NAME_LENGTH + 1;
          std::string fld_type(&fld_names[offset]);
          offset += CGNS_MAX_NAME_LENGTH + 1;

          block->field_add(Ioss::Field(std::move(fld_name), Ioss::Field::DOUBLE, fld_type,
                                       Ioss::Field::TRANSIENT, 0));
        }
      }
      else {
        offset += (CGNS_MAX_NAME_LENGTH + 1) * 2 * fld_count[2 * i];
      }
      assert(block->field_count(Ioss::Field::TRANSIENT) == (size_t)fld_count[2 * i]);

      auto &nb = block->get_node_block();
      if (nb.field_count(Ioss::Field::TRANSIENT) != (size_t)fld_count[2 * i + 1]) {
        // Verify that either has 0 or correct number of fields...
        assert(nb.field_count(Ioss::Field::TRANSIENT) == 0);

        // Extract the field name and storage type...
        for (int nf = 0; nf < fld_count[2 * i + 1]; nf++) {
          std::string fld_name(&fld_names[offset]);
          offset += CGNS_MAX_NAME_LENGTH + 1;
          std::string fld_type(&fld_names[offset]);
          offset += CGNS_MAX_NAME_LENGTH + 1;

          nb.field_add(
              Ioss::Field(fld_name, Ioss::Field::DOUBLE, fld_type, Ioss::Field::TRANSIENT, 0));
        }
      }
      else {
        offset += (CGNS_MAX_NAME_LENGTH + 1) * 2 * fld_count[2 * i + 1];
      }
      assert(nb.field_count(Ioss::Field::TRANSIENT) == (size_t)fld_count[2 * i + 1]);
    }
  }

#if IOSS_DEBUG_OUTPUT
  void output_table(const Ioss::ElementBlockContainer             &ebs,
                    std::map<std::string, Ioss::FaceUnorderedSet> &boundary_faces)
  {
    // Get maximum name and face_count length...
    size_t max_name = std::string("Block Name").length();
    size_t max_face = std::string("Face Count").length();
    for (auto &eb : ebs) {
      const std::string &name = eb->name();
      max_name                = std::max(name.length(), max_name);
      size_t face_width       = Ioss::Utils::number_width(boundary_faces[name].size());
      max_face                = std::max(face_width, max_face);
    }
    max_name += 4; // Padding
    max_face += 4;

    fmt::print("\t+{2:-^{0}}+{2:-^{1}}+\n", max_name, max_face, "");
    fmt::print("\t|{2:^{0}}|{3:^{1}}|\n", max_name, max_face, "Block Name", "Face Count");
    fmt::print("\t+{2:-^{0}}+{2:-^{1}}+\n", max_name, max_face, "");
    for (auto &eb : ebs) {
      const std::string &name = eb->name();
      fmt::print("\t|{2:^{0}}|{3:{1}}  |\n", max_name, max_face - 2, name,
                 fmt::group_digits(boundary_faces[name].size()));
    }
    fmt::print("\t+{2:-^{0}}+{2:-^{1}}+\n", max_name, max_face, "");
  }
#endif

} // namespace

std::pair<std::string, int> Iocgns::Utils::decompose_name(const std::string &name, bool is_parallel)
{
  int         proc = is_parallel ? -1 : 0;
  std::string zname{name};

  if (is_parallel) {
    // Name should/might be of the form `basename_proc-#`.  Strip
    // off the `_proc-#` portion and return just the basename.
    auto tokens = Ioss::tokenize(zname, "_");
    zname       = tokens[0];
    if (tokens.size() >= 2) {
      size_t idx = tokens.size() - 1;
      if (tokens[idx].substr(0, 5) == "proc-") {
        auto ptoken = Ioss::tokenize(tokens[idx], "-");
        proc        = std::stoi(ptoken[1]);
        idx--;
        zname = tokens[idx];
      }
    }
  }
  return std::make_pair(zname, proc);
}

std::string Iocgns::Utils::decompose_sb_name(const std::string &name)
{
  std::string zname{name};

  // Name should/might be of the form `zonename/sb_name`.  Extract
  // the 'sb_name' and return that
  auto tokens = Ioss::tokenize(zname, "/");
  if (tokens.size() >= 2) {
    zname = tokens.back();
  }
  return zname;
}

void Iocgns::Utils::cgns_error(int cgnsid, const char *file, const char *function, int lineno,
                               int processor)
{
  std::ostringstream errmsg;
  fmt::print(errmsg, "CGNS error '{}' at line {} in file '{}' in function '{}'", cg_get_error(),
             lineno, file, function);
  if (processor >= 0) {
    fmt::print(errmsg, " on processor {}", processor);
  }
  fmt::print(errmsg, ". Please report to gdsjaar@sandia.gov if you need help.");
  if (cgnsid > 0) {
#if CG_BUILD_PARALLEL
    // This can cause a hang if not all processors call this routine
    // and then the error is not output...
    //    cgp_close(cgnsid);
#else
    cg_close(cgnsid);
#endif
  }
  IOSS_ERROR(errmsg);
}

Ioss::MeshType Iocgns::Utils::check_mesh_type(int cgns_file_ptr)
{
  // ========================================================================
  // Get the number of zones (element/structured blocks) in the mesh...
  int base      = 1;
  int num_zones = 0;
  CGCHECKNP(cg_nzones(cgns_file_ptr, base, &num_zones));

  CGNS_ENUMT(ZoneType_t) common_zone_type = CGNS_ENUMV(ZoneTypeNull);

  for (int zone = 1; zone <= num_zones; zone++) {
    CGNS_ENUMT(ZoneType_t) zone_type;
    CGCHECKNP(cg_zone_type(cgns_file_ptr, base, zone, &zone_type));

    if (common_zone_type == CGNS_ENUMV(ZoneTypeNull)) {
      common_zone_type = zone_type;
    }

    if (common_zone_type != zone_type) {
#if IOSS_ENABLE_HYBRID
      common_zone_type = CGNS_ENUMV(ZoneTypeUserDefined); // This is how we represent hybrid...
      break;
#else
      IOSS_ERROR(fmt::format("ERROR: CGNS: Zone {} is not the same zone type as previous zones."
                             " This is currently not allowed or supported (hybrid mesh).",
                             zone));
#endif
    }
  }

  switch (common_zone_type) {
  case CGNS_ENUMV(ZoneTypeUserDefined): return Ioss::MeshType::HYBRID;
  case CGNS_ENUMV(Structured): return Ioss::MeshType::STRUCTURED;
  case CGNS_ENUMV(Unstructured): return Ioss::MeshType::UNSTRUCTURED;
  default: return Ioss::MeshType::UNKNOWN;
  }
}

void Iocgns::Utils::update_db_zone_property(int cgns_file_ptr, const Ioss::Region *region,
                                            int myProcessor, bool is_parallel, bool is_parallel_io)
{
  // If an output file is closed/opened, make sure that the zones in the Region
  // match the zones on the database (file). CGNS likes to sort the zones, so they
  // might be in a different order after reopening.  Update the 'db_zone_id' property...
  int num_zones = 0;
  int base      = 1;
  CGCHECK(cg_nzones(cgns_file_ptr, base, &num_zones));

  // Read each zone and put names in a map indexed by zone id.
  // Then iterate all of the region's structured blocks and element blocks
  // and make sure that the zone exists in the map and then update the 'db_zone'
  std::map<std::string, int> zones;

  for (int zone = 1; zone <= num_zones; zone++) {
    cgsize_t size[9];
    char     zname[CGNS_MAX_NAME_LENGTH + 1];
    CGCHECK(cg_zone_read(cgns_file_ptr, base, zone, zname, size));
    auto name_proc         = decompose_name(std::string(zname), is_parallel && !is_parallel_io);
    zones[name_proc.first] = zone;
  }

  const auto &sblocks = region->get_structured_blocks();
  for (const auto &block : sblocks) {
    if (is_parallel_io || block->is_active()) {
      const std::string &name = block->name();
      auto               iter = zones.find(name);
      if (iter != zones.end()) {
        auto db_zone = (*iter).second;
        block->property_update("db_zone", db_zone);
      }
      else {
        IOSS_ERROR(
            fmt::format("ERROR: CGNS: Structured Block '{}' was not found on the CGNS database on "
                        "processor {}.",
                        name, myProcessor));
      }
    }
  }

  const auto &eblocks = region->get_element_blocks();
  for (const auto &block : eblocks) {
    const std::string &name = block->name();
    auto               iter = zones.find(name);
    if (iter != zones.end()) {
      auto db_zone = (*iter).second;
      block->property_update("db_zone", db_zone);
    }
    else {
      IOSS_ERROR(fmt::format(
          "ERROR: CGNS: Element Block '{}' was not found on the CGNS database on processor {}.",
          name, myProcessor));
    }
  }
}

int Iocgns::Utils::get_db_zone(const Ioss::GroupingEntity *entity)
{
  // Returns the zone of the entity as it appears on the cgns database.
  // Usually, but not always the same as the IOSS zone...
  // Can differ on fpp reads and maybe writes.
  if (entity->property_exists("db_zone")) {
    return entity->get_property("db_zone").get_int();
  }
  if (entity->property_exists("zone")) {
    return entity->get_property("zone").get_int();
  }
  IOSS_ERROR(fmt::format(
      "ERROR: CGNS: Entity '{}' of type '{}' does not have the 'zone' property assigned.",
      entity->name(), entity->type_string()));
}

namespace {
  const size_t CG_CELL_CENTER_FIELD_ID = 1ul << 30;
  const size_t CG_VERTEX_FIELD_ID      = 1ul << 31;
} // namespace

size_t Iocgns::Utils::index(const Ioss::Field &field) { return field.get_index() & 0x00ffffff; }

void Iocgns::Utils::set_field_index(const Ioss::Field &field, size_t index,
                                    CGNS_ENUMT(GridLocation_t) location)
{
  if (location == CGNS_ENUMV(CellCenter)) {
    index |= CG_CELL_CENTER_FIELD_ID;
  }
  if (location == CGNS_ENUMV(Vertex)) {
    index |= CG_VERTEX_FIELD_ID;
  }
  field.set_index(index);
}

bool Iocgns::Utils::is_cell_field(const Ioss::Field &field)
{
  size_t index = field.get_index();
  if (index & CG_VERTEX_FIELD_ID) {
    return false;
  }
  if (index & CG_CELL_CENTER_FIELD_ID) {
    return true;
  }
  return !(field.get_name() == "mesh_model_coordinates" ||
           field.get_name() == "mesh_model_coordinates_x" ||
           field.get_name() == "mesh_model_coordinates_y" ||
           field.get_name() == "mesh_model_coordinates_z" ||
           field.get_name() == "cell_node_ids"); // Default to cell field...
}

namespace {
#if CG_BUILD_PARALLEL
  void union_zgc_range(Ioss::ZoneConnectivity &zgc_i, const Ioss::ZoneConnectivity &zgc_j)
  {
    assert(zgc_i.m_transform == zgc_j.m_transform);
    for (int i = 0; i < 3; i++) {
      if (zgc_i.m_ownerRangeBeg[i] <= zgc_i.m_ownerRangeEnd[i]) {
        zgc_i.m_ownerRangeBeg[i] = std::min(zgc_i.m_ownerRangeBeg[i], zgc_j.m_ownerRangeBeg[i]);
        zgc_i.m_ownerRangeEnd[i] = std::max(zgc_i.m_ownerRangeEnd[i], zgc_j.m_ownerRangeEnd[i]);
      }
      else {
        zgc_i.m_ownerRangeBeg[i] = std::max(zgc_i.m_ownerRangeBeg[i], zgc_j.m_ownerRangeBeg[i]);
        zgc_i.m_ownerRangeEnd[i] = std::min(zgc_i.m_ownerRangeEnd[i], zgc_j.m_ownerRangeEnd[i]);
      }

      if (zgc_i.m_donorRangeBeg[i] <= zgc_i.m_donorRangeEnd[i]) {
        zgc_i.m_donorRangeBeg[i] = std::min(zgc_i.m_donorRangeBeg[i], zgc_j.m_donorRangeBeg[i]);
        zgc_i.m_donorRangeEnd[i] = std::max(zgc_i.m_donorRangeEnd[i], zgc_j.m_donorRangeEnd[i]);
      }
      else {
        zgc_i.m_donorRangeBeg[i] = std::max(zgc_i.m_donorRangeBeg[i], zgc_j.m_donorRangeBeg[i]);
        zgc_i.m_donorRangeEnd[i] = std::min(zgc_i.m_donorRangeEnd[i], zgc_j.m_donorRangeEnd[i]);
      }
    }
  }
#endif

  void consolidate_zgc(IOSS_MAYBE_UNUSED const Ioss::Region &region)
  {
    // In parallel, the zgc are not necessarily consistent across processors...
    // and the owner/donor ranges are processor specific.
    // Need to make sure all processors have a consistent list of zgc and the
    // owner/donor ranges contain the union of the ranges on each
    // processor.
    // ...Could do this on a per sb basis, but better to do all at once...
    // Data:
    // CGNS_MAX_NAME_LENGTH - connectionName -- CGNS_MAX_NAME_LENGTH char max
    // 1 - int zone
    // 1 - int donor_zone -- get by mapping donorName to zone
    // 6 cgsize_t[6] ownerRange (can probably use 32-bit int...)
    // 6 cgsize_t[6] donorRange (can probably use 32-bit int...)
    // 3 int[3] transform; (values range from -3 to +3 (could store as single int)
    // CGNS_MAX_NAME_LENGTH characters + 17 ints / connection.

    IOSS_PAR_UNUSED(region);
#if CG_BUILD_PARALLEL
    const int BYTE_PER_NAME = CGNS_MAX_NAME_LENGTH;
    const int INT_PER_ZGC   = 17;
    // Gather all to processor 0, consolidate, and then scatter back...
    int         my_count          = 0;
    const auto &structured_blocks = region.get_structured_blocks();
    for (const auto &sb : structured_blocks) {
      my_count += std::count_if(
          sb->m_zoneConnectivity.begin(), sb->m_zoneConnectivity.end(),
          [](const Ioss::ZoneConnectivity &z) { return !z.is_from_decomp() && z.is_active(); });
    }

    std::vector<int> rcv_data_cnt;
    region.get_database()->util().all_gather(
        my_count, rcv_data_cnt); // Allgather instead of gather so can bail if count=0
    int count = std::accumulate(rcv_data_cnt.begin(), rcv_data_cnt.end(), 0);
    if (count == 0) {
      for (auto &sb : structured_blocks) {
        sb->m_zoneConnectivity.clear();
      }
      return;
    }

    std::vector<char> snd_zgc_name(my_count * BYTE_PER_NAME);
    std::vector<int>  snd_zgc_data(my_count * INT_PER_ZGC);

    // Pack data for gathering to processor 0...
    int off_name = 0;
    int off_data = 0;
    int off_cnt  = 0;

    // ========================================================================
    auto pack_lambda = [&off_data, &off_name, &off_cnt, &snd_zgc_data,
                        &snd_zgc_name](const std::vector<Ioss::ZoneConnectivity> &zgc) {
      for (const auto &z : zgc) {
        if (!z.is_from_decomp() && z.is_active()) {
          Ioss::Utils::copy_string(&snd_zgc_name[off_name], z.m_connectionName, BYTE_PER_NAME);
          off_cnt++;
          off_name += BYTE_PER_NAME;

          snd_zgc_data[off_data++] = z.m_ownerZone;
          snd_zgc_data[off_data++] = z.m_donorZone;

          snd_zgc_data[off_data++] = z.m_ownerRangeBeg[0];
          snd_zgc_data[off_data++] = z.m_ownerRangeBeg[1];
          snd_zgc_data[off_data++] = z.m_ownerRangeBeg[2];
          snd_zgc_data[off_data++] = z.m_ownerRangeEnd[0];
          snd_zgc_data[off_data++] = z.m_ownerRangeEnd[1];
          snd_zgc_data[off_data++] = z.m_ownerRangeEnd[2];

          snd_zgc_data[off_data++] = z.m_donorRangeBeg[0];
          snd_zgc_data[off_data++] = z.m_donorRangeBeg[1];
          snd_zgc_data[off_data++] = z.m_donorRangeBeg[2];
          snd_zgc_data[off_data++] = z.m_donorRangeEnd[0];
          snd_zgc_data[off_data++] = z.m_donorRangeEnd[1];
          snd_zgc_data[off_data++] = z.m_donorRangeEnd[2];

          snd_zgc_data[off_data++] = z.m_transform[0];
          snd_zgc_data[off_data++] = z.m_transform[1];
          snd_zgc_data[off_data++] = z.m_transform[2];
        }
      }
    };
    // ========================================================================

    off_data = off_name = off_cnt = 0;
    for (const auto &sb : structured_blocks) {
      pack_lambda(sb->m_zoneConnectivity);
    }
    assert(off_cnt == my_count);
    assert(my_count == 0 || (off_data % my_count == 0));
    assert(my_count == 0 || (off_data / my_count == INT_PER_ZGC));
    assert(my_count == 0 || (off_name % my_count == 0 && off_name / my_count == BYTE_PER_NAME));

    std::vector<char> rcv_zgc_name;
    std::vector<int>  rcv_zgc_data;
    region.get_database()->util().gather(my_count, BYTE_PER_NAME, snd_zgc_name, rcv_zgc_name);
    region.get_database()->util().gather(my_count, INT_PER_ZGC, snd_zgc_data, rcv_zgc_data);

    // Processor 0 now has all the zgc instances from all blocks on all processors.
    std::vector<Ioss::ZoneConnectivity> zgc;
    if (region.get_database()->util().parallel_rank() == 0) {
      zgc.reserve(count);

      // Unpack data...
      off_data = 0;
      off_name = 0;
      for (int i = 0; i < count; i++) {
        std::string name{&rcv_zgc_name[off_name]};
        off_name += BYTE_PER_NAME;
        int         zone  = rcv_zgc_data[off_data++];
        int         donor = rcv_zgc_data[off_data++];
        Ioss::IJK_t range_beg{
            {rcv_zgc_data[off_data++], rcv_zgc_data[off_data++], rcv_zgc_data[off_data++]}};
        Ioss::IJK_t range_end{
            {rcv_zgc_data[off_data++], rcv_zgc_data[off_data++], rcv_zgc_data[off_data++]}};
        Ioss::IJK_t donor_beg{
            {rcv_zgc_data[off_data++], rcv_zgc_data[off_data++], rcv_zgc_data[off_data++]}};
        Ioss::IJK_t donor_end{
            {rcv_zgc_data[off_data++], rcv_zgc_data[off_data++], rcv_zgc_data[off_data++]}};
        Ioss::IJK_t transform{
            {rcv_zgc_data[off_data++], rcv_zgc_data[off_data++], rcv_zgc_data[off_data++]}};
        zgc.emplace_back(name, zone, "", donor, transform, range_beg, range_end, donor_beg,
                         donor_end);
      }
      assert(off_data % count == 0);
      assert(off_data / count == INT_PER_ZGC);
      assert(off_name % count == 0 && off_name / count == BYTE_PER_NAME);

#if IOSS_DEBUG_OUTPUT
      fmt::print(Ioss::DebugOut(), "ZGC_CONSOLIDATE: Before consolidation: ({})\n", zgc.size());
      for (const auto &z : zgc) {
        fmt::print(Ioss::DebugOut(), "\tOZ {}{}\n", z.m_ownerZone, z);
      }
#endif

      // Consolidate down to the minimum set that has the union of all ranges.
      for (size_t i = 0; i < zgc.size(); i++) {
        if (zgc[i].is_active()) {
          auto owner_zone = zgc[i].m_ownerZone;
          auto donor_zone = zgc[i].m_donorZone;

          for (size_t j = i + 1; j < zgc.size(); j++) {
            if (zgc[j].is_active() && zgc[j].m_connectionName == zgc[i].m_connectionName &&
                zgc[j].m_ownerZone == owner_zone) {
              if (zgc[j].m_donorZone == donor_zone) {
                // Found another instance of the "same" zgc...  Union the ranges
                union_zgc_range(zgc[i], zgc[j]);
                assert(zgc[i].is_valid());

                // Flag the 'j' instance so it is processed only this time.
                zgc[j].m_isActive = false;
              }
              else {
                // We have a bad zgc -- name and owner_zone match, but not donor_zone.
                IOSS_ERROR(fmt::format(
                    "ERROR: CGNS: Found zgc named '{}' on zone {} which has two different "
                    "donor zones: {} and {}\n",
                    zgc[i].m_connectionName, owner_zone, donor_zone, zgc[j].m_donorZone));
              }
            }
          }
        }
      }

      // Cull out all 'non-active' zgc instances (owner and donor zone <= 0)
      zgc.erase(std::remove_if(zgc.begin(), zgc.end(),
                               [](Ioss::ZoneConnectivity &z) { return !z.is_active(); }),
                zgc.end());

      count = (int)zgc.size();
      snd_zgc_name.resize(count * BYTE_PER_NAME);
      snd_zgc_data.resize(count * INT_PER_ZGC);
      // Now have a unique set of zgc over all processors with a union
      // of the ranges on each individual processor.  Pack the data
      // and broadcast back to all processors so all processors can
      // output the same data for Zone Connectivity.
      off_data = off_name = off_cnt = 0;
      pack_lambda(zgc);

      assert(off_cnt == count);
      assert(off_data % count == 0);
      assert(off_data / count == INT_PER_ZGC);
      assert(off_name % count == 0 && off_name / count == BYTE_PER_NAME);

#if IOSS_DEBUG_OUTPUT
      fmt::print(Ioss::DebugOut(), "ZGC_CONSOLIDATE: After consolidation: ({})\n", zgc.size());
      for (const auto &z : zgc) {
        fmt::print(Ioss::DebugOut(), "\tOZ {}{}\n", z.m_ownerZone, z);
      }
#endif
    } // End of processor 0 only processing...

    // Send the list of unique zgc instances to all processors so they can all output.
    region.get_database()->util().broadcast(count);
    snd_zgc_name.resize(count * BYTE_PER_NAME);
    snd_zgc_data.resize(count * INT_PER_ZGC);
    region.get_database()->util().broadcast(snd_zgc_name);
    region.get_database()->util().broadcast(snd_zgc_data);

    // Now clean out existing ZGC lists for all blocks and add on the consolidated instances.
    // Also create a vector for mapping from zone to sb name.
    Ioss::NameList sb_names(structured_blocks.size() + 1);
    for (auto &sb : structured_blocks) {
      sb->m_zoneConnectivity.clear();
      auto zone = sb->get_property("zone").get_int();
      assert(zone < (int)sb_names.size());
      sb_names[zone] = sb->name();
    }

    // Unpack data and apply to the correct structured block.
    off_data = 0;
    off_name = 0;
    for (int i = 0; i < count; i++) {
      std::string name{&snd_zgc_name[off_name]};
      off_name += BYTE_PER_NAME;
      int zone = snd_zgc_data[off_data++];
      assert(zone < (int)sb_names.size());
      int donor = snd_zgc_data[off_data++];
      assert(donor < (int)sb_names.size());
      Ioss::IJK_t range_beg{
          {snd_zgc_data[off_data++], snd_zgc_data[off_data++], snd_zgc_data[off_data++]}};
      Ioss::IJK_t range_end{
          {snd_zgc_data[off_data++], snd_zgc_data[off_data++], snd_zgc_data[off_data++]}};
      Ioss::IJK_t donor_beg{
          {snd_zgc_data[off_data++], snd_zgc_data[off_data++], snd_zgc_data[off_data++]}};
      Ioss::IJK_t donor_end{
          {snd_zgc_data[off_data++], snd_zgc_data[off_data++], snd_zgc_data[off_data++]}};
      Ioss::IJK_t transform{
          {snd_zgc_data[off_data++], snd_zgc_data[off_data++], snd_zgc_data[off_data++]}};

      auto sb = structured_blocks[zone - 1];
      assert(sb->get_property("zone").get_int() == zone);
      sb->m_zoneConnectivity.emplace_back(name, zone, sb_names[donor], donor, transform, range_beg,
                                          range_end, donor_beg, donor_end);
    }
#endif
  }
} // namespace

void Iocgns::Utils::output_assembly(int file_ptr, const Ioss::Assembly *assembly,
                                    bool is_parallel_io, bool appending)
{
  int base = 1;
  int fam  = 0;
  CGERR(cg_family_write(file_ptr, base, assembly->name().c_str(), &fam));

  int64_t id = assembly->get_optional_property("id", 0);
  CGERR(cg_goto(file_ptr, base, "Family_t", fam, nullptr));
  CGERR(cg_descriptor_write("FamVC_TypeId", "0"));
  CGERR(cg_descriptor_write("FamVC_TypeName", "Unspecified"));
  CGERR(cg_descriptor_write("FamVC_UserId", std::to_string(id).c_str()));
  CGERR(cg_descriptor_write("FamVC_UserName", assembly->name().c_str()));

  const auto &members = assembly->get_members();
  // Now, iterate the members of the assembly and add the reference to the structured block
  if (assembly->get_member_type() == Ioss::STRUCTUREDBLOCK) {
    for (const auto &mem : members) {
      base           = mem->get_property("base").get_int();
      const auto *sb = dynamic_cast<const Ioss::StructuredBlock *>(mem);
      Ioss::Utils::check_dynamic_cast(sb);
      if (is_parallel_io || sb->is_active()) {
        int db_zone = get_db_zone(sb);
        if (cg_goto(file_ptr, base, "Zone_t", db_zone, "end") == CG_OK) {
          CGERR(cg_famname_write(assembly->name().c_str()));
        }
      }
    }
  }
  else if (assembly->get_member_type() == Ioss::ELEMENTBLOCK) {
    for (const auto &mem : members) {
      if (appending) {
        // Modifying an existing database so the element blocks
        // should exist on the output database...
        int db_zone = get_db_zone(mem);
        if (cg_goto(file_ptr, base, "Zone_t", db_zone, "end") == CG_OK) {
          CGERR(cg_famname_write(assembly->name().c_str()));
        }
      }
      else {
        // The element blocks have not yet been output.  To make
        // it easier when they are written, add a property that
        // specifies what assembly they are in.  Currently, the way
        // CGNS represents assemblies limits membership to at most one
        // assembly.
        auto *new_mem = const_cast<Ioss::GroupingEntity *>(mem);
        new_mem->property_add(Ioss::Property("assembly", assembly->name()));
      }
    }
  }
}

void Iocgns::Utils::output_assemblies(int file_ptr, const Ioss::Region &region, bool is_parallel_io)
{
  region.get_database()->progress("\tOutput Assemblies");
  const auto &assemblies = region.get_assemblies();
  for (const auto &assem : assemblies) {
    output_assembly(file_ptr, assem, is_parallel_io);
  }
}
void Iocgns::Utils::write_state_meta_data(int file_ptr, const Ioss::Region &region,
                                          bool is_parallel_io)
{
  // Write the metadata to the state file...
  // Not everything is needed, we just need the zones output so we can put the FlowSolutionPointers
  // node under the Zone nodes.
  int base           = 0;
  int phys_dimension = region.get_property("spatial_dimension").get_int();
  CGERR(cg_base_write(file_ptr, "Base", phys_dimension, phys_dimension, &base));

  region.get_database()->progress("\tElement Blocks");
  const Ioss::ElementBlockContainer &ebs = region.get_element_blocks();
  for (auto &eb : ebs) {
    const std::string &name    = eb->name();
    int                db_zone = 0;
    cgsize_t           size[3] = {0, 0, 0};
    size[1]                    = eb->get_property("zone_element_count").get_int();
    size[0]                    = eb->get_property("zone_node_count").get_int();

    if (is_parallel_io) {}

    CGERR(cg_zone_write(file_ptr, base, name.c_str(), size, CGNS_ENUMV(Unstructured), &db_zone));
    int prev_db_zone = get_db_zone(eb);
    if (db_zone != prev_db_zone) {
      IOSS_ERROR(fmt::format(
          "ERROR: CGNS: The 'db_zone' does not match in the state file {} and the base file {}.",
          db_zone, prev_db_zone));
    }
  }

  region.get_database()->progress("\tStructured Blocks");
  const auto &structured_blocks = region.get_structured_blocks();

  // If `is_parallel` and `!is_parallel_io`, then writing file-per-processor
  bool is_parallel = region.get_database()->util().parallel_size() > 1;
  int  rank        = region.get_database()->util().parallel_rank();
  for (const auto &sb : structured_blocks) {
    cgsize_t size[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
    if (is_parallel_io) {
      size[3] = sb->get_property("ni_global").get_int();
      size[4] = sb->get_property("nj_global").get_int();
      size[5] = sb->get_property("nk_global").get_int();
    }
    else {
      size[3] = sb->get_property("ni").get_int();
      size[4] = sb->get_property("nj").get_int();
      size[5] = sb->get_property("nk").get_int();
    }
    size[0] = size[3] + 1;
    size[1] = size[4] + 1;
    size[2] = size[5] + 1;

    if (is_parallel_io || sb->is_active()) {
      std::string name = sb->name();
      if (is_parallel && !is_parallel_io) {
        name += "_proc-";
        name += std::to_string(rank);
      }
      int db_zone = 0;
      CGERR(cg_zone_write(file_ptr, base, name.c_str(), size, CGNS_ENUMV(Structured), &db_zone));
      if (db_zone != sb->get_property("db_zone").get_int()) {
        IOSS_ERROR(fmt::format(
            "ERROR: CGNS: The 'db_zone' does not match in the state file {} and the base file {}.",
            db_zone, sb->get_property("db_zone").get_int()));
      }
    }
  }
}

size_t Iocgns::Utils::common_write_metadata(int file_ptr, const Ioss::Region &region,
                                            std::vector<size_t> &zone_offset, bool is_parallel_io)
{
#if !IOSS_ENABLE_HYBRID
  // Make sure mesh is not hybrid...
  if (region.mesh_type() == Ioss::MeshType::HYBRID) {
    IOSS_ERROR(fmt::format("ERROR: CGNS: The mesh on region '{}' is of type 'hybrid'."
                           " This is currently not allowed or supported.",
                           region.name()));
  }
#endif

  region.get_database()->progress("\tEnter common_write_metadata");
  int base           = 0;
  int phys_dimension = region.get_property("spatial_dimension").get_int();
  CGERR(cg_base_write(file_ptr, "Base", phys_dimension, phys_dimension, &base));

  CGERR(cg_goto(file_ptr, base, "end"));
  std::time_t t    = std::time(nullptr);
  std::string date = fmt::format("{:%Y/%m/%d}", fmt::localtime(t));
  std::string time = fmt::format("{:%H:%M:%S}", fmt::localtime(t));

  std::string code_version = region.get_optional_property("code_version", "unknown");
  std::string code_name    = region.get_optional_property("code_name", "unknown");

  std::string mpi_version{};
#if CG_BUILD_PARALLEL
  {
    char version[MPI_MAX_LIBRARY_VERSION_STRING];
    int  length = 0;
    MPI_Get_library_version(version, &length);
    mpi_version = fmt::format("MPI Version: {}", version);
  }
#endif

  std::string config  = Iocgns::Utils::show_config();
  auto        version = fmt::format(
      "Written by `{}-{}` on {} at {}\n{}{}\nIOSS: CGNS Writer version {}\nPlatform: {}", code_name,
      code_version, date, time, config, mpi_version, __DATE__, Ioss::Utils::platform_information());

#if CG_BUILD_PARALLEL
  if (is_parallel_io) {
    // Need to make sure the version string is the same on all
    // processors since they are all writing to the same file.  There
    // was a difficult to track bug in which the
    // platform_information() contained different node info ("ser9"
    // and "ser43") on certain ranks which caused an HDF5 failure way
    // downstream -- basically at file close.
    region.get_database()->util().broadcast(version);
  }
#endif

  CGERR(cg_descriptor_write("Information", version.c_str()));
  CGERR(cg_goto(file_ptr, base, "end"));
  CGERR(cg_dataclass_write(CGNS_ENUMV(Dimensional)));
  CGERR(cg_units_write(CGNS_ENUMV(MassUnitsUserDefined), CGNS_ENUMV(LengthUnitsUserDefined),
                       CGNS_ENUMV(TimeUnitsUserDefined), CGNS_ENUMV(TemperatureUnitsUserDefined),
                       CGNS_ENUMV(AngleUnitsUserDefined)));

  // Output the sidesets as Family_t nodes
  region.get_database()->progress("\tOutput Sidesets");
  const auto &sidesets = region.get_sidesets();
  for (const auto &ss : sidesets) {
    int fam = 0;
    CGERR(cg_family_write(file_ptr, base, ss->name().c_str(), &fam));

    int bc_index                  = 0;
    CGNS_ENUMT(BCType_t) bocotype = CGNS_ENUMV(BCTypeNull);
    if (ss->property_exists("bc_type")) {
      bocotype = (CGNS_ENUMT(BCType_t))ss->get_property("bc_type").get_int();
    }

    int64_t id = ss->get_optional_property("id", fam);

    CGERR(cg_fambc_write(file_ptr, base, fam, "FamBC", bocotype, &bc_index));
    CGERR(cg_goto(file_ptr, base, "Family_t", fam, nullptr));
    CGERR(cg_descriptor_write("FamBC_TypeId", std::to_string(bocotype).c_str()));
    CGERR(cg_descriptor_write("FamBC_TypeName", cg_BCTypeName(bocotype)));
    CGERR(cg_descriptor_write("FamBC_UserId", std::to_string(id).c_str()));
    CGERR(cg_descriptor_write("FamBC_UserName", ss->name().c_str()));
  }

  // NOTE: Element Block zone write is deferred to put_field_internal so can
  // generate the node count based on connectivity traversal...
  // Just getting processor element count here...
  region.get_database()->progress("\tElement Blocks");
  const auto &element_blocks = region.get_element_blocks();

  size_t element_count = 0;
  for (const auto &eb : element_blocks) {
    int64_t local_count = eb->entity_count();
#if CG_BUILD_PARALLEL
    if (is_parallel_io) {
      int64_t start = 0;
      MPI_Exscan(&local_count, &start, 1, Ioss::mpi_type(start), MPI_SUM,
                 region.get_database()->util().communicator());
      // Of the cells/elements in this zone, this proc handles
      // those starting at 'proc_offset+1' to 'proc_offset+num_entity'
      eb->property_update("proc_offset", start);
    }
#endif
    element_count += (size_t)local_count;
  }

  region.get_database()->progress("\tStructured Blocks");
  const auto &structured_blocks = region.get_structured_blocks();

  // If `is_parallel` and `!is_parallel_io`, then writing file-per-processor
  bool is_parallel = region.get_database()->util().parallel_size() > 1;
  int  rank        = region.get_database()->util().parallel_rank();
  int  zone        = 0;
  for (const auto &sb : structured_blocks) {
    cgsize_t size[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
    if (is_parallel_io) {
      size[3] = sb->get_property("ni_global").get_int();
      size[4] = sb->get_property("nj_global").get_int();
      size[5] = sb->get_property("nk_global").get_int();
    }
    else {
      size[3] = sb->get_property("ni").get_int();
      size[4] = sb->get_property("nj").get_int();
      size[5] = sb->get_property("nk").get_int();
    }
    size[0] = size[3] + 1;
    size[1] = size[4] + 1;
    size[2] = size[5] + 1;

    if (is_parallel_io || sb->is_active()) {
      std::string name = sb->name();
      if (is_parallel && !is_parallel_io) {
        name += "_proc-";
        name += std::to_string(rank);
      }
      int db_zone = 0;
      CGERR(cg_zone_write(file_ptr, base, name.c_str(), size, CGNS_ENUMV(Structured), &db_zone));
      sb->property_update("db_zone", db_zone);
      // Add GridCoordinates Node...
      int grid_idx = 0;
      CGERR(cg_grid_write(file_ptr, base, db_zone, "GridCoordinates", &grid_idx));
    }
    else {
      sb->property_update("db_zone", -1);
    }
    zone++;
    assert(zone > 0);
    zone_offset[zone] = zone_offset[zone - 1] + sb->get_property("cell_count").get_int();
    sb->property_update("zone", zone);
    sb->property_update("base", base);
  }

  // TODO: Are there multi-level assemblies in CGNS?
  // Output the assembly data.
  // The assembly itself is Family data at top level.
  // For each assembly, iterate members and add the 'FamilyName' node linking it to the Assembly
  output_assemblies(file_ptr, region, is_parallel_io);

  region.get_database()->progress("\tMapping sb_name to zone");
  if (is_parallel_io || !is_parallel) { // Only for single file output or serial...
    // Create a vector for mapping from sb_name to zone -- used to update zgc instances
    std::map<std::string, int> sb_zone;
    for (const auto &sb : structured_blocks) {
      zone                = sb->get_property("zone").get_int();
      sb_zone[sb->name()] = zone;
    }

    // Update zgc instances to make sure the ownerZone and donorZone are
    // consistent with the zones on the output database (from cg_zone_write call)
    for (const auto &sb : structured_blocks) {
      int owner_zone = sb->get_property("zone").get_int();
      for (auto &zgc : sb->m_zoneConnectivity) {
        int donor_zone  = sb_zone[zgc.m_donorName];
        zgc.m_ownerZone = owner_zone;
        zgc.m_ownerGUID = region.get_database()->util().generate_guid(owner_zone);
        zgc.m_donorZone = donor_zone;
        zgc.m_donorGUID = region.get_database()->util().generate_guid(donor_zone);
      }
    }
  }

  region.get_database()->progress("\tConsolidate zgc");
  if (is_parallel_io) {
    consolidate_zgc(region);
  }

  region.get_database()->progress("\tStructured Block Loop");
  for (const auto &sb : structured_blocks) {
    if (!is_parallel_io && !sb->is_active()) {
      continue;
    }

    auto        db_zone = get_db_zone(sb);
    std::string name    = sb->name();
    if (is_parallel && !is_parallel_io) {
      name += "_proc-";
      name += std::to_string(rank);
    }

    // Transfer boundary condition nodes...
    // The bc.m_ownerRange argument needs to be the union of the size on all processors
    // Instead of requiring that of the caller, do the union in this routine.
    // TODO: Calculate it outside of the loop...
    // Need to handle possible range == 0,0,0.  Only affects the beg data...
    if (is_parallel_io) {
      region.get_database()->progress("\t\tBoundary Conditions");
    }
    CGNSIntVector bc_range(sb->m_boundaryConditions.size() * 6);
    size_t        idx = 0;
    for (const auto &bc : sb->m_boundaryConditions) {
      for (size_t i = 0; i < 3; i++) {
        if (bc.m_rangeBeg[i] == 0) {
          bc_range[idx++] = std::numeric_limits<int>::min();
        }
        else {
          bc_range[idx++] = -bc.m_rangeBeg[i];
        }
      }
      for (size_t i = 0; i < 3; i++) {
        bc_range[idx++] = bc.m_rangeEnd[i];
      }
    }

    if (is_parallel_io) {
      region.get_database()->util().global_array_minmax(bc_range, Ioss::ParallelUtils::DO_MAX);
    }

    for (idx = 0; idx < bc_range.size(); idx += 6) {
      bc_range[idx + 0] = -bc_range[idx + 0];
      bc_range[idx + 1] = -bc_range[idx + 1];
      bc_range[idx + 2] = -bc_range[idx + 2];
    }

    Ioss::IJK_t offset;
    offset[0] = sb->get_property("offset_i").get_int();
    offset[1] = sb->get_property("offset_j").get_int();
    offset[2] = sb->get_property("offset_k").get_int();

    idx = 0;
    for (const auto &bc : sb->m_boundaryConditions) {
      int bc_idx = 0;
      if (!is_parallel_io) {
        bc_range[idx + 0] -= offset[0];
        bc_range[idx + 1] -= offset[1];
        bc_range[idx + 2] -= offset[2];
        bc_range[idx + 3] -= offset[0];
        bc_range[idx + 4] -= offset[1];
        bc_range[idx + 5] -= offset[2];
      }

      if (is_parallel_io ||
          (bc_range[idx + 3] > 0 && bc_range[idx + 4] > 0 && bc_range[idx + 5] > 0)) {
        CGERR(cg_boco_write(file_ptr, base, db_zone, bc.m_bcName.c_str(),
                            CGNS_ENUMV(FamilySpecified), CGNS_ENUMV(PointRange), 2, &bc_range[idx],
                            &bc_idx));
        CGERR(
            cg_goto(file_ptr, base, name.c_str(), 0, "ZoneBC_t", 1, bc.m_bcName.c_str(), 0, "end"));
        CGERR(cg_famname_write(bc.m_famName.c_str()));
        CGERR(cg_boco_gridlocation_write(file_ptr, base, db_zone, bc_idx, CGNS_ENUMV(Vertex)));
      }
      idx += 6;
    }
    // Transfer Zone Grid Connectivity...
    if (is_parallel_io) {
      region.get_database()->progress("\t\tZone Grid Connectivity");
    }

    // Used to detect duplicate zgc names in parallel but non-parallel-io case
    std::set<std::string> zgc_names;

    for (const auto &zgc : sb->m_zoneConnectivity) {
      if (zgc.is_valid() &&
          (zgc.is_active() || (!is_parallel && zgc.m_donorProcessor != zgc.m_ownerProcessor))) {
        int                     zgc_idx = 0;
        std::array<cgsize_t, 6> owner_range{{zgc.m_ownerRangeBeg[0], zgc.m_ownerRangeBeg[1],
                                             zgc.m_ownerRangeBeg[2], zgc.m_ownerRangeEnd[0],
                                             zgc.m_ownerRangeEnd[1], zgc.m_ownerRangeEnd[2]}};
        std::array<cgsize_t, 6> donor_range{{zgc.m_donorRangeBeg[0], zgc.m_donorRangeBeg[1],
                                             zgc.m_donorRangeBeg[2], zgc.m_donorRangeEnd[0],
                                             zgc.m_donorRangeEnd[1], zgc.m_donorRangeEnd[2]}};

        std::string donor_name    = zgc.m_donorName;
        std::string connect_name  = zgc.m_connectionName;
        std::string original_name = zgc.m_connectionName;
        if (is_parallel && !is_parallel_io) {
          if (zgc.is_from_decomp()) {
            connect_name = std::to_string(zgc.m_ownerGUID) + "--" + std::to_string(zgc.m_donorGUID);
          }
          else {
            auto iter = zgc_names.insert(connect_name);
            if (!iter.second) {
              // Name collision...
              for (char c = 'A'; c <= 'Z'; c++) {
                std::string potential = connect_name + c;
                iter                  = zgc_names.insert(potential);
                if (iter.second) {
                  connect_name = potential;
                  break;
                }
              }
              if (connect_name == zgc.m_connectionName) {
                bool done = false;
                for (char c1 = 'A'; c1 <= 'Z' && !done; c1++) {
                  for (char c2 = 'A'; c2 <= 'Z' && !done; c2++) {
                    std::string potential = connect_name + c1 + c2;
                    iter                  = zgc_names.insert(potential);
                    if (iter.second) {
                      connect_name = potential;
                      done         = true;
                    }
                  }
                }
                if (connect_name == zgc.m_connectionName) {
                  IOSS_ERROR(fmt::format(
                      "ERROR: CGNS: Duplicate ZGC Name '{}' on zone '{}', processor {}\n",
                      zgc.m_connectionName, sb->name(), zgc.m_ownerProcessor));
                }
              }
            }
          }
          donor_name += "_proc-";
          donor_name += std::to_string(zgc.m_donorProcessor);
          owner_range[0] -= zgc.m_ownerOffset[0];
          owner_range[1] -= zgc.m_ownerOffset[1];
          owner_range[2] -= zgc.m_ownerOffset[2];
          owner_range[3] -= zgc.m_ownerOffset[0];
          owner_range[4] -= zgc.m_ownerOffset[1];
          owner_range[5] -= zgc.m_ownerOffset[2];

          donor_range[0] -= zgc.m_donorOffset[0];
          donor_range[1] -= zgc.m_donorOffset[1];
          donor_range[2] -= zgc.m_donorOffset[2];
          donor_range[3] -= zgc.m_donorOffset[0];
          donor_range[4] -= zgc.m_donorOffset[1];
          donor_range[5] -= zgc.m_donorOffset[2];
        }

        if (is_parallel_io || !is_parallel) {
          if (zgc.m_ownerZone == zgc.m_donorZone && zgc.m_ownerRangeBeg == zgc.m_donorRangeBeg &&
              zgc.m_ownerRangeEnd == zgc.m_donorRangeEnd) {
#if IOSS_DEBUG_OUTPUT
            fmt::print("Removing ZGC {} on zone {}\n", connect_name, db_zone);
#endif
            continue;
          }
        }

        CGERR(cg_1to1_write(file_ptr, base, db_zone, connect_name.c_str(), donor_name.c_str(),
                            Data(owner_range), Data(donor_range), Data(zgc.m_transform), &zgc_idx));

        if (zgc.is_from_decomp()) {
          CGERR(cg_goto(file_ptr, base, "Zone_t", db_zone, "ZoneGridConnectivity", 0,
                        "GridConnectivity1to1_t", zgc_idx, "end"));
          CGERR(cg_descriptor_write("Decomp", "is_decomp"));
        }
        else if (original_name != connect_name) {
          CGERR(cg_goto(file_ptr, base, "Zone_t", db_zone, "ZoneGridConnectivity", 0,
                        "GridConnectivity1to1_t", zgc_idx, "end"));
          CGERR(cg_descriptor_write("OriginalName", original_name.c_str()));
        }
      }
    }
  }

  region.get_database()->progress("\tReturn from common_write_metadata");
  return element_count;
}

std::string Iocgns::Utils::map_cgns_to_topology_type(CGNS_ENUMT(ElementType_t) type)
{
  std::string topology = "unknown";
  switch (type) {
  case CGNS_ENUMV(NODE): topology = Ioss::Node::name; break;
  case CGNS_ENUMV(BAR_2): topology = Ioss::Beam2::name; break;
  case CGNS_ENUMV(BAR_3): topology = Ioss::Beam3::name; break;
  case CGNS_ENUMV(TRI_3): topology = Ioss::Tri3::name; break;
  case CGNS_ENUMV(TRI_6): topology = Ioss::Tri6::name; break;
  case CGNS_ENUMV(QUAD_4): topology = Ioss::Quad4::name; break;
  case CGNS_ENUMV(QUAD_8): topology = Ioss::Quad8::name; break;
  case CGNS_ENUMV(QUAD_9): topology = Ioss::Quad9::name; break;
  case CGNS_ENUMV(TETRA_4): topology = Ioss::Tet4::name; break;
  case CGNS_ENUMV(TETRA_10): topology = Ioss::Tet10::name; break;
  case CGNS_ENUMV(PYRA_5): topology = Ioss::Pyramid5::name; break;
  case CGNS_ENUMV(PYRA_13): topology = Ioss::Pyramid13::name; break;
  case CGNS_ENUMV(PYRA_14): topology = Ioss::Pyramid14::name; break;
  case CGNS_ENUMV(PENTA_6): topology = Ioss::Wedge6::name; break;
  case CGNS_ENUMV(PENTA_15): topology = Ioss::Wedge15::name; break;
  case CGNS_ENUMV(PENTA_18): topology = Ioss::Wedge18::name; break;
  case CGNS_ENUMV(HEXA_8): topology = Ioss::Hex8::name; break;
  case CGNS_ENUMV(HEXA_20): topology = Ioss::Hex20::name; break;
  case CGNS_ENUMV(HEXA_27): topology = Ioss::Hex27::name; break;
  default:
    fmt::print(Ioss::WarnOut(), "Found topology of type {} which is not currently supported.\n",
               cg_ElementTypeName(type));
    topology = Ioss::Unknown::name;
  }
  return topology;
}

CGNS_ENUMT(ElementType_t) Iocgns::Utils::map_topology_to_cgns(const std::string &name)
{
  CGNS_ENUMT(ElementType_t) topo = CGNS_ENUMV(ElementTypeNull);
  if (name == Ioss::Node::name) {
    topo = CGNS_ENUMV(NODE);
  }
  else if (name == Ioss::Spring2::name) {
    topo = CGNS_ENUMV(BAR_2);
  }
  else if (name == Ioss::Spring3::name) {
    topo = CGNS_ENUMV(BAR_3);
  }
  else if (name == Ioss::Beam2::name) {
    topo = CGNS_ENUMV(BAR_2);
  }
  else if (name == Ioss::Beam3::name) {
    topo = CGNS_ENUMV(BAR_3);
  }
  else if (name == Ioss::Tri3::name) {
    topo = CGNS_ENUMV(TRI_3);
  }
  else if (name == Ioss::Tri6::name) {
    topo = CGNS_ENUMV(TRI_6);
  }
  else if (name == Ioss::Quad4::name) {
    topo = CGNS_ENUMV(QUAD_4);
  }
  else if (name == Ioss::Quad8::name) {
    topo = CGNS_ENUMV(QUAD_8);
  }
  else if (name == Ioss::Quad9::name) {
    topo = CGNS_ENUMV(QUAD_9);
  }
  else if (name == Ioss::Tet4::name) {
    topo = CGNS_ENUMV(TETRA_4);
  }
  else if (name == Ioss::Tet10::name) {
    topo = CGNS_ENUMV(TETRA_10);
  }
  else if (name == Ioss::Pyramid5::name) {
    topo = CGNS_ENUMV(PYRA_5);
  }
  else if (name == Ioss::Pyramid13::name) {
    topo = CGNS_ENUMV(PYRA_13);
  }
  else if (name == Ioss::Pyramid14::name) {
    topo = CGNS_ENUMV(PYRA_14);
  }
  else if (name == Ioss::Wedge6::name) {
    topo = CGNS_ENUMV(PENTA_6);
  }
  else if (name == Ioss::Wedge15::name) {
    topo = CGNS_ENUMV(PENTA_15);
  }
  else if (name == Ioss::Wedge18::name) {
    topo = CGNS_ENUMV(PENTA_18);
  }
  else if (name == Ioss::Hex8::name) {
    topo = CGNS_ENUMV(HEXA_8);
  }
  else if (name == Ioss::Hex20::name) {
    topo = CGNS_ENUMV(HEXA_20);
  }
  else if (name == Ioss::Hex27::name) {
    topo = CGNS_ENUMV(HEXA_27);
  }
  else {
    fmt::print(Ioss::WarnOut(), "Found topology of type {} which is not currently supported.\n",
               name);
  }
  return topo;
}

void Iocgns::Utils::write_flow_solution_metadata(int file_ptr, int base_ptr, Ioss::Region *region,
                                                 int state, const int *vertex_solution_index,
                                                 const int *cell_center_solution_index,
                                                 bool       is_parallel_io)
{
  std::string c_name = fmt::format("CellCenterSolutionAtStep{:05}", state);
  std::string v_name = fmt::format("VertexSolutionAtStep{:05}", state);
  std::string step   = std::to_string(state);

  const auto &nblocks                 = region->get_node_blocks();
  const auto &nblock                  = nblocks[0];
  bool        global_has_nodal_fields = nblock->field_count(Ioss::Field::TRANSIENT) > 0;
  bool        is_file_per_state       = (base_ptr >= 0);

  // IF the `base_ptr` is positive, then we are in file-per-state option.
  // `file_ptr` points to the linked-to file where the state data is being
  // written and `base_ptr` points to the "base" file which has the mesh
  // metadata and links to the solution data "state" files.
  std::string linked_file_name;
  if (is_file_per_state) {
    linked_file_name = region->get_database()->get_filename();
  }

  // Create a lambda to avoid code duplication for similar treatment
  // of structured blocks and element blocks.
  auto sol_lambda = [=](Ioss::EntityBlock *block, bool has_nodal_fields) {
    int base = block->get_property("base").get_int();
    int zone = get_db_zone(block);
    if (has_nodal_fields) {
      if (is_file_per_state) {
        // We are using file-per-state and we need to add a link from the base file (base_ptr)
        // to the state file (file_ptr).
        CGERR(cg_goto(base_ptr, base, "Zone_t", zone, "end"));
        std::string linkpath = "/Base/" + block->name() + "/" + v_name;
        CGERR(cg_link_write(v_name.c_str(), linked_file_name.c_str(), linkpath.c_str()));
      }
      CGERR(cg_sol_write(file_ptr, base, zone, v_name.c_str(), CGNS_ENUMV(Vertex),
                         (int *)vertex_solution_index));
      CGERR(
          cg_goto(file_ptr, base, "Zone_t", zone, "FlowSolution_t", *vertex_solution_index, "end"));
      CGERR(cg_gridlocation_write(CGNS_ENUMV(Vertex)));
      CGERR(cg_descriptor_write("Step", step.c_str()));
    }
    if (block->field_count(Ioss::Field::TRANSIENT) > 0) {
      if (is_file_per_state) {
        // We are using file-per-state and we need to add a link from the base file (base_ptr)
        // to the state file (file_ptr).
        CGERR(cg_goto(base_ptr, base, "Zone_t", zone, "end"));
        std::string linkpath = "/Base/" + block->name() + "/" + c_name;
        CGERR(cg_link_write(c_name.c_str(), linked_file_name.c_str(), linkpath.c_str()));
      }
      CGERR(cg_sol_write(file_ptr, base, zone, c_name.c_str(), CGNS_ENUMV(CellCenter),
                         (int *)cell_center_solution_index));
      CGERR(cg_goto(file_ptr, base, "Zone_t", zone, "FlowSolution_t", *cell_center_solution_index,
                    "end"));
      CGERR(cg_descriptor_write("Step", step.c_str()));
    }
  };

  // Use the lambda
  const auto &sblocks = region->get_structured_blocks();
  for (auto &block : sblocks) {
    if (is_parallel_io || block->is_active()) {
      const auto &nb        = block->get_node_block();
      bool has_nodal_fields = global_has_nodal_fields || nb.field_count(Ioss::Field::TRANSIENT) > 0;
      sol_lambda(block, has_nodal_fields);
    }
  }
  // Use the lambda
  const auto &eblocks = region->get_element_blocks();
  for (auto &block : eblocks) {
    sol_lambda(block, global_has_nodal_fields);
  }
}

int Iocgns::Utils::find_solution_index(int cgns_file_ptr, int base, int zone, int step,
                                       CGNS_ENUMT(GridLocation_t) location)
{
  auto str_step = std::to_string(step);
  int  nsols    = 0;
  CGCHECKNP(cg_nsols(cgns_file_ptr, base, zone, &nsols));
  bool location_matches = false;
  for (int i = 0; i < nsols; i++) {
    CGNS_ENUMT(GridLocation_t) db_location;
    char db_name[CGNS_MAX_NAME_LENGTH + 1];
    CGCHECKNP(cg_sol_info(cgns_file_ptr, base, zone, i + 1, db_name, &db_location));
    if (location == db_location) {
      location_matches = true;
      // Check if steps match.
      // NOTE: Using non-standard "Descriptor_t" node in FlowSolution_t
      CGCHECKNP(cg_goto(cgns_file_ptr, base, "Zone_t", zone, "FlowSolution_t", i + 1, "end"));
      int descriptor_count = 0;
      CGCHECKNP(cg_ndescriptors(&descriptor_count));

      bool found_step_descriptor = false;
      for (int d = 0; d < descriptor_count; d++) {
        char *db_step = nullptr;
        char  name[CGNS_MAX_NAME_LENGTH + 1];
        CGCHECKNP(cg_descriptor_read(d + 1, name, &db_step));
        if (strcmp(name, "step") == 0) {
          found_step_descriptor = true;
          if (str_step == db_step) {
            cg_free(db_step);
            return i + 1;
          }

          cg_free(db_step);
          break; // Found "step" descriptor, but wasn't correct step...
        }
        cg_free(db_step);
      }
      if (!found_step_descriptor) {
        // There was no Descriptor_t node with the name "step",
        // Try to decode the step from the FlowSolution_t name.
        // If `db_name` does not have `Step` or `step` in name,
        // then don't search
        if (strcasestr(db_name, "step") != nullptr) {
          int nstep = extract_trailing_int(db_name);
          if (nstep == step) {
            return i + 1;
          }
        }
      }
    }
  }

  if (location_matches) {
    return step;
  }

  fmt::print(Ioss::WarnOut(),
             "CGNS: Could not find valid solution index for step {}, zone {}, and location {}\n",
             step, zone, cg_GridLocationName(location));
  return 0;
}

void Iocgns::Utils::add_sidesets(int cgns_file_ptr, Ioss::DatabaseIO *db)
{
  static int fake_id =
      std::numeric_limits<int>::max(); // Used in case CGNS file does not specify an id.

  int base         = 1;
  int num_families = 0;
  CGCHECKNP(cg_nfamilies(cgns_file_ptr, base, &num_families));

  for (int family = 1; family <= num_families; family++) {
    char name[CGNS_MAX_NAME_LENGTH + 1];
    CGNS_ENUMT(BCType_t) bocotype;
    int num_bc  = 0;
    int num_geo = 0;
    CGCHECKNP(cg_family_read(cgns_file_ptr, base, family, name, &num_bc, &num_geo));

#if IOSS_DEBUG_OUTPUT
    if (db->parallel_rank() == 0) {
      fmt::print(Ioss::DebugOut(), "Family {} named {} has {} BC, and {} geometry references.\n",
                 family, name, num_bc, num_geo);
    }
#endif
    if (num_bc > 0) {
      // Create a sideset...
      std::string ss_name(name); // Use name here before cg_fambc_read call overwrites it...

      CGCHECKNP(cg_fambc_read(cgns_file_ptr, base, family, 1, name, &bocotype));

      CGCHECKNP(cg_goto(cgns_file_ptr, base, "Family_t", family, "end"));
      int ndescriptors = 0;
      int id           = 0;
      CGCHECKNP(cg_ndescriptors(&ndescriptors));
      if (ndescriptors > 0) {
        for (int ndesc = 1; ndesc <= ndescriptors; ndesc++) {
          char  dname[CGNS_MAX_NAME_LENGTH + 1];
          char *dtext;
          CGCHECKNP(cg_descriptor_read(ndesc, dname, &dtext));
          if (strcmp(dname, "FamBC_UserId") == 0) {
            // Convert text in `dtext` to integer...
            id = Ioss::Utils::get_number(dtext);
            cg_free(dtext);
            break;
          }
          cg_free(dtext);
        }
      }
      if (id == 0) {
        id = Ioss::Utils::extract_id(ss_name);
        if (id == 0 && ss_name != "Unspecified") {
          // Assign a fake_id to this sideset.  No checking to make
          // sure there are no duplicates...
          id = fake_id--;
        }
      }
      if (id != 0) {
        auto *ss = new Ioss::SideSet(db, ss_name);
        ss->property_add(Ioss::Property("id", id));
        ss->property_add(Ioss::Property("guid", db->util().generate_guid(id)));
        ss->property_add(Ioss::Property("bc_type", bocotype));
        db->get_region()->add(ss);
      }
      else {
        if (db->parallel_rank() == 0) {
          fmt::print(Ioss::WarnOut(),
                     "Skipping BC with name '{}' since FamBC_UserId is equal to 0.\n\n", ss_name);
        }
      }
    }
  }
}

void Iocgns::Utils::add_assemblies(int cgns_file_ptr, Ioss::DatabaseIO *db)
{
  int base         = 1;
  int num_families = 0;
  CGCHECKNP(cg_nfamilies(cgns_file_ptr, base, &num_families));

  for (int family = 1; family <= num_families; family++) {
    char name[CGNS_MAX_NAME_LENGTH + 1];
    int  num_bc  = 0;
    int  num_geo = 0;
    CGCHECKNP(cg_family_read(cgns_file_ptr, base, family, name, &num_bc, &num_geo));

    if (num_bc == 0 && num_geo == 0) {
      // See if this is an assembly -- will contain a 'FamVC_UserName' Descriptor_t node
      // The `Node Data` for that node will be the name of the assembly.
      // Assemblies will be created empty and then blocks/zones will be added during
      // the parsing of the zones.
      CGCHECKNP(cg_goto(cgns_file_ptr, base, "Family_t", family, "end"));

      int ndescriptors = 0;
      CGCHECKNP(cg_ndescriptors(&ndescriptors));
      if (ndescriptors > 0) {
        int         id = -1;
        std::string assem_name;
        for (int ndesc = 1; ndesc <= ndescriptors; ndesc++) {
          char  dname[CGNS_MAX_NAME_LENGTH + 1];
          char *dtext = nullptr;
          CGCHECKNP(cg_descriptor_read(ndesc, dname, &dtext));
          if (strcmp(dname, "FamVC_UserId") == 0) {
            // Convert text in `dtext` to integer...
            id = Ioss::Utils::get_number(dtext);
          }
          else if (strcmp(dname, "FamVC_UserName") == 0) {
            assem_name = dtext;
          }
          cg_free(dtext);
        }
        if (!assem_name.empty() && assem_name != "Unspecified") {
          // Create an assembly with this name...
          auto *assem = new Ioss::Assembly(db, assem_name);
          db->get_region()->add(assem);
          if (id >= 0) {
            assem->property_add(Ioss::Property("id", id));
          }

#if IOSS_DEBUG_OUTPUT
          if (db->parallel_rank() == 0) {
            fmt::print(Ioss::DebugOut(),
                       "Adding Family {} named {} as an assembly named {} with id {}.\n", family,
                       name, assem_name, id);
          }
#endif
        }
      }
    }
  }
}

size_t Iocgns::Utils::resolve_nodes(Ioss::Region &region, int my_processor, bool is_parallel)
{
  // Each structured block has its own set of "cell_nodes"
  // At block boundaries, there are duplicate nodes which need to be resolved for the
  // unstructured mesh output.

  // We need to iterate all of the blocks and then each blocks zgc to determine
  // which nodes are shared between blocks. For all shared nodes, the node in the lowest
  // numbered zone is considered the "owner" and all other nodes are shared.

  // At the end of the routine, each block knows where its nodes fit
  // into the implicit ordering of nodes on this processor. This is
  // given by:
  // implicit_location = block->m_blockLocalNodeIndex[i] (0 <= i < #nodes_in_block)
  // Where 0 <= implicit_location < #nodes_on_processor

  // Create a vector of size which is the sum of the on-processor cell_nodes size for each block
  size_t num_total_cell_nodes = 0;
  auto  &blocks               = region.get_structured_blocks();
  for (auto &block : blocks) {
    size_t node_count = block->get_property("node_count").get_int();
    num_total_cell_nodes += node_count;
  }

  int64_t              ss_max = std::numeric_limits<int64_t>::max();
  std::vector<int64_t> cell_node_map(num_total_cell_nodes, ss_max);

  // Each cell_node location in the cell_node_map is currently initialized to ss_max.
  // Iterate each block and then each blocks non-intra-block (i.e., not
  // due to proc decomps) zgc instances and update cell_node_map
  // such that for each shared node, it points to the owner nodes
  // location.
  for (auto &owner_block : blocks) {
    for (const auto &zgc : owner_block->m_zoneConnectivity) {
      if (!zgc.is_from_decomp() &&
          zgc.is_active()) { // Not due to processor decomposition and has faces.
        // NOTE: In parallel, the owner block should exist, but may not have
        // any cells on this processor.  We can access its global i,j,k, but
        // don't store or access any "bulk" data on it.
        auto donor_block = region.get_structured_block(zgc.m_donorName);
        assert(donor_block != nullptr);

        std::vector<int> i_range = zgc.get_range(1);
        std::vector<int> j_range = zgc.get_range(2);
        std::vector<int> k_range = zgc.get_range(3);
        for (auto &k : k_range) {
          for (auto &j : j_range) {
            for (auto &i : i_range) {
              Ioss::IJK_t owner_index{{i, j, k}};
              Ioss::IJK_t donor_index = zgc.transform(owner_index);

              // The nodes as 'index' and 'owner' are contiguous and
              // should refer to the same node. 'owner' should be
              // the owner (unless it is already owned by another
              // block)

              int64_t owner_global_offset = owner_block->get_global_node_offset(owner_index);
              int64_t donor_global_offset = donor_block->get_global_node_offset(donor_index);

              if (owner_global_offset > donor_global_offset) {
                if (is_parallel && (zgc.m_donorProcessor != my_processor)) {
                  size_t owner_block_local_offset =
                      owner_block->get_block_local_node_offset(owner_index);
                  owner_block->m_globalIdMap.emplace_back(owner_block_local_offset,
                                                          donor_global_offset + 1);
                }
                else if (!is_parallel || (zgc.m_ownerProcessor != my_processor)) {
                  size_t  owner_local_offset = owner_block->get_local_node_offset(owner_index);
                  int64_t donor_local_offset = donor_block->get_local_node_offset(donor_index);

                  if (cell_node_map[owner_local_offset] == ss_max) {
                    cell_node_map[owner_local_offset] = donor_local_offset;
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  // Now iterate cell_node_map.  If an entry == ss_max, then it is
  // an owned node and needs to have its index into the unstructed
  // mesh node map set; otherwise, the value points to the owner
  // node, so the index at this location should be set to the owner
  // nodes index.
  size_t index = 0;
  for (auto &node : cell_node_map) {
    if (node == ss_max) {
      node = index++;
    }
    else {
      node = -node;
    }
  }

  for (auto &node : cell_node_map) {
    if (node < 0) {
      node = cell_node_map[-node];
    }
  }

  for (auto &block : blocks) {
    size_t node_count = block->get_property("node_count").get_int();
    block->m_blockLocalNodeIndex.resize(node_count);

    size_t beg = block->get_node_offset();
    size_t end = beg + node_count;
    for (size_t idx = beg, i = 0; idx < end; idx++) {
      block->m_blockLocalNodeIndex[i++] = cell_node_map[idx];
    }
  }
  return index;
}

std::vector<std::vector<std::pair<size_t, size_t>>>
Iocgns::Utils::resolve_processor_shared_nodes(Ioss::Region &region, int my_processor)
{
  // Determine which nodes are shared across processor boundaries.
  // Only need to check on block boundaries..

  // We need to iterate all of the blocks and then each blocks zgc to determine
  // which nodes are shared between processors. For all shared nodes, the node in the lowest
  // numbered zone is considered the "owner" and all other nodes are shared.

  auto &blocks = region.get_structured_blocks();

  std::vector<std::vector<std::pair<size_t, size_t>>> shared_nodes(blocks.size() + 1);

  for (auto &owner_block : blocks) {
    int owner_zone = owner_block->get_property("zone").get_int();
    for (const auto &zgc : owner_block->m_zoneConnectivity) {
      assert(zgc.m_donorProcessor >= 0);
      assert(zgc.m_ownerProcessor >= 0);

      if (zgc.is_active() &&
          (zgc.m_donorProcessor != my_processor || zgc.m_ownerProcessor != my_processor)) {
        // NOTE: In parallel, the donor block should exist, but may not have
        // any cells on this processor.  We can access its global i,j,k, but
        // don't store or access any "bulk" data on it.
        auto donor_block = region.get_structured_block(zgc.m_donorName);
        assert(donor_block != nullptr);
        int              donor_zone = donor_block->get_property("zone").get_int();
        std::vector<int> i_range    = zgc.get_range(1);
        std::vector<int> j_range    = zgc.get_range(2);
        std::vector<int> k_range    = zgc.get_range(3);
        for (auto &k : k_range) {
          for (auto &j : j_range) {
            for (auto &i : i_range) {
              Ioss::IJK_t owner_index{{i, j, k}};
              Ioss::IJK_t donor_index = zgc.transform(owner_index);

              // The nodes as 'index' and 'owner' are contiguous and
              // should refer to the same node.

              if (my_processor == zgc.m_ownerProcessor) {
                int64_t owner_offset = owner_block->get_block_local_node_offset(owner_index);
                shared_nodes[owner_zone].emplace_back(owner_offset, zgc.m_donorProcessor);
              }
              else if (my_processor == zgc.m_donorProcessor) {
                int64_t donor_offset = donor_block->get_block_local_node_offset(donor_index);
                shared_nodes[donor_zone].emplace_back(donor_offset, zgc.m_ownerProcessor);
              }
            }
          }
        }
      }
    }
#if IOSS_DEBUG_OUTPUT
    fmt::print(Ioss::DebugOut(), "P{}, Block {} Shared Nodes: {}\n", my_processor,
               owner_block->name(), shared_nodes[owner_zone].size());
#endif
  }
  return shared_nodes;
}

void Iocgns::Utils::add_to_assembly(int cgns_file_ptr, Ioss::Region *region,
                                    Ioss::EntityBlock *block, int base, int zone)
{
  // See if there is a 'FamilyName' node...
  if (cg_goto(cgns_file_ptr, base, "Zone_t", zone, "end") == CG_OK) {
    char name[CGNS_MAX_NAME_LENGTH + 1];
    if (cg_famname_read(name) == CG_OK) {
      auto *assem = region->get_assembly(name);
      if (assem != nullptr) {
        assem->add(block);
        block->property_add(Ioss::Property("assembly", assem->name()));
      }
    }
  }
}

void Iocgns::Utils::add_structured_boundary_conditions(int                    cgns_file_ptr,
                                                       Ioss::StructuredBlock *block,
                                                       bool                   is_parallel_io)
{
  // `is_parallel_io` is true if all processors reading single file.
  // `is_parallel_io` is false if serial, or each processor reading its own file (fpp)
  if (is_parallel_io) {
    add_structured_boundary_conditions_pio(cgns_file_ptr, block);
  }
  else {
    add_structured_boundary_conditions_fpp(cgns_file_ptr, block);
  }
}

void Iocgns::Utils::add_structured_boundary_conditions_pio(int                    cgns_file_ptr,
                                                           Ioss::StructuredBlock *block)
{
  int base = block->get_property("base").get_int();
  int zone = get_db_zone(block);

  // Called by Parallel run reading single file only.
  // Data needed:
  // * boco_name (CGNS_MAX_NAME_LENGTH + 1 chars)
  // * fam_name  (CGNS_MAX_NAME_LENGTH + 1 chars)
  // * data     (cgsize_t * 7) (bocotype + range[6])

  int num_bcs = 0;

  CGCHECKNP(cg_nbocos(cgns_file_ptr, base, zone, &num_bcs));

  std::vector<int>  bc_data(7 * num_bcs);
  std::vector<char> bc_names(2 * (CGNS_MAX_NAME_LENGTH + 1) * num_bcs);

  for (int ibc = 0; ibc < num_bcs; ibc++) {
    cgsize_t range[6];
    char     boco_name[CGNS_MAX_NAME_LENGTH + 1];
    char     fam_name[CGNS_MAX_NAME_LENGTH + 1];
    CGNS_ENUMT(BCType_t) bocotype;
    CGNS_ENUMT(PointSetType_t) ptset_type;
    cgsize_t npnts;
    cgsize_t NormalListSize;
    CGNS_ENUMT(DataType_t) NormalDataType;
    int ndataset;

    // All we really want from this is 'boco_name'
    CGCHECKNP(cg_boco_info(cgns_file_ptr, base, zone, ibc + 1, boco_name, &bocotype, &ptset_type,
                           &npnts, nullptr, &NormalListSize, &NormalDataType, &ndataset));

    if (bocotype == CGNS_ENUMV(FamilySpecified)) {
      // Get family name associated with this boco_name
      CGCHECKNP(
          cg_goto(cgns_file_ptr, base, "Zone_t", zone, "ZoneBC_t", 1, "BC_t", ibc + 1, "end"));
      CGCHECKNP(cg_famname_read(fam_name));
    }
    else {
      Ioss::Utils::copy_string(fam_name, boco_name);
    }

    CGCHECKNP(cg_boco_read(cgns_file_ptr, base, zone, ibc + 1, range, nullptr));

    // There are some BC that are applied on an edge or a vertex;
    // Don't want those (yet?), so filter them out at this time...
    {
      int same_count = (range[0] == range[3] ? 1 : 0) + (range[1] == range[4] ? 1 : 0) +
                       (range[2] == range[5] ? 1 : 0);
      if (same_count != 1) {
        fmt::print(Ioss::WarnOut(),
                   "CGNS: Skipping Boundary Condition '{}' on block '{}'. It is applied to "
                   "{}. This code only supports surfaces.\n",
                   boco_name, block->name(), (same_count == 2 ? "an edge" : "a vertex"));
        continue;
      }
    }

    bool is_parallel_io = true;
    add_bc_to_block(block, boco_name, fam_name, ibc, range, bocotype, is_parallel_io);
  }
}

void Iocgns::Utils::generate_boundary_faces(
    Ioss::Region *region, std::map<std::string, Ioss::FaceUnorderedSet> &boundary_faces,
    Ioss::Field::BasicType field_type)
{
  // See if we already generated the faces for this model...
  Ioss::FaceGenerator face_generator(*region);
  if (field_type == Ioss::Field::INT32) {
    face_generator.generate_faces((int)0, true);
  }
  else {
    face_generator.generate_faces((int64_t)0, true);
  }
  const Ioss::ElementBlockContainer &ebs = region->get_element_blocks();
  for (auto &eb : ebs) {
    const std::string &name     = eb->name();
    auto              &boundary = boundary_faces[name];
    auto              &faces    = face_generator.faces(name);
    for (auto &face : faces) {
      if (face.element_count() == 1) {
        boundary.insert(face);
      }
    }
  }
#if IOSS_DEBUG_OUTPUT
  output_table(ebs, boundary_faces);
#endif
}

void Iocgns::Utils::add_structured_boundary_conditions_fpp(int                    cgns_file_ptr,
                                                           Ioss::StructuredBlock *block)
{
  int base = block->get_property("base").get_int();
  int zone = get_db_zone(block);

  // Called by both parallel fpp and serial runs.
  // In parallel, the 'cgns_file_ptr' is specific for each processor

  int num_bcs = 0;
  CGCHECKNP(cg_nbocos(cgns_file_ptr, base, zone, &num_bcs));

  for (int ibc = 0; ibc < num_bcs; ibc++) {
    char boco_name[CGNS_MAX_NAME_LENGTH + 1];
    char fam_name[CGNS_MAX_NAME_LENGTH + 1];
    CGNS_ENUMT(BCType_t) bocotype;
    CGNS_ENUMT(PointSetType_t) ptset_type;
    cgsize_t npnts;
    cgsize_t NormalListSize;
    CGNS_ENUMT(DataType_t) NormalDataType;
    int      ndataset;
    cgsize_t range[6];

    // All we really want from this is 'boco_name'
    CGCHECKNP(cg_boco_info(cgns_file_ptr, base, zone, ibc + 1, boco_name, &bocotype, &ptset_type,
                           &npnts, nullptr, &NormalListSize, &NormalDataType, &ndataset));

    if (bocotype == CGNS_ENUMV(FamilySpecified)) {
      // Get family name associated with this boco_name
      CGCHECKNP(
          cg_goto(cgns_file_ptr, base, "Zone_t", zone, "ZoneBC_t", 1, "BC_t", ibc + 1, "end"));
      CGCHECKNP(cg_famname_read(fam_name));
    }
    else {
      Ioss::Utils::copy_string(fam_name, boco_name);
    }

    CGCHECKNP(cg_boco_read(cgns_file_ptr, base, zone, ibc + 1, range, nullptr));

    // There are some BC that are applied on an edge or a vertex;
    // Don't want those (yet?), so filter them out at this time...
    int same_count = (range[0] == range[3] ? 1 : 0) + (range[1] == range[4] ? 1 : 0) +
                     (range[2] == range[5] ? 1 : 0);
    if (same_count != 1) {
      fmt::print(Ioss::WarnOut(),
                 "CGNS: Skipping Boundary Condition '{}' on block '{}'. It is applied to "
                 "{}. This code only supports surfaces.\n",
                 boco_name, block->name(), (same_count == 2 ? "an edge" : "a vertex"));
      continue;
    }

    int num_proc = block->get_database()->util().parallel_size();
    if (num_proc > 1) {
      // Need to modify range with block offset to put into global space
      Ioss::IJK_t offset;
      offset[0] = block->get_property("offset_i").get_int();
      offset[1] = block->get_property("offset_j").get_int();
      offset[2] = block->get_property("offset_k").get_int();
      range[0] += offset[0];
      range[1] += offset[1];
      range[2] += offset[2];
      range[3] += offset[0];
      range[4] += offset[1];
      range[5] += offset[2];
    }

    bool is_parallel_io = false;
    add_bc_to_block(block, boco_name, fam_name, ibc, range, bocotype, is_parallel_io);
  }
}

void Iocgns::Utils::finalize_database(int cgns_file_ptr, const std::vector<double> &timesteps,
                                      Ioss::Region *region, int myProcessor, bool is_parallel_io)
{
  int base = 1;
  CGCHECK(cg_biter_write(cgns_file_ptr, base, "TimeIterValues", timesteps.size()));

  // Now write the timestep time values...
  CGCHECK(cg_goto(cgns_file_ptr, base, "BaseIterativeData_t", 1, "end"));
  cgsize_t dimtv[1] = {(cgsize_t)timesteps.size()};
  CGCHECK(cg_array_write("TimeValues", CGNS_ENUMV(RealDouble), 1, dimtv, Data(timesteps)));

  // Output the ZoneIterativeData which maps a zones flow solutions to timesteps.
  // One per zone and the number of entries matches the number of timesteps...
  const auto &nblocks = region->get_node_blocks();
  auto       &nblock  = nblocks[0];

  bool has_nodal_fields = nblock->field_count(Ioss::Field::TRANSIENT) > 0;

  // Create a lambda to avoid code duplication for similar treatment
  // of structured blocks and element blocks.
  auto ziter = [=](Ioss::EntityBlock *block) {
    int              zone = get_db_zone(block);
    std::vector<int> indices(timesteps.size());
    bool             has_cell_center_fields = block->field_count(Ioss::Field::TRANSIENT) > 0;
    std::string      base_type;
    if (has_nodal_fields && !has_cell_center_fields) {
      base_type = "VertexSolutionAtStep";
    }
    else if (!has_nodal_fields && has_cell_center_fields) {
      base_type = "CellCenterSolutionAtStep";
    }
    else {
      base_type = "SolutionAtStep";
    }

    std::vector<char> names(32 * timesteps.size(), ' ');
    for (size_t state = 0; state < timesteps.size(); state++) {
      // This name is the "postfix" or common portion of all FlowSolution names...
      std::string name = fmt::format("{}{:05}", base_type, state + 1);
      Ioss::Utils::copy_string(&names[state * 32], name, 32);
      for (size_t i = name.size(); i < 32; i++) {
        names[state * 32 + i] = ' ';
      }
    }

    cgsize_t dim[2] = {32, (cgsize_t)timesteps.size()};
    if (has_cell_center_fields || has_nodal_fields) {
      CGCHECK(cg_ziter_write(cgns_file_ptr, base, zone, "ZoneIterativeData"));
      CGCHECK(cg_goto(cgns_file_ptr, base, "Zone_t", zone, "ZoneIterativeData_t", 1, "end"));
      CGCHECK(cg_array_write("FlowSolutionPointers", CGNS_ENUMV(Character), 2, dim, Data(names)));

      if (has_nodal_fields) {
        int index     = 1;
        int increment = has_cell_center_fields ? 2 : 1;
        for (size_t state = 0; state < timesteps.size(); state++) {
          indices[state] = index;
          index += increment;
        }

        CGCHECK(cg_array_write("VertexSolutionIndices", CGNS_ENUMV(Integer), 1, &dim[1],
                               Data(indices)));
        CGCHECK(cg_descriptor_write("VertexPrefix", "Vertex"));
      }
      if (has_cell_center_fields) {
        int index     = has_nodal_fields ? 2 : 1;
        int increment = has_nodal_fields ? 2 : 1;
        for (size_t state = 0; state < timesteps.size(); state++) {
          indices[state] = index;
          index += increment;
        }

        CGCHECK(
            cg_array_write("CellCenterIndices", CGNS_ENUMV(Integer), 1, &dim[1], Data(indices)));
        CGCHECK(cg_descriptor_write("CellCenterPrefix", "CellCenter"));
      }
    }
  };

  // Use the lambda...
  const auto &sblocks = region->get_structured_blocks();
  for (auto &block : sblocks) {
    if (is_parallel_io || block->is_active()) {
      ziter(block);
    }
  }

  // Use the lambda...
  const auto &eblocks = region->get_element_blocks();
  for (auto &block : eblocks) {
    ziter(block);
  }
}

void Iocgns::Utils::add_transient_variables(int cgns_file_ptr, const std::vector<double> &timesteps,
                                            Ioss::Region *region, int myProcessor,
                                            bool is_parallel_io)
{
  // ==========================================
  // Add transient variables (if any) to all zones...
  // Create a lambda to avoid code duplication for similar treatment
  // of structured blocks and element blocks.

  // Assuming that the fields on all steps are the same, but can vary
  // from zone to zone.
  auto sol_iter = [=](Ioss::EntityBlock *block) {
    int b = block->get_property("base").get_int();
    int z = get_db_zone(block);

    int sol_count = 0;
    CGCHECK(cg_nsols(cgns_file_ptr, b, z, &sol_count));
    int sol_per_step = sol_count / (int)timesteps.size();
    assert(sol_count % (int)timesteps.size() == 0);

    for (int sol = 1; sol <= sol_per_step; sol++) {
      char solution_name[CGNS_MAX_NAME_LENGTH + 1];
      CGNS_ENUMT(GridLocation_t) grid_loc;
      CGCHECK(cg_sol_info(cgns_file_ptr, b, z, sol, solution_name, &grid_loc));

      int field_count = 0;
      CGCHECK(cg_nfields(cgns_file_ptr, b, z, sol, &field_count));

      Ioss::NameList field_names;
      field_names.reserve(field_count);
      for (int field = 1; field <= field_count; field++) {
        CGNS_ENUMT(DataType_t) data_type;
        char field_name[CGNS_MAX_NAME_LENGTH + 1];
        CGCHECK(cg_field_info(cgns_file_ptr, b, z, sol, field, &data_type, field_name));
        field_names.emplace_back(field_name);
      }

      // Convert raw field names into composite fields (a_x, a_y, a_z ==> 3D vector 'a')
      std::vector<Ioss::Field> fields;
      if (grid_loc == CGNS_ENUMV(CellCenter)) {
        size_t entity_count = block->entity_count();
        Ioss::Utils::get_fields(entity_count, field_names, Ioss::Field::TRANSIENT,
                                region->get_database(), nullptr, fields);
        size_t index = 1;
        for (const auto &field : fields) {
          Utils::set_field_index(field, index, grid_loc);
          index += field.raw_storage()->component_count();
          block->field_add(field);
        }
      }
      else {
        assert(grid_loc == CGNS_ENUMV(Vertex));
        const Ioss::NodeBlock *cnb =
            (block->type() == Ioss::STRUCTUREDBLOCK)
                ? &(dynamic_cast<Ioss::StructuredBlock *>(block)->get_node_block())
                : region->get_node_blocks()[0];
        auto *nb = const_cast<Ioss::NodeBlock *>(cnb);
        if (nb == nullptr) {
          IOSS_ERROR(
              fmt::format("ERROR: CGNS: Null entity accessing nodeblock for structured block {}.",
                          block->name()));
        }
        size_t entity_count = nb->entity_count();
        Ioss::Utils::get_fields(entity_count, field_names, Ioss::Field::TRANSIENT,
                                region->get_database(), nullptr, fields);
        size_t index = 1;
        for (const auto &field : fields) {
          Utils::set_field_index(field, index, grid_loc);
          index += field.raw_storage()->component_count();
          nb->field_add(field);
        }
      }
    }
  };
  // ==========================================

  if (!timesteps.empty()) {
    const auto &sblocks = region->get_structured_blocks();
    for (auto &block : sblocks) {
      if (is_parallel_io || block->is_active()) {
        sol_iter(block);
      }
    }
    const auto &eblocks = region->get_element_blocks();
    for (auto &block : eblocks) {
      sol_iter(block);
    }
    bool is_parallel = region->get_database()->util().parallel_size() > 1;
    if (is_parallel && !is_parallel_io) {
      sync_transient_variables_fpp(region);
    }
  }
}

int Iocgns::Utils::get_step_times(int cgns_file_ptr, std::vector<double> &timesteps,
                                  Ioss::Region *region, double timeScaleFactor, int myProcessor)
{
  int  base          = 1;
  int  num_timesteps = 0;
  char bitername[CGNS_MAX_NAME_LENGTH + 1];
  int  ierr = cg_biter_read(cgns_file_ptr, base, bitername, &num_timesteps);
  if (ierr == CG_NODE_NOT_FOUND) {
    return num_timesteps;
  }
  if (ierr == CG_ERROR) {
    Utils::cgns_error(cgns_file_ptr, __FILE__, __func__, __LINE__, myProcessor);
  }

  if (num_timesteps <= 0) {
    return num_timesteps;
  }

  // Read the timestep time values.
  CGCHECK(cg_goto(cgns_file_ptr, base, "BaseIterativeData_t", 1, "end"));
  std::vector<double> times(num_timesteps);
  CGCHECK(cg_array_read_as(1, CGNS_ENUMV(RealDouble), Data(times)));

  timesteps.reserve(num_timesteps);
  for (int i = 0; i < num_timesteps; i++) {
    if (nullptr != region) {
      region->add_state(times[i] * timeScaleFactor);
    }
    timesteps.push_back(times[i]);
  }
  return num_timesteps;
}

void Iocgns::Utils::set_line_decomposition(int cgns_file_ptr, const std::string &line_decomposition,
                                           std::vector<Iocgns::StructuredZoneData *> &zones,
                                           int rank, bool verbose)
{
  // The "line_decomposition" string is a list of 0 or more BC
  // (Family) names.  For all structured zones which this BC
  // touches, the ordinal of the face (i,j,k) will be set such that
  // a parallel decomposition will not split the zone along this
  // ordinal.  For example, if the BC "wall1" has the definition
  // [1->1, 1->5, 1->8], then it is on the constant 'i' face of the
  // zone and therefore, the zone will *not* be split along the 'i'
  // ordinal.

  // Get names of all valid 'bcs' on the mesh
  int base         = 1;
  int num_families = 0;
  CGCHECKNP(cg_nfamilies(cgns_file_ptr, base, &num_families));

  Ioss::NameList families;
  families.reserve(num_families);
  for (int family = 1; family <= num_families; family++) {
    char name[CGNS_MAX_NAME_LENGTH + 1];
    int  num_bc  = 0;
    int  num_geo = 0;
    CGCHECKNP(cg_family_read(cgns_file_ptr, base, family, name, &num_bc, &num_geo));
    if (num_bc > 0) {
      Ioss::Utils::fixup_name(name);
      families.emplace_back(name);
    }
  }

  // Slit into fields using the commas as delimiters
  auto bcs = Ioss::tokenize(line_decomposition, ",");
  for (auto &bc : bcs) {
    Ioss::Utils::fixup_name(bc);
    if (std::find(families.begin(), families.end(), bc) == families.end()) {
      std::ostringstream errmsg;
      fmt::print(errmsg,
                 "ERROR: CGNS: The family/bc name '{}' specified as a line decomposition surface "
                 "does not exist on this CGNS file.\n"
                 "             Valid names are: ",
                 bc);
      for (const auto &fam : families) {
        fmt::print(errmsg, "'{}', ", fam);
      }
      IOSS_ERROR(errmsg);
    }
  }

  for (auto &zone : zones) {
    // Read BCs applied to this zone and see if they match any of
    // the BCs in 'bcs' list.  If so, determine the face the BC is
    // applied to and set the m_lineOrdinal to the ordinal
    // perpendicular to this face.
    int izone = zone->m_zone;
    int num_bcs;
    CGCHECKNP(cg_nbocos(cgns_file_ptr, base, izone, &num_bcs));

    for (int ibc = 0; ibc < num_bcs; ibc++) {
      char boconame[CGNS_MAX_NAME_LENGTH + 1];
      CGNS_ENUMT(BCType_t) bocotype;
      CGNS_ENUMT(PointSetType_t) ptset_type;
      cgsize_t npnts;
      cgsize_t NormalListSize;
      CGNS_ENUMT(DataType_t) NormalDataType;
      int ndataset;

      // All we really want from this is 'boconame'
      CGCHECKNP(cg_boco_info(cgns_file_ptr, base, izone, ibc + 1, boconame, &bocotype, &ptset_type,
                             &npnts, nullptr, &NormalListSize, &NormalDataType, &ndataset));

      if (bocotype == CGNS_ENUMV(FamilySpecified)) {
        // Need to get boconame from cg_famname_read
        CGCHECKNP(
            cg_goto(cgns_file_ptr, base, "Zone_t", izone, "ZoneBC_t", 1, "BC_t", ibc + 1, "end"));
        CGCHECKNP(cg_famname_read(boconame));
      }

      Ioss::Utils::fixup_name(boconame);
      if (std::find(bcs.begin(), bcs.end(), boconame) != bcs.end()) {
        cgsize_t range[6];
        CGCHECKNP(cg_boco_read(cgns_file_ptr, base, izone, ibc + 1, range, nullptr));

        // There are some BC that are applied on an edge or a vertex;
        // Don't want those, so filter them out at this time...
        bool i = range[0] == range[3];
        bool j = range[1] == range[4];
        bool k = range[2] == range[5];

        int sum = (i ? 1 : 0) + (j ? 1 : 0) + (k ? 1 : 0);
        // Only set m_lineOrdinal if only a single ordinal selected.
        if (sum == 1) {
          unsigned int ordinal = 0;
          if (i) {
            ordinal = Ordinal::I;
          }
          else if (j) {
            ordinal = Ordinal::J;
          }
          else if (k) {
            ordinal = Ordinal::K;
          }
          zone->m_lineOrdinal |= ordinal;
          if (verbose && rank == 0) {
            fmt::print(Ioss::DebugOut(), "Setting line ordinal to {} on {} for surface: {}\n",
                       zone->m_lineOrdinal, zone->m_name, boconame);
            if (zone->m_lineOrdinal == 7) {
              fmt::print(Ioss::DebugOut(),
                         "NOTE: Zone {} with work {} will not be decomposed due to line ordinal "
                         "setting.\n",
                         zone->m_name, fmt::group_digits(zone->work()));
            }
          }
        }
      }
    }
  }
}

void Iocgns::Utils::decompose_model(std::vector<Iocgns::StructuredZoneData *> &zones,
                                    int proc_count, int rank, double load_balance_threshold,
                                    bool verbose)
{
  size_t work = 0;
  for (const auto &z : zones) {
    work += z->work();
    assert(z->is_active());
  }

  double avg_work = (double)work / proc_count;

  if (verbose) {
    auto num_active = zones.size();
    if (rank == 0) {
      fmt::print(
          Ioss::OUTPUT(),
          "Decomposing structured mesh with {} zones for {} processors.\nAverage workload is {}, "
          "Load Balance Threshold is {}, Work range {} to {}\n",
          num_active, proc_count, fmt::group_digits((size_t)avg_work), load_balance_threshold,
          fmt::group_digits((size_t)(avg_work / load_balance_threshold)),
          fmt::group_digits((size_t)(avg_work * load_balance_threshold)));
    }
  }

  if (avg_work < 1.0) {
    IOSS_ERROR(
        fmt::format("ERROR: Model size too small to distribute over {} processors.\n", proc_count));
  }

  if (verbose) {
    if (rank == 0) {
      fmt::print(Ioss::DebugOut(),
                 "========================================================================\n");
      fmt::print(Ioss::DebugOut(), "Pre-Splitting: (Average = {}, LB Threshold = {}\n",
                 fmt::group_digits((size_t)avg_work), load_balance_threshold);
    }
  }
  // Split all blocks where block->work() > avg_work * load_balance_threshold
  int new_zone_id =
      Utils::pre_split(zones, avg_work, load_balance_threshold, rank, proc_count, verbose);

  // At this point, there should be no zone with block->work() > avg_work * load_balance_threshold
  if (verbose) {
    if (rank == 0) {
      fmt::print(Ioss::DebugOut(),
                 "========================================================================\n");
    }
  }
  size_t num_split = 0;
  size_t px        = 0;
  do {
    std::vector<size_t> work_vector(proc_count);
    Utils::assign_zones_to_procs(zones, work_vector, verbose);

    // Calculate workload ratio for each processor...
    px = 0; // Number of processors where workload ratio exceeds threshold.
    std::vector<bool> exceeds(proc_count);
    for (size_t i = 0; i < work_vector.size(); i++) {
      double workload_ratio = double(work_vector[i]) / avg_work;
      if (workload_ratio > load_balance_threshold) {
        exceeds[i] = true;
        px++;
        if (verbose && rank == 0) {
          fmt::print(Ioss::DebugOut(), "{}",
                     fmt::format(
#if !defined __NVCC__
                         fg(fmt::color::red),
#endif
                         "\nProcessor {} work: {}, workload ratio: {} (exceeds)", i,
                         fmt::group_digits(work_vector[i]), workload_ratio));
        }
      }
      else {
        if (verbose && rank == 0) {
          fmt::print(Ioss::DebugOut(), "\nProcessor {} work: {}, workload ratio: {}", i,
                     fmt::group_digits(work_vector[i]), workload_ratio);
        }
      }
    }
    if (verbose && rank == 0) {
      fmt::print(Ioss::DebugOut(), "\n\nWorkload threshold exceeded on {} processors.\n", px);
    }
    bool single_zone = zones.size() == 1;
    if (single_zone) { // GDS: Don't understand this code...  !single_zone?
      auto active = std::count_if(zones.begin(), zones.end(),
                                  [](Iocgns::StructuredZoneData *a) { return a->is_active(); });
      if (active >= proc_count) {
        px = 0;
      }
    }
    num_split = 0;
    if (px > 0) {
      auto zone_new(zones);
      for (auto &zone : zones) {
        if (zone->is_active() && exceeds[zone->m_proc]) {
          // Since 'zones' is sorted from most work to least,
          // we just iterate zones and check whether the zone
          // is on a proc where the threshold was exceeded.
          // if so, split the block and set exceeds[proc] to false;
          // Exit the loop when num_split >= px.
          auto children = zone->split(new_zone_id, zone->work() / 2.0, rank, verbose);
          if (children.first != nullptr && children.second != nullptr) {
            zone_new.push_back(children.first);
            zone_new.push_back(children.second);

            new_zone_id += 2;
            exceeds[zone->m_proc] = false;
            num_split++;
            if (num_split >= px) {
              break;
            }
          }
        }
      }
      std::swap(zone_new, zones);
    }
    if (verbose) {
      auto active = std::count_if(zones.begin(), zones.end(),
                                  [](Iocgns::StructuredZoneData *a) { return a->is_active(); });
      if (rank == 0) {
        fmt::print(Ioss::DebugOut(), "Number of active zones = {}, average work = {}\n", active,
                   fmt::group_digits((size_t)avg_work));
        fmt::print(Ioss::DebugOut(),
                   "========================================================================\n");
      }
    }
  } while (px > 0 && num_split > 0);
}

void Iocgns::Utils::assign_zones_to_procs(std::vector<Iocgns::StructuredZoneData *> &all_zones,
                                          std::vector<size_t> &work_vector, bool verbose)
{
  for (auto &zone : all_zones) {
    zone->m_proc = -1;
  }

  // Sort zones based on work.  Most work first.. Filtered to active only...
  std::vector<Iocgns::StructuredZoneData *> zones;
  std::copy_if(all_zones.begin(), all_zones.end(), std::back_inserter(zones),
               [](Iocgns::StructuredZoneData *z) { return z->is_active(); });

  Ioss::sort(zones.begin(), zones.end(),
             [](Iocgns::StructuredZoneData *a, Iocgns::StructuredZoneData *b) {
               return a->work() > b->work();
             });

  std::set<std::pair<int, int>> proc_adam_map;

  // On first entry, work_vector will be all zeros.  To avoid any
  // searching, assign the first `nproc` zones to the `nproc` entries
  // in `work_vector`.  Avoids searching...
  if (zones.size() < work_vector.size()) {
    IOSS_ERROR(fmt::format(
        "IOCGNS error: Could not decompose mesh across {} processors based on constraints.",
        work_vector.size()));
  }
  assert(zones.size() >= work_vector.size());
  size_t i = 0;
  for (; i < work_vector.size(); i++) {
    auto &zone   = zones[i];
    zone->m_proc = i;
    if (verbose) {
      fmt::print(Ioss::DebugOut(),
                 "Assigning zone '{}' with work {} to processor {}. Changing work from {} to {}\n",
                 zone->m_name, fmt::group_digits(zone->work()), zone->m_proc,
                 fmt::group_digits(work_vector[i]),
                 fmt::group_digits(zone->work() + work_vector[i]));
    }
    work_vector[i] += zone->work();
    proc_adam_map.insert(std::make_pair(zone->m_adam->m_zone, zone->m_proc));
  }

  for (; i < zones.size(); i++) {
    auto &zone = zones[i];

    // Assign zone to processor with minimum work that does not already have a zone with the same
    // adam zone...
    int proc = proc_with_minimum_work(zone, work_vector, proc_adam_map);

    // See if any other zone on this processor has the same adam zone...
    if (proc >= 0) {
      auto success = proc_adam_map.insert(std::make_pair(zone->m_adam->m_zone, proc));
      if (success.second) {
        zone->m_proc = proc;
        if (verbose) {
          fmt::print(Ioss::DebugOut(),
                     "Assigning zone '{}' with work {} to processor {}. Changing work from {} "
                     "to {}\n",
                     zone->m_name, fmt::group_digits(zone->work()), zone->m_proc,
                     fmt::group_digits(work_vector[proc]),
                     fmt::group_digits(zone->work() + work_vector[proc]));
        }
        work_vector[proc] += zone->work();
      }
      else {
        IOSS_ERROR(
            fmt::format("IOCGNS error: Could not assign zones to processors in {}", __func__));
      }
    }
    else {
      IOSS_ERROR(fmt::format("IOCGNS error: Could not assign zones to processors in {}", __func__));
    }
  }
}

int Iocgns::Utils::pre_split(std::vector<Iocgns::StructuredZoneData *> &zones, double avg_work,
                             double load_balance, int proc_rank, int proc_count, bool verbose)
{
  auto original_zones(zones); // In case we need to call this again...

  auto new_zones(zones);
  int  new_zone_id = static_cast<int>(zones.size()) + 1;

  // See if can split each zone over a set of procs...
  std::vector<int> splits(zones.size(), 1);

  if ((int)zones.size() < proc_count) {
    for (size_t i = 0; i < zones.size(); i++) {
      auto zone = zones[i];
      if (zone->m_lineOrdinal != 7) {
        double work = zone->work();
        if (load_balance <= 1.2) {
          splits[i] = int(std::ceil(work / avg_work));
        }
        else {
          splits[i] = int(std::round(work / avg_work + 0.2));
        }
        splits[i] = splits[i] == 0 ? 1 : splits[i];
      }
    }
  }

  int  num_splits        = std::accumulate(splits.begin(), splits.end(), 0);
  int  diff              = proc_count - num_splits;
  bool adjustment_needed = diff > 0;

  if (num_splits != (int)zones.size()) {
    while (diff != 0) {
      // Adjust splits so sum is equal to proc_count.
      // Adjust the largest split count(s)
      int    step      = diff < 0 ? -1 : 1;
      size_t min_z     = 0;
      double min_delta = 1.0e27;
      for (size_t i = 0; i < zones.size(); i++) {
        auto   zone = zones[i];
        double work = zone->work();

        if (splits[i] == 0) {
          continue;
        }
        if ((splits[i] + step) > 0) {
          double delta = std::abs(avg_work - work / (double)(splits[i] + step));
          if (delta < min_delta) {
            min_delta = delta;
            min_z     = i;
          }
        }
      }
      splits[min_z] += step;
      diff -= step;
    }
    assert(diff == 0);
    assert(std::accumulate(splits.begin(), splits.end(), 0) == proc_count);
  }

  // See if splits result in avg_work for all zones in range...
  double min_avg      = avg_work / load_balance;
  double max_avg      = avg_work * load_balance;
  bool   adaptive_avg = true;
  if (!adjustment_needed) {
    for (size_t i = 0; i < zones.size(); i++) {
      auto   zone = zones[i];
      double work = zone->work();
      if (splits[i] == 0) {
        adaptive_avg = false;
        break;
      }
      double zone_avg = work / (double)splits[i];
      if (zone_avg < min_avg || zone_avg > max_avg) {
        adaptive_avg = false;
        break;
      }
    }
  }

  if (adaptive_avg) {
    for (size_t i = 0; i < zones.size(); i++) {
      auto zone = zones[i];

      auto work_average = avg_work;
      int  split_cnt    = splits[i];
      if (split_cnt == 1) {
        continue;
      }

      std::vector<std::pair<int, Iocgns::StructuredZoneData *>> active;
      active.emplace_back(split_cnt, zone);
      int num_active = 0;
      do {
        assert(!active.empty());
        split_cnt = active.back().first;
        zone      = active.back().second;
        active.pop_back();

        if (zone->is_active()) {
          if (split_cnt != 1) {
            int max_power_2 = power_2(split_cnt);
            if (max_power_2 == split_cnt) {
              work_average = zone->work() / 2.0;
              max_power_2 /= 2;
            }
            else {
              work_average = zone->work() / (double(split_cnt) / double(max_power_2));
            }

            auto children = zone->split(new_zone_id, work_average, proc_rank, verbose);
            if (children.first != nullptr && children.second != nullptr) {
              new_zones.push_back(children.first);
              new_zones.push_back(children.second);
              new_zone_id += 2;
              active.emplace_back(split_cnt - max_power_2, children.second);
              active.emplace_back(max_power_2, children.first);
              num_active++;
            }
          }
        }
        if (num_active >=
            proc_count) { // Don't split a single zone into more than `proc_count` pieces
          break;
        }
      } while (!active.empty());
    }
  }
  else {
    for (auto &zone : zones) {
      if (zone->work() <= max_avg) {
        // This zone is already in `new_zones`; just skip doing anything else with it.
      }
      else {
        std::vector<std::pair<int, Iocgns::StructuredZoneData *>> active;

        double work       = zone->work();
        int    split_cnt  = int(work / avg_work);
        int    num_active = 0;

        // Find modulus of work % avg_work and split off that amount
        // which will be < avg_work.
        double mod_work = work - avg_work * split_cnt;
        if (mod_work > max_avg - avg_work) {
          auto children = zone->split(new_zone_id, mod_work, proc_rank, verbose);
          if (children.first != nullptr && children.second != nullptr) {
            new_zones.push_back(children.first);
            new_zones.push_back(children.second);
            new_zone_id += 2;
            num_active++;
            active.emplace_back(split_cnt, children.second);
          }
          else {
            active.emplace_back(split_cnt, zone);
          }
        }
        else {
          active.emplace_back(split_cnt, zone);
        }

        // The work remaining on this zone should be approximately
        // equally divided among `split_cnt` processors.
        do {
          assert(!active.empty());
          split_cnt = active.back().first;
          zone      = active.back().second;
          active.pop_back();

          if (zone->is_active()) {
            int    max_power_2  = power_2(split_cnt);
            double work_average = 0.0;
            if (max_power_2 == split_cnt) {
              work_average = zone->work() / 2.0;
            }
            else {
              work_average = zone->work() / (double(split_cnt) / double(max_power_2));
            }

            if (max_power_2 != 1) {
              if (max_power_2 == split_cnt) {
                max_power_2 /= 2;
              }
              auto children = zone->split(new_zone_id, work_average, proc_rank, verbose);
              if (children.first != nullptr && children.second != nullptr) {
                new_zones.push_back(children.first);
                new_zones.push_back(children.second);
                new_zone_id += 2;
                active.emplace_back(split_cnt - max_power_2, children.second);
                active.emplace_back(max_power_2, children.first);
                num_active++;
              }
            }
          }
          if (num_active >=
              proc_count) { // Don't split a single zone into more than `proc_count` pieces
            break;
          }
        } while (!active.empty());
      }
    }
  }
  std::swap(new_zones, zones);
  size_t active = std::count_if(zones.begin(), zones.end(),
                                [](const Iocgns::StructuredZoneData *z) { return z->is_active(); });

  if (active < (size_t)proc_count && load_balance > 1.05) {
    // Tighten up the load_balance factor to get some decomposition going...
    double new_load_balance = (1.0 + load_balance) / 2.0;

    // If any of the original zones were split the first time we called this routine,
    // we need to delete the zones created via splitting.
    // Also reset the parent zone to not have any children...
    for (auto &zone : zones) {
      if (!zone->is_active()) {
        zone->m_child1 = nullptr;
        zone->m_child2 = nullptr;
      }
      if (zone->m_adam != zone) {
        // Created via a split; delete...
        delete zone;
      }
    }

    // Revert `zones` back to original version (with no zones split)
    zones       = std::move(original_zones);
    new_zone_id = pre_split(zones, avg_work, new_load_balance, proc_rank, proc_count, verbose);
  }
  return new_zone_id;
}

std::vector<Iocgns::ZoneBC> Iocgns::Utils::parse_zonebc_sideblocks(int cgns_file_ptr, int base,
                                                                   int zone, int myProcessor)
{
  int num_bc;
  CGCHECK(cg_nbocos(cgns_file_ptr, base, zone, &num_bc));

  std::vector<Iocgns::ZoneBC> zonebc;
  zonebc.reserve(num_bc);

  for (int i = 0; i < num_bc; i++) {
    char boco_name[CGNS_MAX_NAME_LENGTH + 1];
    CGNS_ENUMT(BCType_t) boco_type;
    CGNS_ENUMT(PointSetType_t) ptset_type;
    cgsize_t num_pnts;
    cgsize_t normal_list_size;               // ignore
    CGNS_ENUMT(DataType_t) normal_data_type; // ignore
    int num_dataset;                         // ignore
    CGCHECK(cg_boco_info(cgns_file_ptr, base, zone, i + 1, boco_name, &boco_type, &ptset_type,
                         &num_pnts, nullptr, &normal_list_size, &normal_data_type, &num_dataset));

    if (num_pnts != 2 || ptset_type != CGNS_ENUMV(PointRange)) {
      IOSS_ERROR(fmt::format(
          "CGNS: In Zone {}, boundary condition '{}' has a PointSetType of '{}' and {} points.\n"
          "      The type must be 'PointRange' and there must be 2 points.",
          zone, boco_name, cg_PointSetTypeName(ptset_type), num_pnts));
    }

    std::array<cgsize_t, 2> point_range;
    CGCHECK(cg_boco_read(cgns_file_ptr, base, zone, i + 1, Data(point_range), nullptr));
    zonebc.emplace_back(boco_name, point_range);
  }
  return zonebc;
}

#ifdef CG_BUILD_HDF5
/* extern "C" int H5get_libversion(unsigned *, unsigned *, unsigned *); */
#endif

std::string Iocgns::Utils::show_config()
{
  std::stringstream config;
  fmt::print(config, "\tCGNS Library Version: {}\n", CGNS_DOTVERS);
#if CG_BUILD_64BIT
  fmt::print(config, "\t\tDefault integer size is 64-bit.\n");
#else
  fmt::print(config, "\t\tDefault integer size is 32-bit.\n");
#endif
#if defined(CGNS_SCOPE_ENUMS)
  fmt::print(config, "\t\tScoped Enums enabled\n");
#else
  fmt::print(config, "\t\tScoped Enums NOT enabled\n");
#endif
#if defined(CG_COMPACT)
  fmt::print(config, "\t\tCompact Storage enabled\n");
#else
  fmt::print(config, "\t\tCompact Storage NOT enabled\n");
#endif
#if CG_BUILD_PARALLEL
  fmt::print(config, "\t\tParallel enabled\n");
#else
  fmt::print(config, "\t\tParallel NOT enabled\n");
#endif
#if CG_BUILD_HDF5
  unsigned major;
  unsigned minor;
  unsigned release;
  H5get_libversion(&major, &minor, &release);
  fmt::print(config, "\t\tHDF5 enabled ({}.{}.{})\n", major, minor, release);
#else
#error "Not defined..."
#endif
#if HDF5_HAVE_COLL_METADATA
  fmt::print(config, "\t\tUsing HDF5 Collective Metadata.\n");
#else
  fmt::print(config, "\t\tHDF5 Collective Metadata NOT Available.\n");
#endif
#if HDF5_HAVE_MULTI_DATASET
  fmt::print(config, "\t\tHDF5 Multi-Dataset Available.\n\n");
#else
  fmt::print(config, "\t\tHDF5 Multi-Dataset NOT Available.\n\n");
#endif
  return config.str();
}

namespace {
  void create_face(Ioss::FaceUnorderedSet &faces, size_t id, std::array<size_t, 4> &conn,
                   size_t element, int local_face)
  {
    Ioss::Face face(id, conn);
    auto       face_iter = faces.insert(face);

    (*(face_iter.first)).add_element(element * 10 + local_face);
  }
} // namespace

template <typename INT>
void Iocgns::Utils::generate_block_faces(Ioss::ElementTopology *topo, size_t num_elem,
                                         const std::vector<INT> &connectivity,
                                         Ioss::FaceUnorderedSet &boundary,
                                         const std::vector<INT> &zone_local_zone_global)
{
  // Only handle continuum elements at this time...
  if (topo->parametric_dimension() != 3) {
    return;
  }

  int num_face_per_elem = topo->number_faces();
  assert(num_face_per_elem <= 6);
  std::array<Ioss::IntVector, 6> face_conn;
  std::array<int, 6>             face_node_count{};
  for (int face = 0; face < num_face_per_elem; face++) {
    face_conn[face]       = topo->face_connectivity(face + 1);
    face_node_count[face] = topo->face_type(face + 1)->number_corner_nodes();
  }

  Ioss::FaceUnorderedSet all_faces;
  int                    num_node_per_elem = topo->number_nodes();
  for (size_t elem = 0, offset = 0; elem < num_elem; elem++, offset += num_node_per_elem) {
    for (int face = 0; face < num_face_per_elem; face++) {
      size_t id = 0;
      assert(face_node_count[face] <= 4);
      std::array<size_t, 4> conn = {{0, 0, 0, 0}};
      for (int j = 0; j < face_node_count[face]; j++) {
        size_t fnode = offset + face_conn[face][j];
        size_t lnode = connectivity[fnode]; // local since "connectivity_raw"
        conn[j]      = lnode;
        id += Ioss::FaceGenerator::id_hash(lnode);
      }
      auto elem_id = zone_local_zone_global[elem];
      create_face(all_faces, id, conn, elem_id, face);
    }
  }

  // All faces generated for this element block; now extract boundary faces...
  for (auto &face : all_faces) {
    if (face.element_count() == 1) {
      boundary.insert(face);
    }
  }
}

template void Iocgns::Utils::generate_block_faces<int>(
    Ioss::ElementTopology *topo, size_t num_elem, const std::vector<int> &connectivity,
    Ioss::FaceUnorderedSet &boundary, const std::vector<int> &zone_local_zone_global);
template void Iocgns::Utils::generate_block_faces<int64_t>(
    Ioss::ElementTopology *topo, size_t num_elem, const std::vector<int64_t> &connectivity,
    Ioss::FaceUnorderedSet &boundary, const std::vector<int64_t> &zone_local_zone_global);
