/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderPassCollection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRenderPassCollection.h"
#include "vtkObjectFactory.h"
#include "vtkRenderPass.h"

vtkStandardNewMacro(vtkRenderPassCollection);

// ----------------------------------------------------------------------------
// Description:
// Reentrant safe way to get an object in a collection. Just pass the
// same cookie back and forth.
vtkRenderPass *vtkRenderPassCollection::GetNextRenderPass(
vtkCollectionSimpleIterator &cookie)
{
  return static_cast<vtkRenderPass *>(this->GetNextItemAsObject(cookie));
}

// ----------------------------------------------------------------------------
vtkRenderPassCollection::vtkRenderPassCollection()
{
}

// ----------------------------------------------------------------------------
vtkRenderPassCollection::~vtkRenderPassCollection()
{
}

// ----------------------------------------------------------------------------
// hide the standard AddItem from the user and the compiler.
void vtkRenderPassCollection::AddItem(vtkObject *o)
{
  this->vtkCollection::AddItem(o);
}

// ----------------------------------------------------------------------------
void vtkRenderPassCollection::AddItem(vtkRenderPass *a)
{
  this->vtkCollection::AddItem(a);
}

// ----------------------------------------------------------------------------
vtkRenderPass *vtkRenderPassCollection::GetNextRenderPass()
{
  return static_cast<vtkRenderPass *>(this->GetNextItemAsObject());
}

// ----------------------------------------------------------------------------
vtkRenderPass *vtkRenderPassCollection::GetLastRenderPass()
{
  return (this->Bottom) ?
    static_cast<vtkRenderPass *>(this->Bottom->Item) : NULL;
}

// ----------------------------------------------------------------------------
void vtkRenderPassCollection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
