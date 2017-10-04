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

/**
 * @class   vtkAbstractContextItem
 * @brief   base class for items that are part of a
 * vtkContextScene.
 *
 *
 * This class is the common base for all context scene items. You should
 * generally derive from vtkContextItem, rather than this class, as it provides
 * most of the commonly used API.
*/

#ifndef vtkAbstractContextItem_h
#define vtkAbstractContextItem_h

#include "vtkRenderingContext2DModule.h" // For export macro
#include "vtkObject.h"

class vtkContext2D;
class vtkContextMouseEvent;
class vtkContextKeyEvent;
class vtkContextScene;
class vtkContextScenePrivate;
class vtkVector2f;

class VTKRENDERINGCONTEXT2D_EXPORT vtkAbstractContextItem : public vtkObject
{
public:
  vtkTypeMacro(vtkAbstractContextItem, vtkObject);
  void PrintSelf(ostream &os, vtkIndent indent) override;

  /**
   * Perform any updates to the item that may be necessary before rendering.
   * The scene should take care of calling this on all items before their
   * Paint function is invoked.
   */
  virtual void Update();

  /**
   * Paint event for the item, called whenever the item needs to be drawn.
   */
  virtual bool Paint(vtkContext2D *painter);

  /**
   * Paint the children of the item, should be called whenever the children
   * need to be rendered.
   */
  virtual bool PaintChildren(vtkContext2D *painter);

  /**
   * Release graphics resources hold by the item. The default implementation
   * is empty.
   */
  virtual void ReleaseGraphicsResources();

  /**
   * Add child items to this item. Increments reference count of item.
   * \return the index of the child item.
   */
  vtkIdType AddItem(vtkAbstractContextItem* item);

  /**
   * Remove child item from this item. Decrements reference count of item.
   * \param item the item to be removed.
   * \return true on success, false otherwise.
   */
  bool RemoveItem(vtkAbstractContextItem* item);

  /**
   * Remove child item from this item. Decrements reference count of item.
   * \param index of the item to be removed.
   * \return true on success, false otherwise.
   */
  bool RemoveItem(vtkIdType index);

  /**
   * Get the item at the specified index.
   * \return the item at the specified index (null if index is invalid).
   */
  vtkAbstractContextItem* GetItem(vtkIdType index);

  /**
   * Get the index of the specified item in itemIndex.
   * \return the item index if found or -1 if not.
   */
  vtkIdType GetItemIndex(vtkAbstractContextItem* item);

  /**
   * Get the number of child items.
   */
  vtkIdType GetNumberOfItems();

  /**
   * Remove all child items from this item.
   */
  void ClearItems();

  /**
   * Raises the \a child to the top of the item's stack.
   * \return The new index of the item
   * \sa StackAbove(), Lower(), LowerUnder()
   */
  vtkIdType Raise(vtkIdType index);

  /**
   * Raises the \a child above the \a under sibling. If \a under is invalid,
   * the item is raised to the top of the item's stack.
   * \return The new index of the item
   * \sa Raise(), Lower(), StackUnder()
   */
  virtual vtkIdType StackAbove(vtkIdType index,
                               vtkIdType under);

  /**
   * Lowers the \a child to the bottom of the item's stack.
   * \return The new index of the item
   * \sa StackUnder(), Raise(), StackAbove()
   */
  vtkIdType Lower(vtkIdType index);

  /**
   * Lowers the \a child under the \a above sibling. If \a above is invalid,
   * the item is lowered to the bottom of the item's stack
   * \return The new index of the item
   * \sa Lower(), Raise(), StackAbove()
   */
  virtual vtkIdType StackUnder(vtkIdType child,
                               vtkIdType above);

  /**
   * Return true if the supplied x, y coordinate is inside the item.
   */
  virtual bool Hit(const vtkContextMouseEvent &mouse);

  /**
   * Return the item under the mouse.
   * If no item is under the mouse, the method returns a null pointer.
   */
  virtual vtkAbstractContextItem* GetPickedItem(const vtkContextMouseEvent &mouse);

  /**
   * Mouse enter event.
   * Return true if the item holds the event, false if the event can be
   * propagated to other items.
   */
  virtual bool MouseEnterEvent(const vtkContextMouseEvent &mouse);

  /**
   * Mouse move event.
   * Return true if the item holds the event, false if the event can be
   * propagated to other items.
   */
  virtual bool MouseMoveEvent(const vtkContextMouseEvent &mouse);

  /**
   * Mouse leave event.
   * Return true if the item holds the event, false if the event can be
   * propagated to other items.
   */
  virtual bool MouseLeaveEvent(const vtkContextMouseEvent &mouse);

  /**
   * Mouse button down event
   * Return true if the item holds the event, false if the event can be
   * propagated to other items.
   */
  virtual bool MouseButtonPressEvent(const vtkContextMouseEvent &mouse);

  /**
   * Mouse button release event.
   * Return true if the item holds the event, false if the event can be
   * propagated to other items.
   */
  virtual bool MouseButtonReleaseEvent(const vtkContextMouseEvent &mouse);

  /**
   * Mouse button double click event.
   * Return true if the item holds the event, false if the event can be
   * propagated to other items.
   */
  virtual bool MouseDoubleClickEvent(const vtkContextMouseEvent &mouse);

  /**
   * Mouse wheel event, positive delta indicates forward movement of the wheel.
   * Return true if the item holds the event, false if the event can be
   * propagated to other items.
   */
  virtual bool MouseWheelEvent(const vtkContextMouseEvent &mouse, int delta);

  /**
   * Key press event.
   */
  virtual bool KeyPressEvent(const vtkContextKeyEvent &key);

  /**
   * Key release event.
   */
  virtual bool KeyReleaseEvent(const vtkContextKeyEvent &key);

  /**
   * Set the vtkContextScene for the item, always set for an item in a scene.
   */
  virtual void SetScene(vtkContextScene *scene);

  /**
   * Get the vtkContextScene for the item, always set for an item in a scene.
   */
  vtkContextScene* GetScene()
  {
    return this->Scene;
  }

  /**
   * Set the parent item. The parent will be set for all items except top
   * level items in a scene.
   */
  virtual void SetParent(vtkAbstractContextItem *parent);

  /**
   * Get the parent item. The parent will be set for all items except top
   * level items in a tree.
   */
  vtkAbstractContextItem* GetParent()
  {
    return this->Parent;
  }

  /**
   * Maps the point to the parent coordinate system.
   */
  virtual vtkVector2f MapToParent(const vtkVector2f& point);

  /**
   * Maps the point from the parent coordinate system.
   */
  virtual vtkVector2f MapFromParent(const vtkVector2f& point);

  /**
   * Maps the point to the scene coordinate system.
   */
  virtual vtkVector2f MapToScene(const vtkVector2f& point);

  /**
   * Maps the point from the scene coordinate system.
   */
  virtual vtkVector2f MapFromScene(const vtkVector2f& point);

  //@{
  /**
   * Get the visibility of the item (should it be drawn).
   */
  vtkGetMacro(Visible, bool);
  //@}

  //@{
  /**
   * Set the visibility of the item (should it be drawn).
   * Visible by default.
   */
  vtkSetMacro(Visible, bool);
  //@}

  //@{
  /**
   * Get if the item is interactive (should respond to mouse events).
   */
  vtkGetMacro(Interactive, bool);
  //@}

  //@{
  /**
   * Set if the item is interactive (should respond to mouse events).
   */
  vtkSetMacro(Interactive, bool);
  //@}

protected:
  vtkAbstractContextItem();
  ~vtkAbstractContextItem() override;

  /**
   * Point to the scene the item is on - can be null.
   */
  vtkContextScene* Scene;

  /**
   * Point to the parent item - can be null.
   */
  vtkAbstractContextItem* Parent;

  /**
   * This structure provides a list of children, along with convenience
   * functions to paint the children etc. It is derived from
   * std::vector<vtkAbstractContextItem>, defined in a private header.
   */
  vtkContextScenePrivate* Children;

  /**
   * Store the visibility of the item (default is true).
   */
  bool Visible;

  /**
   * Store whether the item should respond to interactions (default is true).
   */
  bool Interactive;

private:
  vtkAbstractContextItem(const vtkAbstractContextItem &) = delete;
  void operator=(const vtkAbstractContextItem &) = delete;

};

#endif //vtkContextItem_h
