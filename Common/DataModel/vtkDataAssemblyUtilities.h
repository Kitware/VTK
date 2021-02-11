/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataAssemblyUtilities.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
   * composite dataset `input`. If `output` is non-null, then the input is also
   * converted to a `vtkPartitionedDataSetCollection` that, together with the
   * `hierarchy`, can represent similar logical organization of datasets
   * as the `input`.
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
   * Convert nodes selected using the provided selectors to a list of composite
   * indices using the hierarchy. The hierarchy is assumed to be generated using
   * `GenerateHierarchy` and has encoded meta-data that helps us recover the
   * composite indices for the input dataset.
   *
   * Currently, this is only supported for hierarchy that represent a
   * vtkMultiBlockDataSet. This is so since filters that work with composite
   * indices are to be treated as legacy and should be updated to work directly
   * with selectors instead.
   */
  static std::vector<unsigned int> GenerateCompositeIndicesFromSelectors(vtkDataAssembly* hierarchy,
    const std::vector<std::string>& selectors, bool leaf_nodes_only = false);

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

#endif
