/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkConnectivityFilter.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkConnectivityFilter.hh"

// Description:
// Construct with default extraction mode to extract largest regions.
vtkConnectivityFilter::vtkConnectivityFilter()
{
  this->ExtractionMode = EXTRACT_LARGEST_REGION;
  this->ColorRegions = 0;
  this->MaxRecursionDepth = 10000;
}

static NumExceededMaxDepth;
static int *Visited, *PointMap;
static vtkFloatScalars *NewScalars;
static int RecursionDepth;
static int RegionNumber, PointNumber;    
static int NumCellsInRegion;
static  vtkIdList *RecursionSeeds;

void vtkConnectivityFilter::Execute()
{
  int cellId, i, j, pt;
  int numPts, numCells;
  vtkFloatPoints *newPts;
  vtkIdList cellIds(MAX_CELL_SIZE), ptIds(MAX_CELL_SIZE);
  vtkPointData *pd;
  int id;
  int maxCellsInRegion, largestRegionId;
   
  vtkDebugMacro(<<"Executing connectivity filter.");
  this->Initialize();
//
//  Check input/allocate storage
//
  if ( (numPts=this->Input->GetNumberOfPoints()) < 1 ||
  (numCells=this->Input->GetNumberOfCells()) < 1 )
    {
    vtkDebugMacro(<<"No data to connect!");
    return;
    }
  this->Allocate(numCells,numCells);
//
// Initialize.  Keep track of points and cells visited.
//
  this->RegionSizes.Reset();
  Visited = new int[numCells];
  for ( i=0; i < numCells; i++ ) Visited[i] = -1;
  PointMap = new int[numPts];  
  for ( i=0; i < numPts; i++ ) PointMap[i] = -1;

  NewScalars = new vtkFloatScalars(numPts);
  newPts = new vtkFloatPoints(numPts);
//
// Traverse all cells marking those visited.  Each new search
// starts a new connected region.  Note: have to truncate recursion
// and keep track of seeds to start up again.
//
  RecursionSeeds = new vtkIdList(1000,10000);

  NumExceededMaxDepth = 0;
  PointNumber = 0;
  RegionNumber = 0;
  maxCellsInRegion = 0;

  if ( this->ExtractionMode != EXTRACT_POINT_SEEDED_REGIONS && 
  this->ExtractionMode != EXTRACT_CELL_SEEDED_REGIONS ) 
    { //visit all cells marking with region number
    for (cellId=0; cellId < numCells; cellId++)
      {
      if ( Visited[cellId] < 0 ) 
        {
        NumCellsInRegion = 0;
        RecursionDepth = 0;
        this->TraverseAndMark (cellId);

        for (i=0; i < RecursionSeeds->GetNumberOfIds(); i++) 
          {
          RecursionDepth = 0;
          this->TraverseAndMark (RecursionSeeds->GetId(i));
          }

        if ( NumCellsInRegion > maxCellsInRegion )
          {
          maxCellsInRegion = NumCellsInRegion;
          largestRegionId = RegionNumber;
          }

        this->RegionSizes.InsertValue(RegionNumber++,NumCellsInRegion);
        RecursionSeeds->Reset();
        }
      }
    }
  else // regions have been seeded, everything considered in same region
    {
    NumCellsInRegion = 0;

    if ( this->ExtractionMode == EXTRACT_POINT_SEEDED_REGIONS )
      {
      for (i=0; i < this->Seeds.GetNumberOfIds(); i++) 
        {
        pt = this->Seeds.GetId(i);
        if ( pt >= 0 ) 
          {
          this->Input->GetPointCells(pt,cellIds);
          for (j=0; j < cellIds.GetNumberOfIds(); j++) 
            RecursionSeeds->InsertNextId(cellIds.GetId(j));
          }
        }
      }
    else if ( this->ExtractionMode == EXTRACT_CELL_SEEDED_REGIONS )
      {
      for (i=0; i < this->Seeds.GetNumberOfIds(); i++) 
        {
        cellId = this->Seeds.GetId(i);
        if ( cellId >= 0 ) RecursionSeeds->InsertNextId(cellId);
        }
      }

    //mark all seeded regions
    for (i=0; i < RecursionSeeds->GetNumberOfIds(); i++) 
      {
      RecursionDepth = 0;
      this->TraverseAndMark (RecursionSeeds->GetId(i));
      }
    this->RegionSizes.InsertValue(RegionNumber,NumCellsInRegion);
    }

  vtkDebugMacro (<<"Extracted " << RegionNumber << " region(s)\n");
  vtkDebugMacro (<<"Exceeded recursion depth " << NumExceededMaxDepth 
                << " times\n");

  RecursionSeeds->Delete();
//
// Now that points and cells have been marked, traverse these lists pulling
// everything that has been visited.
//
  //Pass through point data that has been visited
  pd = this->Input->GetPointData();
  if ( this->ColorRegions ) this->PointData.CopyScalarsOff();
  this->PointData.CopyAllocate(pd);

  for (i=0; i < numPts; i++)
    {
    if ( PointMap[i] > -1 )
      {
      newPts->SetPoint(PointMap[i],this->Input->GetPoint(i));
      this->PointData.CopyData(pd,i,PointMap[i]);
      }
    }

  // if coloring regions; send down new scalar data
  if ( this->ColorRegions ) this->PointData.SetScalars(NewScalars);
  NewScalars->Delete();

  this->SetPoints(newPts);
  newPts->Delete();
//
// Create output cells
//
  if ( this->ExtractionMode == EXTRACT_POINT_SEEDED_REGIONS ||
  this->ExtractionMode == EXTRACT_CELL_SEEDED_REGIONS )
    { // extract any cell that's been visited
    for (cellId=0; cellId < numCells; cellId++)
      {
      if ( Visited[cellId] >= 0 )
        {
        this->Input->GetCellPoints(cellId, ptIds);
        for (i=0; i < ptIds.GetNumberOfIds(); i++)
          {
          id = PointMap[ptIds.GetId(i)];
          ptIds.SetId(i,id);
          }
        this->InsertNextCell(this->Input->GetCellType(cellId),ptIds);
        }
      }
    }
  else if ( this->ExtractionMode == EXTRACT_SPECIFIED_REGIONS )
    {
    for (cellId=0; cellId < numCells; cellId++)
      {
      int inReg, regionId;
      if ( (regionId=Visited[cellId]) >= 0 )
        {
        for (inReg=0,i=0; i<this->SpecifiedRegionIds.GetNumberOfIds(); i++)
          {
          if ( regionId == this->SpecifiedRegionIds.GetId(i) )
            {
            inReg = 1;
            break;
            }
          }
        if ( inReg )
          {
          this->Input->GetCellPoints(cellId, ptIds);
          for (i=0; i < ptIds.GetNumberOfIds(); i++)
            {
            id = PointMap[ptIds.GetId(i)];
            ptIds.SetId(i,id);
            }
          this->InsertNextCell(this->Input->GetCellType(cellId),ptIds);
          }
        }
      }
    }
  else //extract largest region
    {
    for (cellId=0; cellId < numCells; cellId++)
      {
      if ( Visited[cellId] == largestRegionId )
        {
        this->Input->GetCellPoints(cellId, ptIds);
        for (i=0; i < ptIds.GetNumberOfIds(); i++)
          {
          id = PointMap[ptIds.GetId(i)];
          ptIds.SetId(i,id);
          }
        this->InsertNextCell(this->Input->GetCellType(cellId),ptIds);
        }
      }
   }

  delete [] Visited;
  delete [] PointMap;

  return;
}

//
// Mark current cell as visited and assign region number.  Note:
// traversal occurs across shared vertices.
//
void vtkConnectivityFilter::TraverseAndMark (int cellId)
{
  int j, k, ptId;
  vtkIdList ptIds, cellIds;

  Visited[cellId] = RegionNumber;
  NumCellsInRegion++;

  if ( RecursionDepth++ > this->MaxRecursionDepth ) 
    {
    RecursionSeeds->InsertNextId(cellId);
    NumExceededMaxDepth++;
    return;
    }

  this->Input->GetCellPoints(cellId, ptIds);

  for (j=0; j < ptIds.GetNumberOfIds(); j++) 
    {
    if ( PointMap[ptId=ptIds.GetId(j)] < 0 )
      {
      PointMap[ptId] = PointNumber++;
      NewScalars->SetScalar(PointMap[ptId], RegionNumber);
      }
     
    this->Input->GetPointCells(ptId,cellIds);

    for (k=0; k < cellIds.GetNumberOfIds(); k++)
      if ( Visited[cellIds.GetId(k)] < 0 )
         TraverseAndMark (cellIds.GetId(k));

    } // for all cells of this element

  RecursionDepth--;
  return;
}

// Description:
// Obtain the number of connected regions.
int vtkConnectivityFilter::GetNumberOfExtractedRegions()
{
  return this->RegionSizes.GetSize();
}

// Description:
// Set the extraction mode to extract regions sharing specified point ids.
void vtkConnectivityFilter::ExtractPointSeededRegions()
{
  if ( this->ExtractionMode != EXTRACT_POINT_SEEDED_REGIONS )
    {
    this->Modified();
    this->ExtractionMode = EXTRACT_POINT_SEEDED_REGIONS;
    }
}

// Description:
// Set the extraction mode to extract regions sharing specified cell ids.
void vtkConnectivityFilter::ExtractCellSeededRegions()
{
  if ( this->ExtractionMode != EXTRACT_CELL_SEEDED_REGIONS )
    {
    this->Modified();
    this->ExtractionMode = EXTRACT_CELL_SEEDED_REGIONS;
    }
}

// Description:
// Set the extraction mode to extract regions of specified id. You may 
// have to execute filter first to determine region ids.
void vtkConnectivityFilter::ExtractSpecifiedRegions()
{
  if ( this->ExtractionMode != EXTRACT_SPECIFIED_REGIONS )
    {
    this->Modified();
    this->ExtractionMode = EXTRACT_SPECIFIED_REGIONS;
    }
}

// Description:
// Set the extraction mode to extract the largest region found.
void vtkConnectivityFilter::ExtractLargestRegion()
{
  if ( this->ExtractionMode != EXTRACT_LARGEST_REGION )
    {
    this->Modified();
    this->ExtractionMode = EXTRACT_LARGEST_REGION;
    }
}

// Description:
// Initialize list of point ids/cell ids used to seed regions.
void vtkConnectivityFilter::InitializeSeedList()
{
  this->Modified();
  this->Seeds.Reset();
}

// Description:
// Add a seed id (point or cell id). Note: ids are 0-offset.
void vtkConnectivityFilter::AddSeed(int id)
{
  this->Modified();
  this->Seeds.InsertNextId(id);
}

// Description:
// Delete a seed id (point or cell id). Note: ids are 0-offset.
void vtkConnectivityFilter::DeleteSeed(int id)
{
  this->Modified();
  this->Seeds.DeleteId(id);
}

// Description:
// Initialize list of region ids to extract.
void vtkConnectivityFilter::InitializeSpecifiedRegionList()
{
  this->Modified();
  this->SpecifiedRegionIds.Reset();
}

// Description:
// Add a region id to extract. Note: ids are 0-offset.
void vtkConnectivityFilter::AddSpecifiedRegion(int id)
{
  this->Modified();
  this->SpecifiedRegionIds.InsertNextId(id);
}

// Description:
// Delete a region id to extract. Note: ids are 0-offset.
void vtkConnectivityFilter::DeleteSpecifiedRegion(int id)
{
  this->Modified();
  this->SpecifiedRegionIds.DeleteId(id);
}

void vtkConnectivityFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToUnstructuredGridFilter::PrintSelf(os,indent);

  os << indent << "Extraction Mode: ";
  switch (this->ExtractionMode)
    {
    case EXTRACT_POINT_SEEDED_REGIONS:
      os << "(Extract point seeded regions)\n";
      break;
    case EXTRACT_CELL_SEEDED_REGIONS:
      os << "(Extract cell seeded regions)\n";
      break;
    case EXTRACT_SPECIFIED_REGIONS:
      os << "(Extract specified regions)\n";
      break;
    case EXTRACT_LARGEST_REGION:
      os << "(Extract largest region)\n";
      break;

    os << indent << "Color Regions: " << (this->ColorRegions ? "On\n" : "Off\n");
    os << indent << "Maximum Recursion Depth: " << this->MaxRecursionDepth << "\n";
    }
}

