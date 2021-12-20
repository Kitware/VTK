/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStaticCleanPolyData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkStaticCleanPolyData.h"

#include "vtkArrayDispatch.h"
#include "vtkArrayListTemplate.h" // For processing attribute data
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataArrayRange.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMergePoints.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSMPTools.h"
#include "vtkStaticCleanUnstructuredGrid.h"
#include "vtkStaticPointLocator.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <algorithm>

vtkStandardNewMacro(vtkStaticCleanPolyData);

// This filter uses methods found in vtkStaticCleanUnstructuredGrid.
using PointUses = unsigned char;

//------------------------------------------------------------------------------
// Construct object with initial Tolerance of 0.0
vtkStaticCleanPolyData::vtkStaticCleanPolyData()
{
  this->ToleranceIsAbsolute = false;
  this->Tolerance = 0.0;
  this->AbsoluteTolerance = 0.0;

  this->MergingArray = nullptr;
  this->SetMergingArray("");

  this->ConvertPolysToLines = false;
  this->ConvertLinesToPoints = false;
  this->ConvertStripsToPolys = false;

  this->RemoveUnusedPoints = true;
  this->ProduceMergeMap = false;
  this->AveragePointData = false;
  this->OutputPointsPrecision = vtkAlgorithm::DEFAULT_PRECISION;
  this->PieceInvariant = true;

  this->Locator.TakeReference(vtkStaticPointLocator::New());
}

//------------------------------------------------------------------------------
int vtkStaticCleanPolyData::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (this->PieceInvariant)
  {
    // Although piece > 1 is handled by superclass, we should be thorough.
    if (outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()) == 0)
    {
      inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
      inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
    }
    else
    {
      inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
      inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 0);
    }
  }
  else
  {
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkStaticCleanPolyData::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPolyData* input = vtkPolyData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkPoints* inPts = input->GetPoints();
  vtkIdType numPts = input->GetNumberOfPoints();

  vtkDebugMacro(<< "Beginning PolyData clean");
  if ((numPts < 1) || (inPts == nullptr))
  {
    vtkDebugMacro(<< "No data to Operate On!");
    return 1;
  }

  // we'll be needing these
  vtkIdType newId;
  vtkIdType i;
  vtkIdType ptId;
  vtkIdType npts = 0;
  const vtkIdType* pts = nullptr;

  vtkCellArray* inVerts = input->GetVerts();
  vtkSmartPointer<vtkCellArray> newVerts;
  vtkCellArray* inLines = input->GetLines();
  vtkSmartPointer<vtkCellArray> newLines;
  vtkCellArray* inPolys = input->GetPolys();
  vtkSmartPointer<vtkCellArray> newPolys;
  vtkCellArray* inStrips = input->GetStrips();
  vtkSmartPointer<vtkCellArray> newStrips;

  vtkPointData* inPD = input->GetPointData();
  vtkCellData* inCD = input->GetCellData();
  vtkPointData* outPD = output->GetPointData();
  vtkCellData* outCD = output->GetCellData();

  // The merge map indicates which points are merged with what points
  this->Locator->SetDataSet(input);
  this->Locator->BuildLocator();
  this->UpdateProgress(0.25);

  // Compute the tolerance
  double tol =
    (this->ToleranceIsAbsolute ? this->AbsoluteTolerance : this->Tolerance * input->GetLength());

  // Now merge the points to create a merge map. The order of traversal can
  // be specified through the locator, the default is BIN_ORDER when the
  // tolerance is non-zero. Also, check whether merging data is enabled.
  std::vector<vtkIdType> mergeMap(numPts);
  vtkDataArray* mergingData = nullptr;
  if (this->MergingArray)
  {
    if ((mergingData = inPD->GetArray(this->MergingArray)))
    {
      this->Locator->MergePointsWithData(mergingData, mergeMap.data());
    }
  }
  if (!mergingData)
  {
    this->Locator->MergePoints(tol, mergeMap.data());
  }
  this->UpdateProgress(0.5);

  // If removing unused points, traverse the connectivity array to mark the
  // points that are used by one or more cells. This requires processing all
  // for input arrays.
  std::unique_ptr<PointUses[]> uPtUses; // reference counted to prevent leakage
  PointUses* ptUses = nullptr;
  if (this->RemoveUnusedPoints)
  {
    uPtUses = std::unique_ptr<PointUses[]>(new PointUses[numPts]);
    ptUses = uPtUses.get();
    std::fill_n(ptUses, numPts, 0);
    vtkStaticCleanUnstructuredGrid::MarkPointUses(inVerts, mergeMap.data(), ptUses);
    vtkStaticCleanUnstructuredGrid::MarkPointUses(inLines, mergeMap.data(), ptUses);
    vtkStaticCleanUnstructuredGrid::MarkPointUses(inPolys, mergeMap.data(), ptUses);
    vtkStaticCleanUnstructuredGrid::MarkPointUses(inStrips, mergeMap.data(), ptUses);
  }

  // Create a map that maps old point ids into new, renumbered point
  // ids.
  vtkNew<vtkIdTypeArray> ptMap;
  ptMap->SetNumberOfTuples(numPts);
  ptMap->SetName("PointMergeMap");
  vtkIdType* pmap = ptMap->GetPointer(0);
  if (this->ProduceMergeMap)
  {
    output->GetFieldData()->AddArray(ptMap);
  }

  // Build the map from old points to new points.
  vtkIdType numNewPts =
    vtkStaticCleanUnstructuredGrid::BuildPointMap(numPts, pmap, ptUses, mergeMap);

  // Create new points of the appropriate type
  vtkNew<vtkPoints> newPts;
  if (this->OutputPointsPrecision == vtkAlgorithm::DEFAULT_PRECISION)
  {
    newPts->SetDataType(inPts->GetDataType());
  }
  else if (this->OutputPointsPrecision == vtkAlgorithm::SINGLE_PRECISION)
  {
    newPts->SetDataType(VTK_FLOAT);
  }
  else if (this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
  {
    newPts->SetDataType(VTK_DOUBLE);
  }
  newPts->SetNumberOfPoints(numNewPts);
  output->SetPoints(newPts);

  // Produce output points and associated point data. If point averaging is
  // requested, then point coordinates and point attribute values must be
  // combined - a relatively compute intensive process.
  outPD->CopyAllocate(inPD);
  if (this->AveragePointData)
  {
    vtkStaticCleanUnstructuredGrid::AveragePoints(inPts, inPD, newPts, outPD, pmap, tol);
  }
  else
  {
    vtkStaticCleanUnstructuredGrid::CopyPoints(inPts, inPD, newPts, outPD, pmap);
  }
  this->UpdateProgress(0.6);

  // Finally, remap the topology to use new point ids. Celldata needs to be
  // copied correctly. If a poly is converted to a line, or a line to a
  // point, then using a CellCounter will not do, as the cells should be
  // ordered verts, lines, polys, strips. We need to maintain separate cell
  // data lists so we can copy them all correctly. Tedious but easy to
  // implement. We can use outCD for vertex cell data, then add the rest
  // at the end.
  vtkSmartPointer<vtkCellData> outLineData;
  vtkSmartPointer<vtkCellData> outPolyData;
  vtkSmartPointer<vtkCellData> outStrpData;
  outCD->CopyAllocate(inCD);

  // Begin to adjust topology. We need to cull out duplicate points and see
  // what's left.  Just use a vector to keep track of unique ids - it's a
  // small set so find() will execute relatively fast.
  std::vector<vtkIdType> cellIds;
  vtkIdType inCellID = 0;

  //
  // Vertices are renumbered and we remove duplicates
  if (!this->GetAbortExecute() && inVerts->GetNumberOfCells() > 0)
  {
    newVerts.TakeReference(vtkCellArray::New());
    newVerts->AllocateEstimate(inVerts->GetNumberOfCells(), 1);

    vtkDebugMacro(<< "Starting Verts " << inCellID);
    for (inVerts->InitTraversal(); inVerts->GetNextCell(npts, pts); inCellID++)
    {
      cellIds.clear();
      for (i = 0; i < npts; i++)
      {
        ptId = pmap[pts[i]];
        if (std::find(cellIds.begin(), cellIds.end(), ptId) == cellIds.end())
        {
          cellIds.push_back(ptId);
        }
      } // for all points of vertex cell

      if (!cellIds.empty())
      {
        newId = newVerts->InsertNextCell(cellIds.size(), cellIds.data());
        outCD->CopyData(inCD, inCellID, newId);
      }
    }
  }
  this->UpdateProgress(0.7);

  // lines reduced to one point are eliminated or made into verts
  if (!this->GetAbortExecute() && inLines->GetNumberOfCells() > 0)
  {
    newLines.TakeReference(vtkCellArray::New());
    newLines->AllocateEstimate(inLines->GetNumberOfCells(), 2);
    outLineData.TakeReference(vtkCellData::New());
    outLineData->CopyAllocate(inCD);
    //
    vtkDebugMacro(<< "Starting Lines " << inCellID);
    for (inLines->InitTraversal(); inLines->GetNextCell(npts, pts); inCellID++)
    {
      cellIds.clear();
      for (i = 0; i < npts; i++)
      {
        ptId = pmap[pts[i]];
        if (std::find(cellIds.begin(), cellIds.end(), ptId) == cellIds.end())
        {
          cellIds.push_back(ptId);
        }
      } // for all cell points

      if (cellIds.size() > 1)
      {
        newId = newLines->InsertNextCell(cellIds.size(), cellIds.data());
        outLineData->CopyData(inCD, inCellID, newId);
      }
      else if (cellIds.size() == 1 && this->ConvertLinesToPoints)
      {
        if (!newVerts)
        {
          newVerts.TakeReference(vtkCellArray::New());
          newVerts->AllocateEstimate(5, 1);
        }
        newId = newVerts->InsertNextCell(cellIds.size(), cellIds.data());
        outCD->CopyData(inCD, inCellID, newId);
      }
    }
    vtkDebugMacro(<< "Removed " << inLines->GetNumberOfCells() - newLines->GetNumberOfCells()
                  << " lines");
  }
  this->UpdateProgress(0.8);

  // polygons reduced to two points or less are either eliminated
  // or converted to lines or points if enabled
  if (!this->GetAbortExecute() && inPolys->GetNumberOfCells() > 0)
  {
    newPolys.TakeReference(vtkCellArray::New());
    newPolys->AllocateCopy(inPolys);
    outPolyData.TakeReference(vtkCellData::New());
    outPolyData->CopyAllocate(inCD);

    vtkDebugMacro(<< "Starting Polys " << inCellID);
    for (inPolys->InitTraversal(); inPolys->GetNextCell(npts, pts); inCellID++)
    {
      cellIds.clear();
      for (i = 0; i < npts; i++)
      {
        ptId = pmap[pts[i]];
        if (std::find(cellIds.begin(), cellIds.end(), ptId) == cellIds.end())
        {
          cellIds.push_back(ptId);
        }
      } // for points in cell

      if (cellIds.size() > 2)
      {
        newId = newPolys->InsertNextCell(cellIds.size(), cellIds.data());
        outPolyData->CopyData(inCD, inCellID, newId);
      }
      else if (cellIds.size() == 2 && this->ConvertPolysToLines)
      {
        if (!newLines)
        {
          newLines.TakeReference(vtkCellArray::New());
          newLines->AllocateEstimate(5, 2);
          outLineData.TakeReference(vtkCellData::New());
          outLineData->CopyAllocate(inCD);
        }
        newId = newLines->InsertNextCell(cellIds.size(), cellIds.data());
        outLineData->CopyData(inCD, inCellID, newId);
      }
      else if (cellIds.size() == 1 && this->ConvertLinesToPoints)
      {
        if (!newVerts)
        {
          newVerts.TakeReference(vtkCellArray::New());
          newVerts->AllocateEstimate(5, 1);
        }
        newId = newVerts->InsertNextCell(cellIds.size(), cellIds.data());
        outCD->CopyData(inCD, inCellID, newId);
      }
    }
    vtkDebugMacro(<< "Removed " << inPolys->GetNumberOfCells() - newPolys->GetNumberOfCells()
                  << " polys");
  }
  this->UpdateProgress(0.9);

  // triangle strips can reduced to polys/lines/points etc
  if (!this->GetAbortExecute() && inStrips->GetNumberOfCells() > 0)
  {
    newStrips.TakeReference(vtkCellArray::New());
    newStrips->AllocateCopy(inStrips);
    outStrpData.TakeReference(vtkCellData::New());
    outStrpData->CopyAllocate(inCD);

    for (inStrips->InitTraversal(); inStrips->GetNextCell(npts, pts); inCellID++)
    {
      cellIds.clear();
      for (i = 0; i < npts; i++)
      {
        ptId = pmap[pts[i]];
        if (std::find(cellIds.begin(), cellIds.end(), ptId) == cellIds.end())
        {
          cellIds.push_back(ptId);
        }
      }
      if (cellIds.size() > 3)
      {
        newId = newStrips->InsertNextCell(cellIds.size(), cellIds.data());
        outStrpData->CopyData(inCD, inCellID, newId);
      }
      else if (cellIds.size() == 3 && this->ConvertStripsToPolys)
      {
        if (!newPolys)
        {
          newPolys.TakeReference(vtkCellArray::New());
          newPolys->AllocateEstimate(5, 3);
          outPolyData.TakeReference(vtkCellData::New());
          outPolyData->CopyAllocate(inCD);
        }
        newId = newPolys->InsertNextCell(cellIds.size(), cellIds.data());
        outPolyData->CopyData(inCD, inCellID, newId);
      }
      else if (cellIds.size() == 2 && this->ConvertPolysToLines)
      {
        if (!newLines)
        {
          newLines.TakeReference(vtkCellArray::New());
          newLines->AllocateEstimate(5, 2);
          outLineData.TakeReference(vtkCellData::New());
          outLineData->CopyAllocate(inCD);
        }
        newId = newLines->InsertNextCell(cellIds.size(), cellIds.data());
        outLineData->CopyData(inCD, inCellID, newId);
      }
      else if (cellIds.size() == 1 && this->ConvertLinesToPoints)
      {
        if (!newVerts)
        {
          newVerts.TakeReference(vtkCellArray::New());
          newVerts->AllocateEstimate(5, 1);
        }
        newId = newVerts->InsertNextCell(cellIds.size(), cellIds.data());
        outCD->CopyData(inCD, inCellID, newId);
      }
    }
    vtkDebugMacro(<< "Removed " << inStrips->GetNumberOfCells() - newStrips->GetNumberOfCells()
                  << " strips");
  }

  vtkDebugMacro(<< "Removed " << numNewPts - newPts->GetNumberOfPoints() << " points");

  // Update ourselves and release memory
  //
  this->Locator->Initialize(); // release memory.

  // Now transfer all CellData from Lines/Polys/Strips into final
  // cell attribute data. The vertex cell data has already been inserted.
  vtkIdType cellCounter = (newVerts == nullptr ? 0 : newVerts->GetNumberOfCells());
  vtkIdType lineCounter = (newLines == nullptr ? 0 : newLines->GetNumberOfCells());
  vtkIdType polyCounter = (newPolys == nullptr ? 0 : newPolys->GetNumberOfCells());
  vtkIdType strpCounter = (newStrips == nullptr ? 0 : newStrips->GetNumberOfCells());

  if (newLines)
  {
    for (i = 0; i < lineCounter; ++i, ++cellCounter)
    {
      outCD->CopyData(outLineData, i, cellCounter);
    }
  }
  if (newPolys)
  {
    for (i = 0; i < polyCounter; ++i, ++cellCounter)
    {
      outCD->CopyData(outPolyData, i, cellCounter);
    }
  }
  if (newStrips)
  {
    for (i = 0; i < strpCounter; ++i, ++cellCounter)
    {
      outCD->CopyData(outStrpData, i, cellCounter);
    }
  }

  // Update the output connectivity
  if (newVerts)
  {
    output->SetVerts(newVerts);
  }
  if (newLines)
  {
    output->SetLines(newLines);
  }
  if (newPolys)
  {
    output->SetPolys(newPolys);
  }
  if (newStrips)
  {
    output->SetStrips(newStrips);
  }

  return 1;
}

//------------------------------------------------------------------------------
vtkMTimeType vtkStaticCleanPolyData::GetMTime()
{
  vtkMTimeType mTime = this->vtkObject::GetMTime();
  vtkMTimeType time = this->Locator->GetMTime();
  return (time > mTime ? time : mTime);
}

//------------------------------------------------------------------------------
void vtkStaticCleanPolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ToleranceIsAbsolute: " << (this->ToleranceIsAbsolute ? "On\n" : "Off\n");
  os << indent << "Tolerance: " << (this->Tolerance ? "On\n" : "Off\n");
  os << indent << "AbsoluteTolerance: " << (this->AbsoluteTolerance ? "On\n" : "Off\n");

  if (this->MergingArray)
  {
    os << indent << "Merging Array: " << this->MergingArray << "\n";
  }
  else
  {
    os << indent << "Merging Array: (none)\n";
  }

  os << indent << "ConvertPolysToLines: " << (this->ConvertPolysToLines ? "On\n" : "Off\n");
  os << indent << "ConvertLinesToPoints: " << (this->ConvertLinesToPoints ? "On\n" : "Off\n");
  os << indent << "ConvertStripsToPolys: " << (this->ConvertStripsToPolys ? "On\n" : "Off\n");

  if (this->Locator)
  {
    os << indent << "Locator: " << this->Locator << "\n";
  }
  else
  {
    os << indent << "Locator: (none)\n";
  }
  os << indent << "Remove Unused Points: " << (this->RemoveUnusedPoints ? "On\n" : "Off\n");
  os << indent << "Produce Merge Map: " << (this->ProduceMergeMap ? "On\n" : "Off\n");
  os << indent << "Average Point Data: " << (this->AveragePointData ? "On\n" : "Off\n");
  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
  os << indent << "PieceInvariant: " << (this->PieceInvariant ? "On\n" : "Off\n");
}
