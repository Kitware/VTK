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
#include "vtkContextScene.h"
#include "vtkObjectFactory.h"

//-----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkContextItem, Transform, vtkTransform2D);

//-----------------------------------------------------------------------------
vtkContextItem::vtkContextItem()
{
  this->Transform = NULL;//vtkTransform2D::New();
  this->Scene = NULL;
  this->Visible = true;
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
  this->SetScene(NULL);
}

//-----------------------------------------------------------------------------
void vtkContextItem::Update()
{
}

//-----------------------------------------------------------------------------
void vtkContextItem::SetScene(vtkContextScene *scene)
{
  // Cannot have a reference counted pointer to the scene as this causes a
  // reference loop, where the scene and the item never get to a reference
  // count of zero.
  this->Scene = scene;
}

vtkContextScene* vtkContextItem::GetScene()
{
  // Return the underlying pointer
  return this->Scene.GetPointer();
}

//-----------------------------------------------------------------------------
bool vtkContextItem::Hit(const vtkContextMouseEvent &)
{
  return false;
}

//-----------------------------------------------------------------------------
bool vtkContextItem::MouseEnterEvent(const vtkContextMouseEvent &)
{
  return false;
}

//-----------------------------------------------------------------------------
bool vtkContextItem::MouseMoveEvent(const vtkContextMouseEvent &)
{
  return false;
}

//-----------------------------------------------------------------------------
bool vtkContextItem::MouseLeaveEvent(const vtkContextMouseEvent &)
{
  return false;
}

//-----------------------------------------------------------------------------
bool vtkContextItem::MouseButtonPressEvent(const vtkContextMouseEvent &)
{
  return false;
}

//-----------------------------------------------------------------------------
bool vtkContextItem::MouseButtonReleaseEvent(const vtkContextMouseEvent &)
{
  return false;
}

//-----------------------------------------------------------------------------
bool vtkContextItem::MouseWheelEvent(const vtkContextMouseEvent &, int)
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

// ----------------------------------------------------------------------------
void vtkContextItem::ReleaseGraphicsResources()
{
}

//-----------------------------------------------------------------------------
void vtkContextItem::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
