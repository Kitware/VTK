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

vtkCxxRevisionMacro(vtkInformationInformationKey, "1.1");

//----------------------------------------------------------------------------
vtkInformationInformationKey::vtkInformationInformationKey(const char* name, const char* location):
  vtkInformationKey(name, location)
{
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
  return vtkInformation::SafeDownCast(this->GetAsObjectBase(info));
}

//----------------------------------------------------------------------------
int vtkInformationInformationKey::Has(vtkInformation* info)
{
  return vtkInformation::SafeDownCast(this->GetAsObjectBase(info))?1:0;
}

//----------------------------------------------------------------------------
void vtkInformationInformationKey::Copy(vtkInformation* from,
                                        vtkInformation* to)
{
  this->Set(to, this->Get(from));
}
