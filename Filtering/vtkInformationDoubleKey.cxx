/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationDoubleKey.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInformationDoubleKey.h"

vtkCxxRevisionMacro(vtkInformationDoubleKey, "1.1");

//----------------------------------------------------------------------------
vtkInformationDoubleKey::vtkInformationDoubleKey(const char* name, const char* location):
  vtkInformationKey(name, location)
{
  vtkFilteringInformationKeyManager::Register(this);
}

//----------------------------------------------------------------------------
vtkInformationDoubleKey::~vtkInformationDoubleKey()
{
}

//----------------------------------------------------------------------------
void vtkInformationDoubleKey::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
class vtkInformationDoubleValue: public vtkObjectBase
{
public:
  vtkTypeMacro(vtkInformationDoubleValue, vtkObjectBase);
  double Value;
};

//----------------------------------------------------------------------------
void vtkInformationDoubleKey::Set(vtkInformation* info, double value)
{
  if(vtkInformationDoubleValue* oldv =
     vtkInformationDoubleValue::SafeDownCast(
       this->GetAsObjectBase(info)))
    {
    // Replace the existing value.
    oldv->Value = value;
    }
  else
    {
    // Allocate a new value.
    vtkInformationDoubleValue* v = new vtkInformationDoubleValue;
    this->ConstructClass("vtkInformationDoubleValue");
    v->Value = value;
    this->SetAsObjectBase(info, v);
    v->Delete();
    }
}

//----------------------------------------------------------------------------
double vtkInformationDoubleKey::Get(vtkInformation* info)
{
  vtkInformationDoubleValue* v =
    vtkInformationDoubleValue::SafeDownCast(
      this->GetAsObjectBase(info));
  return v?v->Value:0;
}

//----------------------------------------------------------------------------
int vtkInformationDoubleKey::Has(vtkInformation* info)
{
  vtkInformationDoubleValue* v =
    vtkInformationDoubleValue::SafeDownCast(
      this->GetAsObjectBase(info));
  return v?1:0;
}

//----------------------------------------------------------------------------
void vtkInformationDoubleKey::Copy(vtkInformation* from, vtkInformation* to)
{
  this->Set(to, this->Get(from));
}

//----------------------------------------------------------------------------
double* vtkInformationDoubleKey::GetWatchAddress(vtkInformation* info)
{
  if(vtkInformationDoubleValue* v =
     vtkInformationDoubleValue::SafeDownCast(
       this->GetAsObjectBase(info)))
    {
    return &v->Value;
    }
  return 0;
}
