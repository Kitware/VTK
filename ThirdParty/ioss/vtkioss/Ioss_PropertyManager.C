// Copyright(C) 1999-2020 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include <Ioss_Property.h>
#include <Ioss_PropertyManager.h>
#include <Ioss_Utils.h>
#include <cstddef>
#include <fmt/ostream.h>
#include <map>
#include <ostream>
#include <string>
#include <utility>

Ioss::PropertyManager::PropertyManager(const PropertyManager &from)
    : m_properties(from.m_properties)
{
}

Ioss::PropertyManager::~PropertyManager()
{
  try {
    m_properties.clear();
  }
  catch (...) {
  }
}

/** \brief Add a property to the property manager.
 *
 *  \param[in] new_prop The property to add.
 */
void Ioss::PropertyManager::add(const Ioss::Property &new_prop)
{
  IOSS_FUNC_ENTER(m_);
  auto iter = m_properties.find(new_prop.get_name());
  if (iter != m_properties.end()) {
    m_properties.erase(iter);
  }
  m_properties.insert(ValuePair(new_prop.get_name(), new_prop));
}

/** \brief Checks if a property exists in the database.
 *
 *  \param[in] property_name The property to check
 *  \returns True if the property exists, false otherwise.
 */
bool Ioss::PropertyManager::exists(const std::string &property_name) const
{
  return (m_properties.find(property_name) != m_properties.end());
}

/** \brief Get a property object from the property manager.
 *
 *  \param[in] property_name The name of the property to get.
 *  \returns The property object.
 */
Ioss::Property Ioss::PropertyManager::get(const std::string &property_name) const
{
  IOSS_FUNC_ENTER(m_);
  auto iter = m_properties.find(property_name);
  if (iter == m_properties.end()) {
    std::ostringstream errmsg;
    fmt::print(errmsg, "ERROR: Could not find property '{}'\n", property_name);
    IOSS_ERROR(errmsg);
  }
  return (*iter).second;
}

/** \brief Get an optional property object from the property manager.
 *
 *  \param[in] property_name The name of the property to get.
 *  \param[in] optional_value The value to return if the property does not exist.
 *  \returns The property object.
 */
int64_t Ioss::PropertyManager::get_optional(const std::string &property_name,
                                            int64_t            optional_value) const
{
  IOSS_FUNC_ENTER(m_);
  auto iter = m_properties.find(property_name);
  if (iter == m_properties.end()) {
    return optional_value;
  }
  return (*iter).second.get_int();
}

std::string Ioss::PropertyManager::get_optional(const std::string &property_name,
                                                const std::string &optional_value) const
{
  IOSS_FUNC_ENTER(m_);
  auto iter = m_properties.find(property_name);
  if (iter == m_properties.end()) {
    return optional_value;
  }
  return (*iter).second.get_string();
}

/** \brief Remove a property from the property manager.
 *
 *  Assumes that the property with the given name already exists in the property manager.
 *
 *  \param[in] property_name The name of the property to remove.
 *
 */
void Ioss::PropertyManager::erase(const std::string &property_name)
{
  IOSS_FUNC_ENTER(m_);
  auto iter = m_properties.find(property_name);
  if (iter != m_properties.end()) {
    m_properties.erase(iter);
  }
}

/** \brief Get the names of all properties in the property manager
 *
 *  \param[out] names All the property names in the property manager.
 *  \returns The number of properties extracted from the property manager.
 */
int Ioss::PropertyManager::describe(NameList *names) const
{
  IOSS_FUNC_ENTER(m_);
  int                         the_count = 0;
  PropMapType::const_iterator I;
  for (I = m_properties.begin(); I != m_properties.end(); ++I) {
    names->push_back((*I).first);
    the_count++;
  }
  return the_count;
}

/** \brief Get the names of all properties in the property manager that have the origin `origin`
 *
 *  \param[in]  origin The origin of the property: IMPLICIT, EXTERNAL, ATTRIBUTE
 *  \param[out] names All the property names in the property manager.
 *  \returns The number of properties extracted from the property manager.
 */
int Ioss::PropertyManager::describe(Ioss::Property::Origin origin, NameList *names) const
{
  IOSS_FUNC_ENTER(m_);
  int                         the_count = 0;
  PropMapType::const_iterator I;
  for (I = m_properties.begin(); I != m_properties.end(); ++I) {
    if ((*I).second.get_origin() == origin) {
      names->push_back((*I).first);
      the_count++;
    }
  }
  return the_count;
}

/** Get the number of properties in the property manager
 *
 *  \returns The number of properties in the property manager.
 */
size_t Ioss::PropertyManager::count() const
{
  IOSS_FUNC_ENTER(m_);
  return m_properties.size();
}
