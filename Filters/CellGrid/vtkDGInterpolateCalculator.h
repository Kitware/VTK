// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDGInterpolateCalculator
 * @brief   Interpolate a field's value and possibly derivatives at a point in a cell.
 */

#ifndef vtkDGInterpolateCalculator_h
#define vtkDGInterpolateCalculator_h

#include "vtkCellAttribute.h"   // For CellTypeInfo.
#include "vtkDGCell.h"          // For ivar.
#include "vtkDGOperatorEntry.h" // For ivar.
#include "vtkInterpolateCalculator.h"
#include "vtkSmartPointer.h" // For ivar.
#include "vtkStringToken.h"  // For ivar.

VTK_ABI_NAMESPACE_BEGIN

class vtkCellAttribute;
class vtkDGRangeResponder;
class vtkDataArray;
class vtkTypeInt64Array;

/**\brief Calculate field values at a point in a cell's parametric space.
 *
 */
class VTKFILTERSCELLGRID_EXPORT vtkDGInterpolateCalculator : public vtkInterpolateCalculator
{
public:
  static vtkDGInterpolateCalculator* New();
  vtkTypeMacro(vtkDGInterpolateCalculator, vtkInterpolateCalculator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void Evaluate(vtkIdType cellId, const vtkVector3d& rst, std::vector<double>& value) override;
  void Evaluate(vtkIdTypeArray* cellIds, vtkDataArray* rst, vtkDataArray* result) override;

  bool AnalyticDerivative() const override;
  void EvaluateDerivative(vtkIdType cellId, const vtkVector3d& rst, std::vector<double>& jacobian,
    double neighborhood) override;
  void EvaluateDerivative(
    vtkIdTypeArray* cellIds, vtkDataArray* rst, vtkDataArray* result) override;

  vtkSmartPointer<vtkCellAttributeCalculator> PrepareForGrid(
    vtkCellMetadata* cell, vtkCellAttribute* field) override;

protected:
  friend class vtkDGRangeResponder;
  vtkDGInterpolateCalculator() = default;
  ~vtkDGInterpolateCalculator() override = default;

  template <bool UseShape>
  void InternalDerivative(
    vtkIdType cellId, const vtkVector3d& rst, std::vector<double>& jacobian, double neighborhood);

  /// Array pointers populated by PrepareForGrid.
  ///
  /// These arrays are used to look up values used to interpolate within cells.
  /// The Connectivity arrays should return vtkDataArray::IsIntegral() == true.
  vtkSmartPointer<vtkDataArray> FieldConnectivity;
  vtkSmartPointer<vtkDataArray> FieldValues;
  vtkSmartPointer<vtkDataArray> ShapeConnectivity;
  vtkSmartPointer<vtkDataArray> ShapeValues;

  vtkCellAttribute* Field{ nullptr };

  /// The parametric dimension of the current cell-type.
  int Dimension{ 3 };
  /// The shape of the current cell type.
  vtkDGCell::Shape CellShape{ vtkDGCell::Shape::None };

  /// The function space, basis, etc. of the target field.
  ///
  /// This is populated by PrepareForGrid.
  vtkCellAttribute::CellTypeInfo FieldCellInfo;

  /// The basis-function operator to use.
  vtkDGOperatorEntry FieldBasisOp;
  vtkDGOperatorEntry FieldGradientOp;

  /// The function space, basis, etc. of the shape attribute.
  vtkCellAttribute::CellTypeInfo ShapeCellInfo;

  /// The shape-basis operators (if any are needed).
  vtkDGOperatorEntry ShapeBasisOp;
  vtkDGOperatorEntry ShapeGradientOp;

private:
  vtkDGInterpolateCalculator(const vtkDGInterpolateCalculator&) = delete;
  void operator=(const vtkDGInterpolateCalculator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkDGInterpolateCalculator_h
