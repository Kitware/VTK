/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLLight.h"

#include "vtkObjectFactory.h"


vtkStandardNewMacro(vtkOpenGLLight);

// Implement base class method.
void vtkOpenGLLight::Render(vtkRenderer *vtkNotUsed(ren), int vtkNotUsed(light_index))
{
  // all handled by the mappers
}

//----------------------------------------------------------------------------
void vtkOpenGLLight::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
