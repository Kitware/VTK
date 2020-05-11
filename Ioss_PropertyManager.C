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
    IOSS_FUNC_ENTER(m_);
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
  IOSS_FUNC_ENTER(m_);
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
