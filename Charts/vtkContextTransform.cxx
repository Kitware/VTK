/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContextTransform.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkContextTransform.h"
#include "vtkObjectFactory.h"
#include "vtkContextScenePrivate.h"
#include "vtkContext2D.h"
#include "vtkTransform2D.h"
#include "vtkVector.h"

vtkStandardNewMacro(vtkContextTransform);

//-----------------------------------------------------------------------------
vtkContextTransform::vtkContextTransform()
{
  this->Transform = vtkSmartPointer<vtkTransform2D>::New();
}

//-----------------------------------------------------------------------------
vtkContextTransform::~vtkContextTransform()
{
}

//-----------------------------------------------------------------------------
bool vtkContextTransform::Paint(vtkContext2D *painter)
{
  painter->PushMatrix();
  painter->AppendTransform(this->Transform);
  bool result = this->PaintChildren(painter);
  painter->PopMatrix();
  return result;
}

//-----------------------------------------------------------------------------
void vtkContextTransform::Update()
{
}

//-----------------------------------------------------------------------------
void vtkContextTransform::Translate(float dx, float dy)
{
  float d[] = { dx, dy };
  this->Transform->Translate(d);
}

//-----------------------------------------------------------------------------
void vtkContextTransform::Scale(float dx, float dy)
{
  float d[] = { dx, dy };
  this->Transform->Scale(d);
}

//-----------------------------------------------------------------------------
void vtkContextTransform::Rotate(float angle)
{
  this->Transform->Rotate(double(angle));
}

//-----------------------------------------------------------------------------
vtkTransform2D* vtkContextTransform::GetTransform()
{
  return this->Transform;
}

//-----------------------------------------------------------------------------
bool vtkContextTransform::Hit(const vtkContextMouseEvent &mouse)
{
  vtkContextMouseEvent event = mouse;
  this->TransformMouse(mouse, event);
  return this->Superclass::Hit(event);
}

//-----------------------------------------------------------------------------
bool vtkContextTransform::MouseEnterEvent(const vtkContextMouseEvent &mouse)
{
  vtkContextMouseEvent event = mouse;
  this->TransformMouse(mouse, event);
  return this->Superclass::MouseEnterEvent(event);
}

//-----------------------------------------------------------------------------
bool vtkContextTransform::MouseMoveEvent(const vtkContextMouseEvent &mouse)
{
  vtkContextMouseEvent event = mouse;
  this->TransformMouse(mouse, event);
  return this->Superclass::MouseMoveEvent(event);
}

//-----------------------------------------------------------------------------
bool vtkContextTransform::MouseLeaveEvent(const vtkContextMouseEvent &mouse)
{
  vtkContextMouseEvent event = mouse;
  this->TransformMouse(mouse, event);
  return this->Superclass::MouseLeaveEvent(event);
}

//-----------------------------------------------------------------------------
bool vtkContextTransform::MouseButtonPressEvent(const vtkContextMouseEvent &mouse)
{
  vtkContextMouseEvent event = mouse;
  this->TransformMouse(mouse, event);
  return this->Superclass::MouseButtonPressEvent(event);
}

//-----------------------------------------------------------------------------
bool vtkContextTransform::MouseButtonReleaseEvent(const vtkContextMouseEvent &mouse)
{
  vtkContextMouseEvent event = mouse;
  this->TransformMouse(mouse, event);
  return this->Superclass::MouseButtonReleaseEvent(event);
}

//-----------------------------------------------------------------------------
bool vtkContextTransform::MouseWheelEvent(const vtkContextMouseEvent &mouse,
                                             int delta)
{
  vtkContextMouseEvent event = mouse;
  this->TransformMouse(mouse, event);
  return this->Superclass::MouseWheelEvent(event, delta);
}

//-----------------------------------------------------------------------------
inline void vtkContextTransform::TransformMouse(const vtkContextMouseEvent &mouse,
                                                vtkContextMouseEvent &event)
{
  this->Transform->InverseTransformPoints(mouse.Pos.GetData(),
                                          event.Pos.GetData(), 1);
  this->Transform->InverseTransformPoints(mouse.LastPos.GetData(),
                                          event.LastPos.GetData(), 1);
}

//-----------------------------------------------------------------------------
void vtkContextTransform::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
