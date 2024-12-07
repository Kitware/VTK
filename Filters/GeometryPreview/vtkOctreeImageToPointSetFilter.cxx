// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkOctreeImageToPointSetFilter.h"

#include "vtkArrayDispatch.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataArrayRange.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSMPTools.h"
#include "vtkUnsignedCharArray.h"

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkOctreeImageToPointSetFilter);

//------------------------------------------------------------------------------
vtkOctreeImageToPointSetFilter::vtkOctreeImageToPointSetFilter() = default;

//------------------------------------------------------------------------------
vtkOctreeImageToPointSetFilter::~vtkOctreeImageToPointSetFilter() = default;

//------------------------------------------------------------------------------
void vtkOctreeImageToPointSetFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "CreateVerticesCellArray: " << this->CreateVerticesCellArray << endl;
  os << indent << "ProcessInputCellArray: " << this->ProcessInputCellArray << endl;
  os << indent << "CellArrayComponent: " << this->CellArrayComponent << endl;
}

//------------------------------------------------------------------------------
int vtkOctreeImageToPointSetFilter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;
}

namespace
{
struct CellInformation
{
  vtkIdType CellId;
  vtkIdType PointOffset;
};

// Compute the number of output points per cell and globally
struct ComputePointsOffsetsFunctor
{
  vtkUnsignedCharArray* Octree;
  vtkIdType NumberOfInputCells;

  vtkSMPThreadLocal<vtkIdType> TLNumberOfOutputPoints;

  vtkIdType NumberOfOutputPoints;
  vtkSmartPointer<vtkUnsignedCharArray> PointsPerCell;
  std::vector<CellInformation> CellsInfo;

  ComputePointsOffsetsFunctor(vtkUnsignedCharArray* octree, vtkIdType numberOfInputCells)
    : Octree(octree)
    , NumberOfInputCells(numberOfInputCells)
    , NumberOfOutputPoints(0)
  {
    this->PointsPerCell = vtkSmartPointer<vtkUnsignedCharArray>::New();
    this->PointsPerCell->SetNumberOfValues(this->NumberOfInputCells);
  }

  void Initialize() { this->TLNumberOfOutputPoints.Local() = 0; }

  void operator()(vtkIdType begin, vtkIdType end)
  {
    vtkIdType& numberOfOutputPoints = this->TLNumberOfOutputPoints.Local();
    unsigned char* octree = this->Octree->GetPointer(0);
    unsigned char* pointsPerCell = this->PointsPerCell->GetPointer(0);
    unsigned char numberOfCellPoints;
    for (vtkIdType cellId = begin; cellId < end; ++cellId)
    {
      unsigned char& octreeValue = octree[cellId];
      // get the number of points of the cell
      numberOfCellPoints = static_cast<bool>(octreeValue & 1) + static_cast<bool>(octreeValue & 2) +
        static_cast<bool>(octreeValue & 4) + static_cast<bool>(octreeValue & 8) +
        static_cast<bool>(octreeValue & 16) + static_cast<bool>(octreeValue & 32) +
        static_cast<bool>(octreeValue & 64) + static_cast<bool>(octreeValue & 128);
      pointsPerCell[cellId] = numberOfCellPoints;
      numberOfOutputPoints += numberOfCellPoints;
    }
  }

  void Reduce()
  {
    // compute the number of output points
    this->NumberOfOutputPoints = 0;
    for (auto& numberOfOutputPoints : this->TLNumberOfOutputPoints)
    {
      this->NumberOfOutputPoints += numberOfOutputPoints;
    }

    // compute the offset of each cell
    this->CellsInfo.reserve(static_cast<size_t>(this->NumberOfOutputPoints));
    vtkIdType pointOffset = 0;
    unsigned char* pointsPerCell = this->PointsPerCell->GetPointer(0);
    for (vtkIdType cellId = 0; cellId < this->NumberOfInputCells; ++cellId)
    {
      if (pointsPerCell[cellId] > 0)
      {
        this->CellsInfo.push_back({ cellId, pointOffset });
        pointOffset += pointsPerCell[cellId];
      }
    }
  }
};

// Compute the output points
template <typename TInputField, typename TOutputField>
struct ComputeOutputPointsFunctor
{
  vtkImageData* Input;
  vtkUnsignedCharArray* Octree;
  vtkUnsignedCharArray* PointsPerCell;
  vtkDoubleArray* OutputPoints;
  const std::vector<CellInformation>& CellsInfo;
  TInputField* InputField;
  TOutputField* OutputField;
  int CellArrayComponent;

  bool UseFieldArray;

  ComputeOutputPointsFunctor(vtkImageData* input, vtkUnsignedCharArray* octree,
    vtkUnsignedCharArray* pointPerCell, vtkDoubleArray* outputPoints,
    const std::vector<CellInformation>& cellsInfo, TInputField* inputField,
    TOutputField* outputField, int cellArrayComponent)
    : Input(input)
    , Octree(octree)
    , PointsPerCell(pointPerCell)
    , OutputPoints(outputPoints)
    , CellsInfo(cellsInfo)
    , InputField(inputField)
    , OutputField(outputField)
    , CellArrayComponent(cellArrayComponent)
    , UseFieldArray(inputField != nullptr)
  {
  }

  void operator()(vtkIdType begin, vtkIdType end)
  {
    unsigned char* octree = this->Octree->GetPointer(0);
    unsigned char* pointsPerCell = this->PointsPerCell->GetPointer(0);
    double* outputPoints = this->OutputPoints->GetPointer(0);
    const int* extent = this->Input->GetExtent();
    const int xExtent = extent[1] - extent[0];
    const int yExtent = extent[3] - extent[2];
    const double* spacing = this->Input->GetSpacing();
    const double spacing_4[3] = { spacing[0] / 4.0, spacing[1] / 4.0, spacing[2] / 4.0 };
    vtk::detail::TupleRange<TInputField, vtk::detail::DynamicTupleSize> inField;
    vtk::detail::ValueRange<TOutputField, 1> outField;
    if (this->UseFieldArray)
    {
      inField = vtk::DataArrayTupleRange(this->InputField);
      outField = vtk::DataArrayValueRange<1>(this->OutputField);
    }

    unsigned char j;
    double ijk[3], centerPoint[3];
    for (vtkIdType i = begin; i < end; ++i)
    {
      const CellInformation& cellInfo = this->CellsInfo[i];
      const vtkIdType& cellId = cellInfo.CellId;
      const vtkIdType& pointOffset = cellInfo.PointOffset;
      const unsigned char& octreeValue = octree[cellId];
      double* point = &outputPoints[3 * pointOffset];
      ijk[0] = (cellId % xExtent) + 0.5;
      ijk[1] = ((cellId / xExtent) % yExtent) + 0.5;
      ijk[2] = (cellId / (xExtent * yExtent)) + 0.5;
      this->Input->TransformContinuousIndexToPhysicalPoint(ijk, centerPoint);
      if (octreeValue & 1)
      {
        point[0] = centerPoint[0] - spacing_4[0];
        point[1] = centerPoint[1] - spacing_4[1];
        point[2] = centerPoint[2] - spacing_4[2];
        point += 3;
      }
      if (octreeValue & 2)
      {
        point[0] = centerPoint[0] + spacing_4[0];
        point[1] = centerPoint[1] - spacing_4[1];
        point[2] = centerPoint[2] - spacing_4[2];
        point += 3;
      }
      if (octreeValue & 4)
      {
        point[0] = centerPoint[0] - spacing_4[0];
        point[1] = centerPoint[1] + spacing_4[1];
        point[2] = centerPoint[2] - spacing_4[2];
        point += 3;
      }
      if (octreeValue & 8)
      {
        point[0] = centerPoint[0] + spacing_4[0];
        point[1] = centerPoint[1] + spacing_4[1];
        point[2] = centerPoint[2] - spacing_4[2];
        point += 3;
      }
      if (octreeValue & 16)
      {
        point[0] = centerPoint[0] - spacing_4[0];
        point[1] = centerPoint[1] - spacing_4[1];
        point[2] = centerPoint[2] + spacing_4[2];
        point += 3;
      }
      if (octreeValue & 32)
      {
        point[0] = centerPoint[0] + spacing_4[0];
        point[1] = centerPoint[1] - spacing_4[1];
        point[2] = centerPoint[2] + spacing_4[2];
        point += 3;
      }
      if (octreeValue & 64)
      {
        point[0] = centerPoint[0] - spacing_4[0];
        point[1] = centerPoint[1] + spacing_4[1];
        point[2] = centerPoint[2] + spacing_4[2];
        point += 3;
      }
      if (octreeValue & 128)
      {
        point[0] = centerPoint[0] + spacing_4[0];
        point[1] = centerPoint[1] + spacing_4[1];
        point[2] = centerPoint[2] + spacing_4[2];
        point += 3;
      }
      if (this->UseFieldArray)
      {
        const auto& numberOfPoints = pointsPerCell[cellId];
        const auto& inTuple = inField[cellId];
        // fill the output field array with the input field array value
        for (j = 0; j < numberOfPoints; ++j)
        {
          outField[pointOffset + j] = inTuple[this->CellArrayComponent];
        }
      }
    }
  }
};

struct ComputeOutputPointsWorker
{
  template <typename TInputField, typename TOutputField>
  void operator()(TInputField* inputField, TOutputField* outputField, vtkImageData* input,
    vtkUnsignedCharArray* octree, vtkUnsignedCharArray* pointsPerCell, vtkDoubleArray* outputPoints,
    const std::vector<CellInformation>& cellsInfo, int cellArrayComponent)
  {
    ComputeOutputPointsFunctor<TInputField, TOutputField> functor(input, octree, pointsPerCell,
      outputPoints, cellsInfo, inputField, outputField, cellArrayComponent);
    vtkSMPTools::For(0, static_cast<vtkIdType>(cellsInfo.size()), functor);
  }
};
}

//------------------------------------------------------------------------------
int vtkOctreeImageToPointSetFilter::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the input
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkImageData* input = vtkImageData::GetData(inInfo);

  // get the output
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkPolyData* output = vtkPolyData::GetData(outInfo);

  if (!input || input->GetNumberOfCells() == 0)
  {
    vtkErrorMacro("No input or empty input.");
    return 0;
  }

  // get the octree array
  auto octree = vtkUnsignedCharArray::SafeDownCast(input->GetCellData()->GetArray("octree"));
  if (!octree)
  {
    vtkErrorMacro("No octree array in input.");
    return 0;
  }

  // compute the number of output points per cell and globally and their offsets
  ComputePointsOffsetsFunctor computePointsOffsets(octree, input->GetNumberOfCells());
  vtkSMPTools::For(0, input->GetNumberOfCells(), computePointsOffsets);
  vtkIdType numberOfOutputPoints = computePointsOffsets.NumberOfOutputPoints;
  std::vector<CellInformation> cellsInfo = std::move(computePointsOffsets.CellsInfo);
  vtkSmartPointer<vtkUnsignedCharArray> pointsPerCell = computePointsOffsets.PointsPerCell;

  // create the output points
  vtkNew<vtkDoubleArray> pointsArray;
  pointsArray->SetNumberOfComponents(3);
  pointsArray->SetNumberOfTuples(numberOfOutputPoints);

  // create the pointData data array
  vtkSmartPointer<vtkDataArray> outField = nullptr;
  vtkDataArray* inField = nullptr;
  if (this->ProcessInputCellArray)
  {
    inField = this->GetInputArrayToProcess(0, inputVector);
    if (!inField)
    {
      vtkErrorMacro("Array to process is null.");
      return 0;
    }
    if (inField->GetNumberOfTuples() != input->GetNumberOfCells())
    {
      vtkErrorMacro("Array to process must have as many tuples as the number of cells.");
      return 0;
    }
    if (inField->GetNumberOfComponents() <= this->CellArrayComponent)
    {
      vtkErrorMacro("Invalid component.");
      return 0;
    }
    outField = vtkSmartPointer<vtkDataArray>::Take(inField->NewInstance());
    outField->SetName(inField->GetName());
    outField->SetNumberOfComponents(1);
    outField->SetNumberOfTuples(numberOfOutputPoints);
  }

  using Dispatcher = vtkArrayDispatch::Dispatch2SameValueType;
  ComputeOutputPointsWorker computeOutputPointsWorker;
  if (!Dispatcher::Execute(inField, outField.Get(), computeOutputPointsWorker, input, octree,
        pointsPerCell.Get(), pointsArray, cellsInfo, this->CellArrayComponent))
  {
    computeOutputPointsWorker(inField, outField.Get(), input, octree, pointsPerCell.Get(),
      pointsArray, cellsInfo, this->CellArrayComponent);
  }

  // set the output points
  vtkNew<vtkPoints> points;
  points->SetData(pointsArray);
  output->SetPoints(points);

  // set the output field array
  if (this->ProcessInputCellArray)
  {
    output->GetPointData()->AddArray(outField);
  }

  // create output vertices cell array
  if (this->CreateVerticesCellArray)
  {
    vtkNew<vtkIdTypeArray> connectivity;
    connectivity->SetNumberOfValues(numberOfOutputPoints);
    vtkSMPTools::For(0, numberOfOutputPoints,
      [&](vtkIdType begin, vtkIdType end)
      {
        auto connectivityPtr = connectivity->GetPointer(0);
        for (vtkIdType i = begin; i < end; ++i)
        {
          connectivityPtr[i] = i;
        }
      });
    vtkNew<vtkIdTypeArray> offsets;
    offsets->SetNumberOfValues(numberOfOutputPoints + 1);
    vtkSMPTools::For(0, numberOfOutputPoints + 1,
      [&](vtkIdType begin, vtkIdType end)
      {
        auto offsetsPtr = offsets->GetPointer(0);
        for (vtkIdType i = begin; i < end; ++i)
        {
          offsetsPtr[i] = i;
        }
      });
    vtkNew<vtkCellArray> cells;
    cells->SetData(offsets, connectivity);
    output->SetVerts(cells);
  }

  return 1;
}
VTK_ABI_NAMESPACE_END
