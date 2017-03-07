/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCollection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCollection.h"

#include "vtkCollectionIterator.h"
#include "vtkGarbageCollector.h"
#include "vtkObjectFactory.h"

#include <cassert>
#include <cstdlib>
#include <cmath>

vtkStandardNewMacro(vtkCollection);

// Construct with empty list.
vtkCollection::vtkCollection()
{
  this->NumberOfItems = 0;
  this->Top = NULL;
  this->Bottom = NULL;
  this->Current = NULL;
}

// Destructor for the vtkCollection class. This removes all
// objects from the collection.
vtkCollection::~vtkCollection()
{
  this->RemoveAllItems();
}

// protected function to delete an element. Internal use only.
void vtkCollection::DeleteElement(vtkCollectionElement *e)
{
  if (e->Item != NULL)
  {
    e->Item->UnRegister(this);
  }
  delete e;
}

// protected function to remove an element. Internal use only.
void vtkCollection::
RemoveElement(vtkCollectionElement *elem, vtkCollectionElement *prev)
{
  assert(elem);
  if (prev)
  {
    prev->Next = elem->Next;
  }
  else
  {
    this->Top = elem->Next;
  }

  if (!elem->Next)
  {
    this->Bottom = prev;
  }

  if ( this->Current == elem )
  {
    this->Current = elem->Next;
  }

  this->NumberOfItems--;
  this->DeleteElement(elem);
}

// Add an object to the bottom of the list. Does not prevent duplicate entries.
void vtkCollection::AddItem(vtkObject *a)
{
  vtkCollectionElement *elem;

  elem = new vtkCollectionElement;

  if (!this->Top)
  {
    this->Top = elem;
  }
  else
  {
    this->Bottom->Next = elem;
  }
  this->Bottom = elem;

  a->Register(this);
  elem->Item = a;
  elem->Next = NULL;

  this->Modified();

  this->NumberOfItems++;
}

// Insert an object into the list. There must be at least one
// entry pre-existing.
void vtkCollection::InsertItem(int i, vtkObject *a)
{
  if( i >= this->NumberOfItems || !this->Top )
  {
    return;
  }

  vtkCollectionElement *elem;

  elem = new vtkCollectionElement;
  vtkCollectionElement *curr = this->Top;

  if( i < 0 )
  {
    this->Top = elem;
    elem->Next = curr;
  }
  else
  {
    vtkCollectionElement *next = curr->Next;

    int j = 0;
    while( j != i )
    {
      curr = next;
      next = curr->Next;
      j++;
    }

    curr->Next = elem;
    if( curr == this->Bottom )
    {
      this->Bottom = elem;
    }
    else
    {
      elem->Next = next;
    }
  }

  a->Register(this);
  elem->Item = a;

  this->Modified();

  this->NumberOfItems++;
}

// Remove an object from the list. Removes the first object found, not
// all occurrences. If no object found, list is unaffected.  See warning
// in description of RemoveItem(int).
void vtkCollection::RemoveItem(vtkObject *a)
{
  if (!this->Top)
  {
    return;
  }

  vtkCollectionElement *prev = NULL;
  vtkCollectionElement *elem = this->Top;
  for (int i = 0; i < this->NumberOfItems; i++)
  {
    if (elem->Item == a)
    {
      this->RemoveElement(elem, prev);
      this->Modified();
      return;
    }
    else
    {
      prev = elem;
      elem = elem->Next;
    }
  }
}

// Remove all objects from the list.
void vtkCollection::RemoveAllItems()
{
  // Don't modify if collection is empty
  if(this->NumberOfItems == 0)
  {
    return;
  }

  while (this->NumberOfItems)
  {
    this->RemoveElement(this->Top, NULL);
  }

  this->Modified();
}

// Search for an object and return location in list. If location == 0,
// object was not found.
int vtkCollection::IsItemPresent(vtkObject *a)
{
  int i;
  vtkCollectionElement *elem;

  if (!this->Top)
  {
    return 0;
  }

  elem = this->Top;
  for (i = 0; i < this->NumberOfItems; i++)
  {
    if (elem->Item == a)
    {
      return i + 1;
    }
    else
    {
      elem = elem->Next;
    }
  }

  return 0;
}


void vtkCollection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Number Of Items: " << this->NumberOfItems << "\n";
}


// Get the i'th item in the collection. NULL is returned if i is out
// of range
vtkObject *vtkCollection::GetItemAsObject(int i)
{
  vtkCollectionElement *elem=this->Top;

  if (i < 0)
  {
    return NULL;
  }

  if (i == this->NumberOfItems - 1)
  {
    // optimize for the special case where we're looking for the last elem
    elem = this->Bottom;
  }
  else
  {
    while (elem != NULL && i > 0)
    {
      elem = elem->Next;
      i--;
    }
  }
  if ( elem != NULL )
  {
    return elem->Item;
  }
  else
  {
    return NULL;
  }
}


// Replace the i'th item in the collection with a
void vtkCollection::ReplaceItem(int i, vtkObject *a)
{
  vtkCollectionElement *elem;

  if( i < 0 || i >= this->NumberOfItems )
  {
    return;
  }

  elem = this->Top;
  if (i == this->NumberOfItems - 1)
  {
    elem = this->Bottom;
  }
  else
  {
    for (int j = 0; j < i; j++, elem = elem->Next )
      {}
  }

  // Take care of reference counting
  if (elem->Item != NULL)
  {
    elem->Item->UnRegister(this);
  }
  a->Register(this);

  // j == i
  elem->Item = a;

  this->Modified();
}


// Remove the i'th item in the list.
// Be careful if using this function during traversal of the list using
// GetNextItemAsObject (or GetNextItem in derived class).  The list WILL
// be shortened if a valid index is given!  If this->Current is equal to the
// element being removed, have it point to then next element in the list.
void vtkCollection::RemoveItem(int i)
{
  vtkCollectionElement *elem,*prev;

  if( i < 0 || i >= this->NumberOfItems )
  {
    return;
  }

  elem = this->Top;
  prev = NULL;
  for (int j = 0; j < i; j++)
  {
    prev = elem;
    elem = elem->Next;
  }

  this->RemoveElement(elem, prev);
  this->Modified();
}

vtkCollectionIterator* vtkCollection::NewIterator()
{
  vtkCollectionIterator* it = vtkCollectionIterator::New();
  it->SetCollection(this);
  return it;
}

//----------------------------------------------------------------------------
void vtkCollection::Register(vtkObjectBase* o)
{
  this->RegisterInternal(o, 1);
}

//----------------------------------------------------------------------------
void vtkCollection::UnRegister(vtkObjectBase* o)
{
  this->UnRegisterInternal(o, 1);
}

//----------------------------------------------------------------------------
void vtkCollection::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  for(vtkCollectionElement* elem = this->Top; elem; elem = elem->Next)
  {
    vtkGarbageCollectorReport(collector, elem->Item, "Element");
  }
}
