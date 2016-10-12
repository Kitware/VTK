/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLoopSubdivisionFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLoopSubdivisionFilter.h"

#include "vtkSmartPointer.h"
#include "vtkMath.h"
#include "vtkCell.h"
#include "vtkCellArray.h"
#include "vtkEdgeTable.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkLoopSubdivisionFilter);

static double LoopWeights[4] =
  {.375, .375, .125, .125};

int vtkLoopSubdivisionFilter::GenerateSubdivisionPoints (vtkPolyData *inputDS,vtkIntArray *edgeData, vtkPoints *outputPts, vtkPointData *outputPD)
{
  double *weights;
  vtkIdType *pts = 0;
  vtkIdType numPts, cellId, newId;
  int edgeId;
  vtkIdType npts;
  vtkIdType p1, p2;
  vtkCellArray *inputPolys=inputDS->GetPolys();
  vtkSmartPointer<vtkIdList> cellIds = vtkSmartPointer<vtkIdList>::New();
  vtkSmartPointer<vtkIdList> stencil = vtkSmartPointer<vtkIdList>::New();
  vtkPoints *inputPts=inputDS->GetPoints();
  vtkPointData *inputPD=inputDS->GetPointData();

  weights = new double[256];

  // Create an edge table to keep track of which edges we've processed
  vtkSmartPointer<vtkEdgeTable> edgeTable =
    vtkSmartPointer<vtkEdgeTable>::New();
  edgeTable->InitEdgeInsertion(inputDS->GetNumberOfPoints());

  // Generate even points. these are derived from the old points
  numPts = inputDS->GetNumberOfPoints();
  for (vtkIdType ptId=0; ptId < numPts; ptId++)
  {
    this->GenerateEvenStencil (ptId, inputDS, stencil, weights);
    this->InterpolatePosition (inputPts, outputPts, stencil, weights);
    outputPD->InterpolatePoint (inputPD, ptId, stencil, weights);
  }

  // Generate odd points. These will be inserted into the new dataset
  for (cellId=0, inputPolys->InitTraversal();
       inputPolys->GetNextCell(npts, pts); cellId++)
  {
    if ( inputDS->GetCellType(cellId) != VTK_TRIANGLE )
    {
      continue;
    }

    // start with one edge
    p1 = pts[2];
    p2 = pts[0];

    for (edgeId=0; edgeId < 3; edgeId++)
    {
      // Do we need to  create a point on this edge?
      if (edgeTable->IsEdge (p1, p2) == -1)
      {
        edgeTable->InsertEdge (p1, p2);
        inputDS->GetCellEdgeNeighbors (-1, p1, p2, cellIds);
        if (cellIds->GetNumberOfIds() == 1)
        {
          // Compute new Position and PointData using the same subdivision scheme
          stencil->SetNumberOfIds(2);
          stencil->SetId(0,p1);
          stencil->SetId(1,p2);
          weights[0] = .5; weights[1] = .5;
        } // boundary edge
        else if (cellIds->GetNumberOfIds() == 2)
        {
          this->GenerateOddStencil (p1, p2,
                                    inputDS, stencil, weights);
        }
        else
        {
          delete [] weights;
          vtkErrorMacro ("Dataset is non-manifold and cannot be subdivided.");
          return 0;
        }
        newId = this->InterpolatePosition (inputPts, outputPts,
                                           stencil, weights);
        outputPD->InterpolatePoint (inputPD, newId, stencil, weights);
      }
      else // we have already created a point on this edge. find it
      {
        newId = this->FindEdge (inputDS, cellId, p1, p2, edgeData, cellIds);
      }
      edgeData->InsertComponent(cellId,edgeId,newId);
      p1 = p2;
      if (edgeId < 2)
      {
        p2 = pts[edgeId + 1];
      }
    } // each interior edge
  } // each cell

  // cleanup
  delete [] weights;
  return 1;
}

void vtkLoopSubdivisionFilter::GenerateEvenStencil (vtkIdType p1,
                                                    vtkPolyData *polys,
                                                    vtkIdList *stencilIds,
                                                    double *weights)
{
  vtkIdList *cellIds = vtkIdList::New();
  vtkIdList *ptIds = vtkIdList::New();
  vtkCell *cell;

  int i;
  vtkIdType j;
  vtkIdType startCell, nextCell;
  vtkIdType p, p2;
  vtkIdType bp1, bp2;
  vtkIdType K;
  double beta, cosSQ;

  // Get the cells that use this point
  polys->GetPointCells (p1, cellIds);
  vtkIdType numCellsInLoop = cellIds->GetNumberOfIds();
  if (numCellsInLoop < 1)
  {
      vtkWarningMacro("numCellsInLoop < 1: " << numCellsInLoop);
      stencilIds->Reset();
      return;
  }
  // Find an edge to start with that contains p1
  polys->GetCellPoints (cellIds->GetId(0), ptIds);
  p2 = ptIds->GetId(0);
  i = 1;
  while (p1 == p2)
  {
    p2 = ptIds->GetId(i++);
  }
  polys->GetCellEdgeNeighbors (-1, p1, p2, cellIds);

  nextCell = cellIds->GetId(0);
  bp2 = -1;
  bp1 = p2;
  if (cellIds->GetNumberOfIds() == 1)
  {
    startCell = -1;
  }
  else
  {
    startCell = cellIds->GetId(1);
  }

  stencilIds->Reset();
  stencilIds->InsertNextId(p2);

  // walk around the loop counter-clockwise and get cells
  for (j = 0; j < numCellsInLoop; j++)
  {
    cell = polys->GetCell(nextCell);
    p = -1;
    for (i = 0; i < 3; i++)
    {
      if ((p = cell->GetPointId(i)) != p1 && cell->GetPointId(i) != p2)
      {
        break;
      }
    }
    p2 = p;
    stencilIds->InsertNextId (p2);
    polys->GetCellEdgeNeighbors (nextCell, p1, p2, cellIds);
    if (cellIds->GetNumberOfIds() != 1)
    {
      bp2 = p2;
      j++;
      break;
    }
    nextCell = cellIds->GetId(0);
  }

  // now walk around the other way. this will only happen if there
  // is a boundary cell left that we have not visited
  nextCell = startCell;
  p2 = bp1;
  for (; j < numCellsInLoop && startCell != -1; j++)
  {
    cell = polys->GetCell(nextCell);
    p = -1;
    for (i = 0; i < 3; i++)
    {
      if ((p = cell->GetPointId(i)) != p1 && cell->GetPointId(i) != p2)
      {
        break;
      }
    }
    p2 = p;
    stencilIds->InsertNextId (p2);
    polys->GetCellEdgeNeighbors (nextCell, p1, p2, cellIds);
    if (cellIds->GetNumberOfIds() != 1)
    {
      bp1 = p2;
      break;
    }
    nextCell = cellIds->GetId(0);
  }

  if (bp2 != -1) // boundary edge
  {
    stencilIds->SetNumberOfIds(3);
    stencilIds->SetId(0,bp2);
    stencilIds->SetId(1,bp1);
    stencilIds->SetId(2,p1);
    weights[0] = .125;
    weights[1] = .125;
    weights[2] = .75;
  }
  else
  {
    K = stencilIds->GetNumberOfIds();
   // Remove last id. It's a duplicate of the first
    K--;
    if (K > 3)
    {
      // Generate weights
#define VTK_PI vtkMath::Pi()
      cosSQ = .375 + .25 * cos (2.0 * VTK_PI / (double) K);
      cosSQ = cosSQ * cosSQ;
      beta = (.625 -  cosSQ) / (double) K;
    }
    else
    {
      beta = 3.0 / 16.0;
    }
    for (j = 0; j < K; j++)
    {
      weights[j] = beta;
    }
    weights[K] = 1.0 - K * beta;
    stencilIds->SetId (K,p1);
  }
  cellIds->Delete();
  ptIds->Delete();
}

void vtkLoopSubdivisionFilter::GenerateOddStencil (vtkIdType p1, vtkIdType p2,
                                                   vtkPolyData *polys,
                                                   vtkIdList *stencilIds,
                                                   double *weights)
{
  vtkIdList *cellIds = vtkIdList::New();
  vtkCell *cell;
  int i;
  vtkIdType cell0, cell1;
  vtkIdType p3=0, p4=0;

  polys->GetCellEdgeNeighbors (-1, p1, p2, cellIds);
  cell0 = cellIds->GetId(0);
  cell1 = cellIds->GetId(1);

  cell = polys->GetCell(cell0);
  for (i = 0; i < 3; i++)
  {
    if ((p3 = cell->GetPointId(i)) != p1 && cell->GetPointId(i) != p2)
    {
      break;
    }
  }
  cell = polys->GetCell(cell1);
  for (i = 0; i < 3; i++)
  {
    if ((p4 = cell->GetPointId(i)) != p1 && cell->GetPointId(i) != p2)
    {
      break;
    }
  }

  stencilIds->SetNumberOfIds (4);
  stencilIds->SetId(0, p1);
  stencilIds->SetId(1, p2);
  stencilIds->SetId(2, p3);
  stencilIds->SetId(3, p4);

  for (i = 0; i < stencilIds->GetNumberOfIds (); i++)
  {
    weights[i] = LoopWeights[i];
  }
  cellIds->Delete();
}

int vtkLoopSubdivisionFilter::RequestUpdateExtent(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  int numPieces, ghostLevel;
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  if (!this->Superclass::RequestUpdateExtent(request, inputVector,
                                             outputVector))
  {
    return 0;
  }

  numPieces =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  ghostLevel =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  if (numPieces > 1 && this->NumberOfSubdivisions > 0)
  {
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
                ghostLevel + 1);
  }

  return 1;
}

int vtkLoopSubdivisionFilter::RequestData(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  vtkPolyData *input = vtkPolyData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkCellArray *polys = input->GetPolys();
  int hasTris = 0;
  vtkIdType numPts = 0, *pts = 0;

  input->BuildLinks();

  polys->InitTraversal();
  while (polys->GetNextCell(numPts, pts))
  {
    if (numPts == 3)
    {
      if (input->IsTriangle(pts[0], pts[1], pts[2]))
      {
        hasTris = 1;
        break;
      }
    }
  }

  if (!hasTris)
  {
    vtkWarningMacro("vtkLoopSubdivisionFilter only operates on triangles, but this data set has no triangles to operate on.")
    return 0;
  }

  return this->Superclass::RequestData(request, inputVector, outputVector);
}
