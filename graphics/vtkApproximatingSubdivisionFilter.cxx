/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkApproximatingSubdivisionFilter.cxx
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
#include "vtkApproximatingSubdivisionFilter.h"
#include "vtkEdgeTable.h"

// Construct object with number of subdivisions set to 1.
vtkApproximatingSubdivisionFilter::vtkApproximatingSubdivisionFilter()
{
  this->NumberOfSubdivisions = 1;
}

void vtkApproximatingSubdivisionFilter::Execute()
{
  int numCells, numPts;
  int level;
  vtkPoints *outputPts;

  vtkCellArray *outputPolys;
  vtkPolyData *input = this->GetInput();
  vtkPolyData *output = this->GetOutput();
  vtkPointData *outputPD;
  vtkCellData *outputCD;
  vtkIntArray *edgeData;

  vtkDebugMacro(<< "Generating subdivision surface using approximating scheme");
  if (input == NULL)
    {
    vtkErrorMacro(<<"Input is NULL");
    return;
    }

  numPts=input->GetNumberOfPoints();
  numCells=input->GetNumberOfCells();

  if (numPts < 1 || numCells < 1)
    {
    vtkErrorMacro(<<"No data to approximate!");
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
    } // each level

  inputDS->Squeeze();

  output->SetPoints(inputDS->GetPoints());
  output->SetPolys(inputDS->GetPolys());
  output->GetPointData()->PassData(inputDS->GetPointData());
  output->GetCellData()->PassData(inputDS->GetCellData());
  inputDS->Delete();
}

int vtkApproximatingSubdivisionFilter::FindEdge (vtkPolyData *mesh, int cellId, int p1, int p2, vtkIntArray *edgeData, vtkIdList *cellIds)
{
 
  int edgeId;
  int currentCellId;
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

int vtkApproximatingSubdivisionFilter::InterpolatePosition (
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


void vtkApproximatingSubdivisionFilter::GenerateSubdivisionCells (vtkPolyData *inputDS, vtkIntArray *edgeData, vtkCellArray *outputPolys, vtkCellData *outputCD)
{
  int numCells = inputDS->GetNumberOfCells();
  int cellId, newId, id;
  int npts;
  int *pts;
  float edgePts[3];
  int newCellPts[3];
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

void vtkApproximatingSubdivisionFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "Number of subdivisions: " << this->NumberOfSubdivisions << endl;
}


