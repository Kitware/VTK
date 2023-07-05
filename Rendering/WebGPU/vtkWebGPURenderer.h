// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkWebGPURenderer_h
#define vtkWebGPURenderer_h

#include "vtkRenderer.h"

#include "vtkRenderingWebGPUModule.h" // for export macro
#include "vtkSmartPointer.h"          // for ivar
#include "vtkTypeUInt32Array.h"       // for ivar
#include "vtk_wgpu.h"                 // for webgpu

#include <string>        // for ivar
#include <unordered_map> // for ivar

class vtkAbstractMapper;
class vtkRenderState;
class vtkFrameBufferObjectBase;

VTK_ABI_NAMESPACE_BEGIN
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

  void DeviceRender() override;

  void Clear() override;

  /**
   * Ask all props to update themselves. This process should be limited
   * to wgpu::Buffer uploads, creation of bind groups, bind group layouts,
   * graphics pipeline. Basically, do everything necessary but do NOT encode
   * render pass commads.
   */
  int UpdateGeometry(vtkFrameBufferObjectBase* fbo = nullptr) override;

  /**
   * Request props to encode render commands.
   */
  int RenderGeometry();

  int UpdateLights() override;

  void SetEnvironmentTexture(vtkTexture* texture, bool isSRGB = false) override;

  void ReleaseGraphicsResources(vtkWindow* w) override;

  inline wgpu::RenderPassEncoder GetRenderPassEncoder() { return this->WGPURenderEncoder; }
  inline wgpu::BindGroup GetActorBindGroup() { return this->ActorBindGroup; }
  inline wgpu::BindGroup GetSceneBindGroup() { return this->SceneBindGroup; }

  inline void PopulateBindgroupLayouts(std::vector<wgpu::BindGroupLayout>& layouts)
  {
    layouts.emplace_back(this->SceneBindGroupLayout);
    layouts.emplace_back(this->ActorBindGroupLayout);
  }

  wgpu::ShaderModule HasShaderCache(const std::string& source);
  void InsertShader(const std::string& source, wgpu::ShaderModule shader);

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
   * @warning LEAKS MEMORY. See vtkWebGPURenderer::DeviceRender
   */
  vtkSetMacro(UseRenderBundles, bool);
  vtkBooleanMacro(UseRenderBundles, bool);
  vtkGetMacro(UseRenderBundles, bool);
  ///@}

protected:
  vtkWebGPURenderer();
  ~vtkWebGPURenderer() override;

  /**
   * Request mappers to run the vtkAlgorithm pipeline (if needed)
   * and consequently update device buffers corresponding to shader module bindings.
   * Ex: positions, colors, normals, indices
   */
  int UpdateOpaquePolygonalGeometry() override;
  int UpdateTranslucentPolygonalGeometry() override;

  /**
   * Request mappers to bind descriptor sets (bind groups) and encode draw commands.
   */
  void DeviceRenderOpaqueGeometry(vtkFrameBufferObjectBase* fbo) override;
  void DeviceRenderTranslucentPolygonalGeometry(vtkFrameBufferObjectBase* fbo) override;

  // Setup scene and actor bindgroups. Actor has dynamic offsets.
  void SetupBindGroupLayouts();
  // Creates buffers for the bind groups.
  void CreateBuffers();
  // Updates the bound buffers with data.
  std::size_t UpdateBufferData();
  // Creates scene bind group.
  void SetupSceneBindGroup();
  // Creates actor bind group.
  void SetupActorBindGroup();

  // Start, finish recording commands with render pass encoder
  void BeginEncoding();
  void EndEncoding();

  std::size_t WriteLightsBuffer(std::size_t offset = 0);
  std::size_t WriteSceneTransformsBuffer(std::size_t offset = 0);
  std::size_t WriteActorBlocksBuffer(std::size_t offset = 0);

  wgpu::RenderPassEncoder WGPURenderEncoder;
  wgpu::Buffer SceneTransformBuffer;
  wgpu::Buffer SceneLightsBuffer;
  wgpu::Buffer ActorBlocksBuffer;
  wgpu::BindGroup SceneBindGroup;
  wgpu::BindGroupLayout SceneBindGroupLayout;

  wgpu::BindGroup ActorBindGroup;
  wgpu::BindGroupLayout ActorBindGroupLayout;

#ifdef __EMSCRIPTEN__
  bool UseRenderBundles = true;
#else
  bool UseRenderBundles = false;
#endif
  // one bundle per actor. bundle gets reused every frame.
  // these bundles can be built in parallel with vtkSMPTools. holding off because not
  // sure how to get emscripten to thread.
  std::vector<wgpu::RenderBundle> Bundles;
  struct vtkWGPUPropItem
  {
    wgpu::RenderBundle Bundle = nullptr;
    vtkSmartPointer<vtkTypeUInt32Array> DynamicOffsets;
  };
  std::unordered_map<vtkProp*, vtkWGPUPropItem> PropWGPUItems;

  std::unordered_map<std::string, wgpu::ShaderModule> ShaderCache;
  std::size_t NumberOfPropsUpdated = 0;
  int LightingComplexity = 0;
  std::size_t NumberOfLightsUsed = 0;
  std::vector<std::size_t> LightIDs;

  vtkMTimeType LightingUpdateTime;
  vtkTimeStamp LightingUploadTimestamp;

  struct
  {
    uint32_t Hits = 0;
    uint32_t Misses = 0;
    uint32_t TotalRequests = 0;
  } BundleCacheStats;

  /**
   * Optional user transform for lights
   */
  vtkSmartPointer<vtkTransform> UserLightTransform;

private:
  vtkWebGPURenderer(const vtkWebGPURenderer&) = delete;
  void operator=(const vtkWebGPURenderer&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
