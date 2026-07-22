// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOpenGLArrayTextureBufferCache.h"

#include "vtkDataArray.h"
#include "vtkObjectFactory.h"

#include <algorithm>
#include <utility>
#include <vector>

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
    it->second->LastAccess = ++this->AccessCounter;
    return it->second;
  }
  // Create a fresh, empty entry. The owning adapter fills in Texture/Buffer on
  // its next Upload(). Holding a smart pointer to the array keeps it alive while
  // cached so its address cannot be reused by a different array (which would
  // otherwise alias to this entry).
  auto entry = std::make_shared<Entry>();
  entry->Array = array;
  entry->LastAccess = ++this->AccessCounter;
  this->Cache[key] = entry;
  return entry;
}

//------------------------------------------------------------------------------
std::size_t vtkOpenGLArrayTextureBufferCache::GetEntrySize(const Entry& entry)
{
  if (entry.Buffer)
  {
    // Desktop: the buffer object is the texture's actual storage.
    return entry.Buffer->GetSize();
  }
  if (entry.Texture && entry.Texture->GetHandle())
  {
    // GLES: the samplerBuffer is emulated with a 2D texture that owns its
    // storage; compute the allocation from its dimensions.
    const int components = std::max(entry.Texture->GetComponents(), 1);
    const int typeSize =
      std::max(vtkAbstractArray::GetDataTypeSize(entry.Texture->GetVTKDataType()), 1);
    return static_cast<std::size_t>(entry.Texture->GetWidth()) * entry.Texture->GetHeight() *
      components * typeSize;
  }
  return 0;
}

//------------------------------------------------------------------------------
bool vtkOpenGLArrayTextureBufferCache::EntryIsPinned(const Entry& entry)
{
  return (entry.Texture && entry.Texture->GetReferenceCount() > 1) ||
    (entry.Buffer && entry.Buffer->GetReferenceCount() > 1);
}

//------------------------------------------------------------------------------
void vtkOpenGLArrayTextureBufferCache::ReleaseEntry(Entry& entry, vtkWindow* window)
{
  if (entry.Texture)
  {
    entry.Texture->ReleaseGraphicsResources(window);
    entry.Texture = nullptr;
  }
  if (entry.Buffer)
  {
    entry.Buffer->ReleaseGraphicsResources();
    entry.Buffer = nullptr;
  }
  entry.UploadTime = 0;
}

//------------------------------------------------------------------------------
std::size_t vtkOpenGLArrayTextureBufferCache::GetCurrentCacheSize() const
{
  std::size_t total = 0;
  for (const auto& pair : this->Cache)
  {
    total += vtkOpenGLArrayTextureBufferCache::GetEntrySize(*pair.second);
  }
  return total;
}

//------------------------------------------------------------------------------
void vtkOpenGLArrayTextureBufferCache::RemoveUnusedTextureBuffers(vtkWindow* window)
{
  if (window == nullptr)
  {
    // Releasing a texture requires the owning render window (it deactivates the
    // texture unit); without one, eviction is unsafe.
    return;
  }
  // Pass 1: drop orphans unconditionally. An orphaned array is owned solely by
  // the cache. The client destroyed every other reference, so its key can
  // never be requested again and both the array and its GL objects are garbage.
  for (auto it = this->Cache.begin(); it != this->Cache.end();)
  {
    Entry& entry = *it->second;
    if (!vtkOpenGLArrayTextureBufferCache::EntryIsPinned(entry) && entry.Array &&
      entry.Array->GetReferenceCount() == 1)
    {
      vtkOpenGLArrayTextureBufferCache::ReleaseEntry(entry, window);
      it = this->Cache.erase(it);
    }
    else
    {
      ++it;
    }
  }
  // Pass 2: enforce the byte budget on what survived the first pass, evicting
  // unpinned entries LRU first. Pinned entries are exempt, so the total can stay
  // above the budget while a large scene is live.
  if (this->MaximumCacheSize == 0)
  {
    return;
  }
  std::size_t total = this->GetCurrentCacheSize();
  if (total <= this->MaximumCacheSize)
  {
    return;
  }
  std::vector<std::pair<std::uint64_t, Key>> candidates;
  candidates.reserve(this->Cache.size());
  for (const auto& pair : this->Cache)
  {
    if (!vtkOpenGLArrayTextureBufferCache::EntryIsPinned(*pair.second))
    {
      candidates.emplace_back(pair.second->LastAccess, pair.first);
    }
  }
  std::sort(candidates.begin(), candidates.end(),
    [](const std::pair<std::uint64_t, Key>& a, const std::pair<std::uint64_t, Key>& b)
    { return a.first < b.first; });
  for (const auto& candidate : candidates)
  {
    if (total <= this->MaximumCacheSize)
    {
      break;
    }
    auto it = this->Cache.find(candidate.second);
    if (it == this->Cache.end())
    {
      continue;
    }
    const std::size_t entrySize = vtkOpenGLArrayTextureBufferCache::GetEntrySize(*it->second);
    vtkOpenGLArrayTextureBufferCache::ReleaseEntry(*it->second, window);
    this->Cache.erase(it);
    total -= std::min(entrySize, total);
  }
}

//------------------------------------------------------------------------------
std::size_t vtkOpenGLArrayTextureBufferCache::RemoveTextureBuffer(
  vtkDataArray* array, vtkWindow* window)
{
  if (array == nullptr || window == nullptr)
  {
    return 0;
  }
  std::size_t removed = 0;
  for (bool scalarComponents : { false, true })
  {
    for (bool integerTexture : { false, true })
    {
      auto it = this->Cache.find(Key{ array, scalarComponents, integerTexture });
      if (it == this->Cache.end() || vtkOpenGLArrayTextureBufferCache::EntryIsPinned(*it->second))
      {
        continue;
      }
      vtkOpenGLArrayTextureBufferCache::ReleaseEntry(*it->second, window);
      this->Cache.erase(it);
      ++removed;
    }
  }
  return removed;
}

//------------------------------------------------------------------------------
unsigned long vtkOpenGLArrayTextureBufferCache::GetCPUMemorySize() const
{
  unsigned long totalKiB = 0;
  for (const auto& pair : this->Cache)
  {
    if (pair.second->Array)
    {
      totalKiB += pair.second->Array->GetActualMemorySize();
    }
  }
  return totalKiB;
}

//------------------------------------------------------------------------------
void vtkOpenGLArrayTextureBufferCache::ReleaseGraphicsResources(vtkWindow* window)
{
  for (auto& pair : this->Cache)
  {
    vtkOpenGLArrayTextureBufferCache::ReleaseEntry(*pair.second, window);
  }
  this->Cache.clear();
}

//------------------------------------------------------------------------------
void vtkOpenGLArrayTextureBufferCache::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Number of cached texture buffers: " << this->Cache.size() << "\n";
  os << indent << "CPU memory pinned by cached arrays: " << this->GetCPUMemorySize() << " KiB\n";
  os << indent << "MaximumCacheSize: " << this->MaximumCacheSize << " bytes\n";
  os << indent << "Current GPU cache size: " << this->GetCurrentCacheSize() << " bytes\n";
}
VTK_ABI_NAMESPACE_END
