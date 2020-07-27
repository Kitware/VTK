/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataEdgeConnectivityFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPolyDataEdgeConnectivityFilter.h"

#include "vtkCell.h"
#include "vtkCellArray.h"
#include "vtkCellArrayIterator.h"
#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkDoubleArray.h"
#include "vtkEdgeTable.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolygon.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <algorithm> // for fill_n
#include <memory>    //for unique_ptr

vtkStandardNewMacro(vtkPolyDataEdgeConnectivityFilter);

//------------------------------------------------------------------------------
// Construct with default extraction mode to extract largest regions.
vtkPolyDataEdgeConnectivityFilter::vtkPolyDataEdgeConnectivityFilter()
{
  this->RegionSizes = vtkSmartPointer<vtkIdTypeArray>::New();
  this->ExtractionMode = VTK_EXTRACT_LARGEST_REGION;

  // Controlling connectivity
  this->BarrierEdges = 0;
  this->BarrierEdgeLength[0] = VTK_DOUBLE_MAX;
  this->BarrierEdgeLength[1] = VTK_DOUBLE_MAX;

  this->ScalarConnectivity = 0;
  this->ScalarRange[0] = 0.0;
  this->ScalarRange[1] = 1.0;

  this->ClosestPoint[0] = this->ClosestPoint[1] = this->ClosestPoint[2] = 0.0;

  this->CellNeighbors = vtkSmartPointer<vtkIdList>::New();
  this->CellEdgeNeighbors = vtkSmartPointer<vtkIdList>::New();

  this->GrowLargeRegions = 0;
  this->LargeRegionThreshold = 0.10;

  this->ColorRegions = 1;

  this->OutputPointsPrecision = DEFAULT_PRECISION;

  // optional 2nd input
  this->SetNumberOfInputPorts(2);
}

//------------------------------------------------------------------------------
vtkPolyDataEdgeConnectivityFilter::~vtkPolyDataEdgeConnectivityFilter() {}

//------------------------------------------------------------------------------
void vtkPolyDataEdgeConnectivityFilter::SetSourceData(vtkPolyData* input)
{
  this->Superclass::SetInputData(1, input);
}

//------------------------------------------------------------------------------
// Specify the input data or filter. New style.
void vtkPolyDataEdgeConnectivityFilter::SetSourceConnection(vtkAlgorithmOutput* algOutput)
{
  this->Superclass::SetInputConnection(1, algOutput);
}

//------------------------------------------------------------------------------
vtkPolyData* vtkPolyDataEdgeConnectivityFilter::GetSource()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return nullptr;
  }
  return vtkPolyData::SafeDownCast(this->GetExecutive()->GetInputData(1, 0));
}

//------------------------------------------------------------------------------
int vtkPolyDataEdgeConnectivityFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPolyData* input = vtkPolyData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* source = nullptr;
  if (sourceInfo)
  {
    source = vtkPolyData::SafeDownCast(sourceInfo->Get(vtkDataObject::DATA_OBJECT()));
  }
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType cellId, newCellId, i, pt;
  vtkPoints* inPts;
  vtkIdType *cells, id, n;
  vtkIdType npts;
  const vtkIdType* pts;
  vtkIdType ncells;
  vtkIdType maxCellsInRegion;
  vtkIdType largestRegionId = 0;
  vtkPointData *pd = input->GetPointData(), *outputPD = output->GetPointData();
  vtkCellData *cd = input->GetCellData(), *outputCD = output->GetCellData();

  vtkDebugMacro(<< "Executing polygon edge-connected filter.");

  //  Check input/allocate storage
  //
  inPts = input->GetPoints();

  if (inPts == nullptr)
  {
    vtkErrorMacro("No input points!");
    return 1;
  }

  const vtkIdType numPts = inPts->GetNumberOfPoints();
  const vtkIdType numCells = input->GetNumberOfCells();

  if (numPts < 1 || numCells < 1)
  {
    vtkDebugMacro(<< "No data to connect!");
    return 1;
  }

  // See whether to consider (cell) scalar connectivity
  //
  this->InScalars = input->GetCellData()->GetScalars();
  if (!this->ScalarConnectivity)
  {
    this->InScalars = nullptr;
  }
  else
  {
    if (this->ScalarRange[1] < this->ScalarRange[0])
    {
      this->ScalarRange[1] = this->ScalarRange[0];
    }
  }

  // Build cell structure. Note that although only polygons are processed, we
  // have to worry about the cell data to ensure the ids match when copying
  // the cell data from input to output.
  //
  this->Mesh = vtkSmartPointer<vtkPolyData>::New();
  this->Mesh->CopyStructure(input);
  this->Mesh->BuildLinks();
  this->UpdateProgress(0.10);

  // If barrier edges are enabled, set these up
  if (this->BarrierEdges)
  {
    this->BRange2[0] = this->BarrierEdgeLength[0] * this->BarrierEdgeLength[0];
    this->BRange2[1] = this->BarrierEdgeLength[1] * this->BarrierEdgeLength[1];
    if (source)
    {
      this->Barriers = vtkSmartPointer<vtkEdgeTable>::New();
      this->Barriers->InitEdgeInsertion(source->GetNumberOfPoints());
      vtkCellArray* lines = source->GetLines();
      for (lines->InitTraversal(); lines->GetNextCell(npts, pts);)
      {
        for (i = 0; i < (npts - 1); ++i)
        {
          this->Barriers->InsertEdge(pts[i], pts[i + 1]);
        }
      }
    }
  }

  // Initialize.  Keep track of points and cells visited, and the region ids
  // of the cells.
  this->RegionIds.reserve(numCells);
  std::fill_n(this->RegionIds.begin(), numCells, (-1));

  this->PointMap.reserve(numPts);
  std::fill_n(this->PointMap.begin(), numPts, (-1));

  this->RegionSizes->Reset();

  // Set the desired precision for the points in the output.
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
  newPts->Allocate(numPts);

  // Traverse all cells marking those visited.  Each new search
  // starts a new connected region. Connected region grows
  // using a connected wave propagation.
  //
  this->Wave.reserve(numPts);
  this->Wave2.reserve(numPts);

  this->NumberOfPoints = 0;
  this->NumberOfRegions = 0;
  maxCellsInRegion = 0;

  this->CellIds = vtkSmartPointer<vtkIdList>::New();
  this->CellIds->Allocate(8, VTK_CELL_SIZE);
  this->PointIds = vtkSmartPointer<vtkIdList>::New();
  this->PointIds->Allocate(8, VTK_CELL_SIZE);

  if (this->ExtractionMode != VTK_EXTRACT_POINT_SEEDED_REGIONS &&
    this->ExtractionMode != VTK_EXTRACT_CELL_SEEDED_REGIONS &&
    this->ExtractionMode != VTK_EXTRACT_CLOSEST_POINT_REGION)
  { // visit all cells marking with region number
    for (cellId = 0; cellId < numCells; cellId++)
    {
      if (cellId && !(cellId % 5000))
      {
        this->UpdateProgress(0.1 + 0.8 * cellId / numCells);
      }

      if (this->RegionIds[cellId] < 0)
      {
        this->NumCellsInRegion = 0;
        this->Wave.push_back(cellId);
        this->TraverseAndMark();

        if (this->NumCellsInRegion > maxCellsInRegion)
        {
          maxCellsInRegion = this->NumCellsInRegion;
          largestRegionId = this->NumberOfRegions;
        }

        this->RegionSizes->InsertValue(this->NumberOfRegions++, this->NumCellsInRegion);
        this->Wave.clear();
        this->Wave2.clear();
      }
    }
  }
  else // regions have been seeded, everything considered in same region
  {
    this->NumCellsInRegion = 0;

    if (this->ExtractionMode == VTK_EXTRACT_POINT_SEEDED_REGIONS)
    {
      for (i = 0; i < (vtkIdType)this->Seeds.size(); i++)
      {
        pt = this->Seeds[i];
        if (pt >= 0)
        {
          this->Mesh->GetPointCells(pt, ncells, cells);
          for (vtkIdType j = 0; j < ncells; ++j)
          {
            this->Wave.push_back(cells[j]);
          }
        }
      }
    }
    else if (this->ExtractionMode == VTK_EXTRACT_CELL_SEEDED_REGIONS)
    {
      for (i = 0; i < (vtkIdType)this->Seeds.size(); i++)
      {
        cellId = this->Seeds[i];
        if (cellId >= 0)
        {
          this->Wave.push_back(cellId);
        }
      }
    }
    else if (this->ExtractionMode == VTK_EXTRACT_CLOSEST_POINT_REGION)
    { // loop over points, find closest one
      double minDist2, dist2, x[3];
      int minId = 0;
      for (minDist2 = VTK_DOUBLE_MAX, i = 0; i < numPts; i++)
      {
        inPts->GetPoint(i, x);
        dist2 = vtkMath::Distance2BetweenPoints(x, this->ClosestPoint);
        if (dist2 < minDist2)
        {
          minId = i;
          minDist2 = dist2;
        }
      }
      this->Mesh->GetPointCells(minId, ncells, cells);
      for (vtkIdType j = 0; j < ncells; ++j)
      {
        this->Wave.push_back(cells[j]);
      }
    }
    this->UpdateProgress(0.5);

    // mark all seeded regions
    this->TraverseAndMark();
    this->RegionSizes->InsertValue(this->NumberOfRegions, this->NumCellsInRegion);
    this->UpdateProgress(0.9);
  } // else extracted seeded cells

  vtkDebugMacro(<< "Identified " << this->NumberOfRegions << " region(s)");

  // Optionally, assimilate small regions into bigger regions.
  if (this->ExtractionMode == VTK_EXTRACT_LARGE_REGIONS || this->GrowLargeRegions)
  {
    this->ComputeRegionAreas();
    if (this->GrowLargeRegions)
    {
      this->GrowRegions();
    }
  }

  // Now that points and cells have been marked, traverse these lists pulling
  // everything that has been visited.
  //
  // Pass through point data that has been visited
  outputPD->CopyAllocate(pd);
  outputCD->CopyAllocate(cd);

  for (i = 0; i < numPts; i++)
  {
    if (this->PointMap[i] > -1)
    {
      newPts->InsertPoint(this->PointMap[i], inPts->GetPoint(i));
      outputPD->CopyData(pd, i, this->PointMap[i]);
    }
  }

  // if coloring regions; send down new scalar data
  vtkSmartPointer<vtkIdTypeArray> cellRegionIds;
  if (this->ColorRegions)
  {
    cellRegionIds = vtkSmartPointer<vtkIdTypeArray>::New();
    cellRegionIds->SetName("RegionId");
    cellRegionIds->Allocate(numCells);
    int idx = outputCD->AddArray(cellRegionIds);
    outputCD->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);
  }

  // Set up
  output->SetPoints(newPts);
  if ((n = input->GetPolys()->GetNumberOfCells()) > 0)
  {
    vtkNew<vtkCellArray> newPolys;
    newPolys->AllocateEstimate(n, 3);
    output->SetPolys(newPolys);
  }

  if (this->ExtractionMode == VTK_EXTRACT_POINT_SEEDED_REGIONS ||
    this->ExtractionMode == VTK_EXTRACT_CELL_SEEDED_REGIONS ||
    this->ExtractionMode == VTK_EXTRACT_CLOSEST_POINT_REGION ||
    this->ExtractionMode == VTK_EXTRACT_ALL_REGIONS)
  { // extract any cell that's been visited
    for (cellId = 0; cellId < numCells; cellId++)
    {
      if (this->RegionIds[cellId] >= 0)
      {
        this->Mesh->GetCellPoints(cellId, npts, pts);
        this->PointIds->Reset();
        for (i = 0; i < npts; i++)
        {
          id = this->PointMap[pts[i]];
          this->PointIds->InsertId(i, id);
        }
        newCellId = output->InsertNextCell(this->Mesh->GetCellType(cellId), this->PointIds);
        outputCD->CopyData(cd, cellId, newCellId);
        if (cellRegionIds != nullptr)
        {
          cellRegionIds->InsertValue(newCellId, this->RegionIds[cellId]);
        }
      }
    }
  }
  else if (this->ExtractionMode == VTK_EXTRACT_SPECIFIED_REGIONS)
  {
    for (cellId = 0; cellId < numCells; cellId++)
    {
      int inReg, regionId;
      if ((regionId = this->RegionIds[cellId]) >= 0)
      {
        for (inReg = 0, i = 0; i < (vtkIdType)this->SpecifiedRegionIds.size(); i++)
        {
          if (regionId == this->SpecifiedRegionIds[i])
          {
            inReg = 1;
            break;
          }
        }
        if (inReg)
        {
          this->Mesh->GetCellPoints(cellId, npts, pts);
          this->PointIds->Reset();
          for (i = 0; i < npts; i++)
          {
            id = this->PointMap[pts[i]];
            this->PointIds->InsertId(i, id);
          }
          newCellId = output->InsertNextCell(this->Mesh->GetCellType(cellId), this->PointIds);
          outputCD->CopyData(cd, cellId, newCellId);
          if (cellRegionIds != nullptr)
          {
            cellRegionIds->InsertValue(newCellId, this->RegionIds[cellId]);
          }
        }
      }
    }
  }
  else if (this->ExtractionMode == VTK_EXTRACT_LARGEST_REGION) // extract largest region
  {
    for (cellId = 0; cellId < numCells; cellId++)
    {
      if (this->RegionIds[cellId] == largestRegionId)
      {
        this->Mesh->GetCellPoints(cellId, npts, pts);
        this->PointIds->Reset();
        for (i = 0; i < npts; i++)
        {
          id = this->PointMap[pts[i]];
          this->PointIds->InsertId(i, id);
        }
        newCellId = output->InsertNextCell(this->Mesh->GetCellType(cellId), this->PointIds);
        outputCD->CopyData(cd, cellId, newCellId);
        if (cellRegionIds != nullptr)
        {
          cellRegionIds->InsertValue(newCellId, this->RegionIds[cellId]);
        }
      }
    }
  }
  else // if ( this->ExtractionMode == VTK_EXTRACT_LARGE_REGIONS )
  {
    for (cellId = 0; cellId < numCells; cellId++)
    {
      if (this->RegionIds[cellId] >= 0 && this->RegionClassification[this->RegionIds[cellId]] == 1)
      {
        this->Mesh->GetCellPoints(cellId, npts, pts);
        this->PointIds->Reset();
        for (i = 0; i < npts; i++)
        {
          id = this->PointMap[pts[i]];
          this->PointIds->InsertId(i, id);
        }
        newCellId = output->InsertNextCell(this->Mesh->GetCellType(cellId), this->PointIds);
        outputCD->CopyData(cd, cellId, newCellId);
        if (cellRegionIds != nullptr)
        {
          cellRegionIds->InsertValue(newCellId, this->RegionIds[cellId]);
        }
      }
    }
  }

  output->Squeeze();

#ifndef NDEBUG
  int num = this->GetNumberOfExtractedRegions();
  vtkIdType count = 0;
  for (int ii = 0; ii < num; ii++)
  {
    count += this->RegionSizes->GetValue(ii);
  }
  vtkDebugMacro(<< "Total # of cells accounted for: " << count);
  vtkDebugMacro(<< "Extracted " << output->GetNumberOfCells() << " cells");
#endif

  return 1;
}

//------------------------------------------------------------------------------
int vtkPolyDataEdgeConnectivityFilter::IsScalarConnected(vtkIdType cellId, vtkIdType neiId)
{
  double* range = this->ScalarRange;
  double sCell = this->InScalars->GetComponent(cellId, 0);
  double sNei = this->InScalars->GetComponent(neiId, 0);
  return (
    ((sCell >= range[0] && sCell <= range[1]) && (sNei >= range[0] && sNei <= range[1])) ? 1 : 0);
}

//------------------------------------------------------------------------------
// Is this edge a barrier to traversal?
bool vtkPolyDataEdgeConnectivityFilter::IsBarrierEdge(vtkIdType p0, vtkIdType p1)
{
  if (this->BarrierEdges)
  {
    double x0[3], x1[3];
    this->Mesh->GetPoint(p0, x0);
    this->Mesh->GetPoint(p1, x1);
    double len2 = vtkMath::Distance2BetweenPoints(x0, x1);
    if ((len2 >= this->BRange2[0] && len2 <= this->BRange2[1]) ||
      (this->Barriers != nullptr && this->Barriers->IsEdge(p0, p1) >= 0))
    {
      return true;
    }
  }
  return false;
}

//------------------------------------------------------------------------------
// Get the neighbors that satisfy all constraints including on scalar values
// and barrier edges.
void vtkPolyDataEdgeConnectivityFilter::GetConnectedNeighbors(
  vtkIdType cellId, vtkIdType npts, const vtkIdType* pts, vtkIdList* neis)
{
  neis->Reset();

  // Make sure input is valid
  if (npts < 2)
  {
    return;
  }

  // For each edge of the polygon, add the edge neighbor
  vtkIdType p0, p1, numEdgeNeis, neiId;
  for (auto i = 0; i < npts; ++i)
  {
    p0 = pts[i];
    p1 = pts[(i + 1) % npts];
    if (!this->IsBarrierEdge(p0, p1))
    {
      this->Mesh->GetCellEdgeNeighbors(cellId, p0, p1, this->CellEdgeNeighbors);
      numEdgeNeis = this->CellEdgeNeighbors->GetNumberOfIds();
      for (auto j = 0; j < numEdgeNeis; ++j)
      {
        neiId = this->CellEdgeNeighbors->GetId(j);
        // Check scalar connectivity
        if (!this->InScalars || this->IsScalarConnected(cellId, neiId))
        {
          neis->InsertNextId(neiId);
        }
      }
    } // if edge is not a barrier
  }   // for each edge
}

//------------------------------------------------------------------------------
// Mark current cell as visited and assign region number.  Note:
// traversal occurs across shared vertices.
//
void vtkPolyDataEdgeConnectivityFilter::TraverseAndMark()
{
  vtkIdType cellId, neiId, ptId, numIds, i, j;
  vtkIdType numNeis;
  const vtkIdType* pts;
  vtkIdType npts;
  const vtkIdType numCells = this->Mesh->GetNumberOfCells();

  while ((numIds = static_cast<vtkIdType>(this->Wave.size())) > 0)
  {
    for (i = 0; i < numIds; i++)
    {
      cellId = this->Wave[i];
      if (this->RegionIds[cellId] < 0)
      {
        this->RegionIds[cellId] = this->NumberOfRegions;
        this->NumCellsInRegion++;

        // This method is needed to mark points, and get cell neighbors.
        this->Mesh->GetCellPoints(cellId, npts, pts);

        // Mark points as being used
        for (j = 0; j < npts; j++)
        {
          if (this->PointMap[ptId = pts[j]] < 0)
          {
            this->PointMap[ptId] = this->NumberOfPoints++;
          }
        }

        // Add cell edge neighbors to queue. This takes into account
        // the barrier edges, and scalar connectivity.
        this->GetConnectedNeighbors(cellId, npts, pts, this->CellNeighbors);
        numNeis = this->CellNeighbors->GetNumberOfIds();
        for (j = 0; j < numNeis; ++j)
        {
          neiId = this->CellNeighbors->GetId(j);
          if (this->RegionIds[neiId] < 0)
          {
            this->Wave2.push_back(neiId);
          }
        } // for all neighboring cells to this cell
      }   // if cell not yet visited
    }     // for all cells in this wave

    this->Wave = this->Wave2;
    this->Wave2.clear();
    this->Wave2.reserve(numCells);
  } // while wave is not empty
}

//------------------------------------------------------------------------------
// Obtain the number of connected regions.
int vtkPolyDataEdgeConnectivityFilter::GetNumberOfExtractedRegions()
{
  return this->RegionSizes->GetMaxId() + 1;
}

//------------------------------------------------------------------------------
// Initialize list of point ids/cell ids used to seed regions.
void vtkPolyDataEdgeConnectivityFilter::InitializeSeedList()
{
  this->Modified();
  this->Seeds.clear();
}

//------------------------------------------------------------------------------
// Add a seed id (point or cell id). Note: ids are 0-offset.
void vtkPolyDataEdgeConnectivityFilter::AddSeed(int id)
{
  this->Modified();
  this->Seeds.push_back(id);
}

//------------------------------------------------------------------------------
// Delete a seed id (point or cell id). Note: ids are 0-offset.
void vtkPolyDataEdgeConnectivityFilter::DeleteSeed(int id)
{
  this->Modified();
  auto iter = remove_if(
    this->Seeds.begin(), this->Seeds.end(), [&id](vtkIdType val) -> bool { return val == id; });
  this->Seeds.erase(iter, this->Seeds.end());
}

//------------------------------------------------------------------------------
// Initialize list of region ids to extract.
void vtkPolyDataEdgeConnectivityFilter::InitializeSpecifiedRegionList()
{
  this->Modified();
  this->SpecifiedRegionIds.clear();
}

//------------------------------------------------------------------------------
// Add a region id to extract. Note: ids are 0-offset.
void vtkPolyDataEdgeConnectivityFilter::AddSpecifiedRegion(int id)
{
  this->Modified();
  this->SpecifiedRegionIds.push_back(id);
}

//------------------------------------------------------------------------------
// Delete a region id to extract. Note: ids are 0-offset.
void vtkPolyDataEdgeConnectivityFilter::DeleteSpecifiedRegion(int id)
{
  this->Modified();
  auto iter = remove_if(this->SpecifiedRegionIds.begin(), this->SpecifiedRegionIds.end(),
    [&id](vtkIdType val) -> bool { return val == id; });
  this->SpecifiedRegionIds.erase(iter, this->SpecifiedRegionIds.end());
}

//------------------------------------------------------------------------------
// Return the number of specified regions.
int vtkPolyDataEdgeConnectivityFilter::GetNumberOfSpecifiedRegions()
{
  return static_cast<int>(this->SpecifiedRegionIds.size());
}

//------------------------------------------------------------------------------
int vtkPolyDataEdgeConnectivityFilter::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  }
  else if (port == 1)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  return 1;
}

//------------------------------------------------------------------------------
double vtkPolyDataEdgeConnectivityFilter::ComputeRegionAreas()
{
  // Computer the area of each cell, and the total mesh area.
  // Also accumulate the area of each region.
  double totalArea = 0.0;
  const vtkIdType numCells = this->Mesh->GetPolys()->GetNumberOfCells();

  this->CellAreas.reserve(numCells);

  this->RegionAreas.reserve(this->NumberOfRegions);
  std::fill_n(this->RegionAreas.begin(), this->NumberOfRegions, 0.0);

  this->RegionClassification.reserve(this->NumberOfRegions);
  std::fill_n(this->RegionClassification.begin(), this->NumberOfRegions, 0);

  // Traverse polygons and compute area
  double area, normal[3];
  vtkIdType cellId, numCellPts;
  const vtkIdType* cellPts;
  vtkPoints* pts = this->Mesh->GetPoints();

  auto iter = vtk::TakeSmartPointer(this->Mesh->GetPolys()->NewIterator());
  for (cellId = 0; cellId < numCells; ++cellId)
  {
    if (this->RegionIds[cellId] >= 0) // cell assigned to region
    {
      iter->GetCellAtId(cellId, numCellPts, cellPts);
      area = vtkPolygon::ComputeArea(pts, numCellPts, cellPts, normal);
      totalArea += area;
      this->CellAreas[cellId] = area;
      this->RegionAreas[this->RegionIds[cellId]] += area;
    }
  }

  // Mark large regions
  double areaThreshold = this->LargeRegionThreshold * totalArea;
  for (int regNum = 0; regNum < this->NumberOfRegions; ++regNum)
  {
    if (this->RegionAreas[regNum] >= areaThreshold)
    {
      this->RegionClassification[regNum] = 1;
    }
  }

  return totalArea;
}

//------------------------------------------------------------------------------
// Loop over cells, those in small regions are assigned to larger regions if they
// are "close" enough. This is iterative.
void vtkPolyDataEdgeConnectivityFilter::GrowRegions()
{
  // Reuse the Wave vector to load up cells in small regions. We want to eliminate
  // looping over all cells and just process the cells in small regions.
  this->Wave.clear();
  const vtkIdType numCells = this->Mesh->GetPolys()->GetNumberOfCells();
  for (auto cellId = 0; cellId < numCells; ++cellId)
  {
    vtkIdType regId = this->RegionIds[cellId];
    if (regId >= 0 && this->RegionClassification[regId] == 0)
    {
      this->Wave.push_back(cellId);
    }
  }

  // Iteratively assign cells to large regions. It may be that some cells
  // cannot be assigned, so when nothing changes, we terminate. Note that
  // currently this is a two-pass algorithm. In the first pass, if a large
  // region borders the longest cell edge (of a candidate cell in a small
  // region), then the large region assimilates the cell. Once this first
  // pass is exhausted, then in the second pass, any large region neighboring
  // the candidate cell will assimilate the candidate cell (regardless of
  // edge length).
  auto iter = vtk::TakeSmartPointer(this->Mesh->GetPolys()->NewIterator());
  vtkIdType npts;
  const vtkIdType* pts;
  vtkIdType numCandidates = (vtkIdType)this->Wave.size();

  // Grow large regions over multiple passes
  for (this->CurrentGrowPass = 0; this->CurrentGrowPass < 2; ++this->CurrentGrowPass)
  {
    bool somethingChanged = true;
    while (somethingChanged)
    {
      somethingChanged = false;
      for (auto candidate = 0; candidate < numCandidates; ++candidate)
      {
        vtkIdType cellId = this->Wave[candidate];
        vtkIdType regId = this->RegionIds[cellId];
        if (regId >= 0 && this->RegionClassification[regId] == 0)
        {
          iter->GetCellAtId(cellId, npts, pts);
          vtkIdType largeRegId = this->AssimilateCell(cellId, npts, pts);
          if (largeRegId >= 0)
          {
            somethingChanged = true;
            this->RegionIds[cellId] = largeRegId;
          }
        } // if in small region, or no region
      }   // for all candidates
    }     // while things are changing
  }       // for each region growing pass
}

//------------------------------------------------------------------------------
// Return a non-negative region id if the cell specified can be assimilated
// into a large region. Currently a two-pass algorithm is used. This certainly
// could be improved in the future.
int vtkPolyDataEdgeConnectivityFilter::AssimilateCell(
  vtkIdType cellId, vtkIdType npts, const vtkIdType* pts)
{
  double e2, longestAdjacentEdge2 = 0.0, longestEdge2 = 0.0, x0[3], x1[3];
  int adjacentRegion = (-1);
  vtkIdType p0, p1, numEdgeNeis;
  vtkIdType regId, neiId;
  vtkIdType longestAdjacentNum(-1), longestEdgeNum = (-1);

  // Loop over all edges
  for (auto i = 0; i < npts; ++i)
  {
    p0 = pts[i];
    p1 = pts[(i + 1) % npts];

    // Identify the longest edge in the cell
    this->Mesh->GetPoint(p0, x0);
    this->Mesh->GetPoint(p1, x1);
    e2 = vtkMath::Distance2BetweenPoints(x0, x1);
    if (e2 > longestEdge2)
    {
      longestEdgeNum = i;
      longestEdge2 = e2;
    }

    // Find the longest edge with a neighbor cell classified in a large region.
    this->Mesh->GetCellEdgeNeighbors(cellId, p0, p1, this->CellEdgeNeighbors);
    numEdgeNeis = this->CellEdgeNeighbors->GetNumberOfIds();
    for (auto j = 0; j < numEdgeNeis; ++j)
    {
      neiId = this->CellEdgeNeighbors->GetId(j);
      regId = this->RegionIds[neiId];

      if (regId >= 0 && this->RegionClassification[regId] == 1)
      {
        if (e2 > longestAdjacentEdge2)
        {
          longestAdjacentNum = i;
          longestAdjacentEdge2 = e2;
          adjacentRegion = regId;
        }
      }
    }
  } // for each edge

  // Depending on pass number, return the appropriate information.
  if (this->CurrentGrowPass == 0)
  {
    return (longestEdgeNum >= 0 && longestEdgeNum == longestAdjacentNum ? adjacentRegion : -1);
  }
  else // relax constraint on longest edge
  {
    return adjacentRegion;
  }
}

//------------------------------------------------------------------------------
void vtkPolyDataEdgeConnectivityFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Extraction Mode: ";
  os << this->GetExtractionModeAsString() << "\n";

  os << indent << "Barrier Edges: " << (this->BarrierEdges ? "On\n" : "Off\n");
  double* elen = this->GetBarrierEdgeLength();
  os << indent << "Barrier Edge Length: (" << elen[0] << ", " << elen[1] << ")\n";

  os << indent << "Scalar Connectivity: " << (this->ScalarConnectivity ? "On\n" : "Off\n");
  double* range = this->GetScalarRange();
  os << indent << "Scalar Range: (" << range[0] << ", " << range[1] << ")\n";

  os << indent << "Closest Point: (" << this->ClosestPoint[0] << ", " << this->ClosestPoint[1]
     << ", " << this->ClosestPoint[2] << ")\n";

  os << indent << "RegionSizes: ";
  if (this->GetNumberOfExtractedRegions() > 10)
  {
    os << "Only first ten of " << this->GetNumberOfExtractedRegions() << " listed";
  }
  os << std::endl;

  for (vtkIdType id = 0;
       id < (this->GetNumberOfExtractedRegions() > 10 ? 10 : this->GetNumberOfExtractedRegions());
       id++)
  {
    os << indent << indent << id << ": " << this->RegionSizes->GetValue(id) << std::endl;
  }

  os << indent << "Grow Large Regions: " << (this->GrowLargeRegions ? "On\n" : "Off\n");
  os << indent << "Large Region Threshold: " << this->LargeRegionThreshold << "\n";

  os << indent << "Color Regions: " << (this->ColorRegions ? "On\n" : "Off\n");

  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
}
