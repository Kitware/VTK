/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCollectionIterator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCollectionIterator.h"
#include "vtkObjectFactory.h"
#include "vtkCollection.h"

vtkStandardNewMacro(vtkCollectionIterator);

//----------------------------------------------------------------------------
vtkCollectionIterator::vtkCollectionIterator()
{
  this->Element = 0;
  this->Collection = 0;
}

//----------------------------------------------------------------------------
vtkCollectionIterator::~vtkCollectionIterator()
{
  this->SetCollection(0);
}

//----------------------------------------------------------------------------
void vtkCollectionIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  if(this->Collection)
  {
    os << indent << "Collection: " << this->Collection << "\n";
  }
  else
  {
    os << indent << "Collection: (none)\n";
  }
}

//----------------------------------------------------------------------------
void vtkCollectionIterator::SetCollection(vtkCollection* collection)
{
  vtkSetObjectBodyMacro(Collection, vtkCollection, collection);
  this->GoToFirstItem();
}

//----------------------------------------------------------------------------
void vtkCollectionIterator::GoToFirstItem()
{
  if(this->Collection)
  {
    this->Element = this->Collection->Top;
  }
  else
  {
    this->Element = 0;
  }
}

//----------------------------------------------------------------------------
void vtkCollectionIterator::GoToNextItem()
{
  if(this->Element)
  {
    this->Element = this->Element->Next;
  }
}

//----------------------------------------------------------------------------
int vtkCollectionIterator::IsDoneWithTraversal()
{
  return (this->Element? 0:1);
}

//----------------------------------------------------------------------------
vtkObject* vtkCollectionIterator::GetCurrentObject()
{
  if(this->Element)
  {
    return this->Element->Item;
  }
  return 0;
}
