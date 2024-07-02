// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUInternalsComputePassTextureStorage.h"
#include "vtkObjectFactory.h"
#include "vtkWebGPUComputePass.h"
#include "vtkWebGPUComputePipeline.h"
#include "vtkWebGPUInternalsTexture.h"
#include <vtkSmartPointer.h>

#include <algorithm> // for std::remove_if

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkWebGPUInternalsComputePassTextureStorage);

namespace
{
struct InternalMapTextureAsyncData
{
  // Buffer currently being mapped
  wgpu::Buffer buffer;
  // Label of the buffer currently being mapped. Used for printing errors
  std::string bufferLabel;
  // Size of the buffer being mapped in bytes
  vtkIdType byteSize;

  // Userdata passed to userCallback. This is typically the structure that contains the CPU-side
  // buffer into which the data of the mapped buffer will be copied
  void* userdata;

  // Bytes per row of the padded buffer that contains the mapped texture data
  int bytesPerRow;
  // Callback given by the user
  vtkWebGPUInternalsComputePassTextureStorage::TextureMapAsyncCallback userCallback;
};
}

//------------------------------------------------------------------------------
void vtkWebGPUInternalsComputePassTextureStorage::SetParentDevice(wgpu::Device device)
{
  this->ParentPassDevice = device;
}

//------------------------------------------------------------------------------
void vtkWebGPUInternalsComputePassTextureStorage::SetComputePass(
  vtkWeakPointer<vtkWebGPUComputePass> parentComputePass)
{
  this->ParentComputePass = parentComputePass;
  this->ParentPassDevice = parentComputePass->Internals->Device;
}

//------------------------------------------------------------------------------
bool vtkWebGPUInternalsComputePassTextureStorage::CheckTextureIndex(
  int textureIndex, const std::string& callerFunctionName)
{
  if (textureIndex < 0)
  {
    vtkLog(ERROR,
      "Negative textureIndex given to "
        << callerFunctionName << ". Make sure to use an index that was returned by AddTexture().");

    return false;
  }
  else if (textureIndex >= this->Textures.size())
  {
    vtkLog(ERROR,
      "Invalid textureIndex given to "
        << callerFunctionName << ". Index was '" << textureIndex << "' while there are "
        << this->Textures.size()
        << " available textures. Make sure to use an index that was returned by AddTexture().");

    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkWebGPUInternalsComputePassTextureStorage::CheckTextureViewIndex(
  int textureViewIndex, const std::string& callerFunctionName)
{
  if (textureViewIndex < 0)
  {
    vtkLog(ERROR,
      "Negative textureViewIndex given to "
        << callerFunctionName
        << ". Make sure to use an index that was returned by AddTextureView().");

    return false;
  }
  else if (textureViewIndex >= this->TextureViewsToWebGPUTextureViews.size())
  {
    vtkLog(ERROR,
      "Invalid textureViewIndex given to " << callerFunctionName << ". Index was '"
                                           << textureViewIndex << "' while there are "
                                           << this->TextureViewsToWebGPUTextureViews.size()
                                           << " available texture views. Make sure to use an index "
                                              "that was returned by AddTextureView().");

    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkWebGPUInternalsComputePassTextureStorage::CheckTextureCorrectness(
  vtkWebGPUComputeTexture* texture)
{
  const char* textureLabel = texture->GetLabel().c_str();

  if (texture->GetWidth() == 0 || texture->GetHeight() == 0 || texture->GetDepth() == 0)
  {
    vtkLog(ERROR,
      "The texture with label "
        << textureLabel
        << " had one of its size (width, heigh or depth) 0. Did you forget to call SetSize()?");

    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkWebGPUInternalsComputePassTextureStorage::CheckTextureViewCorrectness(
  vtkWebGPUComputeTextureView* textureView)
{
  std::string textureViewLabel = textureView->GetLabel();

  if (textureView->GetBinding() == -1)
  {
    vtkLog(ERROR,
      "The texture with label "
        << textureViewLabel
        << " had its binding uninitialized. Did you forget to call SetBinding()?");

    return false;
  }
  else if (textureView->GetGroup() == -1)
  {
    vtkLog(ERROR,
      "The texture with label "
        << textureViewLabel << " had its group uninitialized. Did you forget to call SetGroup()?");

    return false;
  }
  else
  {
    // Checking that the buffer isn't already used
    for (auto texViewEntry : this->TextureViewsToWebGPUTextureViews)
    {
      vtkSmartPointer<vtkWebGPUComputeTextureView> existingTextureView = texViewEntry.first;

      if (textureView->GetBinding() == existingTextureView->GetBinding() &&
        textureView->GetGroup() == existingTextureView->GetGroup())
      {
        vtkLog(ERROR,
          "The texture with label"
            << textureViewLabel << " is bound to binding " << textureView->GetBinding()
            << " but that binding is already used by texture with label \""
            << existingTextureView->GetLabel() << "\" in bind group " << textureView->GetGroup());

        return false;
      }
    }
  }

  return true;
}

//------------------------------------------------------------------------------
void vtkWebGPUInternalsComputePassTextureStorage::RecreateTexture(int textureIndex)
{
  vtkSmartPointer<vtkWebGPUComputeTexture> texture = this->Textures[textureIndex];

  std::string textureLabel = texture->GetLabel();
  wgpu::TextureDimension dimension =
    vtkWebGPUInternalsComputePassTextureStorage::ComputeTextureDimensionToWebGPU(
      texture->GetDimension());
  wgpu::TextureFormat format =
    vtkWebGPUInternalsComputePassTextureStorage::ComputeTextureFormatToWebGPU(texture->GetFormat());
  wgpu::TextureUsage usage = vtkWebGPUInternalsComputePassTextureStorage::ComputeTextureModeToUsage(
    texture->GetMode(), textureLabel);
  int mipLevelCount = texture->GetMipLevelCount();

  wgpu::Extent3D extents = { texture->GetWidth(), texture->GetHeight(), texture->GetDepth() };

  this->WebGPUTextures[textureIndex] = vtkWebGPUInternalsTexture::CreateATexture(
    this->ParentPassDevice, extents, dimension, format, usage, mipLevelCount, textureLabel);
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkWebGPUComputeTexture>
vtkWebGPUInternalsComputePassTextureStorage::GetComputeTexture(int textureIndex)
{
  if (!this->CheckTextureIndex(textureIndex, "GetComputeTexture"))
  {
    return nullptr;
  }

  return this->Textures[textureIndex];
}

//------------------------------------------------------------------------------
void vtkWebGPUInternalsComputePassTextureStorage::UpdateComputeTextureAndViews(
  vtkSmartPointer<vtkWebGPUComputeTexture> texture, wgpu::Texture newWgpuTexture)
{
  int textureIndex = 0;

  // Finding the index of the texture that needs to be updated as well as updating it with the
  // newWgpuTexture
  for (const vtkSmartPointer<vtkWebGPUComputeTexture>& computePassTexture : this->Textures)
  {
    if (computePassTexture == texture)
    {
      this->WebGPUTextures[textureIndex] = newWgpuTexture;

      break;
    }

    textureIndex++;
  }

  if (textureIndex == this->Textures.size())
  {
    // The texture isn't in the pipeline, nothing to update

    return;
  }

  // Updating the views that were using this texture
  for (vtkSmartPointer<vtkWebGPUComputeTextureView> textureView :
    this->ComputeTextureToViews[texture])
  {
    // Update the view
    wgpu::TextureView newTextureView = CreateWebGPUTextureView(textureView, newWgpuTexture);
    this->TextureViewsToWebGPUTextureViews[textureView] = newTextureView;

    // Finding the bind group / bind group layout entries that need to be recreated
    uint32_t binding = textureView->GetBinding();
    uint32_t group = textureView->GetGroup();

    int entryIndex = 0;
    auto find = this->ParentComputePass->Internals->BindGroupLayoutEntries.find(group);
    if (find == this->ParentComputePass->Internals->BindGroupLayoutEntries.end())
    {
      // The group of the texture view isn't in the bindings, this may be because the texture view
      // isn't bound the shader yet

      continue;
    }

    std::vector<wgpu::BindGroupLayoutEntry>& bgEntries = find->second;
    for (wgpu::BindGroupLayoutEntry& bglEntry : bgEntries)
    {
      if (bglEntry.binding == binding)
      {
        break;
      }

      entryIndex++;
    }

    if (entryIndex == this->ParentComputePass->Internals->BindGroupLayoutEntries[group].size())
    {
      // The binding of the texture view wasn't found in the group. This may not be
      // an error if the user intends to rebind the texture views later i.e. if the user has 5 views
      // of the same texture for example but only 2 bindings in the shader. The user may then want
      // to rebind one of the five texture view to one of the two bindings in the shader. This means
      // that texture views not currently bound to the shader will not be found in the bindings and
      // we get here.
      // No bind groups to recreate for this texture view, moving on to the next.
      continue;
    }

    // Now that we have the index of the entries that need to be recreated, we can recreate them
    // with the newTextureView
    wgpu::BindGroupLayoutEntry newBglEntry =
      this->ParentComputePass->Internals->CreateBindGroupLayoutEntry(binding, texture, textureView);
    wgpu::BindGroupEntry newBgEntry =
      this->ParentComputePass->Internals->CreateBindGroupEntry(binding, newTextureView);

    this->ParentComputePass->Internals->BindGroupLayoutEntries[group][entryIndex] = newBglEntry;
    this->ParentComputePass->Internals->BindGroupEntries[group][entryIndex] = newBgEntry;
  }

  this->ParentComputePass->Internals->BindGroupOrLayoutsInvalidated = true;
}

//------------------------------------------------------------------------------
void vtkWebGPUInternalsComputePassTextureStorage::RecreateComputeTexture(int textureIndex)
{
  if (!this->CheckTextureIndex(textureIndex, "RecreateComputeTexture"))
  {
    return;
  }

  vtkSmartPointer<vtkWebGPUComputeTexture> texture = this->Textures[textureIndex];

  this->RecreateTexture(textureIndex);
  this->RecreateTextureViews(textureIndex);
  this->ParentComputePass->Internals->RecreateTextureBindGroup(textureIndex);

  // Registering the texture with the new texture recreated by previous calls
  this->ParentComputePass->Internals->RegisterTextureToPipeline(
    texture, this->WebGPUTextures[textureIndex]);
}

//------------------------------------------------------------------------------
void vtkWebGPUInternalsComputePassTextureStorage::RecreateTextureViews(int textureIndex)
{
  if (!this->CheckTextureIndex(textureIndex, "RecreateTextureViews"))
  {
    return;
  }

  wgpu::Texture wgpuTexture = this->WebGPUTextures[textureIndex];
  vtkSmartPointer<vtkWebGPUComputeTexture> texture = this->Textures[textureIndex];
  for (vtkSmartPointer<vtkWebGPUComputeTextureView> textureView :
    this->ComputeTextureToViews[texture])
  {
    wgpu::TextureView newWgpuTextureView = CreateWebGPUTextureView(textureView, wgpuTexture);

    this->TextureViewsToWebGPUTextureViews[textureView] = newWgpuTextureView;
  }
}

//------------------------------------------------------------------------------
wgpu::TextureView vtkWebGPUInternalsComputePassTextureStorage::CreateWebGPUTextureView(
  vtkSmartPointer<vtkWebGPUComputeTextureView> textureView, wgpu::Texture wgpuTexture)
{
  std::string textureViewLabel = textureView->GetLabel().c_str();
  wgpu::TextureViewDimension textureViewDimension =
    vtkWebGPUInternalsComputePassTextureStorage::ComputeTextureDimensionToViewDimension(
      textureView->GetDimension());
  // Creating a "full" view of the texture
  wgpu::TextureAspect textureViewAspect =
    vtkWebGPUInternalsComputePassTextureStorage::ComputeTextureViewAspectToWebGPU(
      textureView->GetAspect());
  wgpu::TextureFormat textureViewFormat =
    vtkWebGPUInternalsComputePassTextureStorage::ComputeTextureFormatToWebGPU(
      textureView->GetFormat());
  int baseMipLevel = textureView->GetBaseMipLevel();
  int mipLevelCount = textureView->GetMipLevelCount();

  return vtkWebGPUInternalsTexture::CreateATextureView(this->ParentPassDevice, wgpuTexture,
    textureViewDimension, textureViewAspect, textureViewFormat, baseMipLevel, mipLevelCount,
    textureViewLabel);
}

//------------------------------------------------------------------------------
void vtkWebGPUInternalsComputePassTextureStorage::AddRenderTexture(
  vtkSmartPointer<vtkWebGPUComputeRenderTexture> renderTexture)
{
  renderTexture->SetAssociatedComputePass(this->ParentComputePass);

  this->RenderTextures.push_back(renderTexture);
}

//------------------------------------------------------------------------------
int vtkWebGPUInternalsComputePassTextureStorage::AddTexture(
  vtkSmartPointer<vtkWebGPUComputeTexture> texture)
{
  wgpu::Extent3D textureExtents = { texture->GetWidth(), texture->GetHeight(),
    texture->GetDepth() };

  if (!CheckTextureCorrectness(texture))
  {
    return -1;
  }

  std::string textureLabel = texture->GetLabel();
  wgpu::Texture wgpuTexture;

  // Check if this texture has already been created for another compute pass and has been registered
  // in the compute pipeline. If not, we need to create it
  if (!this->ParentComputePass->Internals->GetRegisteredTextureFromPipeline(texture, wgpuTexture))
  {
    wgpu::TextureUsage textureUsage =
      vtkWebGPUInternalsComputePassTextureStorage::ComputeTextureModeToUsage(
        texture->GetMode(), texture->GetLabel());
    wgpu::TextureFormat format =
      vtkWebGPUInternalsComputePassTextureStorage::ComputeTextureFormatToWebGPU(
        texture->GetFormat());
    wgpu::TextureDimension dimension =
      vtkWebGPUInternalsComputePassTextureStorage::ComputeTextureDimensionToWebGPU(
        texture->GetDimension());
    int mipLevelCount = texture->GetMipLevelCount();

    wgpuTexture = vtkWebGPUInternalsTexture::CreateATexture(this->ParentPassDevice, textureExtents,
      dimension, format, textureUsage, mipLevelCount, textureLabel.c_str());

    texture->SetByteSize(textureExtents.width * textureExtents.height *
      textureExtents.depthOrArrayLayers * texture->GetBytesPerPixel());

    // The texture is read only by the shader if it doesn't have CopySrc (meaning that we would be
    // mapping the texture from the GPU to read its results on the CPU meaning that the shader
    // writes to the texture)
    bool textureReadOnly = !(textureUsage | wgpu::TextureUsage::CopySrc);
    // Uploading from std::vector or vtkDataArray if one of the two is present
    switch (texture->GetDataType())
    {
      case vtkWebGPUComputeTexture::TextureDataType::STD_VECTOR:
      {
        if (texture->GetDataPointer() != nullptr)
        {
          vtkWebGPUInternalsTexture::Upload(this->ParentPassDevice, wgpuTexture,
            texture->GetBytesPerPixel() * textureExtents.width, texture->GetByteSize(),
            texture->GetDataPointer());
        }
        else if (textureReadOnly)
        {
          // Only warning if we're using a read only texture without uploading data to initialize it

          vtkLog(WARNING,
            "The texture with label \""
              << textureLabel
              << "\" has data type STD_VECTOR but no std::vector data was "
                 "given. No data uploaded.");
        }
        break;
      }

      case vtkWebGPUComputeTexture::TextureDataType::VTK_DATA_ARRAY:
      {
        if (texture->GetDataArray() != nullptr)
        {
          vtkWebGPUInternalsTexture::UploadFromDataArray(this->ParentPassDevice, wgpuTexture,
            texture->GetBytesPerPixel() * textureExtents.width, texture->GetDataArray());
        }
        else if (textureReadOnly)
        {
          // Only warning if we're using a read only texture without uploading data to initialize it

          vtkLog(WARNING,
            "The texture with label \""
              << textureLabel
              << "\" has data type VTK_DATA_ARRAY but no vtkDataArray data "
                 "was given. No data uploaded.");
        }
        break;
      }

      default:
        break;
    }

    // The texture view isn't created immediately so we're registering with a null textureView for
    // now
    this->ParentComputePass->Internals->RegisterTextureToPipeline(texture, wgpuTexture);
  }

  this->Textures.push_back(texture);
  this->WebGPUTextures.push_back(wgpuTexture);

  return this->Textures.size() - 1;
}

//------------------------------------------------------------------------------
int vtkWebGPUInternalsComputePassTextureStorage::AddTextureView(
  vtkSmartPointer<vtkWebGPUComputeTextureView> textureView)
{
  int associatedTextureIndex = textureView->GetAssociatedTextureIndex();
  if (associatedTextureIndex == -1)
  {
    vtkLog(ERROR,
      "The texture view with label \""
        << textureView->GetLabel()
        << "\" has no assicated texture index. Make sure you obtained the textureView by calling "
           "vtkWebGPUComputePass::CreateTextureView().");

    return -1;
  }

  vtkSmartPointer<vtkWebGPUComputeTexture> texture = this->Textures[associatedTextureIndex];
  wgpu::Texture wgpuTexture = this->WebGPUTextures[associatedTextureIndex];
  wgpu::TextureView wgpuTextureView = this->CreateWebGPUTextureView(textureView, wgpuTexture);

  // Note that here, group and binding may be -1 if the texture view wasn't given a group/binding
  // combination. This is valid if the user intends to rebind the texture view to a group / binding
  // later. If the user actually forgot to set the group / binding, and doesn't rebind the texture
  // view, the compute pass will crash when dispatching anyway so the error will be caught at some
  // point
  vtkIdType group = textureView->GetGroup();
  vtkIdType binding = textureView->GetBinding();
  if (group > -1 && binding > -1)
  {
    // Only creating the bind group layout and bind group if the group and binding are valid,
    // they will be created by RebindTextureView otherwise
    wgpu::BindGroupLayoutEntry bglEntry;
    wgpu::BindGroupEntry bgEntry;
    bglEntry =
      this->ParentComputePass->Internals->CreateBindGroupLayoutEntry(binding, texture, textureView);
    bgEntry = this->ParentComputePass->Internals->CreateBindGroupEntry(binding, wgpuTextureView);

    this->ParentComputePass->Internals->BindGroupLayoutEntries[group].push_back(bglEntry);
    this->ParentComputePass->Internals->BindGroupEntries[group].push_back(bgEntry);
  }

  this->ComputeTextureToViews[texture].insert(textureView);
  this->TextureViews.push_back(textureView);
  this->TextureViewsToWebGPUTextureViews[textureView] = wgpuTextureView;

  return this->TextureViews.size() - 1;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkWebGPUComputeTextureView>
vtkWebGPUInternalsComputePassTextureStorage::CreateTextureView(int textureIndex)
{
  if (!CheckTextureIndex(textureIndex, "CreateTextureView"))
  {
    return nullptr;
  }

  vtkSmartPointer<vtkWebGPUComputeTexture> texture = this->Textures[textureIndex];

  vtkSmartPointer<vtkWebGPUComputeTextureView> textureView =
    vtkSmartPointer<vtkWebGPUComputeTextureView>::New();

  textureView->SetDimension(texture->GetDimension());
  textureView->SetFormat(texture->GetFormat());
  textureView->SetAssociatedTextureIndex(textureIndex);

  return textureView;
}

//------------------------------------------------------------------------------
void vtkWebGPUInternalsComputePassTextureStorage::SetupRenderTexture(
  vtkSmartPointer<vtkWebGPUComputeRenderTexture> renderTexture,
  wgpu::TextureViewDimension textureViewDimension, wgpu::TextureView textureView)
{
  if (renderTexture->GetWebGPUTexture() == nullptr)
  {
    vtkLog(ERROR,
      "The given render texture with label \""
        << renderTexture->GetLabel()
        << "\" does not have an assigned WebGPUTexture meaning that it will not reuse an "
           "existing "
           "texture of the render pipeline. The issue probably is that SetWebGPUTexture() wasn't "
           "called.");

    return;
  }

  // Creating the entries for this existing render texture
  uint32_t group = renderTexture->GetGroup();
  uint32_t binding = renderTexture->GetBinding();
  wgpu::BindGroupLayoutEntry bglEntry;
  wgpu::BindGroupEntry bgEntry;
  bglEntry = this->ParentComputePass->Internals->CreateBindGroupLayoutEntry(
    binding, renderTexture, textureViewDimension);
  bgEntry = this->ParentComputePass->Internals->CreateBindGroupEntry(binding, textureView);

  this->ParentComputePass->Internals->BindGroupLayoutEntries[group].push_back(bglEntry);
  this->ParentComputePass->Internals->BindGroupEntries[group].push_back(bgEntry);
  this->ParentComputePass->Internals->BindGroupOrLayoutsInvalidated = true;

  this->RenderTexturesToWebGPUTexture[renderTexture] = renderTexture->GetWebGPUTexture();
}

//------------------------------------------------------------------------------
void vtkWebGPUInternalsComputePassTextureStorage::RecreateRenderTexture(
  vtkSmartPointer<vtkWebGPUComputeRenderTexture> renderTexture,
  wgpu::TextureViewDimension textureViewDimension, wgpu::TextureView textureView)
{
  if (renderTexture->GetWebGPUTexture() == nullptr)
  {
    vtkLog(ERROR,
      "The given render texture with label \""
        << renderTexture->GetLabel()
        << "\" does not have an assigned WebGPUTexture meaning that it will not reuse an "
           "existing "
           "texture of the render pipeline. The issue probably is that SetWebGPUTexture() wasn't "
           "called.");

    return;
  }

  // Creating the entries for this existing render texture
  uint32_t group = renderTexture->GetGroup();
  uint32_t binding = renderTexture->GetBinding();

  // Finding the index of the bind group layout / bind group entry that corresponds to the
  // previously created render texture
  int entryIndex = 0;
  for (wgpu::BindGroupLayoutEntry& existingBglEntry :
    this->ParentComputePass->Internals->BindGroupLayoutEntries[group])
  {
    if (existingBglEntry.binding == renderTexture->GetBinding())
    {
      break;
    }

    // Incrementing the index to know which bind group / bind group layout entry we're going to
    // override
    entryIndex++;
  }

  if (entryIndex == this->ParentComputePass->Internals->BindGroupLayoutEntries[group].size())
  {
    // We couldn't find the entry
    vtkLog(ERROR,
      "Couldn't find the bind group layout entry of the render texture with label \""
        << renderTexture->GetLabel()
        << "\". Did you forget to call SetupRenderTexture() before trying to recreate the "
           "texture?");

    return;
  }

  wgpu::BindGroupLayoutEntry bglEntry =
    this->ParentComputePass->Internals->CreateBindGroupLayoutEntry(
      binding, renderTexture, textureViewDimension);
  wgpu::BindGroupEntry bgEntry =
    this->ParentComputePass->Internals->CreateBindGroupEntry(binding, textureView);
  this->ParentComputePass->Internals->BindGroupLayoutEntries[group][entryIndex] = bglEntry;
  this->ParentComputePass->Internals->BindGroupEntries[group][entryIndex] = bgEntry;
  this->ParentComputePass->Internals->BindGroupOrLayoutsInvalidated = true;

  this->RenderTexturesToWebGPUTexture[renderTexture] = renderTexture->GetWebGPUTexture();
}

//------------------------------------------------------------------------------
void vtkWebGPUInternalsComputePassTextureStorage::DeleteTextureViews(int textureIndex)
{
  if (!this->CheckTextureIndex(textureIndex, "DeleteTextureViews"))
  {
    return;
  }

  vtkSmartPointer<vtkWebGPUComputeTexture> texture = this->Textures[textureIndex];
  std::unordered_set<vtkSmartPointer<vtkWebGPUComputeTextureView>> textureViewsToDelete =
    this->ComputeTextureToViews[texture];

  // New vector of texture views that will contain all the texture views of this pipeline but
  // without the texture views that we're about to delete
  std::vector<vtkSmartPointer<vtkWebGPUComputeTextureView>> updatedTextureViews;
  updatedTextureViews.reserve(this->TextureViews.size() - textureViewsToDelete.size());

  // Constructing the new list of texture views that doesn't contain the texture views we're
  // deleting
  for (vtkSmartPointer<vtkWebGPUComputeTextureView> textureView : this->TextureViews)
  {
    if (textureViewsToDelete.find(textureView) == textureViewsToDelete.end())
    {
      // The texture view isn't in the list of texture views that need to be deleted so we're adding
      // it to the list of texture views that are going to be kept
      updatedTextureViews.push_back(textureView);
    }
  }

  // Deleting all the binding entries that were using the texture views we deleted
  for (vtkSmartPointer<vtkWebGPUComputeTextureView> toDelete : textureViewsToDelete)
  {
    uint32_t binding = toDelete->GetBinding();
    uint32_t group = toDelete->GetGroup();

    auto find = this->ParentComputePass->Internals->BindGroupLayoutEntries.find(group);
    if (find != this->ParentComputePass->Internals->BindGroupLayoutEntries.end())
    {
      std::vector<wgpu::BindGroupLayoutEntry>& bglLayoutEntries = find->second;

      // Now removing the bind group layout entry that corresponded to the texture view
      std::remove_if(bglLayoutEntries.begin(), bglLayoutEntries.end(),
        [binding](wgpu::BindGroupLayoutEntry entry) { return entry.binding == binding; });
    }
  }

  // Finally, deleting the texture views from our bookkeeping
  for (vtkSmartPointer<vtkWebGPUComputeTextureView> toDelete : textureViewsToDelete)
  {
    this->TextureViewsToWebGPUTextureViews.erase(toDelete);
  }

  this->ComputeTextureToViews[texture] =
    std::unordered_set<vtkSmartPointer<vtkWebGPUComputeTextureView>>();
  this->TextureViews = updatedTextureViews;
  this->ParentComputePass->Internals->BindGroupOrLayoutsInvalidated = true;
}

//------------------------------------------------------------------------------
void vtkWebGPUInternalsComputePassTextureStorage::RebindTextureView(
  int group, int binding, int textureViewIndex)
{
  if (!this->CheckTextureViewIndex(textureViewIndex, "RebindTextureView"))
  {
    return;
  }

  vtkSmartPointer<vtkWebGPUComputeTextureView> computeTextureView;
  vtkSmartPointer<vtkWebGPUComputeTexture> computeTexture;
  wgpu::TextureView wgpuTextureView;
  computeTextureView = this->TextureViews[textureViewIndex];
  computeTexture = this->Textures[computeTextureView->GetAssociatedTextureIndex()];
  wgpuTextureView = this->TextureViewsToWebGPUTextureViews[computeTextureView];

  std::vector<wgpu::BindGroupEntry>& bgEntries =
    this->ParentComputePass->Internals->BindGroupEntries[group];
  std::vector<wgpu::BindGroupLayoutEntry>& bglEntries =
    this->ParentComputePass->Internals->BindGroupLayoutEntries[group];

  bool found = false;
  // Recreating the bind group layout. We need to find the existing bind group layout entry for
  // this group / binding to replace it with the new bgl entry
  for (wgpu::BindGroupLayoutEntry& bglEntry : bglEntries)
  {
    if (bglEntry.binding == binding)
    {
      bglEntry = this->ParentComputePass->Internals->CreateBindGroupLayoutEntry(
        binding, computeTexture, computeTextureView);
      found = true;
    }
  }

  // Recreating the bind group by finding it first as above for the bgl entry
  for (wgpu::BindGroupEntry& bgEntry : bgEntries)
  {
    if (bgEntry.binding == binding)
    {
      bgEntry = this->ParentComputePass->Internals->CreateBindGroupEntry(binding, wgpuTextureView);
      found = true;
    }
  }

  if (found)
  {

    this->ParentComputePass->Internals->BindGroupOrLayoutsInvalidated = true;

    return;
  }

  // If we're here, this means that we couldn't find the bind group entry that correspond to the
  // group / binding combination. This means that the texture view wasn't bound by AddTextureView
  // (because the user didn't give a proper group / binding combination at the time) so we're
  // binding it here.
  vtkSmartPointer<vtkWebGPUComputeTextureView> textureView;
  vtkSmartPointer<vtkWebGPUComputeTexture> texture;
  textureView = this->TextureViews[textureViewIndex];
  texture = this->Textures[textureView->GetAssociatedTextureIndex()];

  wgpu::BindGroupLayoutEntry bglEntry;
  wgpu::BindGroupEntry bgEntry;

  bglEntry =
    this->ParentComputePass->Internals->CreateBindGroupLayoutEntry(binding, texture, textureView);
  bgEntry = this->ParentComputePass->Internals->CreateBindGroupEntry(binding, wgpuTextureView);

  this->ParentComputePass->Internals->BindGroupLayoutEntries[group].push_back(bglEntry);
  this->ParentComputePass->Internals->BindGroupEntries[group].push_back(bgEntry);

  this->ParentComputePass->Internals->BindGroupOrLayoutsInvalidated = true;
}

//------------------------------------------------------------------------------
void vtkWebGPUInternalsComputePassTextureStorage::ReadTextureFromGPU(int textureIndex, int mipLevel,
  vtkWebGPUInternalsComputePassTextureStorage::TextureMapAsyncCallback callback, void* userdata)
{
  if (!CheckTextureIndex(textureIndex, "ReadTextureFromGPU"))
  {
    return;
  }

  vtkSmartPointer<vtkWebGPUComputeTexture> texture = this->Textures[textureIndex];
  wgpu::Texture wgpuTexture = this->WebGPUTextures[textureIndex];

  // Bytes needs to be a multiple of 256
  vtkIdType bytesPerRow =
    std::ceil(wgpuTexture.GetWidth() * texture->GetBytesPerPixel() / 256.0f) * 256.0f;

  // Creating the buffer that will hold the data of the texture
  wgpu::BufferDescriptor bufferDescriptor;
  bufferDescriptor.label = "Buffer descriptor for mapping texture";
  bufferDescriptor.mappedAtCreation = false;
  bufferDescriptor.nextInChain = nullptr;
  bufferDescriptor.size = bytesPerRow * texture->GetHeight();
  bufferDescriptor.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead;

  wgpu::Buffer buffer = this->ParentPassDevice.CreateBuffer(&bufferDescriptor);

  // Parameters for copying the texture
  wgpu::ImageCopyTexture imageCopyTexture;
  imageCopyTexture.mipLevel = mipLevel;
  imageCopyTexture.origin = { 0, 0, 0 };
  imageCopyTexture.texture = wgpuTexture;

  // Parameters for copying the buffer
  unsigned int mipLevelWidth = std::floor(texture->GetWidth() / std::pow(2, mipLevel));
  unsigned int mipLevelHeight = std::floor(texture->GetHeight() / std::pow(2, mipLevel));
  wgpu::ImageCopyBuffer imageCopyBuffer;
  imageCopyBuffer.buffer = buffer;
  imageCopyBuffer.layout.nextInChain = nullptr;
  imageCopyBuffer.layout.offset = 0;
  imageCopyBuffer.layout.rowsPerImage = mipLevelHeight;
  imageCopyBuffer.layout.bytesPerRow = bytesPerRow;

  // Copying the texture to the buffer
  wgpu::CommandEncoder commandEncoder = this->ParentComputePass->Internals->CreateCommandEncoder();
  wgpu::Extent3D copySize = { mipLevelWidth, mipLevelHeight, texture->GetDepth() };
  commandEncoder.CopyTextureToBuffer(&imageCopyTexture, &imageCopyBuffer, &copySize);

  // Submitting the comand
  wgpu::CommandBuffer commandBuffer = commandEncoder.Finish();
  this->ParentPassDevice.GetQueue().Submit(1, &commandBuffer);

  auto bufferMapCallback = [](WGPUBufferMapAsyncStatus status, void* userdata) {
    InternalMapTextureAsyncData* mapData = reinterpret_cast<InternalMapTextureAsyncData*>(userdata);

    if (status == WGPUBufferMapAsyncStatus_Success)
    {
      const void* mappedRange = mapData->buffer.GetConstMappedRange(0, mapData->byteSize);
      mapData->userCallback(mappedRange, mapData->bytesPerRow, mapData->userdata);

      mapData->buffer.Unmap();
      // Freeing the callbackData structure as it was dynamically allocated
      delete mapData;
    }
    else
    {
      vtkLogF(WARNING, "Could not map texture '%s' with error status: %d",
        mapData->bufferLabel.empty() ? "(nolabel)" : mapData->bufferLabel.c_str(), status);

      // Freeing the callbackData structure as it was dynamically allocated
      delete mapData;
    }
  };

  // Now mapping the buffer that contains the texture data to the CPU
  // Dynamically allocating here because we callbackData to stay alive even after exiting this
  // function (because buffer.MapAsync is asynchronous). buffer.MapAsync() also takes a raw pointer
  // so we cannot use smart pointers here
  InternalMapTextureAsyncData* callbackData = new InternalMapTextureAsyncData;
  callbackData->buffer = buffer;
  callbackData->bufferLabel = "ReadTextureFromGPU map buffer";
  callbackData->byteSize = bufferDescriptor.size;
  callbackData->bytesPerRow = bytesPerRow;
  callbackData->userCallback = callback;
  callbackData->userdata = userdata;

  buffer.MapAsync(wgpu::MapMode::Read, 0, bufferDescriptor.size, bufferMapCallback, callbackData);
}

//------------------------------------------------------------------------------
wgpu::TextureFormat vtkWebGPUInternalsComputePassTextureStorage::ComputeTextureFormatToWebGPU(
  vtkWebGPUComputeTexture::TextureFormat format)
{
  switch (format)
  {
    case vtkWebGPUComputeTexture::TextureFormat::RGBA8_UNORM:
      return wgpu::TextureFormat::RGBA8Unorm;

    case vtkWebGPUComputeTexture::TextureFormat::R32_FLOAT:
      return wgpu::TextureFormat::R32Float;

    default:
      vtkLog(ERROR, "Unhandled texture format in ComputeTextureFormatToWebGPU: " << format);
      return wgpu::TextureFormat::Undefined;
  }
}

//------------------------------------------------------------------------------
wgpu::TextureDimension vtkWebGPUInternalsComputePassTextureStorage::ComputeTextureDimensionToWebGPU(
  vtkWebGPUComputeTexture::TextureDimension dimension)
{
  switch (dimension)
  {
    case vtkWebGPUComputeTexture::TextureDimension::DIMENSION_1D:
      return wgpu::TextureDimension::e1D;

    case vtkWebGPUComputeTexture::TextureDimension::DIMENSION_2D:
      return wgpu::TextureDimension::e2D;

    case vtkWebGPUComputeTexture::TextureDimension::DIMENSION_3D:
      return wgpu::TextureDimension::e3D;

    default:
      vtkLog(ERROR,
        "Unhandled texture dimension in ComputeTextureDimensionToWebGPU: "
          << dimension << ". Assuming DIMENSION_2D.");
      return wgpu::TextureDimension::e2D;
  }
}

//------------------------------------------------------------------------------
wgpu::TextureViewDimension
vtkWebGPUInternalsComputePassTextureStorage::ComputeTextureDimensionToViewDimension(
  vtkWebGPUComputeTexture::TextureDimension dimension)
{
  switch (dimension)
  {
    case vtkWebGPUComputeTexture::TextureDimension::DIMENSION_1D:
      return wgpu::TextureViewDimension::e1D;

    case vtkWebGPUComputeTexture::TextureDimension::DIMENSION_2D:
      return wgpu::TextureViewDimension::e2D;

    case vtkWebGPUComputeTexture::TextureDimension::DIMENSION_3D:
      return wgpu::TextureViewDimension::e3D;

    default:
      vtkLog(ERROR,
        "Unhandled texture view dimension in ComputeTextureDimensionToViewDimension: "
          << dimension << ". Assuming DIMENSION_2D.");
      return wgpu::TextureViewDimension::e2D;
  }
}

//------------------------------------------------------------------------------
wgpu::TextureUsage vtkWebGPUInternalsComputePassTextureStorage::ComputeTextureModeToUsage(
  vtkWebGPUComputeTexture::TextureMode mode, const std::string& textureLabel)
{
  switch (mode)
  {
    case vtkWebGPUComputeTexture::TextureMode::READ_ONLY:
      return wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst;

    case vtkWebGPUComputeTexture::TextureMode::WRITE_ONLY_STORAGE:
      return wgpu::TextureUsage::StorageBinding | wgpu::TextureUsage::CopySrc;

    case vtkWebGPUComputeTexture::READ_WRITE_STORAGE:
      return wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::StorageBinding |
        wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::CopyDst;

    default:
      vtkLog(ERROR,
        "Compute texture \"" << textureLabel
                             << "\" has undefined mode. Did you forget to call "
                                "vtkWebGPUComputeTexture::SetMode()?");

      return wgpu::TextureUsage::None;
  }
}

//------------------------------------------------------------------------------
wgpu::StorageTextureAccess
vtkWebGPUInternalsComputePassTextureStorage::ComputeTextureModeToShaderStorage(
  vtkWebGPUComputeTexture::TextureMode mode, const std::string& textureLabel)
{
  switch (mode)
  {
    case vtkWebGPUComputeTexture::TextureMode::READ_ONLY:
      return wgpu::StorageTextureAccess::ReadOnly;

    case vtkWebGPUComputeTexture::TextureMode::WRITE_ONLY_STORAGE:
      return wgpu::StorageTextureAccess::WriteOnly;

    case vtkWebGPUComputeTexture::READ_WRITE_STORAGE:
      return wgpu::StorageTextureAccess::ReadWrite;

    default:
      vtkLog(ERROR,
        "Compute texture \"" << textureLabel
                             << "\" has undefined mode. Did you forget to call "
                                "vtkWebGPUComputeTexture::SetMode()?");

      return wgpu::StorageTextureAccess::Undefined;
  }
}

//------------------------------------------------------------------------------
wgpu::StorageTextureAccess
vtkWebGPUInternalsComputePassTextureStorage::ComputeTextureViewModeToShaderStorage(
  vtkWebGPUComputeTextureView::TextureViewMode mode, const std::string& textureViewLabel)
{
  switch (mode)
  {
    case vtkWebGPUComputeTextureView::TextureViewMode::READ_ONLY:
      return wgpu::StorageTextureAccess::ReadOnly;

    case vtkWebGPUComputeTextureView::TextureViewMode::WRITE_ONLY_STORAGE:
      return wgpu::StorageTextureAccess::WriteOnly;

    case vtkWebGPUComputeTextureView::TextureViewMode::READ_WRITE_STORAGE:
      return wgpu::StorageTextureAccess::ReadWrite;

    default:
      vtkLog(ERROR,
        "Compute texture view \"" << textureViewLabel
                                  << "\" has undefined mode. Did you forget to call "
                                     "vtkWebGPUComputeTextureView::SetMode()?");

      return wgpu::StorageTextureAccess::Undefined;
  }
}

//------------------------------------------------------------------------------
wgpu::TextureSampleType
vtkWebGPUInternalsComputePassTextureStorage::ComputeTextureSampleTypeToWebGPU(
  vtkWebGPUComputeTexture::TextureSampleType sampleType)
{
  switch (sampleType)
  {
    case vtkWebGPUComputeTexture::TextureSampleType::FLOAT:
      return wgpu::TextureSampleType::Float;

    case vtkWebGPUComputeTexture::TextureSampleType::UNFILTERABLE_FLOAT:
      return wgpu::TextureSampleType::UnfilterableFloat;

    case vtkWebGPUComputeTexture::TextureSampleType::DEPTH:
      return wgpu::TextureSampleType::Depth;

    case vtkWebGPUComputeTexture::TextureSampleType::SIGNED_INT:
      return wgpu::TextureSampleType::Sint;

    case vtkWebGPUComputeTexture::TextureSampleType::UNSIGNED_INT:
      return wgpu::TextureSampleType::Uint;

    default:
      vtkLog(
        ERROR, "Unhandled texture sampleType in ComputeTextureSampleTypeToWebGPU: " << sampleType);
      return wgpu::TextureSampleType::Undefined;
  }
}

//------------------------------------------------------------------------------
wgpu::TextureAspect vtkWebGPUInternalsComputePassTextureStorage::ComputeTextureViewAspectToWebGPU(
  vtkWebGPUComputeTextureView::TextureViewAspect aspect)
{
  switch (aspect)
  {
    case vtkWebGPUComputeTextureView::TextureViewAspect::ASPECT_ALL:
      return wgpu::TextureAspect::All;

    case vtkWebGPUComputeTextureView::TextureViewAspect::ASPECT_DEPTH:
      return wgpu::TextureAspect::DepthOnly;

    case vtkWebGPUComputeTextureView::TextureViewAspect::ASPECT_STENCIL:
      return wgpu::TextureAspect::StencilOnly;

    default:
      vtkLog(ERROR,
        "Unhandled texture view aspect in ComputeTextureViewAspectToWebGPU: "
          << aspect << ". Assuming ASPECT_ALL.");
      return wgpu::TextureAspect::All;
  }
}

VTK_ABI_NAMESPACE_END
