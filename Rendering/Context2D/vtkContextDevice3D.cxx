/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContextDevice3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkContextDevice3D.h"
#include "vtkMathTextUtilities.h"
#include "vtkTextProperty.h"

vtkContextDevice3D::vtkContextDevice3D()
{
  this->TextProp = vtkTextProperty::New();
}

vtkContextDevice3D::~vtkContextDevice3D()
{
  this->TextProp->Delete();
}

//-----------------------------------------------------------------------------
void vtkContextDevice3D::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
bool vtkContextDevice3D::MathTextIsSupported()
{
  return vtkMathTextUtilities::GetInstance() != NULL;
}

//-----------------------------------------------------------------------------
void vtkContextDevice3D::ApplyTextProp(vtkTextProperty *prop)
{
  // This is a deep copy, but is called shallow for some reason...
  this->TextProp->ShallowCopy(prop);
}
