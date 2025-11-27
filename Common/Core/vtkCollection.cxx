// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Hide VTK_DEPRECATED_IN_X_Y_Z() warnings for this class.
#define VTK_DEPRECATION_LEVEL 0

#include "vtkCollection.h"

#include "vtkCollectionIterator.h"
#include "vtkGarbageCollector.h"
#include "vtkObjectFactory.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdlib>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkCollection);

//------------------------------------------------------------------------------
// Construct with empty list.
vtkCollection::vtkCollection()
{
  this->Current = this->Objects.end();
}

//------------------------------------------------------------------------------
// Destructor for the vtkCollection class. This removes all
// objects from the collection.
vtkCollection::~vtkCollection()
{
  this->RemoveAllItems();
}

//------------------------------------------------------------------------------
void vtkCollection::AddItem(vtkObject* a)
{
  this->Objects.push_back(a);
  a->Register(this);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkCollection::InsertItem(int i, vtkObject* a)
{
  if (this->Objects.empty())
  {
    return;
  }

  if (i < 0)
  {
    // If negative, insert at the beginning of the collection.
    this->Objects.insert(this->Objects.begin(), a);
  }
  else if (static_cast<size_t>(i) >= this->Objects.size())
  {
    return;
  }
  else
  {
    i++; // insert after the i'th item instead of before
    this->Objects.insert(this->Objects.begin() + i, a);
  }

  a->Register(this);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkCollection::RemoveItem(vtkObject* a)
{
  if (this->Objects.empty() || !a)
  {
    return;
  }

  auto it = std::find(this->Objects.begin(), this->Objects.end(), a);
  if (it != this->Objects.end())
  {
    if (it < this->Current)
    {
      this->Current--;
    }

    (*it)->UnRegister(this);
    this->Objects.erase(it);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkCollection::RemoveAllItems()
{
  // Don't modify if collection is empty
  if (this->Objects.empty())
  {
    return;
  }

  for (auto obj : this->Objects)
  {
    if (obj)
    {
      obj->UnRegister(this);
    }
  }
  this->Objects.clear();
  this->Current = this->Objects.end();
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
  if (this->Objects.empty() || !a)
  {
    return -1;
  }

  auto it = std::find(this->Objects.begin(), this->Objects.end(), a);
  if (it == this->Objects.end())
  {
    return -1;
  }
  return static_cast<int>(it - this->Objects.begin());
}

//------------------------------------------------------------------------------
int vtkCollection::IsItemPresent(vtkObject* a) VTK_FUTURE_CONST
{
  return this->IndexOfFirstOccurrence(a) + 1;
}

//------------------------------------------------------------------------------
void vtkCollection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Number Of Items: " << this->Objects.size() << "\n";
}

//------------------------------------------------------------------------------
vtkObject* vtkCollection::GetItemAsObject(int i) VTK_FUTURE_CONST
{
  if (i < 0)
  {
    return nullptr;
  }

  size_t idx = static_cast<size_t>(i);
  if (idx >= this->Objects.size())
  {
    return nullptr;
  }

  return this->Objects[idx];
}

//------------------------------------------------------------------------------
void vtkCollection::ReplaceItem(int i, vtkObject* a)
{
  if (i < 0)
  {
    return;
  }

  size_t idx = static_cast<size_t>(i);
  if (idx >= this->Objects.size())
  {
    return;
  }

  // Take care of reference counting
  this->Objects[idx]->UnRegister(this);
  a->Register(this);

  // Replace item
  this->Objects[idx] = a;

  this->Modified();
}

//------------------------------------------------------------------------------
void vtkCollection::RemoveItem(int i)
{
  if (i < 0)
  {
    return;
  }

  size_t idx = static_cast<size_t>(i);
  if (idx >= this->Objects.size())
  {
    return;
  }

  if (this->Objects.begin() + i < this->Current)
  {
    this->Current--;
  }

  this->Objects[idx]->UnRegister(this);
  this->Objects.erase(this->Objects.begin() + i);
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
  for (auto objIter = this->Objects.begin(); objIter < this->Objects.end(); objIter++)
  {
    vtkGarbageCollectorReport(collector, *objIter, "Element");
  }
}
VTK_ABI_NAMESPACE_END
