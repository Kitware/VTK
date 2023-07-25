// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkUnstructuredGridVolumeRayCastIterator.h"

//------------------------------------------------------------------------------

VTK_ABI_NAMESPACE_BEGIN
vtkUnstructuredGridVolumeRayCastIterator::vtkUnstructuredGridVolumeRayCastIterator()
{
  this->Bounds[0] = 0.0;
  this->Bounds[1] = 1.0;

  this->MaxNumberOfIntersections = 32;
}

vtkUnstructuredGridVolumeRayCastIterator::~vtkUnstructuredGridVolumeRayCastIterator() = default;

void vtkUnstructuredGridVolumeRayCastIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Bounds: [" << this->Bounds[0] << ", " << this->Bounds[1] << "]" << endl;
  os << indent << "MaxNumberOfIntersections: " << this->MaxNumberOfIntersections << endl;
}
VTK_ABI_NAMESPACE_END
