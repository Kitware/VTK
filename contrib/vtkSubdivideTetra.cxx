/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSubdivideTetra.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkSubdivideTetra.h"
#include "vtkMergePoints.h"

// Description:
// Construct with all types of clipping turned off.
vtkSubdivideTetra::vtkSubdivideTetra()
{
  this->Output = vtkUnstructuredGrid::New();
  this->Output->SetSource(this);
}

void vtkSubdivideTetra::Execute()
{
  int numPts=((vtkDataSet *)this->Input)->GetNumberOfPoints();
  int numCells=((vtkDataSet *)this->Input)->GetNumberOfCells();
  vtkUnstructuredGrid *input=(vtkUnstructuredGrid *)this->Input;
  vtkPoints *inPts=input->GetPoints();
  int cellId, i, pts[4];
  vtkCell *cell;
  vtkPointData *pd = input->GetPointData();
  vtkUnstructuredGrid *output = (vtkUnstructuredGrid *)this->Output;
  vtkPointData *outputPD = output->GetPointData();
  vtkPoints *newPts;
  int ptId;
  vtkCellTypes *types=vtkCellTypes::New();
  vtkCellArray *connections;
  float weights[4], x0[3], x1[3], x2[3], x3[3], x[3];
  int p0, p1, p2, p3, center;
  int e01, e02, e03, e12, e13, e23;
  unsigned char type;
  vtkMergePoints *locator;
  
  vtkDebugMacro(<<"Executing mesh subdivide");

  input->GetCellTypes(types);
  if ( types->GetNumberOfTypes() != 1 || 
  (type=types->GetCellType(0)) != VTK_TETRA )
    {
    vtkErrorMacro(<<"Must be tetrahedra");
    return;
    }
  connections = input->GetCells();
  
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

  // loop over tetrahedra, generating sixteen new ones for each. This is
  // done by introducing mid-edge nodes and a single mid-tetra node.
  for(cellId=0; cellId < numCells; cellId++)
    {
    cell = input->GetCell(cellId);

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

  vtkDebugMacro(<<"Subdivided " << numCells << " cells");

  types->Delete();
  locator->Delete();
  newPts->Delete();
  output->Squeeze();
}

void vtkSubdivideTetra::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkUnstructuredGridFilter::PrintSelf(os,indent);
}

