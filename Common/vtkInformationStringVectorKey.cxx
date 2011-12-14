/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationStringVectorKey.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInformationStringVectorKey.h"

#include "vtkInformation.h" // For vtkErrorWithObjectMacro
#include "vtkStdString.h"

#include <algorithm>
#include <vector>


//----------------------------------------------------------------------------
vtkInformationStringVectorKey
::vtkInformationStringVectorKey(const char* name, const char* location,
                                 int length):
  vtkInformationKey(name, location), RequiredLength(length)
{
  vtkCommonInformationKeyManager::Register(this);
}

//----------------------------------------------------------------------------
vtkInformationStringVectorKey::~vtkInformationStringVectorKey()
{
}

//----------------------------------------------------------------------------
void vtkInformationStringVectorKey::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
class vtkInformationStringVectorValue: public vtkObjectBase
{
public:
  vtkTypeMacro(vtkInformationStringVectorValue, vtkObjectBase);
  std::vector<std::string> Value;
};

//----------------------------------------------------------------------------
void vtkInformationStringVectorKey::Append(vtkInformation* info, const char* value)
{
  vtkInformationStringVectorValue* v =
    static_cast<vtkInformationStringVectorValue *>
    (this->GetAsObjectBase(info));
  if(v)
    {
    v->Value.push_back(value);
    }
  else
    {
    this->Set(info, value, 0);
    }
}

//----------------------------------------------------------------------------
void vtkInformationStringVectorKey::Set(vtkInformation* info, const char* value,
                                        int index)
{
  vtkInformationStringVectorValue* oldv =
    static_cast<vtkInformationStringVectorValue *>
    (this->GetAsObjectBase(info));
  if(oldv)
    {
    if (   (static_cast<int>(oldv->Value.size()) <= index)
        || (oldv->Value[index] != value))
      {
      while(static_cast<int>(oldv->Value.size()) <= index)
        {
        oldv->Value.push_back("");
        }
      oldv->Value[index] = value;
      // Since this sets a value without call SetAsObjectBase(),
      // the info has to be modified here (instead of 
      // vtkInformation::SetAsObjectBase()
      info->Modified(this);
      }
    }
  else
    {
    vtkInformationStringVectorValue* v =
      new vtkInformationStringVectorValue;
    this->ConstructClass("vtkInformationStringVectorValue");
    while(static_cast<int>(v->Value.size()) <= index)
      {
      v->Value.push_back("");
      }
    v->Value[index] = value;
    this->SetAsObjectBase(info, v);
    v->Delete();
    }  
}

//----------------------------------------------------------------------------
const char* vtkInformationStringVectorKey::Get(vtkInformation* info, int idx)
{
  if (idx < 0 || idx >= this->Length(info))
    {
    return 0;
    }
  vtkInformationStringVectorValue* v =
    static_cast<vtkInformationStringVectorValue *>
    (this->GetAsObjectBase(info));
  return v->Value[idx].c_str();
}

//----------------------------------------------------------------------------
int vtkInformationStringVectorKey::Length(vtkInformation* info)
{
  vtkInformationStringVectorValue* v =
    static_cast<vtkInformationStringVectorValue *>
    (this->GetAsObjectBase(info));
  return v?static_cast<int>(v->Value.size()):0;
}

//----------------------------------------------------------------------------
void vtkInformationStringVectorKey::ShallowCopy(vtkInformation* from,
                                                vtkInformation* to)
{
  int length = this->Length(from);
  for(int i = 0; i < length; ++i)
    {
    this->Set(to, this->Get(from, i), i);
    }
}

//----------------------------------------------------------------------------
void vtkInformationStringVectorKey::Print(ostream& os, vtkInformation* info)
{
  // Print the value.
  if(this->Has(info))
    {
    int length = this->Length(info);
    const char* sep = "";
    for(int i=0; i < length; ++i)
      {
      os << sep << this->Get(info, i);
      sep = " ";
      }
    }
}
