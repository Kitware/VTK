/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInterpolatingSubdivisionFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    This work was supported bt PHS Research Grant No. 1 P41 RR13218-01
             from the National Center for Research Resources


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
#include "vtkInterpolatingSubdivisionFilter.h"
#include "vtkEdgeTable.h"

// Construct object with number of subdivisions set to 1.
vtkInterpolatingSubdivisionFilter::vtkInterpolatingSubdivisionFilter()
{
  this->NumberOfSubdivisions = 1;
}

void vtkInterpolatingSubdivisionFilter::Execute()
{
  int numPts, numCells;
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
    vtkErrorMacro(<<"No data to interpolate!");
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

int vtkInterpolatingSubdivisionFilter::FindEdge (vtkPolyData *mesh, int cellId, int p1, int p2, vtkIntArray *edgeData, vtkIdList *cellIds)
{
 
  int edgeId = 0;
  int currentCellId = 0;
  int i;
  int numEdges;
  int tp1, tp2;
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
  int numCells = inputDS->GetNumberOfCells();
  int cellId, newId, id;
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
  vtkPolyDataToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "Number of subdivisions: " << this->NumberOfSubdivisions << endl;
}


