/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationRequestKey.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInformationRequestKey.h"

#include "vtkInformation.h"


//----------------------------------------------------------------------------
vtkInformationRequestKey::vtkInformationRequestKey(const char* name, const char* location):
  vtkInformationKey(name, location)
{
  vtkCommonInformationKeyManager::Register(this);
}

//----------------------------------------------------------------------------
vtkInformationRequestKey::~vtkInformationRequestKey()
{
}

//----------------------------------------------------------------------------
void vtkInformationRequestKey::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkInformationRequestKey::Set(vtkInformation* info)
{
  if (info->GetRequest() != this)
  {
    if (info->GetRequest())
    {
      vtkGenericWarningMacro("Setting request key when one is already set. Current request is " << info->GetRequest()->GetName() << " while setting " << this->GetName() << "\n");
    }
    info->SetRequest(this);
    info->Modified(this);
  }
}

//----------------------------------------------------------------------------
int vtkInformationRequestKey::Has(vtkInformation* info)
{
  return (info->GetRequest() == this)?1:0;
}

//----------------------------------------------------------------------------
void vtkInformationRequestKey::Remove(vtkInformation* info)
{
  info->SetRequest(0);
}

//----------------------------------------------------------------------------
void vtkInformationRequestKey::ShallowCopy(vtkInformation* from, vtkInformation* to)
{
  to->SetRequest(from->GetRequest());
}

//----------------------------------------------------------------------------
void vtkInformationRequestKey::Print(ostream& os, vtkInformation* info)
{
  // Print the value.
  if(this->Has(info))
  {
    os << "1\n";
  }
}
