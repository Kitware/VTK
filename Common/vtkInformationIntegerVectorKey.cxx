/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationIntegerVectorKey.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInformationIntegerVectorKey.h"

#include <vtkstd/vector>

vtkCxxRevisionMacro(vtkInformationIntegerVectorKey, "1.3");

//----------------------------------------------------------------------------
vtkInformationIntegerVectorKey::vtkInformationIntegerVectorKey(const char* name, const char* location):
  vtkInformationKey(name, location)
{
}

//----------------------------------------------------------------------------
vtkInformationIntegerVectorKey::~vtkInformationIntegerVectorKey()
{
}

//----------------------------------------------------------------------------
void vtkInformationIntegerVectorKey::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
class vtkInformationIntegerVectorValue: public vtkObjectBase
{
public:
  vtkTypeMacro(vtkInformationIntegerVectorValue, vtkObjectBase);
  vtkstd::vector<int> Value;
};

//----------------------------------------------------------------------------
void vtkInformationIntegerVectorKey::Set(vtkInformation* info, int* value,
                                         int length)
{
  if(value)
    {
    vtkInformationIntegerVectorValue* v =
      new vtkInformationIntegerVectorValue;
    this->ConstructClass("vtkInformationIntegerVectorValue");
    v->Value.insert(v->Value.begin(), value, value+length);
    this->SetAsObjectBase(info, v);
    v->Delete();
    }
  else
    {
    this->SetAsObjectBase(info, 0);
    }
}

//----------------------------------------------------------------------------
int* vtkInformationIntegerVectorKey::Get(vtkInformation* info)
{
  vtkInformationIntegerVectorValue* v =
    vtkInformationIntegerVectorValue::SafeDownCast(
      this->GetAsObjectBase(info));
  return v?(&v->Value[0]):0;
}

//----------------------------------------------------------------------------
void vtkInformationIntegerVectorKey::Get(vtkInformation* info,
                                     int* value)
{
  vtkInformationIntegerVectorValue* v =
    vtkInformationIntegerVectorValue::SafeDownCast(
      this->GetAsObjectBase(info));
  if(v && value)
    {
    for(vtkstd::vector<int>::size_type i = 0;
        i < v->Value.size(); ++i)
      {
      value[i] = v->Value[i];
      }
    }
}

//----------------------------------------------------------------------------
int vtkInformationIntegerVectorKey::Length(vtkInformation* info)
{
  vtkInformationIntegerVectorValue* v =
    vtkInformationIntegerVectorValue::SafeDownCast(
      this->GetAsObjectBase(info));
  return v?static_cast<int>(v->Value.size()):0;
}

//----------------------------------------------------------------------------
int vtkInformationIntegerVectorKey::Has(vtkInformation* info)
{
  vtkInformationIntegerVectorValue* v =
    vtkInformationIntegerVectorValue::SafeDownCast(
      this->GetAsObjectBase(info));
  return v?1:0;
}
