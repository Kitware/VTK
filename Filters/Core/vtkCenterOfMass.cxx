// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCenterOfMass.h"

#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkSmartPointer.h"

#include <cassert>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkCenterOfMass);

vtkCenterOfMass::vtkCenterOfMass()
{
  this->UseScalarsAsWeights = false;
  this->Center[0] = this->Center[1] = this->Center[2] = 0.0;
  this->SetNumberOfOutputPorts(0);
}

void vtkCenterOfMass::ComputeCenterOfMass(
  vtkPoints* points, vtkDataArray* scalars, double center[3])
{
  vtkIdType n = points->GetNumberOfPoints();
  // Initialize the center to zero
  center[0] = 0.0;
  center[1] = 0.0;
  center[2] = 0.0;

  assert("pre: no points" && n > 0);

  if (scalars)
  {
    // If weights are to be used
    double weightTotal = 0.0;

    assert("pre: wrong array size" && scalars->GetNumberOfTuples() == n);

    for (vtkIdType i = 0; i < n; i++)
    {
      double point[3];
      points->GetPoint(i, point);

      double weight = scalars->GetComponent(i, 0);
      weightTotal += weight;

      vtkMath::MultiplyScalar(point, weight);
      vtkMath::Add(center, point, center);
    }

    assert("pre: sum of weights must be positive" && weightTotal > 0.0);

    if (weightTotal > 0.0)
    {
      vtkMath::MultiplyScalar(center, 1.0 / weightTotal);
    }
  }
  else
  {
    // No weights
    for (vtkIdType i = 0; i < n; i++)
    {
      double point[3];
      points->GetPoint(i, point);

      vtkMath::Add(center, point, center);
    }

    vtkMath::MultiplyScalar(center, 1.0 / n);
  }
}

int vtkCenterOfMass::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{
  // Get the input and output
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkPointSet* input = vtkPointSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkPoints* points = input->GetPoints();

  if (points == nullptr || points->GetNumberOfPoints() == 0)
  {
    vtkErrorMacro("Input must have at least 1 point!");
    return 1;
  }

  vtkDataArray* scalars = nullptr;
  if (this->UseScalarsAsWeights)
  {
    scalars = input->GetPointData()->GetScalars();

    if (!scalars)
    {
      vtkErrorWithObjectMacro(input, "To use weights PointData::Scalars must be set!");
      return 1;
    }
  }

  this->ComputeCenterOfMass(points, scalars, this->Center);

  this->CheckAbort();

  return 1;
}

void vtkCenterOfMass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Center: " << this->Center[0] << " " << this->Center[1] << " " << this->Center[2]
     << endl;
  os << indent << "UseScalarsAsWeights: " << this->UseScalarsAsWeights << endl;
}
VTK_ABI_NAMESPACE_END
