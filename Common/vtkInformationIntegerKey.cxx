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

vtkCxxRevisionMacro(vtkInformationIntegerKey, "1.4");

//----------------------------------------------------------------------------
vtkInformationIntegerKey::vtkInformationIntegerKey(const char* name, const char* location):
  vtkInformationKey(name, location)
{
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
  vtkInformationIntegerValue* v = new vtkInformationIntegerValue;
  this->ConstructClass("vtkInformationIntegerValue");
  v->Value = value;
  this->SetAsObjectBase(info, v);
  v->Delete();
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
void vtkInformationIntegerKey::Copy(vtkInformation* from, vtkInformation* to)
{
  this->Set(to, this->Get(from));
}
