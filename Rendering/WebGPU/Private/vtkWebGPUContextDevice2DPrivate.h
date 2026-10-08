// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file vtkWebGPUContextDevice2DPrivate.h
 * @brief Private implementation details and internal structures for vtkWebGPUContextDevice2D.
 */

#ifndef vtkWebGPUContextDevice2DPrivate_h
#define vtkWebGPUContextDevice2DPrivate_h

#include "vtkWebGPUContextDevice2D.h"
#include "vtkWebGPUHandle.h"
#include "vtkWebGPURenderWindow.h"
#include <unordered_map>
#include <webgpu/webgpu.h>

VTK_ABI_NAMESPACE_BEGIN

/**
 * @class vtkWebGPULinearAllocator
 * @brief Linear allocator for dynamic WebGPU buffers.
 */

class vtkWebGPULinearAllocator
{
public:
  vtkWebGPULinearAllocator(const std::string& label)
    : Label(label)
  {
  }

  ~vtkWebGPULinearAllocator()
  {
    if (this->CurrentBuffer)
      wgpuBufferRelease(this->CurrentBuffer);
    for (WGPUBuffer buf : this->StaleBuffers)
      wgpuBufferRelease(buf);
  }

  /**
   * @brief Resets the internal offset and releases all stale buffers.
   */
  void Reset()
  {
    this->CurrentOffset = 0;

    for (WGPUBuffer buf : this->StaleBuffers)
    {
      wgpuBufferRelease(buf);
    }
    this->StaleBuffers.clear();
  }

  /**
   * @brief Reserves a contiguous block of memory in the current WebGPU buffer.
   *
   * If the requested size exceeds the remaining capacity, the current buffer is marked
   * as stale, and a new buffer is allocated (doubling the capacity).
   *
   * @param config The WebGPU configuration used to create new buffers.
   * @param sizeInBytes The exact number of bytes needed.
   * @param usage The expected WebGPU usage flags for the buffer.
   * @return The starting byte offset within the buffer returned by GetBuffer().
   */
  uint64_t Allocate(vtkWebGPUConfiguration* config, uint64_t sizeInBytes, WGPUBufferUsage usage)
  {
    uint64_t alignedSize = (sizeInBytes + 3) & ~3ull;

    if (!this->CurrentBuffer || this->CurrentOffset + alignedSize > this->Capacity)
    {
      if (this->CurrentBuffer)
      {
        this->StaleBuffers.push_back(this->CurrentBuffer);
      }

      this->Capacity =
        std::max({ this->Capacity * 2, alignedSize, static_cast<uint64_t>(4194304) });

      this->CurrentBuffer = config->CreateBuffer(
        this->Capacity, usage | WGPUBufferUsage_CopyDst, false, this->Label.c_str());

      this->CurrentOffset = 0;
    }

    uint64_t allocatedOffset = this->CurrentOffset;
    this->CurrentOffset += alignedSize;
    return allocatedOffset;
  }

  /**
   * @brief Gets the active WebGPU buffer object.
   * @return The current WGPUBuffer handling allocations.
   */
  WGPUBuffer GetBuffer() const { return this->CurrentBuffer; }

private:
  std::string Label;
  WGPUBuffer CurrentBuffer = nullptr;
  uint64_t Capacity = 0;
  uint64_t CurrentOffset = 0;
  std::vector<WGPUBuffer> StaleBuffers;
};

/**
 * @class vtkWebGPUContextDevice2D::Private
 * @brief Container for vtkWebGPUContextDevice2D internal state.
 */
class vtkWebGPUContextDevice2D::Private
{
public:
  vtkWebGPUConfiguration* Config = nullptr;
  vtkWebGPURenderPipelineCache* PipelineCache = nullptr;

  /**
   * @struct PipelineConfig
   * @brief Configuration parameters for creating or retrieving WebGPU render pipelines.
   */
  struct PipelineConfig
  {
    WGPUPrimitiveTopology topology;
    double haveColors;
    double haveTCoords;
    double haveLines;
    vtkWebGPU::RenderPipeline* outPipeline;
  };

  /**
   * @struct CachedTexture
   * @brief Holds WebGPU texture resources and metadata for caching images and text.
   */
  struct CachedTexture
  {
    WGPUTexture Texture = nullptr;
    WGPUTextureView View = nullptr;
    WGPUBindGroup BindGroup = nullptr;
    WGPUSampler Sampler = nullptr;
    vtkMTimeType ModifiedTime = 0;
    int Properties = 0;
    int Width = 0;
    int Height = 0;
  };

  /**
   * @struct CachedTextData
   * @brief Stores rasterized text images and their bounding box dimensions.
   */
  struct CachedTextData
  {
    vtkSmartPointer<vtkImageData> Image;
    int BBox[4];
  };

  WGPURenderPipeline TrianglePipeline = nullptr;
  WGPURenderPipeline LinePipeline = nullptr;
  WGPURenderPipeline TexturedPipeline = nullptr;

  vtkWebGPULinearAllocator VertexAllocator{ "Context2D Dynamic VBO" };
  WGPUBuffer ColorBuffer = nullptr;
  WGPUBuffer MatrixUniformBuffer = nullptr;
  WGPUBindGroup TransformBindGroup;
  WGPUPipelineLayout PipelineLayout = nullptr;
  WGPUVertexBufferLayout VertexBufferLayout;

  WGPUTexture DummyTexture = nullptr;
  WGPUTextureView DummyTextureView = nullptr;
  WGPUSampler DefaultSampler = nullptr;
  WGPUBindGroup ActiveTextureBindGroup = nullptr;
  WGPUBindGroupLayout BindGroupLayout = nullptr;

  std::unordered_map<vtkImageData*, CachedTexture> ImageCache;
  std::unordered_map<std::string, CachedTexture> TextCache;
  std::unordered_map<std::string, CachedTextData> TextImageCache;

  std::vector<float> BatchedVertices;
  WGPURenderPipeline CurrentBatchPipeline = nullptr;
  WGPUBindGroup CurrentBatchBindGroup = nullptr;

  bool ClippingEnabled = false;
  int ClipRect[4] = { 0, 0, 0, 0 };

  bool MatricesDirty = true;
};

VTK_ABI_NAMESPACE_END

#endif // vtkWebGPUContextDevice2DPrivate_h
