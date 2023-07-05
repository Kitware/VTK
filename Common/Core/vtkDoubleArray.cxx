// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Instantiate superclass first to give the template a DLL interface.
#define VTK_AOS_DATA_ARRAY_TEMPLATE_INSTANTIATING
#include "vtkAOSDataArrayTemplate.txx"
VTK_AOS_DATA_ARRAY_TEMPLATE_INSTANTIATE(double);

#include "vtkDoubleArray.h"

#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkDoubleArray);
vtkStandardExtendedNewMacro(vtkDoubleArray);

//------------------------------------------------------------------------------
vtkDoubleArray::vtkDoubleArray() = default;

//------------------------------------------------------------------------------
vtkDoubleArray::~vtkDoubleArray() = default;

//------------------------------------------------------------------------------
void vtkDoubleArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->RealSuperclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
