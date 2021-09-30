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

// Hide VTK_DEPRECATED_IN_9_1_0() warnings for this class.
#define VTK_DEPRECATION_LEVEL 0

#include "vtkLabelRenderStrategy.h"

#include "vtkRenderer.h"
#include "vtkTextProperty.h"

vtkCxxSetObjectMacro(vtkLabelRenderStrategy, Renderer, vtkRenderer);
vtkCxxSetObjectMacro(vtkLabelRenderStrategy, DefaultTextProperty, vtkTextProperty);

//------------------------------------------------------------------------------
vtkLabelRenderStrategy::vtkLabelRenderStrategy()
{
  this->Renderer = nullptr;
  this->DefaultTextProperty = vtkTextProperty::New();
}

//------------------------------------------------------------------------------
vtkLabelRenderStrategy::~vtkLabelRenderStrategy()
{
  this->SetRenderer(nullptr);
  this->SetDefaultTextProperty(nullptr);
}

//------------------------------------------------------------------------------
void vtkLabelRenderStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Renderer: " << this->Renderer << endl;
  os << indent << "DefaultTextProperty: " << this->DefaultTextProperty << endl;
}

//------------------------------------------------------------------------------
void vtkLabelRenderStrategy::ComputeLabelBounds(
  vtkTextProperty* tprop, vtkStdString label, double bds[4])
{
  this->ComputeLabelBounds(tprop, vtkUnicodeString::from_utf8(label.c_str()), bds);
}

//------------------------------------------------------------------------------
void vtkLabelRenderStrategy::RenderLabel(int x[2], vtkTextProperty* tprop, vtkStdString label)
{
  this->RenderLabel(x, tprop, vtkUnicodeString::from_utf8(label));
}

//------------------------------------------------------------------------------
void vtkLabelRenderStrategy::RenderLabel(
  int x[2], vtkTextProperty* tprop, vtkStdString label, int maxWidth)
{
  this->RenderLabel(x, tprop, vtkUnicodeString::from_utf8(label), maxWidth);
}

//------------------------------------------------------------------------------
void vtkLabelRenderStrategy::RenderLabel(
  int x[2], vtkTextProperty* tprop, vtkUnicodeString label, int vtkNotUsed(maxWidth))
{
  this->RenderLabel(x, tprop, label);
}
