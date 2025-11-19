// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPURenderTextureCache.h"
#include "vtkObjectFactory.h"
#include "vtkWebGPURenderTextureDeviceResource.h"

#include <limits>
#include <stack>
#include <unordered_map>

VTK_ABI_NAMESPACE_BEGIN

class vtkWebGPURenderTextureCache::vtkInternals
{
public:
  std::stack<int> AvailableIndices;
  std::atomic<int> NextAvailableIndex{ 0 };
  std::unordered_map<int, vtkSmartPointer<vtkWebGPURenderTextureDeviceResource>> RenderTextureCache;

  int GetNextAvailableIndex(vtkObject* caller)
  {
    // Reuse an available index if possible
    if (!this->AvailableIndices.empty())
    {
      const int index = this->AvailableIndices.top();
      this->AvailableIndices.pop();
      return index;
    }
    // Otherwise, generate a new index
    if (NextAvailableIndex == std::numeric_limits<int>::max())
    {
      vtkErrorWithObjectMacro(
        caller, "RenderTextureCache has reached maximum capacity. Please release unused textures.");
      return INVALID_TEXTURE_INDEX;
    }
    return NextAvailableIndex++;
  }
};

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkWebGPURenderTextureCache);

//------------------------------------------------------------------------------
vtkWebGPURenderTextureCache::vtkWebGPURenderTextureCache()
  : Internals(new vtkInternals())
{
}

//------------------------------------------------------------------------------
vtkWebGPURenderTextureCache::~vtkWebGPURenderTextureCache() = default;

//------------------------------------------------------------------------------
void vtkWebGPURenderTextureCache::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "RenderTextureCache: \n";
  for (const auto& iter : this->Internals->RenderTextureCache)
  {
    os << iter.first << ": " << iter.second.Get() << '\n';
  }
}

//------------------------------------------------------------------------------
void vtkWebGPURenderTextureCache::ReleaseGraphicsResources(vtkWindow* window)
{
  for (const auto& iter : this->Internals->RenderTextureCache)
  {
    iter.second->ReleaseGraphicsResources(window);
  }
  // This will clear the cache, reset the available indices stack, and
  // reset the next available index counter.
  this->Internals.reset(new vtkInternals());
}

//------------------------------------------------------------------------------
int vtkWebGPURenderTextureCache::AddRenderTexture(
  vtkSmartPointer<vtkWebGPURenderTextureDeviceResource> renderTexture)
{
  const auto index = this->Internals->GetNextAvailableIndex(this);
  if (index != INVALID_TEXTURE_INDEX)
  {
    this->Internals->RenderTextureCache[index] = renderTexture;
  }
  // If index == INVALID_TEXTURE_INDEX, the caller must handle this error case.
  return index;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkWebGPURenderTextureDeviceResource> vtkWebGPURenderTextureCache::GetRenderTexture(
  int index)
{
  auto iter = this->Internals->RenderTextureCache.find(index);
  if (iter != this->Internals->RenderTextureCache.end())
  {
    return iter->second;
  }
  return nullptr;
}

//------------------------------------------------------------------------------
bool vtkWebGPURenderTextureCache::RemoveRenderTexture(int index)
{
  const bool wasErased = this->Internals->RenderTextureCache.erase(index) > 0;
  if (wasErased)
  {
    this->Internals->AvailableIndices.push(index);
  }
  return wasErased;
}
VTK_ABI_NAMESPACE_END
