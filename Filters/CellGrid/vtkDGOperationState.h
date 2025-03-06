// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkDGOperationState_h
#define vtkDGOperationState_h

#include "vtkDGCell.h"
#include "vtkFiltersCellGridModule.h" // For export macro.

#include <array>

VTK_ABI_NAMESPACE_BEGIN

class vtkDGOperationStateEntryBase;

/**@class vtkDGOperationState
 * Encapsulate the state required to evaluate DG cell-attributes.
 *
 * This object holds input vtkDGOperatorEntry objects, input array
 * pointers, and working-space tuples (`std::vector` and `std::array` ivars)
 * required to evaluate a single vtkCellAttribute on cells corresponding
 * to a single vtkDGCell::Source entry. vtkDGOperation holds one instance
 * of vtkDGOperationState for each vtkDGCell::Source entry in a particular
 * vtkDGCell instance with arrays populated by a single vtkCellAttribute.
 */
class VTKFILTERSCELLGRID_EXPORT vtkDGOperationState
{
public:
  vtkDGOperationState(
    // Attribute arrays/operation
    vtkDGOperatorEntry& op, vtkDataArray* connectivity, vtkDataArray* values,
    vtkDataArray* sideConn, vtkTypeUInt64 offset,
    // Shape arrays/operation
    vtkDGOperatorEntry shapeGradient = vtkDGOperatorEntry(),
    vtkDataArray* shapeConnectivity = nullptr, vtkDataArray* shapeValues = nullptr)
    : OpEntry(op)
    , CellConnectivity(connectivity)
    , CellValues(values)
    , SideConnectivity(sideConn)
    , Offset(offset)
    , ShapeGradientEntry(shapeGradient)
    , ShapeConnectivity(shapeConnectivity)
    , ShapeValues(shapeValues)
  {
  }
  vtkDGOperationState(const vtkDGOperationState& other) = default;
  virtual ~vtkDGOperationState() = default;

  virtual void CloneInto(vtkDGOperationStateEntryBase& entry) const = 0;

  vtkDGOperatorEntry OpEntry;
  vtkDataArray* CellConnectivity;
  vtkDataArray* CellValues;
  vtkDataArray* SideConnectivity;
  vtkTypeUInt64 Offset;
  mutable std::array<vtkTypeUInt64, 2> SideTuple;
  mutable std::array<double, 3> RST{ { 0, 0, 0 } };
  mutable std::vector<vtkTypeUInt64> ConnTuple;
  mutable std::vector<double> ValueTuple;
  mutable std::vector<double> BasisTuple;
  mutable vtkTypeUInt64 LastCellId{ ~0ULL };
  mutable int NumberOfValuesPerFunction{ 0 };

  vtkDGOperatorEntry ShapeGradientEntry;
  vtkDataArray* ShapeConnectivity;
  vtkDataArray* ShapeValues;
  mutable std::vector<vtkTypeUInt64> ShapeConnTuple;
  mutable std::vector<double> ShapeValueTuple;
  mutable std::vector<double> ShapeBasisTuple;
  mutable std::vector<double> Jacobian;
  mutable int NumberOfShapeValuesPerFunction{ 0 };
  mutable vtkTypeUInt64 LastShapeCellId{ ~0ULL };
};

VTK_ABI_NAMESPACE_END

#endif // vtkDGOperationState_h
// VTK-HeaderTest-Exclude: vtkDGOperationState.h
