/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWarpLens.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkWarpLens.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkWarpLens, "1.23");
vtkStandardNewMacro(vtkWarpLens);

//
// Preserve old Kappa instance variable. It appears to be the
// second order symmetric radial lens distortion parameter
//
void vtkWarpLens::SetKappa(float kappa)
{
  this->SetK1(kappa);
}

float vtkWarpLens::GetKappa()
{
  return this->GetK1();
}

//
// Preserve old Center point instance variable.
// It appears to be the center of radial distortion in pixel coordinates
//
void vtkWarpLens::SetCenter(float centerX, float centerY)
{
  this->SetPrincipalPoint(centerX, centerY);
}

float *vtkWarpLens::GetCenter()
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

void vtkWarpLens::Execute()
{
  vtkPoints *inPts;
  vtkPoints *newPts;
  vtkIdType ptId, numPts;
  float *pixel, newPixel[3];
  vtkPointSet *input = this->GetInput();
  vtkPointSet *output = this->GetOutput();
  float x;
  float y;
  float newX;
  float newY;
  float rSquared;
  
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
  newPts = vtkPoints::New(); 
  newPts->SetNumberOfPoints(numPts);

  //
  // Loop over all pixels, adjusting locations
  //
  for (ptId=0; ptId < numPts; ptId++)
    {
    pixel = inPts->GetPoint(ptId);

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
