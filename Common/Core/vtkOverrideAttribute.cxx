// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOverrideAttribute.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkOverrideAttribute);

//------------------------------------------------------------------------------
vtkOverrideAttribute::vtkOverrideAttribute() = default;

//------------------------------------------------------------------------------
vtkOverrideAttribute::~vtkOverrideAttribute() = default;

//------------------------------------------------------------------------------
void vtkOverrideAttribute::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Name: " << (this->Name.empty() ? "(none)" : this->Name) << "\n";
  os << indent << "Value: " << (this->Value.empty() ? "(none)" : this->Value) << "\n";
  os << indent << "Next: " << (this->Next ? this->Next->GetObjectDescription() : "(none)") << "\n";
}

//------------------------------------------------------------------------------
vtkOverrideAttribute* vtkOverrideAttribute::CreateAttributeChain(
  const char* name, const char* value, vtkOverrideAttribute* nextInChain)
{
  vtkOverrideAttribute* attribute = vtkOverrideAttribute::New();
  attribute->Name = name ? name : "";
  attribute->Value = value ? value : "";
  attribute->Next.TakeReference(nextInChain);
  return attribute;
}
VTK_ABI_NAMESPACE_END
