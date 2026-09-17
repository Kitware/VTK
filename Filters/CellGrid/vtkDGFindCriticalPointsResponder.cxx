// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDGFindCriticalPointsResponder.h"

#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkCellGridFindCriticalPoints.h"
#include "vtkDGBezierFindCriticalPoints.h"
#include "vtkDGCell.h"
#include "vtkDGInterpolateCalculator.h"
#include "vtkDGOperatorEntry.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSMPTools.h"
#include "vtkStringToken.h"
#include "vtkTypeUInt32Array.h"

#include "vtk_eigen.h"
#include VTK_EIGEN(Dense)

#include <array>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

vtkStandardNewMacro(vtkDGFindCriticalPointsResponder);

namespace
{

/// Classify a converged critical point from the sign of its Hessian,
/// estimated by central-differencing the (analytic) gradient that
/// vtk::basis::bezier::EvaluateField() reports.
int Classify(vtkDGOperatorEntry& basisOp, vtkDGOperatorEntry& gradOp, int dimension,
  const std::vector<double>& coefficients, const std::array<double, 3>& rst)
{
  constexpr double h = 1e-4;
  Eigen::MatrixXd hessian(dimension, dimension);
  for (int axis = 0; axis < dimension; ++axis)
  {
    std::array<double, 3> plus = rst;
    std::array<double, 3> minus = rst;
    plus[axis] += h;
    minus[axis] -= h;
    std::array<double, 3> gradPlus;
    std::array<double, 3> gradMinus;
    vtk::basis::bezier::EvaluateField(basisOp, gradOp, dimension, coefficients, plus, gradPlus);
    vtk::basis::bezier::EvaluateField(basisOp, gradOp, dimension, coefficients, minus, gradMinus);
    for (int b = 0; b < dimension; ++b)
    {
      hessian(b, axis) = (gradPlus[b] - gradMinus[b]) / (2. * h);
    }
  }
  hessian = 0.5 * (hessian + hessian.transpose());
  Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> eig(hessian);
  const Eigen::VectorXd& evals = eig.eigenvalues();
  bool allPositive = true;
  bool allNegative = true;
  for (int axis = 0; axis < dimension; ++axis)
  {
    const double lambda = evals[axis];
    allPositive &= lambda > 0.;
    allNegative &= lambda < 0.;
  }
  if (allPositive)
  {
    return 0; // Minimum
  }
  if (allNegative)
  {
    return 1; // Maximum
  }
  return 2; // Saddle
}

} // anonymous namespace

bool vtkDGFindCriticalPointsResponder::Query(vtkCellGridFindCriticalPoints::Query* query,
  vtkCellMetadata* cellType, vtkCellGridResponders* caches)
{
  (void)caches;
  auto* dgCell = vtkDGCell::SafeDownCast(cellType);
  if (!dgCell)
  {
    return false;
  }
  switch (query->GetPass())
  {
    case vtkCellGridFindCriticalPoints::Query::PassType::CountOutputs:
      return this->CountCriticalPoints(query, dgCell);
    case vtkCellGridFindCriticalPoints::Query::PassType::AllocateOutputs:
      query->Allocate();
      return true;
    case vtkCellGridFindCriticalPoints::Query::PassType::GenerateOutputs:
      return this->GenerateCriticalPoints(query, dgCell);
    default:
      break;
  }
  return true;
}

bool vtkDGFindCriticalPointsResponder::CountCriticalPoints(
  vtkCellGridFindCriticalPoints::Query* query, vtkDGCell* cellType)
{
  auto* attribute = query->GetCellAttribute();
  if (!attribute)
  {
    vtkErrorMacro("No cell-attribute to search.");
    return false;
  }

  vtkStringToken cellTypeName(cellType->GetClassName());
  auto info = attribute->GetCellTypeInfo(cellTypeName);

  // Cell types whose attribute is not Bezier are left alone: a grid may hold
  // several cell types and only some may be in a basis whose coefficients
  // bound the field the way this search relies on.
  if (info.FunctionSpace != "Bezier"_token)
  {
    return true;
  }

  // Tensor-product shapes (vertex, edge, quadrilateral, hexahedron),
  // barycentric ones (triangle, tetrahedron), and the mixed ones: wedges (a
  // barycentric cross-section extruded along a prismatic axis) and pyramids
  // (a square cross-section collapsing to the apex).
  const vtkDGCell::Shape shape = cellType->GetShape();

  auto basisOp = cellType->GetOperatorEntry("Basis"_token, info);
  auto gradOp = cellType->GetOperatorEntry("BasisGradient"_token, info);
  if (!basisOp || !gradOp)
  {
    vtkErrorMacro("No Bezier basis operator for this cell type.");
    return false;
  }

  std::vector<int> order = cellType->GetBasisOrder(info);
  const int dimension = static_cast<int>(order.size());
  if (dimension < 1)
  {
    // Vertices have no interior or boundary to search.
    return true;
  }

  const int numFunctions = basisOp.NumberOfFunctions;

  auto* values = info.GetArrayForRoleAs<vtkDataArray>("values"_token);
  if (!values)
  {
    vtkErrorMacro("The attribute has no array in the \"values\" role.");
    return false;
  }
  const bool shared = info.DOFSharing.IsValid();
  auto* connectivity = info.GetArrayForRoleAs<vtkDataArray>("connectivity"_token);
  if (shared && (!connectivity || connectivity->GetNumberOfComponents() != numFunctions))
  {
    vtkErrorMacro("The attribute shares degrees of freedom but has no matching connectivity.");
    return false;
  }
  const int numComponents =
    shared ? values->GetNumberOfComponents() : values->GetNumberOfComponents() / numFunctions;
  if (numComponents != 1)
  {
    vtkErrorMacro("Only scalar attributes are supported so far; this one has " << numComponents
                                                                               << " components.");
    return false;
  }
  const vtkIdType numCells =
    shared ? connectivity->GetNumberOfTuples() : values->GetNumberOfTuples();

  std::vector<std::vector<vtk::basis::bezier::CriticalPoint>> criticalPointsByCell(numCells);
  std::vector<std::vector<int>> typesByCell(numCells);
  // Every cell's solve reads its own coefficients and writes only its own
  // slot of criticalPointsByCell/typesByCell, so cells may be solved
  // concurrently; basisOp/gradOp are read-only here (only Evaluate()'d,
  // never rebound). Classifying here too, while coefficients are already
  // at hand, is what lets GenerateOutputs skip re-extracting them.
  vtkSMPTools::For(0, numCells,
    [&](vtkIdType begin, vtkIdType end)
    {
      std::vector<vtkTypeUInt64> dofIds(shared ? numFunctions : 0);
      std::vector<double> coefficients(numFunctions);
      for (vtkIdType cellId = begin; cellId < end; ++cellId)
      {
        if (!shared)
        {
          values->GetTuple(cellId, coefficients.data());
        }
        else
        {
          connectivity->GetUnsignedTuple(cellId, dofIds.data());
          for (int ff = 0; ff < numFunctions; ++ff)
          {
            coefficients[ff] = values->GetComponent(static_cast<vtkIdType>(dofIds[ff]), 0);
          }
        }

        auto& found = criticalPointsByCell[cellId] =
          vtk::basis::bezier::FindCriticalPoints(shape, basisOp, gradOp, order, coefficients);

        auto& types = typesByCell[cellId];
        types.reserve(found.size());
        for (const auto& point : found)
        {
          types.push_back(Classify(basisOp, gradOp, dimension, coefficients, point.Parameter));
        }
      }
    });

  query->SetCriticalPoints(cellTypeName, std::move(criticalPointsByCell));
  query->SetCriticalPointTypes(cellTypeName, std::move(typesByCell));
  return true;
}

bool vtkDGFindCriticalPointsResponder::GenerateCriticalPoints(
  vtkCellGridFindCriticalPoints::Query* query, vtkDGCell* cellType)
{
  vtkStringToken cellTypeName(cellType->GetClassName());
  const auto& criticalPointsByCell = query->GetCriticalPoints(cellTypeName);
  const auto& typesByCell = query->GetCriticalPointTypes(cellTypeName);
  if (criticalPointsByCell.empty())
  {
    // Nothing was cached for this cell type in the CountOutputs pass: either
    // it is not one this responder handles, or it has no critical points.
    return true;
  }

  auto* grid = cellType->GetCellGrid();
  if (!grid)
  {
    return false;
  }

  auto* shapeAttribute = grid->GetShapeAttribute();
  if (!shapeAttribute)
  {
    vtkErrorMacro("The cell grid has no shape attribute to place critical points with.");
    return false;
  }
  vtkNew<vtkDGInterpolateCalculator> shapeCalcProto;
  auto rawShapeCalc = shapeCalcProto->PrepareForGrid(cellType, shapeAttribute);
  auto* shapeCalc = vtkDGInterpolateCalculator::SafeDownCast(rawShapeCalc);
  if (!shapeCalc)
  {
    vtkErrorMacro("Could not prepare a calculator for the grid's shape attribute.");
    return false;
  }

  auto* output = query->GetOutput();
  auto* outputCellTypes =
    vtkTypeUInt32Array::SafeDownCast(output->GetPointData()->GetArray("CellType"));
  auto* outputCellIds = vtkIdTypeArray::SafeDownCast(output->GetPointData()->GetArray("CellId"));
  auto* outputRst = vtkDoubleArray::SafeDownCast(output->GetPointData()->GetArray("RST"));
  auto* outputValues = vtkDoubleArray::SafeDownCast(output->GetPointData()->GetArray("Value"));
  auto* outputTypes = vtkIntArray::SafeDownCast(output->GetPointData()->GetArray("Type"));

  const vtkIdType cellTypeOffset = query->GetCellTypeOffset(cellTypeName);
  const vtkIdType numCells = static_cast<vtkIdType>(criticalPointsByCell.size());
  vtkSMPTools::For(0, numCells,
    [&](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType cellId = begin; cellId < end; ++cellId)
      {
        const auto& criticalPoints = criticalPointsByCell[cellId];
        if (criticalPoints.empty())
        {
          continue;
        }
        const auto& types = typesByCell[cellId];

        vtkIdType writeIndex = query->GetCellOffset(cellTypeOffset, cellId);
        for (std::size_t i = 0; i < criticalPoints.size(); ++i)
        {
          const auto& criticalPoint = criticalPoints[i];
          outputCellTypes->SetValue(writeIndex, cellTypeName.GetId());
          outputCellIds->SetValue(writeIndex, cellId);
          outputRst->SetTypedTuple(writeIndex, criticalPoint.Parameter.data());
          outputValues->SetValue(writeIndex, criticalPoint.Value);
          outputTypes->SetValue(writeIndex, types[i]);
          ++writeIndex;
        }
      }
    });

  // One batched Evaluate() over this cell type's whole slice, instead of
  // one call per point.
  const vtkIdType start = query->GetCellOffset(cellTypeOffset, 0);
  const vtkIdType count = query->GetCellOffset(cellTypeOffset, numCells) - start;
  if (count > 0)
  {
    vtkNew<vtkIdTypeArray> cellIdsWindow;
    cellIdsWindow->SetArray(outputCellIds->GetPointer(start), count, /*save*/ 1);
    vtkNew<vtkDoubleArray> rstWindow;
    rstWindow->SetNumberOfComponents(3);
    rstWindow->SetArray(outputRst->GetPointer(3 * start), 3 * count, /*save*/ 1);
    vtkNew<vtkDoubleArray> pointsWindow;
    pointsWindow->SetNumberOfComponents(3);
    auto* outputPointsData = vtkDoubleArray::SafeDownCast(output->GetPoints()->GetData());
    pointsWindow->SetArray(outputPointsData->GetPointer(3 * start), 3 * count, /*save*/ 1);
    shapeCalc->Evaluate(cellIdsWindow, rstWindow, pointsWindow, /*useMultithreading*/ true);
  }

  return true;
}

VTK_ABI_NAMESPACE_END
