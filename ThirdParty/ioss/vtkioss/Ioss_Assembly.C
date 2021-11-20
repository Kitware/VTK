// Copyright(C) 1999-2021 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include <Ioss_Assembly.h>
#include <Ioss_DatabaseIO.h>
#include <Ioss_GroupingEntity.h>
#include <Ioss_Property.h>
#include <Ioss_PropertyManager.h>
#include <Ioss_Region.h>
#include <algorithm>
#include <cstddef>
#include <fmt/ostream.h>
#include <string>
#include <vector>

namespace {
  const std::string id_str() { return std::string("id"); }
  void              check_is_valid(const Ioss::Assembly *assem, const Ioss::GroupingEntity *member)
  {
    // Ensure that `member` is not already a member and that its type matches
    // the current type.
    const std::string &name = member->name();

    // Don't add an assembly to itself...
    if (assem == member) {
      std::ostringstream errmsg;
      fmt::print(errmsg,
                 "\nERROR: Attempting to add assembly '{}' to itself.  This is not allowed.", name);
      IOSS_ERROR(errmsg);
    }

    // See if there is a member with this name...
    const Ioss::GroupingEntity *old_ge = assem->get_member(name);

    if (old_ge != nullptr) {
      std::string        filename = assem->get_database()->get_filename();
      int64_t            id1      = member->get_optional_property(id_str(), 0);
      int64_t            id2      = old_ge->get_optional_property(id_str(), 0);
      std::ostringstream errmsg;
      fmt::print(errmsg,
                 "\nERROR: There are multiple assembly members named '{}' "
                 "defined in assembly '{}' in the database file '{}'.\n"
                 "\tBoth {} {} and {} {} are named '{}'.  All names must be unique.",
                 name, assem->name(), filename, member->type_string(), id1, old_ge->type_string(),
                 id2, name);
      IOSS_ERROR(errmsg);
    }

    if (assem->member_count() > 0) {
      if (member->type() != assem->get_member_type()) {
        std::ostringstream errmsg;
        std::string        filename = assem->get_database()->get_filename();
        fmt::print(errmsg,
                   "\nERROR: The entity type of '{}' ({}) does not match the entity type of "
                   "assembly '{}' ({}).\n\tAn assembly's member entities must be "
                   "homogeneous. In the database file '{}'.\n",
                   member->name(), member->type_string(), assem->name(), assem->contains_string(),
                   filename);
        IOSS_ERROR(errmsg);
      }
    }
  }
} // namespace

namespace Ioss {
  class Field;
} // namespace Ioss

/** \brief Create an assembly with no members initially.
 *
 *  \param[in] io_database The database associated with the region containing the assembly.
 *  \param[in] my_name The assembly's name.
 */
Ioss::Assembly::Assembly(Ioss::DatabaseIO *io_database, const std::string &my_name)
    : Ioss::GroupingEntity(io_database, my_name, 1)
{
  properties.add(Ioss::Property(this, "member_count", Ioss::Property::INTEGER));
  properties.add(Ioss::Property(this, "member_type", Ioss::Property::INTEGER));
}

Ioss::Assembly::Assembly(const Ioss::Assembly &other)
    : GroupingEntity(other), m_members(other.m_members), m_type(other.m_type)
{
}

const Ioss::EntityContainer &Ioss::Assembly::get_members() const { return m_members; }

const Ioss::GroupingEntity *Ioss::Assembly::get_member(const std::string &my_name) const
{
  IOSS_FUNC_ENTER(m_);
  const Ioss::GroupingEntity *ge = nullptr;
  for (const auto &mem : m_members) {
    if (mem->name() == my_name) {
      ge = mem;
      break;
    }
  }
  return ge;
}

bool Ioss::Assembly::add(const Ioss::GroupingEntity *member)
{
  check_is_valid(this, member);
  IOSS_FUNC_ENTER(m_);
  m_members.push_back(member);
  if (m_members.size() == 1) {
    m_type = member->type();
  }
  return true;
}

void Ioss::Assembly::remove_members()
{
  IOSS_FUNC_ENTER(m_);
  m_members.clear();
  m_members.shrink_to_fit();
}

bool Ioss::Assembly::remove(const Ioss::GroupingEntity *removal)
{
  IOSS_FUNC_ENTER(m_);
  for (size_t i = 0; i < m_members.size(); i++) {
    if (m_members[i] == removal) {
      m_members.erase(m_members.begin() + i);
      return true;
    }
  }
  return false;
}

int64_t Ioss::Assembly::internal_get_field_data(const Ioss::Field &field, void *data,
                                                size_t data_size) const
{
  return get_database()->get_field(this, field, data, data_size);
}

int64_t Ioss::Assembly::internal_put_field_data(const Ioss::Field &field, void *data,
                                                size_t data_size) const
{
  return get_database()->put_field(this, field, data, data_size);
}

Ioss::Property Ioss::Assembly::get_implicit_property(const std::string &my_name) const
{
  if (my_name == "member_count") {
    return Ioss::Property(my_name, static_cast<int>(m_members.size()));
  }
  if (my_name == "member_type") {
    return Ioss::Property(my_name, static_cast<int>(m_type));
  }

  return Ioss::GroupingEntity::get_implicit_property(my_name);
}
