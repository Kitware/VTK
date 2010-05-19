/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWarpLens.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkWarpLens.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"

vtkStandardNewMacro(vtkWarpLens);

//
// Preserve old Kappa instance variable. It appears to be the
// second order symmetric radial lens distortion parameter
//
void vtkWarpLens::SetKappa(double kappa)
{
  this->SetK1(kappa);
}

double vtkWarpLens::GetKappa()
{
  return this->GetK1();
}

//
// Preserve old Center point instance variable.
// It appears to be the center of radial distortion in pixel coordinates
//
void vtkWarpLens::SetCenter(double centerX, double centerY)
{
  this->SetPrincipalPoint(centerX, centerY);
}

double *vtkWarpLens::GetCenter()
{
  return this->GetPrincipalPoint();
}

vtkWarpLens::vtkWarpLens()
{
  this->PrincipalPoint[0] = 0.0;
  this->PrincipalPoint[1] = 0.0;
  this->K1 = -1.0e-6;
  this->K2 = 0.0;
  this->P1 = 0.0;
  this->P2 = 0.0;
  this->FormatWidth = 1.0;
  this->FormatHeight = 1.0;
  this->ImageWidth = 1;
  this->ImageHeight = 1;        
}

int vtkWarpLens::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPointSet *input = vtkPointSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPointSet *output = vtkPointSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkPoints *inPts;
  vtkPoints *newPts;
  vtkIdType ptId, numPts;
  double pixel[3], newPixel[3];
  double x;
  double y;
  double newX;
  double newY;
  double rSquared;
  
  vtkDebugMacro(<<"Warping data to a point");

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  inPts = input->GetPoints();  
  if (!inPts )
    {
    vtkErrorMacro(<<"No input data");
    return 1;
    }

  numPts = inPts->GetNumberOfPoints();
  newPts = vtkPoints::New(); 
  newPts->SetNumberOfPoints(numPts);

  //
  // Loop over all pixels, adjusting locations
  //
  for (ptId=0; ptId < numPts; ptId++)
    {
    inPts->GetPoint(ptId, pixel);

    //
    // Convert to working in mm from pixels and make the Principal Point (0,0)
    //
    x = pixel[0] / this->ImageWidth * this->FormatWidth -
      this->PrincipalPoint[0];
    y = (- pixel[1]) / this->ImageHeight * this->FormatHeight +
      this->PrincipalPoint[1];

    //
    // Lens distortion causes a point's image on the imaging surface to
    // shifted from its true position as if it had been imaged by an ideal pin-
    // hole camera.
    //
    // The corrected location adds the correction for radial len distortion
    // and for the decentering lens distortion
    //
    rSquared = x*x + y*y;

    newX = x * (1 + this->K1 * rSquared + this->K2 * rSquared*rSquared ) +
      this->P1 * (rSquared + 2 * x*x) + 2 * this->P2 * x * y;

    newY = y * (1 + this->K1 * rSquared + this->K2 * rSquared*rSquared ) +
      this->P2 * (rSquared + 2 * y*y) + 2 * this->P1 * x * y;

    //
    // Convert back to pixels
    //
    newPixel[0] = (newX + this->PrincipalPoint[0]) / this->FormatWidth *
      this->ImageWidth; 
    newPixel[1] = (newY - this->PrincipalPoint[1]) / 
      this->FormatHeight * this->ImageHeight * -1; 

    newPixel[2] = pixel[2];             // pixel color
    newPts->SetPoint(ptId, newPixel);
    }

  //
  // Update ourselves and release memory
  //
  output->GetPointData()->CopyNormalsOff(); // distorted geometry
  output->GetPointData()->PassData(input->GetPointData());

  output->SetPoints(newPts);
  newPts->Delete();

  return 1;
}

void vtkWarpLens::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "PrincipalPoint: (" << this->PrincipalPoint[0] << ", " 
    << this->PrincipalPoint[1] << ") in mm\n";
  os << indent << "K1: " << this->K1 << "\n";
  os << indent << "K2: " << this->K2 << "\n";
  os << indent << "P1: " << this->P1 << "\n";
  os << indent << "P2: " << this->P2 << "\n";
  os << indent << "FormatWidth: " << this->FormatWidth << " in mm\n";
  os << indent << "FormatHeight: " << this->FormatHeight << " in mm\n";
  os << indent << "ImageWidth: " << this->ImageWidth << " in pixels\n";
  os << indent << "ImageHeight: " << this->ImageHeight << " in pixels\n";
}
