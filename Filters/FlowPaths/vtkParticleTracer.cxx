// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkParticleTracer.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSetGet.h"

VTK_ABI_NAMESPACE_BEGIN
vtkObjectFactoryNewMacro(vtkParticleTracer);

vtkParticleTracer::vtkParticleTracer()
{
  this->IgnorePipelineTime = 0;
}

int vtkParticleTracer::OutputParticles(vtkPolyData* poly)
{
  this->Output = poly;
  return 1;
}

void vtkParticleTracer::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
