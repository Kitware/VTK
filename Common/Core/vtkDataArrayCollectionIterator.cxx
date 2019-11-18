/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataArrayCollectionIterator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataArrayCollectionIterator.h"
#include "vtkDataArray.h"
#include "vtkDataArrayCollection.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkDataArrayCollectionIterator);

//----------------------------------------------------------------------------
vtkDataArrayCollectionIterator::vtkDataArrayCollectionIterator() = default;

//----------------------------------------------------------------------------
vtkDataArrayCollectionIterator::~vtkDataArrayCollectionIterator() = default;

//----------------------------------------------------------------------------
void vtkDataArrayCollectionIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkDataArrayCollectionIterator::SetCollection(vtkCollection* c)
{
  if (c)
  {
    this->Superclass::SetCollection(vtkDataArrayCollection::SafeDownCast(c));
    if (!this->Collection)
    {
      vtkErrorMacro("vtkDataArrayCollectionIterator cannot traverse a " << c->GetClassName());
    }
  }
  else
  {
    this->Superclass::SetCollection(nullptr);
  }
}

//----------------------------------------------------------------------------
void vtkDataArrayCollectionIterator::SetCollection(vtkDataArrayCollection* c)
{
  this->Superclass::SetCollection(c);
}

//----------------------------------------------------------------------------
vtkDataArray* vtkDataArrayCollectionIterator::GetDataArray()
{
  return static_cast<vtkDataArray*>(this->GetCurrentObject());
}
