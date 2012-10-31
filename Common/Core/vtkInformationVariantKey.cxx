/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationVariantKey.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInformationVariantKey.h"

#include "vtkInformation.h"
#include "vtkVariant.h"


//----------------------------------------------------------------------------
vtkInformationVariantKey::vtkInformationVariantKey(const char* name, const char* location):
  vtkInformationKey(name, location)
{
  vtkCommonInformationKeyManager::Register(this);
}

//----------------------------------------------------------------------------
vtkInformationVariantKey::~vtkInformationVariantKey()
{
}

//----------------------------------------------------------------------------
void vtkInformationVariantKey::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
class vtkInformationVariantValue: public vtkObjectBase
{
public:
  vtkTypeMacro(vtkInformationVariantValue, vtkObjectBase);
  vtkVariant Value;
  static vtkVariant Invalid;
};

vtkVariant vtkInformationVariantValue::Invalid;

//----------------------------------------------------------------------------
void vtkInformationVariantKey::Set(vtkInformation* info, const vtkVariant& value)
{
  if(vtkInformationVariantValue* oldv =
     static_cast<vtkInformationVariantValue *>(
       this->GetAsObjectBase(info)))
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
    this->ConstructClass("vtkInformationVariantValue");
    v->Value = value;
    this->SetAsObjectBase(info, v);
    v->Delete();
    }
}

//----------------------------------------------------------------------------
const vtkVariant& vtkInformationVariantKey::Get(vtkInformation* info)
{
  vtkInformationVariantValue* v =
    static_cast<vtkInformationVariantValue *>(
      this->GetAsObjectBase(info));
  return v?v->Value:vtkInformationVariantValue::Invalid;
}

//----------------------------------------------------------------------------
void vtkInformationVariantKey::ShallowCopy(vtkInformation* from, vtkInformation* to)
{
  if (this->Has(from))
    {
    this->Set(to, this->Get(from));
    }
  else
    {
    this->SetAsObjectBase(to, 0); // doesn't exist in from, so remove the key
    }
}

//----------------------------------------------------------------------------
void vtkInformationVariantKey::Print(ostream& os, vtkInformation* info)
{
  // Print the value.
  if(this->Has(info))
    {
    os << this->Get(info);
    }
}

//----------------------------------------------------------------------------
vtkVariant* vtkInformationVariantKey::GetWatchAddress(vtkInformation* info)
{
  if(vtkInformationVariantValue* v =
     static_cast<vtkInformationVariantValue *>(
       this->GetAsObjectBase(info)))
    {
    return &v->Value;
    }
  return 0;
}
