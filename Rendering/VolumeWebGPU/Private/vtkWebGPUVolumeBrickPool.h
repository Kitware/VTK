// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkWebGPUVolumeBrickPool
 * @brief   Fixed size 3D texture that holds the resident bricks of every volume in a pass
 *
 * Volumes are not uploaded whole. They are cut into bricks of BrickSize voxels a side, and the
 * bricks that the rays actually need are kept in a pool of uniform slots. A page table, built by
 * the coordinator, tells the shader which slot holds a given brick, so a volume larger than the
 * pool - or larger than memory - still renders: the pages that are not resident are the ones the
 * pool is asked to bring in next.
 *
 * Every slot carries a one voxel apron around its brick so that trilinear interpolation inside a
 * brick never needs a neighbouring slot. That is what makes the partitioning invisible: a ray
 * crossing from one brick to the next reads the same values it would have read from one large
 * texture.
 *
 * Slots are recycled least recently used first, and a slot touched during the frame being built
 * is never taken, so the frame either gets what it asked for or is told the pool is full.
 *
 * This is a private implementation header; it is not installed.
 */

#ifndef vtkWebGPUVolumeBrickPool_h
#define vtkWebGPUVolumeBrickPool_h

#include "Private/vtkWebGPUHandle.h" // for the reference counted handle wrappers

#include "vtkABINamespace.h"

#include "vtk_wgpu.h"

#include <cstdint>
#include <map>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

class vtkWebGPURenderWindow;

class vtkWebGPUVolumeBrickPool
{
public:
  /// A page whose slot is this is not resident and must not be sampled.
  static constexpr std::uint32_t InvalidSlot = 0xFFFFFFFFu;

  /// How a volume's scalars are stored. One pool texture per kind, created on first use. Volumes
  /// of more than one component take the four channel kinds, padded where they have fewer.
  enum class Kind
  {
    Byte = 0,
    Float = 1,
    ByteRGBA = 2,
    FloatRGBA = 3,
    /// Two bytes per voxel: whether it is hidden, by a ghost, blanking or a binary mask, and its
    /// label for a volume drawn through a label map. Shares slot numbering with the scalar pool.
    Mask = 4,
    Count = 5
  };

  /// Identifies one brick of one volume for as long as the volume keeps offering itself.
  struct Key
  {
    const void* Owner = nullptr;
    std::uint32_t Brick = 0;

    bool operator<(const Key& other) const
    {
      return this->Owner != other.Owner ? this->Owner < other.Owner : this->Brick < other.Brick;
    }
  };

  /**
   * Make room for \a wantedSlots bricks of \a brickSize voxels a side within \a budgetBytes.
   * \a bytesPerVoxel is the widest format the pass will store, so a scene of single component
   * volumes is not sized as four channel ones. The pool only grows, so a settled scene stops
   * rebuilding.
   */
  void Configure(vtkWebGPURenderWindow* window, int brickSize, std::size_t budgetBytes,
    std::size_t wantedSlots, std::uint32_t bytesPerVoxel);

  /// Start gathering a frame. Slots touched after this point are safe from eviction until the
  /// next call.
  void BeginFrame();

  /**
   * Look up a brick, or make room for one. Lookup touches the slot it finds so the frame being
   * built keeps it. Allocate returns InvalidSlot when every slot is spoken for by this frame,
   * which tells the caller to draw what it has.
   */
  std::uint32_t Lookup(const Key& key);
  std::uint32_t Allocate(const Key& key);
  ///@}

  /**
   * Fill a slot. \a voxels must hold GetSlotSize() cubed values, the brick and its apron in
   * x fastest order.
   */
  void Upload(Kind kind, std::uint32_t slot, const void* voxels);

  /// Give a slot back, for when the brick it was allocated for could not be fetched after all.
  void Release(const Key& key);

  /// Let go of every slot that \a owner holds.
  void Forget(const void* owner);

  void ReleaseGraphicsResources();

  ///@{
  /// Geometry of the pool, shared with the shader.
  int GetBrickSize() const { return this->BrickSize; }
  int GetSlotSize() const { return this->BrickSize + 2; }
  const std::uint32_t* GetSlotGrid() const { return this->SlotGrid; }
  std::uint32_t GetCapacity() const { return this->Capacity; }
  ///@}

  /// Views of the pool textures. A kind that no volume has used yet is a one voxel placeholder.
  WGPUTextureView GetView(Kind kind);

  /// Whether \a kind has been asked for since the pool was configured.
  bool IsUsed(Kind kind) const { return this->Textures[static_cast<int>(kind)] != nullptr; }

private:
  void EnsureTexture(Kind kind);
  void SlotOrigin(std::uint32_t slot, std::uint32_t origin[3]) const;

  vtkWebGPURenderWindow* Window = nullptr;

  int BrickSize = 0;
  std::uint32_t SlotGrid[3] = { 0, 0, 0 };
  std::uint32_t Capacity = 0;

  struct SlotState
  {
    Key Owner;
    std::uint64_t LastUsed = 0;
    bool Occupied = false;
  };
  std::vector<SlotState> Slots;
  std::map<Key, std::uint32_t> Residency;
  std::uint64_t Frame = 0;
  /// Round robin starting point, so a full pool does not always evict the same slot first.
  std::uint32_t NextVictim = 0;

  static constexpr int KindCount = static_cast<int>(Kind::Count);
  vtkWebGPU::Texture Textures[KindCount];
  vtkWebGPU::TextureView Views[KindCount];
  vtkWebGPU::Texture Placeholders[KindCount];
  vtkWebGPU::TextureView PlaceholderViews[KindCount];
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkWebGPUVolumeBrickPool.h
