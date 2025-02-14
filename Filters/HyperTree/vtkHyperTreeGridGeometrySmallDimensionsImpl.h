// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkHyperTreeGridGeometrySmallDimensionsImpl
 * @brief Common interface for 1D and 2D vtkHyperTreeGridGeometry internal classes
 *
 * This class defines the common interface for or 1D and 2D vtkHyperTreeGridGeometry
 * internal classes, because their overall logic is very close.
 */

#ifndef vtkHyperTreeGridGeometrySmallDimensionsImpl_h
#define vtkHyperTreeGridGeometrySmallDimensionsImpl_h

#include "vtkHyperTreeGridGeometryImpl.h"

#include <vector> // For std::vector

VTK_ABI_NAMESPACE_BEGIN

class vtkDataArray;
class vtkIncrementalPointLocator;
class vtkPoints;
class vtkUnsignedCharArray;
class vtkHyperTreeGridNonOrientedGeometryCursor;

class vtkHyperTreeGridGeometrySmallDimensionsImpl : public vtkHyperTreeGridGeometryImpl
{
public:
  vtkHyperTreeGridGeometrySmallDimensionsImpl(vtkHyperTreeGrid* input, vtkPoints* outPoints,
    vtkCellArray* outCells, vtkDataSetAttributes* inCellDataAttributes,
    vtkDataSetAttributes* outCellDataAttributes, bool passThroughCellIds,
    const std::string& originalCellIdArrayName, bool fillMaterial);

  ~vtkHyperTreeGridGeometrySmallDimensionsImpl() override = default;

  /**
   * Generate the external surface of the input vtkHyperTreeGrid.
   */
  void GenerateGeometry() override;

protected:
  /**
   * Recursively browse the input HTG in order to generate the output surface.
   * Tis method is called by GenerateGeometry.
   *
   * XXX: We need to determine a common interface for all cursors in order to
   * define RecursivelyProcessTree as virtual in upper classes.
   */
  void RecursivelyProcessTree(vtkHyperTreeGridNonOrientedGeometryCursor* cursor);

  /**
   * Generate the surface for a leaf cell with a defined interface.
   */
  void ProcessLeafCellWithInterface(vtkHyperTreeGridNonOrientedGeometryCursor* cursor);

  /**
   * Generate the surface for a leaf cell with no interface.
   */
  virtual void ProcessLeafCellWithoutInterface(vtkHyperTreeGridNonOrientedGeometryCursor* cursor);

  /**
   * Generate the surface for a leaf cell cut by one interface.
   * Called by ProcessLeafCellWithInterface.
   */
  virtual void ProcessLeafCellWithOneInterface(vtkHyperTreeGridNonOrientedGeometryCursor* cursor,
    double sign, const std::vector<double>& distancesToInterface) = 0;

  /**
   * Generate the surface for a leaf cell cut by two interfaces.
   * Called by ProcessLeafCellWithInterface.
   */
  virtual void ProcessLeafCellWithDoubleInterface(vtkHyperTreeGridNonOrientedGeometryCursor* cursor,
    const std::vector<double>& distancesToInterfaceA,
    const std::vector<double>& distancesToInterfaceB) = 0;

  /**
   * Compute the point coordinates of the surface of the current cell, independently
   * of the fact that the current cell has a defined interface or not.
   *
   * Used as a pre-process in ProcessLeafCellWithInterface.
   *
   * XXX: can be used in ProcessLeafCellWithoutInterface ?
   */
  virtual void BuildCellPoints(vtkHyperTreeGridNonOrientedGeometryCursor* cursor) = 0;

  /**
   * Contains the point coordinates of the current cell surface,
   * without considering eventual cuts made by interfaces.
   *
   * XXX: cache variable for the "current" cell
   */
  vtkNew<vtkPoints> CellPoints;
};

VTK_ABI_NAMESPACE_END
#endif /* vtkHyperTreeGridGeometrySmallDimensionsImpl_h */
