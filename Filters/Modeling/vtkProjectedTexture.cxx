/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProjectedTexture.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkProjectedTexture.h"

#include "vtkDataSet.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

vtkStandardNewMacro(vtkProjectedTexture);

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
  this->MirrorSeparation = 1.0;
  this->CameraMode = VTK_PROJECTED_TEXTURE_USE_PINHOLE;
  this->SRange[0] = 0.0;
  this->SRange[1] = 1.0;
  this->TRange[0] = 0.0;
  this->TRange[1] = 1.0;
}


void vtkProjectedTexture::SetFocalPoint(double fp[3])
{
  this->SetFocalPoint(fp[0], fp[1], fp[2]);
}

void vtkProjectedTexture::SetFocalPoint(double x, double y, double z)
{
  double orientation[3];

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

int vtkProjectedTexture::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet *output = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  double tcoords[2];
  vtkIdType numPts;
  vtkFloatArray *newTCoords;
  vtkIdType i;
  int j;
  double proj;
  double rightv[3], upv[3], diff[3];
  double sScale, tScale, sOffset, tOffset, sSize, tSize, s, t;
  double p[3];

  vtkDebugMacro(<<"Generating texture coordinates!");

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  numPts=input->GetNumberOfPoints();

  //
  //  Allocate texture data
  //

  newTCoords = vtkFloatArray::New();
  newTCoords->SetName("ProjectedTextureCoordinates");
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
      output->GetPoint(i, p);

      for (j = 0; j < 3; j++)
        {
        diff[j] = p[j] - this->Position[j];
        }

      proj = vtkMath::Dot(diff, this->Orientation);

      // New mode to handle a two mirror camera with separation of
      // MirrorSeparation -- In this case, we assume that the first mirror
      // controls the elevation and the second controls the azimuth.  Texture
      // coordinates for the elevation are handled as normal, while those for
      // the azimuth must be calculated based on a new baseline difference to
      // include the mirror separation.
      if (this->CameraMode == VTK_PROJECTED_TEXTURE_USE_TWO_MIRRORS)
        {
        // First calculate elevation coordinate t.
        if(proj < 1.0e-10 && proj > -1.0e-10)
          {
          vtkWarningMacro(<<"Singularity:  point located at elevation frustum Position");
          tcoords[1] = tOffset;
          }
        else
          {
          for (j = 0; j < 3; j++)
            {
            diff[j] = diff[j]/proj - this->Orientation[j];
            }

          t = vtkMath::Dot(diff, upv);
          tcoords[1] = t * tScale + tOffset;
          }

        // Now with t complete, continue on to calculate coordinate s
        // by offsetting the center of the lens back by MirrorSeparation
        // in direction opposite to the orientation.
        for (j = 0; j < 3; j++)
          {
          diff[j] = p[j] - this->Position[j] + (this->MirrorSeparation*this->Orientation[j]);
          }

        proj = vtkMath::Dot(diff, this->Orientation);

        if(proj < 1.0e-10 && proj > -1.0e-10)
          {
          vtkWarningMacro(<<"Singularity:  point located at azimuth frustum Position");
          tcoords[0] = sOffset;
          }
        else
          {
          for (j = 0; j < 3; j++)
            {
            diff[j] = diff[j]/proj - this->Orientation[j];
            }

          s = vtkMath::Dot(diff, rightv);
          sSize = this->AspectRatio[0] / (this->AspectRatio[2] + this->MirrorSeparation);
          sScale = (this->SRange[1] - this->SRange[0])/sSize;
          sOffset = (this->SRange[1] - this->SRange[0])/2.0 + this->SRange[0];
          tcoords[0] = s * sScale + sOffset;
          }
        }
      else
        {
        if(proj < 1.0e-10 && proj > -1.0e-10)
          {
          vtkWarningMacro(<<"Singularity:  point located at frustum Position");
          tcoords[0] = sOffset;
          tcoords[1] = tOffset;
          }
        else
          {
          for (j = 0; j < 3; j++)
            {
            diff[j] = diff[j]/proj - this->Orientation[j];
            }

          s = vtkMath::Dot(diff, rightv);
          t = vtkMath::Dot(diff, upv);

          tcoords[0] = s * sScale + sOffset;
          tcoords[1] = t * tScale + tOffset;
          }
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

  return 1;
}

void vtkProjectedTexture::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

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

  os << indent << "CameraMode: ";
  if (this->CameraMode == VTK_PROJECTED_TEXTURE_USE_PINHOLE)
  {
    os << "Pinhole\n";
  }
  else if (this->CameraMode == VTK_PROJECTED_TEXTURE_USE_TWO_MIRRORS)
  {
    os << "Two Mirror\n";
  }
  else
  {
    os << "Illegal Mode\n";
  }

  os << indent << "MirrorSeparation: " << this->MirrorSeparation << "\n";
}
