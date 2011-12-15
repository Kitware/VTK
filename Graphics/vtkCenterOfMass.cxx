/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkCenterOfMass.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCenterOfMass.h"

#include "vtkPointSet.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"

vtkStandardNewMacro(vtkCenterOfMass);

vtkCenterOfMass::vtkCenterOfMass()
{
  this->UseScalarsAsWeights = false;
}

void vtkCenterOfMass::ComputeCenterOfMass(vtkPointSet* input, double center[3], bool useWeights)
{
  // Initialize the center to zero
  center[0] = 0.0;
  center[1] = 0.0;
  center[2] = 0.0;
  if(useWeights)
    {
    if(!input->GetPointData()->GetScalars())
      {
      vtkErrorWithObjectMacro(input, "To use weights PointData::Scalars must be set!");
      return;
      }
    }

  double weightTotal = 0.0;

  vtkDataArray* scalars = input->GetPointData()->GetScalars();

  for(vtkIdType i = 0; i < input->GetNumberOfPoints(); i++)
    {
    double point[3];
    input->GetPoint(i, point);

    // The weights will all be 1 if we are not using the scalars as weights
    double weight = 1.0;
    if(useWeights)
      {
      weight = scalars->GetComponent(0,i);
      }
    weightTotal += weight;
    vtkMath::MultiplyScalar(point, weight);
    vtkMath::Add(center, point, center);
    }

  if(weightTotal <= 0)
    {
    vtkErrorWithObjectMacro(scalars, "The sum of the weights must be > 0!");
    }

  vtkMath::MultiplyScalar(center, 1./weightTotal);
}

int vtkCenterOfMass::RequestData(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* vtkNotUsed(outputVector) )
{
  // Get the input and ouptut
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkPointSet* input = vtkPointSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  if(input->GetNumberOfPoints() == 0)
    {
    vtkErrorMacro(<<"Input must have at least 1 point!");
    }
  this->ComputeCenterOfMass(input, this->Center, this->UseScalarsAsWeights);

  return 1;
}

void vtkCenterOfMass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Center: "
     << this->Center[0] << " "
     << this->Center[1] << " "
     << this->Center[2] << endl;
  os << indent << "UseScalarsAsWeights: " << this->UseScalarsAsWeights << endl;
}
