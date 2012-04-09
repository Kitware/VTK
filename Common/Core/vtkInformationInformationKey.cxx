/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationInformationKey.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInformationInformationKey.h"

#include "vtkInformation.h"


//----------------------------------------------------------------------------
vtkInformationInformationKey::vtkInformationInformationKey(const char* name, const char* location):
  vtkInformationKey(name, location)
{
  vtkCommonInformationKeyManager::Register(this);
}

//----------------------------------------------------------------------------
vtkInformationInformationKey::~vtkInformationInformationKey()
{
}

//----------------------------------------------------------------------------
void vtkInformationInformationKey::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkInformationInformationKey::Set(vtkInformation* info,
                                      vtkInformation* value)
{
  this->SetAsObjectBase(info, value);
}

//----------------------------------------------------------------------------
vtkInformation* vtkInformationInformationKey::Get(vtkInformation* info)
{
  return static_cast<vtkInformation *>(this->GetAsObjectBase(info));
}

//----------------------------------------------------------------------------
void vtkInformationInformationKey::ShallowCopy(vtkInformation* from,
                                        vtkInformation* to)
{
  this->Set(to, this->Get(from));
}

//----------------------------------------------------------------------------
void vtkInformationInformationKey::DeepCopy(vtkInformation* from,
                                        vtkInformation* to)
{
  vtkInformation *toInfo = vtkInformation::New();
  toInfo->Copy(this->Get(from), 1);
  this->Set(to, toInfo);
  toInfo->Delete();
}
