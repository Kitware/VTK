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

vtkCxxRevisionMacro(vtkInformationUnsignedLongKey, "1.7");

//----------------------------------------------------------------------------
vtkInformationUnsignedLongKey::vtkInformationUnsignedLongKey(const char* name, const char* location):
  vtkInformationKey(name, location)
{
  vtkFilteringInformationKeyManager::Register(this);
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
     vtkInformationUnsignedLongValue::SafeDownCast(
       this->GetAsObjectBase(info)))
    {
    // Replace the existing value.
    oldv->Value = value;
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
    vtkInformationUnsignedLongValue::SafeDownCast(
      this->GetAsObjectBase(info));
  return v?v->Value:0;
}

//----------------------------------------------------------------------------
int vtkInformationUnsignedLongKey::Has(vtkInformation* info)
{
  vtkInformationUnsignedLongValue* v =
    vtkInformationUnsignedLongValue::SafeDownCast(
      this->GetAsObjectBase(info));
  return v?1:0;
}

//----------------------------------------------------------------------------
void vtkInformationUnsignedLongKey::ShallowCopy(vtkInformation* from,
                                         vtkInformation* to)
{
  this->Set(to, this->Get(from));
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
     vtkInformationUnsignedLongValue::SafeDownCast(
       this->GetAsObjectBase(info)))
    {
    return &v->Value;
    }
  return 0;
}
