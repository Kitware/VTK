/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContextItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkAbstractContextItem - base class for items that are part of a
// vtkContextScene.
//
// .SECTION Description
// This class is the common base for all context scene items. You should
// generally derive from vtkContextItem, rather than this class, as it provides
// most of the commonly used API.

#ifndef __vtkAbstractContextItem_h
#define __vtkAbstractContextItem_h

#include "vtkObject.h"

class vtkContext2D;
class vtkContextMouseEvent;
class vtkContextKeyEvent;
class vtkContextScene;
class vtkContextScenePrivate;
class vtkVector2f;

class VTK_CHARTS_EXPORT vtkAbstractContextItem : public vtkObject
{
public:
  vtkTypeMacro(vtkAbstractContextItem, vtkObject);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Perform any updates to the item that may be necessary before rendering.
  // The scene should take care of calling this on all items before their
  // Paint function is invoked.
  virtual void Update();

  // Description:
  // Paint event for the item, called whenever the item needs to be drawn.
  virtual bool Paint(vtkContext2D *painter);

  // Description:
  // Paint the children of the item, should be called whenever the children
  // need to be rendered.
  bool PaintChildren(vtkContext2D *painter);

  // Description:
  // Release graphics resources hold by the item. The default implementation
  // is empty.
  virtual void ReleaseGraphicsResources();

  // Description:
  // Add child items to this item. Increments reference count of item.
  // \return the index of the child item.
  unsigned int AddItem(vtkAbstractContextItem* item);

  // Description:
  // Remove child item from this item. Decrements reference count of item.
  // \param item the item to be removed.
  // \return true on success, false otherwise.
  bool RemoveItem(vtkAbstractContextItem* item);

  // Description:
  // Remove child item from this item. Decrements reference count of item.
  // \param index of the item to be removed.
  // \return true on success, false otherwise.
  bool RemoveItem(unsigned int index);

  // Description:
  // Get the item at the specified index.
  // \return the item at the specified index (null if index is invalid).
  vtkAbstractContextItem* GetItem(unsigned int index);

  // Description:
  // Get the number of child items.
  unsigned int GetNumberOfItems();

  // Description:
  // Remove all child items from this item.
  void ClearItems();

//BTX
  // Description:
  // Return true if the supplied x, y coordinate is inside the item.
  virtual bool Hit(const vtkContextMouseEvent &mouse);

  // Description:
  // Return the item under the mouse.
  // If no item is under the mouse, the method returns a null pointer.
  virtual vtkAbstractContextItem* GetPickedItem(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse enter event.
  // Return true if the item holds the event, false if the event can be
  // propagated to other items.
  virtual bool MouseEnterEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse move event.
  // Return true if the item holds the event, false if the event can be
  // propagated to other items.
  virtual bool MouseMoveEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse leave event.
  // Return true if the item holds the event, false if the event can be
  // propagated to other items.
  virtual bool MouseLeaveEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse button down event
  // Return true if the item holds the event, false if the event can be
  // propagated to other items.
  virtual bool MouseButtonPressEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse button release event.
  // Return true if the item holds the event, false if the event can be
  // propagated to other items.
  virtual bool MouseButtonReleaseEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse button double click event.
  // Return true if the item holds the event, false if the event can be
  // propagated to other items.
  virtual bool MouseDoubleClickEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse wheel event, positive delta indicates forward movement of the wheel.
  // Return true if the item holds the event, false if the event can be
  // propagated to other items.
  virtual bool MouseWheelEvent(const vtkContextMouseEvent &mouse, int delta);

  // Description:
  // Key press event.
  virtual bool KeyPressEvent(const vtkContextKeyEvent &key);

  // Description:
  // Key release event.
  virtual bool KeyReleaseEvent(const vtkContextKeyEvent &key);
//ETX

  // Description:
  // Set the vtkContextScene for the item, always set for an item in a scene.
  virtual void SetScene(vtkContextScene *scene);

  // Description:
  // Get the vtkContextScene for the item, always set for an item in a scene.
  vtkContextScene* GetScene()
    {
    return this->Scene;
    }

  // Description:
  // Set the parent item. The parent will be set for all items except top
  // level items in a scene.
  virtual void SetParent(vtkAbstractContextItem *parent);

  // Description:
  // Get the parent item. The parent will be set for all items except top
  // level items in a tree.
  vtkAbstractContextItem* GetParent()
    {
    return this->Parent;
    }

  // Description:
  // Maps the point to the parent coordinate system.
  virtual vtkVector2f MapToParent(const vtkVector2f& point);

  // Description:
  // Maps the point from the parent coordinate system.
  virtual vtkVector2f MapFromParent(const vtkVector2f& point);

  // Description:
  // Maps the point to the scene coordinate system.
  virtual vtkVector2f MapToScene(const vtkVector2f& point);

  // Description:
  // Maps the point from the scene coordinate system.
  virtual vtkVector2f MapFromScene(const vtkVector2f& point);

  // Description:
  // Get the visibility of the item (should it be drawn).
  vtkGetMacro(Visible, bool);

  // Description:
  // Set the visibility of the item (should it be drawn).
  // Visible by default.
  vtkSetMacro(Visible, bool);

//BTX
protected:
  vtkAbstractContextItem();
  ~vtkAbstractContextItem();

  // Description:
  // Point to the scene the item is on - can be null.
  vtkContextScene* Scene;

  // Description:
  // Point to the parent item - can be null.
  vtkAbstractContextItem* Parent;

  // Description:
  // This structure provides a list of children, along with convenience
  // functions to paint the children etc. It is derived from
  // std::vector<vtkAbstractContextItem>, defined in a private header.
  vtkContextScenePrivate* Children;

  // Description: Store the visibility of the item (default is true).
  bool Visible;

private:
  vtkAbstractContextItem(const vtkAbstractContextItem &); // Not implemented.
  void operator=(const vtkAbstractContextItem &);   // Not implemented.
//ETX
};

#endif //__vtkContextItem_h
