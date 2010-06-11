/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractContextItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAbstractContextItem.h"
#include "vtkObjectFactory.h"

//-----------------------------------------------------------------------------
vtkAbstractContextItem::vtkAbstractContextItem()
{
}

//-----------------------------------------------------------------------------
vtkAbstractContextItem::~vtkAbstractContextItem()
{
}

//-----------------------------------------------------------------------------
void vtkAbstractContextItem::Update()
{
}

//-----------------------------------------------------------------------------
bool vtkAbstractContextItem::Hit(const vtkContextMouseEvent &)
{
  return false;
}

//-----------------------------------------------------------------------------
bool vtkAbstractContextItem::MouseEnterEvent(const vtkContextMouseEvent &)
{
  return false;
}

//-----------------------------------------------------------------------------
bool vtkAbstractContextItem::MouseMoveEvent(const vtkContextMouseEvent &)
{
  return false;
}

//-----------------------------------------------------------------------------
bool vtkAbstractContextItem::MouseLeaveEvent(const vtkContextMouseEvent &)
{
  return false;
}

//-----------------------------------------------------------------------------
bool vtkAbstractContextItem::MouseButtonPressEvent(const vtkContextMouseEvent &)
{
  return false;
}

//-----------------------------------------------------------------------------
bool vtkAbstractContextItem::MouseButtonReleaseEvent(const vtkContextMouseEvent &)
{
  return false;
}

//-----------------------------------------------------------------------------
bool vtkAbstractContextItem::MouseWheelEvent(const vtkContextMouseEvent &, int)
{
  return false;
}

// ----------------------------------------------------------------------------
void vtkAbstractContextItem::ReleaseGraphicsResources()
{
}

//-----------------------------------------------------------------------------
void vtkAbstractContextItem::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
