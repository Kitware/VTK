/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationStringKey.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInformationStringKey.h"

#include <vtkstd/string>

vtkCxxRevisionMacro(vtkInformationStringKey, "1.4");

//----------------------------------------------------------------------------
vtkInformationStringKey::vtkInformationStringKey(const char* name, const char* location):
  vtkInformationKey(name, location)
{
}

//----------------------------------------------------------------------------
vtkInformationStringKey::~vtkInformationStringKey()
{
}

//----------------------------------------------------------------------------
void vtkInformationStringKey::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
class vtkInformationStringValue: public vtkObjectBase
{
public:
  vtkTypeMacro(vtkInformationStringValue, vtkObjectBase);
  vtkstd::string Value;
};

//----------------------------------------------------------------------------
void vtkInformationStringKey::Set(vtkInformation* info, const char* value)
{
  if(value)
    {
    vtkInformationStringValue* v = new vtkInformationStringValue;
    this->ConstructClass("vtkInformationStringValue");
    v->Value = value;
    this->SetAsObjectBase(info, v);
    v->Delete();
    }
  else
    {
    this->SetAsObjectBase(info, 0);
    }
}

//----------------------------------------------------------------------------
const char* vtkInformationStringKey::Get(vtkInformation* info)
{
  vtkInformationStringValue* v =
    vtkInformationStringValue::SafeDownCast(this->GetAsObjectBase(info));
  return v?v->Value.c_str():0;
}

//----------------------------------------------------------------------------
int vtkInformationStringKey::Has(vtkInformation* info)
{
  vtkInformationStringValue* v =
    vtkInformationStringValue::SafeDownCast(this->GetAsObjectBase(info));
  return v?1:0;
}

//----------------------------------------------------------------------------
void vtkInformationStringKey::Copy(vtkInformation* from, vtkInformation* to)
{
  this->Set(to, this->Get(from));
}
