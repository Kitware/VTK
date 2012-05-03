/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationDoubleKey.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInformationDoubleKey.h"

#include "vtkInformation.h"


//----------------------------------------------------------------------------
vtkInformationDoubleKey::vtkInformationDoubleKey(const char* name, const char* location):
  vtkInformationKey(name, location)
{
  vtkCommonInformationKeyManager::Register(this);
}

//----------------------------------------------------------------------------
vtkInformationDoubleKey::~vtkInformationDoubleKey()
{
}

//----------------------------------------------------------------------------
void vtkInformationDoubleKey::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
class vtkInformationDoubleValue: public vtkObjectBase
{
public:
  vtkTypeMacro(vtkInformationDoubleValue, vtkObjectBase);
  double Value;
};

//----------------------------------------------------------------------------
void vtkInformationDoubleKey::Set(vtkInformation* info, double value)
{
  if(vtkInformationDoubleValue* oldv =
     static_cast<vtkInformationDoubleValue *>(
       this->GetAsObjectBase(info)))
    {
    if (oldv->Value != value)
      {
      // Replace the existing value.
      oldv->Value = value;
      // Since this sets a value without call SetAsObjectBase(),
      // the info has to be modified here (instead of
      // vtkInformation::SetAsObjectBase()
      info->Modified(this);
      }
    }
  else
    {
    // Allocate a new value.
    vtkInformationDoubleValue* v = new vtkInformationDoubleValue;
    this->ConstructClass("vtkInformationDoubleValue");
    v->Value = value;
    this->SetAsObjectBase(info, v);
    v->Delete();
    }
}

//----------------------------------------------------------------------------
double vtkInformationDoubleKey::Get(vtkInformation* info)
{
  vtkInformationDoubleValue* v =
    static_cast<vtkInformationDoubleValue *>(
      this->GetAsObjectBase(info));
  return v?v->Value:0;
}

//----------------------------------------------------------------------------
void vtkInformationDoubleKey::ShallowCopy(vtkInformation* from, vtkInformation* to)
{
  if (this->Has(from))
    {
    this->Set(to, this->Get(from));
    }
  else
    {
    this->SetAsObjectBase(to, 0); // doesn't exist in from, so remove the key
    }
}

//----------------------------------------------------------------------------
void vtkInformationDoubleKey::Print(ostream& os, vtkInformation* info)
{
  // Print the value.
  if(this->Has(info))
    {
    os << this->Get(info);
    }
}

//----------------------------------------------------------------------------
double* vtkInformationDoubleKey::GetWatchAddress(vtkInformation* info)
{
  if(vtkInformationDoubleValue* v =
     static_cast<vtkInformationDoubleValue *>(
       this->GetAsObjectBase(info)))
    {
    return &v->Value;
    }
  return 0;
}
