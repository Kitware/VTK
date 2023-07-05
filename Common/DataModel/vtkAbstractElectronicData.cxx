// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAbstractElectronicData.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkAbstractElectronicData::vtkAbstractElectronicData()
  : Padding(0.0)
{
}

//------------------------------------------------------------------------------
vtkAbstractElectronicData::~vtkAbstractElectronicData() = default;

//------------------------------------------------------------------------------
void vtkAbstractElectronicData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Padding: " << this->Padding << "\n";
}

//------------------------------------------------------------------------------
void vtkAbstractElectronicData::DeepCopy(vtkDataObject* obj)
{
  vtkAbstractElectronicData* aed = vtkAbstractElectronicData::SafeDownCast(obj);
  if (!aed)
  {
    vtkErrorMacro("Can only deep copy from vtkAbstractElectronicData "
                  "or subclass.");
    return;
  }

  // Call superclass
  this->Superclass::DeepCopy(aed);

  // Copy ivars
  this->Padding = aed->Padding;
}
VTK_ABI_NAMESPACE_END
