/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVectorDot.cxx
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
#include "vtkVectorDot.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkFloatArray.h"

//--------------------------------------------------------------------------
vtkVectorDot* vtkVectorDot::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkVectorDot");
  if(ret)
    {
    return (vtkVectorDot*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkVectorDot;
}

// Construct object with scalar range is (-1,1).
vtkVectorDot::vtkVectorDot()
{
  this->ScalarRange[0] = -1.0;
  this->ScalarRange[1] = 1.0;
}

//
// Compute dot product.
//
void vtkVectorDot::Execute()
{
  vtkIdType ptId, numPts;
  vtkFloatArray *newScalars;
  vtkDataSet *input = this->GetInput();
  vtkNormals *inNormals;
  vtkVectors *inVectors;
  float s, *n, *v, min, max, dR, dS;
  vtkDataSet *output = this->GetOutput();
  vtkPointData *pd=input->GetPointData(), *outPD=output->GetPointData();

  // Initialize
  //
  vtkDebugMacro(<<"Generating vector/normal dot product!");

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  if ( (numPts=input->GetNumberOfPoints()) < 1 )
    {
    vtkErrorMacro(<< "No points!");
    return;
    }
  if ( (inVectors=pd->GetVectors()) == NULL )
    {
    vtkErrorMacro(<< "No vectors defined!");
    return;
    }
  if ( (inNormals=pd->GetNormals()) == NULL )
    {
    vtkErrorMacro(<< "No normals defined!");
    return;
    }

  // Allocate
  //
  newScalars = vtkFloatArray::New();
  newScalars->Allocate(numPts);

  // Compute initial scalars
  //
  int abort=0;
  vtkIdType progressInterval=numPts/20 + 1;
  for (min=VTK_LARGE_FLOAT,max=(-VTK_LARGE_FLOAT),ptId=0; 
       ptId < numPts && !abort; ptId++)
    {
    if ( ! (ptId % progressInterval) ) 
      {
      this->UpdateProgress ((float)ptId/numPts);
      abort = this->GetAbortExecute();
      }
    n = inNormals->GetNormal(ptId);
    v = inVectors->GetVector(ptId);
    s = vtkMath::Dot(n,v);
    if ( s < min )
      {
      min = s;
      }
    if ( s > max )
      {
      max = s;
      }
    newScalars->InsertTuple(ptId,&s);
    }

  // Map scalars into scalar range
  //
  if ( (dR=this->ScalarRange[1]-this->ScalarRange[0]) == 0.0 )
    {
    dR = 1.0;
    }
  if ( (dS=max-min) == 0.0 )
    {
    dS = 1.0;
    }

  for ( ptId=0; ptId < numPts; ptId++ )
    {
    s = newScalars->GetComponent(ptId,0);
    s = ((s - min)/dS) * dR + this->ScalarRange[0];
    newScalars->InsertTuple(ptId,&s);
    }

  // Update self and relase memory
  //
  outPD->CopyScalarsOff();
  outPD->PassData(input->GetPointData());

  outPD->SetScalars(newScalars);
  newScalars->Delete();
}

void vtkVectorDot::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToDataSetFilter::PrintSelf(os,indent);

  os << indent << "Scalar Range: (" << this->ScalarRange[0] << ", "
                                    << this->ScalarRange[1] << ")\n";
}
