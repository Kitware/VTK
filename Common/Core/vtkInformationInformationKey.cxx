// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkInformationInformationKey.h"

#include "vtkInformation.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkInformationInformationKey::vtkInformationInformationKey(const char* name, const char* location)
  : vtkInformationKey(name, location)
{
  vtkCommonInformationKeyManager::Register(this);
}

//------------------------------------------------------------------------------
vtkInformationInformationKey::~vtkInformationInformationKey() = default;

//------------------------------------------------------------------------------
void vtkInformationInformationKey::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkInformationInformationKey::Set(vtkInformation* info, vtkInformation* value)
{
  this->SetAsObjectBase(info, value);
}

//------------------------------------------------------------------------------
vtkInformation* vtkInformationInformationKey::Get(vtkInformation* info)
{
  return static_cast<vtkInformation*>(this->GetAsObjectBase(info));
}

//------------------------------------------------------------------------------
void vtkInformationInformationKey::ShallowCopy(vtkInformation* from, vtkInformation* to)
{
  this->Set(to, this->Get(from));
}

//------------------------------------------------------------------------------
void vtkInformationInformationKey::DeepCopy(vtkInformation* from, vtkInformation* to)
{
  vtkInformation* toInfo = vtkInformation::New();
  toInfo->Copy(this->Get(from), 1);
  this->Set(to, toInfo);
  toInfo->Delete();
}
VTK_ABI_NAMESPACE_END
