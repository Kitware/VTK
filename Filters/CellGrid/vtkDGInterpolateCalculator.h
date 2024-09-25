// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDGInterpolateCalculator
 * @brief   Interpolate a field's value and possibly derivatives at a point in a cell.
 */

#ifndef vtkDGInterpolateCalculator_h
#define vtkDGInterpolateCalculator_h

#include "vtkCellAttribute.h"         // For CellTypeInfo.
#include "vtkDGArrayOutputAccessor.h" // For ivars.
#include "vtkDGArraysInputAccessor.h" // For ivars.
#include "vtkDGCell.h"                // For ivar.
#include "vtkDGOperation.h"           // For ivars.
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

  /// The cell-type for which interpolation will be performed.
  ///
  /// This is set by PrepareForGrid().
  vtkDGCell* CellType{ nullptr };
  /// The cell-attribute for which interpolation will be performed.
  ///
  /// This is set by PrepareForGrid().
  vtkCellAttribute* Field{ nullptr };

  /// Used to compute a field value for a cell.
  vtkDGOperation<vtkDGArraysInputAccessor, vtkDGArrayOutputAccessor> FieldEvaluator;
  /// Used to compute a field derivative for a cell.
  vtkDGOperation<vtkDGArraysInputAccessor, vtkDGArrayOutputAccessor> FieldDerivative;

  /// Used when an array passed to Evaluate()/EvaluateDerivative() is not a double-array.
  ///
  /// The basis operators only process doubles (on the CPU).
  /// If needed, we copy the parameter and/or output arrays to/from a "local"
  /// double-valued array into what was passed.
  vtkNew<vtkDoubleArray> LocalField;

  /// The parametric dimension of the current cell-type.
  int Dimension{ 3 };
  /// The shape of the current cell type.
  vtkDGCell::Shape CellShape{ vtkDGCell::Shape::None };

  /// The function space, basis, etc. of the target field.
  ///
  /// This is populated by PrepareForGrid.
  vtkCellAttribute::CellTypeInfo FieldCellInfo;

private:
  vtkDGInterpolateCalculator(const vtkDGInterpolateCalculator&) = delete;
  void operator=(const vtkDGInterpolateCalculator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkDGInterpolateCalculator_h
