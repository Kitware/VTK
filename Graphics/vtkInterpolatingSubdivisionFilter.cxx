/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInterpolatingSubdivisionFilter.cxx
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
#include "vtkInterpolatingSubdivisionFilter.h"
#include "vtkEdgeTable.h"

vtkCxxRevisionMacro(vtkInterpolatingSubdivisionFilter, "1.18");

// Construct object with number of subdivisions set to 1.
vtkInterpolatingSubdivisionFilter::vtkInterpolatingSubdivisionFilter()
{
  this->NumberOfSubdivisions = 1;
}

void vtkInterpolatingSubdivisionFilter::Execute()
{
  vtkIdType numPts, numCells;
  int level;
  vtkPoints *outputPts;
  vtkCellArray *outputPolys;
  vtkPolyData *input = this->GetInput();
  vtkPolyData *output = this->GetOutput();
  vtkPointData *outputPD;
  vtkCellData *outputCD;
  vtkIntArray *edgeData;

  vtkDebugMacro(<< "Generating subdivision surface using interpolating scheme");

  
  if (input == NULL)
    {
    vtkErrorMacro(<<"Input is NULL");
    return;
    }

  numPts=input->GetNumberOfPoints();
  numCells=input->GetNumberOfCells();

  if (numPts < 1 || numCells < 1)
    {
    vtkDebugMacro(<<"No data to interpolate!");
    return;
    }

  //
  // Initialize and check input
  //

  vtkPolyData *inputDS = vtkPolyData::New();
  inputDS->CopyStructure (input);
  inputDS->GetPointData()->PassData(input->GetPointData());
  inputDS->GetCellData()->PassData(input->GetCellData());

  for (level = 0; level < this->NumberOfSubdivisions; level++)
    {
    // Generate topology  for the input dataset
    inputDS->BuildLinks();
    numCells = inputDS->GetNumberOfCells ();

    // Copy points from input. The new points will include the old points
    // and points calculated by the subdivision algorithm
    outputPts = vtkPoints::New();
    outputPts->GetData()->DeepCopy(inputDS->GetPoints()->GetData());

    // Copy pointdata structure from input
    outputPD = vtkPointData::New();
    outputPD->CopyAllocate(inputDS->GetPointData(),
                           2*inputDS->GetNumberOfPoints());

    // Copy celldata structure from input
    outputCD = vtkCellData::New();
    outputCD->CopyAllocate(inputDS->GetCellData(),4*numCells);

    // Create triangles
    outputPolys = vtkCellArray::New();
    outputPolys->Allocate(outputPolys->EstimateSize(4*numCells,3));

    // Create an array to hold new location indices
    edgeData = vtkIntArray::New();
    edgeData->SetNumberOfComponents(3);
    edgeData->SetNumberOfTuples(numCells);

    this->GenerateSubdivisionPoints (inputDS, edgeData, outputPts, outputPD);
    this->GenerateSubdivisionCells (inputDS, edgeData, outputPolys, outputCD);

    // start the next iteration with the input set to the output we just created
    edgeData->Delete();
    inputDS->Delete();
    inputDS = vtkPolyData::New();
    inputDS->SetPoints(outputPts); outputPts->Delete();
    inputDS->SetPolys(outputPolys); outputPolys->Delete();
    inputDS->GetPointData()->PassData(outputPD); outputPD->Delete();
    inputDS->GetCellData()->PassData(outputCD); outputCD->Delete();
    inputDS->Squeeze();
    } // each level

  output->SetPoints(inputDS->GetPoints());
  output->SetPolys(inputDS->GetPolys());
  output->GetPointData()->PassData(inputDS->GetPointData());
  output->GetCellData()->PassData(inputDS->GetCellData());
  inputDS->Delete();
}

int vtkInterpolatingSubdivisionFilter::FindEdge (vtkPolyData *mesh,
                                                 vtkIdType cellId,
                                                 vtkIdType p1, vtkIdType p2,
                                                 vtkIntArray *edgeData,
                                                 vtkIdList *cellIds)
{
  int edgeId = 0;
  int currentCellId = 0;
  int i;
  int numEdges;
  vtkIdType tp1, tp2;
  vtkCell *cell;

  // get all the cells that use the edge (except for cellId)
  mesh->GetCellEdgeNeighbors (cellId, p1, p2, cellIds);

  // find the edge that has the point we are looking for
  for ( i=0; i < cellIds->GetNumberOfIds(); i++)
    {
    currentCellId = cellIds->GetId(i);
    cell = mesh->GetCell(currentCellId);
    numEdges = cell->GetNumberOfEdges();
    tp1 = cell->GetPointId(2);
    tp2 = cell->GetPointId(0);
    for (edgeId=0; edgeId < numEdges; edgeId++)
      {
      if ( (tp1 == p1 && tp2 == p2) ||
           (tp2 == p1 && tp1 == p2))
        {
        break;
        }
      tp1 = tp2;
      tp2 = cell->GetPointId(edgeId + 1);
      }
    }
    // found the edge, return the stored value
    return (int) edgeData->GetComponent(currentCellId,edgeId);
}

vtkIdType vtkInterpolatingSubdivisionFilter::InterpolatePosition (
        vtkPoints *inputPts, vtkPoints *outputPts,
        vtkIdList *stencil, float *weights)
{
  float *xx, x[3];
  int i, j;

  for (j = 0; j < 3; j++)
    {
    x[j] = 0.0;
    }

  for (i = 0; i < stencil->GetNumberOfIds(); i++)
    {
    xx = inputPts->GetPoint(stencil->GetId(i));
    for (j = 0; j < 3; j++)
      {
      x[j] += xx[j] * weights[i];
      }
    }
  return outputPts->InsertNextPoint (x);
}


void vtkInterpolatingSubdivisionFilter::GenerateSubdivisionCells (vtkPolyData *inputDS, vtkIntArray *edgeData, vtkCellArray *outputPolys, vtkCellData *outputCD)
{
  vtkIdType numCells = inputDS->GetNumberOfCells();
  vtkIdType cellId, newId;
  int id;
  vtkIdType npts;
  vtkIdType *pts;
  float edgePts[3];
  vtkIdType newCellPts[3];
  vtkCellData *inputCD = inputDS->GetCellData();

  // Now create new cells from existing points and generated edge points
  for (cellId=0; cellId < numCells; cellId++)
    {
    if ( inputDS->GetCellType(cellId) != VTK_TRIANGLE )
      {
      continue;
      }
    // get the original point ids and the ids stored as cell data
    inputDS->GetCellPoints(cellId, npts, pts);
    edgeData->GetTuple(cellId, edgePts);

    id = 0;
    newCellPts[id++] = pts[0];
    newCellPts[id++] = (int) edgePts[1];
    newCellPts[id++] = (int) edgePts[0];
    newId = outputPolys->InsertNextCell (3, newCellPts);
    outputCD->CopyData (inputCD, cellId, newId);

    id = 0;
    newCellPts[id++] = (int) edgePts[1];
    newCellPts[id++] = pts[1];
    newCellPts[id++] = (int) edgePts[2];
    newId = outputPolys->InsertNextCell (3, newCellPts);
    outputCD->CopyData (inputCD, cellId, newId);

    id = 0;
    newCellPts[id++] = (int) edgePts[2];
    newCellPts[id++] = pts[2];
    newCellPts[id++] = (int) edgePts[0];
    newId = outputPolys->InsertNextCell (3, newCellPts);
    outputCD->CopyData (inputCD, cellId, newId);

    id = 0;
    newCellPts[id++] = (int) edgePts[1];
    newCellPts[id++] = (int) edgePts[2];
    newCellPts[id++] = (int) edgePts[0];
    newId = outputPolys->InsertNextCell (3, newCellPts);
    outputCD->CopyData (inputCD, cellId, newId);
    }
}

void vtkInterpolatingSubdivisionFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Number of subdivisions: " << this->NumberOfSubdivisions << endl;
}


