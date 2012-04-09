/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBrownianPoints.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBrownianPoints.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkFieldData.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

vtkStandardNewMacro(vtkBrownianPoints);

vtkBrownianPoints::vtkBrownianPoints()
{
  this->MinimumSpeed = 0.0;
  this->MaximumSpeed = 1.0;
}

int vtkBrownianPoints::RequestData(
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

  vtkIdType i, numPts;
  int j;
  vtkFloatArray *newVectors;
  double v[3], norm, speed;

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  vtkDebugMacro(<< "Executing Brownian filter");

  if ( ((numPts=input->GetNumberOfPoints()) < 1) )
    {
    vtkDebugMacro(<< "No input!\n");
    return 1;
    }

  newVectors = vtkFloatArray::New();
  newVectors->SetNumberOfComponents(3);
  newVectors->SetNumberOfTuples(numPts);
  newVectors->SetName("BrownianVectors");

  // Check consistency of minimum and maximum speed
  //
  if ( this->MinimumSpeed > this->MaximumSpeed )
    {
    vtkErrorMacro(<< " Minimum speed > maximum speed; reset to (0,1).");
    this->MinimumSpeed = 0.0;
    this->MaximumSpeed = 1.0;
    }

  int tenth = numPts/10 + 1;
  for (i=0; i<numPts; i++)
    {
    if ( ! (i % tenth) )
      {
      this->UpdateProgress (static_cast<double>(i)/numPts);
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
        v[j] = vtkMath::Random(-1.0,1.0);
        }
      norm = vtkMath::Norm(v);
      for (j=0; j<3; j++)
        {
        v[j] *= (speed / norm);
        }
      }
    else
      {
      v[0] = 0.0;
      v[1] = 0.0;
      v[2] = 0.0;
      }

    newVectors->SetTuple(i,v);
    }

  // Update ourselves
  //
  output->GetPointData()->CopyVectorsOff();
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());
  output->GetFieldData()->PassData(input->GetFieldData());

  output->GetPointData()->SetVectors(newVectors);
  newVectors->Delete();

  return 1;
}

void vtkBrownianPoints::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Minimum Speed: " << this->MinimumSpeed << "\n";
  os << indent << "Maximum Speed: " << this->MaximumSpeed << "\n";
}
