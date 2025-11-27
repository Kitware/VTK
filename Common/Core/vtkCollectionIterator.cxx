// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Hide VTK_DEPRECATED_IN_X_Y_Z() warnings for this class.
#define VTK_DEPRECATION_LEVEL 0

#include "vtkCollectionIterator.h"
#include "vtkCollection.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkCollectionIterator);

//------------------------------------------------------------------------------
vtkCollectionIterator::vtkCollectionIterator()
{
  this->Collection = nullptr;
}

//------------------------------------------------------------------------------
vtkCollectionIterator::~vtkCollectionIterator()
{
  this->SetCollection(nullptr);
}

//------------------------------------------------------------------------------
void vtkCollectionIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->Collection)
  {
    os << indent << "Collection: " << this->Collection << "\n";
  }
  else
  {
    os << indent << "Collection: (none)\n";
  }
}

//------------------------------------------------------------------------------
void vtkCollectionIterator::SetCollection(vtkCollection* collection)
{
  vtkSetObjectBodyMacro(Collection, vtkCollection, collection);
  this->GoToFirstItem();
}

//------------------------------------------------------------------------------
void vtkCollectionIterator::GoToFirstItem()
{
  if (this->Collection)
  {
    this->Iterator = this->Collection->begin();
  }
}

//------------------------------------------------------------------------------
void vtkCollectionIterator::GoToNextItem()
{
  if (this->Collection && this->Iterator < this->Collection->end())
  {
    this->Iterator++;
  }
}

//------------------------------------------------------------------------------
int vtkCollectionIterator::IsDoneWithTraversal()
{
  return (this->Iterator >= this->Collection->end());
}

//------------------------------------------------------------------------------
vtkObject* vtkCollectionIterator::GetCurrentObject()
{
  if (this->Collection && this->Iterator < this->Collection->end())
  {
    return *this->Iterator;
  }
  return nullptr;
}
VTK_ABI_NAMESPACE_END
