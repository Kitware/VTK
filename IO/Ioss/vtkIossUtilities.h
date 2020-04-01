/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIossUtilities.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @namespace vtkIossUtilities
 * @brief internal utilities for vtkIossReader
 *
 * vtkIossReader provides some helper functions to go between VTK and Ioss.
 * Not intended for public consumption. API likely to change without notice.
 *
 * @section DeveloperNotes Developer Notes
 *
 * We limit this namespace for utility functions that go between Ioss and VTK or
 * vice-versa. Thus, methods that are not straddling that fence should be not be
 * added here.
 *
 */

#ifndef vtkIossUtilities_h
#define vtkIossUtilities_h

#include "vtkDataArraySelection.h"
#include "vtkDoubleArray.h"
#include "vtkIossReader.h"
#include "vtkObject.h"
#include "vtkSmartPointer.h"
#include "vtkTypeInt32Array.h"
#include "vtkTypeInt64Array.h"
#include "vtkTypeList.h" // Needed for ArrayList definition

// Ioss includes
#include <vtk_ioss.h>
// clang-format off
#include VTK_IOSS(Ioss_Region.h)
#include VTK_IOSS(Ioss_Transform.h)
// clang-format on

#include <cassert>
#include <set>

class vtkCellArray;
namespace vtkIossUtilities
{

/**
 * Cache
 */
class Cache
{
public:
  Cache();
  ~Cache();

  /**
   * Call this to clear internal count for hits.
   */
  void ResetAccessCounts();

  /**
   * Removes all cached entries not accessed since
   * most recent call to `ResetAccessCounts`.
   */
  void ClearUnused();

  /**
   * Clears the cache.
   */
  void Clear();

  vtkObject* Find(const Ioss::GroupingEntity* entity, const std::string& cachekey) const;
  void Insert(const Ioss::GroupingEntity* entity, const std::string& cachekey, vtkObject* array);

private:
  Cache(const Cache&) = delete;
  void operator=(const Cache&) = delete;

  class CacheInternals;
  CacheInternals* Internals;
};

using EntityNameType = std::pair<vtkTypeUInt64, std::string>;

/**
 * List of possible ArrayTypes that are produced by vtkIossUtilities.
 *
 * This can be used with vtkArrayDispatch::DispatchByArray, etc. when dealing
 * with arrays read from Ioss.
 */
using ArrayList = typename vtkTypeList::Unique<
  vtkTypeList::Create<vtkDoubleArray, vtkTypeInt32Array, vtkTypeInt64Array>>::Result;

/**
 * Reads time / timestep information from a region. Returns an empty vector if
 * no time information in available in the Ioss::Region.
 */
std::set<double> GetTimeValues(const Ioss::Region* region);

/**
 * Populates `entitySelection` with available entity block (or set) names and
 * populates `fieldSelection` with transient and attribute fields on the chosen
 * entity block (or set).
 */
template <typename EntityType>
void GetEntityAndFieldNames(const Ioss::Region*, const std::vector<EntityType*>& entities,
  std::set<EntityNameType>& entity_names, std::set<std::string>& field_names)
{
  for (const auto& entity : entities)
  {
    const int64_t id = entity->property_exists("id") ? entity->get_property("id").get_int() : 0;
    entity_names.insert(EntityNameType{ static_cast<vtkTypeUInt64>(id), entity->name() });

    Ioss::NameList attributeNames;
    entity->field_describe(Ioss::Field::TRANSIENT, &attributeNames);
    entity->field_describe(Ioss::Field::ATTRIBUTE, &attributeNames);
    std::copy(
      attributeNames.begin(), attributeNames.end(), std::inserter(field_names, field_names.end()));
  }
}

/**
 * For the given vtkIossReader::EntityType return the corresponding
 * Ioss::EntityType.
 *
 * Throws `std::runtime_error` for invalid values.
 */
Ioss::EntityType GetIossEntityType(vtkIossReader::EntityType vtk_type);

/**
 * Create an array for the given `field`. Uses type information from the field
 * to create the correct type of array. Also resizes the array using count
 * and component information from the field.
 *
 * Throws `std::runtime_error` for unsupported types.
 */
vtkSmartPointer<vtkDataArray> CreateArray(const Ioss::Field& field);

/**
 * Returns a VTK array for a given field (`fieldname`) on the chosen
 * block (or set) entity.
 *
 * Throws `std::runtime_error` on error or field not present.
 */
vtkSmartPointer<vtkDataArray> GetData(const Ioss::GroupingEntity* entity,
  const std::string& fieldname, Ioss::Transform* transform = nullptr, Cache* cache = nullptr,
  const std::string& cachekey = std::string());

/**
 * Returns VTK celltype for a Ioss topology element.
 *
 * Throws `std::runtime_error` for unknown and unsupported element types.
 *
 * Note that the returned VTK cell type may have fewer points than the
 * corresponding Ioss element type.
 */
int GetCellType(const Ioss::ElementTopology* topology);

/**
 * Read connectivity information from the group_entity.
 *
 * Returns the `vtkCellArray` and the element type for all elements in this
 * group_entity.
 *
 * NOTE: this does not support entity groups with mixed topological elements.
 *
 * Throws `std::runtime_error` on error.
 */
vtkSmartPointer<vtkCellArray> GetConnectivity(
  Ioss::GroupingEntity* group_entity, int& vtk_topology_type, Cache* cache = nullptr);

/**
 * Read points from the group_entity.
 *
 * Throws `std::runtime_error` on error or coordinates not present.
 */
vtkSmartPointer<vtkPoints> GetMeshModelCoordinates(
  Ioss::GroupingEntity* group_entity, Cache* cache = nullptr);

/**
 * Returns true if the field is transient.
 *
 * This method supports SIDESETs. It iterates into the nest SIDEBLOCK elements
 * to check for the field.
 */
bool IsFieldTransient(Ioss::GroupingEntity* entity, const std::string& fieldname);

/**
 * Finds a displacement field name. Returns empty string if none can be found.
 */
std::string GetDisplacementFieldName(Ioss::GroupingEntity* nodeblock);
};

#endif
// VTK-HeaderTest-Exclude: vtkIossUtilities.h
