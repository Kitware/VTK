/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSubdivideTetra.cxx
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
#include "vtkSubdivideTetra.h"
#include "vtkMergePoints.h"
#include "vtkObjectFactory.h"

//-------------------------------------------------------------------------
vtkSubdivideTetra* vtkSubdivideTetra::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkSubdivideTetra");
  if(ret)
    {
    return (vtkSubdivideTetra*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkSubdivideTetra;
}

// Description:
// Construct with all types of clipping turned off.
vtkSubdivideTetra::vtkSubdivideTetra()
{
}

void vtkSubdivideTetra::Execute()
{
  vtkUnstructuredGrid *input=(vtkUnstructuredGrid *)this->GetInput();
  vtkIdType numPts = input->GetNumberOfPoints();
  vtkIdType numCells = input->GetNumberOfCells();
  vtkPoints *inPts=input->GetPoints();
  vtkIdType cellId, i;
  vtkIdType pts[4];
  vtkGenericCell *cell;
  vtkPointData *pd = input->GetPointData();
  vtkUnstructuredGrid *output = this->GetOutput();
  vtkPointData *outputPD = output->GetPointData();
  vtkPoints *newPts;
  vtkIdType ptId;

  float weights[4], x0[3], x1[3], x2[3], x3[3], x[3];
  int p0, p1, p2, p3;
  vtkIdType center, e01, e02, e03, e12, e13, e23;
  vtkMergePoints *locator;
  
  vtkDebugMacro(<<"Executing mesh subdivide");

  if (input->IsHomogeneous() == 0 ||
      input->GetCellType(0) != VTK_TETRA)
    {
    vtkErrorMacro(<<"all cells must be tetrahedra.");
    return;
    }

  // Copy original points and point data
  newPts = vtkPoints::New();
  newPts->Allocate(5*numPts,numPts);
  outputPD->InterpolateAllocate(pd,5*numPts,numPts);
  
  output->Allocate(numCells);
  output->SetPoints(newPts);

  locator = vtkMergePoints::New();
  locator->InitPointInsertion (newPts, input->GetBounds());

  for (ptId=0; ptId < numPts; ptId++)
    {
    locator->InsertNextPoint(inPts->GetPoint(ptId));
    outputPD->CopyData(pd,ptId,ptId);
    }

  cell = vtkGenericCell::New();

  // loop over tetrahedra, generating sixteen new ones for each. This is
  // done by introducing mid-edge nodes and a single mid-tetra node.
  for(cellId=0; cellId < numCells; cellId++)
    {
    input->GetCell(cellId, cell);

    // get tetra points
    cell->Points->GetPoint(0,x0);
    cell->Points->GetPoint(1,x1);
    cell->Points->GetPoint(2,x2);
    cell->Points->GetPoint(3,x3);

    p0 = cell->PointIds->GetId(0);
    p1 = cell->PointIds->GetId(1);
    p2 = cell->PointIds->GetId(2);
    p3 = cell->PointIds->GetId(3);

    // compute center point
    weights[0] = weights[1] = weights[2] = weights[3] = 0.25;
    for (i=0; i<3; i++)
      {
      x[i] = 0.25*(x0[i] + x1[i] + x2[i] + x3[i]);
      }
    center = locator->InsertNextPoint(x);
    outputPD->InterpolatePoint(pd, center, cell->PointIds, weights);
    
    // compute edge points
    // edge 0-1
    for (i=0; i<3; i++)
      {
      x[i] = 0.5 * (x1[i] + x0[i]);
      }
    e01 = locator->InsertNextPoint(x);
    outputPD->InterpolateEdge(pd, e01, p0, p1, 0.5);
    
    // edge 1-2
    for (i=0; i<3; i++)
      {
      x[i] = 0.5 * (x2[i] + x1[i]);
      }
    e12 = locator->InsertNextPoint(x);
    outputPD->InterpolateEdge(pd, e12, p1, p2, 0.5);
    
    // edge 2-0
    for (i=0; i<3; i++)
      {
      x[i] = 0.5 * (x2[i] + x0[i]);
      }
    e02 = locator->InsertNextPoint(x);
    outputPD->InterpolateEdge(pd, e02, p2, p0, 0.5);
    
    // edge 0-3
    for (i=0; i<3; i++)
      {
      x[i] = 0.5 * (x3[i] + x0[i]);
      }
    e03 = locator->InsertNextPoint(x);
    outputPD->InterpolateEdge(pd, e03, p0, p3, 0.5);
    
    // edge 1-3
    for (i=0; i<3; i++)
      {
      x[i] = 0.5 * (x3[i] + x1[i]);
      }
    e13 = locator->InsertNextPoint(x);
    outputPD->InterpolateEdge(pd, e13, p1, p3, 0.5);
    
    // edge 2-3
    for (i=0; i<3; i++)
      {
      x[i] = 0.5 * (x3[i] + x2[i]);
      }
    e23 = locator->InsertNextPoint(x);
    outputPD->InterpolateEdge(pd, e23, p2, p3, 0.5);

    // Now create tetrahedra
    // First, four tetra from each vertex
    pts[0] = p0;
    pts[1] = e01;
    pts[2] = e02;
    pts[3] = e03;
    output->InsertNextCell(VTK_TETRA, 4, pts);
    pts[0] = p1;
    pts[1] = e01;
    pts[2] = e12;
    pts[3] = e13;
    output->InsertNextCell(VTK_TETRA, 4, pts);
    pts[0] = p2;
    pts[1] = e02;
    pts[2] = e12;
    pts[3] = e23;
    output->InsertNextCell(VTK_TETRA, 4, pts);
    pts[0] = p3;
    pts[1] = e03;
    pts[2] = e13;
    pts[3] = e23;
    output->InsertNextCell(VTK_TETRA, 4, pts);

    // Now four tetra from cut-off tetra corners
    pts[0] = center;
    pts[1] = e01;
    pts[2] = e02;
    pts[3] = e03;
    output->InsertNextCell(VTK_TETRA, 4, pts);
    pts[1] = e01;
    pts[2] = e12;
    pts[3] = e13;
    output->InsertNextCell(VTK_TETRA, 4, pts);
    pts[1] = e02;
    pts[2] = e12;
    pts[3] = e23;
    output->InsertNextCell(VTK_TETRA, 4, pts);
    pts[1] = e03;
    pts[2] = e13;
    pts[3] = e23;
    output->InsertNextCell(VTK_TETRA, 4, pts);

    // Now four tetra from triangles on tetra faces
    pts[0] = center;
    pts[1] = e01;
    pts[2] = e12;
    pts[3] = e02;
    output->InsertNextCell(VTK_TETRA, 4, pts);
    pts[1] = e01;
    pts[2] = e13;
    pts[3] = e03;
    output->InsertNextCell(VTK_TETRA, 4, pts);
    pts[1] = e12;
    pts[2] = e23;
    pts[3] = e13;
    output->InsertNextCell(VTK_TETRA, 4, pts);
    pts[1] = e02;
    pts[2] = e23;
    pts[3] = e03;
    output->InsertNextCell(VTK_TETRA, 4, pts);

    } //for all cells
  cell->Delete();

  vtkDebugMacro(<<"Subdivided " << numCells << " cells");

  locator->Delete();
  newPts->Delete();
  output->Squeeze();
}

void vtkSubdivideTetra::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkUnstructuredGridToUnstructuredGridFilter::PrintSelf(os,indent);
}

