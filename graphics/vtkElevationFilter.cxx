/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkElevationFilter.cxx
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
#include "vtkElevationFilter.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkFloatArray.h"


//------------------------------------------------------------------------------
vtkElevationFilter* vtkElevationFilter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkElevationFilter");
  if(ret)
    {
    return (vtkElevationFilter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkElevationFilter;
}




// Construct object with LowPoint=(0,0,0) and HighPoint=(0,0,1). Scalar
// range is (0,1).
vtkElevationFilter::vtkElevationFilter()
{
  this->LowPoint[0] = 0.0;
  this->LowPoint[1] = 0.0;
  this->LowPoint[2] = 0.0;
 
  this->HighPoint[0] = 0.0;
  this->HighPoint[1] = 0.0;
  this->HighPoint[2] = 1.0;

  this->ScalarRange[0] = 0.0;
  this->ScalarRange[1] = 1.0;
}

//
// Convert position along ray into scalar value.  Example use includes 
// coloring terrain by elevation.
//
void vtkElevationFilter::Execute()
{
  int i, j, numPts;
  vtkFloatArray *newScalars;
  float l, *x, s, v[3];
  float diffVector[3], diffScalar;
  vtkDataSet *input = this->GetInput();

  // Initialize
  //
  vtkDebugMacro(<<"Generating elevation scalars!");

  // First, copy the input to the output as a starting point
  this->GetOutput()->CopyStructure( input );

  if ( ((numPts=input->GetNumberOfPoints()) < 1) )
    {
    //vtkErrorMacro(<< "No input!");
    return;
    }

  // Allocate
  //
  newScalars = vtkFloatArray::New();
  newScalars->SetNumberOfTuples(numPts);

  // Set up 1D parametric system
  //
  for (i=0; i<3; i++)
    {
    diffVector[i] = this->HighPoint[i] - this->LowPoint[i];
    }
  if ( (l = vtkMath::Dot(diffVector,diffVector)) == 0.0)
    {
    vtkErrorMacro(<< this << ": Bad vector, using (0,0,1)\n");
    diffVector[0] = diffVector[1] = 0.0; diffVector[2] = 1.0;
    l = 1.0;
    }

  // Compute parametric coordinate and map into scalar range
  //
  diffScalar = this->ScalarRange[1] - this->ScalarRange[0];
  for (i=0; i<numPts; i++)
    {
    if ( ! (i % 10000) ) 
      {
      this->UpdateProgress ((float)i/numPts);
      if (this->GetAbortExecute())
	{
	break;
	}
      }

    x = input->GetPoint(i);
    for (j=0; j<3; j++)
      {
      v[j] = x[j] - this->LowPoint[j];
      }
    s = vtkMath::Dot(v,diffVector) / l;
    s = (s < 0.0 ? 0.0 : s > 1.0 ? 1.0 : s);
    newScalars->SetValue(i,this->ScalarRange[0]+s*diffScalar);
    }

  // Update self
  //
  this->GetOutput()->GetPointData()->CopyScalarsOff();
  this->GetOutput()->GetPointData()->PassData(input->GetPointData());

  this->GetOutput()->GetCellData()->PassData(input->GetCellData());

  newScalars->SetName("Elevation");
  this->GetOutput()->GetPointData()->SetScalars(newScalars);
  newScalars->Delete();
}

void vtkElevationFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToDataSetFilter::PrintSelf(os,indent);

  os << indent << "Low Point: (" << this->LowPoint[0] << ", "
                                << this->LowPoint[1] << ", "
                                << this->LowPoint[2] << ")\n";
  os << indent << "High Point: (" << this->HighPoint[0] << ", "
                                << this->HighPoint[1] << ", "
                                << this->HighPoint[2] << ")\n";
  os << indent << "Scalar Range: (" << this->ScalarRange[0] << ", "
                                << this->ScalarRange[1] << ")\n";
}
