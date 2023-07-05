// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkInformationVariantKey.h"

#include "vtkInformation.h"
#include "vtkVariant.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkInformationVariantKey::vtkInformationVariantKey(const char* name, const char* location)
  : vtkInformationKey(name, location)
{
  vtkCommonInformationKeyManager::Register(this);
}

//------------------------------------------------------------------------------
vtkInformationVariantKey::~vtkInformationVariantKey() = default;

//------------------------------------------------------------------------------
void vtkInformationVariantKey::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
class vtkInformationVariantValue : public vtkObjectBase
{
public:
  vtkBaseTypeMacro(vtkInformationVariantValue, vtkObjectBase);
  vtkVariant Value;
  static vtkVariant Invalid;
};

vtkVariant vtkInformationVariantValue::Invalid;

//------------------------------------------------------------------------------
void vtkInformationVariantKey::Set(vtkInformation* info, const vtkVariant& value)
{
  if (vtkInformationVariantValue* oldv =
        static_cast<vtkInformationVariantValue*>(this->GetAsObjectBase(info)))
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
    vtkInformationVariantValue* v = new vtkInformationVariantValue;
    v->InitializeObjectBase();
    v->Value = value;
    this->SetAsObjectBase(info, v);
    v->Delete();
  }
}

//------------------------------------------------------------------------------
const vtkVariant& vtkInformationVariantKey::Get(vtkInformation* info)
{
  vtkInformationVariantValue* v =
    static_cast<vtkInformationVariantValue*>(this->GetAsObjectBase(info));
  return v ? v->Value : vtkInformationVariantValue::Invalid;
}

//------------------------------------------------------------------------------
void vtkInformationVariantKey::ShallowCopy(vtkInformation* from, vtkInformation* to)
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
void vtkInformationVariantKey::Print(ostream& os, vtkInformation* info)
{
  // Print the value.
  if (this->Has(info))
  {
    os << this->Get(info);
  }
}

//------------------------------------------------------------------------------
vtkVariant* vtkInformationVariantKey::GetWatchAddress(vtkInformation* info)
{
  if (vtkInformationVariantValue* v =
        static_cast<vtkInformationVariantValue*>(this->GetAsObjectBase(info)))
  {
    return &v->Value;
  }
  return nullptr;
}
VTK_ABI_NAMESPACE_END
