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
#include "vtkContextMouseEvent.h"
#include "vtkContextScenePrivate.h"

//-----------------------------------------------------------------------------
vtkAbstractContextItem::vtkAbstractContextItem()
{
  this->Scene = NULL;
  this->Parent = NULL;
  this->Children = new vtkContextScenePrivate(this);
  this->Visible = true;
  this->Interactive = true;
}

//-----------------------------------------------------------------------------
vtkAbstractContextItem::~vtkAbstractContextItem()
{
  delete this->Children;
}

//-----------------------------------------------------------------------------
bool vtkAbstractContextItem::Paint(vtkContext2D *painter)
{
  this->Children->PaintItems(painter);
  return true;
}

//-----------------------------------------------------------------------------
bool vtkAbstractContextItem::PaintChildren(vtkContext2D *painter)
{
  this->Children->PaintItems(painter);
  return true;
}

//-----------------------------------------------------------------------------
void vtkAbstractContextItem::Update()
{
}

//-----------------------------------------------------------------------------
unsigned int vtkAbstractContextItem::AddItem(vtkAbstractContextItem* item)
{
  return this->Children->AddItem(item);
}

//-----------------------------------------------------------------------------
bool vtkAbstractContextItem::RemoveItem(vtkAbstractContextItem* item)
{
  return this->Children->RemoveItem(item);
}

//-----------------------------------------------------------------------------
bool vtkAbstractContextItem::RemoveItem(unsigned int index)
{
  return this->Children->RemoveItem(index);
}

//-----------------------------------------------------------------------------
vtkAbstractContextItem* vtkAbstractContextItem::GetItem(unsigned int index)
{
  if (index < this->Children->size())
    {
    return this->Children->at(index);
    }
  else
    {
    return 0;
    }
}

//-----------------------------------------------------------------------------
unsigned int vtkAbstractContextItem::GetNumberOfItems()
{
  return static_cast<unsigned int>(this->Children->size());
}

//-----------------------------------------------------------------------------
void vtkAbstractContextItem::ClearItems()
{
  this->Children->Clear();
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
bool vtkAbstractContextItem::MouseButtonReleaseEvent(const vtkContextMouseEvent&)
{
  return false;
}

//-----------------------------------------------------------------------------
bool vtkAbstractContextItem::MouseDoubleClickEvent(const vtkContextMouseEvent&)
{
  return false;
}

//-----------------------------------------------------------------------------
bool vtkAbstractContextItem::MouseWheelEvent(const vtkContextMouseEvent &, int)
{
  return false;
}

//-----------------------------------------------------------------------------
bool vtkAbstractContextItem::KeyPressEvent(const vtkContextKeyEvent&)
{
  return false;
}

//-----------------------------------------------------------------------------
bool vtkAbstractContextItem::KeyReleaseEvent(const vtkContextKeyEvent&)
{
  return false;
}

// ----------------------------------------------------------------------------
vtkAbstractContextItem* vtkAbstractContextItem::GetPickedItem(
  const vtkContextMouseEvent &mouse)
{
  vtkContextMouseEvent childMouse = mouse;
  childMouse.SetPos(this->MapFromParent(mouse.GetPos()));
  childMouse.SetLastPos(this->MapFromParent(mouse.GetLastPos()));
  for(vtkContextScenePrivate::const_reverse_iterator it =
      this->Children->rbegin(); it != this->Children->rend(); ++it)
    {
    vtkAbstractContextItem* item = (*it)->GetPickedItem(childMouse);
    if (item)
      {
      return item;
      }
    }
  return this->Hit(mouse) ? this : NULL;
}

// ----------------------------------------------------------------------------
void vtkAbstractContextItem::ReleaseGraphicsResources()
{
  for(vtkContextScenePrivate::const_iterator it = this->Children->begin();
    it != this->Children->end(); ++it)
    {
    (*it)->ReleaseGraphicsResources();
    }
}

// ----------------------------------------------------------------------------
void vtkAbstractContextItem::SetScene(vtkContextScene* scene)
{
  this->Scene = scene;
  this->Children->SetScene(scene);
}

// ----------------------------------------------------------------------------
void vtkAbstractContextItem::SetParent(vtkAbstractContextItem* parent)
{
  this->Parent = parent;
}

// ----------------------------------------------------------------------------
vtkVector2f vtkAbstractContextItem::MapToParent(const vtkVector2f& point)
{
  return point;
}

// ----------------------------------------------------------------------------
vtkVector2f vtkAbstractContextItem::MapFromParent(const vtkVector2f& point)
{
  return point;
}

// ----------------------------------------------------------------------------
vtkVector2f vtkAbstractContextItem::MapToScene(const vtkVector2f& point)
{
  if (this->Parent)
    {
    vtkVector2f p = this->MapToParent(point);
    p = this->Parent->MapToScene(p);
    return p;
    }
  else
    {
    return this->MapToParent(point);
    }
}

// ----------------------------------------------------------------------------
vtkVector2f vtkAbstractContextItem::MapFromScene(const vtkVector2f& point)
{
  if (this->Parent)
    {
    vtkVector2f p = this->Parent->MapFromScene(point);
    p = this->MapFromParent(p);
    return p;
    }
  else
    {
    return this->MapFromParent(point);
    }
}

//-----------------------------------------------------------------------------
void vtkAbstractContextItem::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
