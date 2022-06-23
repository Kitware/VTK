/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCriticalSection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Hide VTK_DEPRECATED_IN_9_1_0() warnings for this class.
// Remove with VTK_DEPRECATED_IN_9_2_0 because it was not actually deprecated
// in 9.1.0.
#define VTK_DEPRECATION_LEVEL 0

#include "vtkCriticalSection.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkCriticalSection);

void vtkCriticalSection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
