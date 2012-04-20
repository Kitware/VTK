/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractEdges.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractEdges.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkEdgeTable.h"
#include "vtkGenericCell.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMergePoints.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkIncrementalPointLocator.h"

vtkStandardNewMacro(vtkExtractEdges);

//----------------------------------------------------------------------------
// Construct object.
vtkExtractEdges::vtkExtractEdges()
{
  this->Locator = NULL;
}

//----------------------------------------------------------------------------
vtkExtractEdges::~vtkExtractEdges()
{
  if ( this->Locator )
    {
    this->Locator->UnRegister(this);
    this->Locator = NULL;
    }
}

//----------------------------------------------------------------------------
// Generate feature edges for mesh
int vtkExtractEdges::RequestData(
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
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkPoints *newPts;
  vtkCellArray *newLines;
  vtkIdType numCells, cellNum, numPts, newId;
  int edgeNum, numEdgePts, numCellEdges;
  int i, abort = 0;
  vtkIdType pts[2];
  vtkIdType pt1 = 0, pt2;
  double x[3];
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
    return 1;
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
  vtkIdList *edgeIds, *HEedgeIds=vtkIdList::New();
  vtkPoints *edgePts, *HEedgePts=vtkPoints::New();

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
      this->UpdateProgress (static_cast<double>(cellNum) / numCells);
      abort = this->GetAbortExecute();
      }

    input->GetCell(cellNum,cell);
    numCellEdges = cell->GetNumberOfEdges();
    for (edgeNum=0; edgeNum < numCellEdges; edgeNum++ )
      {
      edge = cell->GetEdge(edgeNum);
      numEdgePts = edge->GetNumberOfPoints();

      // Tessellate higher-order edges
      if ( ! edge->IsLinear() )
        {
        edge->Triangulate(0, HEedgeIds, HEedgePts);
        edgeIds = HEedgeIds;
        edgePts = HEedgePts;

        for ( i=0; i < (edgeIds->GetNumberOfIds()/2); i++ )
          {
          pt1 = edgeIds->GetId(2*i);
          pt2 = edgeIds->GetId(2*i+1);
          edgePts->GetPoint(2*i, x);
          if ( this->Locator->InsertUniquePoint(x, pts[0]) )
            {
            outPD->CopyData (pd,pt1,pts[0]);
            }
          edgePts->GetPoint(2*i+1, x);
          if ( this->Locator->InsertUniquePoint(x, pts[1]) )
            {
            outPD->CopyData (pd,pt2,pts[1]);
            }
          if ( edgeTable->IsEdge(pt1,pt2) == -1 )
            {
            edgeTable->InsertEdge(pt1, pt2);
            newId = newLines->InsertNextCell(2,pts);
            outCD->CopyData(cd, cellNum, newId);
            }
          }
        } //if non-linear edge

      else // linear edges
        {
        edgeIds = edge->PointIds;
        edgePts = edge->Points;

        for ( i=0; i < numEdgePts; i++, pt1=pt2, pts[0]=pts[1] )
          {
          pt2 = edgeIds->GetId(i);
          edgePts->GetPoint(i, x);
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
          }//if linear edge
        }
      }//for all edges of cell
    }//for all cells

  vtkDebugMacro(<<"Created " << newLines->GetNumberOfCells() << " edges");

  //  Update ourselves.
  //
  HEedgeIds->Delete();
  HEedgePts->Delete();
  edgeTable->Delete();
  cell->Delete();

  output->SetPoints(newPts);
  newPts->Delete();

  output->SetLines(newLines);
  newLines->Delete();

  output->Squeeze();

  return 1;
}

//----------------------------------------------------------------------------
// Specify a spatial locator for merging points. By
// default an instance of vtkMergePoints is used.
void vtkExtractEdges::SetLocator(vtkIncrementalPointLocator *locator)
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

//----------------------------------------------------------------------------
void vtkExtractEdges::CreateDefaultLocator()
{
  if ( this->Locator == NULL )
    {
    vtkMergePoints *locator = vtkMergePoints::New();
    this->SetLocator(locator);
    locator->Delete();
    }
}

//----------------------------------------------------------------------------
int vtkExtractEdges::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
unsigned long int vtkExtractEdges::GetMTime()
{
  unsigned long mTime=this-> Superclass::GetMTime();
  unsigned long time;

  if ( this->Locator != NULL )
    {
    time = this->Locator->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }
  return mTime;
}
