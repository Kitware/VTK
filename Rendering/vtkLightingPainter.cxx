/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLightingPainter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkLightingPainter.h"

#include "vtkGraphicsFactory.h"
#include "vtkObjectFactory.h"

// Needed when we don't use the vtkStandardNewMacro.
vtkInstantiatorNewMacro(vtkLightingPainter);
//-----------------------------------------------------------------------------
vtkLightingPainter::vtkLightingPainter()
{
}

//-----------------------------------------------------------------------------
vtkLightingPainter::~vtkLightingPainter()
{
}

//-----------------------------------------------------------------------------
vtkLightingPainter* vtkLightingPainter::New()
{
  vtkObject* o = vtkGraphicsFactory::CreateInstance("vtkLightingPainter");
  return static_cast<vtkLightingPainter *>(o);
}

//-----------------------------------------------------------------------------
void vtkLightingPainter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

