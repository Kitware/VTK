/*=========================================================================

  Program:   Visualization Toolkit
  Module:    BrownPts.cc
  Language:  C++
  Date:      9/14/94
  Version:   1.1


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
#include "vtkBrownianPoints.h"
#include "vtkVectors.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkBrownianPoints* vtkBrownianPoints::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkBrownianPoints");
  if(ret)
    {
    return (vtkBrownianPoints*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkBrownianPoints;
}




vtkBrownianPoints::vtkBrownianPoints()
{
  this->MinimumSpeed = 0.0;
  this->MaximumSpeed = 1.0;
}

void vtkBrownianPoints::Execute()
{
  int i, j, numPts;
  vtkVectors *newVectors;
  float v[3], norm, speed;
  vtkDataSet *input= this->GetInput();
  vtkDataSet *output =  this->GetOutput();

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  vtkDebugMacro(<< "Executing Brownian filter");

  if ( ((numPts=input->GetNumberOfPoints()) < 1) )
    {
    vtkErrorMacro(<< "No input!\n");
    return;
    }

  newVectors = vtkVectors::New();
  newVectors->Allocate(numPts);
//
// Check consistency of minumum and maximum speed
//  
  if ( this->MinimumSpeed > this->MaximumSpeed )
    {
    vtkErrorMacro(<< " Minimum speed > maximum speed; reset to (0,1).");
    this->MinimumSpeed = 0.0;
    this->MaximumSpeed = 1.0;
    }

  for (i=0; i<numPts; i++)
    {
    speed = vtkMath::Random(this->MinimumSpeed,this->MaximumSpeed);
    if ( speed != 0.0 )
      {
      for (j=0; j<3; j++)
	{
	v[j] = vtkMath::Random(0,speed);
	}
      norm = vtkMath::Norm(v);
      for (j=0; j<3; j++)
	{
	v[j] *= (speed / norm);
	}
      }
    else
      {
      for (j=0; j<3; j++)
	{
	v[j] = 0.0;
	}
      }

    newVectors->InsertNextVector(v);
    }
//
// Update ourselves
//
  output->GetPointData()->CopyVectorsOff();
  output->GetPointData()->PassData(input->GetPointData());

  output->GetPointData()->SetVectors(newVectors);
  newVectors->Delete();
}

void vtkBrownianPoints::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToDataSetFilter::PrintSelf(os,indent);

  os << indent << "Minimum Speed: " << this->MinimumSpeed << "\n";
  os << indent << "Maximum Speed: " << this->MaximumSpeed << "\n";
}
