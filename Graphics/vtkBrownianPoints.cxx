/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBrownianPoints.cxx
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
#include "vtkBrownianPoints.h"
#include "vtkMath.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkBrownianPoints, "1.33");
vtkStandardNewMacro(vtkBrownianPoints);

vtkBrownianPoints::vtkBrownianPoints()
{
  this->MinimumSpeed = 0.0;
  this->MaximumSpeed = 1.0;
}

void vtkBrownianPoints::Execute()
{
  vtkIdType i, numPts;
  int j;
  vtkFloatArray *newVectors;
  float v[3], norm, speed;
  vtkDataSet *input= this->GetInput();
  vtkDataSet *output =  this->GetOutput();

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  vtkDebugMacro(<< "Executing Brownian filter");

  if ( ((numPts=input->GetNumberOfPoints()) < 1) )
    {
    vtkErrorMacro(<< "No input!\n");
    return;
    }

  newVectors = vtkFloatArray::New();
  newVectors->SetNumberOfComponents(3);
  newVectors->SetNumberOfTuples(numPts);
  newVectors->SetName("BrownianVectors");

  // Check consistency of minumum and maximum speed
  //  
  if ( this->MinimumSpeed > this->MaximumSpeed )
    {
    vtkErrorMacro(<< " Minimum speed > maximum speed; reset to (0,1).");
    this->MinimumSpeed = 0.0;
    this->MaximumSpeed = 1.0;
    }

  for (i=0; i<numPts; i++)
    {
    if ( ! (i % 10000) ) 
      {
      this->UpdateProgress ((float)i/numPts);
      if (this->GetAbortExecute())
        {
        break;
        }
      }

    speed = vtkMath::Random(this->MinimumSpeed,this->MaximumSpeed);
    if ( speed != 0.0 )
      {
      for (j=0; j<3; j++)
        {
        v[j] = vtkMath::Random(0,speed);
        }
      norm = vtkMath::Norm(v);
      for (j=0; j<3; j++)
        {
        v[j] *= (speed / norm);
        }
      }
    else
      {
      for (j=0; j<3; j++)
        {
        v[j] = 0.0;
        }
      }

    newVectors->SetTuple(i,v);
    }

  // Update ourselves
  //
  output->GetPointData()->CopyVectorsOff();
  output->GetPointData()->PassData(input->GetPointData());

  output->GetPointData()->SetVectors(newVectors);
  newVectors->Delete();
}

void vtkBrownianPoints::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Minimum Speed: " << this->MinimumSpeed << "\n";
  os << indent << "Maximum Speed: " << this->MaximumSpeed << "\n";
}
