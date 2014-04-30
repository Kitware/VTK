/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGL2Light.h"

#include "vtkObjectFactory.h"


vtkStandardNewMacro(vtkOpenGL2Light);

// Implement base class method.
void vtkOpenGL2Light::Render(vtkRenderer *vtkNotUsed(ren), int vtkNotUsed(light_index))
{
  // all handled by the mappers
}

//----------------------------------------------------------------------------
void vtkOpenGL2Light::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
