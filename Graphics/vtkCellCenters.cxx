/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellCenters.cxx
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
#include "vtkCellCenters.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkCellCenters* vtkCellCenters::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkCellCenters");
  if(ret)
    {
    return (vtkCellCenters*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkCellCenters;
}




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
  vtkCellData *inCD;
  vtkPointData *outPD;
  vtkPoints *newPts;
  vtkCell *cell;
  float x[3], pcoords[3];
  if (input == NULL)
    {
    vtkErrorMacro(<<"Input is NULL");
    return;
    }
  float *weights = new float [input->GetMaxCellSize()];

  vtkDebugMacro(<<"Generating cell center points");

  inCD=input->GetCellData();
  outPD=output->GetPointData();

  if ( (numCells = input->GetNumberOfCells()) < 1 )
    {
    vtkErrorMacro(<<"No cells to generate center points for");
    if (weights)
      {
      delete [] weights;
      }
    return;
    }

  newPts = vtkPoints::New();
  newPts->SetNumberOfPoints(numCells);

  int abort=0;
  int progressInterval = numCells/10 + 1;
  for (cellId=0; cellId < numCells && !abort; cellId++)
    {
    if ( ! (cellId % progressInterval) ) 
      {
      vtkDebugMacro(<<"Processing #" << cellId);
      this->UpdateProgress (0.5*cellId/numCells);
      abort = this->GetAbortExecute();
      }

    cell = input->GetCell(cellId);
    subId = cell->GetParametricCenter(pcoords);
    cell->EvaluateLocation(subId, pcoords, x, weights);
    newPts->SetPoint(cellId,x);
    }

  if ( this->VertexCells )
    {
    vtkIdType pts[1];
    vtkCellData *outCD=output->GetCellData();
    vtkCellArray *verts = vtkCellArray::New();
    verts->Allocate(verts->EstimateSize(1,numCells),1);

    for (cellId=0; cellId < numCells && !abort; cellId++)
      {
      if ( ! (cellId % progressInterval) ) 
        {
        vtkDebugMacro(<<"Processing #" << cellId);
        this->UpdateProgress (0.5+0.5*cellId/numCells);
        abort = this->GetAbortExecute();
        }

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
  if (weights)
    {
    delete [] weights;
    }
}

void vtkCellCenters::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "Vertex Cells: " << (this->VertexCells ? "On\n" : "Off\n");
}

