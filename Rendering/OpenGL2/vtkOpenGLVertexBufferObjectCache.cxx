// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOpenGLVertexBufferObjectCache.h"

#include "vtkDataArray.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLVertexBufferObject.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOpenGLVertexBufferObjectCache);

//------------------------------------------------------------------------------
vtkOpenGLVertexBufferObjectCache::vtkOpenGLVertexBufferObjectCache() = default;

//------------------------------------------------------------------------------
vtkOpenGLVertexBufferObjectCache::~vtkOpenGLVertexBufferObjectCache() = default;

void vtkOpenGLVertexBufferObjectCache::RemoveVBO(vtkOpenGLVertexBufferObject* vbo)
{
  vtkOpenGLVertexBufferObjectCache::VBOMap::iterator iter = this->MappedVBOs.begin();
  while (iter != this->MappedVBOs.end())
  {
    if (iter->second == vbo)
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

//------------------------------------------------------------------------------
vtkOpenGLVertexBufferObject* vtkOpenGLVertexBufferObjectCache::GetVBO(
  vtkDataArray* array, int destType)
{
  // Check array is valid
  if (array == nullptr || array->GetNumberOfTuples() == 0)
  {
    vtkErrorMacro(<< "Cannot get VBO for empty array.");
    return nullptr;
  }

  // Look for VBO in map
  VBOMap::const_iterator iter = this->MappedVBOs.find(array);
  if (iter != this->MappedVBOs.end())
  {
    vtkOpenGLVertexBufferObject* vbo = iter->second;
    vbo->SetDataType(destType);
    vbo->Register(this);
    return vbo;
  }

  // If vbo not found, create new one
  // Initialize new vbo
  vtkOpenGLVertexBufferObject* vbo = vtkOpenGLVertexBufferObject::New();
  vbo->SetCache(this);
  vbo->SetDataType(destType);
  array->Register(this);

  // Add vbo to map
  this->MappedVBOs[array] = vbo;
  return vbo;
}

//------------------------------------------------------------------------------
void vtkOpenGLVertexBufferObjectCache::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
