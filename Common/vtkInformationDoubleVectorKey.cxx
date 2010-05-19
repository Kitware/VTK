/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationDoubleVectorKey.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInformationDoubleVectorKey.h"

#include "vtkInformation.h" // For vtkErrorWithObjectMacro

#include <vtkstd/vector>


//----------------------------------------------------------------------------
vtkInformationDoubleVectorKey
::vtkInformationDoubleVectorKey(const char* name, const char* location,
                                 int length):
  vtkInformationKey(name, location), RequiredLength(length)
{
  vtkCommonInformationKeyManager::Register(this);
}

//----------------------------------------------------------------------------
vtkInformationDoubleVectorKey::~vtkInformationDoubleVectorKey()
{
}

//----------------------------------------------------------------------------
void vtkInformationDoubleVectorKey::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
class vtkInformationDoubleVectorValue: public vtkObjectBase
{
public:
  vtkTypeMacro(vtkInformationDoubleVectorValue, vtkObjectBase);
  vtkstd::vector<double> Value;
};

//----------------------------------------------------------------------------
void vtkInformationDoubleVectorKey::Append(vtkInformation* info, double value)
{
  vtkInformationDoubleVectorValue* v =
    static_cast<vtkInformationDoubleVectorValue *>(
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
void vtkInformationDoubleVectorKey::Set(vtkInformation* info, double* value,
                                         int length)
{
  if(value)
    {
    if(this->RequiredLength >= 0 && length != this->RequiredLength)
      {
      vtkErrorWithObjectMacro(
        info,
        "Cannot store double vector of length " << length
        << " with key " << this->Location << "::" << this->Name
        << " which requires a vector of length "
        << this->RequiredLength << ".  Removing the key instead.");
      this->SetAsObjectBase(info, 0);
      return;
      }
    vtkInformationDoubleVectorValue* v =
      new vtkInformationDoubleVectorValue;
    this->ConstructClass("vtkInformationDoubleVectorValue");
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
double* vtkInformationDoubleVectorKey::Get(vtkInformation* info)
{
  vtkInformationDoubleVectorValue* v =
    static_cast<vtkInformationDoubleVectorValue *>(
      this->GetAsObjectBase(info));
  return (v && !v->Value.empty())?(&v->Value[0]):0;
}

//----------------------------------------------------------------------------
double vtkInformationDoubleVectorKey::Get(vtkInformation* info, int idx)
{
  if (idx >= this->Length(info))
    {
    vtkErrorWithObjectMacro(info,
                            "Information does not contain " << idx
                            << " elements. Cannot return information value.");
    return 0;
    }
  double* values = this->Get(info);
  return values[idx];
}

//----------------------------------------------------------------------------
void vtkInformationDoubleVectorKey::Get(vtkInformation* info,
                                     double* value)
{
  vtkInformationDoubleVectorValue* v =
    static_cast<vtkInformationDoubleVectorValue *>(
      this->GetAsObjectBase(info));
  if(v && value)
    {
    for(vtkstd::vector<double>::size_type i = 0;
        i < v->Value.size(); ++i)
      {
      value[i] = v->Value[i];
      }
    }
}

//----------------------------------------------------------------------------
int vtkInformationDoubleVectorKey::Length(vtkInformation* info)
{
  vtkInformationDoubleVectorValue* v =
    static_cast<vtkInformationDoubleVectorValue *>(
      this->GetAsObjectBase(info));
  return v?static_cast<int>(v->Value.size()):0;
}

//----------------------------------------------------------------------------
void vtkInformationDoubleVectorKey::ShallowCopy(vtkInformation* from,
                                          vtkInformation* to)
{
  this->Set(to, this->Get(from), this->Length(from));
}

//----------------------------------------------------------------------------
void vtkInformationDoubleVectorKey::Print(ostream& os, vtkInformation* info)
{
  // Print the value.
  if(this->Has(info))
    {
    double* value = this->Get(info);
    int length = this->Length(info);
    const char* sep = "";
    for(int i=0; i < length; ++i)
      {
      os << sep << value[i];
      sep = " ";
      }
    }
}
