// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkDataAssemblyUtilities
 * @brief collections of utilities for vtkDataAssembly
 *
 * vtkDataAssemblyUtilities provides useful utilities for working with
 * vtkDataAssembly.
 */

#ifndef vtkDataAssemblyUtilities_h
#define vtkDataAssemblyUtilities_h

#include "vtkCommonDataModelModule.h" // for export macros
#include "vtkObject.h"
#include "vtkSmartPointer.h" // for vtkSmartPointer

#include <string> // for std::string
#include <vector> // for std::vector

VTK_ABI_NAMESPACE_BEGIN
class vtkCompositeDataSet;
class vtkDataAssembly;
class vtkDataObject;
class vtkMultiBlockDataSet;
class vtkPartitionedDataSetCollection;
class vtkUniformGridAMR;

class VTKCOMMONDATAMODEL_EXPORT vtkDataAssemblyUtilities : public vtkObject
{
public:
  static vtkDataAssemblyUtilities* New();
  vtkTypeMacro(vtkDataAssemblyUtilities, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns the named used by VTK to correspond to a vtkDataAssembly
   * associated with the structure of a composite dataset.
   */
  static const char* HierarchyName() { return "Hierarchy"; }

  /**
   * Convenience method to get a named vtkDataAssembly from a
   * vtkCompositeDataSet, if available. May return nullptr if none exists or
   * possible.
   */
  static vtkSmartPointer<vtkDataAssembly> GetDataAssembly(
    const char* name, vtkCompositeDataSet* cd);

  /**
   * Populates `hierarchy` with a representation of the hierarchy for the given
   * composite dataset `input`. A hierarchy represents the input dataset's
   * structure as represented in the dataset itself.
   *
   * If `output` is non-null, then the input is also
   * converted to a `vtkPartitionedDataSetCollection`. The vtkDataAssembly on
   * the `output` is updated to be a copy of the hierarchy with correct dataset
   * indices so that relationships in the input blocks are preserved.
   *
   * If input is not a `vtkMultiBlockDataSet`, `vtkPartitionedDataSetCollection`, or
   * `vtkUniformGridAMR`, then there's no hierarchy to represent and hence this
   * function will return `false`.
   */
  static bool GenerateHierarchy(vtkCompositeDataSet* input, vtkDataAssembly* hierarchy,
    vtkPartitionedDataSetCollection* output = nullptr);

  /**
   * Inverse of `GenerateHierarchy`. Given a vtkPartitionedDataSetCollection and
   * a vtkDataAssembly representing a target hierarchy, create a
   * appropriate vtkCompositeDataSet subclass representing that hierarchy.
   *
   * A note about vtkOverlappingAMR: since all meta-data necessary for
   * defining a valid vtkOverlappingAMR is not encoded in the hierarchy, the
   * return vtkOverlappingAMR is not complete (or valid) and is missing key
   * meta-data. Calling code must use other mechanisms to make the dataset
   * valid.
   */
  static vtkSmartPointer<vtkCompositeDataSet> GenerateCompositeDataSetFromHierarchy(
    vtkPartitionedDataSetCollection* input, vtkDataAssembly* hierarchy);

  /**
   * Given a vtkDataAssembly and collection of selectors, returns a list of
   * selected composite indices for the selected nodes. The vtkDataAssembly can
   * represent either a hierarchy or simply be an assembly. For the later, an
   * associated `vtkPartitionedDataSetCollection` must be provided to correctly
   * determine the composite index for the selected nodes. When an
   * hierarchy is used, the hierarchy encodes enough information to determine
   * composite ids and hence the `data` argument must be nullptr.
   *
   * `leaf_nodes_only` can be used to indicate if the composite ids must only
   * refer to leaf nodes i.e. nodes that cannot have additional child nodes.
   */
  static std::vector<unsigned int> GetSelectedCompositeIds(
    const std::vector<std::string>& selectors, vtkDataAssembly* hierarchyOrAssembly,
    vtkPartitionedDataSetCollection* data = nullptr, bool leaf_nodes_only = false);

  ///@{
  /**
   * For a vtkDataAssembly representing an hierarchy, returns the selector for
   * the given composite id. Note, the selectors maybe best-match. When dealing
   * with vtkPartitionedDataSetCollection, for example, a composite id may point
   * to a particular dataset in a nested vtkPartitionedDataSet, however,
   * selectors are simply not expressive enough to pick a dataset at that level
   * and hence will simply point to the parent vtkPartitionedDataSet.
   */
  static std::string GetSelectorForCompositeId(unsigned int id, vtkDataAssembly* hierarchy);
  static std::vector<std::string> GetSelectorsForCompositeIds(
    const std::vector<unsigned int>& ids, vtkDataAssembly* hierarchy);
  static std::vector<std::string> GetSelectorsForCompositeIds(
    const std::vector<unsigned int>& ids, vtkDataAssembly* hierarchy, vtkDataAssembly* assembly);
  static std::vector<unsigned int> GetSelectorsCompositeIdsForCompositeIds(
    const std::vector<unsigned int>& ids, vtkDataAssembly* hierarchy);
  ///@}

protected:
  vtkDataAssemblyUtilities();
  ~vtkDataAssemblyUtilities() override;

  static bool GenerateHierarchyInternal(
    vtkUniformGridAMR* input, vtkDataAssembly* hierarchy, vtkPartitionedDataSetCollection* output);
  static bool GenerateHierarchyInternal(vtkMultiBlockDataSet* input, vtkDataAssembly* hierarchy,
    vtkPartitionedDataSetCollection* output);
  static bool GenerateHierarchyInternal(vtkPartitionedDataSetCollection* input,
    vtkDataAssembly* hierarchy, vtkPartitionedDataSetCollection* output);

private:
  vtkDataAssemblyUtilities(const vtkDataAssemblyUtilities&) = delete;
  void operator=(const vtkDataAssemblyUtilities&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
