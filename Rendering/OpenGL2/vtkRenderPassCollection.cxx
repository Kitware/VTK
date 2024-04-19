// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkRenderPassCollection.h"
#include "vtkObjectFactory.h"
#include "vtkRenderPass.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkRenderPassCollection);

//------------------------------------------------------------------------------
// Description:
// Reentrant safe way to get an object in a collection. Just pass the
// same cookie back and forth.
vtkRenderPass* vtkRenderPassCollection::GetNextRenderPass(vtkCollectionSimpleIterator& cookie)
{
  return static_cast<vtkRenderPass*>(this->GetNextItemAsObject(cookie));
}

//------------------------------------------------------------------------------
vtkRenderPassCollection::vtkRenderPassCollection() = default;

//------------------------------------------------------------------------------
vtkRenderPassCollection::~vtkRenderPassCollection() = default;

//------------------------------------------------------------------------------
// hide the standard AddItem from the user and the compiler.
void vtkRenderPassCollection::AddItem(vtkObject* o)
{
  this->vtkCollection::AddItem(o);
}

//------------------------------------------------------------------------------
void vtkRenderPassCollection::AddItem(vtkRenderPass* a)
{
  this->vtkCollection::AddItem(a);
}

//------------------------------------------------------------------------------
vtkRenderPass* vtkRenderPassCollection::GetNextRenderPass()
{
  return static_cast<vtkRenderPass*>(this->GetNextItemAsObject());
}

//------------------------------------------------------------------------------
vtkRenderPass* vtkRenderPassCollection::GetLastRenderPass()
{
  return (this->Bottom) ? static_cast<vtkRenderPass*>(this->Bottom->Item) : nullptr;
}

//------------------------------------------------------------------------------
void vtkRenderPassCollection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
