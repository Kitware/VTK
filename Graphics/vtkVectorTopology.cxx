/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVectorTopology.cxx
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
#include <math.h>
#include "vtkVectorTopology.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkVectorTopology* vtkVectorTopology::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkVectorTopology");
  if(ret)
    {
    return (vtkVectorTopology*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkVectorTopology;
}




// Construct object with distance 0.1.
vtkVectorTopology::vtkVectorTopology()
{
  this->Distance = 0.1;
}

void vtkVectorTopology::Execute()
{
  vtkIdType cellId, ptId;
  int i, j, npts;
  int negative[3], positive[3], subId=0;
  float x[3], pcoords[3], *v;
  vtkCell *cell;
  vtkDataArray *inVectors;
  vtkPoints *newPts;
  vtkCellArray *newVerts;
  vtkDataSet *input=(vtkDataSet *)this->GetInput();
  vtkPointData *pd=input->GetPointData();
  vtkPolyData *output=(vtkPolyData *)this->GetOutput();
  vtkPointData *outputPD=output->GetPointData();
  float *weights=new float[input->GetMaxCellSize()];
//
// Initialize self; check input; create output objects
//
  vtkDebugMacro(<< "Executing vector topology...");

  // make sure we have vector data
  if ( ! (inVectors = input->GetPointData()->GetActiveVectors()) )
    {
    vtkErrorMacro(<<"No vector data, can't create topology markers...");
    return;
    }

  newPts = vtkPoints::New();
  newPts->Allocate(100);
  newVerts = vtkCellArray::New();
  newVerts->Allocate(newVerts->EstimateSize(1,100));
  outputPD->CopyAllocate(pd);
//
// Find cells whose vector components all pass through zero
//
  pcoords[0] = pcoords[1] = pcoords[2] = 0.5;
  newVerts->InsertNextCell(100); //temporary count
  for (cellId=0; cellId<input->GetNumberOfCells(); cellId++)
    {
    cell = input->GetCell(cellId);
    npts = cell->GetNumberOfPoints();
    for (i=0; i<3; i++)
      {
      negative[i] = positive[i] = 0;
      }
    for (i=0; i < npts; i++)
      {
      ptId = cell->GetPointId(i);
      v = inVectors->GetTuple(ptId);
      for (j=0; j<3; j++)
        {
        if ( v[j] < 0.0 )
	  {
	  negative[j] = 1;
	  }
        else if ( v[j] >= 0.0 )
	  {
	  positive[j] = 1;
	  }
        }
      }
    if ( negative[0] && positive[0] && negative[1] && positive[1] &&
    negative[2] && positive[2] )
      { // place point at center of cell
      cell->EvaluateLocation(subId, pcoords, x, weights);
      ptId = newPts->InsertNextPoint(x);
      newVerts->InsertCellPoint(ptId);
      }
    }
  newVerts->UpdateCellCount(newPts->GetNumberOfPoints());
 
  vtkDebugMacro(<< "Created " << newPts->GetNumberOfPoints() << "points");
  delete [] weights;
//
// Update ourselves
//
  output->SetPoints(newPts);
  output->SetVerts(newVerts);
  output->Squeeze();
}

void vtkVectorTopology::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "Distance: " << this->Distance << "\n";
}


