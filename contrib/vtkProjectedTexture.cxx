/*=========================================================================

Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkProjectedTexture.h"
#include "vtkMath.h"
#include "vtkFloatTCoords.h"

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
  this->AspectRatio[0] = 0.0;
  this->AspectRatio[1] = 1.0;
  this->AspectRatio[2] = 0.0;
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
}

void vtkProjectedTexture::Execute()
{
  float tcoords[2];
  int numPts;
  vtkFloatTCoords *newTCoords;
  int i, j;
  float proj;
  float rightv[3], upv[3], diff[3];
  float sScale, tScale, sOffset, tOffset, sSize, tSize, s, t;
  vtkDataSet *input=(vtkDataSet *) this->Input;

  float  *p;
  vtkDataSet *output= (vtkDataSet *) this->Output;

  vtkDebugMacro(<<"Generating texture coordinates!");
  numPts=input->GetNumberOfPoints();

  //
  //  Allocate texture data
  //

  newTCoords = vtkFloatTCoords::New();
  newTCoords->SetNumberOfTCoords(numPts);

  vtkMath::Normalize (this->Orientation);
  
  vtkMath::Cross (this->Orientation, this->Up, rightv);
  vtkMath::Normalize (rightv);

  vtkMath::Cross (rightv, this->Orientation, upv);
  vtkMath::Normalize (upv);

  sSize = this->AspectRatio[0] / this->AspectRatio[2];
  tSize = this->AspectRatio[1] / this->AspectRatio[2];

  sScale = (SRange[1] - SRange[0])/sSize;
  tScale = (TRange[1] - TRange[0])/tSize;

  sOffset = (SRange[1] - SRange[0])/2.0 + SRange[0];
  tOffset = (TRange[1] - TRange[0])/2.0 + TRange[0];
  

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
	for (j = 0; j < 3; j++) diff[j] = diff[j]/proj - this->Orientation[j];

	s = vtkMath::Dot(diff, rightv);
	t = vtkMath::Dot(diff, upv);

	tcoords[0] = s * sScale + sOffset;
	tcoords[1] = t * tScale + tOffset;
      }
      
      newTCoords->SetTCoord(i,tcoords);
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
