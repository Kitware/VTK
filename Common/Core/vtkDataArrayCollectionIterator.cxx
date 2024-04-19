// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDataArrayCollectionIterator.h"
#include "vtkDataArray.h"
#include "vtkDataArrayCollection.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkDataArrayCollectionIterator);

//------------------------------------------------------------------------------
vtkDataArrayCollectionIterator::vtkDataArrayCollectionIterator() = default;

//------------------------------------------------------------------------------
vtkDataArrayCollectionIterator::~vtkDataArrayCollectionIterator() = default;

//------------------------------------------------------------------------------
void vtkDataArrayCollectionIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
void vtkDataArrayCollectionIterator::SetCollection(vtkDataArrayCollection* c)
{
  this->Superclass::SetCollection(c);
}

//------------------------------------------------------------------------------
vtkDataArray* vtkDataArrayCollectionIterator::GetDataArray()
{
  return static_cast<vtkDataArray*>(this->GetCurrentObject());
}
VTK_ABI_NAMESPACE_END
