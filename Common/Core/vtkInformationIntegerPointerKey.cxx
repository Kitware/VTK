/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationIntegerPointerKey.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInformationIntegerPointerKey.h"

#include "vtkInformation.h" // For vtkErrorWithObjectMacro

#include <algorithm>
#include <vector>


//----------------------------------------------------------------------------
vtkInformationIntegerPointerKey
::vtkInformationIntegerPointerKey(const char* name, const char* location,
                                 int length):
  vtkInformationKey(name, location), RequiredLength(length)
{
  vtkCommonInformationKeyManager::Register(this);
}

//----------------------------------------------------------------------------
vtkInformationIntegerPointerKey::~vtkInformationIntegerPointerKey()
{
}

//----------------------------------------------------------------------------
void vtkInformationIntegerPointerKey::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
class vtkInformationIntegerPointerValue: public vtkObjectBase
{
public:
  vtkBaseTypeMacro(vtkInformationIntegerPointerValue, vtkObjectBase);
  int* Value;
  unsigned int Length;
};

//----------------------------------------------------------------------------
void vtkInformationIntegerPointerKey::Set(vtkInformation* info, int* value,
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

    // Allocate a new value.
    vtkInformationIntegerPointerValue* v =
      new vtkInformationIntegerPointerValue;
    v->InitializeObjectBase();
    v->Value = value;
    v->Length = length;
    this->SetAsObjectBase(info, v);
    v->Delete();
  }
  else
  {
    this->SetAsObjectBase(info, 0);
  }
}

//----------------------------------------------------------------------------
int* vtkInformationIntegerPointerKey::Get(vtkInformation* info)
{
  vtkInformationIntegerPointerValue* v =
    static_cast<vtkInformationIntegerPointerValue *>
    (this->GetAsObjectBase(info));
  return v->Value;
}

//----------------------------------------------------------------------------
void vtkInformationIntegerPointerKey::Get(vtkInformation* info,
                                          int* value)
{
  vtkInformationIntegerPointerValue* v =
    static_cast<vtkInformationIntegerPointerValue *>
    (this->GetAsObjectBase(info));
  if(v && value)
  {
    memcpy(value, v->Value, v->Length*sizeof(int));
  }
}

//----------------------------------------------------------------------------
int vtkInformationIntegerPointerKey::Length(vtkInformation* info)
{
  vtkInformationIntegerPointerValue* v =
    static_cast<vtkInformationIntegerPointerValue *>
    (this->GetAsObjectBase(info));
  return v->Length;
}

//----------------------------------------------------------------------------
void vtkInformationIntegerPointerKey::ShallowCopy(vtkInformation* from,
                                                  vtkInformation* to)
{
  this->Set(to, this->Get(from), this->Length(from));
}

//----------------------------------------------------------------------------
void vtkInformationIntegerPointerKey::Print(ostream& os, vtkInformation* info)
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
int* vtkInformationIntegerPointerKey::GetWatchAddress(vtkInformation* info)
{
  if(vtkInformationIntegerPointerValue* v =
     static_cast<vtkInformationIntegerPointerValue *>
     (this->GetAsObjectBase(info)))
  {
    return v->Value;
  }
  return 0;
}
