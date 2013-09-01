/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpaquePass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpaquePass.h"
#include "vtkObjectFactory.h"
#include <cassert>

vtkStandardNewMacro(vtkOpaquePass);

// ----------------------------------------------------------------------------
vtkOpaquePass::vtkOpaquePass()
{
}

// ----------------------------------------------------------------------------
vtkOpaquePass::~vtkOpaquePass()
{
}

// ----------------------------------------------------------------------------
void vtkOpaquePass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

// ----------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void vtkOpaquePass::Render(const vtkRenderState *s)
{
  assert("pre: s_exists" && s!=0);

  this->NumberOfRenderedProps=0;
  this->RenderFilteredOpaqueGeometry(s);
}
