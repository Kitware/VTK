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

#include "vtkInformation.h"
#include <vector>
#include <algorithm> // find()


//----------------------------------------------------------------------------
vtkInformationKeyVectorKey::vtkInformationKeyVectorKey(const char* name, const char* location):
  vtkInformationKey(name, location)
{
  vtkCommonInformationKeyManager::Register(this);
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
  vtkBaseTypeMacro(vtkInformationKeyVectorValue, vtkObjectBase);
  std::vector<vtkInformationKey*> Value;
};

//----------------------------------------------------------------------------
void vtkInformationKeyVectorKey::Append(vtkInformation* info,
                                        vtkInformationKey* value)
{
  vtkInformationKeyVectorValue* v =
    static_cast<vtkInformationKeyVectorValue *>
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
void vtkInformationKeyVectorKey::AppendUnique(vtkInformation* info,
                                              vtkInformationKey* value)
{
  vtkInformationKeyVectorValue* v =
    static_cast<vtkInformationKeyVectorValue *>
    (this->GetAsObjectBase(info));
  if(v)
  {
    int found = 0;
    size_t len = v->Value.size();
    for (size_t i=0; i<len; i++)
    {
      if (v->Value[i] == value)
      {
        found = 1;
        break;
      }
    }
    if (!found)
    {
      v->Value.push_back(value);
    }
  }
  else
  {
    this->Set(info, &value, 1);
  }
}

//----------------------------------------------------------------------------
void vtkInformationKeyVectorKey::Set(vtkInformation* info,
                                     vtkInformationKey* const*
                                     value, int length)
{
  if(value)
  {
    vtkInformationKeyVectorValue* v = new vtkInformationKeyVectorValue;
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
void vtkInformationKeyVectorKey::RemoveItem(vtkInformation* info,
                                            vtkInformationKey* value)
{
  vtkInformationKeyVectorValue* v =
    static_cast<vtkInformationKeyVectorValue *>
    (this->GetAsObjectBase(info));

  if(v)
  {
    std::vector<vtkInformationKey*>::iterator it=std::find(v->Value.begin(),v->Value.end(),value);
    if(it!=v->Value.end())
    {
      v->Value.erase(it);
    }
  }
}

//----------------------------------------------------------------------------
vtkInformationKey** vtkInformationKeyVectorKey::Get(vtkInformation* info)
{
  vtkInformationKeyVectorValue* v =
    static_cast<vtkInformationKeyVectorValue *>
    (this->GetAsObjectBase(info));
  return (v && !v->Value.empty())?(&v->Value[0]):0;
}

//----------------------------------------------------------------------------
vtkInformationKey* vtkInformationKeyVectorKey::Get(vtkInformation* info,
                                                   int idx)
{
  if (idx >= this->Length(info))
  {
    vtkErrorWithObjectMacro(info,
                            "Information does not contain " << idx
                            << " elements. Cannot return information value.");
    return 0;
  }
  vtkInformationKey** values = this->Get(info);
  return values[idx];
}

//----------------------------------------------------------------------------
void vtkInformationKeyVectorKey::Get(vtkInformation* info,
                                     vtkInformationKey** value)
{
  vtkInformationKeyVectorValue* v =
    static_cast<vtkInformationKeyVectorValue *>
    (this->GetAsObjectBase(info));
  if(v && value)
  {
    for(std::vector<vtkInformationKey*>::size_type i = 0;
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
    static_cast<vtkInformationKeyVectorValue *>
    (this->GetAsObjectBase(info));
  return v?static_cast<int>(v->Value.size()):0;
}

//----------------------------------------------------------------------------
void vtkInformationKeyVectorKey::ShallowCopy(vtkInformation* from, vtkInformation* to)
{
  this->Set(to, this->Get(from), this->Length(from));
}

//----------------------------------------------------------------------------
void vtkInformationKeyVectorKey::Print(ostream& os, vtkInformation* info)
{
  // Print the value.
  if(this->Has(info))
  {
    vtkInformationKey** value = this->Get(info);
    int length = this->Length(info);
    const char* sep = "";
    for(int i=0; i < length; ++i)
    {
      os << sep << (value[i]? value[i]->GetName() : "(NULL)");
      sep = " ";
    }
  }
}
