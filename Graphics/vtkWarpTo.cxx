/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWarpTo.cxx
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
#include "vtkWarpTo.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkWarpTo, "1.36");
vtkStandardNewMacro(vtkWarpTo);

vtkWarpTo::vtkWarpTo() 
{
  this->ScaleFactor = 0.5; 
  this->Absolute = 0;
  this->Position[0] = this->Position[1] = this->Position[2] = 0.0;
}

void vtkWarpTo::Execute()
{
  vtkPoints *inPts;
  vtkPoints *newPts;
  vtkIdType ptId, numPts;
  int i;
  float *x, newX[3];
  vtkPointSet *input = this->GetInput();
  vtkPointSet *output = this->GetOutput();
  float mag;
  float minMag = 0;
  
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
  newPts = vtkPoints::New(); newPts->SetNumberOfPoints(numPts);

  if (this->Absolute)
    {
    minMag = 1.0e10;
    for (ptId=0; ptId < numPts; ptId++)
      {
      x = inPts->GetPoint(ptId);
      mag = sqrt(vtkMath::Distance2BetweenPoints(this->Position,x));
      if (mag < minMag)
        {
        minMag = mag;
        }
      }
    }
  
  //
  // Loop over all points, adjusting locations
  //
  for (ptId=0; ptId < numPts; ptId++)
    {
    x = inPts->GetPoint(ptId);
    if (this->Absolute)
      {
      mag = sqrt(vtkMath::Distance2BetweenPoints(this->Position,x));
      for (i=0; i<3; i++)
        {
        newX[i] = this->ScaleFactor*
          (this->Position[i] + minMag*(x[i] - this->Position[i])/mag) + 
          (1.0 - this->ScaleFactor)*x[i];
        }
      }
    else
      {
      for (i=0; i<3; i++)
        {
        newX[i] = (1.0 - this->ScaleFactor)*x[i] + 
          this->ScaleFactor*this->Position[i];
        }
      }
    newPts->SetPoint(ptId, newX);
    }
  //
  // Update ourselves and release memory
  //
  output->GetPointData()->CopyNormalsOff(); // distorted geometry
  output->GetPointData()->PassData(input->GetPointData());

  output->SetPoints(newPts);
  newPts->Delete();
}

void vtkWarpTo::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "Absolute: " << (this->Absolute ? "On\n" : "Off\n");

  os << indent << "Position: (" << this->Position[0] << ", " 
    << this->Position[1] << ", " << this->Position[2] << ")\n";
  os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
}
