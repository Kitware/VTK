// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkWebGPUDeepFramebuffer
 * @brief Per-pixel fragment lists for order independent transparency.
 *
 * Holds the three storage buffers an A-buffer needs:
 *
 * - HeadPointers: one `atomic<u32>` per pixel, holding the index of the first
 *   fragment recorded for that pixel, or `InvalidIndex` when the pixel has none.
 * - FragmentNodes: a pool of fragment records. Each is
 *   `{ color: u32, depth: f32, cellId: u32, propId: u32, next: u32 }`, 20 bytes,
 *   where `next` chains to the previously recorded fragment for the same pixel.
 *   The ids are carried so picking still works through translucent layers: a
 *   translucent fragment is discarded after being recorded, so it never reaches
 *   the id attachment on its own.
 * - FragmentCounter: a single `atomic<u32>` used to hand out node indices.
 *
 * A fragment shader appends with `atomicAdd` on the counter followed by
 * `atomicExchange` on the pixel's head pointer, so the list for a pixel ends up
 * in reverse insertion order and must be sorted by depth when it is resolved.
 *
 * The node pool is sized for an average number of layers per pixel rather than a
 * worst case. Overflow is expected and must be handled by the shader: when the
 * counter runs past the pool, the fragment is dropped instead of written.
 *
 * Only `atomic<u32>` in storage is used, which is core WebGPU. No optional
 * device feature is required.
 */

#ifndef vtkWebGPUDeepFramebuffer_h
#define vtkWebGPUDeepFramebuffer_h

#include "vtkObject.h"

#include "vtkRenderingWebGPUModule.h" // for export macro
#include "vtkSmartPointer.h"          // for ivar
#include "vtk_wgpu.h"                 // for the WebGPU C API

#include <cstdint> // for fixed width types

VTK_ABI_NAMESPACE_BEGIN
class vtkWebGPUConfiguration;

class VTKRENDERINGWEBGPU_EXPORT vtkWebGPUDeepFramebuffer : public vtkObject
{
public:
  static vtkWebGPUDeepFramebuffer* New();
  vtkTypeMacro(vtkWebGPUDeepFramebuffer, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Average number of transparent layers the node pool is sized for. Raising it
   * costs memory proportional to the viewport; lowering it drops more fragments
   * in heavily layered regions.
   */
  vtkGetMacro(AverageLayersPerPixel, std::uint32_t);
  vtkSetClampMacro(AverageLayersPerPixel, std::uint32_t, 1, 64);

  /**
   * The configuration used to allocate the buffers. Must be set before Resize().
   */
  void SetWGPUConfiguration(vtkWebGPUConfiguration* config);

  /**
   * Allocate the buffers for a `width` x `height` viewport, reallocating only
   * when the size actually changed. Returns false if allocation failed, in which
   * case the buffers are left released and transparency must fall back.
   */
  bool Resize(int width, int height);

  /**
   * Reset the counter to zero and every head pointer to InvalidIndex. Must run
   * before each frame's fragment collection pass.
   */
  void Clear();

  /**
   * The buffers, for binding into render and compute passes. Null until a
   * successful Resize(). These are borrowed; the caller must not release them.
   */
  WGPUBuffer GetHeadPointersBuffer() { return this->HeadPointersBuffer; }
  WGPUBuffer GetFragmentNodesBuffer() { return this->FragmentNodesBuffer; }
  WGPUBuffer GetFragmentCounterBuffer() { return this->FragmentCounterBuffer; }

  /**
   * Geometry of the current allocation.
   */
  vtkGetMacro(Width, int);
  vtkGetMacro(Height, int);
  vtkGetMacro(FragmentNodeCapacity, std::uint32_t);

  /**
   * Release the buffers.
   */
  void ReleaseGraphicsResources();

protected:
  vtkWebGPUDeepFramebuffer();
  ~vtkWebGPUDeepFramebuffer() override;

private:
  /**
   * Size of one fragment record, in bytes. Must match the `FragmentNode` struct
   * declared by the WGSL that reads and writes these buffers.
   */
  static constexpr std::uint32_t FragmentNodeSize = 20;

  /**
   * Head pointer value meaning "no fragments recorded for this pixel".
   */
  static constexpr std::uint32_t InvalidIndex = 0xFFFFFFFFu;

  vtkWebGPUDeepFramebuffer(const vtkWebGPUDeepFramebuffer&) = delete;
  void operator=(const vtkWebGPUDeepFramebuffer&) = delete;

  vtkSmartPointer<vtkWebGPUConfiguration> WGPUConfiguration;

  WGPUBuffer HeadPointersBuffer = nullptr;
  WGPUBuffer FragmentNodesBuffer = nullptr;
  WGPUBuffer FragmentCounterBuffer = nullptr;

  int Width = 0;
  int Height = 0;
  std::uint32_t AverageLayersPerPixel = 8;
  std::uint32_t FragmentNodeCapacity = 0;
};
VTK_ABI_NAMESPACE_END

#endif
