/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLoopSubdivisionFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    This work was supported bt PHS Research Grant No. 1 P41 RR13218-01
             from the National Center for Research Resources

Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkLoopSubdivisionFilter.h"
#include "vtkEdgeTable.h"
#include "vtkObjectFactory.h"

static float LoopWeights[4] =
  {.375, .375, .125, .125};

vtkLoopSubdivisionFilter* vtkLoopSubdivisionFilter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkLoopSubdivisionFilter");
  if(ret)
    {
    return (vtkLoopSubdivisionFilter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkLoopSubdivisionFilter;
}

void vtkLoopSubdivisionFilter::GenerateSubdivisionPoints (vtkPolyData *inputDS, vtkIntArray *edgeData, vtkPoints *outputPts, vtkPointData *outputPD)
{
  float *weights;
  int *pts;
  int numPts;
  int cellId, edgeId, newId;
  int npts;
  int p1, p2;
  vtkCellArray *inputPolys=inputDS->GetPolys();
  vtkEdgeTable *edgeTable;
  vtkIdList *cellIds = vtkIdList::New();
  vtkIdList *stencil = vtkIdList::New();
  vtkPoints *inputPts=inputDS->GetPoints();
  vtkPointData *inputPD=inputDS->GetPointData();

  weights = new float[256];

  // Create an edge table to keep track of which edges we've processed
  edgeTable = vtkEdgeTable::New();
  edgeTable->InitEdgeInsertion(inputDS->GetNumberOfPoints());

  // Generate even points. these are derived from the old points
  numPts = inputDS->GetNumberOfPoints();
  for (int ptId=0; ptId < numPts; ptId++)
    {
    this->GenerateEvenStencil (ptId, inputDS, stencil, weights);
    this->InterpolatePosition (inputPts, outputPts, stencil, weights);
    outputPD->InterpolatePoint (inputPD, ptId, stencil, weights);
    }

  // Generate odd points. These will be inserted into the new dataset
  for (cellId=0, inputPolys->InitTraversal(); inputPolys->GetNextCell(npts, pts); cellId++)
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
	else
	  {
	    this->GenerateOddStencil (p1, p2,
				      inputDS, stencil, weights);
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
  edgeTable->Delete();
  stencil->Delete ();
  cellIds->Delete();
}

void vtkLoopSubdivisionFilter::GenerateEvenStencil (int p1, vtkPolyData *polys, vtkIdList *stencilIds, float *weights)
{
  vtkIdList *cellIds = vtkIdList::New();
  vtkIdList *ptIds = vtkIdList::New();
  vtkCell *cell;

  int i, j;
  int numCellsInLoop;
  int startCell, nextCell;
  int p, p2;
  int bp1, bp2;
  int K;
  float beta, cosSQ;

  // Get the cells that use this point
  polys->GetPointCells (p1, cellIds);
  numCellsInLoop = cellIds->GetNumberOfIds();
  if (numCellsInLoop < 1)
      {
      vtkErrorMacro("numCellsInLoop < 1: " << numCellsInLoop);
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
#define VTK_PI 3.14159265358979
      cosSQ = .375 + .25 * cos (2.0 * VTK_PI / (float) K);
      cosSQ = cosSQ * cosSQ;
      beta = (.625 -  cosSQ) / (float) K;
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

void vtkLoopSubdivisionFilter::GenerateOddStencil (int p1, int p2, vtkPolyData *polys, vtkIdList *stencilIds, float *weights)
{
  vtkIdList *cellIds = vtkIdList::New();
  vtkCell *cell;
  int i;
  int cell0, cell1;
  int p3, p4;

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
