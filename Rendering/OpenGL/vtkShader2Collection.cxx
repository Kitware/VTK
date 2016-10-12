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
#include <cassert>

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
vtkMTimeType vtkShader2Collection::GetMTime()
{
  vtkMTimeType result = this->Superclass::GetMTime();
  this->InitTraversal();
  vtkShader2 *s = this->GetNextShader();
  while (s!=0)
  {
    vtkMTimeType time = s->GetMTime();
    if (time > result)
    {
      result = time;
    }
    s = this->GetNextShader();
  }
  return result;
}

// ----------------------------------------------------------------------------
// hide the standard AddItem from the user and the compiler.
void vtkShader2Collection::AddItem(vtkObject *o)
{
  this->vtkCollection::AddItem(o);
}

// ----------------------------------------------------------------------------
void vtkShader2Collection::AddItem(vtkShader2 *s)
{
  this->vtkCollection::AddItem(s);
}

// ----------------------------------------------------------------------------
vtkShader2 *vtkShader2Collection::GetNextShader()
{
  return static_cast<vtkShader2 *>(this->GetNextItemAsObject());
}

// ----------------------------------------------------------------------------
vtkShader2 *vtkShader2Collection::GetLastShader()
{
  return (this->Bottom) ?
    static_cast<vtkShader2 *>(this->Bottom->Item) : NULL;
}

// ----------------------------------------------------------------------------
// Description:
// Add the elements of `other' to the end of `this'.
// \pre other_exists: other!=0
// \pre not_self: other!=this
// \post added: this->GetNumberOfItems()=old this->GetNumberOfItems()+other->GetNumberOfItems()
void vtkShader2Collection::AddCollection(vtkShader2Collection *other)
{
  assert("pre: other_exists" && other != 0);
  assert("pre: not_self" && other != this);

  other->InitTraversal();
  vtkShader2 *s = other->GetNextShader();
  while (s)
  {
    this->AddItem(s);
    s = other->GetNextShader();
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
  assert("pre: other_exists" && other != 0);
  assert("pre: not_self" && other != this);

  other->InitTraversal();
  vtkShader2 *s = other->GetNextShader();
  if (s)
  {
    // `other' is not an empty list.
    int loc = this->IsItemPresent(s);
    if (loc == 0)
    {
      vtkErrorMacro("try to remove the elements of vtkShader2Collection " << other << " but they don't exist in vtkShader2Collection" << this);
      return;
    }
    int size = other->GetNumberOfItems();
    --loc;
    int i = 0;
    while (i < size)
    {
      this->RemoveItem(loc);
      ++i;
    }
  }
}

// ----------------------------------------------------------------------------
// Description:
// Tells if at least one of the shaders is on given type.
bool vtkShader2Collection::HasShadersOfType(int t)
{
  bool result = false;

  this->InitTraversal();
  vtkShader2 *s = this->GetNextShader();
  while (!result && s)
  {
    result = (s->GetType() == t);
    s = this->GetNextShader();
  }

  return result;
}
// ----------------------------------------------------------------------------
// Description:
// Tells if at least one of the shaders is a vertex shader.
// If yes, it means the vertex processing of the fixed-pipeline is bypassed.
// If no, it means the vertex processing of the fixed-pipeline is used.
bool vtkShader2Collection::HasVertexShaders()
{
  return HasShadersOfType(VTK_SHADER_TYPE_VERTEX);
}

// ----------------------------------------------------------------------------
// Description:
// Tells if at least one of the shaders is a tessellation control shader.
bool vtkShader2Collection::HasTessellationControlShaders()
{
  return HasShadersOfType(VTK_SHADER_TYPE_TESSELLATION_CONTROL);
}

// ----------------------------------------------------------------------------
// Description:
// Tells if at least one of the shaders is a tessellation evaluation shader.
bool vtkShader2Collection::HasTessellationEvaluationShaders()
{
  return HasShadersOfType(VTK_SHADER_TYPE_TESSELLATION_EVALUATION);
}

// ----------------------------------------------------------------------------
// Description:
// Tells if at least one of the shaders is a geometry shader.
bool vtkShader2Collection::HasGeometryShaders()
{
  return HasShadersOfType(VTK_SHADER_TYPE_GEOMETRY);
}

// ----------------------------------------------------------------------------
// Description:
// Tells if at least one of the shaders is a fragment shader.
// If yes, it means the fragment processing of the fixed-pipeline is
// bypassed.
// If no, it means the fragment processing of the fixed-pipeline is used.
bool vtkShader2Collection::HasFragmentShaders()
{
  return HasShadersOfType(VTK_SHADER_TYPE_FRAGMENT);
}

// ----------------------------------------------------------------------------
// Description:
// Release OpenGL resources (shader id of each item).
void vtkShader2Collection::ReleaseGraphicsResources()
{
  this->InitTraversal();
  vtkShader2 *s = this->GetNextShader();
  while (s)
  {
    s->ReleaseGraphicsResources();
    s = this->GetNextShader();
  }
}

// ----------------------------------------------------------------------------
void vtkShader2Collection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  int i = 0;
  int c = this->GetNumberOfItems();
  this->InitTraversal();
  vtkShader2 *s = this->GetNextShader();
  while (s)
  {
    os << indent << "shader #" << i << "/" << c <<endl;
    s->PrintSelf(os, indent.GetNextIndent());
    s = this->GetNextShader();
    ++i;
  }
}
