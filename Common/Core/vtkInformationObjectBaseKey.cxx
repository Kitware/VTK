/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationObjectBaseKey.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInformationObjectBaseKey.h"

#include "vtkInformation.h" // For vtkErrorWithObjectMacro


//----------------------------------------------------------------------------
vtkInformationObjectBaseKey
::vtkInformationObjectBaseKey(const char* name, const char* location,
                              const char* requiredClass):
  vtkInformationKey(name, location)
{
  vtkCommonInformationKeyManager::Register(this);

  this->RequiredClass = 0;
  this->SetRequiredClass(requiredClass);
}

//----------------------------------------------------------------------------
vtkInformationObjectBaseKey::~vtkInformationObjectBaseKey()
{
}

//----------------------------------------------------------------------------
void vtkInformationObjectBaseKey::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkInformationObjectBaseKey::Set(vtkInformation* info,
                                      vtkObjectBase* value)
{
  if(value && this->RequiredClass && !value->IsA(this->RequiredClass))
    {
    vtkErrorWithObjectMacro(
      info,
      "Cannot store object of type " << value->GetClassName()
      << " with key " << this->Location << "::" << this->Name
      << " which requires objects of type "
      << this->RequiredClass << ".  Removing the key instead.");
    this->SetAsObjectBase(info, 0);
    return;
    }
  this->SetAsObjectBase(info, value);
}

//----------------------------------------------------------------------------
vtkObjectBase* vtkInformationObjectBaseKey::Get(vtkInformation* info)
{
  return this->GetAsObjectBase(info);
}

//----------------------------------------------------------------------------
void vtkInformationObjectBaseKey::ShallowCopy(vtkInformation* from,
                                       vtkInformation* to)
{
  this->Set(to, this->Get(from));
}

//----------------------------------------------------------------------------
void vtkInformationObjectBaseKey::Report(vtkInformation* info,
                                         vtkGarbageCollector* collector)
{
  this->ReportAsObjectBase(info, collector);
}
