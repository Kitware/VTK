/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContextDevice2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkContextDevice2D.h"
#include "vtkPen.h"
#include "vtkBrush.h"
#include "vtkTextProperty.h"
#include "vtkRect.h"

#include "vtkObjectFactory.h"
#include <assert.h>

//-----------------------------------------------------------------------------
//vtkStandardNewMacro(vtkContextDevice2D);

vtkContextDevice2D::vtkContextDevice2D()
{
  this->Geometry[0] = 0;
  this->Geometry[1] = 0;
  this->BufferId = 0;
  this->Pen = vtkPen::New();
  this->Brush = vtkBrush::New();
  this->TextProp = vtkTextProperty::New();
}

//-----------------------------------------------------------------------------
vtkContextDevice2D::~vtkContextDevice2D()
{
  this->Pen->Delete();
  this->Brush->Delete();
  this->TextProp->Delete();
}

//-----------------------------------------------------------------------------
void vtkContextDevice2D::ApplyPen(vtkPen *pen)
{
  this->Pen->DeepCopy(pen);
}

//-----------------------------------------------------------------------------
void vtkContextDevice2D::ApplyBrush(vtkBrush *brush)
{
  this->Brush->DeepCopy(brush);
}

//-----------------------------------------------------------------------------
void vtkContextDevice2D::ApplyTextProp(vtkTextProperty *prop)
{
  // This is a deep copy, but is called shallow for some reason...
  this->TextProp->ShallowCopy(prop);
}

// ----------------------------------------------------------------------------
bool vtkContextDevice2D::GetBufferIdMode() const
{
  return this->BufferId != 0;
}

// ----------------------------------------------------------------------------
void vtkContextDevice2D::BufferIdModeBegin(
  vtkAbstractContextBufferId *bufferId)
{
  assert("pre: not_yet" && !this->GetBufferIdMode());
  assert("pre: bufferId_exists" && bufferId!=0);

  this->BufferId = bufferId;

  assert("post: started" && this->GetBufferIdMode());
}

// ----------------------------------------------------------------------------
void vtkContextDevice2D::BufferIdModeEnd()
{
  assert("pre: started" && this->GetBufferIdMode());

  this->BufferId = 0;

  assert("post: done" && !this->GetBufferIdMode());
}

//-----------------------------------------------------------------------------
void vtkContextDevice2D::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Pen: ";
  this->Pen->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Brush: ";
  this->Brush->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Text Property: ";
  this->TextProp->PrintSelf(os, indent.GetNextIndent());

}
