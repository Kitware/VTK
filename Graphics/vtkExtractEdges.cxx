/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractEdges.cxx
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
#include "vtkExtractEdges.h"
#include "vtkEdgeTable.h"
#include "vtkMergePoints.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkExtractEdges, "1.41");
vtkStandardNewMacro(vtkExtractEdges);

// Construct object.
vtkExtractEdges::vtkExtractEdges()
{
  this->Locator = NULL;
}

vtkExtractEdges::~vtkExtractEdges()
{
  if ( this->Locator )
    {
    this->Locator->UnRegister(this);
    this->Locator = NULL;
    }
}

// Generate feature edges for mesh
void vtkExtractEdges::Execute()
{
  vtkDataSet *input= this->GetInput();
  vtkPolyData *output= this->GetOutput();
  vtkPoints *newPts;
  vtkCellArray *newLines;
  vtkIdType numCells, cellNum, numPts, newId;
  int edgeNum, numEdgePts, numCellEdges;
  int i, abort = 0;
  vtkIdType pts[2];
  vtkIdType pt1 = 0, pt2;
  float *x;
  vtkEdgeTable *edgeTable;
  vtkGenericCell *cell;
  vtkCell *edge;
  vtkPointData *pd, *outPD;
  vtkCellData *cd, *outCD;

  vtkDebugMacro(<<"Executing edge extractor");

  //  Check input
  //
  numPts=input->GetNumberOfPoints();
  if ( (numCells=input->GetNumberOfCells()) < 1 || numPts < 1 )
    {
    vtkErrorMacro(<<"No input data!");
    return;
    }

  // Set up processing
  //
  edgeTable = vtkEdgeTable::New();
  edgeTable->InitEdgeInsertion(numPts);
  newPts = vtkPoints::New();
  newPts->Allocate(numPts);
  newLines = vtkCellArray::New();
  newLines->EstimateSize(numPts*4,2);

  pd = input->GetPointData();
  outPD = output->GetPointData();
  outPD->CopyAllocate(pd,numPts);

  cd = input->GetCellData();
  outCD = output->GetCellData();
  outCD->CopyAllocate(cd,numCells);
  
  cell = vtkGenericCell::New();
  
  // Get our locator for merging points
  //
  if ( this->Locator == NULL )
    {
    this->CreateDefaultLocator();
    }
  this->Locator->InitPointInsertion (newPts, input->GetBounds());

  // Loop over all cells, extracting non-visited edges. 
  //
  int tenth = numCells/10 + 1;
  for (cellNum=0; cellNum < numCells && !abort; cellNum++ )
    {
    if ( ! (cellNum % tenth) ) //manage progress reports / early abort
      {
      this->UpdateProgress ((float)cellNum / numCells);
      abort = this->GetAbortExecute();
      }

    input->GetCell(cellNum,cell);
    numCellEdges = cell->GetNumberOfEdges();
    for (edgeNum=0; edgeNum < numCellEdges; edgeNum++ )
      {
      edge = cell->GetEdge(edgeNum);
      numEdgePts = edge->GetNumberOfPoints();
      
      for ( i=0; i < numEdgePts; i++, pt1=pt2, pts[0]=pts[1] )
        {
        pt2 = edge->PointIds->GetId(i);
        x = input->GetPoint(pt2);
        if ( this->Locator->InsertUniquePoint(x, pts[1]) )
          {
          outPD->CopyData (pd,pt2,pts[1]);
          }
        if ( i > 0 && edgeTable->IsEdge(pt1,pt2) == -1 )
          {
          edgeTable->InsertEdge(pt1, pt2);
          newId = newLines->InsertNextCell(2,pts);
          outCD->CopyData(cd, cellNum, newId);
          }
        }
      }//for all edges of cell
    }//for all cells

  vtkDebugMacro(<<"Created " << newLines->GetNumberOfCells() << " edges");

  //  Update ourselves.
  //
  edgeTable->Delete();
  cell->Delete();

  output->SetPoints(newPts);
  newPts->Delete();

  output->SetLines(newLines);
  newLines->Delete();

  output->Squeeze();
}

// Specify a spatial locator for merging points. By
// default an instance of vtkMergePoints is used.
void vtkExtractEdges::SetLocator(vtkPointLocator *locator)
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

void vtkExtractEdges::CreateDefaultLocator()
{
  if ( this->Locator == NULL )
    {
    this->Locator = vtkMergePoints::New();
    }
}

void vtkExtractEdges::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if ( this->Locator )
    {
    os << indent << "Locator: " << this->Locator << "\n";
    }
  else
    {
    os << indent << "Locator: (none)\n";
    }
}


unsigned long int vtkExtractEdges::GetMTime()
{
  unsigned long mTime=this-> vtkDataSetToPolyDataFilter::GetMTime();
  unsigned long time;

  if ( this->Locator != NULL )
    {
    time = this->Locator->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }
  return mTime;
}

