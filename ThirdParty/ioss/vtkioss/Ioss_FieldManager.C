// Copyright(C) 1999-2022 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_Field.h"
#include "Ioss_FieldManager.h"
#include "Ioss_Sort.h"
#include "Ioss_Utils.h"
#include <cassert>
#include <cstddef>
#include <string>

#include "Ioss_CodeTypes.h"
#include "robin_hash.h"

/** \brief Add a field to the field manager.
 *
 *  Assumes that a field with the same name does not already exist.
 *
 *  \param[in] new_field The field to add
 *
 */
void Ioss::FieldManager::add(const Ioss::Field &new_field)
{
  std::string key = Ioss::Utils::lowercase(new_field.get_name());
  if (!exists(key)) {
    IOSS_FUNC_ENTER(m_);
    fields.insert(FieldValuePair(key, new_field));
  }
}

/** \brief Checks if a field with a given name exists in the field manager.
 *
 * \param[in] field_name The name of the field to check for.
 * \returns True if the field exists in the field manager.
 *
 */
bool Ioss::FieldManager::exists(const std::string &field_name) const
{
  IOSS_FUNC_ENTER(m_);
  const std::string key = Ioss::Utils::lowercase(field_name);
  return (fields.find(key) != fields.end());
}

/** \brief Get a field from the field manager.
 *
 *  \param[in] field_name The name of the field to get.
 *  \returns The field object.
 *
 */
Ioss::Field Ioss::FieldManager::get(const std::string &field_name) const
{
  IOSS_FUNC_ENTER(m_);
  const std::string key  = Ioss::Utils::lowercase(field_name);
  auto              iter = fields.find(key);
  assert(iter != fields.end());
  return (*iter).second;
}

/** \brief Get a reference to a field from the field manager.
 *
 *  \param[in] field_name The name of the field to get.
 *  \returns A reference to the field object.
 *
 */
const Ioss::Field &Ioss::FieldManager::getref(const std::string &field_name) const
{
  IOSS_FUNC_ENTER(m_);
  const std::string key  = Ioss::Utils::lowercase(field_name);
  auto              iter = fields.find(key);
  assert(iter != fields.end());
  return (*iter).second;
}

/** \brief Remove a field from the field manager.
 *
 * Assumes that a field with the given name exists in the field manager.
 *
 * \param[in] field_name The name of the field to remove.
 */
void Ioss::FieldManager::erase(const std::string &field_name)
{
  assert(exists(field_name));
  IOSS_FUNC_ENTER(m_);
  const std::string key  = Ioss::Utils::lowercase(field_name);
  auto              iter = fields.find(key);
  if (iter != fields.end()) {
    fields.erase(iter);
  }
}

/** \brief Remove all fields of type `role` from the field manager.
 *
 * \param[in] role Remove all fields (if any) of type `role`
 */
void Ioss::FieldManager::erase(Field::RoleType role)
{
  auto names = describe(role);
  IOSS_FUNC_ENTER(m_);

  for (const auto &field_name : names) {
    const std::string key  = Ioss::Utils::lowercase(field_name);
    auto              iter = fields.find(key);
    if (iter != fields.end()) {
      fields.erase(iter);
    }
  }
}

/** \brief Get the names of all fields in the field manager.
 *
 * \returns names All field names in the field manager.
 *
 */
Ioss::NameList Ioss::FieldManager::describe() const
{
  Ioss::NameList names;
  describe(&names);
  return names;
}

/** \brief Get the names of all fields in the field manager.
 *
 * \param[out] names All field names in the field manager.
 * \returns The number of fields extracted from the field manager.
 *
 */
int Ioss::FieldManager::describe(NameList *names) const
{
  IOSS_FUNC_ENTER(m_);
  int the_count = 0;
  for (const auto &field : fields) {
    names->push_back(field.second.get_name());
    the_count++;
  }
  if (the_count > 0) {
    Ioss::sort(names->begin(), names->end());
  }
  return the_count;
}

/** \brief Get the names of all fields of a specified RoleType in the field manager.
 *
 * \param[in] role The role type (MESH, ATTRIBUTE, TRANSIENT, REDUCTION, etc.)
 * \returns names All field names of the specified RoleType in the field manager.
 *
 */
Ioss::NameList Ioss::FieldManager::describe(Ioss::Field::RoleType role) const
{
  Ioss::NameList names;
  describe(role, &names);
  return names;
}

/** \brief Get the names of all fields of a specified RoleType in the field manager.
 *
 * \param[in] role The role type (MESH, ATTRIBUTE, TRANSIENT, REDUCTION, etc.)
 * \param[out] names All field names of the specified RoleType in the field manager.
 * \returns The number of fields extracted from the field manager.
 *
 */
int Ioss::FieldManager::describe(Ioss::Field::RoleType role, NameList *names) const
{
  IOSS_FUNC_ENTER(m_);
  int the_count = 0;
  for (const auto &field : fields) {
    if (field.second.get_role() == role) {
      names->push_back(field.second.get_name());
      the_count++;
    }
  }
  if (the_count > 0) {
    Ioss::sort(names->begin(), names->end());
  }
  return the_count;
}

/** \brief Get the number of fields in the field manager.
 *
 *  \returns The number of fields in the field manager.
 */
size_t Ioss::FieldManager::count() const
{
  IOSS_FUNC_ENTER(m_);
  return fields.size();
}
