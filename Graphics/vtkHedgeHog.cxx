/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHedgeHog.cxx
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
#include "vtkHedgeHog.h"
#include "vtkObjectFactory.h"

//------------------------------------------------------------------------
vtkHedgeHog* vtkHedgeHog::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkHedgeHog");
  if(ret)
    {
    return (vtkHedgeHog*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkHedgeHog;
}

vtkHedgeHog::vtkHedgeHog()
{
  this->ScaleFactor = 1.0;
  this->VectorMode = VTK_USE_VECTOR;
}

void vtkHedgeHog::Execute()
{
  vtkDataSet *input= this->GetInput();
  vtkIdType numPts;
  vtkPoints *newPts;
  vtkPointData *pd;
  vtkDataArray *inVectors;
  vtkDataArray *inNormals;
  vtkIdType ptId;
  int i;
  vtkIdType pts[2];
  vtkCellArray *newLines;
  float *x, *v;
  float newX[3];
  vtkPolyData *output = this->GetOutput();
  vtkPointData *outputPD = output->GetPointData();
  
  // Initialize
  //
  numPts = input->GetNumberOfPoints();
  pd = input->GetPointData();
  inVectors = pd->GetVectors();
  if ( numPts < 1 )
    {
    vtkErrorMacro(<<"No input data");
    return;
    }
  if ( !inVectors && this->VectorMode == VTK_USE_VECTOR)
    {
    vtkErrorMacro(<<"No vectors in input data");
    return;
    }

  inNormals = pd->GetNormals();
  if ( !inNormals && this->VectorMode == VTK_USE_NORMAL)
    {
    vtkErrorMacro(<<"No normals in input data");
    return;
    }
  outputPD->CopyAllocate(pd, 2*numPts);

  newPts = vtkPoints::New(); newPts->SetNumberOfPoints(2*numPts);
  newLines = vtkCellArray::New();
  newLines->Allocate(newLines->EstimateSize(numPts,2));

  // Loop over all points, creating oriented line
  //
  for (ptId=0; ptId < numPts; ptId++)
    {
    if ( ! (ptId % 10000) ) //abort/progress
      {
      this->UpdateProgress ((float)ptId/numPts);
      if (this->GetAbortExecute())
	{
	break;
	}
      }
    
    x = input->GetPoint(ptId);
    if (this->VectorMode == VTK_USE_VECTOR)
      {
      v = inVectors->GetTuple(ptId);
      }
    else
      {
      v = inNormals->GetTuple(ptId);
      }
    for (i=0; i<3; i++)
      {
      newX[i] = x[i] + this->ScaleFactor * v[i];
      }

    pts[0] = ptId;
    pts[1] = ptId + numPts;;

    newPts->SetPoint(pts[0], x);
    newPts->SetPoint(pts[1], newX);

    newLines->InsertNextCell(2,pts);

    outputPD->CopyData(pd,ptId,pts[0]);
    outputPD->CopyData(pd,ptId,pts[1]);
    }

  // Update ourselves and release memory
  //
  output->SetPoints(newPts);
  newPts->Delete();

  output->SetLines(newLines);
  newLines->Delete();
}

void vtkHedgeHog::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
  os << indent << "Orient Mode: " << (this->VectorMode == VTK_USE_VECTOR ? 
                                       "Orient by vector\n" : "Orient by normal\n");
}
