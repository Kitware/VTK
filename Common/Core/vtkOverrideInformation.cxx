// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOverrideInformation.h"

#include "vtkObjectFactory.h"
#include "vtkOverrideAttribute.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOverrideInformation);
vtkCxxSetObjectMacro(vtkOverrideInformation, ObjectFactory, vtkObjectFactory);
vtkCxxSetObjectMacro(vtkOverrideInformation, OverrideAttributes, vtkOverrideAttribute);

vtkOverrideInformation::vtkOverrideInformation()
{
  this->ClassOverrideName = nullptr;
  this->ClassOverrideWithName = nullptr;
  this->Description = nullptr;
  this->ObjectFactory = nullptr;
  this->OverrideAttributes = nullptr;
}

vtkOverrideInformation::~vtkOverrideInformation()
{
  delete[] this->ClassOverrideName;
  delete[] this->ClassOverrideWithName;
  delete[] this->Description;
  if (this->ObjectFactory)
  {
    this->ObjectFactory->Delete();
  }
  if (this->OverrideAttributes)
  {
    this->OverrideAttributes->Delete();
  }
}

void vtkOverrideInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Override: ";
  if (this->ClassOverrideName && this->ClassOverrideWithName && this->Description)
  {
    os << this->ClassOverrideName << "\nWith: " << this->ClassOverrideWithName
       << "\nDescription: " << this->Description;
  }
  else
  {
    os << "(none)\n";
  }

  os << indent << "From Factory:\n";
  if (this->ObjectFactory)
  {
    this->ObjectFactory->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    vtkIndent n = indent.GetNextIndent();
    os << n << "(none)\n";
  }

  os << indent << "Override Attributes:\n";
  if (this->OverrideAttributes)
  {
    this->OverrideAttributes->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    vtkIndent n = indent.GetNextIndent();
    os << n << "(none)\n";
  }
}
VTK_ABI_NAMESPACE_END
