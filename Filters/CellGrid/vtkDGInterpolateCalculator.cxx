// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDGInterpolateCalculator.h"
#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkCellMetadata.h"
#include "vtkDGOperation.txx"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkObjectFactory.h"
#include "vtkTypeInt64Array.h"
#include "vtkVector.h"

#include "vtk_eigen.h"
#include VTK_EIGEN(Eigen)

#include <sstream>

using namespace vtk::literals;

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkDGInterpolateCalculator);

void vtkDGInterpolateCalculator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  vtkIndent i2 = indent.GetNextIndent();
  os << indent << "FieldEvaluator:\n";
  this->FieldEvaluator.PrintSelf(os, i2);
  os << indent << "FieldDerivative:\n";
  this->FieldDerivative.PrintSelf(os, i2);

  // os << indent << "FieldValues: " << this->FieldValues << "\n";
  // os << indent << "ShapeConnectivity: " << this->ShapeConnectivity << "\n";
  // os << indent << "ShapeValues: " << this->ShapeValues << "\n";
}

void vtkDGInterpolateCalculator::Evaluate(
  vtkIdType cellId, const vtkVector3d& rst, std::vector<double>& value)
{
  value.resize(this->FieldEvaluator.GetNumberOfResultComponents());
  std::array<double, 3> arst{ rst[0], rst[1], rst[2] };
  vtkNew<vtkDoubleArray> result;
  vtkNew<vtkDoubleArray> vrst;
  vtkNew<vtkIdTypeArray> ids;
  result->SetNumberOfComponents(this->FieldEvaluator.GetNumberOfResultComponents());
  result->SetArray(value.data(), static_cast<vtkIdType>(value.size()), /*save*/ 1);
  vrst->SetNumberOfComponents(3);
  vrst->SetArray(arst.data(), 3, /*save*/ 1);
  ids->SetNumberOfTuples(1);
  ids->SetValue(0, cellId);
  // value.resize(this->FieldBasisOp.OperatorSize * );
  vtkDGArraysInputAccessor inIt(ids, vrst);
  vtkDGArrayOutputAccessor outIt(result);
  this->FieldEvaluator.Evaluate(inIt, outIt, 0, 1);
}

void vtkDGInterpolateCalculator::Evaluate(
  vtkIdTypeArray* cellIds, vtkDataArray* rst, vtkDataArray* result)
{
  vtkDoubleArray* dresult = vtkDoubleArray::SafeDownCast(result);
  if (!dresult)
  {
    dresult = this->LocalField.GetPointer();
  }

  assert(cellIds->GetNumberOfTuples() == rst->GetNumberOfTuples());

  vtkIdType numEvals = cellIds->GetNumberOfTuples();
  dresult->SetNumberOfComponents(this->Field->GetNumberOfComponents());
  dresult->SetNumberOfTuples(cellIds->GetNumberOfTuples());
  vtkDGArraysInputAccessor inIt(cellIds, rst);
  vtkDGArrayOutputAccessor outIt(dresult);
  this->FieldEvaluator.Evaluate(inIt, outIt, 0, numEvals);

  // Finally, if we were given a non-vtkDoubleArray, copy the results
  // back into the output array.
  if (dresult != result)
  {
    result->DeepCopy(dresult);
  }
}

bool vtkDGInterpolateCalculator::AnalyticDerivative() const
{
  switch (this->FieldCellInfo.FunctionSpace.GetId())
  {
    case "HGRAD"_hash:
      return true;
    default:
      break;
  }
  return false;
}

void vtkDGInterpolateCalculator::EvaluateDerivative(
  vtkIdType cellId, const vtkVector3d& rst, std::vector<double>& jacobian, double neighborhood)
{
  if (!this->AnalyticDerivative())
  {
    // We don't have an analytic derivative; approximate it.
    this->Superclass::EvaluateDerivative(cellId, rst, jacobian, neighborhood);
    return;
  }

  vtkNew<vtkDoubleArray> result;
  vtkNew<vtkDoubleArray> vrst;
  vtkNew<vtkIdTypeArray> ids;
  result->SetNumberOfComponents(this->FieldDerivative.GetNumberOfResultComponents());
  result->SetArray(jacobian.data(), static_cast<vtkIdType>(jacobian.size()), /*save*/ 1);
  vrst->SetNumberOfComponents(3);
  vrst->SetArray(const_cast<double*>(rst.GetData()), 3, /*save*/ 1);
  ids->SetNumberOfTuples(1);
  ids->SetValue(0, cellId);
  // value.resize(this->FieldBasisOp.OperatorSize * );
  vtkDGArraysInputAccessor inIt(ids, vrst);
  vtkDGArrayOutputAccessor outIt(result);
  this->FieldDerivative.Evaluate(inIt, outIt, 0, 1);
}

void vtkDGInterpolateCalculator::EvaluateDerivative(
  vtkIdTypeArray* cellIds, vtkDataArray* rst, vtkDataArray* result)
{
  if (!this->AnalyticDerivative())
  {
    // We don't have an analytic derivative; approximate it.
    // return this->Superclass::EvaluateDerivative(cellIds, rst, result);
    return;
  }

  vtkIdType numEvals = cellIds->GetNumberOfTuples();
  vtkDoubleArray* dresult = vtkDoubleArray::SafeDownCast(result);
  if (!dresult)
  {
    dresult = this->LocalField.GetPointer();
  }

  vtkDGArraysInputAccessor inIt(cellIds, rst);
  vtkDGArrayOutputAccessor outIt(dresult);
  this->FieldDerivative.Evaluate(inIt, outIt, 0, numEvals);

  if (dresult != result)
  {
    result->DeepCopy(dresult);
  }
}

vtkSmartPointer<vtkCellAttributeCalculator> vtkDGInterpolateCalculator::PrepareForGrid(
  vtkCellMetadata* cell, vtkCellAttribute* field)
{
  auto* dgCell = vtkDGCell::SafeDownCast(cell);
  if (!dgCell)
  {
    return nullptr;
  }
  if (!field)
  {
    return nullptr;
  }

  // Clone ourselves for this new context.
  vtkNew<vtkDGInterpolateCalculator> result;

  result->CellType = dgCell;
  result->CellShape = dgCell->GetShape();
  result->Dimension = dgCell->GetDimension();
  result->FieldCellInfo = field->GetCellTypeInfo(dgCell->GetClassName());
  result->Field = field;
  result->FieldEvaluator.Prepare(dgCell, field, "Basis"_token, /* includeShape */ true);
  result->FieldDerivative.Prepare(dgCell, field, "BasisGradient"_token, /* includeShape */ true);

  return result;
}

VTK_ABI_NAMESPACE_END
