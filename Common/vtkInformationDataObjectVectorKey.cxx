/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationDataObjectVectorKey.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInformationDataObjectVectorKey.h"

#include "vtkDataObject.h"
#include "vtkSmartPointer.h"

#include <vtkstd/vector>

vtkCxxRevisionMacro(vtkInformationDataObjectVectorKey, "1.3");

//----------------------------------------------------------------------------
vtkInformationDataObjectVectorKey::vtkInformationDataObjectVectorKey(const char* name, const char* location):
  vtkInformationKey(name, location)
{
}

//----------------------------------------------------------------------------
vtkInformationDataObjectVectorKey::~vtkInformationDataObjectVectorKey()
{
}

//----------------------------------------------------------------------------
void vtkInformationDataObjectVectorKey::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
class vtkInformationDataObjectVectorValue: public vtkObjectBase
{
public:
  vtkTypeMacro(vtkInformationDataObjectVectorValue, vtkObjectBase);
  vtkstd::vector< vtkSmartPointer<vtkDataObject> > References;
  vtkstd::vector< vtkDataObject* > Value;
};

//----------------------------------------------------------------------------
void vtkInformationDataObjectVectorKey::Set(vtkInformation* info,
                                            vtkDataObject** value, int length)
{
  if(value)
    {
    vtkInformationDataObjectVectorValue* v =
      new vtkInformationDataObjectVectorValue;
    this->ConstructClass("vtkInformationDataObjectVectorValue");
    v->Value.insert(v->Value.begin(), value, value+length);
    for(vtkstd::vector<vtkDataObject*>::size_type i = 0;
        i < v->Value.size(); ++i)
      {
      v->References.push_back(v->Value[i]);
      }
    this->SetAsObjectBase(info, v);
    v->Delete();
    }
  else
    {
    this->SetAsObjectBase(info, 0);
    }
}

//----------------------------------------------------------------------------
vtkDataObject** vtkInformationDataObjectVectorKey::Get(vtkInformation* info)
{
  vtkInformationDataObjectVectorValue* v =
    vtkInformationDataObjectVectorValue::SafeDownCast(
      this->GetAsObjectBase(info));
  return v?(&v->Value[0]):0;
}

//----------------------------------------------------------------------------
void vtkInformationDataObjectVectorKey::Get(vtkInformation* info,
                                            vtkDataObject** value)
{
  vtkInformationDataObjectVectorValue* v =
    vtkInformationDataObjectVectorValue::SafeDownCast(
      this->GetAsObjectBase(info));
  if(v && value)
    {
    for(vtkstd::vector<vtkDataObject*>::size_type i = 0;
        i < v->Value.size(); ++i)
      {
      value[i] = v->Value[i];
      }
    }
}

//----------------------------------------------------------------------------
int vtkInformationDataObjectVectorKey::Length(vtkInformation* info)
{
  vtkInformationDataObjectVectorValue* v =
    vtkInformationDataObjectVectorValue::SafeDownCast(
      this->GetAsObjectBase(info));
  return v?static_cast<int>(v->Value.size()):0;
}

//----------------------------------------------------------------------------
int vtkInformationDataObjectVectorKey::Has(vtkInformation* info)
{
  vtkInformationDataObjectVectorValue* v =
    vtkInformationDataObjectVectorValue::SafeDownCast(
      this->GetAsObjectBase(info));
  return v?1:0;
}
