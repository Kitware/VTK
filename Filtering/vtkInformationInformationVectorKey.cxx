/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationInformationVectorKey.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInformationInformationVectorKey.h"

#include "vtkGarbageCollector.h"
#include "vtkInformationVector.h"

vtkCxxRevisionMacro(vtkInformationInformationVectorKey, "1.3");

//----------------------------------------------------------------------------
vtkInformationInformationVectorKey::vtkInformationInformationVectorKey(const char* name, const char* location):
  vtkInformationKey(name, location)
{
  vtkFilteringInformationKeyManager::Register(this);
}

//----------------------------------------------------------------------------
vtkInformationInformationVectorKey::~vtkInformationInformationVectorKey()
{
}

//----------------------------------------------------------------------------
void vtkInformationInformationVectorKey::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkInformationInformationVectorKey::Set(vtkInformation* info,
                                             vtkInformationVector* value)
{
  this->SetAsObjectBase(info, value);
}

//----------------------------------------------------------------------------
vtkInformationVector*
vtkInformationInformationVectorKey::Get(vtkInformation* info)
{
  return vtkInformationVector::SafeDownCast(this->GetAsObjectBase(info));
}

//----------------------------------------------------------------------------
int vtkInformationInformationVectorKey::Has(vtkInformation* info)
{
  return vtkInformationVector::SafeDownCast(this->GetAsObjectBase(info))?1:0;
}

//----------------------------------------------------------------------------
void vtkInformationInformationVectorKey::Copy(vtkInformation* from,
                                              vtkInformation* to)
{
  this->Set(to, this->Get(from));
}

//----------------------------------------------------------------------------
void vtkInformationInformationVectorKey::Report(vtkInformation* info,
                                                vtkGarbageCollector* collector)
{
  collector->ReportReference(this->Get(info), this->GetName());
}
