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

#ifndef IOSS_Ioss_PropertyManager_h
#define IOSS_Ioss_PropertyManager_h

#include "vtk_ioss_mangle.h"

#include <Ioss_CodeTypes.h>
#include <Ioss_Property.h> // for Property
#include <cstddef>         // for size_t
#include <string>          // for string, operator<
#include <unordered_map>
#include <vector> // for vector

namespace Ioss {
  using PropMapType = std::unordered_map<std::string, Property>;
  using ValuePair   = PropMapType::value_type;

  /** \brief A collection of Ioss::Property objects
   */
  class PropertyManager
  {
  public:
    PropertyManager() = default;
    PropertyManager(const PropertyManager &from);
    PropertyManager &operator=(const PropertyManager &from) = delete;
    ~PropertyManager();

    // Add the specified property to the list.
    void add(const Property &new_prop);

    // Assumes: Property 'name' must exist.
    void erase(const std::string &property_name);

    // Checks if a property with 'property_name' exists in the database.
    bool exists(const std::string &property_name) const;

    Property get(const std::string &property_name) const;

    // Returns the names of all properties
    int describe(NameList *names) const;

    // Returns the names of all properties or origin `origin`
    int describe(Ioss::Property::Origin origin, NameList *names) const;

    size_t count() const;

  private:
    PropMapType m_properties;
#if defined(IOSS_THREADSAFE)
    mutable std::mutex m_;
#endif
  };
} // namespace Ioss
#endif
