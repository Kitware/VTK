/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuadricDecimation.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
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
#include "vtkFloatArray.h"

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
  this->MaximumCollapsedEdges = -1;
  this->NumberOfCollapsedEdges = 0;
  this->NumberOfComponents = 0;
  this->Mesh = vtkPolyData::New();

  vtkPolyData *testOutput = vtkPolyData::New();
  this->SetNthOutput(1, testOutput);
  testOutput->Delete();
}

//----------------------------------------------------------------------------
vtkQuadricDecimation::~vtkQuadricDecimation()
{
  this->Edges->Delete();
  this->EdgeCosts->Delete();
  this->EndPoint1List->Delete();
  this->EndPoint2List->Delete();
  this->Mesh->Delete();
}

//----------------------------------------------------------------------------
void vtkQuadricDecimation::Execute()
{
  vtkPolyData *input = this->GetInput();
  vtkPolyData *output = this->GetOutput();
  vtkPolyData *testOutput = this->GetTestOutput();
  vtkPoints *testPoints = vtkPoints::New();
  vtkCellArray *testEdges = vtkCellArray::New();
  vtkIdType testId0, testId1;
  vtkCellArray *triangles = input->GetPolys();
  vtkIdType numTris = triangles->GetNumberOfCells();
  vtkIdType numPts = input->GetNumberOfPoints();
  vtkIdType edgeId, i, newCellPts[3];
  int j;
  vtkIdType *cellPts, numCellPts;
  float cost, x[3];
  vtkPoints *targetPoints = vtkPoints::New();
  vtkPointData *targetPointData = vtkPointData::New();
  vtkPointData *outPD = output->GetPointData();
  vtkIdList *changedEdges = vtkIdList::New();
  vtkIdType collapsedCell;
  vtkIdList *collapsedCells = vtkIdList::New();
  vtkIdType endPtIds[2], edge[2];
  vtkIdList *changedCells = vtkIdList::New();
  vtkIdList *newEdges = vtkIdList::New();
  vtkIdList *outputCellList = vtkIdList::New();
  vtkCellArray *outputPolys = vtkCellArray::New();
  vtkGenericCell *cell = vtkGenericCell::New();
  vtkIdType cellId;

  if (input->GetPolys() == NULL)
    {
    vtkErrorMacro("no triangles to decimate");
    return;
    }

  this->ErrorQuadrics = new VTK_ERROR_QUADRIC[input->GetNumberOfPoints()];
  this->Mesh->DeepCopy(input);
  input->BuildLinks();
  this->Mesh->BuildLinks();
  if (this->MaximumCollapsedEdges < 0)
    {
    this->MaximumCollapsedEdges = input->GetNumberOfCells() * 3;
    }
  
  output->SetPolys(outputPolys);
  this->Edges->InitEdgeInsertion(numPts, 1); // storing edge id as attribute
  this->EdgeCosts->Allocate(input->GetPolys()->GetNumberOfCells() * 3);
  targetPointData->CopyAllocate(input->GetPointData(),
                                input->GetNumberOfPoints());
  outPD->CopyAllocate(input->GetPointData(), input->GetNumberOfPoints());
  triangles->InitTraversal();
  this->GetAttributeComponents();
  for (i = 0; i < numTris; i++)
    {
    triangles->GetNextCell(numCellPts, cellPts);
    outputCellList->InsertNextId(i);
    for (j = 0; j < 3; j++)
      {
      if (this->Edges->IsEdge(cellPts[j], cellPts[(j+1)%3]) == -1)
        {
        // If this edge has not been processed, get an id for it, add it to
        // the edge list (Edges), and add its endpoints to the EndPoint1List
        // and EndPoint2List (the 2 endpoints to different lists).
        edgeId = this->Edges->GetNumberOfEdges();
        this->Edges->InsertEdge(cellPts[j], cellPts[(j+1)%3], edgeId);
        this->EndPoint1List->InsertId(edgeId, cellPts[j]);
        this->EndPoint2List->InsertId(edgeId, cellPts[(j+1)%3]);
        }
      }
    }

  // Compute a quadric matrix for each of the initial points.
  for (i = 0; i < numPts; i++)
    {
    this->ComputeQuadric(i);
    }
  
  // Compute the cost of and target point for collapsing each edge.
  for (i = 0; i < this->Edges->GetNumberOfEdges(); i++)
    {
    cost = this->ComputeCost(i, x, targetPointData);
//    vtkDebugMacro("cost: " << cost << ", point (" << x[0] << ", " << x[1]
//                  << ", " << x[2] << " )");
    this->EdgeCosts->Insert(cost, i);
    targetPoints->InsertPoint(i, x);
    }
  
  this->NumberOfCollapsedEdges = 0;
  // Get id of edge with minimum cost to collapse.
  edgeId = this->EdgeCosts->Pop(cost);
  while (edgeId >= 0 && cost < this->MaximumCost &&
         this->NumberOfCollapsedEdges < this->MaximumCollapsedEdges)
    {
    this->NumberOfCollapsedEdges++;
    vtkDebugMacro("cost: " << cost);
    endPtIds[0] = this->EndPoint1List->GetId(edgeId);
    endPtIds[1] = this->EndPoint2List->GetId(edgeId);
    vtkDebugMacro("pt. 0: " << this->Mesh->GetPoints()->GetPoint(endPtIds[0])[0] << " " << this->Mesh->GetPoints()->GetPoint(endPtIds[0])[1] << " " << this->Mesh->GetPoints()->GetPoint(endPtIds[0])[2]);
    vtkDebugMacro("pt. 1: " << this->Mesh->GetPoints()->GetPoint(endPtIds[1])[0] << " " << this->Mesh->GetPoints()->GetPoint(endPtIds[1])[1] << " " << this->Mesh->GetPoints()->GetPoint(endPtIds[1])[2]);
    vtkDebugMacro("***target pt.: " << targetPoints->GetPoint(edgeId)[0] << " " << targetPoints->GetPoint(edgeId)[1] << " " << targetPoints->GetPoint(edgeId)[2]);
    changedEdges->Reset();
    changedCells->Reset();
    collapsedCells->Reset();
    newEdges->Reset();

    // Set the new coordinates of point0.
    this->Mesh->GetPoints()->SetPoint(endPtIds[0],
                                      targetPoints->GetPoint(edgeId));
    // Set the new point data for point0.
    this->Mesh->GetPointData()->CopyData(targetPointData, edgeId,
                                         endPtIds[0]);

    // Merge the quadrics of the two points.
    this->AddQuadric(endPtIds[1], endPtIds[0]);

    // Find all edges with either of these 2 endpoints.
    this->FindAffectedEdges(endPtIds[0], endPtIds[1], changedEdges);
    // Reset the endpoints for these edges to reflect the new point from the
    // collapsed edge.
    // Add these new edges to the edge table.
    // Remove the the changed edges from the priority queue.

    testPoints->Reset();
    testEdges->Reset();
    for (i = 0; i < changedEdges->GetNumberOfIds(); i++)
      {
      edge[0] = this->EndPoint1List->GetId(changedEdges->GetId(i));
      edge[1] = this->EndPoint2List->GetId(changedEdges->GetId(i));

      testId0 = testPoints->InsertNextPoint(this->Mesh->GetPoints()->GetPoint(edge[0]));
      testId1 = testPoints->InsertNextPoint(this->Mesh->GetPoints()->GetPoint(edge[1]));
      testEdges->InsertNextCell(2);
      testEdges->InsertCellPoint(testId0);
      testEdges->InsertCellPoint(testId1);

      // Remove all affected edges from the priority que. 
      // This does not include collapsed edge.
      this->EdgeCosts->DeleteId(changedEdges->GetId(i));

      // Determine the new set of edges
      if (edge[0] == endPtIds[1])
        {
        if (this->Edges->IsEdge(edge[1], endPtIds[0]) == -1)
          { // The edge will be completely new, add it.
          edgeId = this->Edges->GetNumberOfEdges();
          this->Edges->InsertEdge(edge[1], endPtIds[0], edgeId);
          this->EndPoint1List->InsertId(edgeId, edge[1]);
          this->EndPoint2List->InsertId(edgeId, endPtIds[0]);
          newEdges->InsertNextId(edgeId);
          // Compute cost (target point/data) and add to priority cue.
          cost = this->ComputeCost(edgeId, x, targetPointData);
          this->EdgeCosts->Insert(cost, edgeId);
          targetPoints->InsertPoint(edgeId, x);
          }
        }
      else if (edge[1] == endPtIds[1])
        { // The edge will be completely new, add it.
        if (this->Edges->IsEdge(edge[0], endPtIds[0]) == -1)
          {
          edgeId = this->Edges->GetNumberOfEdges();
          this->Edges->InsertEdge(edge[0], endPtIds[0], edgeId);
          this->EndPoint1List->InsertId(edgeId, edge[0]);
          this->EndPoint2List->InsertId(edgeId, endPtIds[0]);
          newEdges->InsertNextId(edgeId);
          // Compute cost (target point/data) and add to priority cue.
          cost = this->ComputeCost(edgeId, x, targetPointData);
          this->EdgeCosts->Insert(cost, edgeId);
          targetPoints->InsertPoint(edgeId, x);
          }
        }
      else
        { // This edge already has one point as the merged point.
        // Compute cost (target point/data) and add to priority cue.
        cost = this->ComputeCost(edgeId, x, targetPointData);
        this->EdgeCosts->Insert(cost, edgeId);
        targetPoints->InsertPoint(edgeId, x);
        }
      }
    
    // Update the output triangles.
    collapsedCell = this->GetEdgeCellId(endPtIds[0], endPtIds[1]);
    this->Mesh->GetCellEdgeNeighbors(collapsedCell, endPtIds[0], endPtIds[1],
                                     collapsedCells);
    this->Mesh->RemoveCellReference(collapsedCell);
    outputCellList->DeleteId(collapsedCell);
    for (i = 0; i < collapsedCells->GetNumberOfIds(); i++)
      {
      this->Mesh->RemoveCellReference(collapsedCells->GetId(i));
      outputCellList->DeleteId(collapsedCells->GetId(i));
      }
    this->Mesh->GetPointCells(endPtIds[1], changedCells);
    for (i = 0; i < changedCells->GetNumberOfIds(); i++)
      {
      cellId = changedCells->GetId(i);
      this->Mesh->GetCell(cellId, cell);
      newCellPts[0] = cell->GetPointId(0);
      newCellPts[1] = cell->GetPointId(1);
      newCellPts[2] = cell->GetPointId(2);
      // making sure we don't already have the triangle we're about to
      // change this one to
      if (newCellPts[0] == endPtIds[1])
        {
        if (this->Mesh->IsTriangle(endPtIds[0], newCellPts[1], newCellPts[2]))
          {
          this->Mesh->RemoveCellReference(cellId);
          outputCellList->DeleteId(cellId);
          }
        else
          {
          this->Mesh->RemoveReferenceToCell(endPtIds[1], cellId);
          this->Mesh->ResizeCellList(endPtIds[0], 1);
          this->Mesh->AddReferenceToCell(endPtIds[0], cellId);
          this->Mesh->ReplaceCellPoint(cellId, endPtIds[1], endPtIds[0]);
          }
        }
      else if (newCellPts[1] == endPtIds[1])
        {
        if (this->Mesh->IsTriangle(newCellPts[0], endPtIds[0], newCellPts[2]))
          {
          this->Mesh->RemoveCellReference(cellId);
          outputCellList->DeleteId(cellId);
          }
        else
          {
          this->Mesh->RemoveReferenceToCell(endPtIds[1], cellId);
          this->Mesh->ResizeCellList(endPtIds[0], 1);
          this->Mesh->AddReferenceToCell(endPtIds[0], cellId);
          this->Mesh->ReplaceCellPoint(cellId, endPtIds[1], endPtIds[0]);
          }
        }
      else
        {
        if (this->Mesh->IsTriangle(newCellPts[0], newCellPts[1], endPtIds[0]))
          {
          this->Mesh->RemoveCellReference(cellId);
          outputCellList->DeleteId(cellId);
          }
        else
          {
          this->Mesh->RemoveReferenceToCell(endPtIds[1], cellId);
          this->Mesh->ResizeCellList(endPtIds[0], 1);
          this->Mesh->AddReferenceToCell(endPtIds[0], cellId);
          this->Mesh->ReplaceCellPoint(cellId, endPtIds[1], endPtIds[0]);
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
  
  output->CopyCells(this->Mesh, outputCellList);
  
  input->DeleteLinks();
  this->Mesh->DeleteLinks();
  changedEdges->Delete();
  collapsedCells->Delete();
  changedCells->Delete();
  newEdges->Delete();
  targetPoints->Delete();
  targetPointData->Delete();
  outputCellList->Delete();
  outputPolys->Delete();
  cell->Delete();
  for (i = 0; i < input->GetNumberOfPoints(); i++)
    {
    delete [] this->ErrorQuadrics[i].Quadric;
    }
  delete [] this->ErrorQuadrics;

  testOutput->SetPoints(testPoints);
  testOutput->SetLines(testEdges);
  testPoints->Delete();
  testEdges->Delete();
}

//----------------------------------------------------------------------------
void vtkQuadricDecimation::ComputeQuadric(vtkIdType pointId)
{
  vtkIdList *cellIds = vtkIdList::New();
  vtkPolyData *input = this->GetInput();
  int i, j;
  float n[3], d;
  vtkPoints *cellPts;
  vtkIdList *cellPtIdList;
  vtkIdType cellPtIds[3];
  float point0[3], point1[3], point2[3];
  float tempP1[3], tempP2[3];
  float triArea2;
  int scalars, vectors, normals, tcoords, tensors, fieldData;
  vtkPointData *pd = input->GetPointData();
  vtkFloatArray *faceNormals = vtkFloatArray::New();
  double *A[4], A0[4], A1[4], A2[4], A3[4], b[4];

  faceNormals->SetNumberOfComponents(3);
  
  this->ErrorQuadrics[pointId].Quadric =
    new float[11 + 4 * this->NumberOfComponents];
  for (i = 0; i < 11 + 4 * this->NumberOfComponents; i++)
    {
    this->ErrorQuadrics[pointId].Quadric[i] = 0.0;
    }
  
  input->GetPointCells(pointId, cellIds);

  // geometric part of quadric
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
    triArea2 = vtkMath::Normalize(n);
    triArea2 = triArea2 * triArea2 * 0.25;
    faceNormals->InsertTuple(cellIds->GetId(i), n);
    d = -vtkMath::Dot(n, point0);

    this->ErrorQuadrics[pointId].Quadric[0] += n[0] * n[0] * triArea2;
    this->ErrorQuadrics[pointId].Quadric[1] += n[1] * n[1] * triArea2;
    this->ErrorQuadrics[pointId].Quadric[2] += n[2] * n[2] * triArea2;
    this->ErrorQuadrics[pointId].Quadric[3] += n[0] * n[1] * triArea2;
    this->ErrorQuadrics[pointId].Quadric[4] += n[1] * n[2] * triArea2;
    this->ErrorQuadrics[pointId].Quadric[5] += n[0] * n[2] * triArea2;
    this->ErrorQuadrics[pointId].Quadric[6] += d * n[0] * triArea2;
    this->ErrorQuadrics[pointId].Quadric[7] += d * n[1] * triArea2;
    this->ErrorQuadrics[pointId].Quadric[8] += d * n[2] * triArea2;
    this->ErrorQuadrics[pointId].Quadric[9] += d * d * triArea2;
    }
  this->ErrorQuadrics[pointId].Quadric[10] = cellIds->GetNumberOfIds();
  
  // attribute part of quadric
  A[0] = A0;
  A[1] = A1;
  A[2] = A2;
  A[3] = A3;
  for (i = 0; i < cellIds->GetNumberOfIds(); i++)
    {
    scalars = vectors = normals = tcoords = tensors = fieldData = 0;
    cellPts = input->GetCell(cellIds->GetId(i))->GetPoints();
    cellPts->GetPoint(0, point0);
    cellPts->GetPoint(1, point1);
    cellPts->GetPoint(2, point2);
    cellPtIdList = input->GetCell(cellIds->GetId(i))->GetPointIds();
    cellPtIds[0] = cellPtIdList->GetId(0);
    cellPtIds[1] = cellPtIdList->GetId(1);
    cellPtIds[2] = cellPtIdList->GetId(2);
    faceNormals->GetTuple(cellIds->GetId(i), n);
    for (j = 0; j < 3; j++)
      {
      A[0][j] = point0[j];
      A[1][j] = point1[j];
      A[2][j] = point2[j];
      A[3][j] = n[j];
      }
    A[0][3] = A[1][3] = A[2][3] = 1;
    A[3][3] = 0;
    for (j = 0; j < this->NumberOfComponents; j++)
      {
      b[3] = 0;
      if (scalars < this->AttributeComponents[0])
        {
        pd->GetScalars()->SetActiveComponent(scalars);
        b[0] = pd->GetScalars()->GetScalar(cellPtIds[0]);
        b[1] = pd->GetScalars()->GetScalar(cellPtIds[1]);
        b[2] = pd->GetScalars()->GetScalar(cellPtIds[2]);
        scalars++;
        }
      else if (vectors < this->AttributeComponents[1])
        {
        b[0] = pd->GetVectors()->GetVector(cellPtIds[0])[vectors];
        b[1] = pd->GetVectors()->GetVector(cellPtIds[1])[vectors];
        b[2] = pd->GetVectors()->GetVector(cellPtIds[2])[vectors];
        vectors++;
        }
      else if (normals < this->AttributeComponents[2])
        {
        b[0] = pd->GetNormals()->GetNormal(cellPtIds[0])[normals];
        b[1] = pd->GetNormals()->GetNormal(cellPtIds[1])[normals];
        b[2] = pd->GetNormals()->GetNormal(cellPtIds[2])[normals];
        normals++;
        }
      else if (tcoords < this->AttributeComponents[3])
        {
        b[0] = pd->GetTCoords()->GetTCoord(cellPtIds[0])[tcoords];
        b[1] = pd->GetTCoords()->GetTCoord(cellPtIds[1])[tcoords];
        b[2] = pd->GetTCoords()->GetTCoord(cellPtIds[2])[tcoords];
        tcoords++;
        }
      else if (tensors < this->AttributeComponents[4])
        {
        b[0] =
          pd->GetTensors()->GetTensor(cellPtIds[0])->GetComponent(tensors/3,
                                                                  tensors%3);
        b[1] =
          pd->GetTensors()->GetTensor(cellPtIds[1])->GetComponent(tensors/3,
                                                                  tensors%3);
        b[2] =
          pd->GetTensors()->GetTensor(cellPtIds[2])->GetComponent(tensors/3,
                                                                  tensors%3);
        tensors++;
        }
      else if (fieldData < this->AttributeComponents[5])
        {
        b[0] = pd->GetFieldData()->GetComponent(cellPtIds[0], fieldData);
        b[1] = pd->GetFieldData()->GetComponent(cellPtIds[1], fieldData);
        b[2] = pd->GetFieldData()->GetComponent(cellPtIds[2], fieldData);
        fieldData++;
        }
      vtkMath::SolveLinearSystem(A, b, 4);
      this->ErrorQuadrics[pointId].Quadric[0] += b[0] * b[0];
      this->ErrorQuadrics[pointId].Quadric[1] += b[1] * b[1];
      this->ErrorQuadrics[pointId].Quadric[2] += b[2] * b[2];
      this->ErrorQuadrics[pointId].Quadric[3] += b[0] * b[1];
      this->ErrorQuadrics[pointId].Quadric[4] += b[1] * b[2];
      this->ErrorQuadrics[pointId].Quadric[5] += b[0] * b[2];
      this->ErrorQuadrics[pointId].Quadric[6] += b[3] * b[0];
      this->ErrorQuadrics[pointId].Quadric[7] += b[3] * b[1];
      this->ErrorQuadrics[pointId].Quadric[8] += b[3] * b[2];
      this->ErrorQuadrics[pointId].Quadric[9] += b[3] * b[3];
      this->ErrorQuadrics[pointId].Quadric[11 + 4*j] += -b[0];
      this->ErrorQuadrics[pointId].Quadric[11 + 4*j+1] += -b[1];
      this->ErrorQuadrics[pointId].Quadric[11 + 4*j+2] += -b[2];
      this->ErrorQuadrics[pointId].Quadric[11 + 4*j+3] += -b[3];
      }
    }
  cellIds->Delete();
  faceNormals->Delete();
}

//----------------------------------------------------------------------------
void vtkQuadricDecimation::AddQuadric(vtkIdType oldPtId, vtkIdType newPtId)
{
  int i;
  
  for (i = 0; i < 11 + 4*this->NumberOfComponents; i++)
    {
    this->ErrorQuadrics[newPtId].Quadric[i] +=
      this->ErrorQuadrics[oldPtId].Quadric[i];
    }
}

//----------------------------------------------------------------------------
float vtkQuadricDecimation::ComputeCost(vtkIdType edgeId, float x[3],
                                        vtkPointData *pd)
{
  float C[3][3], BBT[3][3], A[3][3], b1[3], Bb2[3], b[3];
  vtkIdType pointIds[2];
  float cost = 0.0;
  float *quad = new float[11 + 4 * this->NumberOfComponents];
  int i, j;
  float *newPoint = new float[3+this->NumberOfComponents];
  float *BTpmin = new float[this->NumberOfComponents];
  int components = 0;
  float normal[3], tcoord[3];
  
  pointIds[0] = this->EndPoint1List->GetId(edgeId);
  pointIds[1] = this->EndPoint2List->GetId(edgeId);
  
  for (i = 0; i < 11 + 4 * this->NumberOfComponents; i++)
    {
    quad[i] = this->ErrorQuadrics[pointIds[0]].Quadric[i] +
      this->ErrorQuadrics[pointIds[1]].Quadric[i];
    }
  
  C[0][0] = quad[0];
  C[1][1] = quad[1];
  C[2][2] = quad[2];
  C[0][1] = C[1][0] = quad[3];
  C[1][2] = C[2][1] = quad[4];
  C[0][2] = C[2][0] = quad[5];
  
  b1[0] = -quad[6];
  b1[1] = -quad[7];
  b1[2] = -quad[8];
  
  for (i = 0; i < 3; i++)
    {
    Bb2[i] = 0.0;
    for (j = 0; j < 3; j++)
      {
      BBT[i][j] = 0.0;
      }
    }
  
  for (i = 0; i < this->NumberOfComponents; i++)
    {
    BBT[0][0] += quad[11 + 4*i] * quad[11 + 4*i];
    BBT[0][1] += quad[11 + 4*i] * quad[11 + 4*i+1];
    BBT[0][2] += quad[11 + 4*i] * quad[11 + 4*i+2];
    BBT[1][1] += quad[11 + 4*i+1] * quad[11 + 4*i+1];
    BBT[1][2] += quad[11 + 4*i+1] * quad[11 + 4*i+2];
    BBT[2][2] += quad[11 + 4*i+2] * quad[11 + 4*i+2];
    Bb2[0] += quad[11 + 4*i] * -quad[11 + 4*i+3];
    Bb2[1] += quad[11 + 4*i+1] * -quad[11 + 4*i+3];
    Bb2[2] += quad[11 + 4*i+2] * -quad[11 + 4*i+3];
    }
  BBT[1][0] = BBT[0][1];
  BBT[2][0] = BBT[0][2];
  BBT[2][1] = BBT[1][2];
  
  for (i = 0; i < 3; i++)
    {
    b[i] = b1[i] - (1/quad[10])*Bb2[i];
    for (j = 0; j < 3; j++)
      {
      A[i][j] = C[i][j] - (1/quad[10])*BBT[i][j];
      }
    }
  
  vtkMath::LinearSolve3x3(A, b, x);
  newPoint[0] = x[0];
  newPoint[1] = x[1];
  newPoint[2] = x[2];
  
  for (i = 0; i < this->NumberOfComponents; i++)
    {
    BTpmin[i] = 0.0;
    }
  
  for (i = 0; i < this->NumberOfComponents; i++)
    {
    BTpmin[i] = quad[11 + 4*i] * x[0] + quad[11 + 4*i+1] * x[1] +
      quad[11 + 4*i+2] * x[2];
    newPoint[i+3] = (1/quad[10])*(-quad[11 + 4*i+3] - BTpmin[i]);
    }
  
  // adding point data for the new point
  //
  // scalars
  if (this->AttributeComponents[0] > 0)
    {
    for (i = 0; i < this->AttributeComponents[0]; i++)
      {
      pd->GetScalars()->SetActiveComponent(components);
      pd->GetScalars()->InsertScalar(edgeId, newPoint[3 + components]);
      components++;
      }
    }
  // vectors
  if (this->AttributeComponents[1] > 0)
    {
    pd->GetVectors()->InsertVector(edgeId, newPoint[3 + components],
                                   newPoint[3 + components+1],
                                   newPoint[3 + components+2]);
    components += 3;
    }
  // normals
  if (this->AttributeComponents[2] > 0)
    {
    normal[0] = newPoint[3 + components];
    normal[1] = newPoint[3 + components+1];
    normal[2] = newPoint[3 + components+2];
    vtkMath::Normalize(normal);
    newPoint[3 + components] = normal[0];
    newPoint[3 + components+1] = normal[1];
    newPoint[3 + components+2] = normal[2];
    pd->GetNormals()->InsertNormal(edgeId, normal);
    components += 3;
    }
  // texture coordinates
  if (this->AttributeComponents[3] > 0)
    {
    for (i = 0; i < this->AttributeComponents[3]; i++)
      {
      tcoord[i] = newPoint[3 + components];
      components++;
      }
    pd->GetTCoords()->InsertTCoord(edgeId, tcoord);
    }
  // tensors
  if (this->AttributeComponents[4] > 0)
    {
    pd->GetTensors()->InsertTensor(edgeId, newPoint[3 + components],
                                   newPoint[3 + components+1],
                                   newPoint[3 + components+2],
                                   newPoint[3 + components+3],
                                   newPoint[3 + components+4],
                                   newPoint[3 + components+5],
                                   newPoint[3 + components+6],
                                   newPoint[3 + components+7],
                                   newPoint[3 + components+8]);
    components += 9;
    }
  // field data
  if (this->AttributeComponents[5] > 0)
    {
    for (i = 0; i < this->AttributeComponents[5]; i++)
      {
      pd->GetFieldData()->InsertComponent(edgeId, this->AttributeComponents[5]
                                          - (this->NumberOfComponents -
                                             components),
                                          newPoint[3 + components]);
      components++;
      }
    }
  
  for (i = 0; i < 3 + this->NumberOfComponents; i++)
    {
    for (j = i; j < 3 + this->NumberOfComponents + 1; j++)
      {
      if (i == j)
        {
        if (i < 3)
          {
          cost += quad[i] * newPoint[i] * newPoint[j];
          }
        else
          {
          cost += quad[10] * newPoint[i] * newPoint[j];
          }
        }
      else
        {
        if (i == 0 && j == 1)
          {
          cost += 2 * quad[3] * newPoint[i] * newPoint[j];
          }
        else if (i == 0 && j == 2)
          {
          cost += 2 * quad[5] * newPoint[i] * newPoint[j];
          }
        else if (i == 1 && j == 2)
          {
          cost += 2 * quad[4] * newPoint[i] * newPoint[j];
          }
        else if (j >= 3 && j < 3 + this->NumberOfComponents && i < 3)
          {
          cost += 2 * quad[11 + 4*(j-3) + i] * newPoint[i] * newPoint[j];
          }
        else if (j == 3 + this->NumberOfComponents)
          {
          if (i < 3)
            {
            cost += 2 * quad[6+i] * newPoint[i];
            }
          else
            {
            cost += 2 * quad[11 + 4*(i-3) + 3] * newPoint[i];
            }
          }
        }
      }
    }
  
  cost += quad[9];
  
  delete [] quad;
  delete [] newPoint;
  delete [] BTpmin;
  
  return cost;
}

//----------------------------------------------------------------------------
void vtkQuadricDecimation::FindAffectedEdges(vtkIdType p1Id, vtkIdType p2Id,
					     vtkIdList *edges)
{
  vtkIdList *cellIds = vtkIdList::New();
  vtkIdType edgeId, pointId;
  int i, j;
  vtkGenericCell *cell = vtkGenericCell::New();
  
  edges->Reset();
  this->Mesh->GetPointCells(p2Id, cellIds);
  
  for (i = 0; i < cellIds->GetNumberOfIds(); i++)
    {
    this->Mesh->GetCell(cellIds->GetId(i), cell);
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
 
  this->Mesh->GetPointCells(p1Id, cellIds);
  for (i = 0; i < cellIds->GetNumberOfIds(); i++)
    {
    this->Mesh->GetCell(cellIds->GetId(i), cell);
    for (j = 0; j < 3; j++) // assuming we have triangles
      {
      pointId = cell->GetPointId(j);
      if (pointId != p1Id && pointId != p2Id)
        {
        if ((edgeId = this->Edges->IsEdge(pointId, p1Id)) >= 0)
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
vtkIdType vtkQuadricDecimation::GetEdgeCellId(vtkIdType p1Id, vtkIdType p2Id)
{
  int i;
  vtkIdType *cells;
  unsigned short int numCells;
  
  this->Mesh->GetPointCells(p1Id, numCells, cells);
  for (i = 0; i < numCells; i++)
    {
    if (this->Mesh->IsPointUsedByCell(p2Id, cells[i]))
      {
      return cells[i];
      }
    }
  return -1;
}

//----------------------------------------------------------------------------
// Determine which point data attributes the data have and the number of
// components per attribute.  Fill the AttributeComponents array accordingly.
//----------------------------------------------------------------------------
void vtkQuadricDecimation::GetAttributeComponents()
{
  int i;
  vtkPointData *pd = this->GetInput()->GetPointData();
  this->NumberOfComponents = 0;
  
  for (i = 0; i < 6; i++)
    {
    this->AttributeComponents[i] = 0;
    }

  // Attributes are not working yet.
  if (0 && pd->GetScalars() != NULL)
    {
    this->AttributeComponents[0] = pd->GetScalars()->GetNumberOfComponents();
    this->NumberOfComponents += this->AttributeComponents[0];
    vtkDebugMacro("scalars");
    }
  if (0 && pd->GetVectors() != NULL)
    {
    this->AttributeComponents[1] = 3;
    this->NumberOfComponents += 3;
    vtkDebugMacro("vectors");
    }
  if (0 && pd->GetNormals() != NULL)
    {
    this->AttributeComponents[2] = 3;
    this->NumberOfComponents += 3;
    vtkDebugMacro("normals");
    }
  if (0 && pd->GetTCoords() != NULL)
    {
    this->AttributeComponents[3] = pd->GetTCoords()->GetNumberOfComponents();
    this->NumberOfComponents += this->AttributeComponents[3];
    vtkDebugMacro("tcoords");
    }
  if (0 && pd->GetTensors() != NULL)
    {
    this->AttributeComponents[4] = 9;
    this->NumberOfComponents += 9;
    vtkDebugMacro("tensors");
    }
  if (0 && pd->GetFieldData() != NULL)
    {
    this->AttributeComponents[5] = pd->GetFieldData()->GetNumberOfComponents();
    this->NumberOfComponents += this->AttributeComponents[5];
    vtkDebugMacro("field data");
    }
  vtkDebugMacro("Num. components: " << this->NumberOfComponents);
}

//----------------------------------------------------------------------------
void vtkQuadricDecimation::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "Maximum Cost: " << this->MaximumCost << "\n";
  os << indent << "MaximumCollapsedEdges: " << this->MaximumCollapsedEdges << "\n";
 }
