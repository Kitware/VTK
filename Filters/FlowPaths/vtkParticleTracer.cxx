// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkParticleTracer.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSetGet.h"

VTK_ABI_NAMESPACE_BEGIN
vtkObjectFactoryNewMacro(vtkParticleTracer);

vtkParticleTracer::vtkParticleTracer()
{
  this->IgnorePipelineTime = 0;
}

void vtkParticleTracer::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}

int vtkParticleTracer::Finalize(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  int retVal = this->Superclass::Finalize(request, inputVector, outputVector);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  auto output = vtkPolyData::SafeDownCast(vtkDataObject::GetData(outInfo));

  vtkNew<vtkPoints> points;
  points->DeepCopy(this->OutputCoordinates);

  for (vtkIdType id = 0; id < this->OutputCoordinates->GetNumberOfPoints(); ++id)
  {
    double p[3];
    this->OutputCoordinates->GetPoint(id, p);
  }

  output->SetPoints(points);
  output->GetPointData()->DeepCopy(this->OutputPointData);

  return retVal;
}

VTK_ABI_NAMESPACE_END
