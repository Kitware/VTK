/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLabelRenderStrategy.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLabelRenderStrategy.h"

#include "vtkRenderer.h"
#include "vtkTextProperty.h"

vtkCxxSetObjectMacro(vtkLabelRenderStrategy, Renderer, vtkRenderer);
vtkCxxSetObjectMacro(vtkLabelRenderStrategy, DefaultTextProperty, vtkTextProperty);

//----------------------------------------------------------------------------
vtkLabelRenderStrategy::vtkLabelRenderStrategy()
{
  this->Renderer = 0;
  this->DefaultTextProperty = vtkTextProperty::New();
}

//----------------------------------------------------------------------------
vtkLabelRenderStrategy::~vtkLabelRenderStrategy()
{
  this->SetRenderer(0);
  this->SetDefaultTextProperty(0);
}

//----------------------------------------------------------------------------
void vtkLabelRenderStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Renderer: " << this->Renderer << endl;
  os << indent << "DefaultTextProperty: " << this->DefaultTextProperty << endl;
}
