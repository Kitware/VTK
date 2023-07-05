// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @namespace vtkIOSSUtilities
 * @brief internal utilities for vtkIOSSReader
 *
 * vtkIOSSReader provides some helper functions to go between VTK and Ioss.
 * Not intended for public consumption. API likely to change without notice.
 *
 * @section DeveloperNotes Developer Notes
 *
 * We limit this namespace for utility functions that go between Ioss and VTK or
 * vice-versa. Thus, methods that are not straddling that fence should be not be
 * added here.
 *
 */

#ifndef vtkIOSSUtilities_h
#define vtkIOSSUtilities_h

#include "vtkDataArraySelection.h"
#include "vtkDoubleArray.h"
#include "vtkIOSSReader.h"
#include "vtkLogger.h"
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
#include VTK_IOSS(Ioss_StructuredBlock.h)
#include VTK_IOSS(Ioss_SideSet.h)
#include VTK_IOSS(Ioss_SideBlock.h)
// clang-format on

#include <cassert>
#include <set>

VTK_ABI_NAMESPACE_BEGIN
class vtkCellArray;
class vtkDataSet;
VTK_ABI_NAMESPACE_END

namespace vtkIOSSUtilities
{
VTK_ABI_NAMESPACE_BEGIN

enum DatabaseFormatType
{
  UNKNOWN,
  EXODUS,
  CGNS,
  CATALYST,
};

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

/**
 * A helper to instantiate on stack to temporarily redirect non-critical messages
 * emanating from IOSS. See paraview/paraview#21193
 */
class CaptureNonErrorMessages
{
public:
  CaptureNonErrorMessages();
  ~CaptureNonErrorMessages();

  /**
   * Provides access to the accumulated messages.
   */
  std::string GetMessages() const;

private:
  std::ostringstream Stream;
  std::ostream* DebugStream;
  std::ostream* WarningStream;
};

using EntityNameType = std::pair<vtkTypeUInt64, std::string>;

/**
 * List of possible ArrayTypes that are produced by vtkIOSSUtilities.
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
std::vector<std::pair<int, double>> GetTime(const Ioss::Region* region);

/**
 * This is primarily intended for CGNS. CGNS ends up naming blocks in separate
 * files separate e.g. block_0_proc-0, block_0_proc-1, etc. This is clunky and
 * causes the block selection as well as the output dataset to be oddly
 * structured. We want all merge all pieces of a block for all procs. This
 * function helps that by stripping out the "proc-\d+" substring.
 */
std::string GetSanitizedBlockName(const Ioss::Region* region, const std::string& name);

/**
 * Populates `entitySelection` with available entity block (or set) names and
 * populates `fieldSelection` with transient and attribute fields on the chosen
 * entity block (or set).
 */
template <typename EntityType>
void GetEntityAndFieldNames(const Ioss::Region* region, const std::vector<EntityType*>& entities,
  std::set<EntityNameType>& entity_names, std::set<std::string>& field_names)
{
  for (const auto& entity : entities)
  {
    const int64_t id = entity->property_exists("id") ? entity->get_property("id").get_int() : 0;
    auto name = vtkIOSSUtilities::GetSanitizedBlockName(region, entity->name());
    entity_names.insert(EntityNameType{ static_cast<vtkTypeUInt64>(id), name });

    Ioss::NameList attributeNames;
    entity->field_describe(Ioss::Field::TRANSIENT, &attributeNames);
    entity->field_describe(Ioss::Field::ATTRIBUTE, &attributeNames);
    std::copy(
      attributeNames.begin(), attributeNames.end(), std::inserter(field_names, field_names.end()));
  }
}
/**
 * Specialization for Ioss::SideSet (see paraview/paraview#21231).
 */
template <>
void GetEntityAndFieldNames<Ioss::SideSet>(const Ioss::Region* region,
  const std::vector<Ioss::SideSet*>& entities, std::set<EntityNameType>& entity_names,
  std::set<std::string>& field_names);

/**
 * For the given vtkIOSSReader::EntityType return the corresponding
 * Ioss::EntityType.
 *
 * Throws `std::runtime_error` for invalid values.
 */
Ioss::EntityType GetIOSSEntityType(vtkIOSSReader::EntityType vtk_type);

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
 * Returns an Ioss topology element, if possible, given a VTK cell type.
 *
 * This is inverse of GetCellType.
 *
 * Throws `std::runtime_error` for unknown and unsupported element types.
 */
const Ioss::ElementTopology* GetElementTopology(int vtk_cell_type);

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
  const Ioss::GroupingEntity* group_entity, Cache* cache = nullptr);

/**
 * Returns true if the field is transient.
 *
 * This method supports SIDESETs. It iterates into the nest SIDEBLOCK elements
 * to check for the field.
 */
bool IsFieldTransient(Ioss::GroupingEntity* entity, const std::string& fieldname);

///@{
/**
 * Finds a displacement field name. Returns empty string if none can be found.
 */
std::string GetDisplacementFieldName(Ioss::GroupingEntity* nodeblock);
std::string GetDisplacementFieldName(vtkDataSet* dataset);
///@}

/**
 * Must be called before using any Ioss library functions. Necessary to
 * initialize factories used internally by Ioss library.
 */
void InitializeEnvironmentForIOSS();

/**
 * Given a filename determines and returns the database type. Currently,
 * this simply looks at the filename.
 */
DatabaseFormatType DetectType(const std::string& dbaseName);

/**
 * Given any GroupingEntity pointer, returns the format that the associated
 * database is in. Use this to determine if we're dealing with Exodus or CGNS
 * database.
 */
DatabaseFormatType GetFormat(const Ioss::GroupingEntity* entity);

/**
 * Returns collection of StructuredBlock's matching the selected blockname.
 * Since vtkIOSSReader may modify block names to avoid creating separate block
 * for each rank for what logically is the same block, we have to use this
 * method to find the blocks user selected. @sa GetSanitizedBlockName
 */
std::vector<Ioss::StructuredBlock*> GetMatchingStructuredBlocks(
  Ioss::Region* region, const std::string& blockname);

VTK_ABI_NAMESPACE_END
}

#endif
// VTK-HeaderTest-Exclude: vtkIOSSUtilities.h
