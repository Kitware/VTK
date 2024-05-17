// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkContextScenePrivate
 * @brief   Private implementation for scene/items.
 *
 *
 * Provides a list of context items, and convenience functions to paint
 * all of the children of the scene/item. This is a private class to be used
 * in vtkContextScene, vtkAbstractContextItem and friends.
 *
 * \internal
 */

#ifndef vtkContextScenePrivate_h
#define vtkContextScenePrivate_h

#include "vtkAbstractContextItem.h"
#include "vtkContextScene.h"

// STL headers
#include <vector> // Needed for STL vector.

VTK_ABI_NAMESPACE_BEGIN
class vtkContext2D;

//-----------------------------------------------------------------------------
class vtkContextScenePrivate : public std::vector<vtkAbstractContextItem*>
{
public:
  /**
   * Default constructor.
   */
  vtkContextScenePrivate(vtkAbstractContextItem* item)
    : Scene(nullptr)
    , Item(item)
  {
  }

  /**
   * Destructor.
   */
  ~vtkContextScenePrivate() { this->Clear(); }

  void PrintSelf(ostream& os, vtkIndent indent)
  {
    os << indent << "Number of children: " << this->size() << '\n';
    os << indent << "Scene: " << this->Scene << '\n';
    for (const_iterator it = this->begin(); it != this->end(); ++it)
    {
      (*it)->PrintSelf(os, indent.GetNextIndent());
    }
  }

  ///@{
  /**
   * A few standard defines
   */
  typedef std::vector<vtkAbstractContextItem*>::const_iterator const_iterator;
  typedef std::vector<vtkAbstractContextItem*>::iterator iterator;
  typedef std::vector<vtkAbstractContextItem*>::const_reverse_iterator const_reverse_iterator;
  typedef std::vector<vtkAbstractContextItem*>::reverse_iterator reverse_iterator;
  ///@}

  /**
   * Paint all items in the list.
   */
  void PaintItems(vtkContext2D* context)
  {
    for (const_iterator it = this->begin(); it != this->end(); ++it)
    {
      if ((*it)->GetVisible())
      {
        (*it)->Paint(context);
      }
    }
  }

  ///@{
  /**
   * Add an item to the list - ensure it is not already in the list.
   */
  unsigned int AddItem(vtkAbstractContextItem* item)
  {
    item->Register(this->Scene);
    item->SetScene(this->Scene);
    item->SetParent(this->Item);
    ///@}

    this->push_back(item);
    return static_cast<unsigned int>(this->size() - 1);
  }

  ///@{
  /**
   * Remove an item from the list.
   */
  bool RemoveItem(vtkAbstractContextItem* item)
  {
    for (iterator it = this->begin(); it != this->end(); ++it)
    {
      if (item == *it)
      {
        item->SetParent(nullptr);
        item->SetScene(nullptr);
        (*it)->Delete();
        this->erase(it);
        return true;
      }
    }
    return false;
  }
  ///@}

  ///@{
  /**
   * Remove an item from the list.
   */
  bool RemoveItem(unsigned int index)
  {
    if (index < this->size())
    {
      return this->RemoveItem(this->at(index));
    }
    return false;
  }
  ///@}

  ///@{
  /**
   * Clear all items from the list - unregister.
   */
  void Clear()
  {
    for (const_iterator it = this->begin(); it != this->end(); ++it)
    {
      (*it)->SetParent(nullptr);
      (*it)->SetScene(nullptr);
      // releases cache from 2D, 3D devices
      (*it)->ReleaseGraphicsResources();
      (*it)->Delete();
    }
    this->clear();
  }
  ///@}

  ///@{
  /**
   * Set the scene for the instance (and its items).
   */
  void SetScene(vtkContextScene* scene)
  {
    if (this->Scene == scene)
    {
      return;
    }
    this->Scene = scene;
    for (const_iterator it = this->begin(); it != this->end(); ++it)
    {
      (*it)->SetScene(scene);
    }
  }
  ///@}

  /**
   * Store a reference to the scene.
   */
  vtkContextScene* Scene;

  ///@{
  /**
   * Store a reference to the item that these children are part of.
   * May be NULL for items in the scene itself.
   */
  vtkAbstractContextItem* Item;
  ///@}
};

VTK_ABI_NAMESPACE_END
#endif // vtkContextScenePrivate_h
// VTK-HeaderTest-Exclude: vtkContextScenePrivate.h
