// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDGInterpolateCalculator
 * @brief   Interpolate a field's value and possibly derivatives at a point in a cell.
 */

#ifndef vtkDGInterpolateCalculator_h
#define vtkDGInterpolateCalculator_h

#include "vtkDGCell.h" // For ivar.
#include "vtkInterpolateCalculator.h"
#include "vtkSmartPointer.h" // For ivar.
#include "vtkStringToken.h"  // For ivar.

VTK_ABI_NAMESPACE_BEGIN

class vtkCellAttribute;
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
  bool AnalyticDerivative() const override;
  void EvaluateDerivative(vtkIdType cellId, const vtkVector3d& rst, std::vector<double>& jacobian,
    double neighborhood) override;

protected:
  vtkDGInterpolateCalculator() = default;
  ~vtkDGInterpolateCalculator() override = default;

  vtkSmartPointer<vtkCellAttributeCalculator> PrepareForGrid(
    vtkCellMetadata* cell, vtkCellAttribute* field) override;

  template <bool UseShape>
  void InternalDerivative(
    vtkIdType cellId, const vtkVector3d& rst, std::vector<double>& jacobian, double neighborhood);

  /// Array pointers populated by PrepareForGrid.
  ///
  /// These arrays are used to look up values used to interpolate within cells.
  vtkSmartPointer<vtkTypeInt64Array> FieldConnectivity;
  vtkSmartPointer<vtkDataArray> FieldValues;
  vtkSmartPointer<vtkTypeInt64Array> ShapeConnectivity;
  vtkSmartPointer<vtkDataArray> ShapeValues;

  /// The number of components in the field
  int NumberOfComponents{ 0 };
  /// The number of basis functions for the current cell-type's shape
  int NumberOfBasisFunctions{ 0 };
  /// The parametric dimension of the current cell-type.
  int Dimension{ 3 };
  /// The shape of the current cell type.
  vtkDGCell::Shape CellShape{ vtkDGCell::Shape::None };

  /// The function space of the target field.
  ///
  /// This is populated by PrepareForGrid.
  vtkStringToken FunctionSpace;

private:
  vtkDGInterpolateCalculator(const vtkDGInterpolateCalculator&) = delete;
  void operator=(const vtkDGInterpolateCalculator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkDGInterpolateCalculator_h
