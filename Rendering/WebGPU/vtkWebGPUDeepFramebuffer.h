// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkWebGPUDeepFramebuffer
 * @brief Per-pixel fragment lists for order independent transparency.
 *
 * Holds the three storage buffers an A-buffer needs:
 *
 * - HeadPointers: one `atomic<u32>` per pixel, holding the index of that pixel's
 *   first fragment, or `InvalidIndex` when it has none.
 * - FragmentNodes: a pool of 28 byte records,
 *   `{ color, depth, cellId, propId, compositeId, processId, next }`. All four
 *   id channels are carried because a translucent fragment is recorded with
 *   color attachment writes disabled and never reaches the id attachment on its own;
 *   vtkHardwareSelector treats a zero composite id as "nothing was drawn here".
 * - FragmentCounter: two `atomic<u32>` values, handing out node indices and
 *   recording resolve/traversal truncation flags.
 *
 * A shader appends with `atomicAdd` on the counter then `atomicExchange` on the
 * head pointer, so a list is in reverse insertion order and must be sorted by
 * depth when resolved. The pool is sized for an average number of layers per
 * pixel, so overflow is expected: past the end, fragments are dropped.
 *
 * Only `atomic<u32>` in storage is used, which is core WebGPU.
 */

#ifndef vtkWebGPUDeepFramebuffer_h
#define vtkWebGPUDeepFramebuffer_h

#include "vtkObject.h"

#include "vtkRenderingWebGPUModule.h" // for export macro
#include "vtkSmartPointer.h"          // for ivar
#include "vtk_wgpu.h"                 // for the WebGPU C API

#include <cstdint> // for fixed width types
#include <memory>  // for asynchronous readback state

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
  vtkGetMacro(AverageLayersPerPixel, vtkTypeUInt32);
  vtkSetClampMacro(AverageLayersPerPixel, vtkTypeUInt32, 1, 64);

  ///@{
  /** Maximum node-pool size in bytes. Zero (default) uses the device limit. */
  vtkSetMacro(MaximumMemoryInBytes, vtkTypeUInt64);
  vtkGetMacro(MaximumMemoryInBytes, vtkTypeUInt64);
  ///@}

  ///@{
  /** Grow after observed overflow, within the memory/device limit. On by default. */
  vtkSetMacro(AutoResize, bool);
  vtkGetMacro(AutoResize, bool);
  vtkBooleanMacro(AutoResize, bool);
  ///@}

  ///@{
  /** Frames between automatic samples after a complete result (default 8).
   * New allocations and incomplete results are sampled immediately.
   * Explicit ReadStatistics() calls bypass this interval.
   */
  vtkSetClampMacro(StatisticsInterval, int, 1, 64);
  vtkGetMacro(StatisticsInterval, int);
  ///@}

  /** Request asynchronous feedback at the configured sampling interval. */
  VTK_MAYSUSPEND void RequestStatistics();

  /** Completeness flags for the most recent statistics readback. */
  enum FrameStatus
  {
    Complete = 0,
    Pending = 1,
    FragmentPoolOverflow = 2,
    LayerLimitExceeded = 4,
    TraversalLimitExceeded = 8,
    ReadbackFailed = 16
  };

  /**
   * Read statistics after the frame's commands have been submitted. Without
   * wait, at most one map is in flight and Pending is returned until the current
   * frame's result is available. With wait, process events until it is available.
   * The returned flags describe dropped fragments, not color quantization.
   */
  VTK_MAYSUSPEND int ReadStatistics(bool wait = false);
  vtkGetMacro(LastFrameStatus, int);
  vtkGetMacro(LastFragmentCount, vtkTypeUInt32);
  vtkGetMacro(MaximumFragmentNodeCapacity, vtkTypeUInt32);

  /**
   * The configuration used to allocate the buffers. Must be set before Resize().
   */
  void SetWGPUConfiguration(vtkWebGPUConfiguration* config);

  /**
   * Allocate the buffers for a `width` x `height` viewport, reallocating only
   * when the dimensions or effective capacity changed. Returns false if the
   * requested allocation is unavailable, in which case transparency must fall back.
   */
  VTK_MAYSUSPEND bool Resize(int width, int height);

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
  vtkGetMacro(FragmentNodeCapacity, vtkTypeUInt32);
  vtkGetMacro(AllocationGeneration, vtkTypeUInt64);

  /**
   * Release the buffers.
   */
  VTK_MAYSUSPEND void ReleaseGraphicsResources();

protected:
  vtkWebGPUDeepFramebuffer();
  ~vtkWebGPUDeepFramebuffer() override;

private:
  /**
   * Size of one fragment record, in bytes. Must match the `FragmentNode` struct
   * declared by the WGSL that reads and writes these buffers.
   */
  static constexpr std::uint32_t FragmentNodeSize = 28;

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
  WGPUBuffer StatisticsReadbackBuffer = nullptr;

  int Width = 0;
  int Height = 0;
  std::uint32_t AverageLayersPerPixel = 8;
  std::uint32_t FragmentNodeCapacity = 0;
  vtkTypeUInt64 MaximumMemoryInBytes = 0;
  vtkTypeUInt64 AllocationGeneration = 0;
  vtkTypeUInt32 MaximumFragmentNodeCapacity = 0;
  bool AutoResize = true;
  int StatisticsInterval = 8;
  vtkTypeUInt32 AllocatedAverageLayers = 0;
  vtkTypeUInt64 AllocatedMemoryLimit = 0;
  vtkTypeUInt64 FrameNumber = 0;
  int LastFrameStatus = Pending;
  vtkTypeUInt32 LastFragmentCount = 0;
  int WarnedStatus = Complete;
  struct Readback;
  std::shared_ptr<Readback> Statistics;
  void UpdateStatistics();
};
VTK_ABI_NAMESPACE_END

#endif
