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

#include "vtkInformation.h" // For vtkErrorWithObjectMacro

#include <algorithm>
#include <vector>


//----------------------------------------------------------------------------
vtkInformationIntegerVectorKey
::vtkInformationIntegerVectorKey(const char* name, const char* location,
                                 int length):
  vtkInformationKey(name, location), RequiredLength(length)
{
  vtkCommonInformationKeyManager::Register(this);
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
  vtkBaseTypeMacro(vtkInformationIntegerVectorValue, vtkObjectBase);
  std::vector<int> Value;
};

//----------------------------------------------------------------------------
void vtkInformationIntegerVectorKey::Append(vtkInformation* info, int value)
{
  vtkInformationIntegerVectorValue* v =
    static_cast<vtkInformationIntegerVectorValue *>
    (this->GetAsObjectBase(info));
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
void vtkInformationIntegerVectorKey::Set(vtkInformation* info)
{
  int someVal;
  this->Set(info, &someVal, 0);
}

//----------------------------------------------------------------------------
void vtkInformationIntegerVectorKey::Set(vtkInformation* info,
                                         const int* value,
                                         int length)
{
  if(value)
  {
    if(this->RequiredLength >= 0 && length != this->RequiredLength)
    {
      vtkErrorWithObjectMacro(
        info,
        "Cannot store integer vector of length " << length
        << " with key " << this->Location << "::" << this->Name
        << " which requires a vector of length "
        << this->RequiredLength << ".  Removing the key instead.");
      this->SetAsObjectBase(info, 0);
      return;
    }

    vtkInformationIntegerVectorValue* oldv =
      static_cast<vtkInformationIntegerVectorValue *>
      (this->GetAsObjectBase(info));
    if(oldv && static_cast<int>(oldv->Value.size()) == length)
    {
      // Replace the existing value.
      std::copy(value, value+length, oldv->Value.begin());
      // Since this sets a value without call SetAsObjectBase(),
      // the info has to be modified here (instead of
      // vtkInformation::SetAsObjectBase()
      info->Modified(this);
    }
    else
    {
      // Allocate a new value.
      vtkInformationIntegerVectorValue* v =
        new vtkInformationIntegerVectorValue;
      v->InitializeObjectBase();
      v->Value.insert(v->Value.begin(), value, value+length);
      this->SetAsObjectBase(info, v);
      v->Delete();
    }
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
    static_cast<vtkInformationIntegerVectorValue *>
    (this->GetAsObjectBase(info));
  return (v && !v->Value.empty())?(&v->Value[0]):0;
}

//----------------------------------------------------------------------------
int vtkInformationIntegerVectorKey::Get(vtkInformation* info, int idx)
{
  if (idx >= this->Length(info))
  {
    vtkErrorWithObjectMacro(info,
                            "Information does not contain " << idx
                            << " elements. Cannot return information value.");
    return 0;
  }
  int* values = this->Get(info);
  return values[idx];
}

//----------------------------------------------------------------------------
void vtkInformationIntegerVectorKey::Get(vtkInformation* info,
                                     int* value)
{
  vtkInformationIntegerVectorValue* v =
    static_cast<vtkInformationIntegerVectorValue *>
    (this->GetAsObjectBase(info));
  if(v && value)
  {
    for(std::vector<int>::size_type i = 0;
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
    static_cast<vtkInformationIntegerVectorValue *>
    (this->GetAsObjectBase(info));
  return v?static_cast<int>(v->Value.size()):0;
}

//----------------------------------------------------------------------------
void vtkInformationIntegerVectorKey::ShallowCopy(vtkInformation* from,
                                          vtkInformation* to)
{
  this->Set(to, this->Get(from), this->Length(from));
}

//----------------------------------------------------------------------------
void vtkInformationIntegerVectorKey::Print(ostream& os, vtkInformation* info)
{
  // Print the value.
  if(this->Has(info))
  {
    int* value = this->Get(info);
    int length = this->Length(info);
    const char* sep = "";
    for(int i=0; i < length; ++i)
    {
      os << sep << value[i];
      sep = " ";
    }
  }
}

//----------------------------------------------------------------------------
int* vtkInformationIntegerVectorKey::GetWatchAddress(vtkInformation* info)
{
  vtkInformationIntegerVectorValue* v =
    static_cast<vtkInformationIntegerVectorValue*>
    (this->GetAsObjectBase(info));
  return (v && !v->Value.empty())?(&v->Value[0]):0;
}
