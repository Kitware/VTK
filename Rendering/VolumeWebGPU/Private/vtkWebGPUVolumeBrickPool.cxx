// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "Private/vtkWebGPUVolumeBrickPool.h"

#include "Private/vtkWebGPUHelpersPrivate.h"

#include "vtkWebGPUConfiguration.h"
#include "vtkWebGPURenderWindow.h"

#include <algorithm>
#include <cmath>

VTK_ABI_NAMESPACE_BEGIN

namespace
{
constexpr std::uint32_t MaximumTextureDimension = 2048;

using Kind = vtkWebGPUVolumeBrickPool::Kind;

WGPUTextureFormat FormatOf(Kind kind)
{
  switch (kind)
  {
    case Kind::Byte:
      return WGPUTextureFormat_R8Unorm;
    case Kind::ByteRGBA:
      return WGPUTextureFormat_RGBA8Unorm;
    case Kind::FloatRGBA:
      return WGPUTextureFormat_RGBA32Float;
    default:
      return WGPUTextureFormat_R32Float;
  }
}

std::uint32_t BytesPerVoxel(Kind kind)
{
  switch (kind)
  {
    case Kind::Byte:
      return 1u;
    case Kind::ByteRGBA:
      return 4u;
    case Kind::FloatRGBA:
      return 16u;
    default:
      return 4u;
  }
}

const char* LabelOf(Kind kind)
{
  switch (kind)
  {
    case Kind::Byte:
      return "VolumeBrickPoolByte";
    case Kind::ByteRGBA:
      return "VolumeBrickPoolByteRGBA";
    case Kind::FloatRGBA:
      return "VolumeBrickPoolFloatRGBA";
    default:
      return "VolumeBrickPoolFloat";
  }
}
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeBrickPool::Configure(vtkWebGPURenderWindow* window, int brickSize,
  std::size_t budgetBytes, std::size_t wantedSlots, std::uint32_t bytesPerVoxel)
{
  brickSize = std::clamp(brickSize, 4, 128);
  const std::uint32_t slotSize = static_cast<std::uint32_t>(brickSize) + 2;
  const std::uint32_t perAxis = std::max(1u, MaximumTextureDimension / slotSize);

  const std::size_t slotBytes =
    static_cast<std::size_t>(slotSize) * slotSize * slotSize * std::max(bytesPerVoxel, 1u);
  const std::size_t affordable =
    std::max<std::size_t>(budgetBytes / std::max<std::size_t>(slotBytes, 1), 1);

  if (this->Window == window && this->BrickSize == brickSize &&
    this->Capacity >= std::min<std::size_t>(wantedSlots, affordable))
  {
    return;
  }
  // Growing costs everything that was resident, so grow in steps rather than a brick at a time.
  const std::size_t wanted = std::min(
    affordable, std::max<std::size_t>(wantedSlots, static_cast<std::size_t>(this->Capacity) * 2));

  // Fill x, then y, then z, so that a budget of a handful of bricks lays them out in a row
  // instead of rounding a cube up to far more slots than were asked for.
  const auto divide = [](std::size_t numerator, std::size_t denominator)
  { return (numerator + denominator - 1) / denominator; };
  std::uint32_t grid[3];
  grid[0] = static_cast<std::uint32_t>(std::min<std::size_t>(perAxis, wanted));
  grid[1] = static_cast<std::uint32_t>(std::min<std::size_t>(perAxis, divide(wanted, grid[0])));
  grid[2] = static_cast<std::uint32_t>(
    std::min<std::size_t>(perAxis, divide(wanted, static_cast<std::size_t>(grid[0]) * grid[1])));

  if (this->Window == window && this->BrickSize == brickSize && this->SlotGrid[0] == grid[0] &&
    this->SlotGrid[1] == grid[1] && this->SlotGrid[2] == grid[2])
  {
    return;
  }

  this->ReleaseGraphicsResources();
  this->Window = window;
  this->BrickSize = brickSize;
  std::copy(grid, grid + 3, this->SlotGrid);
  this->Capacity = grid[0] * grid[1] * grid[2];
  this->Slots.assign(this->Capacity, SlotState{});
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeBrickPool::BeginFrame()
{
  ++this->Frame;
}

//------------------------------------------------------------------------------
std::uint32_t vtkWebGPUVolumeBrickPool::Lookup(const Key& key)
{
  const auto found = this->Residency.find(key);
  if (found == this->Residency.end())
  {
    return InvalidSlot;
  }
  this->Slots[found->second].LastUsed = this->Frame;
  return found->second;
}

//------------------------------------------------------------------------------
std::uint32_t vtkWebGPUVolumeBrickPool::Allocate(const Key& key)
{
  if (const std::uint32_t resident = this->Lookup(key); resident != InvalidSlot)
  {
    return resident;
  }
  if (this->Capacity == 0)
  {
    return InvalidSlot;
  }

  std::uint32_t victim = InvalidSlot;
  std::uint64_t oldest = this->Frame;
  for (std::uint32_t i = 0; i < this->Capacity; ++i)
  {
    const std::uint32_t candidate = (this->NextVictim + i) % this->Capacity;
    SlotState& slot = this->Slots[candidate];
    if (!slot.Occupied)
    {
      victim = candidate;
      break;
    }
    // Anything this frame has already looked at is being drawn and cannot be taken.
    if (slot.LastUsed < oldest)
    {
      oldest = slot.LastUsed;
      victim = candidate;
    }
  }
  if (victim == InvalidSlot)
  {
    return InvalidSlot;
  }

  SlotState& slot = this->Slots[victim];
  if (slot.Occupied)
  {
    this->Residency.erase(slot.Owner);
  }
  slot.Owner = key;
  slot.Occupied = true;
  slot.LastUsed = this->Frame;
  this->Residency[key] = victim;
  this->NextVictim = (victim + 1) % this->Capacity;
  return victim;
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeBrickPool::SlotOrigin(std::uint32_t slot, std::uint32_t origin[3]) const
{
  const std::uint32_t slotSize = static_cast<std::uint32_t>(this->GetSlotSize());
  origin[0] = (slot % this->SlotGrid[0]) * slotSize;
  origin[1] = ((slot / this->SlotGrid[0]) % this->SlotGrid[1]) * slotSize;
  origin[2] = (slot / (this->SlotGrid[0] * this->SlotGrid[1])) * slotSize;
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeBrickPool::EnsureTexture(Kind kind)
{
  const int index = static_cast<int>(kind);
  if (this->Textures[index] != nullptr || this->Window == nullptr || this->Capacity == 0)
  {
    return;
  }
  auto configuration = this->Window->GetWGPUConfiguration();
  const std::uint32_t slotSize = static_cast<std::uint32_t>(this->GetSlotSize());

  WGPUTextureDescriptor descriptor = WGPU_TEXTURE_DESCRIPTOR_INIT;
  descriptor.label = WGPUStringView{ LabelOf(kind), WGPU_STRLEN };
  descriptor.dimension = WGPUTextureDimension_3D;
  descriptor.size = { this->SlotGrid[0] * slotSize, this->SlotGrid[1] * slotSize,
    this->SlotGrid[2] * slotSize };
  descriptor.format = FormatOf(kind);
  descriptor.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;
  descriptor.mipLevelCount = 1;
  descriptor.sampleCount = 1;
  this->Textures[index] = vtkWebGPU::Texture::Acquire(configuration->CreateTexture(descriptor));

  WGPUTextureViewDescriptor viewDescriptor = WGPU_TEXTURE_VIEW_DESCRIPTOR_INIT;
  viewDescriptor.label = descriptor.label;
  viewDescriptor.format = descriptor.format;
  viewDescriptor.dimension = WGPUTextureViewDimension_3D;
  viewDescriptor.baseMipLevel = 0;
  viewDescriptor.mipLevelCount = 1;
  viewDescriptor.baseArrayLayer = 0;
  viewDescriptor.arrayLayerCount = 1;
  this->Views[index] = vtkWebGPU::TextureView::Acquire(
    configuration->CreateView(this->Textures[index], viewDescriptor));
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeBrickPool::Upload(Kind kind, std::uint32_t slot, const void* voxels)
{
  this->EnsureTexture(kind);
  const int index = static_cast<int>(kind);
  if (this->Textures[index] == nullptr || slot >= this->Capacity)
  {
    return;
  }

  std::uint32_t origin[3];
  this->SlotOrigin(slot, origin);
  const std::uint32_t slotSize = static_cast<std::uint32_t>(this->GetSlotSize());
  const std::uint32_t bytesPerVoxel = BytesPerVoxel(kind);

  WGPUTexelCopyTextureInfo destination = {};
  destination.texture = this->Textures[index];
  destination.mipLevel = 0;
  destination.origin = { origin[0], origin[1], origin[2] };
  destination.aspect = WGPUTextureAspect_All;

  WGPUTexelCopyBufferLayout layout = {};
  layout.offset = 0;
  layout.bytesPerRow = slotSize * bytesPerVoxel;
  layout.rowsPerImage = slotSize;

  const WGPUExtent3D extent = { slotSize, slotSize, slotSize };
  const std::size_t bytes =
    static_cast<std::size_t>(slotSize) * slotSize * slotSize * bytesPerVoxel;

  WGPUQueue queue = wgpuDeviceGetQueue(this->Window->GetDevice());
  wgpuQueueWriteTexture(queue, &destination, voxels, bytes, &layout, &extent);
  wgpuQueueRelease(queue);
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeBrickPool::Release(const Key& key)
{
  const auto found = this->Residency.find(key);
  if (found != this->Residency.end())
  {
    this->Slots[found->second] = SlotState{};
    this->Residency.erase(found);
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeBrickPool::Forget(const void* owner)
{
  for (auto it = this->Residency.begin(); it != this->Residency.end();)
  {
    if (it->first.Owner == owner)
    {
      this->Slots[it->second] = SlotState{};
      it = this->Residency.erase(it);
    }
    else
    {
      ++it;
    }
  }
}

//------------------------------------------------------------------------------
WGPUTextureView vtkWebGPUVolumeBrickPool::GetView(Kind kind)
{
  const int index = static_cast<int>(kind);
  if (this->Views[index] != nullptr)
  {
    return this->Views[index];
  }

  // A bind group cannot leave a texture binding empty, so a pool that no volume in this pass
  // uses still needs something the right shape bound to it.
  if (this->PlaceholderViews[index] == nullptr && this->Window != nullptr)
  {
    auto configuration = this->Window->GetWGPUConfiguration();

    WGPUTextureDescriptor descriptor = WGPU_TEXTURE_DESCRIPTOR_INIT;
    descriptor.label = WGPUStringView{ "VolumeBrickPoolPlaceholder", WGPU_STRLEN };
    descriptor.dimension = WGPUTextureDimension_3D;
    descriptor.size = { 1, 1, 1 };
    descriptor.format = FormatOf(kind);
    descriptor.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;
    descriptor.mipLevelCount = 1;
    descriptor.sampleCount = 1;
    this->Placeholders[index] =
      vtkWebGPU::Texture::Acquire(configuration->CreateTexture(descriptor));

    WGPUTextureViewDescriptor viewDescriptor = WGPU_TEXTURE_VIEW_DESCRIPTOR_INIT;
    viewDescriptor.label = descriptor.label;
    viewDescriptor.format = descriptor.format;
    viewDescriptor.dimension = WGPUTextureViewDimension_3D;
    viewDescriptor.baseMipLevel = 0;
    viewDescriptor.mipLevelCount = 1;
    viewDescriptor.baseArrayLayer = 0;
    viewDescriptor.arrayLayerCount = 1;
    this->PlaceholderViews[index] = vtkWebGPU::TextureView::Acquire(
      configuration->CreateView(this->Placeholders[index], viewDescriptor));
  }
  return this->PlaceholderViews[index];
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeBrickPool::ReleaseGraphicsResources()
{
  this->Residency.clear();
  this->Slots.clear();
  this->Capacity = 0;
  this->SlotGrid[0] = this->SlotGrid[1] = this->SlotGrid[2] = 0;
  this->BrickSize = 0;
  this->NextVictim = 0;
  this->Window = nullptr;
  for (int i = 0; i < KindCount; ++i)
  {
    this->Textures[i] = nullptr;
    this->Views[i] = nullptr;
    this->Placeholders[i] = nullptr;
    this->PlaceholderViews[i] = nullptr;
  }
}

VTK_ABI_NAMESPACE_END
