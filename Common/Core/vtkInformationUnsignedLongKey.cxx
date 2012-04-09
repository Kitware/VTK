/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationUnsignedLongKey.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInformationUnsignedLongKey.h"

#include "vtkInformation.h"


//----------------------------------------------------------------------------
vtkInformationUnsignedLongKey::vtkInformationUnsignedLongKey(const char* name, const char* location):
  vtkInformationKey(name, location)
{
  vtkCommonInformationKeyManager::Register(this);
}

//----------------------------------------------------------------------------
vtkInformationUnsignedLongKey::~vtkInformationUnsignedLongKey()
{
}

//----------------------------------------------------------------------------
void vtkInformationUnsignedLongKey::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
class vtkInformationUnsignedLongValue: public vtkObjectBase
{
public:
  vtkTypeMacro(vtkInformationUnsignedLongValue, vtkObjectBase);
  unsigned long Value;
};

//----------------------------------------------------------------------------
void vtkInformationUnsignedLongKey::Set(vtkInformation* info,
                                        unsigned long value)
{
  if(vtkInformationUnsignedLongValue* oldv =
     static_cast<vtkInformationUnsignedLongValue *>
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
    vtkInformationUnsignedLongValue* v = new vtkInformationUnsignedLongValue;
    this->ConstructClass("vtkInformationUnsignedLongValue");
    v->Value = value;
    this->SetAsObjectBase(info, v);
    v->Delete();
    }
}

//----------------------------------------------------------------------------
unsigned long vtkInformationUnsignedLongKey::Get(vtkInformation* info)
{
  vtkInformationUnsignedLongValue* v =
    static_cast<vtkInformationUnsignedLongValue *>
    (this->GetAsObjectBase(info));
  return v?v->Value:0;
}

//----------------------------------------------------------------------------
void vtkInformationUnsignedLongKey::ShallowCopy(vtkInformation* from,
                                         vtkInformation* to)
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
void vtkInformationUnsignedLongKey::Print(ostream& os, vtkInformation* info)
{
  // Print the value.
  if(this->Has(info))
    {
    os << this->Get(info);
    }
}

//----------------------------------------------------------------------------
unsigned long*
vtkInformationUnsignedLongKey::GetWatchAddress(vtkInformation* info)
{
  if(vtkInformationUnsignedLongValue* v =
     static_cast<vtkInformationUnsignedLongValue *>
     (this->GetAsObjectBase(info)))
    {
    return &v->Value;
    }
  return 0;
}
