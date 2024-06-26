// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkWebGPUInternalsComputePipeline_h
#define vtkWebGPUInternalsComputePipeline_h

#include "vtkRenderingWebGPUModule.h"
#include "vtkWGPUContext.h"                    // for vtkWGPUContext
#include "vtkWebGPUComputeBuffer.h"            // for compute buffers
#include "vtkWebGPUComputeRenderBuffer.h"      // for compute render buffers
#include "vtkWebGPUInternalsBindGroup.h"       // for bind groups creation
#include "vtkWebGPUInternalsBindGroupLayout.h" // for bind group layouts creation
#include "vtkWebGPUInternalsCallbacks.h"       // for error callbacks

#include "vtk_wgpu.h" // for webgpu

#include <unordered_map>

VTK_ABI_NAMESPACE_BEGIN

class vtkWebGPUComputePipeline;

/**
 * Implementation details of the vtkWebGPUComputePipeline. Methods only used internally by the
 * compute pipeline which shouldn't be exposed to the user
 */
class VTKRENDERINGWEBGPU_NO_EXPORT vtkWebGPUInternalsComputePipeline
{
public:
  vtkWebGPUInternalsComputePipeline(vtkWebGPUComputePipeline* self)
    : Self(self)
  {
  }

  /**
   * Converts a compute buffer mode to its wgpu::BufferUsage equivalent
   */
  static wgpu::BufferUsage ComputeBufferModeToBufferUsage(vtkWebGPUComputeBuffer::BufferMode mode);

  /**
   * Converts a compute buffer mode to its wgpu::BufferBindingType equivalent
   */
  static wgpu::BufferBindingType ComputeBufferModeToBufferBindingType(
    vtkWebGPUComputeBuffer::BufferMode mode);

  /**
   * Given a buffer, create the associated bind group layout entry
   * that will be used when creating the bind group layouts
   */
  void AddBindGroupLayoutEntry(
    uint32_t bindGroup, uint32_t binding, vtkWebGPUComputeBuffer::BufferMode mode);

  /**
   * Given a buffer, create the associated bind group entry
   * that will be used when creating the bind groups
   */
  void AddBindGroupEntry(wgpu::Buffer wgpuBuffer, uint32_t bindGroup, uint32_t binding,
    vtkWebGPUComputeBuffer::BufferMode mode, uint32_t offset);

  /**
   * Initializes the adapter of the compute pipeline
   */
  void CreateAdapter();

  /**
   * Initializes the device of the compute pipeline
   */
  void CreateDevice();

  /**
   * Compiles the shader source given into a WGPU shader module
   */
  void CreateShaderModule();

  /**
   * Creates all the bind groups and bind group layouts of this compute pipeline from the buffers
   * that have been added so far.
   */
  void CreateBindGroupsAndLayouts();

  /**
   * Creates the bind group layout of a given list of buffers (that must all belong to the same
   * bind group)
   */
  wgpu::BindGroupLayout CreateBindGroupLayout(
    const wgpu::Device& device, const std::vector<wgpu::BindGroupLayoutEntry>& layoutEntries);

  /**
   * Creates the compute pipeline that will be used to dispatch the compute shader
   */
  void CreateComputePipeline();

  /**
   * Creates the compute pipeline layout associated with the bind group layouts of this compute
   * pipeline
   *
   * @warning: The bind group layouts must have been created by CreateBindGroups() prior to
   * calling this function
   */
  wgpu::PipelineLayout CreateComputePipelineLayout();

  /**
   * Creates and returns a command encoder
   */
  wgpu::CommandEncoder CreateCommandEncoder();

  /**
   * Creates a compute pass encoder from a command encoder
   */
  wgpu::ComputePassEncoder CreateComputePassEncoder(const wgpu::CommandEncoder& commandEncoder);

  /**
   * Encodes the compute pass and dispatches the workgroups
   *
   * @warning: The bind groups and the compute pipeline must have been created prior to calling
   * this function
   */
  void DispatchComputePass(unsigned int groupsX, unsigned int groupsY, unsigned int groupsZ);

  /**
   * Finishes the encoding of a command encoder and submits the resulting command buffer
   * to the queue
   */
  void SubmitCommandEncoderToQueue(const wgpu::CommandEncoder& commandEncoder);

  bool Initialized = false;

  wgpu::Adapter Adapter = nullptr;
  wgpu::Device Device = nullptr;
  wgpu::ShaderModule ShaderModule;

  // A list of the bind group index in which bind groups are stored in this->BindGroups. If
  // BindGroupsOrder[0] = 1, this means that this->BindGroups[0] correspond to the bind group of
  // index 1 (@group(1) in WGSL).
  // This list is going to be useful when we're going to want to update a bind group (after
  // resizing a buffer for example when we need to find which bind group in the list to recreate).
  std::vector<int> BindGroupsOrder;
  std::vector<wgpu::BindGroup> BindGroups;
  // Maps a bind group index to to the list of bind group entries for this group. These
  // entries will be used at the creation of the bind groups
  std::unordered_map<int, std::vector<wgpu::BindGroupEntry>> BindGroupEntries;

  std::vector<wgpu::BindGroupLayout> BindGroupLayouts;
  // Maps a bind group index to to the list of bind group layout entries for this group.
  // These layout entries will be used at the creation of the bind group layouts
  std::unordered_map<int, std::vector<wgpu::BindGroupLayoutEntry>> BindGroupLayoutEntries;

  wgpu::ComputePipeline ComputePipeline;

  std::vector<vtkSmartPointer<vtkWebGPUComputeBuffer>> Buffers;
  std::vector<wgpu::Buffer> WGPUBuffers;

  // Render buffers use already existing wgpu buffers (those of poly data mappers for example) and
  // thus need to be handled differently
  std::vector<vtkSmartPointer<vtkWebGPUComputeRenderBuffer>> RenderBuffers;

  // How many groups to launch when dispatching the compute
  unsigned int GroupsX = 0, GroupsY = 0, GroupsZ = 0;

  // Label used for the wgpu compute pipeline of this VTK compute pipeline
  std::string WGPUComputePipelineLabel = "WebGPU compute pipeline of \"VTK Compute pipeline\"";
  // Label used for the wgpu command encoders created and used by this VTK compute pipeline
  std::string WGPUCommandEncoderLabel = "WebGPU command encoder of \"VTK Compute pipeline\"";

  // Main class
  vtkWebGPUComputePipeline* Self = nullptr;
};

VTK_ABI_NAMESPACE_END

#endif
