// Copyright(C) 1999-2022 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include <Ioss_DatabaseIO.h>
#include <Ioss_ElementBlock.h>
#include <Ioss_ElementTopology.h>
#include <Ioss_EntityBlock.h>
#include <Ioss_Field.h>
#include <Ioss_Property.h>
#include <Ioss_SideBlock.h>
#include <cassert>
#include <cstddef>
#include "vtk_fmt.h"
#include VTK_FMT(fmt/ostream.h)
#include <string>
#include <tokenize.h>
#include <vector>

#include "Ioss_FieldManager.h"
#include "Ioss_ParallelUtils.h"
#include "Ioss_PropertyManager.h"

/** \brief Create a side block.
 *
 *  \param[in] io_database The database associated with the region containing the side block.
 *  \param[in] my_name The side block's name.
 *  \param[in] side_type The name of the side topology type for the side block.
 *  \param[in] element_type The name of the element topology of the parent element type for the side
 * block.
 *  \param[in] side_count The number of sides in the side block.
 */
Ioss::SideBlock::SideBlock(Ioss::DatabaseIO *io_database, const std::string &my_name,
                           const std::string &side_type, const std::string &element_type,
                           size_t side_count)
    : Ioss::EntityBlock(io_database, my_name, side_type, side_count)
{
  parentTopology_ = ElementTopology::factory(element_type);
  assert(parentTopology_ != nullptr);

  properties.add(Ioss::Property(this, "parent_topology_type", Ioss::Property::STRING));

  properties.add(Ioss::Property(this, "distribution_factor_count", Ioss::Property::INTEGER));

  fields.add(Ioss::Field("element_side", field_int_type(), "pair", Ioss::Field::MESH, side_count));

  // Same as element_side except that the element id are the local
  // element position (1-based) and not the global element id.
  fields.add(
      Ioss::Field("element_side_raw", field_int_type(), "pair", Ioss::Field::MESH, side_count));

  // Distribution factors are optional...
}

Ioss::SideBlock::SideBlock(const Ioss::SideBlock &other)
    : EntityBlock(other), parentTopology_(other.parentTopology_),
      consistentSideNumber(other.consistentSideNumber)
{
}

std::string Ioss::SideBlock::generate_sideblock_name(const std::string &sideset_name,
                                                     const std::string &block_or_element,
                                                     const std::string &face_topology_name)
{
  // The naming of sideblocks is:
  // * If name is of form surface_{id},
  //   * then {surface} + _ + block-or-element-topology + _ + side_topology + _ + {id}
  //   * Eg – surface_1 would have sideblocks surface_block_1_quad_1
  //
  // * If name is not of that form (e.g. surface_1_foam or “gregs_liner”) then:
  //   * Name + _ + block-or-element-topology + _ + side_topology
  //   * Eg surface_1_foam_block_1_edge2, surface_1_foam_quad4_edge2
  //   * Eg gregs_liner_block_1_edge2, gregs_liner_quad4_edge2

  // Check whether the `block_or_element` portion of the name names a valid element topology...
  std::string tmp_block_or_element = block_or_element;
  auto       *element_topology     = ElementTopology::factory(block_or_element);
  if (element_topology != nullptr) {
    tmp_block_or_element = element_topology->name();
  }

  // Verify that `face_topology_name` is a valid topology and Get its "non-aliased" name.
  std::string tmp_face_topology = face_topology_name;
  auto       *face_topology     = ElementTopology::factory(face_topology_name);
  if (face_topology != nullptr) {
    tmp_face_topology = face_topology->name();
  }
  else {
    std::ostringstream errmsg;
    fmt::print(errmsg, "ERROR: Invalid face topology '{}' in function {}.\n", face_topology_name,
               __func__);
    IOSS_ERROR(errmsg);
  }

  auto tokens = Ioss::tokenize(sideset_name, "_");
  if (tokens.size() == 2) {
    bool all_dig = tokens[1].find_first_not_of("0123456789") == std::string::npos;
    if (all_dig && Ioss::Utils::str_equal(tokens[0], "surface")) {
      std::string sideblock_name =
          tokens[0] + "_" + tmp_block_or_element + "_" + tmp_face_topology + "_" + tokens[1];
      return sideblock_name;
    }
  }

  std::string sideblock_name = sideset_name + "_" + tmp_block_or_element + "_" + tmp_face_topology;
  return sideblock_name;
}

int64_t Ioss::SideBlock::internal_get_field_data(const Ioss::Field &field, void *data,
                                                 size_t data_size) const
{
  return get_database()->get_field(this, field, data, data_size);
}

int64_t Ioss::SideBlock::internal_put_field_data(const Ioss::Field &field, void *data,
                                                 size_t data_size) const
{
  return get_database()->put_field(this, field, data, data_size);
}

Ioss::Property Ioss::SideBlock::get_implicit_property(const std::string &my_name) const
{
  if (my_name == "distribution_factor_count") {
    if (field_exists("distribution_factors")) {
      int64_t nnodes = topology()->number_nodes();
      int64_t nside  = entity_count();
      return Ioss::Property(my_name, nnodes * nside);
    }
    return Ioss::Property(my_name, 0);
  }
  if (my_name == "parent_topology_type") {
    return Ioss::Property(my_name, parent_element_topology()->name());
  }

  return Ioss::EntityBlock::get_implicit_property(my_name);
}

void Ioss::SideBlock::block_membership(std::vector<std::string> &block_members)
{
  // Simplest case.  If the surfaces are split by element block, then this will
  // return non-null
  // and we are done.
  const Ioss::EntityBlock *eb = parent_block();
  if (eb != nullptr) {
    block_members.push_back(eb->name());
    return;
  }

  if (blockMembership.empty()) {
    get_database()->compute_block_membership(this, blockMembership);
  }
  block_members = blockMembership;
}

namespace {
  template <typename INT> int internal_consistent_side_number(std::vector<INT> &element_side)
  {
    size_t ecount = element_side.size();
    int    side   = ecount > 0 ? element_side[1] : 0;
    for (size_t i = 3; i < ecount; i += 2) {
      int this_side = element_side[i];
      if (this_side != side) {
        side = 999; // Indicates the sides are not consistent ;
        break;
      }
    }
    return side;
  }
} // namespace

int Ioss::SideBlock::get_consistent_side_number() const
{
  if (consistentSideNumber == -1) {
    // It wasn't calculated during the metadata reading of the surfaces.
    // Determine it now...
    if (field_exists("element_side")) {
      int side = 0;
      if (get_database()->int_byte_size_api() == 8) {
        std::vector<int64_t> element_side;
        get_field_data("element_side", element_side);
        side = internal_consistent_side_number(element_side);
      }
      else {
        std::vector<int> element_side;
        get_field_data("element_side", element_side);
        side = internal_consistent_side_number(element_side);
      }

      int side_max = get_database()->util().global_minmax(side, Ioss::ParallelUtils::DO_MAX);
      if (side_max != 999) {
        consistentSideNumber = side_max;
      }
      else {
        consistentSideNumber = 0;
      }
    }
    else {
      consistentSideNumber = 0;
    }
  }
  return consistentSideNumber;
}

bool Ioss::SideBlock::equal_(const Ioss::SideBlock &rhs, bool quiet) const
{
  if (this->parentTopology_ != rhs.parentTopology_) {
    if (!quiet) {
      fmt::print(Ioss::OUTPUT(), "SideBlock: parentTopology_ mismatch\n");
    }
    return false;
  }

  if (this->blockMembership != rhs.blockMembership) {
    if (!quiet) {
      fmt::print(Ioss::OUTPUT(), "SideBlock: blockMembership mismatch\n");
    }
    return false;
  }

  if (this->consistentSideNumber != rhs.consistentSideNumber) {
    if (!quiet) {
      fmt::print(Ioss::OUTPUT(), "SideBlock: consistentSideNumber mismatch ({} vs. {})\n",
                 this->consistentSideNumber, rhs.consistentSideNumber);
    }
    return false;
  }

  if (!quiet) {
    return Ioss::EntityBlock::equal(rhs);
  }
  else {
    return Ioss::EntityBlock::operator==(rhs);
  }
}

bool Ioss::SideBlock::operator==(const Ioss::SideBlock &rhs) const { return equal_(rhs, true); }

bool Ioss::SideBlock::operator!=(const Ioss::SideBlock &rhs) const { return !(*this == rhs); }

bool Ioss::SideBlock::equal(const Ioss::SideBlock &rhs) const { return equal_(rhs, false); }
