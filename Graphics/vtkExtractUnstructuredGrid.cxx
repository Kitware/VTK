/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractUnstructuredGrid.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractUnstructuredGrid.h"
#include "vtkMergePoints.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkExtractUnstructuredGrid, "1.28");
vtkStandardNewMacro(vtkExtractUnstructuredGrid);

// Construct with all types of clipping turned off.
vtkExtractUnstructuredGrid::vtkExtractUnstructuredGrid()
{
  this->PointMinimum = 0;
  this->PointMaximum = VTK_LARGE_ID;

  this->CellMinimum = 0;
  this->CellMaximum = VTK_LARGE_ID;

  this->Extent[0] = -VTK_LARGE_FLOAT;
  this->Extent[1] = VTK_LARGE_FLOAT;
  this->Extent[2] = -VTK_LARGE_FLOAT;
  this->Extent[3] = VTK_LARGE_FLOAT;
  this->Extent[4] = -VTK_LARGE_FLOAT;
  this->Extent[5] = VTK_LARGE_FLOAT;

  this->PointClipping = 0;
  this->CellClipping = 0;
  this->ExtentClipping = 0;

  this->Merging = 0;
  this->Locator = NULL;
}

// Specify a (xmin,xmax, ymin,ymax, zmin,zmax) bounding box to clip data.
void vtkExtractUnstructuredGrid::SetExtent(float xMin,float xMax, float yMin,
                                           float yMax, float zMin, float zMax)
{
  float extent[6];

  extent[0] = xMin;
  extent[1] = xMax;
  extent[2] = yMin;
  extent[3] = yMax;
  extent[4] = zMin;
  extent[5] = zMax;

  this->SetExtent(extent);
}

// Specify a (xmin,xmax, ymin,ymax, zmin,zmax) bounding box to clip data.
void vtkExtractUnstructuredGrid::SetExtent(float extent[6])
{
  int i;

  if ( extent[0] != this->Extent[0] || extent[1] != this->Extent[1] ||
       extent[2] != this->Extent[2] || extent[3] != this->Extent[3] ||
       extent[4] != this->Extent[4] || extent[5] != this->Extent[5] )
    {
    this->Modified();
    for (i=0; i<3; i++)
      {
      if ( extent[2*i+1] < extent[2*i] )
        {
        extent[2*i+1] = extent[2*i];
        }
      this->Extent[2*i] = extent[2*i];
      this->Extent[2*i+1] = extent[2*i+1];
      }
    }
}

// Extract cells and pass points and point data through. Also handles
// cell data.
void vtkExtractUnstructuredGrid::Execute()
{
  vtkIdType cellId, i, newCellId;
  vtkIdType newPtId;
  vtkUnstructuredGrid *input=this->GetInput();
  vtkIdType numPts=input->GetNumberOfPoints();
  vtkIdType numCells=input->GetNumberOfCells();
  vtkPoints *inPts=input->GetPoints(), *newPts;
  char *cellVis;
  vtkCell *cell;
  float *x;
  vtkIdList *ptIds;
  vtkIdList *cellIds;
  vtkIdType ptId;
  vtkPointData *pd = input->GetPointData();
  vtkCellData *cd = input->GetCellData();
  int allVisible, numIds;
  vtkUnstructuredGrid *output = this->GetOutput();
  vtkPointData *outputPD = output->GetPointData();
  vtkCellData *outputCD = output->GetCellData();
  vtkIdType *pointMap = NULL;
  
  vtkDebugMacro(<<"Executing geometry filter");

  if ( numPts < 1 || numCells < 1 || !inPts )
    {
    vtkDebugMacro(<<"No data to extract!");
    return;
    }
  cellIds=vtkIdList::New();

  if ( (!this->CellClipping) && (!this->PointClipping) && 
  (!this->ExtentClipping) )
    {
    allVisible = 1;
    cellVis = NULL;
    }
  else
    {
    allVisible = 0;
    cellVis = new char[numCells];
    }

  // Mark cells as being visible or not
  if ( ! allVisible )
    {
    for(cellId=0; cellId < numCells; cellId++)
      {
      if ( this->CellClipping && cellId < this->CellMinimum ||
      cellId > this->CellMaximum )
        {
        cellVis[cellId] = 0;
        }
      else
        {
        cell = input->GetCell(cellId);
        ptIds = cell->GetPointIds();
        numIds = ptIds->GetNumberOfIds();
        for (i=0; i < numIds; i++) 
          {
          ptId = ptIds->GetId(i);
          x = input->GetPoint(ptId);

          if ( (this->PointClipping && (ptId < this->PointMinimum ||
          ptId > this->PointMaximum) ) ||
          (this->ExtentClipping && 
          (x[0] < this->Extent[0] || x[0] > this->Extent[1] ||
          x[1] < this->Extent[2] || x[1] > this->Extent[3] ||
          x[2] < this->Extent[4] || x[2] > this->Extent[5] )) )
            {
            cellVis[cellId] = 0;
            break;
            }
          }
        if ( i >= numIds )
          {
          cellVis[cellId] = 1;
          }
        }
      }
    }

  // Allocate
  newPts = vtkPoints::New();
  newPts->Allocate(numPts);
  output->Allocate(numCells);
  outputPD->CopyAllocate(pd,numPts,numPts/2);
  outputCD->CopyAllocate(cd,numCells,numCells/2);

  if ( this->Merging )
    {
    if ( this->Locator == NULL )
      {
      this->CreateDefaultLocator();
      }
    this->Locator->InitPointInsertion (newPts, input->GetBounds());
    }
  else
    {
    pointMap = new vtkIdType[numPts];
    for (i=0; i<numPts; i++)
      {
      pointMap[i] = (-1); //initialize as unused
      }
    }

  // Traverse cells to extract geometry
  for(cellId=0; cellId < numCells; cellId++)
    {
    if ( allVisible || cellVis[cellId] )
      {
      cell = input->GetCell(cellId);
      numIds = cell->PointIds->GetNumberOfIds();
      cellIds->Reset();
      if ( this->Merging )
        {
        for (i=0; i < numIds; i++)
          {
          ptId = cell->PointIds->GetId(i);
          x = input->GetPoint(ptId);
          if ( this->Locator->InsertUniquePoint(x, newPtId) )
            {
            outputPD->CopyData(pd,ptId,newPtId);
            }
          cellIds->InsertNextId(newPtId);
          }
        }//merging coincident points
      else
        {
        for (i=0; i < numIds; i++)
          {
          ptId = cell->PointIds->GetId(i);
          if ( pointMap[ptId] < 0 )
            {
            pointMap[ptId] = newPtId 
              = newPts->InsertNextPoint(inPts->GetPoint(ptId));
            outputPD->CopyData(pd, ptId, newPtId);
            }
          cellIds->InsertNextId(pointMap[ptId]);
          }
        }//keeping original point list

      newCellId = output->InsertNextCell(input->GetCellType(cellId), cellIds);
      outputCD->CopyData(cd, cellId, newCellId);
        
      } //if cell is visible
    } //for all cells

  vtkDebugMacro(<<"Extracted " << output->GetNumberOfPoints() << " points,"
                << output->GetNumberOfCells() << " cells.");

  // Update ourselves and release memory
  output->SetPoints(newPts);
  newPts->Delete();

  if ( this->Merging && this->Locator )
    {
    this->Locator->Initialize(); 
    }
  else
    {
    delete [] pointMap;
    }
  output->Squeeze();
  
  if ( cellVis )
    {
    delete [] cellVis;
    }
  cellIds->Delete();
}

unsigned long int vtkExtractUnstructuredGrid::GetMTime()
{
  unsigned long mTime=
    this->vtkUnstructuredGridToUnstructuredGridFilter::GetMTime();
  unsigned long time;

  if ( this->Locator != NULL )
    {
    time = this->Locator->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }
  return mTime;
}

void vtkExtractUnstructuredGrid::CreateDefaultLocator()
{
  if ( this->Locator == NULL )
    {
    this->Locator = vtkMergePoints::New();
    }
}

// Specify a spatial locator for merging points. By
// default an instance of vtkMergePoints is used.
void vtkExtractUnstructuredGrid::SetLocator(vtkPointLocator *locator)
{
  if ( this->Locator == locator ) 
    {
    return;
    }
  if ( this->Locator )
    {
    this->Locator->UnRegister(this);
    this->Locator = NULL;
    }    
  if ( locator )
    {
    locator->Register(this);
    }
  this->Locator = locator;
  this->Modified();
}

void vtkExtractUnstructuredGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Point Minimum : " << this->PointMinimum << "\n";
  os << indent << "Point Maximum : " << this->PointMaximum << "\n";

  os << indent << "Cell Minimum : " << this->CellMinimum << "\n";
  os << indent << "Cell Maximum : " << this->CellMaximum << "\n";

  os << indent << "Extent: \n";
  os << indent << "  Xmin,Xmax: (" << this->Extent[0] << ", " << this->Extent[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << this->Extent[2] << ", " << this->Extent[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << this->Extent[4] << ", " << this->Extent[5] << ")\n";

  os << indent << "PointClipping: " << (this->PointClipping ? "On\n" : "Off\n");
  os << indent << "CellClipping: " << (this->CellClipping ? "On\n" : "Off\n");
  os << indent << "ExtentClipping: " << (this->ExtentClipping ? "On\n" : "Off\n");

  os << indent << "Merging: " << (this->Merging ? "On\n" : "Off\n");
  if ( this->Locator )
    {
    os << indent << "Locator: " << this->Locator << "\n";
    }
  else
    {
    os << indent << "Locator: (none)\n";
    }
}
