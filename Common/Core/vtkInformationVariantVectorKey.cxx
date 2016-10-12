/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationVariantVectorKey.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInformationVariantVectorKey.h"

#include "vtkInformation.h" // For vtkErrorWithObjectMacro
#include "vtkVariant.h"

#include <vector>


//----------------------------------------------------------------------------
vtkInformationVariantVectorKey
::vtkInformationVariantVectorKey(const char* name, const char* location,
                                 int length):
  vtkInformationKey(name, location), RequiredLength(length)
{
  vtkCommonInformationKeyManager::Register(this);
}

//----------------------------------------------------------------------------
vtkInformationVariantVectorKey::~vtkInformationVariantVectorKey()
{
}

//----------------------------------------------------------------------------
void vtkInformationVariantVectorKey::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
class vtkInformationVariantVectorValue: public vtkObjectBase
{
public:
  vtkBaseTypeMacro(vtkInformationVariantVectorValue, vtkObjectBase);
  std::vector<vtkVariant> Value;
  static vtkVariant Invalid;
};

vtkVariant vtkInformationVariantVectorValue::Invalid;

//----------------------------------------------------------------------------
void vtkInformationVariantVectorKey::Append(vtkInformation* info, const vtkVariant& value)
{
  vtkInformationVariantVectorValue* v =
    static_cast<vtkInformationVariantVectorValue *>(
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
void vtkInformationVariantVectorKey::Set(vtkInformation* info, const vtkVariant* value,
                                         int length)
{
  if(value)
  {
    if(this->RequiredLength >= 0 && length != this->RequiredLength)
    {
      vtkErrorWithObjectMacro(
        info,
        "Cannot store vtkVariant vector of length " << length
        << " with key " << this->Location << "::" << this->Name
        << " which requires a vector of length "
        << this->RequiredLength << ".  Removing the key instead.");
      this->SetAsObjectBase(info, 0);
      return;
    }
    vtkInformationVariantVectorValue* v =
      new vtkInformationVariantVectorValue;
    v->InitializeObjectBase();
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
const vtkVariant* vtkInformationVariantVectorKey::Get(
  vtkInformation* info) const
{
  const vtkInformationVariantVectorValue* v =
    static_cast<const vtkInformationVariantVectorValue *>(
      this->GetAsObjectBase(info));
  return (v && !v->Value.empty())?(&v->Value[0]):0;
}

//----------------------------------------------------------------------------
const vtkVariant& vtkInformationVariantVectorKey::Get(
  vtkInformation* info, int idx) const
{
  if (idx >= this->Length(info))
  {
    vtkErrorWithObjectMacro(info,
                            "Information does not contain " << idx
                            << " elements. Cannot return information value.");
    return vtkInformationVariantVectorValue::Invalid;
  }
  const vtkVariant* values = this->Get(info);
  return values[idx];
}

//----------------------------------------------------------------------------
void vtkInformationVariantVectorKey::Get(vtkInformation* info,
                                     vtkVariant* value) const
{
  const vtkInformationVariantVectorValue* v =
    static_cast<const vtkInformationVariantVectorValue *>(
      this->GetAsObjectBase(info));
  if(v && value)
  {
    for(std::vector<vtkVariant>::size_type i = 0;
        i < v->Value.size(); ++i)
    {
      value[i] = v->Value[i];
    }
  }
}

//----------------------------------------------------------------------------
int vtkInformationVariantVectorKey::Length(vtkInformation* info) const
{
  const vtkInformationVariantVectorValue* v =
    static_cast<const vtkInformationVariantVectorValue *>(
      this->GetAsObjectBase(info));
  return v?static_cast<int>(v->Value.size()):0;
}

//----------------------------------------------------------------------------
void vtkInformationVariantVectorKey::ShallowCopy(vtkInformation* from,
                                          vtkInformation* to)
{
  this->Set(to, this->Get(from), this->Length(from));
}

//----------------------------------------------------------------------------
void vtkInformationVariantVectorKey::Print(ostream& os, vtkInformation* info)
{
  // Print the value.
  if(this->Has(info))
  {
    const vtkVariant* value = this->Get(info);
    int length = this->Length(info);
    const char* sep = "";
    for(int i=0; i < length; ++i)
    {
      os << sep << value[i];
      sep = " ";
    }
  }
}
