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
vtkAbstractContextItem* vtkAbstractContextItem::GetPickedItem(const vtkContextMouseEvent &mouse)
{
  vtkContextMouseEvent childMouse = mouse;
  this->FromParent(mouse.Pos.GetData(), childMouse.Pos.GetData());
  this->FromParent(mouse.LastPos.GetData(), childMouse.LastPos.GetData());
  for (size_t i = this->Children->size()-1; i >= 0; --i)
    {
    //cerr << "checking child " << i << ": " << (*this->Children)[i]->GetClassName() << endl;
    vtkAbstractContextItem* item = (*this->Children)[i]->GetPickedItem(childMouse);
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

//-----------------------------------------------------------------------------
void vtkAbstractContextItem::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
