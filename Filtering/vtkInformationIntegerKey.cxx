/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationIntegerKey.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInformationIntegerKey.h"

vtkCxxRevisionMacro(vtkInformationIntegerKey, "1.8");

//----------------------------------------------------------------------------
vtkInformationIntegerKey::vtkInformationIntegerKey(const char* name, const char* location):
  vtkInformationKey(name, location)
{
  vtkFilteringInformationKeyManager::Register(this);
}

//----------------------------------------------------------------------------
vtkInformationIntegerKey::~vtkInformationIntegerKey()
{
}

//----------------------------------------------------------------------------
void vtkInformationIntegerKey::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
class vtkInformationIntegerValue: public vtkObjectBase
{
public:
  vtkTypeMacro(vtkInformationIntegerValue, vtkObjectBase);
  int Value;
};

//----------------------------------------------------------------------------
void vtkInformationIntegerKey::Set(vtkInformation* info, int value)
{
  if(vtkInformationIntegerValue* oldv =
     vtkInformationIntegerValue::SafeDownCast(
       this->GetAsObjectBase(info)))
    {
    // Replace the existing value.
    oldv->Value = value;
    }
  else
    {
    // Allocate a new value.
    vtkInformationIntegerValue* v = new vtkInformationIntegerValue;
    this->ConstructClass("vtkInformationIntegerValue");
    v->Value = value;
    this->SetAsObjectBase(info, v);
    v->Delete();
    }
}

//----------------------------------------------------------------------------
int vtkInformationIntegerKey::Get(vtkInformation* info)
{
  vtkInformationIntegerValue* v =
    vtkInformationIntegerValue::SafeDownCast(
      this->GetAsObjectBase(info));
  return v?v->Value:0;
}

//----------------------------------------------------------------------------
int vtkInformationIntegerKey::Has(vtkInformation* info)
{
  vtkInformationIntegerValue* v =
    vtkInformationIntegerValue::SafeDownCast(
      this->GetAsObjectBase(info));
  return v?1:0;
}

//----------------------------------------------------------------------------
void vtkInformationIntegerKey::ShallowCopy(vtkInformation* from, vtkInformation* to)
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
void vtkInformationIntegerKey::Print(ostream& os, vtkInformation* info)
{
  // Print the value.
  if(this->Has(info))
    {
    os << this->Get(info);
    }
}

//----------------------------------------------------------------------------
int* vtkInformationIntegerKey::GetWatchAddress(vtkInformation* info)
{
  if(vtkInformationIntegerValue* v =
     vtkInformationIntegerValue::SafeDownCast(
       this->GetAsObjectBase(info)))
    {
    return &v->Value;
    }
  return 0;
}
