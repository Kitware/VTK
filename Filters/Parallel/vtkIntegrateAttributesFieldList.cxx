// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkIntegrateAttributesFieldList.h"
#include "vtkDoubleArray.h"

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
vtkIntegrateAttributesFieldList::vtkIntegrateAttributesFieldList(int numInputs)
  : Superclass(numInputs)
{
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkAbstractArray> vtkIntegrateAttributesFieldList::CreateArray(int type) const
{
  if (auto array = this->Superclass::CreateArray(type))
  {
    const int is_numeric = (array->IsNumeric());
    if (is_numeric)
    {
      return vtkSmartPointer<vtkAbstractArray>::Take(vtkDoubleArray::New());
    }
  }
  return nullptr;
}
VTK_ABI_NAMESPACE_END
