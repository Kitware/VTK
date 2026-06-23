// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOpenGLArrayTextureBufferCache.h"

#include "vtkDataArray.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOpenGLArrayTextureBufferCache);

//------------------------------------------------------------------------------
vtkOpenGLArrayTextureBufferCache::vtkOpenGLArrayTextureBufferCache() = default;

//------------------------------------------------------------------------------
vtkOpenGLArrayTextureBufferCache::~vtkOpenGLArrayTextureBufferCache() = default;

//------------------------------------------------------------------------------
std::shared_ptr<vtkOpenGLArrayTextureBufferCache::Entry>
vtkOpenGLArrayTextureBufferCache::GetTextureBuffer(
  vtkDataArray* array, bool scalarComponents, bool integerTexture)
{
  if (array == nullptr)
  {
    vtkErrorMacro(<< "Cannot get a texture buffer for a null array.");
    return nullptr;
  }
  const Key key{ array, scalarComponents, integerTexture };
  auto it = this->Cache.find(key);
  if (it != this->Cache.end())
  {
    return it->second;
  }
  // Create a fresh, empty entry. The owning adapter fills in Texture/Buffer on
  // its next Upload(). Holding a smart pointer to the array keeps it alive while
  // cached so its address cannot be reused by a different array (which would
  // otherwise alias to this entry).
  auto entry = std::make_shared<Entry>();
  entry->Array = array;
  this->Cache[key] = entry;
  return entry;
}

//------------------------------------------------------------------------------
void vtkOpenGLArrayTextureBufferCache::ReleaseGraphicsResources(vtkWindow* window)
{
  for (auto& pair : this->Cache)
  {
    auto& entry = pair.second;
    if (entry->Texture)
    {
      entry->Texture->ReleaseGraphicsResources(window);
      entry->Texture = nullptr;
    }
    if (entry->Buffer)
    {
      entry->Buffer->ReleaseGraphicsResources();
      entry->Buffer = nullptr;
    }
    entry->UploadTime = 0;
  }
  this->Cache.clear();
}

//------------------------------------------------------------------------------
void vtkOpenGLArrayTextureBufferCache::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Number of cached texture buffers: " << this->Cache.size() << "\n";
}
VTK_ABI_NAMESPACE_END
