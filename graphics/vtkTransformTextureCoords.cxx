/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransformTextureCoords.cxx
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
#include "vtkTransformTextureCoords.h"
#include "vtkTransform.h"
#include "vtkObjectFactory.h"
#include "vtkFloatArray.h"

//-------------------------------------------------------------------------
vtkTransformTextureCoords* vtkTransformTextureCoords::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkTransformTextureCoords");
  if(ret)
    {
    return (vtkTransformTextureCoords*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkTransformTextureCoords;
}

// Create instance with Origin (0.5,0.5,0.5); Position (0,0,0); and Scale
// set to (1,1,1). Rotation of the texture coordinates is turned off.
vtkTransformTextureCoords::vtkTransformTextureCoords()
{
  this->Origin[0] = this->Origin[1] = this->Origin[2] = 0.5;
  this->Position[0] = this->Position[1] = this->Position[2] = 0.0;
  this->Scale[0] = this->Scale[1] = this->Scale[2] = 1.0;

  this->FlipR = 0;
  this->FlipS = 0;
  this->FlipT = 0;
}

// Incrementally change the position of the texture map (i.e., does a
// translate or shift of the texture coordinates).
void vtkTransformTextureCoords::AddPosition (float dPX, float dPY, float dPZ)
{
  float position[3];

  position[0] = this->Position[0] + dPX;
  position[1] = this->Position[1] + dPY;
  position[2] = this->Position[2] + dPZ;
  
  this->SetPosition(position);
}

void vtkTransformTextureCoords::AddPosition(float deltaPosition[3])
{ 
  this->AddPosition (deltaPosition[0], deltaPosition[1], deltaPosition[2]);
}

void vtkTransformTextureCoords::Execute()
{
  vtkDataSet *input = this->GetInput();
  vtkDataSet *output = this->GetOutput();
  vtkDataArray *inTCoords=input->GetPointData()->GetActiveTCoords();
  vtkDataArray *newTCoords;
  vtkIdType numPts=input->GetNumberOfPoints(), ptId;
  int i, j, texDim;
  vtkTransform *transform = vtkTransform::New();
  vtkMatrix4x4 *matrix = vtkMatrix4x4::New();
  float *TC, newTC[3];

  vtkDebugMacro(<<"Transforming texture coordinates...");

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  if ( inTCoords == NULL || numPts < 1 )
    {
    vtkErrorMacro(<<"No texture coordinates to transform");
    return;
    }

  // create same type as input
  texDim = inTCoords->GetNumberOfComponents();
  newTCoords = inTCoords->MakeObject();
  newTCoords->Allocate(numPts*texDim);

  // just pretend texture coordinate is 3D point and use transform object to
  // manipulate
  transform->PostMultiply();
  // shift back to origin
  transform->Translate(-this->Origin[0], -this->Origin[1], -this->Origin[2]);

  // scale
  transform->Scale(this->Scale[0], this->Scale[1], this->Scale[2]);

  // rotate about z, then x, then y
  if ( this->FlipT )
    {
    transform->RotateZ(180.0);
    }
  if ( this->FlipR )
    {
    transform->RotateX(180.0);
    }
  if ( this->FlipS )
    {
    transform->RotateY(180.0);
    }

  // move back from origin and translate
  transform->Translate(this->Origin[0] + this->Position[0],
                      this->Origin[1] + this->Position[1],
		      this->Origin[2] + this->Position[2]);

  matrix->DeepCopy(transform->GetMatrix());

  newTC[0] = newTC[1] = newTC[2] = 0.0;
  newTC[0] = newTC[1] = newTC[2] = 0.0;

  int abort=0;
  int progressInterval = numPts/20+1;
  
  for (ptId=0; ptId < numPts && !abort; ptId++)
    {
    if ( !(ptId % progressInterval) )
      {
      this->UpdateProgress((float)ptId/numPts);
      abort = this->GetAbortExecute();
      }

    TC = inTCoords->GetTuple(ptId);
    for (i=0; i<texDim; i++)
      {
      newTC[i] = matrix->Element[i][3];
      for (j=0; j<texDim; j++)
        {
        newTC[i] += matrix->Element[i][j] * TC[j];
        }
      }

    newTCoords->InsertTuple(ptId,newTC);
    }

  // Update self
  //
  output->GetPointData()->CopyTCoordsOff();
  output->GetPointData()->PassData(input->GetPointData());

  output->GetPointData()->SetTCoords(newTCoords);
  newTCoords->Delete();
  matrix->Delete();
  transform->Delete();
}

void vtkTransformTextureCoords::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToDataSetFilter::PrintSelf(os,indent);

  os << indent << "Scale: (" 
     << this->Scale[0] << ", " 
     << this->Scale[1] << ", " 
     << this->Scale[2] << ")\n";

  os << indent << "Position: (" 
     << this->Position[0] << ", " 
     << this->Position[1] << ", " 
     << this->Position[2] << ")\n";

  os << indent << "Origin: (" 
     << this->Origin[0] << ", " 
     << this->Origin[1] << ", " 
     << this->Origin[2] << ")\n";

  os << indent << "FlipR: " << (this->FlipR ? "On\n" : "Off\n");
  os << indent << "FlipS: " << (this->FlipS ? "On\n" : "Off\n");
  os << indent << "FlipT: " << (this->FlipT ? "On\n" : "Off\n");
}
