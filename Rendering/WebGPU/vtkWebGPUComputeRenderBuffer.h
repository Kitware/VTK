// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkWebGPUComputeRenderBuffer_h
#define vtkWebGPUComputeRenderBuffer_h

#include "vtkRenderingWebGPUModule.h" // For export macro
#include "vtkWebGPUComputeBuffer.h"
#include "vtkWebGPUPolyDataMapper.h" // for the point/cell attributes

VTK_ABI_NAMESPACE_BEGIN

class vtkWebGPURenderer;

/**
 * Render buffers are returned by calls to
 * vtkWebGPUPolyDataMapper::AcquirePointAttributeComputeRenderBuffer() (or CellAttribute equivalent)
 * and represent a buffer that is used by the rendering pipeline and that can also be added to a
 * compute pipeline
 */
class VTKRENDERINGWEBGPU_EXPORT vtkWebGPUComputeRenderBuffer : public vtkWebGPUComputeBuffer
{
public:
  vtkTypeMacro(vtkWebGPUComputeRenderBuffer, vtkWebGPUComputeBuffer);
  static vtkWebGPUComputeRenderBuffer* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkWebGPUComputeRenderBuffer();
  ~vtkWebGPUComputeRenderBuffer() override;

private:
  friend class vtkWebGPUComputePass;
  friend class vtkWebGPUComputePassBufferStorageInternals;
  friend class vtkWebGPUPolyDataMapper;
  friend class vtkWebGPURenderer;

  vtkWebGPUComputeRenderBuffer(const vtkWebGPUComputeRenderBuffer&) = delete;
  void operator=(const vtkWebGPUComputeRenderBuffer&) = delete;

  ///@{
  /**
   * Get/set the WebGPU buffer (used when this ComputeBuffer points to an already existing device
   * buffer)
   */
  void SetWebGPUBuffer(wgpu::Buffer buffer) { this->wgpuBuffer = buffer; };
  wgpu::Buffer GetWebGPUBuffer() { return this->wgpuBuffer; };
  ///@}

  ///@{
  /**
   * Get/set the attribute represented by the buffer in case the buffer
   */
  vtkGetMacro(PointBufferAttribute, vtkWebGPUPolyDataMapper::PointDataAttributes);
  vtkSetMacro(PointBufferAttribute, vtkWebGPUPolyDataMapper::PointDataAttributes);
  ///@}

  ///@{
  /**
   * Get/set the attribute represented by the buffer in case the buffer
   */
  vtkGetMacro(CellBufferAttribute, vtkWebGPUPolyDataMapper::CellDataAttributes);
  vtkSetMacro(CellBufferAttribute, vtkWebGPUPolyDataMapper::CellDataAttributes);
  ///@}

  ///@{
  /**
   * Get/set the binding of the offset and size uniform buffer automatically bound by the
   * ComputePipeline
   */
  vtkGetMacro(RenderUniformsBinding, uint32_t);
  vtkSetMacro(RenderUniformsBinding, uint32_t);
  ///@}

  ///@{
  /**
   * Get/set the group index of the offset and size uniform buffer automatically bound by the
   * ComputePipeline
   */
  vtkGetMacro(RenderUniformsGroup, uint32_t);
  vtkSetMacro(RenderUniformsGroup, uint32_t);
  ///@}

  ///@{
  /**
   * Get/set the offset (in sizeof(float) units) of the desired attribute (colors, normals, ...)
   * within the whole point / cell data buffer
   */
  vtkGetMacro(RenderBufferOffset, uint32_t);
  vtkSetMacro(RenderBufferOffset, uint32_t);
  ///@}

  ///@{
  /**
   * Get/set the number of element of the desired attribute (colors, normals, ...) within the whole
   * point / cell data buffer
   */
  vtkGetMacro(RenderBufferElementCount, uint32_t);
  vtkSetMacro(RenderBufferElementCount, uint32_t);
  ///@}

  ///@{
  /**
   * Get/set the associated compute pass
   */
  vtkGetMacro(AssociatedComputePass, vtkWebGPUComputePass*);
  vtkSetMacro(AssociatedComputePass, vtkWebGPUComputePass*);
  ///@}
  ///@{

  // We may want vtkWebGPUComputePipeline::AddBuffer() not to create a new device buffer for this
  // vtkWebGPUComputeBuffer but rather use an existing one that has been created elsewhere (by a
  // webGPUPolyDataMapper for example). This is the attribute that points to this 'already existing'
  // buffer.
  wgpu::Buffer wgpuBuffer = nullptr;

  // Attribute used when we're reusing an existing buffer (from the vtkWebGPUPolyDataMapper for
  // example). Can be either a cell attribute or a point attribute but not both at the same time
  vtkWebGPUPolyDataMapper::PointDataAttributes PointBufferAttribute =
    vtkWebGPUPolyDataMapper::PointDataAttributes::POINT_UNDEFINED;
  vtkWebGPUPolyDataMapper::CellDataAttributes CellBufferAttribute =
    vtkWebGPUPolyDataMapper::CellDataAttributes::CELL_UNDEFINED;

  // Because ComputeRenderBuffers give access to the whole buffer of point / cell data, we need to
  // give the user the information on where in the buffer are the colors / normals / uvs / whatever
  // they requested. The RenderUniformsGroup and RenderUniformsBinding give the binding point of the
  // uniforms buffer that will contain these pieces of information
  uint32_t RenderUniformsGroup = -1;
  uint32_t RenderUniformsBinding = -1;

  // RenderBufferOffset in an offset in bytes for where the requested part of the buffer starts.
  // RenderBufferElementCount is the number of elements of interest available in the buffer starting
  // at RenderBufferOffset
  uint32_t RenderBufferOffset = -1;
  uint32_t RenderBufferElementCount = -1;

  // Pipeline this render buffer belongs to.
  // Weak pointer here because the render buffer will also store a pointer to its pipeline. If both
  // are shared pointers, we have a cyclic dependency.
  vtkWebGPUComputePass* AssociatedComputePass = nullptr;
};

VTK_ABI_NAMESPACE_END
#endif
