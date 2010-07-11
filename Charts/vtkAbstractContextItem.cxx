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
#include "vtkContextScenePrivate.h"

//-----------------------------------------------------------------------------
vtkAbstractContextItem::vtkAbstractContextItem()
{
  this->Scene = NULL;
  this->Children = new vtkContextScenePrivate;
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
bool vtkAbstractContextItem::Hit(const vtkContextMouseEvent &mouse)
{
  for(vtkContextScenePrivate::const_iterator it = this->Children->begin();
    it != this->Children->end(); ++it)
    {
    if ((*it)->Hit(mouse))
      {
      return true;
      }
    }
  return false;
}

//-----------------------------------------------------------------------------
bool vtkAbstractContextItem::MouseEnterEvent(const vtkContextMouseEvent &mouse)
{
  for(vtkContextScenePrivate::const_iterator it = this->Children->begin();
    it != this->Children->end(); ++it)
    {
    if ((*it)->MouseEnterEvent(mouse))
      {
      return true;
      }
    }
  return false;
}

//-----------------------------------------------------------------------------
bool vtkAbstractContextItem::MouseMoveEvent(const vtkContextMouseEvent &mouse)
{
  for(vtkContextScenePrivate::const_iterator it = this->Children->begin();
    it != this->Children->end(); ++it)
    {
    if ((*it)->MouseMoveEvent(mouse))
      {
      return true;
      }
    }
  return false;
}

//-----------------------------------------------------------------------------
bool vtkAbstractContextItem::MouseLeaveEvent(const vtkContextMouseEvent &mouse)
{
  for(vtkContextScenePrivate::const_iterator it = this->Children->begin();
    it != this->Children->end(); ++it)
    {
    if ((*it)->MouseLeaveEvent(mouse))
      {
      return true;
      }
    }
  return false;
}

//-----------------------------------------------------------------------------
bool vtkAbstractContextItem::MouseButtonPressEvent(const vtkContextMouseEvent &mouse)
{
  for(vtkContextScenePrivate::const_iterator it = this->Children->begin();
    it != this->Children->end(); ++it)
    {
    if ((*it)->MouseButtonPressEvent(mouse))
      {
      return true;
      }
    }
  return false;
}

//-----------------------------------------------------------------------------
bool vtkAbstractContextItem::MouseButtonReleaseEvent(const vtkContextMouseEvent &mouse)
{
  for(vtkContextScenePrivate::const_iterator it = this->Children->begin();
    it != this->Children->end(); ++it)
    {
    if ((*it)->MouseButtonReleaseEvent(mouse))
      {
      return true;
      }
    }
  return false;
}

//-----------------------------------------------------------------------------
bool vtkAbstractContextItem::MouseWheelEvent(const vtkContextMouseEvent &mouse,
                                             int delta)
{
  for(vtkContextScenePrivate::const_iterator it = this->Children->begin();
    it != this->Children->end(); ++it)
    {
    if ((*it)->MouseWheelEvent(mouse, delta))
      {
      return true;
      }
    }
  return false;
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

//-----------------------------------------------------------------------------
void vtkAbstractContextItem::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
