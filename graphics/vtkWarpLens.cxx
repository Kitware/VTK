/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWarpLens.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkWarpLens.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkWarpLens* vtkWarpLens::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkWarpLens");
  if(ret)
    {
    return (vtkWarpLens*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkWarpLens;
}




vtkWarpLens::vtkWarpLens()
{
  this->Kappa = -1.0e-6;
  this->Center[0] = 0.0;
  this->Center[1] = 0.0;
}

void vtkWarpLens::Execute()
{
  vtkPoints *inPts;
  vtkPoints *newPts;
  int ptId, numPts;
  float *x, newX[3];
  vtkPointSet *input = this->GetInput();
  vtkPointSet *output = this->GetOutput();
  float r;
  float offset;
  
  vtkDebugMacro(<<"Warping data to a point");

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  r = this->Center[0]*this->Center[0];
  offset = this->Center[0]*(1.0 + this->Kappa*r);

  inPts = input->GetPoints();  
  if (!inPts )
    {
    vtkErrorMacro(<<"No input data");
    return;
    }

  numPts = inPts->GetNumberOfPoints();
  newPts = vtkPoints::New(); 
  newPts->SetNumberOfPoints(numPts);

  //
  // Loop over all points, adjusting locations
  //
  for (ptId=0; ptId < numPts; ptId++)
    {
    x = inPts->GetPoint(ptId);
    newX[0] = x[0] - this->Center[0];
    newX[1] = x[1] - this->Center[1];
    r = newX[0]*newX[0] + newX[1]*newX[1];
    newX[0] = offset + newX[0]*(1.0 + this->Kappa*r);
    newX[1] = offset + newX[1]*(1.0 + this->Kappa*r);
    newX[2] = x[2];
    newPts->SetPoint(ptId, newX);
    }
  //
  // Update ourselves and release memory
  //
  output->GetPointData()->CopyNormalsOff(); // distorted geometry
  output->GetPointData()->PassData(input->GetPointData());

  output->SetPoints(newPts);
  newPts->Delete();
}

void vtkWarpLens::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPointSetToPointSetFilter::PrintSelf(os,indent);
  
  os << indent << "Center: (" << this->Center[0] << ", " 
    << this->Center[1] << ")\n";
  os << indent << "Kappa: " << this->Kappa << "\n";
}
