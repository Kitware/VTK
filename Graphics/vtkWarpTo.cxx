/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWarpTo.cxx
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
#include "vtkWarpTo.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkWarpTo* vtkWarpTo::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkWarpTo");
  if(ret)
    {
    return (vtkWarpTo*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkWarpTo;
}

vtkWarpTo::vtkWarpTo() 
{
  this->ScaleFactor = 0.5; 
  this->Absolute = 0;
  this->Position[0] = this->Position[1] = this->Position[2] = 0.0;
}

void vtkWarpTo::Execute()
{
  vtkPoints *inPts;
  vtkPoints *newPts;
  vtkIdType ptId, numPts;
  int i;
  float *x, newX[3];
  vtkPointSet *input = this->GetInput();
  vtkPointSet *output = this->GetOutput();
  float mag;
  float minMag = 0;
  
  vtkDebugMacro(<<"Warping data to a point");

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  inPts = input->GetPoints();

  if (!inPts )
    {
    vtkErrorMacro(<<"No input data");
    return;
    }

  numPts = inPts->GetNumberOfPoints();
  newPts = vtkPoints::New(); newPts->SetNumberOfPoints(numPts);

  if (this->Absolute)
    {
    minMag = 1.0e10;
    for (ptId=0; ptId < numPts; ptId++)
      {
      x = inPts->GetPoint(ptId);
      mag = sqrt(vtkMath::Distance2BetweenPoints(this->Position,x));
      if (mag < minMag)
	{
	minMag = mag;
	}
      }
    }
  
  //
  // Loop over all points, adjusting locations
  //
  for (ptId=0; ptId < numPts; ptId++)
    {
    x = inPts->GetPoint(ptId);
    if (this->Absolute)
      {
      mag = sqrt(vtkMath::Distance2BetweenPoints(this->Position,x));
      for (i=0; i<3; i++)
	{
	newX[i] = this->ScaleFactor*
	  (this->Position[i] + minMag*(x[i] - this->Position[i])/mag) + 
	  (1.0 - this->ScaleFactor)*x[i];
	}
      }
    else
      {
      for (i=0; i<3; i++)
	{
	newX[i] = (1.0 - this->ScaleFactor)*x[i] + 
	  this->ScaleFactor*this->Position[i];
	}
      }
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

void vtkWarpTo::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPointSetToPointSetFilter::PrintSelf(os,indent);
  
  os << indent << "Absolute: " << (this->Absolute ? "On\n" : "Off\n");

  os << indent << "Position: (" << this->Position[0] << ", " 
    << this->Position[1] << ", " << this->Position[2] << ")\n";
  os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
}
