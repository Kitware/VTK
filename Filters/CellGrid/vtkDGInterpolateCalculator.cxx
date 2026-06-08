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

using namespace vtk::literals;

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkDGInterpolateCalculator);

vtkDGInterpolateCalculator::ThreadLocalData::ThreadLocalData()
{
  this->Result = vtkSmartPointer<vtkDoubleArray>::New();
  this->VRst = vtkSmartPointer<vtkDoubleArray>::New();
  this->Ids = vtkSmartPointer<vtkIdTypeArray>::New();

  this->DerResult = vtkSmartPointer<vtkDoubleArray>::New();
  this->DerVRst = vtkSmartPointer<vtkDoubleArray>::New();
  this->DerIds = vtkSmartPointer<vtkIdTypeArray>::New();

  this->LocalField = vtkSmartPointer<vtkDoubleArray>::New();

  this->InAccessor.SetCellIds(this->Ids.GetPointer());
  this->InAccessor.SetRST(this->VRst.GetPointer());
  this->OutAccessor.SetResult(this->Result.GetPointer());
  this->DerInAccessor.SetCellIds(this->DerIds.GetPointer());
  this->DerInAccessor.SetRST(this->DerVRst.GetPointer());
  this->DerOutAccessor.SetResult(this->DerResult.GetPointer());
}

vtkDGInterpolateCalculator::ThreadLocalData::ThreadLocalData(
  const ThreadLocalData& vtkNotUsed(other))
{
  this->Result = vtkSmartPointer<vtkDoubleArray>::New();
  this->VRst = vtkSmartPointer<vtkDoubleArray>::New();
  this->Ids = vtkSmartPointer<vtkIdTypeArray>::New();

  this->DerResult = vtkSmartPointer<vtkDoubleArray>::New();
  this->DerVRst = vtkSmartPointer<vtkDoubleArray>::New();
  this->DerIds = vtkSmartPointer<vtkIdTypeArray>::New();

  this->LocalField = vtkSmartPointer<vtkDoubleArray>::New();

  this->InAccessor.SetCellIds(this->Ids.GetPointer());
  this->InAccessor.SetRST(this->VRst.GetPointer());
  this->OutAccessor.SetResult(this->Result.GetPointer());
  this->DerInAccessor.SetCellIds(this->DerIds.GetPointer());
  this->DerInAccessor.SetRST(this->DerVRst.GetPointer());
  this->DerOutAccessor.SetResult(this->DerResult.GetPointer());
}

void vtkDGInterpolateCalculator::ThreadLocalData::Swap(ThreadLocalData& other)
{
  std::swap(this->Result, other.Result);
  std::swap(this->VRst, other.VRst);
  std::swap(this->Ids, other.Ids);

  std::swap(this->DerResult, other.DerResult);
  std::swap(this->DerVRst, other.DerVRst);
  std::swap(this->DerIds, other.DerIds);

  std::swap(this->LocalField, other.LocalField);

  std::swap(this->InAccessor, other.InAccessor);
  std::swap(this->OutAccessor, other.OutAccessor);
  std::swap(this->DerInAccessor, other.DerInAccessor);
  std::swap(this->DerOutAccessor, other.DerOutAccessor);
  std::swap(this->FieldEvaluator, other.FieldEvaluator);
  std::swap(this->FieldDerivative, other.FieldDerivative);
  std::swap(this->Initialized, other.Initialized);
}

vtkDGInterpolateCalculator::ThreadLocalData& vtkDGInterpolateCalculator::ThreadLocalData::operator=(
  ThreadLocalData other)
{
  if (this != &other)
  {
    ThreadLocalData tmp = ThreadLocalData(other);
    this->Swap(tmp);
  }
  return *this;
}

void vtkDGInterpolateCalculator::ThreadLocalData::EnsureInitialized(
  vtkDGCell* cellType, vtkCellAttribute* field)
{
  if (!this->Initialized)
  {
    this->FieldEvaluator.Prepare(cellType, field, "Basis"_token, /* includeShape */ true);
    this->FieldDerivative.Prepare(cellType, field, "BasisGradient"_token, /* includeShape */ true);
    this->Result->SetNumberOfComponents(this->FieldEvaluator.GetNumberOfResultComponents());
    this->VRst->SetNumberOfComponents(3);
    this->Ids->SetNumberOfValues(1);
    this->DerResult->SetNumberOfComponents(this->FieldDerivative.GetNumberOfResultComponents());
    this->DerVRst->SetNumberOfComponents(3);
    this->DerIds->SetNumberOfValues(1);
    this->Initialized = true;
  }
}

void vtkDGInterpolateCalculator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  vtkIndent i2 = indent.GetNextIndent();
  ThreadLocalData& tl = this->LocalData.Local();
  tl.EnsureInitialized(this->CellType, this->Field);
  os << indent << "FieldEvaluator:\n";
  tl.FieldEvaluator.PrintSelf(os, i2);
  os << indent << "FieldDerivative:\n";
  tl.FieldDerivative.PrintSelf(os, i2);

  // os << indent << "FieldValues: " << this->FieldValues << "\n";
  // os << indent << "ShapeConnectivity: " << this->ShapeConnectivity << "\n";
  // os << indent << "ShapeValues: " << this->ShapeValues << "\n";
}

void vtkDGInterpolateCalculator::Evaluate(
  vtkIdType cellId, const vtkVector3d& rst, std::vector<double>& value)
{
  ThreadLocalData& tl = this->LocalData.Local();
  tl.EnsureInitialized(this->CellType, this->Field);
  value.resize(tl.FieldEvaluator.GetNumberOfResultComponents());
  vtkDoubleArray* result = tl.Result.GetPointer();
  vtkDoubleArray* vrst = tl.VRst.GetPointer();
  vtkIdTypeArray* ids = tl.Ids.GetPointer();
  result->SetArray(value.data(), static_cast<vtkIdType>(value.size()), /*save*/ 1);
  vrst->SetArray(const_cast<double*>(rst.GetData()), 3, /*save*/ 1);
  ids->SetValue(0, cellId);
  tl.InAccessor.Restart();
  tl.OutAccessor.Restart();
  tl.FieldEvaluator.Evaluate(tl.InAccessor, tl.OutAccessor, 0, 1);
}

void vtkDGInterpolateCalculator::Evaluate(
  vtkIdTypeArray* cellIds, vtkDataArray* rst, vtkDataArray* result)
{
  ThreadLocalData& tl = this->LocalData.Local();
  vtkDoubleArray* dresult = vtkDoubleArray::SafeDownCast(result);
  if (!dresult)
  {
    dresult = tl.LocalField.GetPointer();
  }

  assert(cellIds->GetNumberOfTuples() == rst->GetNumberOfTuples());

  vtkIdType numEvals = cellIds->GetNumberOfTuples();
  dresult->SetNumberOfComponents(this->Field->GetNumberOfComponents());
  dresult->SetNumberOfTuples(cellIds->GetNumberOfTuples());
  tl.EnsureInitialized(this->CellType, this->Field);
  vtkDGArraysInputAccessor inIt(cellIds, rst);
  vtkDGArrayOutputAccessor outIt(dresult);
  tl.FieldEvaluator.Evaluate(inIt, outIt, 0, numEvals);

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

  ThreadLocalData& tl = this->LocalData.Local();
  tl.EnsureInitialized(this->CellType, this->Field);
  vtkDoubleArray* result = tl.DerResult.GetPointer();
  vtkDoubleArray* vrst = tl.DerVRst.GetPointer();
  vtkIdTypeArray* ids = tl.DerIds.GetPointer();
  result->SetArray(jacobian.data(), static_cast<vtkIdType>(jacobian.size()), /*save*/ 1);
  vrst->SetArray(const_cast<double*>(rst.GetData()), 3, /*save*/ 1);
  ids->SetValue(0, cellId);
  tl.DerInAccessor.Restart();
  tl.DerOutAccessor.Restart();
  tl.FieldDerivative.Evaluate(tl.DerInAccessor, tl.DerOutAccessor, 0, 1);
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
  ThreadLocalData& tl = this->LocalData.Local();
  vtkDoubleArray* dresult = vtkDoubleArray::SafeDownCast(result);
  if (!dresult)
  {
    dresult = tl.LocalField.GetPointer();
  }
  tl.EnsureInitialized(this->CellType, this->Field);
  vtkDGArraysInputAccessor inIt(cellIds, rst);
  vtkDGArrayOutputAccessor outIt(dresult);
  tl.FieldDerivative.Evaluate(inIt, outIt, 0, numEvals);

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
  for (auto& tlData : this->LocalData)
  {
    tlData.Initialized = false;
  }

  return result;
}

VTK_ABI_NAMESPACE_END
