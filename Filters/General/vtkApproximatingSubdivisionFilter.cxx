/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkApproximatingSubdivisionFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkApproximatingSubdivisionFilter.h"

#include "vtkCell.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkEdgeTable.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedCharArray.h"


// Construct object with number of subdivisions set to 1.
vtkApproximatingSubdivisionFilter::vtkApproximatingSubdivisionFilter()
{
}

int vtkApproximatingSubdivisionFilter::RequestData(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  if (!this->Superclass::RequestData(request, inputVector,
                                     outputVector))
  {
    return 0;
  }

  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPolyData *input = vtkPolyData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType numCells, numPts;
  int level;
  vtkPoints *outputPts;

  vtkCellArray *outputPolys;
  vtkPointData *outputPD;
  vtkCellData *outputCD;
  vtkIntArray *edgeData;

  vtkDebugMacro(<< "Generating subdivision surface using approximating scheme");
  numPts=input->GetNumberOfPoints();
  numCells=input->GetNumberOfCells();

  //
  // Initialize and check input
  //

  vtkPolyData *inputDS = vtkPolyData::New();
  inputDS->CopyStructure (input);
  inputDS->CopyAttributes(input);

  int abort=0;
  for (level = 0; level < this->NumberOfSubdivisions && !abort; level++)
  {
    this->UpdateProgress(static_cast<double>(level+1)/
                                                  this->NumberOfSubdivisions);
    abort = this->GetAbortExecute();

    // Generate topology  for the input dataset
    inputDS->BuildLinks();

    numCells = inputDS->GetNumberOfCells ();
    numPts = inputDS->GetNumberOfPoints();

    // The points for the subdivisions will
    // include even points (computed from old points) and
    // odd points (inserted on edges)
    outputPts = vtkPoints::New();
    outputPts->Allocate (numPts);

    // Copy pointdata structure from input
    outputPD = vtkPointData::New();
    outputPD->CopyAllocate(inputDS->GetPointData(),2*inputDS->GetNumberOfPoints());

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

    if (this->GenerateSubdivisionPoints (inputDS, edgeData, outputPts, outputPD) == 0)
    {
      outputPts->Delete();
      outputPD->Delete();
      outputCD->Delete();
      outputPolys->Delete();
      inputDS->Delete();
      edgeData->Delete();
      vtkErrorMacro("Subdivision failed.");
      return 0;
    }
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
  output->CopyAttributes(inputDS);

  inputDS->Delete();

  return 1;
}

int vtkApproximatingSubdivisionFilter::FindEdge (vtkPolyData *mesh,
                                                 vtkIdType cellId,
                                                 vtkIdType p1, vtkIdType p2,
                                                 vtkIntArray *edgeData,
                                                 vtkIdList *cellIds)
{

  int edgeId = 0;
  vtkIdType currentCellId = 0;
  vtkIdType i;
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
  return static_cast<int>(edgeData->GetComponent(currentCellId,edgeId));
}

vtkIdType vtkApproximatingSubdivisionFilter::InterpolatePosition (
        vtkPoints *inputPts, vtkPoints *outputPts,
        vtkIdList *stencil, double *weights)
{
  double xx[3], x[3];
  vtkIdType i;
  int j;

  for (j = 0; j < 3; j++)
  {
    x[j] = 0.0;
  }

  for (i = 0; i < stencil->GetNumberOfIds(); i++)
  {
    inputPts->GetPoint(stencil->GetId(i), xx);
    for (j = 0; j < 3; j++)
    {
      x[j] += xx[j] * weights[i];
    }
  }
  return outputPts->InsertNextPoint (x);
}


void vtkApproximatingSubdivisionFilter::GenerateSubdivisionCells (
  vtkPolyData *inputDS, vtkIntArray *edgeData, vtkCellArray *outputPolys,
  vtkCellData *outputCD)
{
  vtkIdType numCells = inputDS->GetNumberOfCells();
  vtkIdType cellId, newId, id;
  vtkIdType npts;
  vtkIdType *pts;
  double edgePts[3];
  vtkIdType newCellPts[3];
  vtkCellData *inputCD = inputDS->GetCellData();

  // Now create new cells from existing points and generated edge points
  for (cellId=0; cellId < numCells; cellId++)
  {
    if ( inputDS->GetCellType(cellId) != VTK_TRIANGLE )
    {
      continue;
    }
    // get the original point ids and the ids stored as edge data
    inputDS->GetCellPoints(cellId, npts, pts);
    edgeData->GetTuple(cellId, edgePts);

    id = 0;
    newCellPts[id++] = pts[0];
    newCellPts[id++] = static_cast<int>(edgePts[1]);
    newCellPts[id++] = static_cast<int>(edgePts[0]);
    newId = outputPolys->InsertNextCell (3, newCellPts);
    outputCD->CopyData (inputCD, cellId, newId);

    id = 0;
    newCellPts[id++] = static_cast<int>(edgePts[1]);
    newCellPts[id++] = pts[1];
    newCellPts[id++] = static_cast<int>(edgePts[2]);
    newId = outputPolys->InsertNextCell (3, newCellPts);
    outputCD->CopyData (inputCD, cellId, newId);

    id = 0;
    newCellPts[id++] = static_cast<int>(edgePts[2]);
    newCellPts[id++] = pts[2];
    newCellPts[id++] = static_cast<int>(edgePts[0]);
    newId = outputPolys->InsertNextCell (3, newCellPts);
    outputCD->CopyData (inputCD, cellId, newId);

    id = 0;
    newCellPts[id++] = static_cast<int>(edgePts[1]);
    newCellPts[id++] = static_cast<int>(edgePts[2]);
    newCellPts[id++] = static_cast<int>(edgePts[0]);
    newId = outputPolys->InsertNextCell (3, newCellPts);
    outputCD->CopyData (inputCD, cellId, newId);
  }
}

void vtkApproximatingSubdivisionFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
