// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkDGOperation_txx
#define vtkDGOperation_txx
#include "vtkDGOperation.h"

#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkDGCell.h"
#include "vtkDGOperationEvaluator.h"
#include "vtkDoubleArray.h"
#include "vtkMatrix3x3.h"

VTK_ABI_NAMESPACE_BEGIN

template <typename InputIterator, typename OutputIterator>
vtkDGOperation<InputIterator, OutputIterator>::vtkDGOperation(const SelfType& other)
{
  this->NumberOfResultComponents = other.NumberOfResultComponents;
  this->Evaluators.clear();
  for (const auto& otherEval : other.Evaluators)
  {
    this->Evaluators[otherEval.first] = otherEval.second;
  }
}

template <typename InputIterator, typename OutputIterator>
vtkDGOperation<InputIterator, OutputIterator>::vtkDGOperation(
  vtkDGCell* cellType, vtkCellAttribute* cellAttribute, vtkStringToken operationName)
{
  if (!this->Prepare(cellType, cellAttribute, operationName))
  {
    this->Evaluators.clear();
    throw std::logic_error("Null or invalid inputs.");
  }
}

template <typename InputIterator, typename OutputIterator>
void vtkDGOperation<InputIterator, OutputIterator>::PrintSelf(std::ostream& os, vtkIndent indent)
{
  os << indent << "Evaluators: " << this->Evaluators.size() << "\n";
  vtkIndent i2 = indent.GetNextIndent();
  for (const auto& entry : this->Evaluators)
  {
    os << i2 << "[" << entry.first.Begin << ", " << entry.first.End << "[  "
       << (entry.second.Function ? "non-null" : "null") << "\n";
  }
}

template <typename InputIterator, typename OutputIterator>
bool vtkDGOperation<InputIterator, OutputIterator>::Prepare(vtkDGCell* cellType,
  vtkCellAttribute* cellAttribute, vtkStringToken operationName, bool includeShape)
{
  using namespace vtk::literals;

  this->NumberOfResultComponents = 0;
  if (!cellType || !cellAttribute || !operationName.IsValid())
  {
    return false;
  }
  auto* grid = cellType->GetCellGrid();
  if (!grid || !grid->GetShapeAttribute())
  {
    return false;
  }
  auto cellTypeInfo = cellAttribute->GetCellTypeInfo(cellType->GetClassName());
  auto opEntry = cellType->GetOperatorEntry(operationName, cellTypeInfo);
  if (!opEntry)
  {
    return false;
  }

  // Figure out the number of components per entry in the result.
  // When operationName == "Basis", this is simply the number of
  // components in the cell-attribute. But for "GradientBasis" and
  // other operators, it depends on the product of two matrices
  // of varying sizes.
  vtkAbstractArray* values = cellTypeInfo.ArraysByRole["values"_token];
  int numValPerFunction =
    values ? values->GetNumberOfComponents() : cellAttribute->GetNumberOfComponents();
  if (!cellTypeInfo.DOFSharing.IsValid())
  {
    numValPerFunction = numValPerFunction / opEntry.NumberOfFunctions;
  }
  this->NumberOfResultComponents = opEntry.OperatorSize * numValPerFunction;

  this->AddSource(grid, cellType, std::numeric_limits<std::size_t>::max(), cellAttribute,
    cellTypeInfo, opEntry, includeShape);
  std::size_t numSideSpecs = cellType->GetSideSpecs().size();
  for (std::size_t sideSpecIdx = 0; sideSpecIdx < numSideSpecs; ++sideSpecIdx)
  {
    this->AddSource(
      grid, cellType, sideSpecIdx, cellAttribute, cellTypeInfo, opEntry, includeShape);
  }
  return true;
}

template <typename InputIterator, typename OutputIterator>
bool vtkDGOperation<InputIterator, OutputIterator>::Evaluate(
  InputIterator& inIter, OutputIterator& outIter, vtkTypeUInt64 begin, vtkTypeUInt64 end)
{
  bool ok = true;
  vtkDGCellRangeEvaluator<InputIterator, OutputIterator> currEval;
  RangeKey key;
  typename std::map<RangeKey, OpEvalEntry>::const_iterator eit;
  for (vtkTypeUInt64 ii = begin; ii < end; /* do nothing */)
  {
    key.Begin = inIter.GetCellId(ii);
    eit = FindEvaluator(key, this->Evaluators);
    if (eit == this->Evaluators.end() || !eit->first.Contains(key.Begin))
    {
      vtkGenericWarningMacro(
        "Invalid cell ID " << key.Begin << " at index " << ii << ". Skipping.");
      ok = false;
      // Advance to the next cell ID.
      ++ii;
      continue;
    }
    currEval = eit->second.Function;
    // Now see how many sequential entries in cellIds we can process:
    vtkTypeUInt64 jj;
    for (jj = ii + 1; jj < end; ++jj)
    {
      key.End = inIter.GetCellId(jj);
      if (!eit->first.Contains(key.End))
      {
        break;
      }
    }
    // Invoke the evaluator:
    currEval(inIter, outIter, ii, jj);
    // Advance to the next range. (jj > ii, so this will never stall.)
    ii = jj;
  }
  return ok;
}

template <typename InputIterator, typename OutputIterator>
typename vtkDGOperation<InputIterator, OutputIterator>::OpEval
vtkDGOperation<InputIterator, OutputIterator>::GetEvaluatorForSideSpec(
  vtkDGCell* cell, int sideSpecId)
{
  if (!cell || sideSpecId < -1 || sideSpecId >= static_cast<int>(cell->GetSideSpecs().size()))
  {
    return nullptr;
  }

  const auto& spec(sideSpecId == -1 ? cell->GetCellSpec() : cell->GetSideSpecs()[sideSpecId]);
  if (spec.Blanked)
  {
    return nullptr;
  }
  auto it = this->Evaluators.find({ static_cast<vtkTypeUInt64>(spec.Offset), 0 });
  if (it == this->Evaluators.end())
  {
    return nullptr;
  }
  return it->second.Function;
}

template <typename InputIterator, typename OutputIterator>
void vtkDGOperation<InputIterator, OutputIterator>::AddSource(vtkCellGrid* grid,
  vtkDGCell* cellType, std::size_t sideSpecIdx, vtkCellAttribute* cellAtt,
  const vtkCellAttribute::CellTypeInfo& cellTypeInfo, vtkDGOperatorEntry& op, bool includeShape)
{
  (void)cellAtt;
  (void)includeShape;
  using namespace vtk::literals;

  const auto& cellSpec(cellType->GetCellSpec());
  bool isCellSpec = sideSpecIdx == std::numeric_limits<std::size_t>::max();
  const auto& source(isCellSpec ? cellType->GetCellSpec() : cellType->GetSideSpecs()[sideSpecIdx]);
  if (source.Blanked)
  {
    return; // Cannot evaluate blanked cells.
  }
  bool sharedDOF = cellTypeInfo.DOFSharing.IsValid();
  auto* values = cellTypeInfo.GetArrayForRoleAs<vtkDataArray>("values"_token);
  bool shapeSharing = false;
  vtkDataArray* shapeConn = nullptr;
  vtkDataArray* shapeValues = nullptr;
  vtkDGOperatorEntry shapeGradient;
  vtkDGShapeModifier shapeMod = None;
  if (includeShape)
  {
    if (cellTypeInfo.FunctionSpace == "HCURL")
    {
      shapeMod = InverseJacobian;
    }
    else if (cellTypeInfo.FunctionSpace == "HDIV")
    {
      shapeMod = ScaledJacobian;
    }
  }
  if (shapeMod != None)
  {
    auto shapeTypeInfo = grid->GetShapeAttribute()->GetCellTypeInfo(cellType->GetClassName());
    shapeSharing = shapeTypeInfo.DOFSharing.IsValid();
    shapeConn = shapeTypeInfo.GetArrayForRoleAs<vtkDataArray>("connectivity"_token);
    shapeValues = shapeTypeInfo.GetArrayForRoleAs<vtkDataArray>("values"_token);
    shapeGradient = cellType->GetOperatorEntry("BasisGradient"_token, shapeTypeInfo);
    if (!shapeGradient)
    {
      throw std::logic_error("No gradient operation for shape attribute.");
    }
  }
  OpEvalEntry entry;
  // This is one huge ugly template-parameter dispatch.
  if (isCellSpec)
  {
    if (sharedDOF)
    {
      // Continuous field DOF.
      switch (shapeMod)
      {
        case None:
        {
          vtkDGOperationEvaluator<InputIterator, OutputIterator, SharedDOF, Cells, None>::prepEntry(
            entry, op, cellSpec.Connectivity, values, nullptr, source.Offset);
        }
        break;
        case InverseJacobian:
        {
          if (shapeSharing)
          {
            vtkDGOperationEvaluator<InputIterator, OutputIterator, SharedDOF, Cells,
              InverseJacobian, SharedDOF>::prepEntry(entry, op, cellSpec.Connectivity, values,
              nullptr, source.Offset, shapeGradient, shapeConn, shapeValues);
          }
          else
          {
            vtkDGOperationEvaluator<InputIterator, OutputIterator, SharedDOF, Cells,
              InverseJacobian, Discontinuous>::prepEntry(entry, op, cellSpec.Connectivity, values,
              nullptr, source.Offset, shapeGradient, nullptr, shapeValues);
          }
        }
        break;
        case ScaledJacobian:
        {
          if (shapeSharing)
          {
            vtkDGOperationEvaluator<InputIterator, OutputIterator, SharedDOF, Cells, ScaledJacobian,
              SharedDOF>::prepEntry(entry, op, cellSpec.Connectivity, values, nullptr,
              source.Offset, shapeGradient, shapeConn, shapeValues);
          }
          else
          {
            vtkDGOperationEvaluator<InputIterator, OutputIterator, SharedDOF, Cells, ScaledJacobian,
              Discontinuous>::prepEntry(entry, op, cellSpec.Connectivity, values, nullptr,
              source.Offset, shapeGradient, nullptr, shapeValues);
          }
        }
        break;
      }
    }
    else
    {
      // Discontinuous field DOF
      switch (shapeMod)
      {
        case None:
        {
          vtkDGOperationEvaluator<InputIterator, OutputIterator, Discontinuous, Cells,
            None>::prepEntry(entry, op, nullptr, values, nullptr, source.Offset);
        }
        break;
        case InverseJacobian:
        {
          if (shapeSharing)
          {
            vtkDGOperationEvaluator<InputIterator, OutputIterator, Discontinuous, Cells,
              InverseJacobian, SharedDOF>::prepEntry(entry, op, nullptr, values, nullptr,
              source.Offset, shapeGradient, cellSpec.Connectivity, shapeValues);
          }
          else
          {
            vtkDGOperationEvaluator<InputIterator, OutputIterator, Discontinuous, Cells,
              InverseJacobian, Discontinuous>::prepEntry(entry, op, nullptr, values, nullptr,
              source.Offset, shapeGradient, nullptr, shapeValues);
          }
        }
        break;
        case ScaledJacobian:
        {
          if (shapeSharing)
          {
            vtkDGOperationEvaluator<InputIterator, OutputIterator, Discontinuous, Cells,
              ScaledJacobian, SharedDOF>::prepEntry(entry, op, nullptr, values, nullptr,
              source.Offset, shapeGradient, cellSpec.Connectivity, shapeValues);
          }
          else
          {
            vtkDGOperationEvaluator<InputIterator, OutputIterator, Discontinuous, Cells,
              ScaledJacobian, Discontinuous>::prepEntry(entry, op, nullptr, values, nullptr,
              source.Offset, shapeGradient, nullptr, shapeValues);
          }
        }
        break;
      }
    }
  }
  else
  {
    // Processing sides, not cells:
    if (sharedDOF)
    {
      // Continuous field DOF.
      switch (shapeMod)
      {
        case None:
        {
          vtkDGOperationEvaluator<InputIterator, OutputIterator, SharedDOF, Sides, None>::prepEntry(
            entry, op, cellSpec.Connectivity, values, source.Connectivity, source.Offset);
        }
        break;
        case InverseJacobian:
        {
          if (shapeSharing)
          {
            vtkDGOperationEvaluator<InputIterator, OutputIterator, SharedDOF, Sides,
              InverseJacobian, SharedDOF>::prepEntry(entry, op, cellSpec.Connectivity, values,
              source.Connectivity, source.Offset, shapeGradient, shapeConn, shapeValues);
          }
          else
          {
            vtkDGOperationEvaluator<InputIterator, OutputIterator, SharedDOF, Sides,
              InverseJacobian, Discontinuous>::prepEntry(entry, op, cellSpec.Connectivity, values,
              source.Connectivity, source.Offset, shapeGradient, nullptr, shapeValues);
          }
        }
        break;
        case ScaledJacobian:
        {
          if (shapeSharing)
          {
            vtkDGOperationEvaluator<InputIterator, OutputIterator, SharedDOF, Sides, ScaledJacobian,
              SharedDOF>::prepEntry(entry, op, cellSpec.Connectivity, values, source.Connectivity,
              source.Offset, shapeGradient, shapeConn, shapeValues);
          }
          else
          {
            vtkDGOperationEvaluator<InputIterator, OutputIterator, SharedDOF, Sides, ScaledJacobian,
              Discontinuous>::prepEntry(entry, op, cellSpec.Connectivity, values,
              source.Connectivity, source.Offset, shapeGradient, nullptr, shapeValues);
          }
        }
        break;
      }
    }
    else
    {
      // Discontinuous field DOF.
      switch (shapeMod)
      {
        case None:
        {
          vtkDGOperationEvaluator<InputIterator, OutputIterator, Discontinuous, Sides,
            None>::prepEntry(entry, op, nullptr, values, source.Connectivity, source.Offset);
        }
        break;
        case InverseJacobian:
        {
          if (shapeSharing)
          {
            vtkDGOperationEvaluator<InputIterator, OutputIterator, Discontinuous, Sides,
              InverseJacobian, SharedDOF>::prepEntry(entry, op, nullptr, values,
              source.Connectivity, source.Offset, shapeGradient, shapeConn, shapeValues);
          }
          else
          {
            vtkDGOperationEvaluator<InputIterator, OutputIterator, Discontinuous, Sides,
              InverseJacobian, Discontinuous>::prepEntry(entry, op, nullptr, values,
              source.Connectivity, source.Offset, shapeGradient, nullptr, shapeValues);
          }
        }
        break;
        case ScaledJacobian:
        {
          if (shapeSharing)
          {
            vtkDGOperationEvaluator<InputIterator, OutputIterator, Discontinuous, Sides,
              ScaledJacobian, SharedDOF>::prepEntry(entry, op, nullptr, values, source.Connectivity,
              source.Offset, shapeGradient, shapeConn, shapeValues);
          }
          else
          {
            vtkDGOperationEvaluator<InputIterator, OutputIterator, Discontinuous, Sides,
              ScaledJacobian, Discontinuous>::prepEntry(entry, op, nullptr, values,
              source.Connectivity, source.Offset, shapeGradient, nullptr, shapeValues);
          }
        }
        break;
      }
    }
  }
  RangeKey key{ static_cast<vtkTypeUInt64>(source.Offset),
    static_cast<vtkTypeUInt64>(source.Offset + source.Connectivity->GetNumberOfTuples()) };
  this->Evaluators[key] = entry;
}

/// Find the evaluator that matches the given \a cellId.
template <typename InputIterator, typename OutputIterator>
typename vtkDGOperation<InputIterator, OutputIterator>::EvaluatorMap::const_iterator
vtkDGOperation<InputIterator, OutputIterator>::FindEvaluator(
  RangeKey cellKey, const EvaluatorMap& evaluators)
{
  auto cellId = cellKey.Begin;
  auto it = evaluators.lower_bound(cellKey);
  if (it == evaluators.end())
  {
    // Either \a cellId is past the end of cell-ids covered by \a evaluators
    // or the final entry in evaluators covers \a cellId.
    if (evaluators.empty())
    {
      return it;
    }
    const auto& lastKey(evaluators.rbegin()->first);
    if (lastKey.Contains(cellId))
    {
      // cellId > lastKey.Begin but cellId < lastKey.End.
      return evaluators.find(lastKey);
    }
    // cellId >= lastKey.End
    return it;
  }
  // If it->first.Begin == cellId, we have a match:
  if (it->first.Contains(cellId))
  {
    return it;
  }
  if (it == evaluators.begin())
  {
    // The first entry of evaluators doesn't contain \a cellId but
    // \a cellId >= it->first.Begin and \a cellId < (it++)->first.Begin.
    // This should only be possible if there is a gap between ranges of cell-ids
    // covered by \a evaluators. This should not happen.
    return evaluators.end();
  }
  // it->first.Begin >= \a cellId. Back up to see if the previous evaluator
  // entry contains \a cellId.
  --it;
  if (it->first.Contains(cellId))
  {
    return it;
  }
  return evaluators.end();
}

VTK_ABI_NAMESPACE_END
#endif // vtkDGOperation_txx
// VTK-HeaderTest-Exclude: vtkDGOperation.h
