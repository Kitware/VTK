/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyConnectivityFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkPolyConnectivityFilter.h"

// Description:
// Construct with default extraction mode to extract largest regions.
vtkPolyConnectivityFilter::vtkPolyConnectivityFilter()
{
  this->RegionSizes = new vtkIntArray;
  this->ExtractionMode = VTK_EXTRACT_LARGEST_REGION;
  this->ColorRegions = 0;
  this->MaxRecursionDepth = 10000;

  this->ScalarConnectivity = 0;
  this->ScalarRange[0] = 0.0;
  this->ScalarRange[1] = 1.0;
}
vtkPolyConnectivityFilter::~vtkPolyConnectivityFilter()
{
  this->RegionSizes->Delete();
}


static int NumExceededMaxDepth;
static int *Visited, *PointMap;
static vtkFloatScalars *NewScalars;
static int RecursionDepth;
static int RegionNumber, PointNumber;    
static int NumCellsInRegion;
static vtkIdList *RecursionSeeds;
static vtkScalars *InScalars;
static vtkPolyData *Mesh;

void vtkPolyConnectivityFilter::Execute()
{
  int cellId, i, j, pt;
  int numPts, numCells;
  vtkPoints *inPts;
  vtkFloatPoints *newPts;
  vtkIdList cellIds(VTK_CELL_SIZE), ptIds(VTK_CELL_SIZE);
  vtkPointData *pd;
  int id, npts, *pts, *cells, n;
  unsigned short ncells;
  int maxCellsInRegion;
  int largestRegionId = 0;
  vtkPolyData *input = this->GetInput();
  vtkPolyData *output = this->GetOutput();
  vtkPointData *outputPD = output->GetPointData();
  
  vtkDebugMacro(<<"Executing polygon connectivity filter.");
  //
  //  Check input/allocate storage
  //
  inPts = input->GetPoints();
  numPts = inPts->GetNumberOfPoints();
  numCells = input->GetNumberOfCells();

  if ( numPts < 1 || numCells < 1 )
    {
    vtkDebugMacro(<<"No data to connect!");
    return;
    }
  //
  // See whether to consider scalar connectivity
  //
  InScalars = input->GetPointData()->GetScalars();
  if ( !this->ScalarConnectivity ) 
    InScalars = NULL;
  else
    {
    if ( this->ScalarRange[1] < this->ScalarRange[0] ) 
      this->ScalarRange[1] = this->ScalarRange[0];
    }
  //
  // Build cell structure
  //
  Mesh = new vtkPolyData;
  Mesh->CopyStructure(input);
  Mesh->BuildLinks();

  //
  // Initialize.  Keep track of points and cells visited.
  //
  this->RegionSizes->Reset();
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

  if ( this->ExtractionMode != VTK_EXTRACT_POINT_SEEDED_REGIONS && 
  this->ExtractionMode != VTK_EXTRACT_CELL_SEEDED_REGIONS ) 
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

        this->RegionSizes->InsertValue(RegionNumber++,NumCellsInRegion);
        RecursionSeeds->Reset();
        }
      }
    }
  else // regions have been seeded, everything considered in same region
    {
    NumCellsInRegion = 0;

    if ( this->ExtractionMode == VTK_EXTRACT_POINT_SEEDED_REGIONS )
      {
      for (i=0; i < this->Seeds.GetNumberOfIds(); i++) 
        {
        pt = this->Seeds.GetId(i);
        if ( pt >= 0 ) 
          {
          Mesh->GetPointCells(pt,ncells,cells);
          for (j=0; j < ncells; j++) 
            RecursionSeeds->InsertNextId(cells[j]);
          }
        }
      }
    else if ( this->ExtractionMode == VTK_EXTRACT_CELL_SEEDED_REGIONS )
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
    this->RegionSizes->InsertValue(RegionNumber,NumCellsInRegion);
    }//else extracted seeded cells

  vtkDebugMacro (<<"Extracted " << RegionNumber << " region(s)");
  vtkDebugMacro (<<"Exceeded recursion depth " << NumExceededMaxDepth 
                 << " times");

  RecursionSeeds->Delete();
//
// Now that points and cells have been marked, traverse these lists pulling
// everything that has been visited.
//
  //Pass through point data that has been visited
  pd = input->GetPointData();
  if ( this->ColorRegions ) outputPD->CopyScalarsOff();
  outputPD->CopyAllocate(pd);

  for (i=0; i < numPts; i++)
    {
    if ( PointMap[i] > -1 )
      {
      newPts->InsertPoint(PointMap[i],inPts->GetPoint(i));
      outputPD->CopyData(pd,i,PointMap[i]);
      }
    }

  // if coloring regions; send down new scalar data
  if ( this->ColorRegions ) outputPD->SetScalars(NewScalars);
  NewScalars->Delete();

  output->SetPoints(newPts);
  newPts->Delete();
//
// Create output cells. Have to allocate storage first.
//
  if ( (n=input->GetVerts()->GetNumberOfCells()) > 0 )
    {
    vtkCellArray *newVerts = new vtkCellArray(n,n);
    output->SetVerts(newVerts);
    newVerts->Delete();
    }
  if ( (n=input->GetLines()->GetNumberOfCells()) > 0 )
    {
    vtkCellArray *newLines = new vtkCellArray(2*n,n);
    output->SetLines(newLines);
    newLines->Delete();
    }
  if ( (n=input->GetPolys()->GetNumberOfCells()) > 0 )
    {
    vtkCellArray *newPolys = new vtkCellArray(3*n,n);
    output->SetPolys(newPolys);
    newPolys->Delete();
    }
  if ( (n=input->GetStrips()->GetNumberOfCells()) > 0 )
    {
    vtkCellArray *newStrips = new vtkCellArray(5*n,n);
    output->SetStrips(newStrips);
    newStrips->Delete();
    }
  

  if ( this->ExtractionMode == VTK_EXTRACT_POINT_SEEDED_REGIONS ||
  this->ExtractionMode == VTK_EXTRACT_CELL_SEEDED_REGIONS )
    { // extract any cell that's been visited
    for (cellId=0; cellId < numCells; cellId++)
      {
      if ( Visited[cellId] >= 0 )
        {
        Mesh->GetCellPoints(cellId, npts, pts);
        ptIds.Reset ();
        for (i=0; i < npts; i++)
          {
          id = PointMap[pts[i]];
          ptIds.InsertId(i,id);
          }
        output->InsertNextCell(Mesh->GetCellType(cellId),ptIds);
        }
      }
    }
  else if ( this->ExtractionMode == VTK_EXTRACT_SPECIFIED_REGIONS )
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
          Mesh->GetCellPoints(cellId, npts, pts);
          ptIds.Reset ();
          for (i=0; i < npts; i++)
            {
            id = PointMap[pts[i]];
            ptIds.InsertId(i,id);
            }
          output->InsertNextCell(Mesh->GetCellType(cellId),ptIds);
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
        Mesh->GetCellPoints(cellId, npts, pts);
        ptIds.Reset ();
        for (i=0; i < npts; i++)
          {
          id = PointMap[pts[i]];
          ptIds.InsertId(i,id);
          }
        output->InsertNextCell(Mesh->GetCellType(cellId),ptIds);
        }
      }
   }

  delete [] Visited;
  delete [] PointMap;
  Mesh->Delete();
  output->Squeeze();

  vtkDebugMacro (<<"Extracted " << output->GetNumberOfCells() << " cells");

  return;
}

//
// Mark current cell as visited and assign region number.  Note:
// traversal occurs across shared vertices.
//
void vtkPolyConnectivityFilter::TraverseAndMark (int cellId)
{
  int j, k, ptId, npts, *pts, *cells;
  unsigned short ncells;

  Visited[cellId] = RegionNumber;
  NumCellsInRegion++;

  if ( RecursionDepth++ > this->MaxRecursionDepth ) 
    {
    RecursionSeeds->InsertNextId(cellId);
    NumExceededMaxDepth++;
    return;
    }

  Mesh->GetCellPoints(cellId, npts, pts);

  for (j=0; j < npts; j++) 
    {
    if ( PointMap[ptId=pts[j]] < 0 )
      {
      PointMap[ptId] = PointNumber++;
      NewScalars->SetScalar(PointMap[ptId], RegionNumber);
      }
     
    Mesh->GetPointCells(ptId,ncells,cells);

    // check connectivity criterion (geometric + scalar)
    for (k=0; k < ncells; k++)
      {
      cellId = cells[k];
      if ( Visited[cellId] < 0 )
        {
        if ( InScalars )
          {
          int numScalars, ii;
          float s, range[2];
          static vtkFloatScalars cellScalars(8,VTK_CELL_SIZE);
          static vtkIdList neiCellPointIds(8,VTK_CELL_SIZE);

          Mesh->GetCellPoints(cellId,neiCellPointIds);
          InScalars->GetScalars(neiCellPointIds,cellScalars);
          numScalars = cellScalars.GetNumberOfScalars();
          range[0] = VTK_LARGE_FLOAT; range[1] = -VTK_LARGE_FLOAT;
          for (ii=0; ii < numScalars;  ii++)
            {
            s = cellScalars.GetScalar(ii);
            if ( s < range[0] ) range[0] = s;
            if ( s > range[1] ) range[1] = s;
            }
          if ( range[1] >= this->ScalarRange[0] && 
          range[0] <= this->ScalarRange[1] )
            {
            TraverseAndMark (cellId);
            }
          }
        else
          {
          TraverseAndMark (cellId);
          }
        }
      }
    } // for all cells of this element

  RecursionDepth--;
  return;
}

// Description:
// Obtain the number of connected regions.
int vtkPolyConnectivityFilter::GetNumberOfExtractedRegions()
{
  return this->RegionSizes->GetMaxId() + 1;
}

// Description:
// Set the extraction mode to extract regions sharing specified point ids.
void vtkPolyConnectivityFilter::ExtractPointSeededRegions()
{
  if ( this->ExtractionMode != VTK_EXTRACT_POINT_SEEDED_REGIONS )
    {
    this->Modified();
    this->ExtractionMode = VTK_EXTRACT_POINT_SEEDED_REGIONS;
    }
}

// Description:
// Set the extraction mode to extract regions sharing specified cell ids.
void vtkPolyConnectivityFilter::ExtractCellSeededRegions()
{
  if ( this->ExtractionMode != VTK_EXTRACT_CELL_SEEDED_REGIONS )
    {
    this->Modified();
    this->ExtractionMode = VTK_EXTRACT_CELL_SEEDED_REGIONS;
    }
}

// Description:
// Set the extraction mode to extract regions of specified id. You may 
// have to execute filter first (with debug turned on) to determine region ids.
void vtkPolyConnectivityFilter::ExtractSpecifiedRegions()
{
  if ( this->ExtractionMode != VTK_EXTRACT_SPECIFIED_REGIONS )
    {
    this->Modified();
    this->ExtractionMode = VTK_EXTRACT_SPECIFIED_REGIONS;
    }
}

// Description:
// Set the extraction mode to extract the largest region found.
void vtkPolyConnectivityFilter::ExtractLargestRegion()
{
  if ( this->ExtractionMode != VTK_EXTRACT_LARGEST_REGION )
    {
    this->Modified();
    this->ExtractionMode = VTK_EXTRACT_LARGEST_REGION;
    }
}

// Description:
// Initialize list of point ids/cell ids used to seed regions.
void vtkPolyConnectivityFilter::InitializeSeedList()
{
  this->Modified();
  this->Seeds.Reset();
}

// Description:
// Add a seed id (point or cell id). Note: ids are 0-offset.
void vtkPolyConnectivityFilter::AddSeed(int id)
{
  this->Modified();
  this->Seeds.InsertNextId(id);
}

// Description:
// Delete a seed id (point or cell id). Note: ids are 0-offset.
void vtkPolyConnectivityFilter::DeleteSeed(int id)
{
  this->Modified();
  this->Seeds.DeleteId(id);
}

// Description:
// Initialize list of region ids to extract.
void vtkPolyConnectivityFilter::InitializeSpecifiedRegionList()
{
  this->Modified();
  this->SpecifiedRegionIds.Reset();
}

// Description:
// Add a region id to extract. Note: ids are 0-offset.
void vtkPolyConnectivityFilter::AddSpecifiedRegion(int id)
{
  this->Modified();
  this->SpecifiedRegionIds.InsertNextId(id);
}

// Description:
// Delete a region id to extract. Note: ids are 0-offset.
void vtkPolyConnectivityFilter::DeleteSpecifiedRegion(int id)
{
  this->Modified();
  this->SpecifiedRegionIds.DeleteId(id);
}

void vtkPolyConnectivityFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyToPolyFilter::PrintSelf(os,indent);

  os << indent << "Extraction Mode: ";
  switch (this->ExtractionMode)
    {
    case VTK_EXTRACT_POINT_SEEDED_REGIONS:
      os << "(Extract point seeded regions)\n";
      break;
    case VTK_EXTRACT_CELL_SEEDED_REGIONS:
      os << "(Extract cell seeded regions)\n";
      break;
    case VTK_EXTRACT_SPECIFIED_REGIONS:
      os << "(Extract specified regions)\n";
      break;
    case VTK_EXTRACT_LARGEST_REGION:
      os << "(Extract largest region)\n";
      break;
    }

  os << indent << "Color Regions: " << (this->ColorRegions ? "On\n" : "Off\n");
  os << indent << "Maximum Recursion Depth: " << this->MaxRecursionDepth << "\n";

  os << indent << "Scalar Connectivity: " 
     << (this->ScalarConnectivity ? "On\n" : "Off\n");

  float *range = this->GetScalarRange();
  os << indent << "Scalar Range: (" << range[0] << ", " << range[1] << ")\n";
}

