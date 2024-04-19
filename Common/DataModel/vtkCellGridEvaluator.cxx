// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellGridEvaluator.h"

#include "vtkBoundingBox.h"
#include "vtkCellGrid.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkStaticPointLocator.h"
#include "vtkTypeUInt32Array.h"

#include <algorithm>

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkCellGridEvaluator);

vtkIdType vtkCellGridEvaluator::AllocationsByCellType::GetNumberOfOutputPoints() const
{
  vtkIdType numberOfPoints = 0;
  for (const auto& contains : this->InputPoints)
  {
    numberOfPoints += static_cast<vtkIdType>(contains.second.size());
    // std::cout << "  " << contains.first << " " << contains.second.size() << "\n";
  }
  return numberOfPoints;
}

vtkCellGridEvaluator::vtkCellGridEvaluator()
{
  vtkNew<vtkTypeUInt64Array> pointIds;
  vtkNew<vtkDoubleArray> vals;
  this->SetClassifierPointIDs(pointIds);
  this->SetInterpolatedValues(vals);
}

vtkCellGridEvaluator::~vtkCellGridEvaluator()
{
  this->SetCellAttribute(nullptr);
  this->SetInputPoints(nullptr);
  this->SetClassifierCellTypes(nullptr);
  this->SetClassifierCellOffsets(nullptr);
  this->SetClassifierPointIDs(nullptr);
  this->SetClassifierCellIndices(nullptr);
  this->SetClassifierPointParameters(nullptr);
  this->SetInterpolatedValues(nullptr);
  this->Locator->Initialize();
}

void vtkCellGridEvaluator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "CellAttribute: " << this->CellAttribute << "\n";
  os << indent << "PhasesToPerform: " << this->PhasesToPerform << "\n";
  os << indent << "InputPoints: " << this->InputPoints << "\n";
  os << indent << "ClassifierCellTypes: " << this->ClassifierCellTypes << "\n";
  os << indent << "ClassifierCellOffsets: " << this->ClassifierCellOffsets << "\n";
  os << indent << "ClassifierPointIDs: " << this->ClassifierPointIDs << "\n";
  os << indent << "ClassifierCellIndices: " << this->ClassifierCellIndices << "\n";
  os << indent << "ClassifierPointParameters: " << this->ClassifierPointParameters << "\n";
  os << indent << "InterpolatedValues: " << this->InterpolatedValues << "\n";
}

void vtkCellGridEvaluator::ClassifyPoints(vtkDataArray* points)
{
  this->PhasesToPerform = Phases::Classify;
  this->SetInputPoints(points);
  this->SetClassifierCellTypes(nullptr);
  this->SetClassifierCellOffsets(nullptr);
  this->SetClassifierPointIDs(nullptr);
  this->SetClassifierCellIndices(nullptr);
  this->SetClassifierPointParameters(nullptr);
}

void vtkCellGridEvaluator::InterpolatePoints(vtkDataArray* points)
{
  this->PhasesToPerform = Phases::ClassifyAndInterpolate;
  this->SetInputPoints(points);
  this->SetClassifierCellTypes(nullptr);
  this->SetClassifierCellOffsets(nullptr);
  this->SetClassifierPointIDs(nullptr);
  this->SetClassifierCellIndices(nullptr);
  this->SetClassifierPointParameters(nullptr);
}

void vtkCellGridEvaluator::InterpolateCellParameters(vtkTypeUInt32Array* cellTypes,
  vtkTypeUInt64Array* cellOffsets, vtkTypeUInt64Array* cellIndices, vtkDataArray* pointParameters)
{
  if (!cellTypes || !cellOffsets || !cellIndices || !pointParameters)
  {
    vtkErrorMacro("Null input arrays are unacceptable.");
    return;
  }
  if (cellTypes->GetNumberOfTuples() != cellOffsets->GetNumberOfTuples())
  {
    vtkErrorMacro("Cell type and offset arrays must have the same number of tuples.");
    return;
  }
  if (cellIndices->GetNumberOfTuples() != pointParameters->GetNumberOfTuples())
  {
    vtkErrorMacro("Cell indices and point parameters must have the same number of tuples.");
    return;
  }

  this->PhasesToPerform = Phases::Interpolate;
  this->SetInputPoints(nullptr);
  this->SetClassifierPointIDs(nullptr);
  this->SetClassifierCellTypes(cellTypes);
  this->SetClassifierCellOffsets(cellOffsets);
  this->SetClassifierCellIndices(cellOffsets);
  this->SetClassifierPointParameters(pointParameters);
}

void vtkCellGridEvaluator::Initialize()
{
  // Check our configuration.
  switch (this->PhasesToPerform)
  {
    case Phases::None:
    {
      vtkErrorMacro("Evaluator is not configured.");
      return;
    }
    case Phases::Classify:
      if (!this->InputPoints)
      {
        vtkErrorMacro("No input points provided.");
        return;
      }
      break;
    case Phases::ClassifyAndInterpolate:
      if (!this->InputPoints)
      {
        vtkErrorMacro("No input points provided.");
        return;
      }
      break;
    case Phases::Interpolate:
      if (!this->ClassifierCellIndices || !this->ClassifierCellTypes ||
        !this->ClassifierCellOffsets || !this->ClassifierPointParameters)
      {
        vtkErrorMacro("One or more input arrays are missing.");
        return;
      }
      break;
  }

  // Reset our state.
  this->Allocations.clear();
  this->InterpolatedValues->SetNumberOfTuples(0);
}

void vtkCellGridEvaluator::StartPass()
{
  this->Superclass::StartPass();

  // If we haven't allocated arrays for the pass, do so now.
  switch (this->PhasesToPerform)
  {
    case Interpolate:
      this->AllocateInterpolationOutput();
      break;
    default:
      break;

    case Classify:
    case ClassifyAndInterpolate:
      if (this->Pass == 0)
      {
        // Build a locator so we can find points near cells.
        vtkNew<vtkPointSet> dataset;
        vtkNew<vtkPoints> points;
        points->SetData(this->InputPoints);
        dataset->SetPoints(points);
        this->Locator->SetDataSet(dataset);
        this->Locator->BuildLocator();
      }
      else if (this->Pass == 1)
      {
        this->AllocatePositionOutput();
      }
      else if (this->Pass == 2)
      {
        this->AllocateInterpolationOutput();
      }
      break;
  }
}

void vtkCellGridEvaluator::AllocateClassificationOutput()
{
  // First, ensure the arrays exist
  if (!this->ClassifierCellTypes)
  {
    vtkNew<vtkTypeUInt32Array> arr;
    this->SetClassifierCellTypes(arr);
  }
  if (!this->ClassifierCellOffsets)
  {
    vtkNew<vtkTypeUInt64Array> arr;
    this->SetClassifierCellOffsets(arr);
  }

  // Configure the arrays properly.
  int numberOfCellTypes = static_cast<int>(this->Allocations.size());
  this->ClassifierCellTypes->SetName("CellType");
  this->ClassifierCellTypes->SetNumberOfTuples(numberOfCellTypes + 1);

  this->ClassifierCellOffsets->SetName("CellTypeOffset");
  this->ClassifierCellOffsets->SetNumberOfTuples(numberOfCellTypes + 1);
}

void vtkCellGridEvaluator::AllocatePositionOutput()
{
  // First, ensure the arrays exist
  if (!this->ClassifierPointIDs)
  {
    vtkNew<vtkTypeUInt64Array> arr;
    this->SetClassifierPointIDs(arr);
  }
  if (!this->ClassifierCellIndices)
  {
    vtkNew<vtkTypeUInt64Array> arr;
    this->SetClassifierCellIndices(arr);
  }
  if (!this->ClassifierPointParameters)
  {
    vtkNew<vtkDoubleArray> arr;
    this->SetClassifierPointParameters(arr);
  }

  // Configure the arrays properly.
  this->ClassifierPointIDs->SetName("InputPointIndex");
  this->ClassifierPointIDs->SetNumberOfTuples(this->NumberOfOutputPoints);

  this->ClassifierCellIndices->SetName("ContainingCellID");
  this->ClassifierCellIndices->SetNumberOfTuples(this->NumberOfOutputPoints);

  this->ClassifierPointParameters->SetName("ParametricCoordinates");
  this->ClassifierPointParameters->SetNumberOfComponents(3); // even for 1-d or 2-d cells.
  this->ClassifierPointParameters->SetNumberOfTuples(this->NumberOfOutputPoints);
}

void vtkCellGridEvaluator::AllocateInterpolationOutput()
{
  if (!this->InterpolatedValues)
  {
    vtkNew<vtkDoubleArray> vals;
    this->SetInterpolatedValues(vals);
  }
  this->InterpolatedValues->SetName(this->CellAttribute->GetName().Data().c_str());
  this->InterpolatedValues->SetNumberOfComponents(this->CellAttribute->GetNumberOfComponents());
  this->InterpolatedValues->SetNumberOfTuples(this->NumberOfOutputPoints);
}

bool vtkCellGridEvaluator::IsAnotherPassRequired()
{
  // Assign output offsets to each cell type after the 0-th pass.
  if (this->Pass == 0)
  {
    switch (this->PhasesToPerform)
    {
      case Phases::Classify:
      case Phases::ClassifyAndInterpolate:
      {
        // Allocate per-cell-type arrays:
        this->AllocateClassificationOutput();
        // Populate the ClassifierCellTypes and ClassifierCellOffsets
        // for the next pass, assigning each cell type its allocation
        // in the arrays configured by AllocatePositionOutput().
        vtkIdType offset = 0;
        vtkIdType cellType = 0;
        for (auto& entry : this->Allocations)
        {
          this->ClassifierCellTypes->SetValue(cellType, entry.first.GetId());
          this->ClassifierCellOffsets->SetValue(cellType, offset);
          ++cellType;
          entry.second.Offset = offset;
          offset += entry.second.GetNumberOfOutputPoints();
          // std::cout << entry.first.Data() << " " << entry.second.Offset << " – " << offset <<
          // "\n";
        }
        // Add a trailing entry containing the total number of points.
        this->ClassifierCellOffsets->SetValue(cellType, offset);
        this->ClassifierCellTypes->SetValue(cellType, vtkStringToken::InvalidHash());
        this->NumberOfOutputPoints = offset;
      }
      break;
      case Phases::Interpolate:
      default:
        break;
    }
  }

  // Now decide whether to perform another pass.
  switch (this->PhasesToPerform)
  {
    case Classify:
      // Pass 0: Classify input points
      // Pass 1: Evaluate position
      return this->Pass < 1;
    case ClassifyAndInterpolate:
      // Pass 0: Classify input points
      // Pass 1: Evaluate position
      // Pass 2: Interpolate attribute
      return this->Pass < 2;
    case Interpolate:
      // Pass 0: Interpolate attribute
    default:
      return false;
  }
}

void vtkCellGridEvaluator::Finalize() {}

vtkCellGridEvaluator::AllocationsByCellType& vtkCellGridEvaluator::GetAllocationsForCellType(
  vtkStringToken cellType)
{
  return this->Allocations[cellType];
}

VTK_ABI_NAMESPACE_END
