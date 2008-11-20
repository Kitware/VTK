/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkShader2Collection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkShader2Collection.h"
#include "vtkObjectFactory.h"
#include "vtkShader2.h"
#include <assert.h>

vtkCxxRevisionMacro(vtkShader2Collection, "1.2");
vtkStandardNewMacro(vtkShader2Collection);

// ----------------------------------------------------------------------------
// Description: 
// Reentrant safe way to get an object in a collection. Just pass the
// same cookie back and forth. 
vtkShader2 *vtkShader2Collection::GetNextShader(
vtkCollectionSimpleIterator &cookie)
{
  return static_cast<vtkShader2 *>(this->GetNextItemAsObject(cookie));
}

// ----------------------------------------------------------------------------
vtkShader2Collection::vtkShader2Collection()
{
}

// ----------------------------------------------------------------------------
vtkShader2Collection::~vtkShader2Collection()
{
}
  
// ----------------------------------------------------------------------------
// hide the standard AddItem from the user and the compiler.
void vtkShader2Collection::AddItem(vtkObject *o)
{
  this->vtkCollection::AddItem(o);
}

// ----------------------------------------------------------------------------
void vtkShader2Collection::AddItem(vtkShader2 *a) 
{
  this->vtkCollection::AddItem(a);
}

// ----------------------------------------------------------------------------
vtkShader2 *vtkShader2Collection::GetNextShader() 
{ 
  return static_cast<vtkShader2 *>(this->GetNextItemAsObject());
}

// ----------------------------------------------------------------------------
vtkShader2 *vtkShader2Collection::GetLastShader() 
{ 
  if ( this->Bottom == NULL )
    {
    return NULL;
    }
  else
    {
    return static_cast<vtkShader2 *>(this->Bottom->Item);
    }
}

// ----------------------------------------------------------------------------
// Description:
// Add the elements of `other' to the end of `this'.
// \pre other_exists: other!=0
// \pre not_self: other!=this
// \post added: this->GetNumberOfItems()=old this->GetNumberOfItems()+other->GetNumberOfItems()
void vtkShader2Collection::AddCollection(vtkShader2Collection *other)
{
  assert("pre: other_exists" && other!=0);
  assert("pre: not_self" && other!=this);
  
  other->InitTraversal();
  vtkShader2 *s=other->GetNextShader();
  while(s!=0)
    {
    this->AddItem(s);
    s=other->GetNextShader();
    }
}

// ----------------------------------------------------------------------------
// Description:
// Remove the elements of `other' from `this'. It assumes that `this' already
// has all the elements of `other' added contiguously.
// \pre other_exists: other!=0
// \pre not_self: other!=this
// \post removed: this->GetNumberOfItems()=old this->GetNumberOfItems()-other->GetNumberOfItems()
void vtkShader2Collection::RemoveCollection(vtkShader2Collection *other)
{
  assert("pre: other_exists" && other!=0);
  assert("pre: not_self" && other!=this);
  
  other->InitTraversal();
  vtkShader2 *s=other->GetNextShader();
  if(s!=0)
    {
    // `other' is not an empty list.
    int loc=this->IsItemPresent(s);
    if(loc==0)
      {
      vtkErrorMacro("try to remove the elements of vtkShader2Collection " << other << " but they don't exist in vtkShader2Collection" << this);
      return;
      }
    int size=other->GetNumberOfItems();
    --loc;
    int i=0;
    while(i<size)
      {
      this->RemoveItem(loc);
      ++i;
      }
    }
}

// ----------------------------------------------------------------------------
void vtkShader2Collection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
