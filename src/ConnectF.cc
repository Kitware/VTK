/*=========================================================================

  Program:   Visualization Library
  Module:    ConnectF.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "ConnectF.hh"

// Description:
// Construct with default extraction mode to extract largest regions.
vlConnectivityFilter::vlConnectivityFilter()
{
  this->ExtractionMode = EXTRACT_LARGEST_REGIONS;
  this->ColorRegions = 0;
  this->NumberOfRegionsToExtract = 1;
  this->MaxRecursionDepth = 10000;
}

static NumExceededMaxDepth;
static int *Visited, *PointMap;
static vlFloatScalars *NewScalars;
static int RecursionDepth;
static int RegionNumber, PointNumber;    
static int NumCellsInRegion;
static  vlIdList *RecursionSeeds;

void vlConnectivityFilter::Execute()
{
  int cellId, i, j, pt;
  int numPts, numCells;
  vlFloatPoints *newPts;
  vlIdList cellIds(MAX_CELL_SIZE), ptIds(MAX_CELL_SIZE);
  vlPointData *pd;
  int id;

  vlDebugMacro(<<"Executing connectivity filter.");
  this->Initialize();
//
//  Check input/allocate storage
//
  if ( (numPts=this->Input->GetNumberOfPoints()) < 1 ||
  (numCells=this->Input->GetNumberOfCells()) < 1 )
    {
    vlDebugMacro(<<"No data to connect!");
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

  NewScalars = new vlFloatScalars(numPts);
  newPts = new vlFloatPoints(numPts);
//
// Traverse all cells marking those visited.  Each new search
// starts a new connected region.  Note: have to truncate recursion
// and keep track of seeds to start up again.
//
  RecursionSeeds = new vlIdList(1000,10000);

  NumExceededMaxDepth = 0;
  RegionNumber = 0;

  if ( this->ExtractionMode != EXTRACT_POINT_SEEDED_REGIONS && 
  this->ExtractionMode != EXTRACT_CELL_SEEDED_REGIONS ) 
    { //visit all cells marking with region number
    for (cellId=0; cellId < numCells; cellId++, RegionNumber++)
      {
      if ( Visited[cellId] < 0 ) 
        {
        NumCellsInRegion = 0;
        RegionNumber += 1;
        RecursionDepth = 0;
        this->TraverseAndMark (cellId);
        }

      for (i=0; i < RecursionSeeds->GetNumberOfIds(); i++) 
        {
        RecursionDepth = 0;
        this->TraverseAndMark (RecursionSeeds->GetId(i));
        }

      this->RegionSizes.InsertValue(RegionNumber,NumCellsInRegion);
      RecursionSeeds->Reset();
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

  vlDebugMacro (<<"Extracted " << RegionNumber << " region(s)\n");
  vlDebugMacro (<<"Exceeded recursion depth " << NumExceededMaxDepth 
                << " times\n");

  delete RecursionSeeds;
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
  delete NewScalars;

  this->PointData.Squeeze();
  newPts->Squeeze();
  this->SetPoints(newPts);

//
// Create output cells
//
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

  delete [] Visited;
  delete [] PointMap;

  return;
}

//
// Mark current cell as visited and assign region number.  Note:
// traversal occurs across shared vertices.
//
void vlConnectivityFilter::TraverseAndMark (int cellId)
{
  int j, k, ptId;
  vlIdList ptIds, cellIds;

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
int vlConnectivityFilter::GetNumberOfExtractedRegions()
{
  return this->RegionSizes.GetSize();
}

// Description:
// Set the extraction mode to extract regions sharing specified point ids.
void vlConnectivityFilter::ExtractPointSeededRegions()
{
  if ( this->ExtractionMode != EXTRACT_POINT_SEEDED_REGIONS )
    {
    this->Modified();
    this->ExtractionMode = EXTRACT_POINT_SEEDED_REGIONS;
    }
}

// Description:
// Set the extraction mode to extract regions sharing specified cell ids.
void vlConnectivityFilter::ExtractCellSeededRegions()
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
void vlConnectivityFilter::ExtractSpecifiedRegions()
{
  if ( this->ExtractionMode != EXTRACT_SPECIFIED_REGIONS )
    {
    this->Modified();
    this->ExtractionMode = EXTRACT_SPECIFIED_REGIONS;
    }
}

// Description:
// Set the extraction mode to extract the largest region found.
void vlConnectivityFilter::ExtractLargestRegions(int numRegions)
{
  if ( this->ExtractionMode != EXTRACT_LARGEST_REGIONS 
  || numRegions != this->NumberOfRegionsToExtract)
    {
    this->Modified();
    this->ExtractionMode = EXTRACT_LARGEST_REGIONS;
    this->NumberOfRegionsToExtract = numRegions;
    }
}

// Description:
// Initialize list of point ids/cell ids used to seed regions.
void vlConnectivityFilter::InitializeSeedList()
{
  this->Modified();
  this->Seeds.Reset();
}

// Description:
// Add a seed id (point or cell id).
void vlConnectivityFilter::AddSeed(int id)
{
  this->Modified();
  this->Seeds.InsertNextId(id);
}

// Description:
// Delete a seed id (point or cell id).
void vlConnectivityFilter::DeleteSeed(int id)
{
  this->Modified();
  this->Seeds.DeleteId(id);
}

// Description:
// Initialize list of region ids to extract.
void vlConnectivityFilter::InitializeSpecifiedRegionList()
{
  this->Modified();
  this->SpecifiedRegionIds.Reset();
}

// Description:
// Add a region id to extract.
void vlConnectivityFilter::AddSpecifiedRegion(int id)
{
  this->Modified();
  this->SpecifiedRegionIds.InsertNextId(id);
}

// Description:
// Delete a region id to extract.
void vlConnectivityFilter::DeleteSpecifiedRegion(int id)
{
  this->Modified();
  this->SpecifiedRegionIds.DeleteId(id);
}

void vlConnectivityFilter::PrintSelf(ostream& os, vlIndent indent)
{
  vlDataSetToUnstructuredGridFilter::PrintSelf(os,indent);

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
    case EXTRACT_LARGEST_REGIONS:
      os << "(Extract " << this->NumberOfRegionsToExtract << " largest regions)\n";
      break;

    os << indent << "Color Regions: " << (this->ColorRegions ? "On\n" : "Off\n");
    os << indent << "Maximum Recursion Depth: " << this->MaxRecursionDepth << "\n";
    }
}

