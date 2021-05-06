// Copyright(C) 1999-2020 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

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

    Property    get(const std::string &property_name) const;
    int64_t     get_optional(const std::string &property_name, int64_t optional_value) const;
    std::string get_optional(const std::string &property_name,
                             const std::string &optional_value) const;

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
