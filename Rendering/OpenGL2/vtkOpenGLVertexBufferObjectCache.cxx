/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLVertexBufferObjectCache.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLVertexBufferObjectCache.h"

#include "vtkObjectFactory.h"
#include "vtkDataArray.h"
#include "vtkOpenGLVertexBufferObject.h"


// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLVertexBufferObjectCache);

// ----------------------------------------------------------------------------
vtkOpenGLVertexBufferObjectCache::vtkOpenGLVertexBufferObjectCache()
{
}

// ----------------------------------------------------------------------------
vtkOpenGLVertexBufferObjectCache::~vtkOpenGLVertexBufferObjectCache()
{
}

void vtkOpenGLVertexBufferObjectCache::RemoveVBO(
  vtkOpenGLVertexBufferObject *vbo)
{
  vtkOpenGLVertexBufferObjectCache::VBOMap::iterator iter =
    this->MappedVBOs.begin();
  while(iter != this->MappedVBOs.end())
  {
    if(iter->second == vbo)
    {
      iter->first->UnRegister(this);
      this->MappedVBOs.erase(iter++);
    }
    else
    {
      ++iter;
    }
  }
}

// ----------------------------------------------------------------------------
vtkOpenGLVertexBufferObject* vtkOpenGLVertexBufferObjectCache::GetVBO(
  vtkDataArray *array, int destType)
{
  // Check array is valid
  if (array == NULL || array->GetNumberOfTuples() == 0)
  {
    vtkErrorMacro( << "Cannot get VBO for empty array.");
    return NULL;
  }

  // Look for VBO in map
  VBOMap::const_iterator iter = this->MappedVBOs.find(array);
  if (iter != this->MappedVBOs.end())
  {
    vtkOpenGLVertexBufferObject* vbo = iter->second;

    // Update VBO if array changed
    if (array->GetMTime() > vbo->GetUploadTime())
    {
      vbo->InitVBO(array, destType);
    }

    vbo->Register(this);
    return vbo;
  }

  // If vbo not found, create new one
  // Initialize new vbo
  vtkOpenGLVertexBufferObject* vbo = vtkOpenGLVertexBufferObject::New();
  vbo->SetCache(this);
  array->Register(this);
  vbo->InitVBO(array, destType);

  // Add vbo to map
  this->MappedVBOs[array] = vbo;
  return vbo;
}

// ----------------------------------------------------------------------------
void vtkOpenGLVertexBufferObjectCache::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
