// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkHyperTreeGridGeometry1DImpl
 * @brief vtkHyperTreeGridGeometry internal classes for 1D vtkHyperTreeGrid
 *
 * This class is an internal class used in by the vtkHyperTreeGridGeometry filter
 * to generate the HTG surface in the 1D case.
 */

#ifndef vtkHyperTreeGridGeometry1DImpl_h
#define vtkHyperTreeGridGeometry1DImpl_h

#include "vtkHyperTreeGridGeometrySmallDimensionsImpl.h"

#include <vector> // For std::vector

VTK_ABI_NAMESPACE_BEGIN

class vtkHyperTreeGridGeometry1DImpl : public vtkHyperTreeGridGeometrySmallDimensionsImpl
{
public:
  vtkHyperTreeGridGeometry1DImpl(vtkHyperTreeGrid* input, vtkPoints* outPoints,
    vtkCellArray* outCells, vtkDataSetAttributes* inCellDataAttributes,
    vtkDataSetAttributes* outCellDataAttributes, bool passThroughCellIds,
    const std::string& originalCellIdArrayName);

  ~vtkHyperTreeGridGeometry1DImpl() override = default;

protected:
  /**
   * Generate the surface for a leaf cell cut by one interface.
   * Called by ProcessLeafCellWithInterface.
   */
  void ProcessLeafCellWithOneInterface(vtkHyperTreeGridNonOrientedGeometryCursor* cursor,
    double signe, const std::vector<double>& distancesToInterface) override;

  /**
   * Generate the surface for a leaf cell cut by two interfaces.
   * Called by ProcessLeafCellWithInterface.
   */
  void ProcessLeafCellWithDoubleInterface(vtkHyperTreeGridNonOrientedGeometryCursor* cursor,
    const std::vector<double>& distancesToInterfaceA,
    const std::vector<double>& distancesToInterfaceB) override;

  /**
   * Compute the point coordinates of the surface of the current cell, independently
   * of the fact that the current cell has a defined interface or not.
   *
   * Used as a pre-process in ProcessLeafCellWithInterface.
   */
  void BuildCellPoints(vtkHyperTreeGridNonOrientedGeometryCursor* cursor) override;

private:
  /**
   * Denotes the oriention of the 1D HTG
   * 0, 1, 2 = aligned along the X, Y, Z axis
   */
  unsigned int Axis = 0;
};

VTK_ABI_NAMESPACE_END
#endif /* vtkHyperTreeGridGeometry1DImpl_h */
