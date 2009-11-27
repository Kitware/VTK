/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContextItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkContextItem.h"

// Get my new commands
#include "vtkCommand.h"

#include "vtkAnnotationLink.h"
#include "vtkInteractorStyle.h"
#include "vtkInteractorStyleRubberBand2D.h"
#include "vtkTransform2D.h"
#include "vtkObjectFactory.h"

//-----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkContextItem, "1.1");
vtkCxxSetObjectMacro(vtkContextItem, Transform, vtkTransform2D)

//-----------------------------------------------------------------------------
vtkContextItem::vtkContextItem()
{
  this->Transform = NULL;//vtkTransform2D::New();
  this->Opacity = 1.0;
}

//-----------------------------------------------------------------------------
vtkContextItem::~vtkContextItem()
{
  if (this->Transform)
    {
    this->Transform->Delete();
    this->Transform = NULL;
    }
}

//-----------------------------------------------------------------------------
bool vtkContextItem::Hit(const vtkContextMouseEvent &mouse)
{
  return false;
}

//-----------------------------------------------------------------------------
bool vtkContextItem::MouseEnterEvent(const vtkContextMouseEvent &mouse)
{
  return false;
}

//-----------------------------------------------------------------------------
bool vtkContextItem::MouseMoveEvent(const vtkContextMouseEvent &mouse)
{
  return false;
}

//-----------------------------------------------------------------------------
bool vtkContextItem::MouseLeaveEvent(const vtkContextMouseEvent &mouse)
{
  return false;
}

//-----------------------------------------------------------------------------
bool vtkContextItem::MouseButtonPressEvent(const vtkContextMouseEvent &mouse)
{
  return false;
}

//-----------------------------------------------------------------------------
bool vtkContextItem::MouseButtonReleaseEvent(const vtkContextMouseEvent &mouse)
{
  return false;
}

//-----------------------------------------------------------------------------
void vtkContextItem::Translate(float dx, float dy)
{
  if (!this->Transform)
    {
    this->Transform = vtkTransform2D::New();
    }
  this->Transform->Translate(dx, dy);
}

//-----------------------------------------------------------------------------
void vtkContextItem::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
