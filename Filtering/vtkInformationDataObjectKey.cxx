/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationDataObjectKey.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInformationDataObjectKey.h"

#include "vtkDataObject.h"

vtkCxxRevisionMacro(vtkInformationDataObjectKey, "1.1");

//----------------------------------------------------------------------------
vtkInformationDataObjectKey::vtkInformationDataObjectKey(const char* name, const char* location):
  vtkInformationKey(name, location)
{
}

//----------------------------------------------------------------------------
vtkInformationDataObjectKey::~vtkInformationDataObjectKey()
{
}

//----------------------------------------------------------------------------
void vtkInformationDataObjectKey::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkInformationDataObjectKey::Set(vtkInformation* info,
                                      vtkDataObject* value)
{
  this->SetAsObjectBase(info, value);
}

//----------------------------------------------------------------------------
vtkDataObject* vtkInformationDataObjectKey::Get(vtkInformation* info)
{
  return vtkDataObject::SafeDownCast(this->GetAsObjectBase(info));
}

//----------------------------------------------------------------------------
int vtkInformationDataObjectKey::Has(vtkInformation* info)
{
  return vtkDataObject::SafeDownCast(this->GetAsObjectBase(info))?1:0;
}

//----------------------------------------------------------------------------
void vtkInformationDataObjectKey::Copy(vtkInformation* from,
                                       vtkInformation* to)
{
  this->Set(to, this->Get(from));
}
