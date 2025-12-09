// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCollection.h"

#include "vtkCollectionIterator.h"
#include "vtkGarbageCollector.h"
#include "vtkObjectFactory.h"

#include <cassert>
#include <cmath>
#include <cstdlib>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkCollection);

//------------------------------------------------------------------------------
// Construct with empty list.
vtkCollection::vtkCollection()
{
  this->NumberOfItems = 0;
  this->Top = nullptr;
  this->Bottom = nullptr;
  this->Current = nullptr;
}

//------------------------------------------------------------------------------
// Destructor for the vtkCollection class. This removes all
// objects from the collection.
vtkCollection::~vtkCollection()
{
  this->RemoveAllItems();
}

//------------------------------------------------------------------------------
// protected function to delete an element. Internal use only.
void vtkCollection::DeleteElement(vtkCollectionElement* e)
{
  if (e->Item != nullptr)
  {
    e->Item->UnRegister(this);
  }
  delete e;
}

//------------------------------------------------------------------------------
// protected function to remove an element. Internal use only.
void vtkCollection::RemoveElement(vtkCollectionElement* elem, vtkCollectionElement* prev)
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

  if (this->Current == elem)
  {
    this->Current = elem->Next;
  }

  this->NumberOfItems--;
  this->DeleteElement(elem);
}

//------------------------------------------------------------------------------
void vtkCollection::AddItem(vtkObject* a)
{
  vtkCollectionElement* elem;

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
  elem->Next = nullptr;

  this->NumberOfItems++;

  this->Modified();
}

//------------------------------------------------------------------------------
void vtkCollection::InsertItem(int i, vtkObject* a)
{
  if (i >= this->NumberOfItems || !this->Top)
  {
    return;
  }

  vtkCollectionElement* elem;

  elem = new vtkCollectionElement;
  vtkCollectionElement* curr = this->Top;

  if (i < 0)
  {
    this->Top = elem;
    elem->Next = curr;
  }
  else
  {
    vtkCollectionElement* next = curr->Next;

    int j = 0;
    while (j != i)
    {
      curr = next;
      next = curr->Next;
      j++;
    }

    curr->Next = elem;
    if (curr == this->Bottom)
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

  this->NumberOfItems++;

  this->Modified();
}

//------------------------------------------------------------------------------
void vtkCollection::RemoveItem(vtkObject* a)
{
  if (!this->Top || !a)
  {
    return;
  }

  vtkCollectionElement* prev = nullptr;
  vtkCollectionElement* elem = this->Top;
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

//------------------------------------------------------------------------------
void vtkCollection::RemoveAllItems()
{
  // Don't modify if collection is empty
  if (this->NumberOfItems == 0)
  {
    return;
  }

  while (this->NumberOfItems)
  {
    this->RemoveElement(this->Top, nullptr);
  }

  this->Modified();
}

//------------------------------------------------------------------------------
int vtkCollection::IndexOfFirstOccurence(vtkObject* a) VTK_FUTURE_CONST
{
  return this->IndexOfFirstOccurrence(a);
}

//------------------------------------------------------------------------------
int vtkCollection::IndexOfFirstOccurrence(vtkObject* a) const
{
  if (!this->Top || !a)
  {
    return -1;
  }

  vtkCollectionElement* elem = this->Top;
  for (int i = 0; i < this->NumberOfItems; i++)
  {
    if (elem->Item == a)
    {
      return i;
    }
    else
    {
      elem = elem->Next;
    }
  }

  return -1;
}

//------------------------------------------------------------------------------
int vtkCollection::IsItemPresent(vtkObject* a) VTK_FUTURE_CONST
{
  if (!this->Top || !a)
  {
    return 0;
  }

  vtkCollectionElement* elem = this->Top;
  for (int i = 0; i < this->NumberOfItems; i++)
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

//------------------------------------------------------------------------------
void vtkCollection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Number Of Items: " << this->NumberOfItems << "\n";
}

//------------------------------------------------------------------------------
vtkObject* vtkCollection::GetItemAsObject(int i) VTK_FUTURE_CONST
{
  vtkCollectionElement* elem = this->Top;

  if (i < 0)
  {
    return nullptr;
  }

  if (i == this->NumberOfItems - 1)
  {
    // optimize for the special case where we're looking for the last elem
    elem = this->Bottom;
  }
  else
  {
    while (elem != nullptr && i > 0)
    {
      elem = elem->Next;
      i--;
    }
  }
  if (elem != nullptr)
  {
    return elem->Item;
  }
  else
  {
    return nullptr;
  }
}

//------------------------------------------------------------------------------
void vtkCollection::ReplaceItem(int i, vtkObject* a)
{
  vtkCollectionElement* elem;

  if (i < 0 || i >= this->NumberOfItems)
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
    for (int j = 0; j < i; j++, elem = elem->Next)
    {
    }
  }

  // Take care of reference counting
  if (elem->Item != nullptr)
  {
    elem->Item->UnRegister(this);
  }
  a->Register(this);

  // j == i
  elem->Item = a;

  this->Modified();
}

//------------------------------------------------------------------------------
void vtkCollection::RemoveItem(int i)
{
  vtkCollectionElement *elem, *prev;

  if (i < 0 || i >= this->NumberOfItems)
  {
    return;
  }

  elem = this->Top;
  prev = nullptr;
  for (int j = 0; j < i; j++)
  {
    prev = elem;
    elem = elem->Next;
  }

  this->RemoveElement(elem, prev);
  this->Modified();
}

//------------------------------------------------------------------------------
vtkCollectionIterator* vtkCollection::NewIterator()
{
  vtkCollectionIterator* it = vtkCollectionIterator::New();
  it->SetCollection(this);
  return it;
}

//------------------------------------------------------------------------------
void vtkCollection::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  for (vtkCollectionElement* elem = this->Top; elem; elem = elem->Next)
  {
    vtkGarbageCollectorReport(collector, elem->Item, "Element");
  }
}
VTK_ABI_NAMESPACE_END
