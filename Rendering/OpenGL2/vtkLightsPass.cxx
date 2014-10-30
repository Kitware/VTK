/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLightsPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkLightsPass.h"
#include "vtkObjectFactory.h"
#include <cassert>
#include "vtkRenderState.h"
#include "vtkRenderer.h"

vtkStandardNewMacro(vtkLightsPass);

// ----------------------------------------------------------------------------
vtkLightsPass::vtkLightsPass()
{
}

// ----------------------------------------------------------------------------
vtkLightsPass::~vtkLightsPass()
{
}

// ----------------------------------------------------------------------------
void vtkLightsPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

// ----------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void vtkLightsPass::Render(const vtkRenderState *s)
{
  assert("pre: s_exists" && s!=0);

  this->NumberOfRenderedProps=0;

  this->ClearLights(s->GetRenderer());
  this->UpdateLightGeometry(s->GetRenderer());
  this->UpdateLights(s->GetRenderer());
}
