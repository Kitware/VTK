// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkInformationStringKey.h"

#include "vtkInformation.h"

#include <string>

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkInformationStringKey::vtkInformationStringKey(const char* name, const char* location)
  : vtkInformationKey(name, location)
{
  vtkCommonInformationKeyManager::Register(this);
}

//------------------------------------------------------------------------------
vtkInformationStringKey::~vtkInformationStringKey() = default;

//------------------------------------------------------------------------------
void vtkInformationStringKey::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
class vtkInformationStringValue : public vtkObjectBase
{
public:
  vtkBaseTypeMacro(vtkInformationStringValue, vtkObjectBase);
  std::string Value;
};

//------------------------------------------------------------------------------
void vtkInformationStringKey::Set(vtkInformation* info, const char* value)
{
  if (value)
  {
    if (vtkInformationStringValue* oldv =
          static_cast<vtkInformationStringValue*>(this->GetAsObjectBase(info)))
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
      vtkInformationStringValue* v = new vtkInformationStringValue;
      v->InitializeObjectBase();
      v->Value = value;
      this->SetAsObjectBase(info, v);
      v->Delete();
    }
  }
  else
  {
    this->SetAsObjectBase(info, nullptr);
  }
}

//------------------------------------------------------------------------------
void vtkInformationStringKey::Set(vtkInformation* info, const std::string& s)
{
  this->Set(info, s.c_str());
}

//------------------------------------------------------------------------------
const char* vtkInformationStringKey::Get(vtkInformation* info)
{
  vtkInformationStringValue* v =
    static_cast<vtkInformationStringValue*>(this->GetAsObjectBase(info));
  return v ? v->Value.c_str() : nullptr;
}

//------------------------------------------------------------------------------
void vtkInformationStringKey::ShallowCopy(vtkInformation* from, vtkInformation* to)
{
  this->Set(to, this->Get(from));
}

//------------------------------------------------------------------------------
void vtkInformationStringKey::Print(ostream& os, vtkInformation* info)
{
  // Print the value.
  if (this->Has(info))
  {
    os << this->Get(info);
  }
}
VTK_ABI_NAMESPACE_END
