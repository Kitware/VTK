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

#ifndef __vtkContextScenePrivate_h
#define __vtkContextScenePrivate_h

#include "vtkAbstractContextItem.h"
#include "vtkContextScene.h"

// STL headers
#include <vtkstd/vector> // Needed for STL vector.

class vtkContext2D;

//-----------------------------------------------------------------------------
class vtkContextScenePrivate : public vtkstd::vector<vtkAbstractContextItem*>
{
public:
  // Description:
  // Default constructor.
  vtkContextScenePrivate()
    : vtkstd::vector<vtkAbstractContextItem*>(), Scene(0)
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
  typedef vtkstd::vector<vtkAbstractContextItem*>::const_iterator const_iterator;
  typedef vtkstd::vector<vtkAbstractContextItem*>::iterator iterator;

  // Description:
  // Paint all items in the list.
  void PaintItems(vtkContext2D* context)
    {
    for(const_iterator it = this->begin(); it != this->end(); ++it)
      {
      (*it)->Paint(context);
      }
    }

  // Description:
  // Add an item to the list - ensure it is not already in the list.
  unsigned int AddItem(vtkAbstractContextItem* item)
    {
    item->Register(this->Scene);
    item->SetScene(this->Scene);

    this->push_back(item);
    this->State.push_back(false);
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
      (*it)->Delete();
      }
    this->clear();
    this->State.clear();
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
  // Store the state of the items to calculate enter/leave events.
  vtkstd::vector<bool> State;
};

#endif //__vtkContextScenePrivate_h
