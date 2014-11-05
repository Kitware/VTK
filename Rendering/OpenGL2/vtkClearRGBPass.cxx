/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkClearRGBPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkClearRGBPass.h"
#include "vtkObjectFactory.h"
#include "vtkRenderState.h"
#include "vtkRenderer.h"
#include "vtk_glew.h"

vtkStandardNewMacro(vtkClearRGBPass);

// ----------------------------------------------------------------------------
vtkClearRGBPass::vtkClearRGBPass()
{
  this->Background[0] = 0;
  this->Background[1] = 0;
  this->Background[2] = 0;
}

// ----------------------------------------------------------------------------
vtkClearRGBPass::~vtkClearRGBPass()
{
}

// ----------------------------------------------------------------------------
void vtkClearRGBPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Background:"
     << this->Background[0] << "," << this->Background[1] << ","
     << this->Background[2] << endl;
}

// ----------------------------------------------------------------------------
void vtkClearRGBPass::Render(const vtkRenderState *s)
{
  (void)s;
  this->NumberOfRenderedProps=0;

  glClearColor( static_cast<GLclampf>(this->Background[0]),
                static_cast<GLclampf>(this->Background[1]),
                static_cast<GLclampf>(this->Background[2]),
                static_cast<GLclampf>(0.0));
  glClear(GL_COLOR_BUFFER_BIT);
}
