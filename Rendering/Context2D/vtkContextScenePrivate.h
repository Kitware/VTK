/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContextScenePrivate.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkContextScenePrivate - Private implementation for scene/items.
//
// .SECTION Description
// Provides a list of context items, and convenience functions to paint
// all of the children of the scene/item. This is a private class to be used
// in vtkContextScene, vtkAbstractContextItem and friends.
//
// \internal

#ifndef vtkContextScenePrivate_h
#define vtkContextScenePrivate_h

#include "vtkAbstractContextItem.h"
#include "vtkContextScene.h"

// STL headers
#include <vector> // Needed for STL vector.

class vtkContext2D;

//-----------------------------------------------------------------------------
class vtkContextScenePrivate : public std::vector<vtkAbstractContextItem*>
{
public:
  // Description:
  // Default constructor.
  vtkContextScenePrivate(vtkAbstractContextItem* item)
    : std::vector<vtkAbstractContextItem*>(), Scene(0), Item(item)
    {
    }

  // Description:
  // Destructor.
  ~vtkContextScenePrivate()
    {
    this->Clear();
    }

  // Description:
  // A few standard defines
  typedef std::vector<vtkAbstractContextItem*>::const_iterator
    const_iterator;
  typedef std::vector<vtkAbstractContextItem*>::iterator iterator;
  // Older versions of GCC did not implement comparison operators for the
  // const_reverse_operator, the simplest thing to do is not use the const
  // form of the operator.
#ifdef VTK_CONST_REVERSE_ITERATOR_COMPARISON
  typedef std::vector<vtkAbstractContextItem*>::const_reverse_iterator
    const_reverse_iterator;
#else
  typedef std::vector<vtkAbstractContextItem*>::reverse_iterator
    const_reverse_iterator;
#endif
  typedef std::vector<vtkAbstractContextItem*>::reverse_iterator
    reverse_iterator;

  // Description:
  // Paint all items in the list.
  void PaintItems(vtkContext2D* context)
    {
    for(const_iterator it = this->begin(); it != this->end(); ++it)
      {
      if ((*it)->GetVisible())
        {
        (*it)->Paint(context);
        }
      }
    }

  // Description:
  // Add an item to the list - ensure it is not already in the list.
  unsigned int AddItem(vtkAbstractContextItem* item)
    {
    item->Register(this->Scene);
    item->SetScene(this->Scene);
    item->SetParent(this->Item);

    this->push_back(item);
    return static_cast<unsigned int>(this->size()-1);
    }

  // Description:
  // Remove an item from the list.
  bool RemoveItem(vtkAbstractContextItem* item)
    {
    for(iterator it = this->begin(); it != this->end(); ++it)
      {
      if (item == *it)
        {
        item->SetParent(NULL);
        item->SetScene(NULL);
        (*it)->Delete();
        this->erase(it);
        return true;
        }
      }
    return false;
    }

  // Description:
  // Remove an item from the list.
  bool RemoveItem(unsigned int index)
    {
    if (index < this->size())
      {
      return this->RemoveItem(this->at(index));
      }
    return false;
    }

  // Description:
  // Clear all items from the list - unregister.
  void Clear()
    {
    for(const_iterator it = this->begin(); it != this->end(); ++it)
      {
      (*it)->SetParent(NULL);
      (*it)->SetScene(NULL);
      (*it)->Delete();
      }
    this->clear();
    }

  // Description:
  // Set the scene for the instance (and its items).
  void SetScene(vtkContextScene* scene)
    {
    if (this->Scene == scene)
      {
      return;
      }
    this->Scene = scene;
    for(const_iterator it = this->begin(); it != this->end(); ++it)
      {
        (*it)->SetScene(scene);
      }
    }

  // Description:
  // Store a reference to the scene.
  vtkContextScene* Scene;

  // Description:
  // Store a reference to the item that these children are part of.
  // May be NULL for items in the scene itself.
  vtkAbstractContextItem* Item;
};

#endif //vtkContextScenePrivate_h
// VTK-HeaderTest-Exclude: vtkContextScenePrivate.h
