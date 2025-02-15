// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkWebGPUComputePassInternals_h
#define vtkWebGPUComputePassInternals_h

#include "Private/vtkWebGPUComputePassBufferStorageInternals.h"
#include "Private/vtkWebGPUComputePassTextureStorageInternals.h"
#include "vtkObject.h"
#include "vtkSmartPointer.h"
#include "vtkWebGPUConfiguration.h"
#include "vtk_wgpu.h" // for webgpu

#include <unordered_map>
#include <unordered_set>

class vtkWebGPUComputePipeline;

VTK_ABI_NAMESPACE_BEGIN

/**
 * Internals of the vtkWebGPUComputePass. This class manages the creation/deletion/recreation of
 * bind group and bind group layouts. This is also the class that maintains the state of a compute
 * pass (Texture and Buffer storages)
 */
class VTKRENDERINGWEBGPU_EXPORT vtkWebGPUComputePassInternals : public vtkObject
{
public:
  static vtkWebGPUComputePassInternals* New();
  vtkTypeMacro(vtkWebGPUComputePassInternals, vtkObject);

  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Sets the parent pass of this internals class
   */
  void SetParentPass(vtkWeakPointer<vtkWebGPUComputePass> parentPass);

  ///@{
  /**
   * Get/set the device used by this compute pass (usually the device of the compute pipeline
   * holding this compute pass)
   */
  void SetWGPUConfiguration(vtkWebGPUConfiguration* config);
  vtkGetSmartPointerMacro(WGPUConfiguration, vtkWebGPUConfiguration);
  ///@}

  ///@{
  /**
   * Get/set the compute pipeline to which this compute pass belongs to
   */
  vtkWeakPointer<vtkWebGPUComputePipeline> GetAssociatedPipeline();
  void SetAssociatedPipeline(vtkWeakPointer<vtkWebGPUComputePipeline> associatedPipeline);
  ///@}

  /**
   * Given a buffer, creates the associated bind group layout entry
   * that will be used when creating the bind group layouts and returns it
   */
  wgpu::BindGroupLayoutEntry CreateBindGroupLayoutEntry(
    uint32_t binding, vtkWebGPUComputeBuffer::BufferMode mode);

  /**
   * Given a texture and its view, creates the associated bind group layout entry and returns it
   */
  wgpu::BindGroupLayoutEntry CreateBindGroupLayoutEntry(uint32_t binding,
    vtkSmartPointer<vtkWebGPUComputeTexture> computeTexture,
    vtkSmartPointer<vtkWebGPUComputeTextureView> textureView);

  /**
   * Overload mainly used for creating the layout entry of render textures where we don't have a
   * vtkWebGPUComputeTextureView object and where the view is assumed to be very close in
   * configuration to the texture so the mode of the texture is used for the texture view for
   * example and returns it
   */
  wgpu::BindGroupLayoutEntry CreateBindGroupLayoutEntry(uint32_t binding,
    vtkSmartPointer<vtkWebGPUComputeTexture> computeTexture,
    wgpu::TextureViewDimension textureViewDimension);

  /**
   * Given a buffer, creates the associated bind group entry
   * that will be used when creating the bind groups and returns it
   */
  wgpu::BindGroupEntry CreateBindGroupEntry(wgpu::Buffer buffer, uint32_t binding,
    vtkWebGPUComputeBuffer::BufferMode mode, uint32_t offset);

  /**
   * Given a texture view, creates the associated bind group entry
   * that will be used when creating the bind groups and returns it
   */
  wgpu::BindGroupEntry CreateBindGroupEntry(uint32_t binding, wgpu::TextureView textureView);

  /**
   * Compiles the shader source given into a WGPU shader module
   */
  void CreateShaderModule();

  /**
   * Creates all the bind groups and bind group layouts of this compute pass from the buffers
   * that have been added so far.
   */
  void CreateBindGroupsAndLayouts();

  /**
   * Creates the bind group layout of a given list of buffers (that must all belong to the same bind
   * group)
   */
  static wgpu::BindGroupLayout CreateBindGroupLayout(
    const wgpu::Device& device, const std::vector<wgpu::BindGroupLayoutEntry>& layoutEntries);

  /**
   * Creates the bind group entries given a list of buffers
   */
  std::vector<wgpu::BindGroupEntry> CreateBindGroupEntries(
    const std::vector<vtkWebGPUComputeBuffer*>& buffers);

  /**
   * Checks if a given index is suitable for indexing Buffers. Logs an error if the index is
   * negative or greater than the number of buffer of the compute pass.
   *
   * The callerFunctionName
   * parameter is using to give more information on what function used an invalid buffer index
   *
   * Returns true if the buffer index is valid, false if it's not.
   */
  bool CheckBufferIndex(int bufferIndex, const std::string& callerFunctionName);

  /**
   * Checks if a given index corresponds to a texture of this compute pass.
   *
   * Returns true if the texture index is valid, false if it's not.
   */
  bool CheckTextureIndex(int textureIndex, const std::string& callerFunctionName);

  /**
   * Checks if a given index corresponds to a texture texture view of this compute pass.
   *
   * Returns true if the texture view index is valid, false if it's not.
   */
  bool CheckTextureViewIndex(int textureViewIndex, const std::string& callerFunctionName);

  /**
   * Returns true if the buffer is ready to be added to the compute pass or not.
   * Returns false otherwise.
   */
  bool CheckBufferCorrectness(vtkSmartPointer<vtkWebGPUComputeBuffer> buffer);

  /**
   * Recreates the bind group and bind group entry of a buffer (given by its index)
   *
   * The function is useful after recreating a wgpu::Buffer, the bind group entry (and the bind
   * group) will need to be updated because the wgpu::Buffer object has changed. This function thus
   * assumes that the new buffer can be found in WebGPUBuffers[bufferIndex]
   */
  void RecreateBufferBindGroup(int bufferIndex);

  /**
   * Registers a buffer to the associated compute pipeline of this compute pass so that other
   * compute passes of the same pipeline can reuse it.
   */
  void RegisterBufferToPipeline(
    vtkSmartPointer<vtkWebGPUComputeBuffer> buffer, wgpu::Buffer wgpuBuffer);

  /// @{
  /**
   * Checks whether the given compute buffer/texture has already been registered in
   * the pipeline associated to this compute pass. If so, returns true and sets the parameter to the
   * WebGPU object registered in this pipeline.
   *
   * Returns false and leaves the parameter unchanged otherwise.
   */
  bool GetRegisteredBufferFromPipeline(
    vtkSmartPointer<vtkWebGPUComputeBuffer> buffer, wgpu::Buffer& wgpuBuffer);

  bool GetRegisteredTextureFromPipeline(
    vtkSmartPointer<vtkWebGPUComputeTexture> texture, wgpu::Texture& wgpuTexture);
  /// @}

  /**
   * Returns the wgpu::Buffer object for a buffer in this compute pass buffer storage given its
   * index
   */
  wgpu::Buffer GetWGPUBuffer(std::size_t bufferIndex);

  /**
   * Registers a texture to the associated compute pipeline of this compute pass so that other
   * compute passes of the same pipeline can reuse it.
   */
  void RegisterTextureToPipeline(
    vtkSmartPointer<vtkWebGPUComputeTexture> texture, wgpu::Texture wgpuTexture);

  /**
   * Destroys and recreates a buffer with the given newByteSize
   * Only the wgpu::Buffer object is recreated so the binding/group of the group doesn't change
   */
  void RecreateBuffer(int bufferIndex, vtkIdType newByteSize);

  /**
   * Destroys and recreates the texture with the given index.
   */
  void RecreateTexture(int textureIndex);

  /**
   * Recreates all the texture views of a texture given its index.
   *
   * Useful when a texture has been recreated, meaning that the wgpu::Texture of this compute pass
   * has changed --> the texture view do not point to a correct texture anymore and need to be
   * recreated
   */
  void RecreateTextureViews(int textureIndex);

  /**
   * Utilitary method to create a wgpu::TextureView from a ComputeTextureView and the texture this
   * wgpu::TextureView is going to be a view off
   */
  wgpu::TextureView CreateWebGPUTextureView(
    vtkSmartPointer<vtkWebGPUComputeTextureView> textureView, wgpu::Texture wgpuTexture);

  /**
   * Updates the wgpu::Buffer reference that a compute buffer is associated to. Useful when a
   * compute buffer has been recreated and the associated wgpu::Buffer needs to be updated with the
   * newly created buffer
   */
  void UpdateWebGPUBuffer(vtkSmartPointer<vtkWebGPUComputeBuffer> buffer, wgpu::Buffer wgpuBuffer);

  /**
   * Makes sure that the compute texture given in parameter internally points to the given
   * newWgpuTexture. If this is not initially the case, it will be true after the call to this
   * function. Also, all texture views of this texture will now be views of the given newWgpuTexture
   *
   * This is useful when recreating the compute texture from another compute pass: the compute
   * pipeline will be responsible for calling on all its compute passes to make sure that if a
   * compute pass was using the texture that was recreated, it now uses the recreated texture and
   * not the old one
   */
  void UpdateComputeTextureAndViews(
    vtkSmartPointer<vtkWebGPUComputeTexture> texture, wgpu::Texture newWgpuTexture);

  /**
   * After recreating a wgpu::Buffer, the bind group entry (and the bind group) will need to be
   * updated. This
   */
  void RecreateTextureBindGroup(int textureIndex);

  /**
   * Binds the buffer to the compute pass at the WebGPU level.
   * To use once the buffer has been properly set up.
   */
  void SetupRenderBuffer(vtkSmartPointer<vtkWebGPUComputeRenderBuffer> renderBuffer);

  /**
   * Recreates a render texture given a new textureView and possibly new parameters as specified in
   * the 'renderTexture' parameter. This also recreates the texture views that were created on this
   * render texture.
   *
   * This function is mainly called after the render window has been resized and render textures
   * have thus also been resized.
   */
  void RecreateRenderTexture(vtkSmartPointer<vtkWebGPUComputeRenderTexture> renderTexture);

  /**
   * Creates the compute pipeline that will be used to dispatch the compute shader
   */
  void CreateWebGPUComputePipeline();

  /**
   * Creates the compute pipeline layout associated with the bind group layouts of this compute
   * pass
   *
   * @warning: The bind group layouts must have been created by CreateBindGroups() prior to calling
   * this function
   */
  wgpu::PipelineLayout CreateWebGPUComputePipelineLayout();

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
   * @warning: The bind groups and the compute pipeline must have been created prior to calling this
   * function
   */
  void WebGPUDispatch(unsigned int groupsX, unsigned int groupsY, unsigned int groupsZ);

  /**
   * Finishes the encoding of a command encoder and submits the resulting command buffer
   * to the queue
   */
  void SubmitCommandEncoderToQueue(const wgpu::CommandEncoder& commandEncoder);

  /**
   * Releases the resources of this compute pass internals.
   */
  void ReleaseResources();

  // Compute pass whose internals this class represents
  vtkWeakPointer<vtkWebGPUComputePass> ParentPass;

protected:
  vtkWebGPUComputePassInternals() = default;
  ~vtkWebGPUComputePassInternals() override;

private:
  friend class vtkWebGPUComputePass;
  friend class vtkWebGPUComputePassBufferStorageInternals;
  friend class vtkWebGPUComputePassTextureStorageInternals;
  friend class vtkWebGPUComputePipeline;
  // For the mapper to be able to access the wgpu::Buffer objects for use in a render pipeline
  friend class vtkWebGPUPointCloudMapperInternals;

  vtkWebGPUComputePassInternals(const vtkWebGPUComputePassInternals&) = delete;
  void operator=(const vtkWebGPUComputePassInternals&) = delete;

  /**
   * Whether or not the shader module, binds groups, layouts and the wgpu::ComputePipeline have
   * been created already
   */
  bool Initialized = false;
  // Whether or not the binds group / layouts have changed since they were last created and so they
  // need to be recreated. Defaults to true since the bind group / layouts are initially not created
  // so they are invalid.
  bool BindGroupOrLayoutsInvalidated = true;

  // Device of the compute pipeline this pass belongs to. Used to submit commands.
  vtkSmartPointer<vtkWebGPUConfiguration> WGPUConfiguration;

  // The compute pipeline this compute pass belongs to.
  vtkWeakPointer<vtkWebGPUComputePipeline> AssociatedPipeline;

  wgpu::ShaderModule ShaderModule;
  // List of the bind groups, used to set the bind groups of the compute pass at each dispatch
  std::vector<wgpu::BindGroup> BindGroups;
  // Maps a bind group index to to the list of bind group entries for this group. These
  // entries will be used at the creation of the bind groups
  std::unordered_map<int, std::vector<wgpu::BindGroupEntry>> BindGroupEntries;
  std::vector<wgpu::BindGroupLayout> BindGroupLayouts;
  // Maps a bind group index to to the list of bind group layout entries for this group.
  // These layout entries will be used at the creation of the bind group layouts
  std::unordered_map<int, std::vector<wgpu::BindGroupLayoutEntry>> BindGroupLayoutEntries;
  // WebGPU compute shader pipeline
  wgpu::ComputePipeline ComputePipeline;

  // Object responsible for the management (creation, re-creatioin, deletion, ...) of textures and
  // their texture views
  vtkSmartPointer<vtkWebGPUComputePassTextureStorageInternals> TextureStorage;

  // Object responsible for the management (creation, re-creatioin, deletion, ...) of buffers
  vtkSmartPointer<vtkWebGPUComputePassBufferStorageInternals> BufferStorage;
};

VTK_ABI_NAMESPACE_END

#endif
