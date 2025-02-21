// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkWebGPURenderer_h
#define vtkWebGPURenderer_h

#include "vtkRenderer.h"

#include "vtkRenderingWebGPUModule.h" // for export macro
#include "vtkSmartPointer.h"          // for ivar
#include "vtkWebGPUComputePipeline.h" // for the compute pipelines used by this renderer
#include "vtk_wgpu.h"                 // for webgpu

#include <unordered_set> // for the set of actors rendered last frame

class vtkAbstractMapper;
class vtkRenderState;
class vtkFrameBufferObjectBase;

VTK_ABI_NAMESPACE_BEGIN

class vtkWebGPUComputeOcclusionCuller;

class VTKRENDERINGWEBGPU_EXPORT vtkWebGPURenderer : public vtkRenderer
{
public:
  static vtkWebGPURenderer* New();
  vtkTypeMacro(vtkWebGPURenderer, vtkRenderer);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  struct RenderPipelineBatch
  {
    vtkSmartPointer<vtkPropCollection> Props;
    wgpu::RenderPipeline Pipeline;
  };

  // get the complexity of the current lights as a int
  // 0 = no lighting
  // 1 = headlight
  // 2 = directional lights
  // 3 = positional lights
  enum LightingComplexityEnum
  {
    NoLighting = 0,
    Headlight = 1,
    Directional = 2,
    Positional = 3
  };
  vtkGetMacro(LightingComplexity, int);

  enum RenderStageEnum
  {
    AwaitingPreparation,
    UpdatingBuffers,
    RecordingCommands,
    Finished,
    RenderPostRasterization
  };

  /**
   * Clear the image to the background color.
   */
  void Clear() override;

  /**
   * Create an image.
   */
  void DeviceRender() override;

  /**
   * Updates / creates the various buffer necessary for the rendering of the props.
   * This is a chance for actors, mappers, cameras and lights to push their data
   * from a staging area (or) `vtkDataObject` subclasses into `wgpu::Buffer` or `wgpu::Texture`.
   */
  void UpdateBuffers();

  /**
   * Ask all props to update and draw any opaque and translucent
   * geometry. This includes both vtkActors and vtkVolumes
   * Returns the number of props that rendered geometry.
   */
  int UpdateGeometry(vtkFrameBufferObjectBase* fbo = nullptr) override;

  /**
   * Set up the buffers of a given vtkWebGPUComputePass.
   * Loops through all the actors of this renderer. If an access to the data attributes buffer of
   * the actor was requested by the user through
   * vtkWebGPUPolyDataMapper::AcquirePointAttributeComputeRenderBuffer(), we'll have to set up the
   * WebGPU buffer to access the point data attributes (if it belongs to the right compute pass).
   */
  void ConfigureComputeRenderBuffers(vtkSmartPointer<vtkWebGPUComputePipeline> computePipeline);

  /**
   * Sets the adapter and the device of the render window of this renderer to the compute pipelines
   * of this renderer
   */
  void ConfigureComputePipelines();

  /// @{
  /**
   * Returns the list of compute pipelines of this renderer that have been setup for execution
   * before/after the rendering pass
   */
  const std::vector<vtkSmartPointer<vtkWebGPUComputePipeline>>& GetSetupPreRenderComputePipelines();
  const std::vector<vtkSmartPointer<vtkWebGPUComputePipeline>>&
  GetSetupPostRenderComputePipelines();
  /// @}

  int UpdateLights() override;

  void SetEnvironmentTexture(vtkTexture* texture, bool isSRGB = false) override;

  void ReleaseGraphicsResources(vtkWindow* w) override;

  inline wgpu::RenderPassEncoder GetRenderPassEncoder() { return this->WGPURenderEncoder; }
  inline wgpu::RenderBundleEncoder GetRenderBundleEncoder() { return this->WGPUBundleEncoder; }
  inline wgpu::BindGroup GetSceneBindGroup() { return this->SceneBindGroup; }

  inline void PopulateBindgroupLayouts(std::vector<wgpu::BindGroupLayout>& layouts)
  {
    layouts.emplace_back(this->SceneBindGroupLayout);
  }

  /// @{
  /**
   * Adds a compute pipeline to the renderer that will be executed each frame before/after the
   * rendering pass.
   */
  void AddPreRenderComputePipeline(vtkSmartPointer<vtkWebGPUComputePipeline> pipeline);
  void AddPostRenderComputePipeline(vtkSmartPointer<vtkWebGPUComputePipeline> pipeline);
  /// @}

  /**
   * Returns the list of the actors that were rendered last frame
   */
  std::unordered_set<vtkProp*> GetPropsRendered() { return this->PropsRendered; }

  ///@{
  /**
   * Set the user light transform applied after the camera transform.
   * Can be null to disable it.
   */
  void SetUserLightTransform(vtkTransform* transform);
  vtkTransform* GetUserLightTransform();
  ///@}

  ///@{
  /**
   * Set the usage of render bundles. This speeds up rendering in wasm.
   * Render bundles are a performance optimization that minimize CPU time for rendering large number
   * of props.
   * @warning LEAKS MEMORY. See vtkWebGPURenderer::DeviceRender
   */
  vtkSetMacro(UseRenderBundles, bool);
  vtkBooleanMacro(UseRenderBundles, bool);
  vtkGetMacro(UseRenderBundles, bool);
  ///@}

  /**
   * Query the stage in the rendering process.
   * This property tells the actors and mappers what should be done in their `Render` calls.
   * When it is equal to `UpdatingBuffers`, the actors and mappers can upload data into wgpu
   * buffers. When it is equal to `RecordingCommands`, the mappers should record draw commands,
   * pipeline changes and bindgroup changes into the render pass encoder or a render bundle encoder.
   * Finally, when it is in `RenderPostRasterization` stage, only the actors added into the list
   * of post rasterization actors, and whose mappers support post rasterization will be rendered.
   */
  vtkGetEnumMacro(RenderStage, RenderStageEnum);

  /**
   * Forces the renderer to re-record draw commands into a render bundle.
   *
   * @note This does not use vtkSetMacro because the actor MTime should not be affected when a
   * render bundle is invalidated.
   */
  inline void InvalidateBundle()
  {
    this->RebuildRenderBundle = true;
    this->Bundle = nullptr;
  }

  /**
   * Get whether the render bundle associated with this actor must be reset by the renderer.
   */
  vtkGetMacro(RebuildRenderBundle, bool);

protected:
  vtkWebGPURenderer();
  ~vtkWebGPURenderer() override;

  /**
   * Request mappers to run the vtkAlgorithm pipeline (if needed)
   * and consequently update device buffers corresponding to shader module bindings.
   * Ex: positions, colors, normals, indices
   * Request mappers to bind descriptor sets (bind groups) and encode draw commands.
   */
  int UpdateOpaquePolygonalGeometry() override;
  int UpdateTranslucentPolygonalGeometry() override;

  // Setup scene and actor bindgroups. Actor has dynamic offsets.
  void SetupBindGroupLayouts();
  // Create buffers for the bind groups.
  void CreateBuffers();
  // Create scene bind group.
  void SetupSceneBindGroup();

  // Start, finish recording commands.
  void BeginRecording();
  void EndRecording();

  std::size_t WriteLightsBuffer(std::size_t offset = 0);
  std::size_t WriteSceneTransformsBuffer(std::size_t offset = 0);

  wgpu::RenderPassEncoder WGPURenderEncoder;
  wgpu::RenderBundleEncoder WGPUBundleEncoder;
  wgpu::Buffer SceneTransformBuffer;
  wgpu::Buffer SceneLightsBuffer;

  wgpu::BindGroup SceneBindGroup;
  wgpu::BindGroupLayout SceneBindGroupLayout;

#ifdef __EMSCRIPTEN__
  bool UseRenderBundles = true;
#else
  bool UseRenderBundles = false;
#endif
  bool RebuildRenderBundle = false;
  // the commands in bundle get reused every frame.
  wgpu::RenderBundle Bundle;

  int LightingComplexity = 0;
  std::size_t NumberOfLightsUsed = 0;
  std::vector<std::size_t> LightIDs;

  vtkMTimeType LightingUpdateTime;
  vtkTimeStamp LightingUploadTimestamp;

  /**
   * Optional user transform for lights
   */
  vtkSmartPointer<vtkTransform> UserLightTransform;

private:
  friend class vtkWebGPUComputeOcclusionCuller;
  // For the mapper to access 'AddPostRasterizationActor'
  friend class vtkWebGPUComputePointCloudMapper;
  // For render window accessing PostRenderComputePipelines()
  friend class vtkWebGPURenderWindow;

  vtkWebGPURenderer(const vtkWebGPURenderer&) = delete;
  void operator=(const vtkWebGPURenderer&) = delete;

  /**
   * Sets the device and adapter of the render window of this renderer to the given pipeline
   */
  void InitComputePipeline(vtkSmartPointer<vtkWebGPUComputePipeline> pipeline);

  /// @{
  /**
   * Dispatches the compute pipelines attached to this renderer in the order they were added by
   * AddPreRenderComputePipeline() / AddPostRenderComputePipeline().
   *
   * This function only dispatches the compute pipelines that were given by the user to execute
   * before/after the rendering pass
   */
  void PreRenderComputePipelines();
  void PostRenderComputePipelines();
  /// @}

  /**
   * Renders actors contained in the PostRasterizationActors vector after the pass that rasterizes
   * the other actors of this renderer. This is mainly useful when some actors are rendered with
   * compute shaders (through compute pipelines) because compute shaders that write to the
   * framebuffer of the render window cannot be interleaved with rasterization pipeline render
   * commands (in-between BeginEncoding() and EndEncoding() calls).
   *
   * This method is called by the render window after the rasterization render pass has been flushed
   * to the device to make sure that all resources are up to date (depth buffer, frame buffer)
   */
  void PostRasterizationRender();

  /**
   * Adds an actor to be rendered after the main rasterization pass
   */
  void AddPostRasterizationActor(vtkActor* actor);

  /**
   * Compute pipelines (post and pre render) that have been setup and that will be dispatched
   * by the renderer before the rendering passes
   */
  std::vector<vtkSmartPointer<vtkWebGPUComputePipeline>> SetupPreRenderComputePipelines;
  std::vector<vtkSmartPointer<vtkWebGPUComputePipeline>> SetupPostRenderComputePipelines;

  /**
   * Compute pipelines (post and pre render) that have yet to be setup
   */
  std::vector<vtkSmartPointer<vtkWebGPUComputePipeline>> NotSetupPreRenderComputePipelines;
  std::vector<vtkSmartPointer<vtkWebGPUComputePipeline>> NotSetupPostRenderComputePipelines;

  /**
   * Actors that will be rendered by PostRasterizationRender() after the main rasterization pass.
   * Actors are added to this list when the Render() method of an actor is called but the mapper of
   * this actor determines that it needs to be rendered after the rasterization pass. The mapper
   * will then add the actor to this list of the renderer so that the renderer can render the actor
   * after the rasterization pass.
   */
  std::vector<vtkActor*> PostRasterizationActors;

  /**
   * Encodes a render command for rendering the given props
   */
  wgpu::CommandBuffer EncodePropListRenderCommand(vtkProp** propList, int listLength);

  /**
   * Records commands into a render pass encoder.
   * This method records commands which draw the background texture/clear color
   * and commands which render all the props contained in this renderer.
   */
  void RecordRenderCommands();

  /**
   * Whether the compute render buffers of the mappers of the actors of this renderer have already
   * been initialized or not
   */
  bool ComputeBuffersInitialized = false;

  /**
   * Indicates whether PrepareRender() was called already for this frame or not (and thus we do not
   * need to call it again).
   */
  RenderStageEnum RenderStage = RenderStageEnum::AwaitingPreparation;

  /**
   * Whether to clear the depth/stencil/color buffer before rendering
   */
  bool DrawBackgroundInClearPass = true;

  /**
   * List of the actors rendered last frame. Mainly used by the occlusion culler when we want to
   * render the actors that were rendered last frame in the first pass to build the z-buffer.
   * Using a set here to be able to efficiently run find operations on the list (the set) of actors
   * rendered. It makes no sense to have the same actor twice in the list anyway so a set is fine.
   */
  std::unordered_set<vtkProp*> PropsRendered;
};

VTK_ABI_NAMESPACE_END
#endif
