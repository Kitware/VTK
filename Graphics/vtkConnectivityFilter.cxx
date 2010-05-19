/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkConnectivityFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkConnectivityFilter.h"

#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkUnstructuredGrid.h"
#include "vtkIdTypeArray.h"

vtkStandardNewMacro(vtkConnectivityFilter);

// Construct with default extraction mode to extract largest regions.
vtkConnectivityFilter::vtkConnectivityFilter()
{
  this->RegionSizes = vtkIdTypeArray::New();
  this->ExtractionMode = VTK_EXTRACT_LARGEST_REGION;
  this->ColorRegions = 0;

  this->ScalarConnectivity = 0;
  this->ScalarRange[0] = 0.0;
  this->ScalarRange[1] = 1.0;

  this->ClosestPoint[0] = this->ClosestPoint[1] = this->ClosestPoint[2] = 0.0;

  this->CellScalars = vtkFloatArray::New(); 
  this->CellScalars->Allocate(8);

  this->NeighborCellPointIds = vtkIdList::New();
  this->NeighborCellPointIds->Allocate(8);
  
  this->Seeds = vtkIdList::New();
  this->SpecifiedRegionIds = vtkIdList::New();

  this->NewScalars = 0;
  this->NewCellScalars = 0;
}

vtkConnectivityFilter::~vtkConnectivityFilter()
{
  this->RegionSizes->Delete();
  this->CellScalars->Delete();
  this->NeighborCellPointIds->Delete();
  this->Seeds->Delete();
  this->SpecifiedRegionIds->Delete();
}

int vtkConnectivityFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType numPts, numCells, cellId, newCellId, i, j, pt;
  vtkPoints *newPts;
  int id;
  int maxCellsInRegion;
  int largestRegionId = 0;
  vtkPointData *pd=input->GetPointData(), *outputPD=output->GetPointData();
  vtkCellData *cd=input->GetCellData(), *outputCD=output->GetCellData();
  
  vtkDebugMacro(<<"Executing connectivity filter.");

  //  Check input/allocate storage
  //
  numCells=input->GetNumberOfCells();
  if ( (numPts=input->GetNumberOfPoints()) < 1 || numCells < 1 )
    {
    vtkDebugMacro(<<"No data to connect!");
    return 1;
    }
  output->Allocate(numCells,numCells);

  // See whether to consider scalar connectivity
  //
  this->InScalars = input->GetPointData()->GetScalars();
  if ( !this->ScalarConnectivity ) 
    {
    this->InScalars = NULL;
    }
  else
    {
    if ( this->ScalarRange[1] < this->ScalarRange[0] ) 
      {
      this->ScalarRange[1] = this->ScalarRange[0];
      }
    }

  // Initialize.  Keep track of points and cells visited.
  //
  this->RegionSizes->Reset();
  this->Visited = new vtkIdType[numCells];
  for ( i=0; i < numCells; i++ )
    {
    this->Visited[i] = -1;
    }
  this->PointMap = new vtkIdType[numPts];  
  for ( i=0; i < numPts; i++ )
    {
    this->PointMap[i] = -1;
    }

  this->NewScalars = vtkIdTypeArray::New();
  this->NewScalars->SetName("RegionId");
  this->NewScalars->SetNumberOfTuples(numPts);

  this->NewCellScalars = vtkIdTypeArray::New();
  this->NewCellScalars->SetName("RegionId");
  this->NewCellScalars->SetNumberOfTuples(numCells);

  newPts = vtkPoints::New();
  newPts->Allocate(numPts);

  // Traverse all cells marking those visited.  Each new search
  // starts a new connected region. Connected region grows 
  // using a connected wave propagation.
  //
  this->Wave = vtkIdList::New();
  this->Wave->Allocate(numPts/4+1,numPts);
  this->Wave2 = vtkIdList::New();
  this->Wave2->Allocate(numPts/4+1,numPts);

  this->PointNumber = 0;
  this->RegionNumber = 0;
  maxCellsInRegion = 0;

  this->CellIds = vtkIdList::New(); 
  this->CellIds->Allocate(8, VTK_CELL_SIZE);
  this->PointIds = vtkIdList::New(); 
  this->PointIds->Allocate(8, VTK_CELL_SIZE);

  if ( this->ExtractionMode != VTK_EXTRACT_POINT_SEEDED_REGIONS && 
  this->ExtractionMode != VTK_EXTRACT_CELL_SEEDED_REGIONS &&
  this->ExtractionMode != VTK_EXTRACT_CLOSEST_POINT_REGION ) 
    { //visit all cells marking with region number
    for (cellId=0; cellId < numCells; cellId++)
      {
      if ( cellId && !(cellId % 5000) )
        {
        this->UpdateProgress (0.1 + 0.8*cellId/numCells);
        }

      if ( this->Visited[cellId] < 0 ) 
        {
        this->NumCellsInRegion = 0;
        this->Wave->InsertNextId(cellId);
        this->TraverseAndMark (input);

        if ( this->NumCellsInRegion > maxCellsInRegion )
          {
          maxCellsInRegion = this->NumCellsInRegion;
          largestRegionId = this->RegionNumber;
          }

        this->RegionSizes->InsertValue(this->RegionNumber++,
                                       this->NumCellsInRegion);
        this->Wave->Reset();
        this->Wave2->Reset();
        }
      }
    }
  else // regions have been seeded, everything considered in same region
    {
    this->NumCellsInRegion = 0;

    if ( this->ExtractionMode == VTK_EXTRACT_POINT_SEEDED_REGIONS )
      {
      for (i=0; i < this->Seeds->GetNumberOfIds(); i++) 
        {
        pt = this->Seeds->GetId(i);
        if ( pt >= 0 ) 
          {
          input->GetPointCells(pt,this->CellIds);
          for (j=0; j < this->CellIds->GetNumberOfIds(); j++) 
            {
            this->Wave->InsertNextId(this->CellIds->GetId(j));
            }
          }
        }
      }
    else if ( this->ExtractionMode == VTK_EXTRACT_CELL_SEEDED_REGIONS )
      {
      for (i=0; i < this->Seeds->GetNumberOfIds(); i++) 
        {
        cellId = this->Seeds->GetId(i);
        if ( cellId >= 0 )
          {
          this->Wave->InsertNextId(cellId);
          }
        }
      }
    else if ( this->ExtractionMode == VTK_EXTRACT_CLOSEST_POINT_REGION )
      {//loop over points, find closest one
      double minDist2, dist2, x[3];
      vtkIdType minId = 0;
      for (minDist2=VTK_DOUBLE_MAX, i=0; i<numPts; i++)
        {
        input->GetPoint(i,x);
        dist2 = vtkMath::Distance2BetweenPoints(x,this->ClosestPoint);
        if ( dist2 < minDist2 )
          {
          minId = i;
          minDist2 = dist2;
          }
        }
      input->GetPointCells(minId,this->CellIds);
      for (j=0; j < this->CellIds->GetNumberOfIds(); j++) 
        {
        this->Wave->InsertNextId(this->CellIds->GetId(j));
        }
      }
    this->UpdateProgress (0.5);

    //mark all seeded regions
    this->TraverseAndMark (input);
    this->RegionSizes->InsertValue(this->RegionNumber,this->NumCellsInRegion);
    this->UpdateProgress (0.9);
    }

  vtkDebugMacro (<<"Extracted " << this->RegionNumber << " region(s)");
  this->Wave->Delete();
  this->Wave2->Delete();

  // Now that points and cells have been marked, traverse these lists pulling
  // everything that has been visited.
  //
  //Pass through point data that has been visited
  outputPD->CopyAllocate(pd);
  outputCD->CopyAllocate(cd);

  for (i=0; i < numPts; i++)
    {
    if ( this->PointMap[i] > -1 )
      {
      newPts->InsertPoint(this->PointMap[i],input->GetPoint(i));
      outputPD->CopyData(pd,i,this->PointMap[i]);
      }
    }

  // if coloring regions; send down new scalar data
  if ( this->ColorRegions )
    {
    int idx = outputPD->AddArray(this->NewScalars);
    outputPD->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);
    idx = outputCD->AddArray(this->NewCellScalars);
    outputCD->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);
    }
  this->NewScalars->Delete();
  this->NewCellScalars->Delete();

  output->SetPoints(newPts);
  newPts->Delete();

  // Create output cells
  //
  if ( this->ExtractionMode == VTK_EXTRACT_POINT_SEEDED_REGIONS ||
  this->ExtractionMode == VTK_EXTRACT_CELL_SEEDED_REGIONS ||
  this->ExtractionMode == VTK_EXTRACT_CLOSEST_POINT_REGION ||
  this->ExtractionMode == VTK_EXTRACT_ALL_REGIONS)
    { // extract any cell that's been visited
    for (cellId=0; cellId < numCells; cellId++)
      {
      if ( this->Visited[cellId] >= 0 )
        {
        input->GetCellPoints(cellId, this->PointIds);
        for (i=0; i < this->PointIds->GetNumberOfIds(); i++)
          {
          id = this->PointMap[this->PointIds->GetId(i)];
          this->PointIds->InsertId(i,id);
          }
        newCellId = output->InsertNextCell(input->GetCellType(cellId),
                                           this->PointIds);
        outputCD->CopyData(cd,cellId,newCellId);
        }
      }
    }
  else if ( this->ExtractionMode == VTK_EXTRACT_SPECIFIED_REGIONS )
    {
    for (cellId=0; cellId < numCells; cellId++)
      {
      int inReg, regionId;
      if ( (regionId=this->Visited[cellId]) >= 0 )
        {
        for (inReg=0,i=0; i<this->SpecifiedRegionIds->GetNumberOfIds(); i++)
          {
          if ( regionId == this->SpecifiedRegionIds->GetId(i) )
            {
            inReg = 1;
            break;
            }
          }
        if ( inReg )
          {
          input->GetCellPoints(cellId, this->PointIds);
          for (i=0; i < this->PointIds->GetNumberOfIds(); i++)
            {
            id = this->PointMap[this->PointIds->GetId(i)];
            this->PointIds->InsertId(i,id);
            }
          newCellId =output->InsertNextCell(input->GetCellType(cellId),
                                            this->PointIds);
          outputCD->CopyData(cd,cellId,newCellId);
          }
        }
      }
    }
  else //extract largest region
    {
    for (cellId=0; cellId < numCells; cellId++)
      {
      if ( this->Visited[cellId] == largestRegionId )
        {
        input->GetCellPoints(cellId, this->PointIds);
        for (i=0; i < this->PointIds->GetNumberOfIds(); i++)
          {
          id = this->PointMap[this->PointIds->GetId(i)];
          this->PointIds->InsertId(i,id);
          }
        newCellId = output->InsertNextCell(input->GetCellType(cellId),
                                           this->PointIds);
        outputCD->CopyData(cd,cellId,newCellId);
        }
      }
   }

  delete [] this->Visited;
  delete [] this->PointMap;
  this->PointIds->Delete();
  this->CellIds->Delete();
  output->Squeeze();
  vtkDataArray* outScalars = 0; 
  if (this->ColorRegions && (outScalars=output->GetPointData()->GetScalars()))
    {
    outScalars->Resize(output->GetNumberOfPoints());
    }

  int num = this->GetNumberOfExtractedRegions();
  int count = 0;

  for (int ii = 0; ii < num; ii++)
    {
    count += this->RegionSizes->GetValue (ii);
    }
  vtkDebugMacro (<< "Total # of cells accounted for: " << count);
  vtkDebugMacro (<< "Extracted " << output->GetNumberOfCells() << " cells");

  return 1;
}


// Mark current cell as visited and assign region number.  Note:
// traversal occurs across shared vertices.
//
void vtkConnectivityFilter::TraverseAndMark (vtkDataSet *input)
{
  vtkIdType i, j, k, cellId, numIds, ptId, numPts, numCells;
  vtkIdList *tmpWave;

  while ( (numIds=this->Wave->GetNumberOfIds()) > 0 )
    {
    for ( i=0; i < numIds; i++ )
      {
      cellId = this->Wave->GetId(i);
      if ( this->Visited[cellId] < 0 )
        {
        this->NewCellScalars->SetValue(cellId, this->RegionNumber);
        this->Visited[cellId] = this->RegionNumber;
        this->NumCellsInRegion++;
        input->GetCellPoints(cellId, this->PointIds);

        numPts = this->PointIds->GetNumberOfIds();
        for (j=0; j < numPts; j++) 
          {
          if ( this->PointMap[ptId=this->PointIds->GetId(j)] < 0 )
            {
            this->PointMap[ptId] = this->PointNumber++;
            this->NewScalars->SetValue(this->PointMap[ptId],
                                       this->RegionNumber);
            }

          input->GetPointCells(ptId,this->CellIds);

          // check connectivity criterion (geometric + scalar)
          numCells = this->CellIds->GetNumberOfIds();
          for (k=0; k < numCells; k++)
            {
            cellId = this->CellIds->GetId(k);
            if ( this->InScalars )
              {
              int numScalars, ii;
              double s, range[2];

              input->GetCellPoints(cellId, this->NeighborCellPointIds);
              numScalars = this->NeighborCellPointIds->GetNumberOfIds();
              this->CellScalars->SetNumberOfComponents(this->InScalars->GetNumberOfComponents());
              this->CellScalars->SetNumberOfTuples(numScalars);
              this->InScalars->GetTuples(this->NeighborCellPointIds,
                                         this->CellScalars);
              range[0] = VTK_DOUBLE_MAX; range[1] = -VTK_DOUBLE_MAX;
              for (ii=0; ii < numScalars;  ii++)
                {
                s = this->CellScalars->GetComponent(ii,0);
                if ( s < range[0] )
                  {
                  range[0] = s;
                  }
                if ( s > range[1] )
                  {
                  range[1] = s;
                  }
                }
              if ( range[1] >= this->ScalarRange[0] && 
                   range[0] <= this->ScalarRange[1] )
                {
                this->Wave2->InsertNextId(cellId);
                }
              }
            else
              {
              this->Wave2->InsertNextId(cellId);
              }
            }//for all cells using this point
          }//for all points of this cell
        }//if cell not yet visited
      }//for all cells in this wave

    tmpWave = this->Wave;
    this->Wave = this->Wave2;
    this->Wave2 = tmpWave;
    tmpWave->Reset();
    } //while wave is not empty

  return;
}

// Obtain the number of connected regions.
int vtkConnectivityFilter::GetNumberOfExtractedRegions()
{
  return this->RegionSizes->GetMaxId() + 1;
}

// Initialize list of point ids/cell ids used to seed regions.
void vtkConnectivityFilter::InitializeSeedList()
{
  this->Modified();
  this->Seeds->Reset();
}

// Add a seed id (point or cell id). Note: ids are 0-offset.
void vtkConnectivityFilter::AddSeed(vtkIdType id)
{
  this->Modified();
  this->Seeds->InsertNextId(id);
}

// Delete a seed id (point or cell id). Note: ids are 0-offset.
void vtkConnectivityFilter::DeleteSeed(vtkIdType id)
{
  this->Modified();
  this->Seeds->DeleteId(id);
}

// Initialize list of region ids to extract.
void vtkConnectivityFilter::InitializeSpecifiedRegionList()
{
  this->Modified();
  this->SpecifiedRegionIds->Reset();
}

// Add a region id to extract. Note: ids are 0-offset.
void vtkConnectivityFilter::AddSpecifiedRegion(int id)
{
  this->Modified();
  this->SpecifiedRegionIds->InsertNextId(id);
}

// Delete a region id to extract. Note: ids are 0-offset.
void vtkConnectivityFilter::DeleteSpecifiedRegion(int id)
{
  this->Modified();
  this->SpecifiedRegionIds->DeleteId(id);
}

int vtkConnectivityFilter::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

void vtkConnectivityFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Extraction Mode: ";
  os << this->GetExtractionModeAsString() << "\n";

  os << indent << "Closest Point: (" << this->ClosestPoint[0] << ", " 
     << this->ClosestPoint[1] << ", " << this->ClosestPoint[2] << ")\n";

  os << indent << "Color Regions: " << (this->ColorRegions ? "On\n" : "Off\n");

  os << indent << "Scalar Connectivity: " 
     << (this->ScalarConnectivity ? "On\n" : "Off\n");

  double *range = this->GetScalarRange();
  os << indent << "Scalar Range: (" << range[0] << ", " << range[1] << ")\n";
}

