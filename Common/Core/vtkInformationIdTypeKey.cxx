/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationIdTypeKey.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInformationIdTypeKey.h"

#include "vtkInformation.h"


//----------------------------------------------------------------------------
vtkInformationIdTypeKey::vtkInformationIdTypeKey(const char* name, const char* location):
  vtkInformationKey(name, location)
{
  vtkCommonInformationKeyManager::Register(this);
}

//----------------------------------------------------------------------------
vtkInformationIdTypeKey::~vtkInformationIdTypeKey()
{
}

//----------------------------------------------------------------------------
void vtkInformationIdTypeKey::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
class vtkInformationIdTypeValue: public vtkObjectBase
{
public:
  vtkBaseTypeMacro(vtkInformationIdTypeValue, vtkObjectBase);
  vtkIdType Value;
};

//----------------------------------------------------------------------------
void vtkInformationIdTypeKey::Set(vtkInformation* info, vtkIdType value)
{
  if(vtkInformationIdTypeValue* oldv =
     static_cast<vtkInformationIdTypeValue *>
     (this->GetAsObjectBase(info)))
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
    vtkInformationIdTypeValue* v = new vtkInformationIdTypeValue;
    v->InitializeObjectBase();
    v->Value = value;
    this->SetAsObjectBase(info, v);
    v->Delete();
  }
}

//----------------------------------------------------------------------------
vtkIdType vtkInformationIdTypeKey::Get(vtkInformation* info)
{
  vtkInformationIdTypeValue* v =
    static_cast<vtkInformationIdTypeValue *>
    (this->GetAsObjectBase(info));
  return v?v->Value:0;
}

//----------------------------------------------------------------------------
void vtkInformationIdTypeKey::ShallowCopy(vtkInformation* from, vtkInformation* to)
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
void vtkInformationIdTypeKey::Print(ostream& os, vtkInformation* info)
{
  // Print the value.
  if(this->Has(info))
  {
    os << this->Get(info);
  }
}

//----------------------------------------------------------------------------
vtkIdType* vtkInformationIdTypeKey::GetWatchAddress(vtkInformation* info)
{
  if(vtkInformationIdTypeValue* v =
     static_cast<vtkInformationIdTypeValue *>
     (this->GetAsObjectBase(info)))
  {
    return &v->Value;
  }
  return 0;
}
