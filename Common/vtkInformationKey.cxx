/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationKey.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInformationKey.h"

vtkCxxRevisionMacro(vtkInformationKey, "1.1");

//----------------------------------------------------------------------------
vtkInformationKey::vtkInformationKey()
{
}

//----------------------------------------------------------------------------
vtkInformationKey::~vtkInformationKey()
{
  this->SetReferenceCount(0);
}

//----------------------------------------------------------------------------
void vtkInformationKey::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkInformationKey::Register(vtkObjectBase*)
{
}

//----------------------------------------------------------------------------
void vtkInformationKey::UnRegister(vtkObjectBase*)
{
}
