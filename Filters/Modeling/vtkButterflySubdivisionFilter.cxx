/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkButterflySubdivisionFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkButterflySubdivisionFilter.h"

#include "vtkMath.h"
#include "vtkCellArray.h"
#include "vtkEdgeTable.h"
#include "vtkIdList.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"

vtkStandardNewMacro(vtkButterflySubdivisionFilter);

static double butterflyWeights[8] =
  {.5, .5, .125, .125, -.0625, -.0625, -.0625, -.0625};

void vtkButterflySubdivisionFilter::GenerateSubdivisionPoints(
  vtkPolyData *inputDS, vtkIntArray *edgeData, vtkPoints *outputPts,
  vtkPointData *outputPD)
{
  double *weights, *weights1, *weights2;
  vtkIdType *pts = 0;
  vtkIdType cellId, newId, i, j;
  int edgeId;
  vtkIdType npts = 0;
  vtkIdType p1, p2;
  int valence1, valence2;
  vtkCellArray *inputPolys=inputDS->GetPolys();
  vtkEdgeTable *edgeTable;
  vtkIdList *cellIds = vtkIdList::New();
  vtkIdList *p1CellIds = vtkIdList::New();
  vtkIdList *p2CellIds = vtkIdList::New();
  vtkIdList *stencil = vtkIdList::New();
  vtkIdList *stencil1 = vtkIdList::New();
  vtkIdList *stencil2 = vtkIdList::New();
  vtkPoints *inputPts=inputDS->GetPoints();
  vtkPointData *inputPD=inputDS->GetPointData();

  weights = new double[256];
  weights1 = new double[256];
  weights2 = new double[256];

  // Create an edge table to keep track of which edges we've processed
  edgeTable = vtkEdgeTable::New();
  edgeTable->InitEdgeInsertion(inputDS->GetNumberOfPoints());

  // Generate new points for subdivisions surface
  for (cellId=0, inputPolys->InitTraversal();
       inputPolys->GetNextCell(npts, pts); cellId++)
    {
    if ( inputDS->GetCellType(cellId) != VTK_TRIANGLE )
      {
      continue;
      }

    p1 = pts[2];
    p2 = pts[0];

    for (edgeId=0; edgeId < 3; edgeId++)
      {
      // Do we need to  create a point on this edge?
      if (edgeTable->IsEdge (p1, p2) == -1)
        {
        outputPD->CopyData (inputPD, p1, p1);
        outputPD->CopyData (inputPD, p2, p2);
        edgeTable->InsertEdge (p1, p2);

        inputDS->GetCellEdgeNeighbors (-1, p1, p2, cellIds);
        // If this is a boundary edge. we need to use a special subdivision rule
        if (cellIds->GetNumberOfIds() == 1)
          {
          // Compute new POsition and PointData using the same subdivision scheme
          this->GenerateBoundaryStencil (p1, p2,
                                         inputDS, stencil, weights);
          } // boundary edge
        else
          {
          // find the valence of the two points
          inputDS->GetPointCells (p1, p1CellIds);
          valence1 = p1CellIds->GetNumberOfIds();
          inputDS->GetPointCells (p2, p2CellIds);
          valence2 = p2CellIds->GetNumberOfIds();

          if (valence1 == 6 && valence2 == 6)
            {
            this->GenerateButterflyStencil (p1, p2,
                                            inputDS, stencil, weights);
            }
          else if (valence1 == 6 && valence2 != 6)
            {
            this->GenerateLoopStencil (p2, p1,
                                       inputDS, stencil, weights);
            }
          else if (valence1 != 6 && valence2 == 6)
            {
            this->GenerateLoopStencil (p1, p2,
                                       inputDS, stencil, weights);
            }
          else
            {
            // Edge connects two extraordinary vertices
            this->GenerateLoopStencil (p2, p1,
                                       inputDS, stencil1, weights1);
            this->GenerateLoopStencil (p1, p2,
                                       inputDS, stencil2, weights2);
            // combine the two stencils and halve the weights
            vtkIdType total = stencil1->GetNumberOfIds() +
              stencil2->GetNumberOfIds();
            stencil->SetNumberOfIds (total);

            j = 0;
            for (i = 0; i < stencil1->GetNumberOfIds(); i++)
              {
              stencil->InsertId(j, stencil1->GetId(i));
              weights[j++] = weights1[i] * .5;
              }
            for (i = 0; i < stencil2->GetNumberOfIds(); i++)
              {
              stencil->InsertId(j, stencil2->GetId(i));
              weights[j++] = weights2[i] * .5;
              }
            }
          }
          newId = this->InterpolatePosition (inputPts, outputPts, stencil, weights);
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
  delete [] weights; delete [] weights1; delete [] weights2;
  edgeTable->Delete();
  stencil->Delete (); stencil1->Delete (); stencil2->Delete ();
  cellIds->Delete();
  p1CellIds->Delete();
  p2CellIds->Delete();
}

void vtkButterflySubdivisionFilter::GenerateLoopStencil(
  vtkIdType p1, vtkIdType p2, vtkPolyData *polys, vtkIdList *stencilIds,
  double *weights)
{
  vtkIdList *cellIds = vtkIdList::New();
  vtkCell *cell;
  int j;
  vtkIdType startCell, nextCell, tp2, p;
  int shift[255];
  int processed = 0;
  int boundary = 0;

  // Find another cell with this edge (we assume there is just one)
  polys->GetCellEdgeNeighbors (-1, p1, p2, cellIds);
  startCell = cellIds->GetId(0);

  stencilIds->Reset();
  stencilIds->InsertNextId (p2);
  shift[0] = 0;

  // Walk around the loop and get cells
  nextCell = cellIds->GetId(1);
  tp2 = p2;
  while (nextCell != startCell)
    {
    cell = polys->GetCell(nextCell);
    p = -1;
    for (int i = 0; i < 3; i++)
      {
      if ((p = cell->GetPointId(i)) != p1 && cell->GetPointId(i) != tp2)
        {
        break;
        }
      }
    tp2 = p;
    stencilIds->InsertNextId (tp2);
    processed++;
    shift[processed] = processed;
    polys->GetCellEdgeNeighbors (nextCell, p1, tp2, cellIds);
    if (cellIds->GetNumberOfIds() != 1)
      {
      boundary = 1;
       break;
      }
    nextCell = cellIds->GetId(0);
    }

  // If p1 or p2 is on the boundary, use the butterfly stencil with reflected vertices.
  if (boundary)
    {
    this->GenerateButterflyStencil (p1, p2,
                                    polys, stencilIds, weights);
    cellIds->Delete();
    return;
    }

  // Generate weights
#define VTK_PI vtkMath::Pi()
  int K = stencilIds->GetNumberOfIds();
  if (K >= 5)
    {
    for (j = 0; j < K; j++)
      {
      weights[j] = (.25 +  cos (2.0 * VTK_PI * shift[j] / static_cast<double>(K))
                    + .5 * cos (4.0 * VTK_PI * shift[j] / static_cast<double>(K))) / static_cast<double>(K);
      }
    }
  else if (K == 4)
    {
    static double weights4[4] = {3.0/8.0, 0.0, -1.0/8.0, 0.0};
    weights[0] = weights4[abs(shift[0])];
    weights[1] = weights4[abs(shift[1])];
    weights[2] = weights4[abs(shift[2])];
    weights[3] = weights4[abs(shift[3])];
    }
  else if (K == 3)
    {
    static double weights3[3] = {5.0/12.0, -1.0/12.0, -1.0/12.0};
    weights[0] = weights3[abs(shift[0])];
    weights[1] = weights3[abs(shift[1])];
    weights[2] = weights3[abs(shift[2])];
    }
  else
    {  // K == 2. p1 must be on a boundary edge,
    cell = polys->GetCell(startCell);
    p = -1;
    for (int i = 0; i < 3; i++)
      {
      if ((p = cell->GetPointId(i)) != p1 && cell->GetPointId(i) != p2)
        {
        break;
        }
      }
    p2 = p;
    stencilIds->InsertNextId (p2);
    weights[0] = 5.0 / 12.0;
    weights[1] = -1.0 / 12.0;
    weights[2] = -1.0 / 12.0;
    }
  // add in the extraordinary vertex
  weights[stencilIds->GetNumberOfIds()] = .75;
  stencilIds->InsertNextId (p1);

  cellIds->Delete();
}

void vtkButterflySubdivisionFilter::GenerateBoundaryStencil(
  vtkIdType p1, vtkIdType p2, vtkPolyData *polys, vtkIdList *stencilIds,
  double *weights)
{
  vtkIdList *cellIds = vtkIdList::New();
  vtkIdType *cells;
  unsigned short ncells;
  vtkIdType *pts;
  vtkIdType npts;
  int i, j;
  vtkIdType p0, p3;

  // find a boundary edge that uses p1 other than the one containing p2
  polys->GetPointCells (p1, ncells, cells);
  p0 = -1;
  for (i = 0; i < ncells && p0 == -1; i++)
    {
    polys->GetCellPoints (cells[i], npts, pts);
    for (j = 0; j < npts; j++)
      {
      if (pts[j] == p1 || pts[j] == p2)
        {
        continue;
        }
      polys->GetCellEdgeNeighbors (-1, p1, pts[j], cellIds);
      if (cellIds->GetNumberOfIds() == 1)
        {
        p0 = pts[j];
        break;
        }
      }
    }
  // find a boundary edge that uses p2 other than the one containing p1
  polys->GetPointCells (p2, ncells, cells);
  p3 = -1;
  for (i = 0; i < ncells && p3 == -1; i++)
    {
    polys->GetCellPoints (cells[i], npts, pts);
    for (j = 0; j < npts; j++)
      {
      if (pts[j] == p1 || pts[j] == p2 || pts[j] == p0)
        {
        continue;
        }
      polys->GetCellEdgeNeighbors (-1, p2, pts[j], cellIds);
      if (cellIds->GetNumberOfIds() == 1)
        {
        p3 = pts[j];
        break;
        }
      }
    }
  stencilIds->SetNumberOfIds (4);
  stencilIds->SetId (0, p0);
  stencilIds->SetId (1, p1);
  stencilIds->SetId (2, p2);
  stencilIds->SetId (3, p3);
  weights[0] = -.0625;
  weights[1] = .5625;
  weights[2] = .5625;
  weights[3] = -.0625;

  cellIds->Delete();
}

void vtkButterflySubdivisionFilter::GenerateButterflyStencil (
  vtkIdType p1, vtkIdType p2, vtkPolyData *polys, vtkIdList *stencilIds,
  double *weights)
{
  vtkIdList *cellIds = vtkIdList::New();
  vtkCell *cell;
  int i;
  vtkIdType cell0, cell1;
  vtkIdType p, p3, p4, p5, p6, p7, p8;

  polys->GetCellEdgeNeighbors (-1, p1, p2, cellIds);
  cell0 = cellIds->GetId(0);
  cell1 = cellIds->GetId(1);

  cell = polys->GetCell(cell0);
  p3 = -1;
  for (i = 0; i < 3; i++)
    {
    if ((p = cell->GetPointId(i)) != p1 && cell->GetPointId(i) != p2)
      {
      p3 = p;
      break;
      }
    }
  cell = polys->GetCell(cell1);
  p4 = -1;
  for (i = 0; i < 3; i++)
    {
    if ((p = cell->GetPointId(i)) != p1 && cell->GetPointId(i) != p2)
      {
      p4 = p;
      break;
      }
    }

  polys->GetCellEdgeNeighbors (cell0, p1, p3, cellIds);
  p5 = -1;
  if (cellIds->GetNumberOfIds() > 0)
    {
    cell = polys->GetCell(cellIds->GetId(0));
    for (i = 0; i < 3; i++)
      {
      if ((p = cell->GetPointId(i)) != p1 && cell->GetPointId(i) != p3)
        {
        p5 = p;
        break;
        }
      }
    }

  polys->GetCellEdgeNeighbors (cell0, p2, p3, cellIds);
  p6 = -1;
  if (cellIds->GetNumberOfIds() > 0)
    {
    cell = polys->GetCell(cellIds->GetId(0));
    for (i = 0; i < 3; i++)
      {
      if ((p = cell->GetPointId(i)) != p2 && cell->GetPointId(i) != p3)
        {
        p6 = p;
        break;
        }
      }
    }

  polys->GetCellEdgeNeighbors (cell1, p1, p4, cellIds);
  p7 = -1;
  if (cellIds->GetNumberOfIds() > 0)
    {
    cell = polys->GetCell(cellIds->GetId(0));
    for (i = 0; i < 3; i++)
      {
      if ((p = cell->GetPointId(i)) != p1 && cell->GetPointId(i) != p4)
        {
        p7 = p;
        break;
        }
      }
    }

  p8 = -1;
  polys->GetCellEdgeNeighbors (cell1, p2, p4, cellIds);
  if (cellIds->GetNumberOfIds() > 0)
    {
    cell = polys->GetCell(cellIds->GetId(0));
    for (i = 0; i < 3; i++)
      {
      if ((p = cell->GetPointId(i)) != p2 && cell->GetPointId(i) != p4)
        {
        p8= p;
        break;
        }
      }
    }

  stencilIds->SetNumberOfIds (8);
  stencilIds->SetId(0, p1);
  stencilIds->SetId(1, p2);
  stencilIds->SetId(2, p3);
  stencilIds->SetId(3, p4);
  if (p5 != -1)
    {
    stencilIds->SetId(4, p5);
    }
  else if (p4 != -1)
    {
    stencilIds->SetId(4, p4);
    }
  else
    {
      vtkWarningMacro (<< "bad p5, p4 " << p5  << ", " << p4);
    }

  if (p6 != -1)
    {
    stencilIds->SetId(5, p6);
    }
  else if (p4 != -1)
    {
    stencilIds->SetId(5, p4);
    }
  else
    {
      vtkWarningMacro (<< "bad p5, p4 " << p5 << ", " << p4);
    }

  if (p7 != -1)
    {
    stencilIds->SetId(6, p7);
    }
  else if (p3 != -1)
    {
    stencilIds->SetId(6, p3);
    }
  else
    {
      vtkWarningMacro (<< "bad p7, p3 " << p7 << ", " << p3);
    }

  if (p8 != -1)
    {
    stencilIds->SetId(7, p8);
    }
  else if (p3 != -1)
    {
    stencilIds->SetId(7, p3);
    }
  else
    {
      vtkWarningMacro (<< "bad p7, p8 " << p7 << ", " << p3);
    }


  for (i = 0; i < stencilIds->GetNumberOfIds (); i++)
    {
    weights[i] = butterflyWeights[i];
    }
  cellIds->Delete();
}
