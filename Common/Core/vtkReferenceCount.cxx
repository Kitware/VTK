/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkReferenceCount.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkReferenceCount.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkReferenceCount);

vtkReferenceCount::vtkReferenceCount() 
{
}

vtkReferenceCount::~vtkReferenceCount() 
{
}

void vtkReferenceCount::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
