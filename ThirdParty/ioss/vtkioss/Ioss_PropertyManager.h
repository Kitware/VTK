// Copyright(C) 1999-2022 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "ioss_export.h"

#include "vtk_ioss_mangle.h"

#include <Ioss_CodeTypes.h>
#include <Ioss_Property.h> // for Property
#include <cstddef>         // for size_t
#include <string>          // for string, operator<
#include <vector>          // for vector

#define USE_ROBIN_MAP
#if defined USE_ROBIN_MAP
#include <robin_map.h>
#else
#include <unordered_map>
#endif

namespace Ioss {
#if defined USE_ROBIN_MAP
  using PropMapType = tsl::robin_pg_map<std::string, Property>;
#else
  using PropMapType = std::unordered_map<std::string, Property>;
#endif
  using ValuePair = PropMapType::value_type;

  /** \brief A collection of Ioss::Property objects
   */
  class IOSS_EXPORT PropertyManager
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
    double      get_optional(const std::string &property_name, double optional_value) const;
    int64_t     get_optional(const std::string &property_name, int64_t optional_value) const;
    int         get_optional(const std::string &property_name, int optional_value) const;
    std::string get_optional(const std::string &property_name,
                             const std::string &optional_value) const;

    // Returns the names of all properties
    int      describe(NameList *names) const;
    NameList describe() const;

    // Returns the names of all properties or origin `origin`
    int      describe(Ioss::Property::Origin origin, NameList *names) const;
    NameList describe(Ioss::Property::Origin origin) const;

    size_t count() const;

  private:
    PropMapType m_properties{};
#if defined(IOSS_THREADSAFE)
    mutable std::mutex m_;
#endif
  };
} // namespace Ioss
