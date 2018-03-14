/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPExtractArraysOverTime.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPExtractArraysOverTime.h"

#include "vtkObjectFactory.h"

#ifndef VTK_LEGACY_REMOVE
vtkStandardNewMacro(vtkPExtractArraysOverTime);
//----------------------------------------------------------------------------
vtkPExtractArraysOverTime::vtkPExtractArraysOverTime()
{
  VTK_LEGACY_REPLACED_BODY(vtkPExtractArraysOverTime, "VTK 8.2", vtkPExtractSelectedArraysOverTime);
}

//----------------------------------------------------------------------------
vtkPExtractArraysOverTime::~vtkPExtractArraysOverTime() = default;

//----------------------------------------------------------------------------
void vtkPExtractArraysOverTime::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
#endif // VTK_LEGACY_REMOVE
