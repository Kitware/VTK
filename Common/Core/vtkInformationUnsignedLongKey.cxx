// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkInformationUnsignedLongKey.h"

#include "vtkInformation.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkInformationUnsignedLongKey::vtkInformationUnsignedLongKey(const char* name, const char* location)
  : vtkInformationKey(name, location)
{
  vtkCommonInformationKeyManager::Register(this);
}

//------------------------------------------------------------------------------
vtkInformationUnsignedLongKey::~vtkInformationUnsignedLongKey() = default;

//------------------------------------------------------------------------------
void vtkInformationUnsignedLongKey::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
class vtkInformationUnsignedLongValue : public vtkObjectBase
{
public:
  vtkBaseTypeMacro(vtkInformationUnsignedLongValue, vtkObjectBase);
  unsigned long Value;
};

//------------------------------------------------------------------------------
void vtkInformationUnsignedLongKey::Set(vtkInformation* info, unsigned long value)
{
  if (vtkInformationUnsignedLongValue* oldv =
        static_cast<vtkInformationUnsignedLongValue*>(this->GetAsObjectBase(info)))
  {
    if (oldv->Value != value)
    {
      // Replace the existing value.
      oldv->Value = value;
      // Since this sets a value without call SetAsObjectBase(),
      // the info has to be modified here (instead of
      // vtkInformation::SetAsObjectBase()
      info->Modified(this);
    }
  }
  else
  {
    // Allocate a new value.
    vtkInformationUnsignedLongValue* v = new vtkInformationUnsignedLongValue;
    v->InitializeObjectBase();
    v->Value = value;
    this->SetAsObjectBase(info, v);
    v->Delete();
  }
}

//------------------------------------------------------------------------------
unsigned long vtkInformationUnsignedLongKey::Get(vtkInformation* info)
{
  vtkInformationUnsignedLongValue* v =
    static_cast<vtkInformationUnsignedLongValue*>(this->GetAsObjectBase(info));
  return v ? v->Value : 0;
}

//------------------------------------------------------------------------------
void vtkInformationUnsignedLongKey::ShallowCopy(vtkInformation* from, vtkInformation* to)
{
  if (this->Has(from))
  {
    this->Set(to, this->Get(from));
  }
  else
  {
    this->SetAsObjectBase(to, nullptr); // doesn't exist in from, so remove the key
  }
}

//------------------------------------------------------------------------------
void vtkInformationUnsignedLongKey::Print(ostream& os, vtkInformation* info)
{
  // Print the value.
  if (this->Has(info))
  {
    os << this->Get(info);
  }
}

//------------------------------------------------------------------------------
unsigned long* vtkInformationUnsignedLongKey::GetWatchAddress(vtkInformation* info)
{
  if (vtkInformationUnsignedLongValue* v =
        static_cast<vtkInformationUnsignedLongValue*>(this->GetAsObjectBase(info)))
  {
    return &v->Value;
  }
  return nullptr;
}
VTK_ABI_NAMESPACE_END
