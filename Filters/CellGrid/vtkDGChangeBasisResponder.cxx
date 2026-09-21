// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDGChangeBasisResponder.h"

#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkCellGridChangeBasis.h"
#include "vtkDGCell.h"
#include "vtkDGOperatorEntry.h"
#include "vtkDataSetAttributes.h"
#include "vtkLagrangePoints.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"
#include "vtkStringToken.h"

#include "vtk_eigen.h"
#include VTK_EIGEN(Dense)

#include <algorithm>
#include <atomic>

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

vtkStandardNewMacro(vtkDGChangeBasisResponder);

namespace
{

/// Return the parametric coordinate of each degree of freedom of the basis that
/// \a cellTypeInfo describes, or false if that basis is not nodal.
bool DegreeOfFreedomPoints(vtkDGCell* cellType, vtkCellAttribute* attribute,
  const vtkCellAttribute::CellTypeInfo& cellTypeInfo, std::vector<std::vector<double>>& points)
{
  vtkNew<vtkCellAttribute> probe;
  probe->Initialize(
    attribute->GetName(), attribute->GetSpace(), attribute->GetNumberOfComponents());
  probe->SetCellTypeInfo(cellType->GetClassName(), cellTypeInfo);
  auto calculator = cellType->GetCaches()->AttributeCalculator<vtkLagrangePoints>(
    cellType, probe, cellType->GetAttributeTags(probe, true));
  return calculator && calculator->GetParameters(points);
}

/// Match each of \a target's points to the coincident point of \a source.
///
/// On success \a permutation maps a target degree of freedom to the source
/// degree of freedom occupying the same place in the reference cell, which is
/// therefore the same mesh entity and the same shared value. Returns false
/// unless the two sets coincide exactly, in which case the two bases do not
/// describe the same degrees of freedom and sharing cannot be carried over.
bool MatchPoints(const std::vector<std::vector<double>>& target,
  const std::vector<std::vector<double>>& source, std::vector<int>& permutation)
{
  constexpr double tol = 1e-10;
  constexpr double tol2 = tol * tol;
  if (target.size() != source.size())
  {
    return false;
  }
  permutation.assign(target.size(), -1);
  std::vector<bool> claimed(source.size(), false);
  for (std::size_t ii = 0; ii < target.size(); ++ii)
  {
    for (std::size_t jj = 0; jj < source.size(); ++jj)
    {
      if (claimed[jj] || target[ii].size() != source[jj].size())
      {
        continue;
      }
      double distance = 0.;
      for (std::size_t axis = 0; axis < target[ii].size(); ++axis)
      {
        const double delta = target[ii][axis] - source[jj][axis];
        distance += delta * delta;
      }
      if (distance < tol2)
      {
        permutation[ii] = static_cast<int>(jj);
        claimed[jj] = true;
        break;
      }
    }
    if (permutation[ii] < 0)
    {
      return false;
    }
  }
  return true;
}

} // anonymous namespace

bool vtkDGChangeBasisResponder::Query(
  vtkCellGridChangeBasis::Query* request, vtkCellMetadata* cellType, vtkCellGridResponders* caches)
{
  auto* dgCell = vtkDGCell::SafeDownCast(cellType);
  if (!dgCell)
  {
    return false;
  }
  return this->ChangeBasis(request, dgCell, caches);
}

bool vtkDGChangeBasisResponder::ChangeBasis(
  vtkCellGridChangeBasis::Query* request, vtkDGCell* cellType, vtkCellGridResponders* caches)
{
  (void)caches;
  auto* grid = cellType->GetCellGrid();
  if (!grid)
  {
    return false;
  }
  // Resolve the attribute against the grid being modified. The caller names an
  // attribute of the *input*, but this query runs on a copy of it, and mutating
  // the input's record would corrupt the filter's own input.
  auto* requested = request->GetCellAttribute();
  auto* attribute = requested ? grid->GetCellAttributeByName(requested->GetName().Data())
                              : grid->GetShapeAttribute();
  if (!attribute)
  {
    vtkErrorMacro("No cell-attribute to convert.");
    return false;
  }

  vtkStringToken cellTypeName(cellType->GetClassName());
  auto sourceInfo = attribute->GetCellTypeInfo(cellTypeName);

  // Cell types whose attribute is not in the requested source basis are left
  // alone; a grid may hold several cell types and only some may match.
  if (sourceInfo.FunctionSpace != request->GetSourceFunctionSpace() ||
    sourceInfo.Basis != request->GetSourceBasis() || sourceInfo.Order != request->GetSourceOrder())
  {
    return true;
  }

  // Describe the attribute as it will be once converted. The degrees of freedom
  // are not shared, so no connectivity array participates; an "order" array, if
  // present, carries the per-axis order across unchanged.
  vtkCellAttribute::CellTypeInfo targetInfo;
  targetInfo.FunctionSpace = request->GetTargetFunctionSpace();
  targetInfo.Basis = request->GetTargetBasis();
  targetInfo.Order = request->GetTargetOrder();
  auto orderArrayIt = sourceInfo.ArraysByRole.find("order"_token);
  if (orderArrayIt != sourceInfo.ArraysByRole.end() &&
    request->GetSourceOrder() == request->GetTargetOrder())
  {
    // The array holds one order per parametric axis. It describes the source,
    // so it only carries over when the target keeps the same order.
    targetInfo.ArraysByRole["order"_token] = orderArrayIt->second;
  }

  auto sourceOp = cellType->GetOperatorEntry("Basis"_token, sourceInfo);
  auto targetOp = cellType->GetOperatorEntry("Basis"_token, targetInfo);
  if (!sourceOp || !targetOp)
  {
    vtkErrorMacro("No basis operator for the source or target of the conversion.");
    return false;
  }
  if (sourceOp.OperatorSize != 1 || targetOp.OperatorSize != 1)
  {
    // H(div) and H(curl) bases are vector-valued, so a coefficient is not a
    // simple scalar multiple of a basis function and the linear system below
    // does not describe the conversion.
    vtkErrorMacro("Only scalar-valued bases may be converted.");
    return false;
  }

  int numSourceFunctions = sourceOp.NumberOfFunctions;
  int numTargetFunctions = targetOp.NumberOfFunctions;
  if (numTargetFunctions < numSourceFunctions)
  {
    // The target space is too small to hold every function the source can
    // represent, so the conversion would quietly discard part of the input.
    // That is what lowering the order asks for, and what an enriched ("F")
    // source asks for at the same order. Raising the order is exact and is
    // allowed.
    vtkErrorMacro("Cannot convert to a basis of "
      << numTargetFunctions << " functions from one of " << numSourceFunctions
      << ". The target must span at least what the source does, so the polynomial order may be "
         "raised but not lowered.");
    return false;
  }

  // Sample both bases at points that determine a function in the target basis.
  // The target's own degree-of-freedom locations are such a set, and asking for
  // them keeps the matrix below well conditioned.
  std::vector<std::vector<double>> parameters;
  if (!DegreeOfFreedomPoints(cellType, attribute, targetInfo, parameters))
  {
    vtkErrorMacro("Could not determine where the target basis' degrees of freedom lie.");
    return false;
  }
  if (static_cast<int>(parameters.size()) != numTargetFunctions)
  {
    vtkErrorMacro(
      "Expected " << numTargetFunctions << " sample points, got " << parameters.size() << ".");
    return false;
  }

  // Build the two matrices of David Thompson's relation
  //   B_old(x_i) c_old = B_new(x_i) c_new
  // and solve it once for the whole cell type: both bases are functions of the
  // parametric coordinates alone, so the transformation does not depend on any
  // cell's geometry or values.
  Eigen::MatrixXd targetValues(numTargetFunctions, numTargetFunctions);
  Eigen::MatrixXd sourceValues(numTargetFunctions, numSourceFunctions);
  std::vector<double> sourceRow(numSourceFunctions);
  std::vector<double> targetRow(numTargetFunctions);
  for (int ii = 0; ii < numTargetFunctions; ++ii)
  {
    std::array<double, 3> rst{ { 0., 0., 0. } };
    for (std::size_t axis = 0; axis < parameters[ii].size() && axis < 3; ++axis)
    {
      rst[axis] = parameters[ii][axis];
    }
    sourceOp.Evaluate(rst, sourceRow);
    targetOp.Evaluate(rst, targetRow);
    for (int jj = 0; jj < numSourceFunctions; ++jj)
    {
      sourceValues(ii, jj) = sourceRow[jj];
    }
    for (int jj = 0; jj < numTargetFunctions; ++jj)
    {
      targetValues(ii, jj) = targetRow[jj];
    }
  }

  // A singular-value decomposition rather than a faster factorization: this is
  // computed once per cell type and then applied to every cell, so accuracy is
  // worth more than speed, and raising the order gives a larger and less well
  // conditioned system than converting in place does.
  Eigen::JacobiSVD<Eigen::MatrixXd> solver(targetValues, Eigen::ComputeThinU | Eigen::ComputeThinV);
  if (solver.rank() < numTargetFunctions)
  {
    vtkErrorMacro("The target basis is singular at its own degrees of freedom.");
    return false;
  }
  // transform(i, j) is how much the j-th source coefficient contributes to the
  // i-th target coefficient.
  Eigen::MatrixXd transform = solver.solve(sourceValues);

  auto* values = sourceInfo.GetArrayForRoleAs<vtkDataArray>("values"_token);
  if (!values)
  {
    vtkErrorMacro("The attribute has no array in the \"values\" role.");
    return false;
  }
  bool sharedInput = sourceInfo.DOFSharing.IsValid();
  auto* connectivity = sourceInfo.GetArrayForRoleAs<vtkDataArray>("connectivity"_token);
  if (sharedInput && (!connectivity || connectivity->GetNumberOfComponents() != numSourceFunctions))
  {
    vtkErrorMacro("The attribute shares degrees of freedom but has no matching connectivity.");
    return false;
  }

  // The number of values each basis function carries: for shared degrees of
  // freedom that is the whole tuple, and otherwise one cell's tuple holds every
  // function's values end to end. In the latter case the tuple must divide
  // evenly, or the gather below would read more than it has room for.
  if (!sharedInput && values->GetNumberOfComponents() % numSourceFunctions != 0)
  {
    vtkErrorMacro("The attribute's " << values->GetNumberOfComponents()
                                     << " components are not a whole number of the basis' "
                                     << numSourceFunctions << " functions.");
    return false;
  }
  int numComponents = sharedInput ? values->GetNumberOfComponents()
                                  : values->GetNumberOfComponents() / numSourceFunctions;
  const vtkIdType numCells =
    sharedInput ? connectivity->GetNumberOfTuples() : values->GetNumberOfTuples();

  // Sharing carries over only when the two bases place their degrees of freedom
  // at the same points: a shared value belongs to a mesh entity, and only then
  // does each target degree of freedom describe the same entity as some source
  // one. That holds for a complete basis converted at its own order, and fails
  // for an incomplete or enriched one, whose degree-of-freedom count differs.
  //
  // permutation maps a target degree of freedom to the source degree of
  // freedom at the same point, and need not be the identity: the hand-written
  // fixed-order bases number their degrees of freedom corner-first, while the
  // arbitrary-order and Bezier bases number theirs lexicographically, so
  // converting between them at the same order reorders coincident points.
  std::vector<int> permutation;
  bool shareOutput = false;
  if (sharedInput && numSourceFunctions == numTargetFunctions)
  {
    std::vector<std::vector<double>> sourceParameters;
    shareOutput = DegreeOfFreedomPoints(cellType, attribute, sourceInfo, sourceParameters) &&
      MatchPoints(parameters, sourceParameters, permutation);
  }

  vtkSmartPointer<vtkDataArray> converted;

  if (!shareOutput)
  {
    // Give every cell its own coefficients.
    converted = vtk::TakeSmartPointer(request->CreateNewDataArray(values));
    converted->SetName(values->GetName());
    converted->SetNumberOfComponents(numTargetFunctions * numComponents);
    converted->SetNumberOfTuples(numCells);
    vtkSMPTools::For(0, numCells,
      [&](vtkIdType begin, vtkIdType end)
      {
        std::vector<double> coefficients(numSourceFunctions * numComponents);
        std::vector<double> result(numTargetFunctions * numComponents);
        std::vector<vtkTypeUInt64> dofIds(sharedInput ? numSourceFunctions : 0);
        for (vtkIdType cellId = begin; cellId < end; ++cellId)
        {
          if (!sharedInput)
          {
            values->GetTuple(cellId, coefficients.data());
          }
          else
          {
            connectivity->GetUnsignedTuple(cellId, dofIds.data());
            for (int ff = 0; ff < numSourceFunctions; ++ff)
            {
              values->GetTuple(
                static_cast<vtkIdType>(dofIds[ff]), coefficients.data() + ff * numComponents);
            }
          }

          // Both layouts store one basis function's components contiguously, so
          // the same indexing serves for input and output.
          for (int ii = 0; ii < numTargetFunctions; ++ii)
          {
            for (int cc = 0; cc < numComponents; ++cc)
            {
              double sum = 0.;
              for (int jj = 0; jj < numSourceFunctions; ++jj)
              {
                sum += transform(ii, jj) * coefficients[jj * numComponents + cc];
              }
              result[ii * numComponents + cc] = sum;
            }
          }
          converted->SetTuple(cellId, result.data());
        }
      });

    grid->GetAttributes(cellTypeName)->AddArray(converted);
    targetInfo.ArraysByRole["values"_token] = converted;
  }
  else
  {
    // Keep the degrees of freedom shared. Every cell meeting at a shared value
    // computes the same number for it, so it is enough for one of them to write
    // it. Assign each value to the lowest-numbered cell that references it, so
    // that cells can claim their values in parallel without racing.
    vtkIdType numDOF = values->GetNumberOfTuples();
    std::vector<std::atomic<vtkIdType>> owner(static_cast<std::size_t>(numDOF));
    vtkSMPTools::For(0, numDOF,
      [&](vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType dof = begin; dof < end; ++dof)
        {
          owner[dof].store(-1, std::memory_order_relaxed);
        }
      });
    vtkSMPTools::For(0, numCells,
      [&](vtkIdType begin, vtkIdType end)
      {
        std::vector<vtkTypeUInt64> dofIds(numSourceFunctions);
        for (vtkIdType cell = begin; cell < end; ++cell)
        {
          connectivity->GetUnsignedTuple(cell, dofIds.data());
          for (int ff = 0; ff < numSourceFunctions; ++ff)
          {
            auto& claim = owner[dofIds[ff]];
            vtkIdType expected = claim.load(std::memory_order_relaxed);
            while ((expected < 0 || cell < expected) &&
              !claim.compare_exchange_weak(expected, cell, std::memory_order_relaxed))
            {
            }
          }
        }
      });

    // Several cell types may share one group of degrees of freedom, and this
    // responder sees one cell type at a time. They must all write into a single
    // array, so create it only once and let the rest find it here. Values not
    // referenced by any cell are zeroed rather than left as whatever the
    // allocation happened to contain.
    auto& sharedOutputs = request->GetSharedOutputs();
    auto existing = sharedOutputs.find(values);
    if (existing != sharedOutputs.end())
    {
      converted = existing->second;
    }
    else
    {
      converted = vtk::TakeSmartPointer(request->CreateNewDataArray(values));
      converted->SetName(values->GetName());
      converted->SetNumberOfComponents(numComponents);
      converted->SetNumberOfTuples(numDOF);
      converted->Fill(0.);
      grid->GetAttributes(sourceInfo.DOFSharing)->AddArray(converted);
      sharedOutputs[values] = converted;
    }

    vtkSMPTools::For(0, numCells,
      [&](vtkIdType begin, vtkIdType end)
      {
        std::vector<double> coefficients(numSourceFunctions * numComponents);
        std::vector<double> result(numComponents);
        std::vector<vtkTypeUInt64> dofIds(numSourceFunctions);
        for (vtkIdType cellId = begin; cellId < end; ++cellId)
        {
          connectivity->GetUnsignedTuple(cellId, dofIds.data());
          const bool ownsAny = std::any_of(dofIds.begin(), dofIds.end(), [&](vtkTypeUInt64 dof)
            { return owner[dof].load(std::memory_order_relaxed) == cellId; });
          if (!ownsAny)
          {
            continue;
          }
          for (int ff = 0; ff < numSourceFunctions; ++ff)
          {
            values->GetTuple(
              static_cast<vtkIdType>(dofIds[ff]), coefficients.data() + ff * numComponents);
          }
          for (int ii = 0; ii < numTargetFunctions; ++ii)
          {
            const auto dof = static_cast<vtkIdType>(dofIds[permutation[ii]]);
            if (owner[dof].load(std::memory_order_relaxed) != cellId)
            {
              continue;
            }
            for (int cc = 0; cc < numComponents; ++cc)
            {
              double sum = 0.;
              for (int jj = 0; jj < numSourceFunctions; ++jj)
              {
                sum += transform(ii, jj) * coefficients[jj * numComponents + cc];
              }
              result[cc] = sum;
            }
            converted->SetTuple(dof, result.data());
          }
        }
      });

    // The connectivity must name the degrees of freedom in the target basis'
    // order. When the two bases already agree - as they do whenever both number
    // their degrees of freedom lexicographically - the input's array serves.
    vtkSmartPointer<vtkDataArray> targetConnectivity = connectivity;
    // permutation is a bijection on {0, ..., numTargetFunctions - 1}, so an
    // ascending one can only be the identity.
    const bool reordered = !std::is_sorted(permutation.begin(), permutation.end());
    if (reordered)
    {
      targetConnectivity = vtk::TakeSmartPointer(connectivity->NewInstance());
      targetConnectivity->SetName(connectivity->GetName());
      targetConnectivity->SetNumberOfComponents(numTargetFunctions);
      targetConnectivity->SetNumberOfTuples(numCells);
      vtkSMPTools::For(0, numCells,
        [&](vtkIdType begin, vtkIdType end)
        {
          std::vector<vtkTypeUInt64> sourceIds(numSourceFunctions);
          std::vector<vtkTypeUInt64> targetIds(numTargetFunctions);
          for (vtkIdType cellId = begin; cellId < end; ++cellId)
          {
            connectivity->GetUnsignedTuple(cellId, sourceIds.data());
            for (int ii = 0; ii < numTargetFunctions; ++ii)
            {
              targetIds[ii] = sourceIds[permutation[ii]];
            }
            targetConnectivity->SetUnsignedTuple(cellId, targetIds.data());
          }
        });
      grid->GetAttributes(cellTypeName)->AddArray(targetConnectivity);
    }

    targetInfo.DOFSharing = sourceInfo.DOFSharing;
    targetInfo.ArraysByRole["values"_token] = converted;
    targetInfo.ArraysByRole["connectivity"_token] = targetConnectivity;
  }

  attribute->SetCellTypeInfo(cellTypeName, targetInfo);
  return true;
}

VTK_ABI_NAMESPACE_END
