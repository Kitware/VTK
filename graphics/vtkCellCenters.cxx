/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellCenters.cxx
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
#include "vtkCellCenters.h"

// Description:
// Construct object with vertex cell generation turned off.
vtkCellCenters::vtkCellCenters()
{
  this->VertexCells = 0;
}

// Generate points
void vtkCellCenters::Execute()
{
  int cellId, numCells, subId;
  vtkDataSet *input = this->GetInput();
  vtkPolyData *output = this->GetOutput();
  vtkCellData *inCD=input->GetCellData();
  vtkPointData *outPD=output->GetPointData();
  vtkPoints *newPts;
  vtkCell *cell;
  float x[3], pcoords[3];
  float *weights = new float [input->GetMaxCellSize()];
  
  vtkDebugMacro(<<"Generating cell center points");

  if ( (numCells = input->GetNumberOfCells()) < 1 )
    {
    vtkErrorMacro(<<"No cells to generate center points for");
    return;
    }

  newPts = vtkPoints::New();
  newPts->SetNumberOfPoints(numCells);

  for (cellId=0; cellId < numCells; cellId++)
    {
    cell = input->GetCell(cellId);
    subId = cell->GetParametricCenter(pcoords);
    cell->EvaluateLocation(subId, pcoords, x, weights);
    newPts->SetPoint(cellId,x);
    }

  if ( this->VertexCells )
    {
    int pts[1];
    vtkCellData *outCD=output->GetCellData();
    vtkCellArray *verts = vtkCellArray::New();
    verts->Allocate(verts->EstimateSize(1,numCells),1);

    for (cellId=0; cellId < numCells; cellId++)
      {
      pts[0] = cellId;
      verts->InsertNextCell(1,pts);
      }

    output->SetVerts(verts);
    verts->Delete();
    outCD->PassData(inCD); //only if verts are generated
    }

  // clean up and update output
  output->SetPoints(newPts);
  newPts->Delete();

  outPD->PassData(inCD); //because number of points = number of cells
}

void vtkCellCenters::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "Vertex Cells: " << (this->VertexCells ? "On\n" : "Off\n");
}

