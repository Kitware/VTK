// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWebGPURenderWindow.h"
#include "Private/vtkWebGPUBindGroupInternals.h"
#include "Private/vtkWebGPUBindGroupLayoutInternals.h"
#include "Private/vtkWebGPUBufferInternals.h"
#include "Private/vtkWebGPUCallbacksInternals.h"
#include "Private/vtkWebGPUComputePassInternals.h"
#include "Private/vtkWebGPUPipelineLayoutInternals.h"
#include "Private/vtkWebGPURenderPassCreateInfoInternals.h"
#include "Private/vtkWebGPURenderPipelineDescriptorInternals.h"
#include "Private/vtkWebGPUShaderModuleInternals.h"
#include "vtkCamera.h"
#include "vtkCollectionRange.h"
#include "vtkFloatArray.h"
#include "vtkObject.h"
#include "vtkObjectFactory.h"
#include "vtkRect.h"
#include "vtkRendererCollection.h"
#include "vtkTypeUInt8Array.h"
#include "vtkUnsignedCharArray.h"
#include "vtkWebGPUClearDrawPass.h"
#include "vtkWebGPUConfiguration.h"
#include "vtkWebGPURenderer.h"

#include <exception>
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
struct PixelReadDescriptor
{
  vtkRecti Rect;
  int NumColorComponents = 0;
  int NumBytesPerRow = 0;
  int NumRows = 0;
};

PixelReadDescriptor GetPixelReadDesriptor(
  const wgpu::Texture& colorTexture, const int x, const int y, const int x2, const int y2)
{
  PixelReadDescriptor desc;
  desc.NumColorComponents = 4;
  desc.NumBytesPerRow = vtkWebGPUConfiguration::Align(colorTexture.GetWidth() * 4, 256);
  desc.NumRows = colorTexture.GetHeight();

  int y_low, y_hi;
  int x_low, x_hi;

  if (y < y2)
  {
    y_low = y;
    y_hi = y2;
  }
  else
  {
    y_low = y2;
    y_hi = y;
  }

  if (x < x2)
  {
    x_low = x;
    x_hi = x2;
  }
  else
  {
    x_low = x2;
    x_hi = x;
  }

  desc.Rect.Set(x, y, (x_hi - x_low) + 1, (y_hi - y_low) + 1);
  return desc;
}

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

  this->CreateSwapChain();
  this->CreateOffscreenColorAttachments();
  this->CreateDepthStencilTexture();
  this->CreateFSQGraphicsPipeline();
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
wgpu::TextureView vtkWebGPURenderWindow::GetDepthStencilView()
{
  return this->DepthStencil.View;
}

//------------------------------------------------------------------------------
wgpu::TextureFormat vtkWebGPURenderWindow::GetDepthStencilFormat()
{
  return this->DepthStencil.Format;
}

//------------------------------------------------------------------------------
bool vtkWebGPURenderWindow::HasStencil()
{
  return this->DepthStencil.HasStencil;
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
wgpu::TextureFormat vtkWebGPURenderWindow::GetPreferredSwapChainTextureFormat()
{
  ///@{ TODO: Concrete window subclasses must override this method, query window system
  // for a preferred texture format.
  return wgpu::TextureFormat::BGRA8Unorm;
  ///@}
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
  texture->SetWebGPUTexture(this->DepthStencil.Texture);
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
wgpu::Buffer vtkWebGPURenderWindow::CreateDeviceBuffer(wgpu::BufferDescriptor& bufferDescriptor)
{
  wgpu::Device device = this->WGPUConfiguration->GetDevice();
  if (!vtkWebGPUBufferInternals::CheckBufferSize(device, bufferDescriptor.size))
  {
    wgpu::SupportedLimits supportedDeviceLimits;
    device.GetLimits(&supportedDeviceLimits);

    vtkLog(ERROR,
      "The current WebGPU Device cannot create buffers larger than: "
        << supportedDeviceLimits.limits.maxStorageBufferBindingSize
        << " bytes but the buffer with label " << bufferDescriptor.label << " is "
        << bufferDescriptor.size << " bytes big.");

    return nullptr;
  }

  return device.CreateBuffer(&bufferDescriptor);
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
void vtkWebGPURenderWindow::CreateSwapChain()
{
  vtkDebugMacro(<< __func__ << '(' << this->Size[0] << ',' << this->Size[1] << ')');
  vtkWebGPUCheckUnconfigured(this);

  this->SwapChain.Width = this->Size[0];
  this->SwapChain.Height = this->Size[1];

  wgpu::SwapChainDescriptor swapChainDescriptor = {};
  swapChainDescriptor.usage = wgpu::TextureUsage::RenderAttachment;
  swapChainDescriptor.format = this->GetPreferredSwapChainTextureFormat();
  swapChainDescriptor.width = this->SwapChain.Width;
  swapChainDescriptor.height = this->SwapChain.Height;
  swapChainDescriptor.presentMode = wgpu::PresentMode::Fifo;

  this->SwapChain.TexFormat = swapChainDescriptor.format;
  if (auto device = this->GetDevice())
  {
    this->SwapChain.Instance = device.CreateSwapChain(this->Surface, &swapChainDescriptor);
  }
  else
  {
    vtkErrorMacro(
      << "Cannot create a command encoder because a WebGPU device has not been initialized!");
  }
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::DestroySwapChain()
{
  vtkDebugMacro(<< __func__);
  this->SwapChain.Instance = nullptr;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::CreateDepthStencilTexture()
{
  vtkDebugMacro(<< __func__ << '(' << this->SwapChain.Width << ',' << this->SwapChain.Height
                << ')');
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
  this->DepthStencil.HasStencil = true;

  wgpu::TextureDescriptor textureDesc;
  textureDesc.dimension = wgpu::TextureDimension::e2D;
  textureDesc.size.width = this->SwapChain.Width;
  textureDesc.size.height = this->SwapChain.Height;
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

  if (auto texture = device.CreateTexture(&textureDesc))
  {
    this->DepthStencil.Texture = texture;
    if (auto view = texture.CreateView(&textureViewDesc))
    {
      this->DepthStencil.View = view;
      this->DepthStencil.Format = textureDesc.format;
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
void vtkWebGPURenderWindow::DestroyDepthStencilTexture()
{
  vtkDebugMacro(<< __func__);
  this->DepthStencil.View = nullptr;
  this->DepthStencil.Texture = nullptr;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::CreateOffscreenColorAttachments()
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
  textureExtent.width = this->SwapChain.Width;
  textureExtent.height = this->SwapChain.Height;

  // Color attachment
  wgpu::TextureDescriptor textureDesc;
  textureDesc.size = textureExtent;
  textureDesc.mipLevelCount = 1;
  textureDesc.sampleCount = 1;
  textureDesc.dimension = wgpu::TextureDimension::e2D;
  textureDesc.format = this->GetPreferredSwapChainTextureFormat();
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

  if (auto texture = device.CreateTexture(&textureDesc))
  {
    this->ColorAttachment.Texture = texture;
    if (auto view = texture.CreateView(&textureViewDesc))
    {
      this->ColorAttachment.View = view;
      this->ColorAttachment.Format = textureDesc.format;

      // color attachment texture can be read into this buffer and then mapped into a CPU side
      // buffer.
      const auto alignedWidth =
        vtkWebGPUConfiguration::Align(4 * this->ColorAttachment.Texture.GetWidth(), 256);
      wgpu::BufferDescriptor buffDesc;
      buffDesc.label = "Offscreen buffer";
      buffDesc.mappedAtCreation = false;
      buffDesc.size = this->ColorAttachment.Texture.GetHeight() * alignedWidth;
      buffDesc.usage = wgpu::BufferUsage::MapRead | wgpu::BufferUsage::CopyDst;
      if (auto buffer = device.CreateBuffer(&buffDesc))
      {
        this->ColorAttachment.OffscreenBuffer = buffer;
      }
      else
      {
        vtkErrorMacro(<< "Failed to create a buffer for offscreen color attachment using device "
                      << device.Get());
      }
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
void vtkWebGPURenderWindow::DestroyOffscreenColorAttachments()
{
  if (this->ColorAttachment.OffscreenBuffer.Get() != nullptr)
  {
    this->ColorAttachment.OffscreenBuffer.Destroy();
    this->ColorAttachment.OffscreenBuffer = nullptr;
  }
  this->ColorAttachment.View = nullptr;
  this->ColorAttachment.Texture = nullptr;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::CreateFSQGraphicsPipeline()
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
    });
  bgl.SetLabel("FSQ bind group layout");

  wgpu::PipelineLayout pipelineLayout =
    vtkWebGPUPipelineLayoutInternals::MakeBasicPipelineLayout(device, &bgl);
  pipelineLayout.SetLabel("FSQ graphics pipeline layout");

  this->FSQ.BindGroup = vtkWebGPUBindGroupInternals::MakeBindGroup(device, bgl,
    {
      // clang-formt off
      { 0, this->ColorAttachment.View }
      // clang-format on
    });

  wgpu::ShaderModule shaderModule = vtkWebGPUShaderModuleInternals::CreateFromWGSL(device, R"(
    struct VertexOutput {
      @builtin(position) position: vec4<f32>,
      @location(0) uv: vec2<f32>
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
      output.uv = output.position.xy * 0.5 + 0.5;
      // fip y for texture coordinate.
      output.uv.y = 1.0 - output.uv.y;
      return output;
    }

    struct FragmentInput {
      @builtin(position) position: vec4<f32>,
      @location(0) uv: vec2<f32>
    }

    @group(0) @binding(0) var fsqTexture: texture_2d<f32>;

    @fragment
    fn fragmentMain(fragment: FragmentInput) -> @location(0) vec4<f32> {
      let color = textureLoad(fsqTexture, vec2<i32>(fragment.position.xy), 0);
      return vec4<f32>(color);
    }
  )");
  if (shaderModule == nullptr)
  {
    vtkErrorMacro(<< "Failed to create shader module for full-screen-quad graphics pipeline.");
    return;
  }

  vtkWebGPURenderPipelineDescriptorInternals pipelineDesc;
  pipelineDesc.label = "FSQ Graphics pipeline description";
  pipelineDesc.layout = pipelineLayout;
  pipelineDesc.vertex.module = shaderModule;
  pipelineDesc.vertex.entryPoint = "vertexMain";
  pipelineDesc.vertex.bufferCount = 0;
  pipelineDesc.cFragment.module = shaderModule;
  pipelineDesc.cFragment.entryPoint = "fragmentMain";
  pipelineDesc.cTargets[0].format = this->GetPreferredSwapChainTextureFormat();
  pipelineDesc.DisableDepthStencil();
  pipelineDesc.primitive.topology = wgpu::PrimitiveTopology::TriangleStrip;

  if (auto pipeline = device.CreateRenderPipeline(&pipelineDesc))
  {
    this->FSQ.Pipeline = pipeline;
  }
  else
  {
    vtkErrorMacro(<< "Failed to create the full-screen-quad render pipeline.");
  }
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::RecreateComputeRenderTextures()
{
  vtkWebGPUCheckUnconfigured(this);

  for (const auto& renderTexture : this->ComputeRenderTextures)
  {
    int* dims = this->GetSize();

    // Upadting the size of the texture
    renderTexture->SetSize(dims[0], dims[1]);

    // Updating the WebGPU texture used by the render texture since it has been recreated by the
    // window resize
    switch (renderTexture->GetType())
    {
      case vtkWebGPUComputeRenderTexture::RenderTextureType::DEPTH_BUFFER:
        renderTexture->SetWebGPUTexture(this->DepthStencil.Texture);
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
void vtkWebGPURenderWindow::DestroyFSQGraphicsPipeline()
{
  this->FSQ.BindGroup = nullptr;
  this->FSQ.Pipeline = nullptr;
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
void vtkWebGPURenderWindow::RenderOffscreenTexture()
{
  vtkWebGPUCheckUnconfigured(this);
  if (this->SwapChain.Instance == nullptr)
  {
    vtkErrorMacro(<< "Cannot render offscreen texture because swapchain is null!");
    return;
  }
  if (this->ColorAttachment.Texture == nullptr)
  {
    vtkErrorMacro(<< "Cannot render offscreen texture because the source color attachment "
                     "texture is null!");
    return;
  }
  if (this->FSQ.Pipeline == nullptr)
  {
    vtkErrorMacro(<< "Cannot render offscreen texture because the full-screen-quad render "
                     "pipeline is null!");
    return;
  }
  if (this->FSQ.BindGroup == nullptr)
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
  // prepare the offscreen texture for presentation.
  this->SwapChain.Framebuffer = this->SwapChain.Instance.GetCurrentTextureView();

  vtkWebGPURenderPassDescriptorInternals renderPassDescriptor({ this->SwapChain.Framebuffer });
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
    encoder.SetViewport(0, 0, this->SwapChain.Width, this->SwapChain.Height, 0.0, 1.0);
    encoder.SetScissorRect(0, 0, this->SwapChain.Width, this->SwapChain.Height);
    // set fsq pipeline
#ifndef NDEBUG
    encoder.PushDebugGroup("FSQ Render");
#endif
    encoder.SetPipeline(this->FSQ.Pipeline);
    // bind fsq group
    encoder.SetBindGroup(0, this->FSQ.BindGroup);
    // draw triangle strip
    encoder.Draw(4);
#ifndef NDEBUG
    encoder.PopDebugGroup();
#endif
    encoder.End();
  }
  else
  {
    vtkErrorMacro(<< "Cannot render swapchain contents into offscreen texture because this render "
                     "window failed to build a new render pass!");
    return;
  }

  if (this->ColorAttachment.OffscreenBuffer == nullptr)
  {
    vtkErrorMacro(<< "Cannot copy offscreen texture into offscreen buffer because the destination "
                     "buffer is null!");
    return;
  }
  // Now copy the contents of the color attachment texture into the offscreen buffer.
  // Both source and destination are on the GPU.
  // Later, when we really need the pixels on the CPU, the `ReadPixels` method will map
  // the contents of the offscreen buffer into CPU memory.
  wgpu::Origin3D srcOrigin;
  srcOrigin.x = 0;
  srcOrigin.y = 0;
  srcOrigin.y = 0;

  wgpu::Extent3D srcExtent;
  srcExtent.width = this->ColorAttachment.Texture.GetWidth();
  srcExtent.height = this->ColorAttachment.Texture.GetHeight();
  srcExtent.depthOrArrayLayers = 1;

  wgpu::ImageCopyTexture copySrc;
  copySrc.texture = this->ColorAttachment.Texture;
  copySrc.mipLevel = 0;
  copySrc.origin = srcOrigin;
  copySrc.aspect = wgpu::TextureAspect::All;

  wgpu::TextureDataLayout textureDataLayout;
  textureDataLayout.offset = 0;
  textureDataLayout.bytesPerRow =
    vtkWebGPUConfiguration::Align(4 * this->ColorAttachment.Texture.GetWidth(), 256);
  textureDataLayout.rowsPerImage = this->ColorAttachment.Texture.GetHeight();

  wgpu::ImageCopyBuffer copyDst;
  copyDst.buffer = this->ColorAttachment.OffscreenBuffer;
  copyDst.layout = textureDataLayout;

#ifndef NDEBUG
  this->CommandEncoder.PushDebugGroup("Copy color attachment to offscreen buffer");
#endif
  this->CommandEncoder.CopyTextureToBuffer(&copySrc, &copyDst, &srcExtent);
#ifndef NDEBUG
  this->CommandEncoder.PopDebugGroup();
#endif
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

  if (this->Size[0] != this->SwapChain.Width || this->Size[1] != this->SwapChain.Height)
  {
    // Window's size changed, need to recreate the swap chain, textures, ...
    this->DestroyFSQGraphicsPipeline();
    this->DestroyDepthStencilTexture();
    this->DestroyOffscreenColorAttachments();
    this->DestroySwapChain();
    this->CreateSwapChain();
    this->CreateOffscreenColorAttachments();
    this->CreateDepthStencilTexture();
    this->CreateFSQGraphicsPipeline();
    this->RecreateComputeRenderTextures();
  }

  this->CreateCommandEncoder();
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::Frame()
{
  vtkDebugMacro(<< __func__);
  vtkWebGPUCheckUnconfigured(this);
  if (this->CommandEncoder == nullptr)
  {
    vtkErrorMacro(<< "Cannot render frame because the command encoder is null!");
    return;
  }
  if (this->SwapChain.Instance == nullptr)
  {
    vtkErrorMacro(<< "Cannot render frame because the swapchain is null!");
    return;
  }
  this->Superclass::Frame();

  // Flushing the commands for the props to be rendered
  wgpu::CommandBufferDescriptor cmdBufDesc = {};
  wgpu::CommandBuffer cmdBuffer = this->CommandEncoder.Finish(&cmdBufDesc);

  this->CommandEncoder = nullptr;
  this->FlushCommandBuffers(1, &cmdBuffer);

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
  this->SwapChain.Instance.Present();
#endif
  this->SwapChain.Framebuffer = nullptr;

  // Clean up staging buffer for SetPixelData.
  if (this->StagingPixelData.Buffer.Get() != nullptr)
  {
    this->StagingPixelData.Buffer.Destroy();
    this->StagingPixelData.Buffer = nullptr;
  }

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
    wgpu::ImageCopyTexture destination;
    destination.texture = this->ColorAttachment.Texture;
    destination.mipLevel = 0;
    destination.origin = this->StagingPixelData.Origin;
    destination.aspect = wgpu::TextureAspect::All;

    wgpu::ImageCopyBuffer source;
    source.buffer = this->StagingPixelData.Buffer;
    source.layout = this->StagingPixelData.Layout;
    this->Start();
#ifndef NDEBUG
    this->CommandEncoder.PushDebugGroup("Copy staging RGBA pixel buffer to texture");
#endif
    this->CommandEncoder.CopyBufferToTexture(
      &source, &destination, &(this->StagingPixelData.Extent));
#ifndef NDEBUG
    this->CommandEncoder.PopDebugGroup();
#endif
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
void vtkWebGPURenderWindow::ReadPixels()
{
  vtkWebGPUCheckUnconfigured(this);
  if (this->ColorAttachment.OffscreenBuffer == nullptr)
  {
    vtkErrorMacro(<< "Cannot read pixels from texture because the color attachment's offscreen "
                     "buffer is null!");
    return;
  }
  if (this->CachedPixelBytes->GetNumberOfValues() > 0)
  {
    // use cache
    return;
  }
  this->BufferMapReadContext.src = this->ColorAttachment.OffscreenBuffer;
  this->BufferMapReadContext.size = this->ColorAttachment.OffscreenBuffer.GetSize();
  this->BufferMapReadContext.dst = this->CachedPixelBytes;
  this->BufferMapReadContext.window = this;

  auto onBufferMapped = [](WGPUBufferMapAsyncStatus status, void* userdata) {
    auto ctx = reinterpret_cast<MappingContext*>(userdata);
    if (ctx == nullptr)
    {
      vtkErrorWithObjectMacro(nullptr, << "Unexpected user data from buffer mapped callback in "
                                          "vtkWebGPURenderWindow::ReadPixels");
      return;
    }
    if (!ctx->window)
    {
      vtkErrorWithObjectMacro(nullptr,
        << "Mapping context in vtkWebGPURenderWindow::ReadPixels is missing render window!");
      return;
    }
    if (!ctx->dst)
    {
      vtkErrorWithObjectMacro(
        ctx->window, << "Mapping context in vtkWebGPURenderWindow::ReadPixels is missing "
                        "destination vtkDataArray object!");
      return;
    }
    if (!ctx->src)
    {
      vtkErrorWithObjectMacro(
        ctx->window, << "Mapping context in vtkWebGPURenderWindow::ReadPixels is missing "
                        "source WGPUbuffer object!");
      return;
    }
    switch (status)
    {
      case WGPUBufferMapAsyncStatus_ValidationError:
        vtkErrorWithObjectMacro(ctx->window, << "Validation error occurred");
        break;
      case WGPUBufferMapAsyncStatus_Unknown:
        vtkErrorWithObjectMacro(ctx->window, << "Unknown error occurred");
        break;
      case WGPUBufferMapAsyncStatus_DeviceLost:
        vtkErrorWithObjectMacro(ctx->window, << "Device lost!");
        break;
      case WGPUBufferMapAsyncStatus_DestroyedBeforeCallback:
        vtkErrorWithObjectMacro(ctx->window, << "Buffer destroyed before callback");
        break;
      case WGPUBufferMapAsyncStatus_UnmappedBeforeCallback:
        vtkErrorWithObjectMacro(ctx->window, << "Buffer unmapped before callback");
        break;
      case WGPUBufferMapAsyncStatus_MappingAlreadyPending:
        vtkErrorWithObjectMacro(ctx->window, << "Buffer already has a mapping pending completion");
        break;
      case WGPUBufferMapAsyncStatus_OffsetOutOfRange:
        vtkErrorWithObjectMacro(ctx->window, << "Buffer offset out of range");
        break;
      case WGPUBufferMapAsyncStatus_SizeOutOfRange:
        vtkErrorWithObjectMacro(ctx->window, << "Buffer size out of range");
        break;
      case WGPUBufferMapAsyncStatus_Success:
      {
        // acquire a const mapped range since OffscreenBuffer is assigned a `MapRead` usage.
        auto mapped =
          reinterpret_cast<const vtkTypeUInt8*>(ctx->src.GetConstMappedRange(0, ctx->size));
        if (mapped == nullptr)
        {
          vtkErrorWithObjectMacro(ctx->window, << "Mapped range returned null!");
          break;
        }
        else
        {
          // allocate sufficient space on host.
          ctx->dst->SetNumberOfValues(ctx->size);
          // These are plain bytes. GetABCDPixelData() functions know how to interpret them.
          std::copy(mapped, mapped + ctx->size, ctx->dst->GetPointer(0));
        }
      }
      break;
      default:
        break;
    }
    ctx->src.Unmap();
  };
  this->ColorAttachment.OffscreenBuffer.MapAsync(wgpu::MapMode::Read, 0,
    this->BufferMapReadContext.size, onBufferMapped, &this->BufferMapReadContext);
  this->WaitForCompletion();
}

//------------------------------------------------------------------------------
unsigned char* vtkWebGPURenderWindow::GetPixelData(
  int x, int y, int x2, int y2, int front, int right)
{
  (void)front;
  (void)right;
  this->ReadPixels();

  PixelReadDescriptor desc = ::GetPixelReadDesriptor(this->ColorAttachment.Texture, x, y, x2, y2);
  unsigned char* pixels = new unsigned char[desc.Rect.GetWidth() * desc.Rect.GetHeight() * 3];
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
  else
  {
    // TODO: Handle other formats.
    vtkErrorMacro(<< "Unsupported offscreen texture format!");
  }

  vtkIdType dstIdx = 0;
  for (int j = desc.Rect.GetY(); j < desc.Rect.GetTop(); ++j)
  {
    for (int i = desc.Rect.GetX(); i < desc.Rect.GetRight(); ++i)
    {
      for (auto& comp : componentMap)
      {
        pixels[dstIdx++] = this->CachedPixelBytes->GetValue(
          j * desc.NumBytesPerRow + i * desc.NumColorComponents + comp);
      }
    }
  }
  return pixels;
}

//------------------------------------------------------------------------------
int vtkWebGPURenderWindow::GetPixelData(
  int x, int y, int x2, int y2, int front, vtkUnsignedCharArray* data, int right)
{
  PixelReadDescriptor desc = ::GetPixelReadDesriptor(this->ColorAttachment.Texture, x, y, x2, y2);
  data->SetNumberOfComponents(3);
  data->SetNumberOfTuples(desc.Rect.GetWidth() * desc.Rect.GetHeight());
  unsigned char* pixels = this->GetPixelData(x, y, x2, y2, front, right);
  // take ownership of pixels
  data->SetArray(pixels, desc.Rect.GetWidth() * desc.Rect.GetHeight() * 3, 0);
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

  wgpu::BufferDescriptor desc;
  desc.mappedAtCreation = true;
  desc.label = "Staging buffer for SetPixelData";
  desc.size = size;
  desc.usage = wgpu::BufferUsage::CopySrc;

  this->StagingPixelData.Buffer = this->CreateDeviceBuffer(desc);
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
  int x, int y, int x2, int y2, int front, int right /*=0*/)
{
  (void)front;
  (void)right;
  this->ReadPixels();

  PixelReadDescriptor desc = ::GetPixelReadDesriptor(this->ColorAttachment.Texture, x, y, x2, y2);
  float* pixels = new float[desc.Rect.GetWidth() * desc.Rect.GetHeight() * 4];
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

  vtkIdType dstIdx = 0;
  for (int j = desc.Rect.GetY(); j < desc.Rect.GetTop(); ++j)
  {
    for (int i = desc.Rect.GetX(); i < desc.Rect.GetRight(); ++i)
    {
      for (auto& comp : componentMap)
      {
        pixels[dstIdx++] = this->CachedPixelBytes->GetValue(
                             j * desc.NumBytesPerRow + i * desc.NumColorComponents + comp) /
          255.0;
      }
    }
  }
  return pixels;
}

//------------------------------------------------------------------------------
int vtkWebGPURenderWindow::GetRGBAPixelData(
  int x, int y, int x2, int y2, int front, vtkFloatArray* data, int right /*=0*/)
{
  PixelReadDescriptor desc = ::GetPixelReadDesriptor(this->ColorAttachment.Texture, x, y, x2, y2);
  data->SetNumberOfComponents(4);
  data->SetNumberOfTuples(desc.Rect.GetWidth() * desc.Rect.GetHeight());
  float* pixels = this->GetRGBAPixelData(x, y, x2, y2, front, right);
  // take ownership of pixels
  data->SetArray(pixels, desc.Rect.GetWidth() * desc.Rect.GetHeight() * 4, 0);
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

  wgpu::BufferDescriptor desc;
  desc.mappedAtCreation = true;
  desc.label = "Staging buffer for SetRGBAPixelData";
  desc.size = size;
  desc.usage = wgpu::BufferUsage::CopySrc;

  this->StagingPixelData.Buffer = this->CreateDeviceBuffer(desc);
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
  (void)data;
  // reset cache
  this->CachedPixelBytes->SetNumberOfValues(0);
}

unsigned char* vtkWebGPURenderWindow::GetRGBACharPixelData(
  int x, int y, int x2, int y2, int front, int right /*=0*/)
{
  (void)front;
  (void)right;
  this->ReadPixels();

  PixelReadDescriptor desc = ::GetPixelReadDesriptor(this->ColorAttachment.Texture, x, y, x2, y2);
  unsigned char* pixels = new unsigned char[desc.Rect.GetWidth() * desc.Rect.GetHeight() * 4];
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

  vtkIdType dstIdx = 0;
  for (int j = desc.Rect.GetY(); j < desc.Rect.GetTop(); ++j)
  {
    for (int i = desc.Rect.GetX(); i < desc.Rect.GetRight(); ++i)
    {
      for (auto& comp : componentMap)
      {
        pixels[dstIdx++] = this->CachedPixelBytes->GetValue(
          j * desc.NumBytesPerRow + i * desc.NumColorComponents + comp);
      }
    }
  }
  return pixels;
}

//------------------------------------------------------------------------------
int vtkWebGPURenderWindow::GetRGBACharPixelData(
  int x, int y, int x2, int y2, int front, vtkUnsignedCharArray* data, int right /*=0*/)
{
  PixelReadDescriptor desc = ::GetPixelReadDesriptor(this->ColorAttachment.Texture, x, y, x2, y2);
  data->SetNumberOfComponents(4);
  data->SetNumberOfTuples(desc.Rect.GetWidth() * desc.Rect.GetHeight());
  unsigned char* pixels = this->GetRGBACharPixelData(x, y, x2, y2, front, right);
  // take ownership of pixels
  data->SetArray(pixels, desc.Rect.GetWidth() * desc.Rect.GetHeight() * 4, 0);
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

  wgpu::BufferDescriptor desc;
  desc.mappedAtCreation = true;
  desc.label = "Staging buffer for SetRGBACharPixelData";
  desc.size = size;
  desc.usage = wgpu::BufferUsage::CopySrc;

  this->StagingPixelData.Buffer = this->CreateDeviceBuffer(desc);
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
float* vtkWebGPURenderWindow::GetZbufferData(int, int, int, int)
{
  return nullptr;
}

//------------------------------------------------------------------------------
int vtkWebGPURenderWindow::GetZbufferData(int, int, int, int, float*)
{
  return 0;
}

//------------------------------------------------------------------------------
int vtkWebGPURenderWindow::GetZbufferData(int, int, int, int, vtkFloatArray*)
{
  return 0;
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
  bool done = false;
  if (auto queue = device.GetQueue())
  {
    device.GetQueue().OnSubmittedWorkDone(
#if defined(__EMSCRIPTEN__) &&                                                                     \
  ((__EMSCRIPTEN_major__ < 3) || ((__EMSCRIPTEN_major__ <= 3) && (__EMSCRIPTEN_minor__ < 1)) ||    \
    ((__EMSCRIPTEN_major__ <= 3) && (__EMSCRIPTEN_minor__ <= 1) && (__EMSCRIPTEN_tiny__ < 54)))
      // https://github.com/emscripten-core/emscripten/commit/6daa18bc5ab19730421d2d63b69ddf41f11f1e85
      // removed unused signalValue argument from 3.1.54 onwards.
      0u,
#endif
      [](WGPUQueueWorkDoneStatus, void* userdata) { *static_cast<bool*>(userdata) = true; }, &done);
    while (!done)
    {
      this->WGPUConfiguration->ProcessEvents();
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
  // TODO: Request caps from device
  return "unknown";
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
  for (auto ren : vtk::Range(this->Renderers))
  {
    ren->ReleaseGraphicsResources(this);
  }
  this->DestroyFSQGraphicsPipeline();
  this->DestroyDepthStencilTexture();
  this->DestroyOffscreenColorAttachments();
  this->DestroySwapChain();
  this->BufferMapReadContext.src = nullptr;
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

VTK_ABI_NAMESPACE_END
