// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// We never need to instantiate vtkAOSDataArrayTemplate<vtkIdType> or
// vtkArrayIteratorTemplate<vtkIdType> because they are instantiated
// by the corresponding array for its native type.  Therefore this
// code should not be uncommented and is here for reference:
//   #define VTK_AOS_DATA_ARRAY_TEMPLATE_INSTANTIATING
//   #include "vtkAOSDataArrayTemplate.txx"
//   VTK_AOS_DATA_ARRAY_TEMPLATE_INSTANTIATE(vtkIdType);

#include "vtkIdTypeArray.h"

#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkIdTypeArray);
vtkStandardExtendedNewMacro(vtkIdTypeArray);

//------------------------------------------------------------------------------
vtkIdTypeArray::vtkIdTypeArray() = default;

//------------------------------------------------------------------------------
vtkIdTypeArray::~vtkIdTypeArray() = default;

//------------------------------------------------------------------------------
void vtkIdTypeArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->RealSuperclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
