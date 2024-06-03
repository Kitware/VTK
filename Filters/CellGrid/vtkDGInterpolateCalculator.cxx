// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDGInterpolateCalculator.h"
#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkCellMetadata.h"
#include "vtkDGInvokeOperator.h"
#include "vtkDGOperatorEntry.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkObjectFactory.h"
#include "vtkTypeInt64Array.h"
#include "vtkVectorOperators.h"

#include "vtk_eigen.h"
#include VTK_EIGEN(Eigen)

#include <sstream>

using namespace vtk::literals;

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkDGInterpolateCalculator);

void vtkDGInterpolateCalculator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FieldValues: " << this->FieldValues << "\n";
  os << indent << "ShapeConnectivity: " << this->ShapeConnectivity << "\n";
  os << indent << "ShapeValues: " << this->ShapeValues << "\n";
}

void vtkDGInterpolateCalculator::Evaluate(
  vtkIdType cellId, const vtkVector3d& rst, std::vector<double>& value)
{
  /* static thread_local */ vtkDGInvokeOperator computeValue;
  std::array<double, 3> arst{ rst[0], rst[1], rst[2] };
  // value.resize(this->FieldBasisOp.OperatorSize * );
  computeValue.Invoke(
    this->FieldBasisOp, this->FieldCellInfo, 1, &cellId, arst.data(), value.data());

  // Now, for H(curl) and H(div) elements, transform the resulting vectors
  // by the inverse Jacobian of the cell's *shape* function.
  static std::unordered_set<vtkStringToken> needsInverseJacobian{ { "HGRAD"_token, "HCURL"_token,
    "HGrad"_token, "HCurl"_token, "Hgrad"_token, "Hcurl"_token, "hgrad"_token, "hcurl"_token } };
  if (needsInverseJacobian.find(this->FieldCellInfo.FunctionSpace) != needsInverseJacobian.end())
  {
    /* static thread_local */ std::vector<double> spatialDeriv;
    this->InternalDerivative<true>(cellId, rst, spatialDeriv, 1e-3);

    // Use spatialDeriv as Jacobian and solve J * xx = value (which transforms "value"
    // from the parameter space into world coordinates), then write the results
    // back into value.
    Eigen::Map<Eigen::Matrix<double, 3, 3, Eigen::RowMajor>> map(spatialDeriv.data());
    Eigen::HouseholderQR<Eigen::Matrix3d> solver(map);
    std::size_t numValueVectors = value.size() / 3;
    for (std::size_t ii = 0; ii < numValueVectors; ++ii)
    {
      Eigen::Vector3d edelt(value[3 * ii + 0], value[3 * ii + 1], value[3 * ii + 2]);
      auto xx = solver.solve(edelt);
      for (int jj = 0; jj < 3; ++jj)
      {
        value[3 * ii + jj] = xx[jj];
      }
    }
  }
}

void vtkDGInterpolateCalculator::Evaluate(
  vtkIdTypeArray* cellIds, vtkDataArray* rst, vtkDataArray* result)
{
  // For H(curl) and H(div) elements, transform the resulting vectors
  // by the inverse Jacobian of the cell's *shape* function.
  static std::unordered_set<vtkStringToken> needsInverseJacobian{ { "HGRAD"_token, "HCURL"_token,
    "HGrad"_token, "HCurl"_token, "Hgrad"_token, "Hcurl"_token, "hgrad"_token, "hcurl"_token } };
  static thread_local vtkDGInvokeOperator computeValue;
  static thread_local vtkNew<vtkDoubleArray> localRST;
  static thread_local vtkNew<vtkDoubleArray> localField;
  vtkDoubleArray* drst = vtkDoubleArray::SafeDownCast(rst);
  // Convert parameters to doubles as needed.
  if (!drst)
  {
    localRST->DeepCopy(rst);
    drst = localRST.GetPointer();
  }
  vtkDoubleArray* dresult = vtkDoubleArray::SafeDownCast(result);
  if (!dresult)
  {
    dresult = localField.GetPointer();
  }

  bool invJac =
    (needsInverseJacobian.find(this->FieldCellInfo.FunctionSpace) != needsInverseJacobian.end());
  assert(cellIds->GetNumberOfTuples() == rst->GetNumberOfTuples());
  vtkIdType numEvals = cellIds->GetNumberOfTuples();
  dresult->SetNumberOfComponents(this->Field->GetNumberOfComponents());
  dresult->SetNumberOfTuples(cellIds->GetNumberOfTuples());
  computeValue.Invoke(this->FieldBasisOp, this->FieldCellInfo, numEvals, cellIds->GetPointer(0),
    drst->GetPointer(0), dresult->GetPointer(0));

  if (invJac)
  {
    if (!this->ShapeGradientOp)
    {
      throw std::runtime_error("No shape gradient.");
    }

    auto& cellInfo(this->ShapeCellInfo);
    static thread_local vtkDGInvokeOperator computeGradient;
    static thread_local vtkNew<vtkDoubleArray> jacobian;
    jacobian->SetNumberOfComponents(9); // really 3 * this->Dimension, but we promote 2D to 3D.
    jacobian->SetNumberOfTuples(numEvals);
    computeGradient.Invoke(this->ShapeGradientOp, cellInfo, numEvals, cellIds->GetPointer(0),
      drst->GetPointer(0), jacobian->GetPointer(0));

    // Now we need to invert each Jacobian and multiply each result
    // value by its respective inverse Jacobian.
    int nn = dresult->GetNumberOfComponents() / 3;
    double* dresultP = dresult->GetPointer(0);
    for (vtkIdType jj = 0; jj < numEvals; ++jj)
    {
      // Treat each tuple of jacobian as a matrix and solve J * xx = dresult[jj] (which
      // transforms "result" from the parameter space into world coordinates), then
      // write the results back into "dresult".
      Eigen::Map<Eigen::Matrix<double, 3, 3, Eigen::RowMajor>> map(jacobian->GetPointer(9 * jj));
      Eigen::HouseholderQR<Eigen::Matrix3d> solver(map);
      for (int ii = 0; ii < nn; ++ii)
      {
        Eigen::Vector3d edelt(dresultP[3 * (ii + nn * jj) + 0], dresultP[3 * (ii + nn * jj) + 1],
          dresultP[3 * (ii + nn * jj) + 2]);
        auto xx = solver.solve(edelt);
        for (int kk = 0; kk < 3; ++kk)
        {
          dresultP[3 * (ii + nn * jj) + kk] = xx[kk];
        }
      }
    }
    if (dresult == localField.GetPointer())
    {
      result->DeepCopy(dresult);
    }
  }
}

bool vtkDGInterpolateCalculator::AnalyticDerivative() const
{
  // XXX(c++14)
#if __cplusplus < 201400L
  if (this->FieldCellInfo.FunctionSpace == "HGRAD"_token)
  {
    return true;
  }
#else
  switch (this->FieldCellInfo.FunctionSpace.GetId())
  {
    case "HGRAD"_hash:
      return true;
    default:
      break;
  }
#endif
  return false;
}

template <bool UseShape>
void vtkDGInterpolateCalculator::InternalDerivative(
  vtkIdType cellId, const vtkVector3d& rst, std::vector<double>& jacobian, double neighborhood)
{
  auto& gradOp(UseShape ? this->ShapeGradientOp : this->FieldGradientOp);
  if (!gradOp.Op)
  {
    // The basis does not provide an analytical derivative; approximate it numerically.
    return this->Superclass::EvaluateDerivative(cellId, rst, jacobian, neighborhood);
  }

  auto& cellInfo(UseShape ? this->ShapeCellInfo : this->FieldCellInfo);
  static thread_local vtkDGInvokeOperator computeGradient;
  jacobian.resize(
    UseShape ? 3 * this->Dimension : this->FieldBasisOp.OperatorSize * this->Dimension);
  computeGradient.Invoke(gradOp, cellInfo, 1, &cellId, &rst[0], jacobian.data());
}

void vtkDGInterpolateCalculator::EvaluateDerivative(
  vtkIdType cellId, const vtkVector3d& rst, std::vector<double>& jacobian, double neighborhood)
{
  this->InternalDerivative<false>(cellId, rst, jacobian, neighborhood);
}

void vtkDGInterpolateCalculator::EvaluateDerivative(
  vtkIdTypeArray* cellIds, vtkDataArray* rst, vtkDataArray* result)
{
  if (!this->FieldGradientOp)
  {
    throw std::runtime_error("No shape gradient.");
  }

  vtkIdType numEvals = cellIds->GetNumberOfTuples();
  static thread_local vtkNew<vtkDoubleArray> localRST;
  vtkDoubleArray* drst = vtkDoubleArray::SafeDownCast(rst);
  // Convert parameters to doubles as needed.
  if (!drst)
  {
    localRST->DeepCopy(rst);
    drst = localRST.GetPointer();
  }
  static thread_local vtkNew<vtkDoubleArray> localField;
  vtkDoubleArray* dresult = vtkDoubleArray::SafeDownCast(result);
  if (!dresult)
  {
    dresult = localField.GetPointer();
  }

  auto& cellInfo(this->FieldCellInfo);
  static thread_local vtkDGInvokeOperator computeGradient;
  static thread_local vtkNew<vtkDoubleArray> jacobian;
  jacobian->SetNumberOfComponents(9); // really 3 * this->Dimension, but we promote 2D to 3D.
  jacobian->SetNumberOfTuples(numEvals);
  computeGradient.Invoke(this->FieldGradientOp, cellInfo, numEvals, cellIds->GetPointer(0),
    drst->GetPointer(0), dresult->GetPointer(0));

  if (dresult == localField.GetPointer())
  {
    result->DeepCopy(dresult);
  }

  // We may wish to multiple the result by the inverse Jacobian of the
  // shape attribute to get derivatives in world coordinates.
  // That should be an option to EvaluateDerivative().
#if 0
  // Now we need to invert each Jacobian and multiply each result
  // value by its respective inverse Jacobian.
  int nn = result->GetNumberOfComponents() / 3;
  double* resultP = result->GetPointer(0);
  for (vtkIdType jj = 0; jj < numEvals; ++jj)
  {
    // Treat each tuple of jacobian as a matrix and solve J * xx = result[jj] (which
    // transforms "result" from the parameter space into world coordinates), then
    // write the results back into "result".
    Eigen::Map<Eigen::Matrix<double, 3, 3, Eigen::RowMajor>> map(jacobian->GetPointer(9 * jj));
    Eigen::HouseholderQR<Eigen::Matrix3d> solver(map);
    std::size_t numValueVectors = value.size() / 3;
    for (int ii = 0; ii < nn; ++ii)
    {
      Eigen::Vector3d edelt(resultP[3 * (ii + nn * jj) + 0], resultP[3 * (ii + nn * jj) + 1], resultP[3 * (ii + nn * jj) + 2]);
      auto xx = solver.solve(edelt);
      for (int kk = 0; kk < 3; ++kk)
      {
        resultP[3 * (ii + nn * jj) + kk] = xx[kk];
      } 
    }
  }
#endif
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

  auto* grid = cell->GetCellGrid();
  auto* shape = grid->GetShapeAttribute();
  vtkStringToken cellType = dgCell->GetClassName();
  auto shapeCellInfo = shape->GetCellTypeInfo(cellType);
  // Shape functions must be (1) continuous and (2) have HGRAD/Lagrange basis
  // or, if the shape is a vtkDGVert, be constant and have a trivially null gradient.
  if ((cellType != "vtkDGVert"_token && shapeCellInfo.FunctionSpace != "HGRAD"_token &&
        shapeCellInfo.FunctionSpace != "Lagrange"_token) ||
    (cellType == "vtkDGVert"_token && shapeCellInfo.FunctionSpace != "constant"_token))
  {
    vtkErrorMacro("Unsupported combination of cell shape function "
      << "space \"" << shapeCellInfo.FunctionSpace.Data() << "\" and/or "
      << "DOF sharing (" << (shapeCellInfo.DOFSharing.IsValid() ? "C" : "D") << ").");
    return nullptr;
  }
  result->ShapeBasisOp = dgCell->GetOperatorEntry("Basis"_token, shapeCellInfo);
  result->ShapeGradientOp = dgCell->GetOperatorEntry("BasisGradient"_token, shapeCellInfo);
  result->ShapeCellInfo = shapeCellInfo;

  auto fieldCellInfo = field->GetCellTypeInfo(cellType);
  result->Field = field;
  result->FieldBasisOp = dgCell->GetOperatorEntry("Basis"_token, fieldCellInfo);
  result->FieldGradientOp = dgCell->GetOperatorEntry("BasisGradient"_token, fieldCellInfo);
  result->FieldCellInfo = fieldCellInfo;

  result->Dimension = dgCell->GetDimension();
  result->CellShape = dgCell->GetShape();

  auto& shapeArrays = shapeCellInfo.ArraysByRole;
  result->ShapeValues = vtkDataArray::SafeDownCast(shapeArrays["values"_token]);
  result->ShapeConnectivity = shapeCellInfo.DOFSharing.IsValid()
    ? vtkDataArray::SafeDownCast(shapeArrays["connectivity"_token])
    : nullptr;

  auto& fieldArrays = fieldCellInfo.ArraysByRole;
  result->FieldValues = vtkDataArray::SafeDownCast(fieldArrays["values"_token]);
  result->FieldConnectivity = fieldCellInfo.DOFSharing.IsValid()
    ? vtkDataArray::SafeDownCast(fieldArrays["connectivity"_token])
    : nullptr;

  if (shapeCellInfo.DOFSharing.IsValid())
  {
    if (!result->ShapeConnectivity || !result->ShapeConnectivity->IsIntegral())
    {
      vtkErrorMacro("Shape connectivity array must exist and be integer-valued.");
      return nullptr;
    }
  }

  if (fieldCellInfo.DOFSharing.IsValid())
  {
    if (!result->FieldConnectivity || !result->FieldConnectivity->IsIntegral())
    {
      vtkErrorMacro("Field connectivity array must exist and be integer-valued.");
      return nullptr;
    }
  }

  return result;
}

VTK_ABI_NAMESPACE_END
