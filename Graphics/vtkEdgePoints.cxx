/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEdgePoints.cxx
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
#include "vtkEdgePoints.h"
#include "vtkObjectFactory.h"
#include "vtkFloatArray.h"

vtkCxxRevisionMacro(vtkEdgePoints, "1.46");
vtkStandardNewMacro(vtkEdgePoints);

// Construct object with contour value of 0.0.
vtkEdgePoints::vtkEdgePoints()
{
  this->Value = 0.0;
  this->Locator = vtkMergePoints::New();
}

vtkEdgePoints::~vtkEdgePoints()
{
  this->Locator->Delete();
  this->Locator = NULL;
}

//
// General filter: handles arbitrary input.
//
void vtkEdgePoints::Execute()
{
  vtkDataArray *inScalars;
  vtkPoints *newPts;
  vtkCellArray *newVerts;
  vtkIdType cellId, ptId, edgeId, newCellId;
  int above, below, i, numEdges;
  vtkCell *edge;
  float range[2];
  float s0, s1, x0[3], x1[3], x[3], t;
  float e0Scalar, deltaScalar;
  int e0, e1;
  vtkIdType p1, p2;
  vtkIdType pts[1];
  vtkIdType numCells, estimatedSize;
  vtkDataSet *input = this->GetInput();
  vtkPolyData *output = this->GetOutput();
  vtkDataArray *cellScalars;
  vtkPointData *inPd=input->GetPointData(), *outPd=output->GetPointData();
  vtkCellData *inCd=input->GetCellData(), *outCd=output->GetCellData();

  vtkDebugMacro(<< "Generating edge points");

  // Initialize and check input
  //
  if ( ! (inScalars = input->GetPointData()->GetScalars()) )
    {
    vtkErrorMacro(<<"No scalar data to contour");
    return;
    }

  inScalars->GetRange(range,0);
  if ( this->Value < range[0] || this->Value > range[1] )
    {
    vtkWarningMacro(<<"Value lies outside of scalar range");
    return;
    }

  numCells = input->GetNumberOfCells();
  estimatedSize = (vtkIdType) (numCells * .75);
  estimatedSize = estimatedSize / 1024 * 1024; //multiple of 1024
  if (estimatedSize < 1024)
    {
    estimatedSize = 1024;
    }

  newPts = vtkPoints::New();
  newPts->Allocate(estimatedSize, estimatedSize/2);
  newVerts = vtkCellArray::New();
  newVerts->Allocate(estimatedSize, estimatedSize/2);
  cellScalars = inScalars->MakeObject();
  cellScalars->Allocate(VTK_CELL_SIZE*inScalars->GetNumberOfComponents());
  
  this->Locator->InitPointInsertion (newPts, input->GetBounds());

  // interpolate data along edge; copy cell data
  outPd->InterpolateAllocate(inPd,5000,10000);
  outCd->CopyAllocate(inCd,5000,10000);

  // Traverse all edges. Since edges are not explicitly represented, use a
  // trick: traverse all cells and obtain cell edges and then cell edge
  // neighbors. If cell id < all edge neigbors ids, then this edge has not
  // yet been visited and is processed.
  //
  int abort=0;
  vtkIdType progressInterval = numCells/20 + 1;
  vtkGenericCell *cell = vtkGenericCell::New();
  for (cellId=0; cellId < numCells && !abort; cellId++)
    {
    if ( ! (cellId % progressInterval) ) 
      {
      vtkDebugMacro(<<"Processing #" << cellId);
      this->UpdateProgress ((float)cellId/numCells);
      abort = this->GetAbortExecute();
      }

    input->GetCell(cellId,cell);
    inScalars->GetTuples(cell->PointIds, cellScalars);

    // loop over cell points to check if cell straddles isosurface value
    for ( above=below=0, ptId=0; ptId < cell->GetNumberOfPoints(); ptId++ )
      {
      if ( cellScalars->GetComponent(ptId,0) >= this->Value )
        {
        above = 1;
        }
      else if ( cellScalars->GetComponent(ptId,0) < this->Value )
        {
        below = 1;
        }
      }

    if ( above && below ) //contour passes through cell
      {
      if ( cell->GetCellDimension() < 2 ) //only points can be generated
        {
        cell->Contour(this->Value, cellScalars, this->Locator, newVerts, 
                      NULL, NULL, inPd, outPd, inCd, cellId, outCd);
        }

      else //
        {
        numEdges = cell->GetNumberOfEdges();
        for (edgeId=0; edgeId < numEdges; edgeId++)
          {
          edge = cell->GetEdge(edgeId);
          inScalars->GetTuples(edge->PointIds, cellScalars);

          s0 = cellScalars->GetComponent(0,0);
          s1 = cellScalars->GetComponent(1,0);
          if ( (s0 < this->Value && s1 >= this->Value) ||
          (s0 >= this->Value && s1 < this->Value) )
            {
            //ordering intersection direction avoids numerical problems
            deltaScalar = s1 - s0; 
            if (deltaScalar > 0)
              {
              e0 = 0; e1 = 1;
              e0Scalar = s0;
              }
            else
              {
              e0 = 1; e1 = 0;
              e0Scalar = s1;
              deltaScalar = -deltaScalar;
              }

            t = (this->Value - e0Scalar) / deltaScalar;

            edge->Points->GetPoint(e0,x0);
            edge->Points->GetPoint(e1,x1);

            for (i=0; i<3; i++)
              {
              x[i] = x0[i] + t * (x1[i] - x0[i]);
              }
            if ( this->Locator->InsertUniquePoint(x, pts[0]) )
              {
              newCellId = newVerts->InsertNextCell(1,pts);
              outCd->CopyData(inCd,cellId,newCellId);
              p1 = edge->PointIds->GetId(e0);
              p2 = edge->PointIds->GetId(e1);
              outPd->InterpolateEdge(inPd,pts[0],p1,p2,t);
              } //if point not created before
            } //if edge straddles contour value
          } //for each edge
        } //dimension 2 and higher
      } //above and below
    } //for all cells
  cell->Delete();

  vtkDebugMacro(<<"Created: " << newPts->GetNumberOfPoints() << " points");

  // Update ourselves.  Because we don't know up front how many verts we've 
  // created, take care to reclaim memory. 
  //
  output->SetPoints(newPts);
  newPts->Delete();

  output->SetVerts(newVerts);
  newVerts->Delete();

  this->Locator->Initialize();//free up any extra memory
  output->Squeeze();
  
  cellScalars->Delete();
}

void vtkEdgePoints::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Contour Value: " << this->Value << "\n";
}


