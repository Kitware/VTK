/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProjectedTexture.cxx
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
#include "vtkProjectedTexture.h"
#include "vtkMath.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkProjectedTexture* vtkProjectedTexture::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkProjectedTexture");
  if(ret)
    {
    return (vtkProjectedTexture*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkProjectedTexture;
}




// Description:
// Initialize the projected texture filter with a position of (0, 0, 1),
// a focal point of (0, 0, 0), an up vector on the +y axis, 
// an aspect ratio of the projection frustum of equal width, height, and focal
// length, an S range of (0, 1) and a T range of (0, 1).
vtkProjectedTexture::vtkProjectedTexture()
{
  this->Position[0] = 0.0;
  this->Position[1] = 0.0;
  this->Position[2] = 1.0;
  this->Orientation[0] = this->Orientation[1] = this->Orientation[2] = 0.0;
  this->SetFocalPoint(0.0, 0.0, 0.0);
  this->Up[0] = 0.0;
  this->Up[1] = 1.0;
  this->Up[2] = 0.0;
  this->AspectRatio[0] = 1.0;
  this->AspectRatio[1] = 1.0;
  this->AspectRatio[2] = 1.0;
  this->SRange[0] = 0.0;
  this->SRange[1] = 1.0;
  this->TRange[0] = 0.0;
  this->TRange[1] = 1.0;
}


void vtkProjectedTexture::SetFocalPoint(float fp[3])
{
  this->SetFocalPoint(fp[0], fp[1], fp[2]);
}

void vtkProjectedTexture::SetFocalPoint(float x, float y, float z)
{
  float orientation[3];

  orientation[0] = x - this->Position[0];
  orientation[1] = y - this->Position[1];
  orientation[2] = z - this->Position[2];
  vtkMath::Normalize(orientation);
  if (this->Orientation[0] != orientation[0] ||
      this->Orientation[1] != orientation[1] ||
      this->Orientation[2] != orientation[2])
    {
    this->Orientation[0] = orientation[0];
    this->Orientation[1] = orientation[1];
    this->Orientation[2] = orientation[2];
    this->Modified();
    }
  this->FocalPoint[0] = x;
  this->FocalPoint[1] = y;
  this->FocalPoint[2] = z;
}

void vtkProjectedTexture::Execute()
{
  float tcoords[2];
  vtkIdType numPts;
  vtkFloatArray *newTCoords;
  vtkIdType i;
  int j;
  float proj;
  float rightv[3], upv[3], diff[3];
  float sScale, tScale, sOffset, tOffset, sSize, tSize, s, t;
  vtkDataSet *input = this->GetInput();
  float  *p;
  vtkDataSet *output = this->GetOutput();

  vtkDebugMacro(<<"Generating texture coordinates!");

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  numPts=input->GetNumberOfPoints();

  //
  //  Allocate texture data
  //

  newTCoords = vtkFloatArray::New();
  newTCoords->SetNumberOfComponents(2);
  newTCoords->SetNumberOfTuples(numPts);

  vtkMath::Normalize (this->Orientation);
  
  vtkMath::Cross (this->Orientation, this->Up, rightv);
  vtkMath::Normalize (rightv);

  vtkMath::Cross (rightv, this->Orientation, upv);
  vtkMath::Normalize (upv);

  sSize = this->AspectRatio[0] / this->AspectRatio[2];
  tSize = this->AspectRatio[1] / this->AspectRatio[2];

  sScale = (this->SRange[1] - this->SRange[0])/sSize;
  tScale = (this->TRange[1] - this->TRange[0])/tSize;

  sOffset = (this->SRange[1] - this->SRange[0])/2.0 + this->SRange[0];
  tOffset = (this->TRange[1] - this->TRange[0])/2.0 + this->TRange[0];
  

  // compute s-t coordinates
  for (i = 0; i < numPts; i++) 
    {
      p = output->GetPoint(i);

      for (j = 0; j < 3; j++) {
	diff[j] = p[j] - this->Position[j];
      }
      
      proj = vtkMath::Dot(diff, this->Orientation);
      if(proj < 1.0e-10 && proj > -1.0e-10) {
	vtkWarningMacro(<<"Singularity:  point located at frustum Position");
	tcoords[0] = sOffset;
	tcoords[1] = tOffset;
      } 

      else {
	for (j = 0; j < 3; j++)
	  {
	  diff[j] = diff[j]/proj - this->Orientation[j];
	  }

	s = vtkMath::Dot(diff, rightv);
	t = vtkMath::Dot(diff, upv);

	tcoords[0] = s * sScale + sOffset;
	tcoords[1] = t * tScale + tOffset;
      }
      
      newTCoords->SetTuple(i,tcoords);
    }
  //
  // Update ourselves
  //
  output->GetPointData()->CopyTCoordsOff();
  output->GetPointData()->PassData(input->GetPointData());
  
  output->GetPointData()->SetTCoords(newTCoords);
  newTCoords->Delete();
}

void vtkProjectedTexture::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToDataSetFilter::PrintSelf(os,indent);

  os << indent << "S Range: (" << this->SRange[0] << ", "
                               << this->SRange[1] << ")\n";

  os << indent << "T Range: (" << this->TRange[0] << ", "
                               << this->TRange[1] << ")\n";

  os << indent << "Position: (" << this->Position[0] << ", "
                                << this->Position[1] << ", "
                                << this->Position[2] << ")\n";

  os << indent << "Orientation: (" << this->Orientation[0] << ", "
                                << this->Orientation[1] << ", "
                                << this->Orientation[2] << ")\n";

  os << indent << "Focal Point: (" << this->FocalPoint[0] << ", "
                                   << this->FocalPoint[1] << ", "
                                   << this->FocalPoint[2] << ")\n";

  os << indent << "Up: (" << this->Up[0] << ", "
                                << this->Up[1] << ", "
                                << this->Up[2] << ")\n";

  os << indent << "AspectRatio: (" << this->AspectRatio[0] << ", "
                                << this->AspectRatio[1] << ", "
                                << this->AspectRatio[2] << ")\n";

}
