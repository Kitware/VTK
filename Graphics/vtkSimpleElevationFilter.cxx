/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSimpleElevationFilter.cxx
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
#include "vtkSimpleElevationFilter.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkFloatArray.h"

//------------------------------------------------------------------------------
vtkSimpleElevationFilter* vtkSimpleElevationFilter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkSimpleElevationFilter");
  if(ret)
    {
    return (vtkSimpleElevationFilter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkSimpleElevationFilter;
}


// Construct object with LowPoint=(0,0,0) and HighPoint=(0,0,1). Scalar
// range is (0,1).
vtkSimpleElevationFilter::vtkSimpleElevationFilter()
{
  this->Vector[0] = 0.0;
  this->Vector[1] = 0.0;
  this->Vector[2] = 1.0;
}

// Convert position along ray into scalar value.  Example use includes 
// coloring terrain by elevation.
//
void vtkSimpleElevationFilter::Execute()
{
  vtkIdType i, numPts;
  vtkFloatArray *newScalars;
  float s, x[3];
  vtkDataSet *input = this->GetInput();
  vtkDataSet *output = this->GetOutput();

  // Initialize
  //
  vtkDebugMacro(<<"Generating elevation scalars!");

 // First, copy the input to the output as a starting point
  output->CopyStructure( input );

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
  if ( vtkMath::Dot(this->Vector,this->Vector) == 0.0)
    {
    vtkErrorMacro(<< "Bad vector, using (0,0,1)");
    this->Vector[0] = this->Vector[1] = 0.0; this->Vector[2] = 1.0;
    }

  // Compute dot product
  //
  int abort=0;
  vtkIdType progressInterval=numPts/20 + 1;
  for (i=0; i<numPts && !abort; i++)
    {
    if ( ! (i % progressInterval) ) 
      {
      this->UpdateProgress ((float)i/numPts);
      abort = this->GetAbortExecute();
      }

    input->GetPoint(i,x);
    s = vtkMath::Dot(this->Vector,x);
    newScalars->SetComponent(i,0,s);
    }

  // Update self
  //
  output->GetPointData()->CopyScalarsOff();
  output->GetPointData()->PassData(input->GetPointData());

  output->GetCellData()->PassData(input->GetCellData());

  output->GetPointData()->SetScalars(newScalars);
  newScalars->Delete();
}

void vtkSimpleElevationFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToDataSetFilter::PrintSelf(os,indent);

  os << indent << "Vector: (" << this->Vector[0] << ", "
     << this->Vector[1] << ", " << this->Vector[2] << ")\n";
}
