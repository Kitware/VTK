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

vtkCxxRevisionMacro(vtkInformationDataObjectVectorKey, "1.1");

//----------------------------------------------------------------------------
vtkInformationDataObjectVectorKey::vtkInformationDataObjectVectorKey()
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
