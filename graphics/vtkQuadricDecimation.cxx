/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuadricDecimation.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkQuadricDecimation.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkQuadricDecimation* vtkQuadricDecimation::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkQuadricDecimation");
  if(ret)
    {
    return (vtkQuadricDecimation*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkQuadricDecimation;
}

//----------------------------------------------------------------------------
vtkQuadricDecimation::vtkQuadricDecimation()
{
  this->Edges = vtkEdgeTable::New();
  this->EdgeCosts = vtkPriorityQueue::New();
  this->EndPoint1List = vtkIdList::New();
  this->EndPoint2List = vtkIdList::New();
  this->ErrorQuadrics = NULL;
  this->MaximumCost = 0.1;
}

//----------------------------------------------------------------------------
vtkQuadricDecimation::~vtkQuadricDecimation()
{
  this->Edges->Delete();
  this->EdgeCosts->Delete();
  this->EndPoint1List->Delete();
  this->EndPoint2List->Delete();
}

//----------------------------------------------------------------------------
void vtkQuadricDecimation::Execute()
{
  vtkPolyData *input = this->GetInput();
  vtkPolyData *output = this->GetOutput();
  vtkPolyData *mesh = vtkPolyData::New();
  vtkCellArray *triangles = input->GetPolys();
  int numTris = triangles->GetNumberOfCells();
  int numPts = input->GetNumberOfPoints();
  int i, j, edgeId;
  int numCellPts, *cellPts;
  float cost, x[3];
  vtkPoints *targetPoints = vtkPoints::New();
  vtkPoints *meshPoints = vtkPoints::New();
  vtkCellArray *meshPolys = vtkCellArray::New();
  vtkIdList *changedEdges = vtkIdList::New();
  int collapsedCell;
  vtkIdList *collapsedCells = vtkIdList::New();
  int endPtIds[2], edge[2];
  vtkIdList *changedCells = vtkIdList::New();
  vtkIdList *newEdges = vtkIdList::New();
  vtkIdList *outputCellList = vtkIdList::New();
  vtkCellArray *outputPolys = vtkCellArray::New();
  vtkGenericCell *cell = vtkGenericCell::New();
  int cellId;
  
  if (input->GetPolys() == NULL)
    {
    vtkErrorMacro("no triangles to decimate");
    return;
    }

  this->ErrorQuadrics = new VTK_ERROR_QUADRIC[input->GetNumberOfPoints()];
  meshPoints->DeepCopy(input->GetPoints());
  meshPolys->DeepCopy(input->GetPolys());
  mesh->SetPoints(meshPoints);
  mesh->SetPolys(meshPolys);
  input->BuildLinks();
  mesh->BuildLinks();
  output->SetPolys(outputPolys);
  this->Edges->InitEdgeInsertion(numPts, 1); // storing edge id as attribute
  this->EdgeCosts->Allocate(input->GetPolys()->GetNumberOfCells() * 3);
  triangles->InitTraversal();
  for (i = 0; i < numTris; i++)
    {
    triangles->GetNextCell(numCellPts, cellPts);
    outputCellList->InsertNextId(i);
    for (j = 0; j < 3; j++)
      {
      if (this->Edges->IsEdge(cellPts[j], cellPts[(j+1)%3]) == -1)
        {
        edgeId = this->Edges->GetNumberOfEdges();
        this->Edges->InsertEdge(cellPts[j], cellPts[(j+1)%3], edgeId);
        this->EndPoint1List->InsertId(edgeId, cellPts[j]);
        this->EndPoint2List->InsertId(edgeId, cellPts[(j+1)%3]);
        }
      }
    }

  for (i = 0; i < numPts; i++)
    {
    this->ComputeQuadric(i);
    }

  for (i = 0; i < this->Edges->GetNumberOfEdges(); i++)
    {
    cost = this->ComputeCost(i, x);
    this->EdgeCosts->Insert(cost, i);
    targetPoints->InsertPoint(i, x);
    }
  
  edgeId = this->EdgeCosts->Pop(cost);
  while (cost < this->MaximumCost)
    {
    endPtIds[0] = this->EndPoint1List->GetId(edgeId);
    endPtIds[1] = this->EndPoint2List->GetId(edgeId);
    changedEdges->Reset();
    changedCells->Reset();
    collapsedCells->Reset();
    newEdges->Reset();
    // Set the new coordinates of point1.
    mesh->GetPoints()->SetPoint(endPtIds[0], targetPoints->GetPoint(edgeId));
    // Find all edges with either of these 2 endpoints.
    this->FindAffectedEdges(endPtIds[0], endPtIds[1], changedEdges, mesh);
    // Reset the endpoints for these edges to reflect the new point from the
    // collapsed edge.
    // Add these new edges to the edge table.
    // Remove the the changed edges from the priority queue.
    for (i = 0; i < changedEdges->GetNumberOfIds(); i++)
      {
      edge[0] = this->EndPoint1List->GetId(changedEdges->GetId(i));
      edge[1] = this->EndPoint2List->GetId(changedEdges->GetId(i));
      if (edge[0] == endPtIds[1])
        {
        if (this->Edges->IsEdge(edge[1], endPtIds[0]) == -1)
          {
          edgeId = this->Edges->GetNumberOfEdges();
          this->Edges->InsertEdge(edge[1], endPtIds[0], edgeId);
          this->EndPoint1List->InsertId(edgeId, edge[1]);
          this->EndPoint2List->InsertId(edgeId, endPtIds[0]);
          newEdges->InsertNextId(edgeId);
          }
        this->EdgeCosts->DeleteId(changedEdges->GetId(i));
        }
      else if (edge[1] == endPtIds[1])
        {
        if (this->Edges->IsEdge(edge[0], endPtIds[0]) == -1)
          {
          edgeId = this->Edges->GetNumberOfEdges();
          this->Edges->InsertEdge(edge[0], endPtIds[0], edgeId);
          this->EndPoint1List->InsertId(edgeId, edge[0]);
          this->EndPoint2List->InsertId(edgeId, endPtIds[0]);
          newEdges->InsertNextId(edgeId);
          }
        this->EdgeCosts->DeleteId(changedEdges->GetId(i));
        }
      }
    
    // Update vertex quadric for vertex 0.
    this->AddQuadric(endPtIds[1], endPtIds[0]);
    // Update cost for new edges.
    // Add new edges to priority queue with correct cost.
    // Recompute the contraction targets for the new edges we have.
    for (i = 0; i < newEdges->GetNumberOfIds(); i++)
      {
      cost = this->ComputeCost(newEdges->GetId(i), x);
      this->EdgeCosts->Insert(cost, newEdges->GetId(i));
      targetPoints->InsertPoint(newEdges->GetId(i), x);
      }
    // Update the output triangles.
    collapsedCell = this->GetEdgeCellId(endPtIds[0], endPtIds[1], mesh);
    mesh->GetCellEdgeNeighbors(collapsedCell, endPtIds[0], endPtIds[1],
                               collapsedCells);
    mesh->RemoveCellReference(collapsedCell);
    outputCellList->DeleteId(collapsedCell);
    for (i = 0; i < collapsedCells->GetNumberOfIds(); i++)
      {
      mesh->RemoveCellReference(collapsedCells->GetId(i));
      outputCellList->DeleteId(collapsedCells->GetId(i));
      }
    mesh->GetPointCells(endPtIds[1], changedCells);
    for (i = 0; i < changedCells->GetNumberOfIds(); i++)
      {
      cellId = changedCells->GetId(i);
      mesh->GetCell(cellId, cell);
      cellPts[0] = cell->GetPointId(0);
      cellPts[1] = cell->GetPointId(1);
      cellPts[2] = cell->GetPointId(2);
      // making sure we don't already have the triangle we're about to
      // change this one to
      if (cellPts[0] == endPtIds[1])
        {
        if (mesh->IsTriangle(endPtIds[0], cellPts[1], cellPts[2]))
          {
          mesh->RemoveCellReference(cellId);
          outputCellList->DeleteId(cellId);
          }
        else
          {
          mesh->RemoveReferenceToCell(endPtIds[1], cellId);
          mesh->AddReferenceToCell(endPtIds[0], cellId);
          mesh->ReplaceCellPoint(cellId, endPtIds[1], endPtIds[0]);
          }
        }
      else if (cellPts[1] == endPtIds[1])
        {
        if (mesh->IsTriangle(cellPts[0], endPtIds[0], cellPts[2]))
          {
          mesh->RemoveCellReference(cellId);
          outputCellList->DeleteId(cellId);
          }
        else
          {
          mesh->RemoveReferenceToCell(endPtIds[1], cellId);
          mesh->AddReferenceToCell(endPtIds[0], cellId);
          mesh->ReplaceCellPoint(cellId, endPtIds[1], endPtIds[0]);
          }
        }
      else
        {
        if (mesh->IsTriangle(cellPts[0], cellPts[1], endPtIds[0]))
          {
          mesh->RemoveCellReference(cellId);
          outputCellList->DeleteId(cellId);
          }
        else
          {
          mesh->RemoveReferenceToCell(endPtIds[1], cellId);
          mesh->AddReferenceToCell(endPtIds[0], cellId);
          mesh->ReplaceCellPoint(cellId, endPtIds[1], endPtIds[0]);
          }
        }
      }
    edgeId = this->EdgeCosts->Pop(cost);
    if (edgeId == -1)
      {
      // tried to pop empty priority queue, so need to exit loop
      cost = this->MaximumCost;
      }
    }
  
  output->CopyCells(mesh, outputCellList);
  
  changedEdges->Delete();
  collapsedCells->Delete();
  changedCells->Delete();
  newEdges->Delete();
  targetPoints->Delete();
  meshPoints->Delete();
  meshPolys->Delete();
//  mesh->Delete();
  outputCellList->Delete();
  outputPolys->Delete();
  delete [] this->ErrorQuadrics;
}

//----------------------------------------------------------------------------
void vtkQuadricDecimation::ComputeQuadric(int pointId)
{
  vtkIdList *cellIds = vtkIdList::New();
  vtkPolyData *input = this->GetInput();
  int i, j;
  float n[3], d;
  vtkPoints *cellPts;
  float point0[3], point1[3], point2[3];
  float tempP1[3], tempP2[3];
  
  for (i = 0; i < 10; i++)
    {
    this->ErrorQuadrics[pointId].GeometricQuadric[i] = 0.0;
    }
  
  // geometric part of quadric
  input->GetPointCells(pointId, cellIds);
  for (i = 0; i < cellIds->GetNumberOfIds(); i++)
    {
    cellPts = input->GetCell(cellIds->GetId(i))->GetPoints();
    cellPts->GetPoint(0, point0);
    cellPts->GetPoint(1, point1);
    cellPts->GetPoint(2, point2);
    for (j = 0; j < 3; j++)
      {
      tempP1[j] = point1[j] - point0[j];
      tempP2[j] = point2[j] - point0[j];
      }
    vtkMath::Cross(tempP1, tempP2, n);
    vtkMath::Normalize(n);
    d = -vtkMath::Dot(n, point0);

    this->ErrorQuadrics[pointId].GeometricQuadric[0] += n[0] * n[0];
    this->ErrorQuadrics[pointId].GeometricQuadric[1] += n[1] * n[1];
    this->ErrorQuadrics[pointId].GeometricQuadric[2] += n[2] * n[2];
    this->ErrorQuadrics[pointId].GeometricQuadric[3] += n[0] * n[1];
    this->ErrorQuadrics[pointId].GeometricQuadric[4] += n[1] * n[2];
    this->ErrorQuadrics[pointId].GeometricQuadric[5] += n[0] * n[2];
    this->ErrorQuadrics[pointId].GeometricQuadric[6] += d * n[0];
    this->ErrorQuadrics[pointId].GeometricQuadric[7] += d * n[1];
    this->ErrorQuadrics[pointId].GeometricQuadric[8] += d * n[2];
    this->ErrorQuadrics[pointId].GeometricQuadric[9] += d * d;
    }
}

//----------------------------------------------------------------------------
void vtkQuadricDecimation::AddQuadric(int oldPtId, int newPtId)
{
  int i;
  
  for (i = 0; i < 10; i++)
    {
    this->ErrorQuadrics[newPtId].GeometricQuadric[i] +=
      this->ErrorQuadrics[oldPtId].GeometricQuadric[i];
    }
}

//----------------------------------------------------------------------------
float vtkQuadricDecimation::ComputeCost(int edgeId, float x[3])
{
  // This will change when we actually take attributes into account
  
  float A[3][3], b[3];
  int pointIds[2];
  float cost;
  
  pointIds[0] = this->EndPoint1List->GetId(edgeId);
  pointIds[1] = this->EndPoint2List->GetId(edgeId);
  
  A[0][0] = this->ErrorQuadrics[pointIds[0]].GeometricQuadric[0] +
    this->ErrorQuadrics[pointIds[1]].GeometricQuadric[0];
  A[1][1] = this->ErrorQuadrics[pointIds[0]].GeometricQuadric[1] +
    this->ErrorQuadrics[pointIds[1]].GeometricQuadric[1];
  A[2][2] = this->ErrorQuadrics[pointIds[0]].GeometricQuadric[2] +
    this->ErrorQuadrics[pointIds[1]].GeometricQuadric[2];
  A[0][1] = A[1][0] = this->ErrorQuadrics[pointIds[0]].GeometricQuadric[3] +
    this->ErrorQuadrics[pointIds[1]].GeometricQuadric[3];
  A[1][2] = A[2][1] = this->ErrorQuadrics[pointIds[0]].GeometricQuadric[4] +
    this->ErrorQuadrics[pointIds[1]].GeometricQuadric[4];
  A[0][2] = A[2][0] = this->ErrorQuadrics[pointIds[0]].GeometricQuadric[5] +
    this->ErrorQuadrics[pointIds[1]].GeometricQuadric[5];
  b[0] = -(this->ErrorQuadrics[pointIds[0]].GeometricQuadric[6] +
    this->ErrorQuadrics[pointIds[1]].GeometricQuadric[6]);
  b[1] = -(this->ErrorQuadrics[pointIds[0]].GeometricQuadric[7] +
    this->ErrorQuadrics[pointIds[1]].GeometricQuadric[7]);
  b[2] = -(this->ErrorQuadrics[pointIds[0]].GeometricQuadric[8] +
    this->ErrorQuadrics[pointIds[1]].GeometricQuadric[8]);
  
  vtkMath::LinearSolve3x3(A, b, x);
  cost = (A[0][0] * x[0] * x[0]) + (A[1][1] * x[1] * x[1])
    + (A[2][2] * x[2] * x[2]) + (2 * A[0][1] * x[0] * x[1])
    + (2 * A[0][2] * x[0] * x[2]) + (2 * A[1][2] * x[1] * x[2])
    + (2 * -b[0] * x[0]) + (2 * -b[1] * x[1]) + (2 * -b[2] * x[2]) +
    this->ErrorQuadrics[pointIds[0]].GeometricQuadric[9] +
    this->ErrorQuadrics[pointIds[1]].GeometricQuadric[9];
  
  return cost;
}

//----------------------------------------------------------------------------
void vtkQuadricDecimation::FindAffectedEdges(int p1Id, int p2Id,
					     vtkIdList *edges,
                                             vtkPolyData *mesh)
{
  vtkIdList *cellIds = vtkIdList::New();
  int i, j, edgeId, pointId;
  vtkGenericCell *cell = vtkGenericCell::New();
  
  edges->Reset();
  mesh->GetPointCells(p2Id, cellIds);
  
  for (i = 0; i < cellIds->GetNumberOfIds(); i++)
    {
    mesh->GetCell(cellIds->GetId(i), cell);
    for (j = 0; j < 3; j++) // assuming we have triangles
      {
      pointId = cell->GetPointId(j);
      if (pointId != p1Id && pointId != p2Id)
        {
        if ((edgeId = this->Edges->IsEdge(pointId, p2Id)) >= 0)
          {
          if (edges->IsId(edgeId) == -1)
            {
            edges->InsertNextId(edgeId);
            }
          }
        }
      }
    }
  
  cellIds->Delete();
  cell->Delete();
}

//----------------------------------------------------------------------------
int vtkQuadricDecimation::GetEdgeCellId(int p1Id, int p2Id, vtkPolyData *mesh)
{
  int *cells, i;
  unsigned short int numCells;
  
  mesh->GetPointCells(p1Id, numCells, cells);
  for (i = 0; i < numCells; i++)
    {
    if (mesh->IsPointUsedByCell(p2Id, cells[i]))
      {
      return cells[i];
      }
    }
  return -1;
}

//----------------------------------------------------------------------------
void vtkQuadricDecimation::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "Maximum Cost: " << this->MaximumCost << "\n";
}
