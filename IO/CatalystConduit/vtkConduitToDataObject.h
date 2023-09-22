// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkConduitToDataObject
 * @brief Convert conduit node to VTK data objects.
 */

#ifndef vtkConduitToDataObject_h
#define vtkConduitToDataObject_h

#include "vtkIOCatalystConduitModule.h" // For windows import/export of shared libraries

#include "vtkObject.h" // for ABI namespace
#include "vtkSmartPointer.h"

namespace conduit_cpp
{
VTK_ABI_NAMESPACE_BEGIN
class Node;
VTK_ABI_NAMESPACE_END
}

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractArray;
class vtkCellArray;
class vtkDataObject;
class vtkDataSet;
class vtkImageData;
class vtkOverlappingAMR;
class vtkPartitionedDataSet;
class vtkPoints;
class vtkRectilinearGrid;
class vtkStructuredGrid;
class vtkUnstructuredGrid;
VTK_ABI_NAMESPACE_END

namespace vtkConduitToDataObject
{
VTK_ABI_NAMESPACE_BEGIN

/**
 * Fill the vtkPartitionedDataSet input.
 * Create concrete vtkDataSet subclass to set it as partition and add arrays in its
 * DataSetAttributes.
 *
 * Return true if data was correctly generated, false if an error occurred.
 * Do not throw errors.
 */
VTKIOCATALYSTCONDUIT_EXPORT bool FillPartionedDataSet(
  vtkPartitionedDataSet* output, const conduit_cpp::Node& meshNode);

/**
 * Fill the vtkOverlappingAMR input.
 */
VTKIOCATALYSTCONDUIT_EXPORT bool FillAMRMesh(vtkOverlappingAMR* amr, const conduit_cpp::Node& node);

/**
 * vtkDataSet creation.
 */
///@{
/**
 * Create a vtkDataSet concrete subclass from the given a topology and a coordsets nodes.
 *
 * Throw runtime_error on unsupported input.
 */
VTKIOCATALYSTCONDUIT_EXPORT vtkSmartPointer<vtkDataSet> CreateMesh(
  const conduit_cpp::Node& topology, const conduit_cpp::Node& coordsets);

/**
 * Create a vtkImageData from a coordset node.
 */
VTKIOCATALYSTCONDUIT_EXPORT vtkSmartPointer<vtkImageData> CreateImageData(
  const conduit_cpp::Node& coordset);

/**
 * Create a vtkRectilinearGrid from a coordset node.
 */
VTKIOCATALYSTCONDUIT_EXPORT vtkSmartPointer<vtkRectilinearGrid> CreateRectilinearGrid(
  const conduit_cpp::Node& coordset);

/**
 * Create a vtkStructuredGrid from a topology and a coordset nodes.
 */
VTKIOCATALYSTCONDUIT_EXPORT vtkSmartPointer<vtkStructuredGrid> CreateStructuredGrid(
  const conduit_cpp::Node& topology, const conduit_cpp::Node& coordset);

/**
 * Create a vtkUnstructuredGrid from a topology and a coordset node.
 * Topology should have a unique cell type, i.e. its "elements/shape" should not be "mixed".
 * see CreateMixedUnstructuredGrid.
 */
VTKIOCATALYSTCONDUIT_EXPORT vtkSmartPointer<vtkDataSet> CreateMonoShapedUnstructuredGrid(
  const conduit_cpp::Node& topologyNode, const conduit_cpp::Node& coordset);

/**
 * Create a vtkUnstructuredGrid from a coordset and a topology node.
 * Topology "elements/shape" is expected to be "mixed".
 * see CreateMonoShapedUnstructuredGrid
 * throw a runtime_error on invalid node
 */
VTKIOCATALYSTCONDUIT_EXPORT vtkSmartPointer<vtkDataSet> CreateMixedUnstructuredGrid(
  const conduit_cpp::Node& topologyNode, const conduit_cpp::Node& coords);

///@}

/**
 * Add FieldData arrays to output data object.
 * Return true if node was correctly parsed, false if a fatal error occurred.
 * If isAMReX, data array is added as a `CellData`
 */
VTKIOCATALYSTCONDUIT_EXPORT bool AddFieldData(
  vtkDataObject* output, const conduit_cpp::Node& stateFields, bool isAMReX = false);

/**
 * Create a vtkPoints from a coordset node that respect the following requirements:
 * - "type" should be "explicit"
 * - "values" should have at max 3 components
 * throw a runtime_error on invalid node
 */
VTKIOCATALYSTCONDUIT_EXPORT vtkSmartPointer<vtkPoints> CreatePoints(
  const conduit_cpp::Node& coords);

/**
 * Create polyhedron in grid from elements and subelements.
 */
VTKIOCATALYSTCONDUIT_EXPORT void SetPolyhedralCells(
  vtkUnstructuredGrid* grid, vtkCellArray* elements, vtkCellArray* subelements);

///@{
/**
 * Return the number of points in VTK cell type.
 * throw a runtime_error on unsupported type.
 */
VTKIOCATALYSTCONDUIT_EXPORT vtkIdType GetNumberOfPointsInCellType(int vtk_cell_type);

/**
 * Get vtk cell type from conduit shape name
 * throw a runtime_error on unsupported type.
 */
VTKIOCATALYSTCONDUIT_EXPORT int GetCellType(const std::string& shape);

/**
 * Get vtkDataObject attribute type from conduit association string.
 * Supports only "element" and "vertex".
 *
 * Throw runtime_error on unsupported association.
 */
VTKIOCATALYSTCONDUIT_EXPORT int GetAssociation(const std::string& association);
///@}

VTK_ABI_NAMESPACE_END
}

#endif
// VTK-HeaderTest-Exclude: vtkConduitToDataObject.h
