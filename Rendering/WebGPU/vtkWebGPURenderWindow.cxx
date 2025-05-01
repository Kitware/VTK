// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWebGPURenderWindow.h"
#include "Private/vtkWebGPUBindGroupInternals.h"
#include "Private/vtkWebGPUBindGroupLayoutInternals.h"
#include "Private/vtkWebGPUComputePassInternals.h"
#include "Private/vtkWebGPUPipelineLayoutInternals.h"
#include "Private/vtkWebGPURenderPassDescriptorInternals.h"
#include "Private/vtkWebGPURenderPipelineDescriptorInternals.h"
#include "vtkCollectionRange.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkRendererCollection.h"
#include "vtkTypeUInt32Array.h"
#include "vtkUnsignedCharArray.h"
#include "vtkWebGPUCommandEncoderDebugGroup.h"
#include "vtkWebGPUConfiguration.h"
#include "vtkWebGPUHelpers.h"
#include "vtkWebGPURenderer.h"

#include "vtksys/SystemTools.hxx"

#include "CopyDepthTextureToBuffer.h"

#include <sstream>

#if defined(__EMSCRIPTEN__)
#include "emscripten/version.h"
#endif

VTK_ABI_NAMESPACE_BEGIN

#define vtkWebGPUCheckUnconfiguredWithReturn(renderWindow, retVal)                                 \
  do                                                                                               \
  {                                                                                                \
    if (renderWindow->WGPUConfiguration == nullptr)                                                \
    {                                                                                              \
      vtkErrorWithObjectMacro(                                                                     \
        renderWindow, << "This render window is not configured to use webgpu. Please call "        \
                         "vtkWebGPURenderWindow::SetWGPUConfiguration().");                        \
      return retVal;                                                                               \
    }                                                                                              \
  } while (0)

#define vtkWebGPUCheckUnconfigured(renderWindow)                                                   \
  do                                                                                               \
  {                                                                                                \
    if (renderWindow->WGPUConfiguration == nullptr)                                                \
    {                                                                                              \
      vtkErrorWithObjectMacro(                                                                     \
        renderWindow, << "This render window is not configured to use webgpu. Please call "        \
                         "vtkWebGPURenderWindow::SetWGPUConfiguration().");                        \
      return;                                                                                      \
    }                                                                                              \
  } while (0)

namespace
{
struct InternalMapTextureAsyncData
{
  // Buffer currently being mapped
  wgpu::Buffer buffer;
  // Label of the buffer currently being mapped. Used for printing errors
  std::string bufferLabel;
  // Size of the buffer being mapped in bytes
  vtkIdType byteSize;

  // Userdata passed to userCallback. This is typically the structure that contains the CPU-side
  // buffer into which the data of the mapped buffer will be copied
  void* userData;

  // Bytes per row of the padded buffer that contains the mapped texture data
  int bytesPerRow;
  // Callback given by the user
  vtkWebGPURenderWindow::TextureMapCallback userCallback;
};
}

//------------------------------------------------------------------------------
vtkWebGPURenderWindow::vtkWebGPURenderWindow()
{
  this->ScreenSize[0] = 0;
  this->ScreenSize[1] = 0;
  this->WGPUConfiguration = vtk::TakeSmartPointer(vtkWebGPUConfiguration::New());
}

//------------------------------------------------------------------------------
vtkWebGPURenderWindow::~vtkWebGPURenderWindow() = default;

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
bool vtkWebGPURenderWindow::WGPUInit()
{
  vtkDebugMacro(<< __func__ << " Initialized=" << this->Initialized);
  vtkWebGPUCheckUnconfiguredWithReturn(this, false);
  if (!this->WGPUConfiguration->Initialize())
  {
    return false;
  }
  if (this->WindowName == std::string("Visualization Toolkit"))
  {
    std::string windowNameWithBackend = this->MakeDefaultWindowNameWithBackend();
    this->SetWindowName(windowNameWithBackend.c_str());
  }
  return true;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::Initialize()
{
  if (!this->WindowSetup()) // calls WGPUInit after surface is created.
  {
    vtkLog(ERROR, "Unable to setup WebGPU.");
    return;
  }

  this->ConfigureSurface();
  this->CreateOffscreenColorAttachment();
  this->CreateIdsAttachment();
  this->CreateDepthStencilAttachment();
  this->CreateColorCopyPipeline();
  this->InitializeRendererComputePipelines();

  this->Initialized = true;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::WGPUFinalize()
{
  vtkDebugMacro(<< __func__ << " Initialized=" << this->Initialized);
  vtkWebGPUCheckUnconfigured(this);
  if (!this->Initialized)
  {
    return;
  }
  this->ReleaseGraphicsResources(this);
  this->WGPUConfiguration->Finalize();
  this->Initialized = false;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::Render()
{
  vtkDebugMacro(<< __func__);

  this->Superclass::Render();
}

//------------------------------------------------------------------------------
wgpu::RenderPassEncoder vtkWebGPURenderWindow::NewRenderPass(wgpu::RenderPassDescriptor& descriptor)
{
  if (this->CommandEncoder)
  {
    return this->CommandEncoder.BeginRenderPass(&descriptor);
  }
  else
  {
    vtkErrorMacro(<< "Cannot create a new render pass because CommandEncoder is null!");
    return nullptr;
  }
}

//------------------------------------------------------------------------------
wgpu::RenderBundleEncoder vtkWebGPURenderWindow::NewRenderBundleEncoder(
  wgpu::RenderBundleEncoderDescriptor& descriptor)
{
  vtkWebGPUCheckUnconfiguredWithReturn(this, nullptr);
  if (auto device = this->WGPUConfiguration->GetDevice())
  {
    return device.CreateRenderBundleEncoder(&descriptor);
  }
  else
  {
    vtkErrorMacro(<< "Cannot create a render bundle encoder because WebGPU device is not ready!");
    return nullptr;
  }
}

//------------------------------------------------------------------------------
wgpu::CommandEncoder vtkWebGPURenderWindow::GetCommandEncoder()
{
  return this->CommandEncoder;
}

//------------------------------------------------------------------------------
wgpu::TextureView vtkWebGPURenderWindow::GetOffscreenColorAttachmentView()
{
  return this->ColorAttachment.View;
}

//------------------------------------------------------------------------------
wgpu::TextureView vtkWebGPURenderWindow::GetHardwareSelectorAttachmentView()
{
  return this->IdsAttachment.View;
}

//------------------------------------------------------------------------------
wgpu::TextureView vtkWebGPURenderWindow::GetDepthStencilView()
{
  return this->DepthStencilAttachment.View;
}

//------------------------------------------------------------------------------
wgpu::TextureFormat vtkWebGPURenderWindow::GetDepthStencilFormat()
{
  return this->DepthStencilAttachment.Format;
}

//------------------------------------------------------------------------------
bool vtkWebGPURenderWindow::HasStencil()
{
  return this->DepthStencilAttachment.HasStencil;
}

//------------------------------------------------------------------------------
wgpu::Device vtkWebGPURenderWindow::GetDevice()
{
  vtkWebGPUCheckUnconfiguredWithReturn(this, nullptr);
  return this->WGPUConfiguration->GetDevice();
}

//------------------------------------------------------------------------------
wgpu::Adapter vtkWebGPURenderWindow::GetAdapter()
{
  vtkWebGPUCheckUnconfiguredWithReturn(this, nullptr);
  return this->WGPUConfiguration->GetAdapter();
}

//------------------------------------------------------------------------------
wgpu::TextureFormat vtkWebGPURenderWindow::GetPreferredSurfaceTextureFormat()
{
  return this->PreferredSurfaceTextureFormat;
}

//------------------------------------------------------------------------------
wgpu::TextureFormat vtkWebGPURenderWindow::GetPreferredSelectorIdsTextureFormat()
{
  return this->PreferredSelectorIdsTextureFormat;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkWebGPUComputeRenderTexture>
vtkWebGPURenderWindow::AcquireDepthBufferRenderTexture()
{
  if (!this->Initialized)
  {
    vtkLog(ERROR,
      "You must call vtkRenderWindow::Initialize() before acquiring a render texture from the "
      "RenderWindow.");
    return nullptr;
  }

  vtkSmartPointer<vtkWebGPUComputeRenderTexture> texture =
    vtkSmartPointer<vtkWebGPUComputeRenderTexture>::New();

  int* dims = this->GetSize();

  texture->SetSize(dims[0], dims[1]);
  texture->SetMode(vtkWebGPUComputeTexture::TextureMode::READ_ONLY);
  texture->SetSampleType(vtkWebGPUComputeTexture::TextureSampleType::DEPTH);
  texture->SetAspect(vtkWebGPUComputeTextureView::TextureViewAspect::ASPECT_DEPTH);
  texture->SetLabel("Depth buffer render texture");
  texture->SetType(vtkWebGPUComputeRenderTexture::RenderTextureType::DEPTH_BUFFER);
  texture->SetWebGPUTexture(this->DepthStencilAttachment.Texture);
  texture->SetFormat(vtkWebGPUComputeTexture::TextureFormat::DEPTH_24_PLUS_8_STENCIL);

  this->ComputeRenderTextures.push_back(texture);

  return texture;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkWebGPUComputeRenderTexture>
vtkWebGPURenderWindow::AcquireFramebufferRenderTexture()
{
  vtkSmartPointer<vtkWebGPUComputeRenderTexture> texture =
    vtkSmartPointer<vtkWebGPUComputeRenderTexture>::New();

  int* dims = this->GetSize();

  texture->SetSize(dims[0], dims[1]);
  texture->SetFormat(vtkWebGPUTexture::TextureFormat::BGRA8_UNORM);
  texture->SetSampleType(vtkWebGPUComputeTexture::TextureSampleType::FLOAT);
  texture->SetLabel("Framebuffer render texture");
  texture->SetType(vtkWebGPUComputeRenderTexture::RenderTextureType::COLOR_BUFFER);
  texture->SetMode(vtkWebGPUComputeRenderTexture::TextureMode::WRITE_ONLY_STORAGE);
  texture->SetWebGPUTexture(this->ColorAttachment.Texture);
  texture->SetAspect(vtkWebGPUComputeTextureView::TextureViewAspect::ASPECT_ALL);

  this->ComputeRenderTextures.push_back(texture);

  return texture;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::CreateCommandEncoder()
{
  vtkWebGPUCheckUnconfigured(this);
  wgpu::CommandEncoderDescriptor encDesc = {};
  std::stringstream label;
  encDesc.label = "vtkWebGPURenderWindow::CommandEncoder";
  if (auto device = this->WGPUConfiguration->GetDevice())
  {
    this->CommandEncoder = device.CreateCommandEncoder(&encDesc);
  }
  else
  {
    vtkErrorMacro(
      << "Cannot create a command encoder because a WebGPU device has not been initialized!");
  }
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::InitializeRendererComputePipelines()
{
  for (auto renderer : vtk::Range(this->Renderers))
  {
    vtkWebGPURenderer* wgpuRenderer = vtkWebGPURenderer::SafeDownCast(renderer);
    wgpuRenderer->ConfigureComputePipelines();
  }
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::SubmitCommandBuffer(int count, wgpu::CommandBuffer* commandBuffer)
{
  this->FlushCommandBuffers(count, commandBuffer);
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::ConfigureSurface()
{
  vtkDebugMacro(<< __func__ << '(' << this->Size[0] << ',' << this->Size[1] << ')');
  vtkWebGPUCheckUnconfigured(this);
  // Configure the surface.
  wgpu::SurfaceCapabilities capabilities;
  this->Surface.GetCapabilities(this->GetAdapter(), &capabilities);
  if (capabilities.formatCount > 0)
  {
    wgpu::SurfaceConfiguration config = {};
    config.usage = wgpu::TextureUsage::RenderAttachment;
    config.device = this->GetDevice();
    config.width = this->Size[0];
    config.height = this->Size[1];
    config.presentMode = wgpu::PresentMode::Fifo;
    for (std::size_t i = 0; i < capabilities.formatCount; ++i)
    {
      config.format = capabilities.formats[i];
      // prefer BGRA8Unorm
      if (config.format == wgpu::TextureFormat::BGRA8Unorm)
      {
        break;
      }
    }
    this->Surface.Configure(&config);
    this->PreferredSurfaceTextureFormat = config.format;
    this->SurfaceConfiguredSize[0] = this->Size[0];
    this->SurfaceConfiguredSize[1] = this->Size[1];
  }
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::UnconfigureSurface()
{
  vtkDebugMacro(<< __func__);
  if (this->Surface == nullptr)
  {
    return;
  }
  this->Surface.Unconfigure();
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::CreateDepthStencilAttachment()
{
  vtkDebugMacro(<< __func__ << '(' << this->SurfaceConfiguredSize[0] << ','
                << this->SurfaceConfiguredSize[1] << ')');
  vtkWebGPUCheckUnconfigured(this);
  auto device = this->WGPUConfiguration->GetDevice();
  if (device == nullptr)
  {
    vtkErrorMacro(<< "Cannot create a depth stencil texture because WebGPU device is not ready!");
    return;
  }
  // TODO:
  // setup basic depth attachment
  // todo: verify device supports this depth and stencil format in feature set
  this->DepthStencilAttachment.HasStencil = true;

  const std::string textureLabel = "DepthStencil-" + this->GetObjectDescription();
  wgpu::TextureDescriptor textureDesc;
  textureDesc.label = textureLabel.c_str();
  textureDesc.dimension = wgpu::TextureDimension::e2D;
  textureDesc.size.width = this->SurfaceConfiguredSize[0];
  textureDesc.size.height = this->SurfaceConfiguredSize[1];
  textureDesc.size.depthOrArrayLayers = 1;
  textureDesc.sampleCount = 1;
  textureDesc.format = wgpu::TextureFormat::Depth24PlusStencil8;
  textureDesc.mipLevelCount = 1;
  // TextureBinding here because we may want to use the depth buffer as a vtkWebComputeRenderTexture
  // (which will be bound to a compute shader)
  textureDesc.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding;

  // view
  wgpu::TextureViewDescriptor textureViewDesc;
  textureViewDesc.dimension = wgpu::TextureViewDimension::e2D;
  textureViewDesc.format = textureDesc.format;
  textureViewDesc.baseMipLevel = 0;
  textureViewDesc.mipLevelCount = 1;
  textureViewDesc.baseArrayLayer = 0;
  textureViewDesc.arrayLayerCount = 1;
  // To be able to access the depth part of the depth-stencil buffer in a compute pipeline
  textureViewDesc.aspect = wgpu::TextureAspect::All;

  if (auto texture = this->WGPUConfiguration->CreateTexture(textureDesc))
  {
    this->DepthStencilAttachment.Texture = texture;
    if (auto view = this->WGPUConfiguration->CreateView(texture, textureViewDesc))
    {
      this->DepthStencilAttachment.View = view;
      this->DepthStencilAttachment.Format = textureDesc.format;
    }
    else
    {
      vtkErrorMacro(<< "Failed to create a texture view for depth stencil attachment using texture "
                    << texture.Get());
    }
  }
  else
  {
    vtkErrorMacro(<< "Failed to create a texture for depth stencil attachment using device "
                  << device.Get());
  }
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::DestroyDepthStencilAttachment()
{
  vtkDebugMacro(<< __func__);
  this->DepthStencilAttachment.View = nullptr;
  this->DepthStencilAttachment.Texture = nullptr;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::CreateOffscreenColorAttachment()
{
  vtkWebGPUCheckUnconfigured(this);
  auto device = this->WGPUConfiguration->GetDevice();
  if (device == nullptr)
  {
    vtkErrorMacro(
      << "Cannot create offscreen color attachments because WebGPU device is not ready!");
    return;
  }
  // must match swapchain's dimensions as we'll eventually sample from this.
  wgpu::Extent3D textureExtent;
  textureExtent.depthOrArrayLayers = 1;
  textureExtent.width = this->SurfaceConfiguredSize[0];
  textureExtent.height = this->SurfaceConfiguredSize[1];

  // Color attachment
  const std::string textureLabel = "OffscreenColor-" + this->GetObjectDescription();
  wgpu::TextureDescriptor textureDesc;
  textureDesc.label = textureLabel.c_str();
  textureDesc.size = textureExtent;
  textureDesc.mipLevelCount = 1;
  textureDesc.sampleCount = 1;
  textureDesc.dimension = wgpu::TextureDimension::e2D;
  textureDesc.format = this->GetPreferredSurfaceTextureFormat();
  textureDesc.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding |
    wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::StorageBinding;
  textureDesc.viewFormatCount = 0;
  textureDesc.viewFormats = nullptr;

  // view
  wgpu::TextureViewDescriptor textureViewDesc;
  textureViewDesc.dimension = wgpu::TextureViewDimension::e2D;
  textureViewDesc.format = textureDesc.format;
  textureViewDesc.baseMipLevel = 0;
  textureViewDesc.mipLevelCount = 1;
  textureViewDesc.baseArrayLayer = 0;
  textureViewDesc.arrayLayerCount = 1;

  if (auto texture = this->WGPUConfiguration->CreateTexture(textureDesc))
  {
    this->ColorAttachment.Texture = texture;
    if (auto view = this->WGPUConfiguration->CreateView(texture, textureViewDesc))
    {
      this->ColorAttachment.View = view;
      this->ColorAttachment.Format = textureDesc.format;
    }
    else
    {
      vtkErrorMacro(<< "Failed to create a texture view for color attachment using texture "
                    << texture.Get());
    }
  }
  else
  {
    vtkErrorMacro(<< "Failed to create a texture for color attachment using device "
                  << device.Get());
    return;
  }
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::DestroyOffscreenColorAttachment()
{
  this->ColorAttachment.View = nullptr;
  this->ColorAttachment.Texture = nullptr;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::CreateIdsAttachment()
{
  vtkWebGPUCheckUnconfigured(this);
  auto device = this->WGPUConfiguration->GetDevice();
  if (device == nullptr)
  {
    vtkErrorMacro(
      << "Cannot create offscreen color attachments because WebGPU device is not ready!");
    return;
  }
  // must match swapchain's dimensions as we'll eventually sample from this.
  wgpu::Extent3D textureExtent;
  textureExtent.depthOrArrayLayers = 1;
  textureExtent.width = this->SurfaceConfiguredSize[0];
  textureExtent.height = this->SurfaceConfiguredSize[1];

  // selector attachment for cell id
  const std::string textureLabel = "HardwareSelector-" + this->GetObjectDescription();
  wgpu::TextureDescriptor textureDesc;
  textureDesc.label = textureLabel.c_str();
  textureDesc.size = textureExtent;
  textureDesc.mipLevelCount = 1;
  textureDesc.sampleCount = 1;
  textureDesc.dimension = wgpu::TextureDimension::e2D;
  textureDesc.format = this->PreferredSelectorIdsTextureFormat;
  textureDesc.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::CopySrc;
  textureDesc.viewFormatCount = 0;
  textureDesc.viewFormats = nullptr;

  // view
  wgpu::TextureViewDescriptor textureViewDesc;
  textureViewDesc.dimension = wgpu::TextureViewDimension::e2D;
  textureViewDesc.format = textureDesc.format;
  textureViewDesc.baseMipLevel = 0;
  textureViewDesc.mipLevelCount = 1;
  textureViewDesc.baseArrayLayer = 0;
  textureViewDesc.arrayLayerCount = 1;

  if (auto texture = this->WGPUConfiguration->CreateTexture(textureDesc))
  {
    this->IdsAttachment.Texture = texture;
    if (auto view = this->WGPUConfiguration->CreateView(texture, textureViewDesc))
    {
      this->IdsAttachment.View = view;
      this->IdsAttachment.Format = textureDesc.format;
    }
    else
    {
      vtkErrorMacro(<< "Failed to create a texture view for color attachment using texture "
                    << texture.Get());
    }
  }
  else
  {
    vtkErrorMacro(<< "Failed to create a texture for color attachment using device "
                  << device.Get());
    return;
  }
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::DestroyIdsAttachment()
{
  this->IdsAttachment.View = nullptr;
  this->IdsAttachment.Texture = nullptr;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::CreateColorCopyPipeline()
{
  vtkWebGPUCheckUnconfigured(this);
  auto device = this->WGPUConfiguration->GetDevice();
  if (device == nullptr)
  {
    vtkErrorMacro(<< "Cannot create full-screen-quad graphics pipeline because WebGPU device "
                     "is not ready!");
    return;
  }
  wgpu::BindGroupLayout bgl = vtkWebGPUBindGroupLayoutInternals::MakeBindGroupLayout(device,
    {
      // clang-format off
      { 0, wgpu::ShaderStage::Fragment, wgpu::TextureSampleType::Float, wgpu::TextureViewDimension::e2D, /*multiSampled=*/false }
      // clang-format on
    },
    std::string("ColorCopy-") + this->GetObjectDescription());

  wgpu::PipelineLayout pipelineLayout =
    vtkWebGPUPipelineLayoutInternals::MakeBasicPipelineLayout(device, &bgl);
  pipelineLayout.SetLabel("FSQ Color Copy pipeline layout");

  this->ColorCopyRenderPipeline.BindGroup = vtkWebGPUBindGroupInternals::MakeBindGroup(device, bgl,
    {
      // clang-formt off
      { 0, this->ColorAttachment.View }
      // clang-format on
    },
    std::string("ColorCopy-") + this->GetObjectDescription());

  const char* shaderSource = R"(
    struct VertexOutput {
      @builtin(position) position: vec4<f32>,
    }

    @vertex
    fn vertexMain(@builtin(vertex_index) vertex_id: u32) -> VertexOutput {
      var output: VertexOutput;
      var coords: array<vec2<f32>, 4> = array<vec2<f32>, 4>(
        vec2<f32>(-1, -1), // bottom-left
        vec2<f32>(-1,  1), // top-left
        vec2<f32>( 1, -1), // bottom-right
        vec2<f32>( 1,  1)  // top-right
      );
      output.position = vec4<f32>(coords[vertex_id].xy, 1.0, 1.0);
      return output;
    }

    struct FragmentInput {
      @builtin(position) position: vec4<f32>,
    }

    @group(0) @binding(0) var fsqTexture: texture_2d<f32>;

    @fragment
    fn fragmentMain(fragment: FragmentInput) -> @location(0) vec4<f32> {
      let dims = textureDimensions(fsqTexture);
      let texCoord = vec2(u32(fragment.position.x), dims.y - 1u - u32(fragment.position.y));
      let color = textureLoad(fsqTexture, texCoord, 0);
      return vec4<f32>(color);
    }
  )";

  const std::string pipelineLabel = "ColorCopy-" + this->GetObjectDescription();
  vtkWebGPURenderPipelineDescriptorInternals pipelineDesc;
  pipelineDesc.label = pipelineLabel.c_str();
  pipelineDesc.layout = pipelineLayout;
  pipelineDesc.vertex.entryPoint = "vertexMain";
  pipelineDesc.vertex.bufferCount = 0;
  pipelineDesc.cFragment.entryPoint = "fragmentMain";
  pipelineDesc.cTargets[0].format = this->GetPreferredSurfaceTextureFormat();
  pipelineDesc.DisableDepthStencil();
  pipelineDesc.primitive.topology = wgpu::PrimitiveTopology::TriangleStrip;

  const auto pipelineKey = this->WGPUPipelineCache->GetPipelineKey(&pipelineDesc, shaderSource);
  if (this->ColorCopyRenderPipeline.Key != pipelineKey)
  {
    this->WGPUPipelineCache->CreateRenderPipeline(&pipelineDesc, this, shaderSource);
    this->ColorCopyRenderPipeline.Key = pipelineKey;
  }
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::RecreateComputeRenderTextures()
{
  vtkWebGPUCheckUnconfigured(this);

  for (const auto& renderTexture : this->ComputeRenderTextures)
  {
    int* dims = this->GetSize();

    // Updating the size of the texture
    renderTexture->SetSize(dims[0], dims[1]);

    // Updating the WebGPU texture used by the render texture since it has been recreated by the
    // window resize
    switch (renderTexture->GetType())
    {
      case vtkWebGPUComputeRenderTexture::RenderTextureType::DEPTH_BUFFER:
        renderTexture->SetWebGPUTexture(this->DepthStencilAttachment.Texture);
        break;

      case vtkWebGPUComputeRenderTexture::RenderTextureType::COLOR_BUFFER:
        renderTexture->SetWebGPUTexture(this->ColorAttachment.Texture);
        break;

      default:
        vtkLog(ERROR,
          "Unhandled ComputeRenderTexture type in "
          "vtkWebGPURenderWindow::RecreateComputeRenderTextures. This is an internal error.");
        break;
    }

    vtkWeakPointer<vtkWebGPUComputePass> associatedComputePass =
      renderTexture->GetAssociatedComputePass();

    if (associatedComputePass == nullptr)
    {
      vtkLog(WARNING,
        "The render texture with label \"" << renderTexture->GetLabel()
                                           << "\" didn't have an associated compute pass. Did "
                                              "you forget to add the render texture "
                                              "to a compute pass?");

      continue;
    }
    associatedComputePass->Internals->RecreateRenderTexture(renderTexture);
  }
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::DestroyColorCopyPipeline()
{
  this->ColorCopyRenderPipeline.BindGroup = nullptr;
  this->WGPUPipelineCache->DestroyRenderPipeline(this->ColorCopyRenderPipeline.Key);
  this->ColorCopyRenderPipeline.Key.clear();
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::PostRenderComputePipelines()
{
  for (auto renderer : vtk::Range(this->Renderers))
  {
    vtkWebGPURenderer* wgpuRenderer = vtkWebGPURenderer::SafeDownCast(renderer);
    if (wgpuRenderer == nullptr)
    {
      // Probably not a wgpuRenderer

      continue;
    }

    wgpuRenderer->PostRenderComputePipelines();
  }
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::PostRasterizationRender()
{
  for (auto renderer : vtk::Range(this->Renderers))
  {
    vtkWebGPURenderer* wgpuRenderer = vtkWebGPURenderer::SafeDownCast(renderer);
    if (wgpuRenderer == nullptr)
    {
      // Probably not a wgpuRenderer

      continue;
    }

    wgpuRenderer->PostRasterizationRender();
  }
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::ReadTextureFromGPU(wgpu::Texture& wgpuTexture,
  wgpu::TextureFormat format, std::size_t mipLevel, wgpu::TextureAspect aspect,
  wgpu::Origin3D offsets, wgpu::Extent3D extents, TextureMapCallback callback, void* userData)
{
  int bytesPerPixel = 0;
  switch (format)
  {
    case wgpu::TextureFormat::RGBA8Unorm:
    case wgpu::TextureFormat::BGRA8Unorm:
      bytesPerPixel = 4;
      break;
    case wgpu::TextureFormat::RGBA32Uint:
      bytesPerPixel = 16;
      break;
    case wgpu::TextureFormat::Depth24Plus:
      bytesPerPixel = 3;
      break;
    case wgpu::TextureFormat::Depth24PlusStencil8:
      bytesPerPixel = 4;
      break;
    case wgpu::TextureFormat::R32Uint:
      bytesPerPixel = 4;
      break;
    default:
      vtkErrorMacro(<< "Unhandled texture format in vtkWebGPUTexture::GetBytesPerPixel: "
                    << int(format));
  }

  // Bytes needs to be a multiple of 256
  vtkIdType bytesPerRow = vtkWebGPUConfiguration::Align(extents.width * bytesPerPixel, 256);

  // Creating the buffer that will hold the data of the texture
  wgpu::BufferDescriptor bufferDescriptor;
  bufferDescriptor.label = "Buffer descriptor for mapping texture";
  bufferDescriptor.mappedAtCreation = false;
  bufferDescriptor.nextInChain = nullptr;
  bufferDescriptor.size = bytesPerRow * extents.height;
  bufferDescriptor.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead;

  wgpu::Buffer buffer = this->WGPUConfiguration->CreateBuffer(bufferDescriptor);

  // Parameters for copying the texture
  wgpu::TexelCopyTextureInfo texelCopyTexture;
  texelCopyTexture.mipLevel = mipLevel;
  texelCopyTexture.origin = offsets;
  texelCopyTexture.texture = wgpuTexture;
  texelCopyTexture.aspect = aspect;

  // Parameters for copying the buffer
  unsigned int mipLevelWidth = std::floor(extents.width / std::pow(2, mipLevel));
  unsigned int mipLevelHeight = std::floor(extents.height / std::pow(2, mipLevel));
  wgpu::TexelCopyBufferInfo texelCopyBuffer;
  texelCopyBuffer.buffer = buffer;
  texelCopyBuffer.layout.offset = 0;
  texelCopyBuffer.layout.rowsPerImage = mipLevelHeight;
  texelCopyBuffer.layout.bytesPerRow = bytesPerRow;

  // Copying the texture to the buffer
  wgpu::CommandEncoder commandEncoder = this->WGPUConfiguration->GetDevice().CreateCommandEncoder();
  wgpu::Extent3D copySize = { mipLevelWidth, mipLevelHeight, extents.depthOrArrayLayers };
  commandEncoder.CopyTextureToBuffer(&texelCopyTexture, &texelCopyBuffer, &copySize);

  // Submitting the comand
  wgpu::CommandBuffer commandBuffer = commandEncoder.Finish();
  this->WGPUConfiguration->GetDevice().GetQueue().Submit(1, &commandBuffer);

  auto bufferMapCallback =
    [](wgpu::MapAsyncStatus status, wgpu::StringView message, InternalMapTextureAsyncData* mapData)
  {
    if (status == wgpu::MapAsyncStatus::Success)
    {
      const void* mappedRange = mapData->buffer.GetConstMappedRange(0, mapData->byteSize);
      mapData->userCallback(mappedRange, mapData->bytesPerRow, mapData->userData);
      mapData->buffer.Unmap();
    }
    else
    {
      vtkLog(WARNING, << "Failed to map [Texture \'"
                      << (mapData->bufferLabel.empty() ? "(nolabel)" : mapData->bufferLabel)
                      << "\'] with error=" << static_cast<std::uint32_t>(status) << ". "
                      << vtkWebGPUHelpers::StringViewToStdString(message));
    }
#if defined(__EMSCRIPTEN__)
    wgpuBufferRelease(mapData->buffer.Get());
#endif
    // Freeing the callbackData structure as it was dynamically allocated
    delete mapData;
  };

  // Now mapping the buffer that contains the texture data to the CPU
  // Dynamically allocating here because we callbackData to stay alive even after exiting this
  // function (because buffer.MapAsync is asynchronous). buffer.MapAsync() also takes a raw pointer
  // so we cannot use smart pointers here
  InternalMapTextureAsyncData* callbackData = new InternalMapTextureAsyncData();
  callbackData->buffer = buffer;
  callbackData->bufferLabel = this->GetObjectDescription() + " ReadTextureFromGPU map buffer";
  callbackData->byteSize = bufferDescriptor.size;
  callbackData->bytesPerRow = bytesPerRow;
  callbackData->userCallback = callback;
  callbackData->userData = userData;
#if defined(__EMSCRIPTEN__)
  // keep buffer alive for map.
  // See https://issues.chromium.org/issues/399131918
  wgpuBufferAddRef(callbackData->buffer.Get());
#endif
  callbackData->buffer.MapAsync(wgpu::MapMode::Read, 0, bufferDescriptor.size,
    wgpu::CallbackMode::AllowProcessEvents, bufferMapCallback, callbackData);
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::ReadTextureFromGPU(wgpu::Texture& wgpuTexture,
  wgpu::TextureFormat format, std::size_t mipLevel, wgpu::TextureAspect aspect,
  vtkWebGPURenderWindow::TextureMapCallback callback, void* userData)
{
  return this->ReadTextureFromGPU(wgpuTexture, format, mipLevel, aspect, wgpu::Origin3D{ 0, 0, 0 },
    wgpu::Extent3D{
      wgpuTexture.GetWidth(), wgpuTexture.GetHeight(), wgpuTexture.GetDepthOrArrayLayers() },
    callback, userData);
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::GetIdsData(int x1, int y1, int x2, int y2, vtkTypeUInt32* values)
{
  int inNumberOfComponents = 4;

  struct CallbackData
  {
    vtkTypeUInt32* outputValues;
    int xMin;
    int xMax;
    int yMin;
    int yMax;
  };

  auto* callbackData = new CallbackData();
  callbackData->outputValues = values;
  callbackData->xMin = x1;
  callbackData->xMax = x2;
  callbackData->yMin = y1;
  callbackData->yMax = y2;

  auto onTextureMapped = [inNumberOfComponents](
                           const void* mappedData, int bytesPerRow, void* userData)
  {
    CallbackData* callbackDataPtr = reinterpret_cast<CallbackData*>(userData);
    auto* outputValues = callbackDataPtr->outputValues;
    const vtkTypeUInt32* mappedDataAsUInt32 = reinterpret_cast<const vtkTypeUInt32*>(mappedData);

    // Copying the RGBA channels of each pixel
    vtkIdType dstIdx = 0;
    for (int y = callbackDataPtr->yMin; y <= callbackDataPtr->yMax; y++)
    {
      for (int x = callbackDataPtr->xMin; x <= callbackDataPtr->xMax; x++)
      {
        // Dividing by inNumberOfComponents * sizeof(SampleType) here because we want to multiply Y
        // by the 'width' which is in number of pixels (ex: for RGBA=4, for RGB=3)
        const int mappedIndex =
          x + y * (bytesPerRow / (inNumberOfComponents * sizeof(vtkTypeUInt32)));
        outputValues[dstIdx++] = mappedDataAsUInt32[mappedIndex * inNumberOfComponents + 0];
        outputValues[dstIdx++] = mappedDataAsUInt32[mappedIndex * inNumberOfComponents + 1];
        outputValues[dstIdx++] = mappedDataAsUInt32[mappedIndex * inNumberOfComponents + 2];
        outputValues[dstIdx++] = mappedDataAsUInt32[mappedIndex * inNumberOfComponents + 3];
      }
    }
    delete callbackDataPtr;
  };

  this->ReadTextureFromGPU(this->IdsAttachment.Texture, this->IdsAttachment.Format, 0,
    wgpu::TextureAspect::All, onTextureMapped, reinterpret_cast<void*>(callbackData));
  this->WaitForCompletion();
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::GetIdsData(int x1, int y1, int x2, int y2, vtkTypeUInt32Array* data)
{
  int width = x2 - x1 + 1;
  int height = y2 - y1 + 1;
  const int outNumberOfComponents = 4;
  data->SetNumberOfComponents(outNumberOfComponents);
  data->SetNumberOfTuples(width * height);
  data->SetComponentName(0, "CellId");
  data->SetComponentName(1, "PropId");
  data->SetComponentName(2, "CompositeId");
  data->SetComponentName(3, "ProcessId");
  this->GetIdsData(x1, y1, x2, y2, data->GetPointer(0));
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::RenderOffscreenTexture()
{
  vtkWebGPUCheckUnconfigured(this);
  if (this->Surface == nullptr)
  {
    vtkErrorMacro(<< "Cannot render offscreen texture because surface is null!");
    return;
  }
  // prepare the offscreen texture for presentation.
  wgpu::SurfaceTexture surfaceTexture;
  this->Surface.GetCurrentTexture(&surfaceTexture);
  switch (surfaceTexture.status)
  {
    case wgpu::SurfaceGetCurrentTextureStatus::Timeout:
      vtkErrorMacro(
        << "Cannot render offscreen texture because SurfaceGetCurrentTextureStatus=Timeout");
      return;
    case wgpu::SurfaceGetCurrentTextureStatus::Outdated:
      vtkErrorMacro(
        << "Cannot render offscreen texture because SurfaceGetCurrentTextureStatus=Outdated");
      return;
    case wgpu::SurfaceGetCurrentTextureStatus::Lost:
      vtkErrorMacro(
        << "Cannot render offscreen texture because SurfaceGetCurrentTextureStatus=Lost");
      return;
    case wgpu::SurfaceGetCurrentTextureStatus::Error:
      vtkErrorMacro(
        << "Cannot render offscreen texture because SurfaceGetCurrentTextureStatus=Error");
      return;
    case wgpu::SurfaceGetCurrentTextureStatus::SuccessSuboptimal:
      // TODO: Warn exactly once per Initialize/Finalize duration.
      vtkDebugMacro(<< "SurfaceTexture format is suboptimal!");
      break;
    case wgpu::SurfaceGetCurrentTextureStatus::SuccessOptimal:
    default:
      break;
  }
  if (surfaceTexture.texture == nullptr)
  {
    vtkErrorMacro(<< "Cannot render offscreen texture because SurfaceTexture is null!");
    return;
  }
  if (this->ColorAttachment.Texture == nullptr)
  {
    vtkErrorMacro(<< "Cannot render offscreen texture because the source color attachment "
                     "texture is null!");
    return;
  }
  if (this->ColorCopyRenderPipeline.Key.empty())
  {
    vtkErrorMacro(<< "Cannot render offscreen texture because the full-screen-quad render "
                     "pipeline is not ready!");
    return;
  }
  if (this->ColorCopyRenderPipeline.BindGroup == nullptr)
  {
    vtkErrorMacro(<< "Cannot render offscreen texture because the full-screen-quad render bind "
                     "group is null!");
    return;
  }
  if (this->CommandEncoder == nullptr)
  {
    vtkErrorMacro(<< "Cannot render offscreen texture because the command encoder is null!");
    return;
  }

  vtkWebGPURenderPassDescriptorInternals renderPassDescriptor(
    { surfaceTexture.texture.CreateView() });
  renderPassDescriptor.label = "Render offscreen texture";

  for (auto& colorAttachment : renderPassDescriptor.ColorAttachments)
  {
    colorAttachment.clearValue.r = 0.0;
    colorAttachment.clearValue.g = 0.0;
    colorAttachment.clearValue.b = 0.0;
    colorAttachment.clearValue.a = 1.0f;
  }
  if (auto encoder = this->NewRenderPass(renderPassDescriptor))
  {
    encoder.SetLabel("Encode offscreen texture render commands");
    encoder.SetViewport(
      0, 0, this->SurfaceConfiguredSize[0], this->SurfaceConfiguredSize[1], 0.0, 1.0);
    encoder.SetScissorRect(0, 0, this->SurfaceConfiguredSize[0], this->SurfaceConfiguredSize[1]);
    // set fsq pipeline
    {
      vtkScopedEncoderDebugGroup(encoder, "FSQ Render");
      const auto pipeline =
        this->WGPUPipelineCache->GetRenderPipeline(this->ColorCopyRenderPipeline.Key);
      encoder.SetPipeline(pipeline);
      // bind fsq group
      encoder.SetBindGroup(0, this->ColorCopyRenderPipeline.BindGroup);
      // draw triangle strip
      encoder.Draw(4);
    }
    encoder.End();
  }
  else
  {
    vtkErrorMacro(<< "Cannot render swapchain contents into offscreen texture because this render "
                     "window failed to build a new render pass!");
    return;
  }
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::FlushCommandBuffers(vtkTypeUInt32 count, wgpu::CommandBuffer* buffers)
{
  vtkDebugMacro(<< __func__ << "count=" << count);
  vtkWebGPUCheckUnconfigured(this);
  auto device = this->WGPUConfiguration->GetDevice();
  if (device == nullptr)
  {
    vtkErrorMacro(<< "Cannot flush command buffers because WebGPU device is not ready!");
    return;
  }
  if (count > 0 && buffers == nullptr)
  {
    vtkErrorMacro(<< "Cannot flush command buffers because buffers is null even though count ("
                  << count << ") > 0");
    return;
  }
  if (auto queue = device.GetQueue())
  {
    queue.Submit(count, buffers);
  }
  else
  {
    vtkErrorMacro(<< "Cannot flush command buffers because this render window failed to obtain a "
                     "queue from device "
                  << device.Get());
    return;
  }
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::Start()
{
  int* size = this->GetSize();
  vtkDebugMacro(<< __func__ << '(' << size[0] << ',' << size[1] << ')');

  this->Size[0] = (size[0] > 0 ? size[0] : 300);
  this->Size[1] = (size[1] > 0 ? size[1] : 300);

  vtkDebugMacro(<< __func__ << " Initialized=" << this->Initialized);

  if (!this->Initialized)
  {
    this->Initialize();
  }

  if (this->Size[0] != this->SurfaceConfiguredSize[0] ||
    this->Size[1] != this->SurfaceConfiguredSize[1])
  {
    // Window's size changed, need to recreate the swap chain, textures, ...
    this->DestroyColorCopyPipeline();
    this->DestroyDepthStencilAttachment();
    this->DestroyIdsAttachment();
    this->DestroyOffscreenColorAttachment();
    this->UnconfigureSurface();
    this->ConfigureSurface();
    this->CreateOffscreenColorAttachment();
    this->CreateDepthStencilAttachment();
    this->CreateIdsAttachment();
    this->CreateColorCopyPipeline();
    this->RecreateComputeRenderTextures();
  }

  this->CreateCommandEncoder();
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::Frame()
{
  vtkDebugMacro(<< __func__);
  vtkWebGPUCheckUnconfigured(this);
  if (this->Surface == nullptr)
  {
    vtkErrorMacro(<< "Cannot render frame because the surface is null!");
    return;
  }
  this->Superclass::Frame();

  wgpu::CommandBufferDescriptor cmdBufDesc = {};
  wgpu::CommandBuffer cmdBuffer;
  // Flushing the commands for the props to be rendered
  if (this->CommandEncoder != nullptr)
  {
    cmdBuffer = this->CommandEncoder.Finish(&cmdBufDesc);

    this->CommandEncoder = nullptr;
    this->FlushCommandBuffers(1, &cmdBuffer);
  }

  this->PostRenderComputePipelines();
  this->PostRasterizationRender();

  // New command encoder for the FSQ pass
  this->CreateCommandEncoder();
  this->RenderOffscreenTexture();

  // Flushing the FSQ render pass
  cmdBuffer = this->CommandEncoder.Finish(&cmdBufDesc);

  this->CommandEncoder = nullptr;
  this->FlushCommandBuffers(1, &cmdBuffer);

  // On web, html5 `requestAnimateFrame` takes care of presentation.
#ifndef __EMSCRIPTEN__
  this->Surface.Present();
#endif

  // Clean up staging buffer for SetPixelData.
  if (this->StagingPixelData.Buffer.Get() != nullptr)
  {
    this->StagingPixelData.Buffer.Destroy();
    this->StagingPixelData.Buffer = nullptr;
  }

  this->ReleaseRGBAPixelData(nullptr);

#ifndef NDEBUG
  // This lets the implementation execute all callbacks so that validation errors are output in
  // the console.
  this->WGPUConfiguration->ProcessEvents();
#endif
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::End()
{
  vtkDebugMacro(<< __func__);
  vtkWebGPUCheckUnconfigured(this);

  // If user called SetPixelData or it's variant, source our offscreen texture from that data.
  if (this->StagingPixelData.Buffer.Get() != nullptr)
  {
    if (this->CommandEncoder == nullptr)
    {
      vtkErrorMacro(<< "Cannot copy staging RGBA pixel buffer provided by SetPixelData into "
                       "texture because the command encoder is null!");
      return;
    }
    // copy data to texture.
    wgpu::TexelCopyTextureInfo destination;
    destination.texture = this->ColorAttachment.Texture;
    destination.mipLevel = 0;
    destination.origin = this->StagingPixelData.Origin;
    destination.aspect = wgpu::TextureAspect::All;

    wgpu::TexelCopyBufferInfo source;
    source.buffer = this->StagingPixelData.Buffer;
    source.layout = this->StagingPixelData.Layout;
    this->Start();
    vtkScopedEncoderDebugGroup(this->CommandEncoder, "Copy staging RGBA pixel buffer to texture");
    this->CommandEncoder.CopyBufferToTexture(
      &source, &destination, &(this->StagingPixelData.Extent));
  }
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::StereoMidpoint() {}

//------------------------------------------------------------------------------
const char* vtkWebGPURenderWindow::GetRenderingBackend()
{
  return "";
}

//------------------------------------------------------------------------------
unsigned char* vtkWebGPURenderWindow::GetPixelData(
  int x1, int y1, int x2, int y2, int front, int right)
{
  (void)front;
  (void)right;

  int width = x2 - x1 + 1;
  int height = y2 - y1 + 1;
  const int outNumberOfComponents = 3;
  int inNumberOfComponents = 0;

  struct CallbackData
  {
    unsigned char* outputValues;
    int xMin;
    int xMax;
    int yMin;
    int yMax;
    int componentMap[3] = {};
  };
  auto* pixels = new unsigned char[width * height * outNumberOfComponents];
  // Dynamically allocating here because we callbackData to stay alive even after exiting this
  // function.
  auto* callbackData = new CallbackData();
  callbackData->outputValues = pixels;
  callbackData->xMin = x1;
  callbackData->xMax = x2;
  callbackData->yMin = y1;
  callbackData->yMax = y2;
  if (this->ColorAttachment.Format == wgpu::TextureFormat::BGRA8Unorm)
  {
    callbackData->componentMap[0] = 2;
    callbackData->componentMap[1] = 1;
    callbackData->componentMap[2] = 0;
    inNumberOfComponents = 4;
  }
  else if (this->ColorAttachment.Format == wgpu::TextureFormat::RGBA8Unorm)
  {
    callbackData->componentMap[0] = 0;
    callbackData->componentMap[1] = 1;
    callbackData->componentMap[2] = 2;
    inNumberOfComponents = 4;
  }
  else
  {
    // TODO: Handle other formats.
    vtkErrorMacro(<< "Unsupported offscreen texture format!");
    delete callbackData;
    return pixels;
  }

  auto onTextureMapped = [inNumberOfComponents](
                           const void* mappedData, int bytesPerRow, void* userData)
  {
    CallbackData* callbackDataPtr = reinterpret_cast<CallbackData*>(userData);
    unsigned char* outputValues = callbackDataPtr->outputValues;
    const unsigned char* mappedDataChar = reinterpret_cast<const unsigned char*>(mappedData);

    // Copying the RGB channels of each pixel
    vtkIdType dstIdx = 0;
    for (int y = callbackDataPtr->yMin; y <= callbackDataPtr->yMax; y++)
    {
      for (int x = callbackDataPtr->xMin; x <= callbackDataPtr->xMax; x++)
      {
        // Dividing by inNumberOfComponents * sizeof(SampleType) here because we want to multiply Y
        // by the 'width' which is in number of pixels (ex: for RGBA=4, for RGB=3)
        const int mappedIndex =
          x + y * (bytesPerRow / (inNumberOfComponents * sizeof(unsigned char)));
        // Copying the RGB channels of each pixel
        for (auto& comp : callbackDataPtr->componentMap)
        {
          outputValues[dstIdx++] = mappedDataChar[mappedIndex * inNumberOfComponents + comp];
        }
      }
    }
    delete callbackDataPtr;
  };

  this->ReadTextureFromGPU(this->ColorAttachment.Texture, this->ColorAttachment.Format, 0,
    wgpu::TextureAspect::All, onTextureMapped, reinterpret_cast<void*>(callbackData));
  this->WaitForCompletion();
  return pixels;
}

//------------------------------------------------------------------------------
int vtkWebGPURenderWindow::GetPixelData(
  int x1, int y1, int x2, int y2, int front, vtkUnsignedCharArray* data, int right)
{
  int width = x2 - x1 + 1;
  int height = y2 - y1 + 1;
  const int numberOfComponents = 3;
  data->SetNumberOfComponents(numberOfComponents);
  data->SetNumberOfTuples(width * height);
  unsigned char* pixels = this->GetPixelData(x1, y1, x2, y2, front, right);
  // take ownership of pixels
  data->SetArray(pixels, width * height * numberOfComponents, 0);
  return data->GetNumberOfValues();
}

//------------------------------------------------------------------------------
int vtkWebGPURenderWindow::SetPixelData(
  int x, int y, int x2, int y2, unsigned char* data, int front, int right)
{
  vtkWebGPUCheckUnconfiguredWithReturn(this, 0);
  auto device = this->WGPUConfiguration->GetDevice();
  if (device == nullptr)
  {
    vtkErrorMacro(<< "Cannot set pixel data because WebGPU device is not ready!");
    return 0;
  }
  (void)front;
  (void)right;
  const int nComp = 3;
  int width = (x2 - x) + 1;
  int height = (y2 - y) + 1;
  int bytesPerRow = vtkWebGPUConfiguration::Align(width * nComp, 256);
  int size = bytesPerRow * height;

  const std::string label = "StagingRGBPixelData-" + this->GetObjectDescription();
  wgpu::BufferDescriptor desc;
  desc.mappedAtCreation = true;
  desc.label = label.c_str();
  desc.size = size;
  desc.usage = wgpu::BufferUsage::CopySrc;

  this->StagingPixelData.Buffer = this->WGPUConfiguration->CreateBuffer(desc);
  if (this->StagingPixelData.Buffer == nullptr)
  {
    vtkErrorMacro(<< "Failed to create buffer for staging pixel data using device "
                  << device.Get());
    return 0;
  }
  auto mapped =
    reinterpret_cast<unsigned char*>(this->StagingPixelData.Buffer.GetMappedRange(0, size));
  if (mapped == nullptr)
  {
    vtkErrorMacro(<< "Failed to map staging pixel data!");
    return 0;
  }
  unsigned long dstIdx = 0;
  unsigned long srcIdx = 0;
  const unsigned long nPad = bytesPerRow - width * nComp;
  int componentMap[3] = {};
  if (this->ColorAttachment.Format == wgpu::TextureFormat::BGRA8Unorm)
  {
    componentMap[0] = 2;
    componentMap[1] = 1;
    componentMap[2] = 0;
  }
  else if (this->ColorAttachment.Format == wgpu::TextureFormat::RGBA8Unorm)
  {
    componentMap[0] = 0;
    componentMap[1] = 1;
    componentMap[2] = 2;
  }
  for (int j = 0; j < height; ++j)
  {
    for (int i = 0; i < width; ++i)
    {
      for (const auto& comp : componentMap)
      {
        mapped[dstIdx + comp] = data[srcIdx++];
      }
      mapped[dstIdx + nComp] = 255;
      dstIdx += nComp;
    }
    dstIdx += nPad;
  }

  this->StagingPixelData.Layout.bytesPerRow = bytesPerRow;
  this->StagingPixelData.Layout.offset = 0;
  this->StagingPixelData.Layout.rowsPerImage = height;

  this->StagingPixelData.Extent.width = width;
  this->StagingPixelData.Extent.height = height;
  this->StagingPixelData.Extent.depthOrArrayLayers = 1;

  this->StagingPixelData.Origin.x = x;
  this->StagingPixelData.Origin.y = y;
  this->StagingPixelData.Origin.z = 0;

  return size;
}

//------------------------------------------------------------------------------
int vtkWebGPURenderWindow::SetPixelData(
  int x, int y, int x2, int y2, vtkUnsignedCharArray* data, int front, int right)
{
  return this->SetPixelData(x, y, x2, y2, data->GetPointer(0), front, right);
}

//------------------------------------------------------------------------------
float* vtkWebGPURenderWindow::GetRGBAPixelData(
  int x1, int y1, int x2, int y2, int front, int right /*=0*/)
{
  (void)front;
  (void)right;

  int width = x2 - x1 + 1;
  int height = y2 - y1 + 1;
  const int outNumberOfComponents = 4;
  int inNumberOfComponents = 0;

  struct CallbackData
  {
    float* outputValues;
    int xMin;
    int xMax;
    int yMin;
    int yMax;
    int componentMap[4] = {};
  };
  auto* pixels = new float[width * height * outNumberOfComponents];
  auto* callbackData = new CallbackData();
  callbackData->outputValues = pixels;
  callbackData->xMin = x1;
  callbackData->xMax = x2;
  callbackData->yMin = y1;
  callbackData->yMax = y2;
  if (this->ColorAttachment.Format == wgpu::TextureFormat::BGRA8Unorm)
  {
    callbackData->componentMap[0] = 2;
    callbackData->componentMap[1] = 1;
    callbackData->componentMap[2] = 0;
    callbackData->componentMap[3] = 3;
    inNumberOfComponents = 4;
  }
  else if (this->ColorAttachment.Format == wgpu::TextureFormat::RGBA8Unorm)
  {
    callbackData->componentMap[0] = 0;
    callbackData->componentMap[1] = 1;
    callbackData->componentMap[2] = 2;
    callbackData->componentMap[3] = 3;
    inNumberOfComponents = 4;
  }
  else
  {
    // TODO: Handle other formats.
    vtkErrorMacro(<< "Unsupported offscreen texture format!");
    delete callbackData;
    return pixels;
  }

  auto onTextureMapped = [inNumberOfComponents](
                           const void* mappedData, int bytesPerRow, void* userData)
  {
    CallbackData* callbackDataPtr = reinterpret_cast<CallbackData*>(userData);
    float* outputValues = callbackDataPtr->outputValues;
    const unsigned char* mappedDataChar = reinterpret_cast<const unsigned char*>(mappedData);

    vtkIdType dstIdx = 0;
    for (int y = callbackDataPtr->yMin; y <= callbackDataPtr->yMax; y++)
    {
      for (int x = callbackDataPtr->xMin; x <= callbackDataPtr->xMax; x++)
      {
        // Dividing by inNumberOfComponents * sizeof(SampleType) here because we want to multiply Y
        // by the 'width' which is in number of pixels (ex: for RGBA=4, for RGB=3)
        const int mappedIndex =
          x + y * (bytesPerRow / (inNumberOfComponents * sizeof(unsigned char)));
        // Copying the RGBA channels of each pixel
        for (auto& comp : callbackDataPtr->componentMap)
        {
          outputValues[dstIdx++] =
            mappedDataChar[mappedIndex * inNumberOfComponents + comp] / 255.0f;
        }
      }
    }
    delete callbackDataPtr;
  };

  this->ReadTextureFromGPU(this->ColorAttachment.Texture, this->ColorAttachment.Format, 0,
    wgpu::TextureAspect::All, onTextureMapped, reinterpret_cast<void*>(callbackData));
  this->WaitForCompletion();
  return pixels;
}

//------------------------------------------------------------------------------
int vtkWebGPURenderWindow::GetRGBAPixelData(
  int x1, int y1, int x2, int y2, int front, vtkFloatArray* data, int right /*=0*/)
{
  int width = x2 - x1 + 1;
  int height = y2 - y1 + 1;
  const int numberOfComponents = 4;
  data->SetNumberOfComponents(numberOfComponents);
  data->SetNumberOfTuples(width * height);
  float* pixels = this->GetRGBAPixelData(x1, y1, x2, y2, front, right);
  // take ownership of pixels
  data->SetArray(pixels, width * height * numberOfComponents, 0);
  return data->GetNumberOfValues();
}

//------------------------------------------------------------------------------
int vtkWebGPURenderWindow::SetRGBAPixelData(
  int x, int y, int x2, int y2, float* data, int front, int blend /*=0*/, int right /*=0*/)
{
  vtkWebGPUCheckUnconfiguredWithReturn(this, 0);
  auto device = this->WGPUConfiguration->GetDevice();
  if (device == nullptr)
  {
    vtkErrorMacro(<< "Cannot set RGBA pixel data because WebGPU device is not ready!");
    return 0;
  }
  (void)front;
  (void)blend;
  (void)right;
  const int nComp = 4;
  int width = (x2 - x) + 1;
  int height = (y2 - y) + 1;
  int bytesPerRow = vtkWebGPUConfiguration::Align(width * nComp, 256);
  int size = bytesPerRow * height;

  const std::string label = "StagingRGBAPixelData-" + this->GetObjectDescription();
  wgpu::BufferDescriptor desc;
  desc.mappedAtCreation = true;
  desc.label = label.c_str();
  desc.size = size;
  desc.usage = wgpu::BufferUsage::CopySrc;

  this->StagingPixelData.Buffer = this->WGPUConfiguration->CreateBuffer(desc);
  if (this->StagingPixelData.Buffer == nullptr)
  {
    vtkErrorMacro(<< "Failed to create buffer for staging pixel data using device "
                  << device.Get());
    return 0;
  }
  auto mapped =
    reinterpret_cast<unsigned char*>(this->StagingPixelData.Buffer.GetMappedRange(0, size));
  if (mapped == nullptr)
  {
    vtkErrorMacro(<< "Failed to map staging pixel data!");
    return 0;
  }
  unsigned long dstIdx = 0;
  unsigned long srcIdx = 0;
  const unsigned long nPad = bytesPerRow - width * nComp;
  int componentMap[4] = {};
  if (this->ColorAttachment.Format == wgpu::TextureFormat::BGRA8Unorm)
  {
    componentMap[0] = 2;
    componentMap[1] = 1;
    componentMap[2] = 0;
    componentMap[3] = 3;
  }
  else if (this->ColorAttachment.Format == wgpu::TextureFormat::RGBA8Unorm)
  {
    componentMap[0] = 0;
    componentMap[1] = 1;
    componentMap[2] = 2;
    componentMap[3] = 3;
  }
  for (int j = 0; j < height; ++j)
  {
    for (int i = 0; i < width; ++i)
    {
      for (const auto& comp : componentMap)
      {
        mapped[dstIdx + comp] = static_cast<unsigned char>(data[srcIdx++] * 255.0);
      }
      dstIdx += nComp;
    }
    dstIdx += nPad;
  }
  this->StagingPixelData.Buffer.Unmap();

  this->StagingPixelData.Layout.bytesPerRow = bytesPerRow;
  this->StagingPixelData.Layout.offset = 0;
  this->StagingPixelData.Layout.rowsPerImage = height;

  this->StagingPixelData.Extent.width = width;
  this->StagingPixelData.Extent.height = height;
  this->StagingPixelData.Extent.depthOrArrayLayers = 1;

  this->StagingPixelData.Origin.x = x;
  this->StagingPixelData.Origin.y = y;
  this->StagingPixelData.Origin.z = 0;

  this->Start();
  this->End();
  this->Frame();
  return size;
}

//------------------------------------------------------------------------------
int vtkWebGPURenderWindow::SetRGBAPixelData(
  int x, int y, int x2, int y2, vtkFloatArray* data, int front, int blend /*=0*/, int right /*=0*/)
{
  return this->SetRGBAPixelData(x, y, x2, y2, data->GetPointer(0), front, blend, right);
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::ReleaseRGBAPixelData(float* data)
{
  delete[] data;
}

unsigned char* vtkWebGPURenderWindow::GetRGBACharPixelData(
  int x1, int y1, int x2, int y2, int front, int right /*=0*/)
{
  (void)front;
  (void)right;

  int width = x2 - x1 + 1;
  int height = y2 - y1 + 1;
  const int outNumberOfComponents = 4;
  int inNumberOfComponents = 0;

  struct CallbackData
  {
    unsigned char* outputValues;
    int xMin;
    int xMax;
    int yMin;
    int yMax;
    int componentMap[4] = {};
  };
  auto* pixels = new unsigned char[width * height * outNumberOfComponents];
  auto* callbackData = new CallbackData();
  callbackData->outputValues = pixels;
  callbackData->xMin = x1;
  callbackData->xMax = x2;
  callbackData->yMin = y1;
  callbackData->yMax = y2;
  if (this->ColorAttachment.Format == wgpu::TextureFormat::BGRA8Unorm)
  {
    callbackData->componentMap[0] = 2;
    callbackData->componentMap[1] = 1;
    callbackData->componentMap[2] = 0;
    callbackData->componentMap[3] = 3;
    inNumberOfComponents = 4;
  }
  else if (this->ColorAttachment.Format == wgpu::TextureFormat::RGBA8Unorm)
  {
    callbackData->componentMap[0] = 0;
    callbackData->componentMap[1] = 1;
    callbackData->componentMap[2] = 2;
    callbackData->componentMap[3] = 3;
    inNumberOfComponents = 4;
  }
  else
  {
    // TODO: Handle other formats.
    vtkErrorMacro(<< "Unsupported offscreen texture format!");
    delete callbackData;
    return pixels;
  }

  auto onTextureMapped = [inNumberOfComponents](
                           const void* mappedData, int bytesPerRow, void* userData)
  {
    CallbackData* callbackDataPtr = reinterpret_cast<CallbackData*>(userData);
    unsigned char* outputValues = callbackDataPtr->outputValues;
    const unsigned char* mappedDataChar = reinterpret_cast<const unsigned char*>(mappedData);

    // Copying the RGB channels of each pixel
    vtkIdType dstIdx = 0;
    for (int y = callbackDataPtr->yMin; y <= callbackDataPtr->yMax; y++)
    {
      for (int x = callbackDataPtr->xMin; x <= callbackDataPtr->xMax; x++)
      {
        // Dividing by inNumberOfComponents * sizeof(SampleType) here because we want to multiply Y
        // by the 'width' which is in number of pixels (ex: for RGBA=4, for RGB=3)
        const int mappedIndex =
          x + y * (bytesPerRow / (inNumberOfComponents * sizeof(unsigned char)));
        // Copying the RGBA channels of each pixel
        for (auto& comp : callbackDataPtr->componentMap)
        {
          outputValues[dstIdx++] = mappedDataChar[mappedIndex * inNumberOfComponents + comp];
        }
      }
    }
    delete callbackDataPtr;
  };

  this->ReadTextureFromGPU(this->ColorAttachment.Texture, this->ColorAttachment.Format, 0,
    wgpu::TextureAspect::All, onTextureMapped, reinterpret_cast<void*>(callbackData));
  this->WaitForCompletion();
  return pixels;
}

//------------------------------------------------------------------------------
int vtkWebGPURenderWindow::GetRGBACharPixelData(
  int x1, int y1, int x2, int y2, int front, vtkUnsignedCharArray* data, int right /*=0*/)
{
  int width = x2 - x1 + 1;
  int height = y2 - y1 + 1;
  const int numberOfComponents = 4;
  data->SetNumberOfComponents(numberOfComponents);
  data->SetNumberOfTuples(width * height);
  unsigned char* pixels = this->GetRGBACharPixelData(x1, y1, x2, y2, front, right);
  // take ownership of pixels
  data->SetArray(pixels, width * height * numberOfComponents, 0);
  return data->GetNumberOfValues();
}

//------------------------------------------------------------------------------
int vtkWebGPURenderWindow::SetRGBACharPixelData(
  int x, int y, int x2, int y2, unsigned char* data, int front, int blend /*=0*/, int right /*=0*/)
{
  vtkWebGPUCheckUnconfiguredWithReturn(this, 0);
  auto device = this->WGPUConfiguration->GetDevice();
  if (device == nullptr)
  {
    vtkErrorMacro(<< "Cannot set RGBA char pixel data because WebGPU device is not ready!");
    return 0;
  }
  (void)front;
  (void)blend;
  (void)right;
  const int nComp = 4;
  int width = (x2 - x) + 1;
  int height = (y2 - y) + 1;
  int bytesPerRow = vtkWebGPUConfiguration::Align(width * nComp, 256);
  int size = bytesPerRow * height;

  const std::string label = "StagingRGBACharPixelData-" + this->GetObjectDescription();
  wgpu::BufferDescriptor desc;
  desc.mappedAtCreation = true;
  desc.label = label.c_str();
  desc.size = size;
  desc.usage = wgpu::BufferUsage::CopySrc;

  this->StagingPixelData.Buffer = this->WGPUConfiguration->CreateBuffer(desc);
  if (this->StagingPixelData.Buffer == nullptr)
  {
    vtkErrorMacro(<< "Failed to create buffer for staging pixel data using device "
                  << device.Get());
    return 0;
  }
  auto* mapped =
    reinterpret_cast<unsigned char*>(this->StagingPixelData.Buffer.GetMappedRange(0, size));
  if (mapped == nullptr)
  {
    vtkErrorMacro(<< "Failed to map staging pixel data!");
    return 0;
  }
  unsigned long dstIdx = 0;
  unsigned long srcIdx = 0;
  const unsigned long nPad = bytesPerRow - width * nComp;
  int componentMap[4] = {};
  if (this->ColorAttachment.Format == wgpu::TextureFormat::BGRA8Unorm)
  {
    componentMap[0] = 2;
    componentMap[1] = 1;
    componentMap[2] = 0;
    componentMap[3] = 3;
  }
  else if (this->ColorAttachment.Format == wgpu::TextureFormat::RGBA8Unorm)
  {
    componentMap[0] = 0;
    componentMap[1] = 1;
    componentMap[2] = 2;
    componentMap[3] = 3;
  }
  else
  {
    // TODO: Handle other formats.
    vtkErrorMacro(<< "Unsupported offscreen texture format!");
  }
  for (int j = 0; j < height; ++j)
  {
    for (int i = 0; i < width; ++i)
    {
      for (const auto& comp : componentMap)
      {
        mapped[dstIdx + comp] = data[srcIdx++];
      }
      dstIdx += nComp;
    }
    dstIdx += nPad;
  }
  this->StagingPixelData.Buffer.Unmap();

  this->StagingPixelData.Layout.bytesPerRow = bytesPerRow;
  this->StagingPixelData.Layout.offset = 0;
  this->StagingPixelData.Layout.rowsPerImage = height;

  this->StagingPixelData.Extent.width = width;
  this->StagingPixelData.Extent.height = height;
  this->StagingPixelData.Extent.depthOrArrayLayers = 1;

  this->StagingPixelData.Origin.x = x;
  this->StagingPixelData.Origin.y = y;
  this->StagingPixelData.Origin.z = 0;

  this->Start();
  this->End();
  this->Frame();
  return size;
}

//------------------------------------------------------------------------------
int vtkWebGPURenderWindow::SetRGBACharPixelData(int x, int y, int x2, int y2,
  vtkUnsignedCharArray* data, int front, int blend /*=0*/, int right /*=0*/)
{
  return this->SetRGBACharPixelData(x, y, x2, y2, data->GetPointer(0), front, blend, right);
}

//------------------------------------------------------------------------------
float* vtkWebGPURenderWindow::GetZbufferData(int x1, int y1, int x2, int y2)
{
  int width = x2 - x1 + 1;
  int height = y2 - y1 + 1;
  float* zValues = new float[width * height];
  this->GetZbufferData(x1, y1, x2, y2, zValues);
  return zValues;
}

//------------------------------------------------------------------------------
int vtkWebGPURenderWindow::GetZbufferData(int x1, int y1, int x2, int y2, float* zValues)
{
  // Create a compute pipeline
  if (this->DepthCopyPipeline == nullptr)
  {
    this->DepthCopyPipeline = vtk::TakeSmartPointer(vtkWebGPUComputePipeline::New());
    this->DepthCopyPipeline->SetLabel("DepthCopy-" + this->GetObjectDescription());
    this->DepthCopyPipeline->SetWGPUConfiguration(this->WGPUConfiguration);
  }
  unsigned int textureWidth = 0;

  // Create a compute pass which copies the depth texture values into a wgpu::Buffer
  if (this->DepthCopyPass == nullptr)
  {
    this->DepthCopyPass = this->DepthCopyPipeline->CreateComputePass();
    this->DepthCopyPass->SetLabel("DepthCopy-" + this->GetObjectDescription());
    vtkSmartPointer<vtkWebGPUComputeRenderTexture> depthTexture;
    depthTexture = this->AcquireDepthBufferRenderTexture();
    textureWidth = depthTexture->GetWidth();

    depthTexture->SetLabel("DepthCopy-" + this->GetObjectDescription());
    this->DepthCopyPass->SetShaderSource(CopyDepthTextureToBuffer);
    this->DepthCopyPass->SetShaderEntryPoint("computeMain");
    this->DepthCopyTextureIndex = this->DepthCopyPass->AddRenderTexture(depthTexture);

    auto depthTextureView = this->DepthCopyPass->CreateTextureView(this->DepthCopyTextureIndex);
    depthTextureView->SetGroup(0);
    depthTextureView->SetBinding(0);
    depthTextureView->SetLabel("DepthCopy-" + this->GetObjectDescription());
    depthTextureView->SetMode(vtkWebGPUTextureView::TextureViewMode::READ_ONLY);
    depthTextureView->SetAspect(vtkWebGPUTextureView::TextureViewAspect::ASPECT_DEPTH);
    depthTextureView->SetFormat(vtkWebGPUTexture::TextureFormat::DEPTH_24_PLUS);
    this->DepthCopyPass->AddTextureView(depthTextureView);

    vtkNew<vtkWebGPUComputeBuffer> buffer;
    buffer->SetGroup(0);
    buffer->SetBinding(1);
    buffer->SetLabel("DepthCopy-" + this->GetObjectDescription());
    buffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::READ_WRITE_MAP_COMPUTE_STORAGE);
    buffer->SetByteSize(
      depthTexture->GetBytesPerPixel() * textureWidth * depthTexture->GetHeight());

    this->DepthCopyBufferIndex = this->DepthCopyPass->AddBuffer(buffer);
  }
  else
  {
    // Resize depth buffer if needed.
    auto depthTexture = this->DepthCopyPass->GetComputeTexture(this->DepthCopyTextureIndex);
    textureWidth = depthTexture->GetWidth();

    const auto byteSize =
      depthTexture->GetBytesPerPixel() * textureWidth * depthTexture->GetHeight();
    if (this->DepthCopyPass->GetBufferByteSize(this->DepthCopyBufferIndex) != byteSize)
    {
      this->DepthCopyPass->ResizeBuffer(this->DepthCopyBufferIndex, byteSize);
    }
  }

  int nbGroupsX = std::ceil(this->SurfaceConfiguredSize[0] / 8.0f);
  int nbGroupsY = std::ceil(this->SurfaceConfiguredSize[1] / 8.0f);
  this->DepthCopyPass->SetWorkgroups(nbGroupsX, nbGroupsY, 1);

  this->DepthCopyPass->Dispatch();

  struct CallbackData
  {
    float* outputValues;
    int xMin;
    int xMax;
    int yMin;
    int yMax;
    unsigned int width;
  };
  auto onBufferMapped = [](const void* mappedData, void* userData)
  {
    CallbackData* callbackDataPtr = reinterpret_cast<CallbackData*>(userData);
    float* outputValues = callbackDataPtr->outputValues;
    const float* mappedDataAsF32 = reinterpret_cast<const float*>(mappedData);
    vtkIdType dstIdx = 0;
    for (int y = callbackDataPtr->yMin; y <= callbackDataPtr->yMax; y++)
    {
      for (int x = callbackDataPtr->xMin; x <= callbackDataPtr->xMax; x++)
      {
        const int mappedIndex = x + y * callbackDataPtr->width;
        outputValues[dstIdx++] = mappedDataAsF32[mappedIndex];
      }
    }
  };
  CallbackData callbackData;
  callbackData.xMin = x1;
  callbackData.xMax = x2;
  callbackData.yMin = y1;
  callbackData.yMax = y2;
  callbackData.outputValues = zValues;
  callbackData.width = textureWidth;
  this->DepthCopyPass->ReadBufferFromGPU(this->DepthCopyBufferIndex, onBufferMapped, &callbackData);
  this->DepthCopyPipeline->Update();
  return VTK_OK;
}

//------------------------------------------------------------------------------
int vtkWebGPURenderWindow::GetZbufferData(int x1, int y1, int x2, int y2, vtkFloatArray* buffer)
{
  int width = x2 - x1 + 1;
  int height = y2 - y1 + 1;
  buffer->SetNumberOfComponents(1);
  buffer->SetNumberOfTuples(width * height);
  return this->GetZbufferData(x1, y1, x2, y2, buffer->GetPointer(0));
}

//------------------------------------------------------------------------------
int vtkWebGPURenderWindow::SetZbufferData(int, int, int, int, float*)
{
  return 0;
}

//------------------------------------------------------------------------------
int vtkWebGPURenderWindow::SetZbufferData(int, int, int, int, vtkFloatArray*)
{
  return 0;
}

//------------------------------------------------------------------------------
int vtkWebGPURenderWindow::GetColorBufferSizes(int* rgba)
{
  rgba[0] = 8;
  rgba[1] = 8;
  rgba[2] = 8;
  rgba[3] = 8;
  return 32;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::WaitForCompletion()
{
  vtkWebGPUCheckUnconfigured(this);
  auto device = this->WGPUConfiguration->GetDevice();
  if (device == nullptr)
  {
    vtkErrorMacro(<< "Cannot wait for completion because WebGPU device is not ready!");
    return;
  }
  if (auto queue = device.GetQueue())
  {
    wgpu::QueueWorkDoneStatus workStatus = wgpu::QueueWorkDoneStatus::Error;
    bool done = false;
    this->WGPUConfiguration->GetDevice().GetQueue().OnSubmittedWorkDone(
      wgpu::CallbackMode::AllowProcessEvents,
      [&workStatus, &done](wgpu::QueueWorkDoneStatus status)
      {
        workStatus = status;
        done = true;
      });
    while (!done)
    {
      this->WGPUConfiguration->ProcessEvents();
    }
    if (workStatus != wgpu::QueueWorkDoneStatus::Success)
    {
      vtkErrorMacro(<< "Submitted work did not complete!");
    }
  }
  else
  {
    vtkErrorMacro(<< "Cannot wait for completion because this render window failed to obtain a "
                     "queue from device "
                  << device.Get());
    return;
  }
}

//------------------------------------------------------------------------------
int vtkWebGPURenderWindow::SupportsOpenGL()
{
  return 0;
}

//------------------------------------------------------------------------------
const char* vtkWebGPURenderWindow::ReportCapabilities()
{
  static std::string caps = "unknown";
  caps = this->WGPUConfiguration->ReportCapabilities();
  return caps.c_str();
}

//------------------------------------------------------------------------------
bool vtkWebGPURenderWindow::InitializeFromCurrentContext()
{
  // TODO: Integrate with dawn::native for d3d12, vulkan and metal swapchain bindings.
  return false;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::ReleaseGraphicsResources(vtkWindow* w)
{
  if (w != this || !this->Renderers)
  {
    return;
  }
  for (auto renderer : vtk::Range(this->Renderers))
  {
    renderer->ReleaseGraphicsResources(this);
  }

  this->DepthCopyPass = nullptr;
  this->DepthCopyPipeline = nullptr;

  this->WGPUPipelineCache->ReleaseGraphicsResources(w);
  this->DestroyColorCopyPipeline();
  this->DestroyIdsAttachment();
  this->DestroyDepthStencilAttachment();
  this->DestroyOffscreenColorAttachment();
  this->UnconfigureSurface();
  this->Surface = nullptr;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::SetWGPUConfiguration(vtkWebGPUConfiguration* config)
{
  // Release all wgpu objects from the current device.
  const bool reInitialize = this->Initialized;
  if (this->Initialized)
  {
    this->WGPUFinalize();
  }
  vtkSetSmartPointerBodyMacro(WGPUConfiguration, vtkWebGPUConfiguration, config);
  if (reInitialize)
  {
    this->Initialize();
  }
}

//------------------------------------------------------------------------------
std::string vtkWebGPURenderWindow::PreprocessShaderSource(const std::string& source) const
{
  std::istringstream is(source);
  std::ostringstream os;
  std::string line;

  while (std::getline(is, line))
  {
    if (line.find("#include") != std::string::npos)
    {
      auto pieces = vtksys::SystemTools::SplitString(line, ' ');
      if (pieces.size() == 3)
      {
        const auto key = pieces[2].substr(1, pieces[2].size() - 2);
        const auto replacement = this->WGPUShaderDatabase->GetShaderSource(key);
        if (!replacement.empty())
        {
          os << "// Start " << key << '\n';
          os << this->PreprocessShaderSource(replacement) << '\n';
          os << "// End " << key << '\n';
        }
        else
        {
          vtkErrorMacro(<< "Failed to substitute " << line);
        }
      }
    }
    else
    {
      os << line << '\n';
    }
  }
  return os.str();
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkImageData> vtkWebGPURenderWindow::SaveAttachmentToVTI(
  AttachmentTypeForVTISnapshot type)
{
  auto image = vtk::TakeSmartPointer(vtkImageData::New());
  vtkNew<vtkFloatArray> colorF32;
  vtkNew<vtkUnsignedCharArray> colorU8;
  vtkNew<vtkTypeUInt32Array> colorU32;
  std::array<int, 3> dims = { 0, 0, 1 };

  switch (type)
  {
    case AttachmentTypeForVTISnapshot::ColorRGBA:
      dims[0] = this->ColorAttachment.Texture.GetWidth();
      dims[1] = this->ColorAttachment.Texture.GetHeight();
      image->SetDimensions(dims.data());
      this->GetRGBAPixelData(0, 0, dims[0] - 1, dims[1] - 1, 0, colorF32, 0);
      image->GetPointData()->SetScalars(colorF32);
      break;
    case AttachmentTypeForVTISnapshot::ColorRGB:
      dims[0] = this->ColorAttachment.Texture.GetWidth();
      dims[1] = this->ColorAttachment.Texture.GetHeight();
      image->SetDimensions(dims.data());
      this->GetPixelData(0, 0, dims[0] - 1, dims[1] - 1, 0, colorU8, 0);
      image->GetPointData()->SetScalars(colorU8);
      break;
    case AttachmentTypeForVTISnapshot::Depth:
      dims[0] = this->DepthStencilAttachment.Texture.GetWidth();
      dims[1] = this->DepthStencilAttachment.Texture.GetHeight();
      image->SetDimensions(dims.data());
      this->GetZbufferData(0, 0, dims[0] - 1, dims[1] - 1, colorF32);
      image->GetPointData()->SetScalars(colorF32);
      break;
    case AttachmentTypeForVTISnapshot::Ids:
      dims[0] = this->IdsAttachment.Texture.GetWidth();
      dims[1] = this->IdsAttachment.Texture.GetHeight();
      image->SetDimensions(dims.data());
      this->GetIdsData(0, 0, dims[0] - 1, dims[1] - 1, colorU32);
      image->GetPointData()->SetScalars(colorU32);
      break;
  }
  return image;
}

VTK_ABI_NAMESPACE_END
