// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkHyperTreeGridGeometry2DImpl
 * @brief vtkHyperTreeGridGeometry internal classes for 2D vtkHyperTreeGrid
 *
 * This class is an internal class used in by the vtkHyperTreeGridGeometry filter
 * to generate the HTG surface in the 2D case.
 */

#ifndef vtkHyperTreeGridGeometry2DImpl_h
#define vtkHyperTreeGridGeometry2DImpl_h

#include "vtkHyperTreeGridGeometrySmallDimensionsImpl.h"

#include <vector> // For std::vector

VTK_ABI_NAMESPACE_BEGIN

class vtkHyperTreeGridGeometry2DImpl : public vtkHyperTreeGridGeometrySmallDimensionsImpl
{
public:
  vtkHyperTreeGridGeometry2DImpl(vtkHyperTreeGrid* input, vtkPoints* outPoints,
    vtkCellArray* outCells, vtkDataSetAttributes* inCellDataAttributes,
    vtkDataSetAttributes* outCellDataAttributes, bool passThroughCellIds,
    const std::string& originalCellIdArrayName);

  ~vtkHyperTreeGridGeometry2DImpl() override = default;

protected:
  /**
   * Generate the surface for a leaf cell cut by one interface.
   * Called by ProcessLeafCellWithInterface.
   */
  void ProcessLeafCellWithOneInterface(vtkHyperTreeGridNonOrientedGeometryCursor* cursor,
    double sign, const std::vector<double>& distancesToInterface) override;

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
   *
   * XXX: can be used in ProcessLeafCellWithoutInterface ?
   */
  void BuildCellPoints(vtkHyperTreeGridNonOrientedGeometryCursor* cursor) override;

private:
  /**
   * Denotes the oriention of the 2D HTG
   */
  unsigned int Axis1 = 0;
  unsigned int Axis2 = 0;
};

VTK_ABI_NAMESPACE_END
#endif /* vtkHyperTreeGridGeometry2DImpl_h */
