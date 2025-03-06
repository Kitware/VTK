// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDecimatePolylineFilter.h"

#include "vtkCellArray.h"
#include "vtkCellArrayIterator.h"
#include "vtkCellData.h"
#include "vtkDecimatePolylineStrategy.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPriorityQueue.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <map>
#include <memory>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkDecimatePolylineFilter);

//------------------------------------------------------------------------------
// Representation of a polyline as a doubly linked list of vertices.
class vtkDecimatePolylineFilter::Polyline
{
public:
  struct Vertex
  {
    unsigned int index;
    vtkIdType id;
    Vertex* prev;
    Vertex* next;
    bool removable;
  };

  Polyline(const vtkIdType* vertexOrdering, vtkIdType size)
  {
    this->Size = size;
    this->Vertices.resize(size);
    for (vtkIdType idx = 0; idx < size; ++idx)
    {
      this->Vertices[idx].index = static_cast<unsigned int>(idx);
      this->Vertices[idx].id = vertexOrdering[idx];
      this->Vertices[idx].prev = (idx > 0 ? &this->Vertices[idx - 1] : nullptr);
      this->Vertices[idx].next = (idx < size - 1 ? &this->Vertices[idx + 1] : nullptr);
      this->Vertices[idx].removable = true;
    }
    this->Vertices[0].removable = Vertices[size - 1].removable = false;
    // Some polylines close in on themselves
    this->IsLoop = Vertices[0].id == Vertices[size - 1].id;
  }

  void Remove(vtkIdType vertexIdx)
  {
    this->Size--;
    (*(this->Vertices[vertexIdx].prev)).next = this->Vertices[vertexIdx].next;
    (*(this->Vertices[vertexIdx].next)).prev = this->Vertices[vertexIdx].prev;
  }

  vtkIdType Size;
  std::vector<Vertex> Vertices;
  bool IsLoop;
};

//------------------------------------------------------------------------------
vtkMTimeType vtkDecimatePolylineFilter::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  if (this->DecimationStrategy != nullptr)
  {
    vtkMTimeType strategyMTime = this->DecimationStrategy->GetMTime();
    mTime = strategyMTime > mTime ? strategyMTime : mTime;
  }
  return mTime;
}

//------------------------------------------------------------------------------
double vtkDecimatePolylineFilter::ComputeError(
  vtkPolyData* input, Polyline* polyline, vtkIdType idx)
{
  vtkIdType p1Id = polyline->Vertices[idx].prev->id;
  vtkIdType originId = polyline->Vertices[idx].id;
  vtkIdType p2Id = polyline->Vertices[idx].next->id;

  return this->DecimationStrategy->ComputeError(input, originId, p1Id, p2Id);
}

//------------------------------------------------------------------------------
int vtkDecimatePolylineFilter::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  int numPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  int ghostLevel = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  if (numPieces > 1)
  {
    // This filter requires at least 1 ghost level to work in distributed mode
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
      vtkMath::Max(ghostLevel, 1));
  }
  return 1;
}

//------------------------------------------------------------------------------
int vtkDecimatePolylineFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkPolyData* input = vtkPolyData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkCellArray* inputLines = input->GetLines();
  vtkPoints* inputPoints = input->GetPoints();

  vtkDebugMacro("Decimating polylines");

  if (!inputLines || !inputPoints)
  {
    return 1;
  }

  vtkIdType numLines = inputLines->GetNumberOfCells();
  vtkIdType numPts = inputPoints->GetNumberOfPoints();
  if (numLines < 1 || numPts < 1)
  {
    return 1;
  }

  if (this->DecimationStrategy == nullptr)
  {
    vtkWarningMacro("No decimation strategy has been defined. The decimation cannot be performed.");
    return 1;
  }

  if (!this->DecimationStrategy->IsStateValid(input))
  {
    vtkWarningMacro(
      "Decimation Strategy is not in a valid state. The decimation cannot be performed.");
    return 1;
  }

  // Allocate memory and prepare for data processing
  vtkNew<vtkPoints> newPts;

  // Set the desired precision for the points in the output.
  if (this->OutputPointsPrecision == vtkAlgorithm::DEFAULT_PRECISION)
  {
    newPts->SetDataType(inputPoints->GetDataType());
  }
  else if (this->OutputPointsPrecision == vtkAlgorithm::SINGLE_PRECISION)
  {
    newPts->SetDataType(VTK_FLOAT);
  }
  else if (this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
  {
    newPts->SetDataType(VTK_DOUBLE);
  }

  vtkNew<vtkCellArray> newLines;
  newLines->AllocateExact(numLines, numLines * 2);

  vtkPointData* inPD = input->GetPointData();
  vtkPointData* outPD = output->GetPointData();
  outPD->CopyAllocate(inPD);

  vtkCellData* inCD = input->GetCellData();
  vtkCellData* outCD = output->GetCellData();
  outCD->CopyAllocate(inCD);

  auto lineIter = vtkSmartPointer<vtkCellArrayIterator>::Take(inputLines->NewIterator());
  vtkIdType firstVertexIndex = 0;

  vtkIdType polylineSize = 0;
  const vtkIdType* polyLineVerts;
  vtkIdType checkAbortInterval = std::min(inputLines->GetNumberOfCells() / 10 + 1, (vtkIdType)1000);
  vtkIdType progessInterval = 0;

  std::map<vtkIdType, vtkIdType> pointIdMap;
  // Decimate each polyline (represented as a single cell) in series
  for (lineIter->GoToFirstCell(); !lineIter->IsDoneWithTraversal();
       lineIter->GoToNextCell(), firstVertexIndex += polylineSize)
  {
    if (progessInterval % checkAbortInterval == 0 && this->CheckAbort())
    {
      break;
    }
    progessInterval++;
    lineIter->GetCurrentCell(polylineSize, polyLineVerts);

    // construct a polyline as a doubly linked list
    std::shared_ptr<Polyline> polyline = std::make_shared<Polyline>(polyLineVerts, polylineSize);

    // Can only decimate polyline with at least 3 points
    if (polylineSize > 2)
    {
      for (vtkIdType vertexIdx = 0; vertexIdx < polyline->Size; ++vertexIdx)
      {
        // only vertices that are removable have associated error values
        if (polyline->Vertices[vertexIdx].removable)
        {
          double error = this->ComputeError(input, polyline.get(), vertexIdx);
          if (error <= this->MaximumError)
          {
            this->PriorityQueue->Insert(error, vertexIdx);
          }
        }
      }
    }

    // Now process structures,
    // deleting vertices until the decimation target is met.
    vtkIdType currentNumPts = polylineSize;
    while (1.0 - (static_cast<double>(currentNumPts) / static_cast<double>(polylineSize)) <
        this->TargetReduction &&
      ((!polyline->IsLoop && currentNumPts > 2) || (polyline->IsLoop && currentNumPts > 3)))
    {
      vtkIdType poppedIdx = this->PriorityQueue->Pop();
      if (poppedIdx < 0)
      {
        break; // all points are exhausted, get out
      }

      --currentNumPts;
      polyline->Remove(poppedIdx);
      vtkIdType prevIdx = polyline->Vertices[poppedIdx].prev->index;
      vtkIdType nextIdx = polyline->Vertices[poppedIdx].next->index;

      // again, only vertices that are removable have associated error values
      if (polyline->Vertices[poppedIdx].prev->removable)
      {
        double error = this->ComputeError(input, polyline.get(), prevIdx);
        this->PriorityQueue->DeleteId(prevIdx);
        if (error <= this->MaximumError)
        {
          this->PriorityQueue->Insert(error, prevIdx);
        }
      }

      if (polyline->Vertices[poppedIdx].next->removable)
      {
        double error = this->ComputeError(input, polyline.get(), nextIdx);
        this->PriorityQueue->DeleteId(nextIdx);
        if (error <= this->MaximumError)
        {
          this->PriorityQueue->Insert(error, nextIdx);
        }
      }
    }

    // What's left over is now spit out as a new polyline
    vtkIdType newId = newLines->InsertNextCell(static_cast<int>(currentNumPts));
    outCD->CopyData(inCD, lineIter->GetCurrentCellId(), newId);

    Polyline::Vertex* vertex = polyline->Vertices.data();
    while (vertex != nullptr)
    {
      // points that are repeated within polylines are represented by
      // only one point instance
      auto it = pointIdMap.find(vertex->id);
      if (it == pointIdMap.end())
      {
        newId = newPts->InsertNextPoint(inputPoints->GetPoint(vertex->id));
        newLines->InsertCellPoint(newId);
        outPD->CopyData(inPD, vertex->id, newId);
        pointIdMap[vertex->id] = newId;
      }
      else
      {
        newLines->InsertCellPoint(it->second);
      }

      vertex = vertex->next;
    }

    this->PriorityQueue->Reset();
  }

  // Create output and clean up
  output->SetPoints(newPts);
  output->SetLines(newLines);

  return 1;
}

//------------------------------------------------------------------------------
void vtkDecimatePolylineFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Target Reduction: " << this->TargetReduction << "\n";
  os << indent << "Maximum Error: " << this->MaximumError << "\n";
  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
}
VTK_ABI_NAMESPACE_END
