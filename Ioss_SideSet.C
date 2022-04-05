// Copyright(C) 1999-2021 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include <Ioss_DatabaseIO.h>
#include <Ioss_ElementTopology.h>
#include <Ioss_Property.h>
#include <Ioss_Region.h>
#include <Ioss_SideBlock.h>
#include <Ioss_SideSet.h>
#include <algorithm>
#include <cstddef>
#include "vtk_fmt.h"
#include VTK_FMT(fmt/ostream.h)
#include <string>
#include <vector>

#include "Ioss_GroupingEntity.h"
#include "Ioss_PropertyManager.h"

namespace {
  std::string id_str() { return std::string("id"); }
  void check_for_duplicate_names(const Ioss::SideSet *sset, const Ioss::SideBlock *side_block)
  {
    const std::string &name = side_block->name();

    // See if there is a side_block with this name...
    const Ioss::SideBlock *old_ge = sset->get_side_block(name);

    if (old_ge != nullptr) {
      std::string        filename = sset->get_database()->get_filename();
      std::ostringstream errmsg;
      int64_t            id1 = side_block->get_optional_property(id_str(), 0);
      int64_t            id2 = old_ge->get_optional_property(id_str(), 0);
      fmt::print(errmsg,
                 "\nERROR: There are multiple side blocks with the same name "
                 "defined in side set '{}' in the database file '{}'.\n"
                 "\tBoth {} {} and {} {} are named '{}'.  All names must be unique.",
                 sset->name(), filename, side_block->type_string(), id1, old_ge->type_string(), id2,
                 name);
      IOSS_ERROR(errmsg);
    }
  }
} // namespace

namespace Ioss {
  class Field;
} // namespace Ioss

/** \brief Create a side set with no members initially.
 *
 *  \param[in] io_database The database associated with the region containing the side set.
 *  \param[in] my_name The side set's name.
 */
Ioss::SideSet::SideSet(Ioss::DatabaseIO *io_database, const std::string &my_name)
    : Ioss::GroupingEntity(io_database, my_name, 0)
{
  properties.add(Ioss::Property(this, "side_block_count", Ioss::Property::INTEGER));
  properties.add(Ioss::Property(this, "block_count", Ioss::Property::INTEGER));
}

Ioss::SideSet::SideSet(const Ioss::SideSet &other) : Ioss::GroupingEntity(other)
{
  for (const auto &block : other.sideBlocks) {
    auto *sb = new Ioss::SideBlock(*block);
    add(sb);
  }
}

Ioss::SideSet::~SideSet()
{
  try {
    for (auto &sb : sideBlocks) {
      delete sb;
    }
  }
  catch (...) {
  }
}

const Ioss::SideBlockContainer &Ioss::SideSet::get_side_blocks() const { return sideBlocks; }

Ioss::SideBlock *Ioss::SideSet::get_block(size_t which) const
{
  IOSS_FUNC_ENTER(m_);
  if (which < sideBlocks.size()) {
    return sideBlocks[which];
  }

  return nullptr;
}

Ioss::SideBlock *Ioss::SideSet::get_side_block(const std::string &my_name) const
{
  IOSS_FUNC_ENTER(m_);
  Ioss::SideBlock *ge = nullptr;
  for (auto &sb : sideBlocks) {
    if (sb->name() == my_name) {
      ge = sb;
      break;
    }
  }
  return ge;
}

bool Ioss::SideSet::add(Ioss::SideBlock *side_block)
{
  check_for_duplicate_names(this, side_block);
  IOSS_FUNC_ENTER(m_);
  sideBlocks.push_back(side_block);
  side_block->owner_ = this;
  return true;
}

int64_t Ioss::SideSet::internal_get_field_data(const Ioss::Field &field, void *data,
                                               size_t data_size) const
{
  return get_database()->get_field(this, field, data, data_size);
}

int64_t Ioss::SideSet::internal_put_field_data(const Ioss::Field &field, void *data,
                                               size_t data_size) const
{
  return get_database()->put_field(this, field, data, data_size);
}

Ioss::Property Ioss::SideSet::get_implicit_property(const std::string &my_name) const
{
  if (my_name == "side_block_count") {
    return Ioss::Property(my_name, static_cast<int>(sideBlocks.size()));
  }
  if (my_name == "block_count") {
    return Ioss::Property(my_name, static_cast<int>(sideBlocks.size()));
  }

  return Ioss::GroupingEntity::get_implicit_property(my_name);
}

int Ioss::SideSet::max_parametric_dimension() const
{
  IOSS_FUNC_ENTER(m_);
  int max_par_dim = 0;
  for (auto &sideblock : sideBlocks) {
    int parametric_dim = sideblock->topology()->parametric_dimension();
    if (parametric_dim > max_par_dim) {
      max_par_dim = parametric_dim;
    }
  }
  if (max_par_dim == 0) {
    // If the sideset is empty, return the maximum that the parametric dimension
    // could be...
    // Faces for 3D model; Edges for 2D model
    const Ioss::Region *reg = get_database()->get_region();
    max_par_dim             = reg->get_property("spatial_dimension").get_int() - 1;
  }
  return max_par_dim;
}

void Ioss::SideSet::block_membership(std::vector<std::string> &block_members)
{
  IOSS_FUNC_ENTER(m_);
  if (blockMembership.empty()) {
    for (auto &sb : sideBlocks) {
      std::vector<std::string> blocks;
      sb->block_membership(blocks);
      blockMembership.insert(blockMembership.end(), blocks.begin(), blocks.end());
    }
    Ioss::Utils::uniquify(blockMembership);
  }
  block_members = blockMembership;
}

bool Ioss::SideSet::equal_(const SideSet &rhs, const bool /* quiet */) const
{
  std::vector<SideBlock *> lhs_side_blocks = this->sideBlocks;
  std::vector<SideBlock *> rhs_side_blocks = rhs.sideBlocks;

  // COMPARE SideBlocks
  for (auto &lhs_side_block : lhs_side_blocks) {
    std::vector<SideBlock *>::iterator it;
    for (it = rhs_side_blocks.begin(); it != rhs_side_blocks.end(); it++) {
      if ((*(*it)).operator==(*lhs_side_block))
        break;
    }

    if (it == rhs_side_blocks.end()) {
      // NO match for this side block
      return false;
    }

    rhs_side_blocks.erase(it);
  }

  // COMPARE block membership
  std::vector<std::string> lhs_block_membership = this->blockMembership;
  std::vector<std::string> rhs_block_membership = rhs.blockMembership;

  for (auto &lhs_block_member : lhs_block_membership) {
    std::vector<std::string>::iterator it;
    for (it = rhs_block_membership.begin(); it != rhs_block_membership.end(); it++) {
      if ((*it).compare(lhs_block_member) == 0)
        break;
    }

    if (it == rhs_block_membership.end()) {
      // NO match for this side block
      return false;
    }

    rhs_block_membership.erase(it);
  }

  return true;
}

bool Ioss::SideSet::operator==(const SideSet &rhs) const { return equal_(rhs, false); }

bool Ioss::SideSet::operator!=(const SideSet &rhs) const { return !(*this == rhs); }

bool Ioss::SideSet::equal(const SideSet &rhs) const { return equal_(rhs, true); }
