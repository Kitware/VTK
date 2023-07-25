// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkUnstructuredGridVolumeRayIntegrator.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkUnstructuredGridVolumeRayIntegrator::vtkUnstructuredGridVolumeRayIntegrator() = default;

vtkUnstructuredGridVolumeRayIntegrator::~vtkUnstructuredGridVolumeRayIntegrator() = default;

void vtkUnstructuredGridVolumeRayIntegrator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
