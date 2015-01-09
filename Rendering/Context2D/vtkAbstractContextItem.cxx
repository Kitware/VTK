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

// STL headers
#include <algorithm>

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
unsigned int vtkAbstractContextItem::GetItemIndex(vtkAbstractContextItem* item)
{
  vtkContextScenePrivate::const_iterator it =
    std::find(this->Children->begin(), this->Children->end(), item);
  if (it == this->Children->end())
    {
    return static_cast<unsigned int>(-1);
    }
  return it - this->Children->begin();
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
unsigned int vtkAbstractContextItem::Raise(unsigned int index)
{
  return this->StackAbove(index, this->GetNumberOfItems() - 1);
}

//-----------------------------------------------------------------------------
unsigned int vtkAbstractContextItem::StackAbove(unsigned int index,
                                                unsigned int under)
{
  unsigned int res = index;
  if (index == under)
    {
    return res;
    }
  unsigned int start = 0;
  unsigned int middle = 0;
  unsigned int end = 0;
  if (under == static_cast<unsigned int>(-1))
    {
    start = 0;
    middle = index;
    end = index + 1;
    res = 0;
    }
  else if (index > under)
    {
    start = under + 1;
    middle = index;
    end = index + 1;
    res = start;
    }
  else // if (index < under)
    {
    start = index;
    middle = index + 1;
    end = under + 1;
    res = end - 1;
    }
  std::rotate(this->Children->begin() + start,
              this->Children->begin() + middle,
              this->Children->begin() + end);
  return res;
}

//-----------------------------------------------------------------------------
unsigned int vtkAbstractContextItem::Lower(unsigned int index)
{
  return this->StackUnder(index, 0);
}

//-----------------------------------------------------------------------------
unsigned int vtkAbstractContextItem::StackUnder(unsigned int child,
                                                unsigned int above)
{
  return this->StackAbove(child, above - 1);
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
