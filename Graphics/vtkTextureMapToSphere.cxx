/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextureMapToSphere.cxx
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
#include "vtkTextureMapToSphere.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkFloatArray.h"

vtkCxxRevisionMacro(vtkTextureMapToSphere, "1.27");
vtkStandardNewMacro(vtkTextureMapToSphere);

// Create object with Center (0,0,0) and the PreventSeam ivar is set to true. The 
// sphere center is automatically computed.
vtkTextureMapToSphere::vtkTextureMapToSphere()
{
  this->Center[0] = this->Center[1] = this->Center[2] = 0.0;

  this->AutomaticSphereGeneration = 1;
  this->PreventSeam = 1;
}

void vtkTextureMapToSphere::Execute()
{
  vtkFloatArray *newTCoords;
  vtkDataSet *input = this->GetInput();
  vtkDataSet *output = this->GetOutput();
  vtkIdType numPts=input->GetNumberOfPoints();
  vtkIdType ptId;
  float *x, rho, r, tc[2], phi=0.0, thetaX, thetaY;
  double diff, PiOverTwo=vtkMath::Pi()/2.0;

  vtkDebugMacro(<<"Generating Spherical Texture Coordinates");

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  if ( numPts < 1 )
    {
    vtkErrorMacro(<<"Can't generate texture coordinates without points");
    return;
    }

  if ( this->AutomaticSphereGeneration )
    {
    this->Center[0] = this->Center[1] = this->Center[2] = 0.0;
    for ( ptId=0; ptId < numPts; ptId++ )
      {
      x = input->GetPoint(ptId);
      this->Center[0] += x[0];
      this->Center[1] += x[1];
      this->Center[2] += x[2];
      }
    this->Center[0] /= numPts;
    this->Center[1] /= numPts;
    this->Center[2] /= numPts;

    vtkDebugMacro(<<"Center computed as: (" << this->Center[0] <<", "
                  << this->Center[1] <<", " << this->Center[2] <<")");
    }

  //loop over all points computing spherical coordinates. Only tricky part
  //is keeping track of singularities/numerical problems.
  newTCoords = vtkFloatArray::New();
  newTCoords->SetNumberOfComponents(2);
  newTCoords->SetNumberOfTuples(numPts);
  for ( ptId=0; ptId < numPts; ptId++ )
    {
    x = input->GetPoint(ptId);
    rho = sqrt((double)vtkMath::Distance2BetweenPoints(x,this->Center));
    if ( rho != 0.0 )
      {
      // watch for truncation problems
      if ( fabs((diff=x[2]-this->Center[2])) > rho )
        {
        phi = 0.0;
        if ( diff > 0.0 )
          {
          tc[1] = 0.0;
          }
        else
          {
          tc[1] = 1.0;
          }
        }
      else
        {
        phi = acos((double)(diff/rho));
        tc[1] = phi / vtkMath::Pi();
        }
      }
    else
      {
      tc[1] = 0.0;
      }

    r = rho * sin((double)phi);
    if ( r != 0.0 )
      {
      // watch for truncation problems
      if ( fabs((diff=x[0]-this->Center[0])) > r )
        {
        if ( diff > 0.0 )
          {
          thetaX = 0.0;
          }
        else
          {
          thetaX = vtkMath::Pi();
          }
        }
      else
        {
        thetaX = acos ((double)diff/r);
        }

      if ( fabs((diff=x[1]-this->Center[1])) > r )
        {
        if ( diff > 0.0 )
          {
          thetaY = PiOverTwo;
          }
        else
          {
          thetaY = -PiOverTwo;
          }
        }
      else
        {
        thetaY = asin ((double)diff/r);
        }
      }
    else
      {
      thetaX = thetaY = 0.0;
      }

    if ( this->PreventSeam )
      {
      tc[0] = thetaX / vtkMath::Pi();
      }
    else
      {
      tc[0] = thetaX / (2.0*vtkMath::Pi());
      if ( thetaY < 0.0 )
        {
        tc[0] = 1.0 - tc[0];
        }
      }

    newTCoords->SetTuple(ptId,tc);
    }

  output->GetPointData()->CopyTCoordsOff();
  output->GetPointData()->PassData(input->GetPointData());

  output->GetCellData()->PassData(input->GetCellData());

  output->GetPointData()->SetTCoords(newTCoords);
  newTCoords->Delete();
}

void vtkTextureMapToSphere::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Automatic Sphere Generation: " << 
                  (this->AutomaticSphereGeneration ? "On\n" : "Off\n");
  os << indent << "Prevent Seam: " << 
                  (this->PreventSeam ? "On\n" : "Off\n");
  os << indent << "Center: (" << this->Center[0] << ", "
                              << this->Center[1] << ", "
                              << this->Center[2] << ")\n";
}

