/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationKeyVectorKey.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInformationKeyVectorKey.h"

#include <vtkstd/vector>

vtkCxxRevisionMacro(vtkInformationKeyVectorKey, "1.1");

//----------------------------------------------------------------------------
vtkInformationKeyVectorKey::vtkInformationKeyVectorKey(const char* name, const char* location):
  vtkInformationKey(name, location)
{
}

//----------------------------------------------------------------------------
vtkInformationKeyVectorKey::~vtkInformationKeyVectorKey()
{
}

//----------------------------------------------------------------------------
void vtkInformationKeyVectorKey::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
class vtkInformationKeyVectorValue: public vtkObjectBase
{
public:
  vtkTypeMacro(vtkInformationKeyVectorValue, vtkObjectBase);
  vtkstd::vector<vtkInformationKey*> Value;
};

//----------------------------------------------------------------------------
void vtkInformationKeyVectorKey::Append(vtkInformation* info,
                                        vtkInformationKey* value)
{
  vtkInformationKeyVectorValue* v =
    vtkInformationKeyVectorValue::SafeDownCast(
      this->GetAsObjectBase(info));
  if(v)
    {
    v->Value.push_back(value);
    }
  else
    {
    this->Set(info, &value, 1);
    }
}

//----------------------------------------------------------------------------
void vtkInformationKeyVectorKey::Set(vtkInformation* info,
                                     vtkInformationKey** value, int length)
{
  if(value)
    {
    vtkInformationKeyVectorValue* v =
      new vtkInformationKeyVectorValue;
    this->ConstructClass("vtkInformationKeyVectorValue");
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
vtkInformationKey** vtkInformationKeyVectorKey::Get(vtkInformation* info)
{
  vtkInformationKeyVectorValue* v =
    vtkInformationKeyVectorValue::SafeDownCast(
      this->GetAsObjectBase(info));
  return v?(&v->Value[0]):0;
}

//----------------------------------------------------------------------------
void vtkInformationKeyVectorKey::Get(vtkInformation* info,
                                     vtkInformationKey** value)
{
  vtkInformationKeyVectorValue* v =
    vtkInformationKeyVectorValue::SafeDownCast(
      this->GetAsObjectBase(info));
  if(v && value)
    {
    for(vtkstd::vector<vtkInformationKey*>::size_type i = 0;
        i < v->Value.size(); ++i)
      {
      value[i] = v->Value[i];
      }
    }
}

//----------------------------------------------------------------------------
int vtkInformationKeyVectorKey::Length(vtkInformation* info)
{
  vtkInformationKeyVectorValue* v =
    vtkInformationKeyVectorValue::SafeDownCast(
      this->GetAsObjectBase(info));
  return v?static_cast<int>(v->Value.size()):0;
}

//----------------------------------------------------------------------------
int vtkInformationKeyVectorKey::Has(vtkInformation* info)
{
  vtkInformationKeyVectorValue* v =
    vtkInformationKeyVectorValue::SafeDownCast(
      this->GetAsObjectBase(info));
  return v?1:0;
}

//----------------------------------------------------------------------------
void vtkInformationKeyVectorKey::Copy(vtkInformation* from, vtkInformation* to)
{
  this->Set(to, this->Get(from), this->Length(from));
}
