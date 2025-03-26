// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWebGPURenderWindow.h"
#include "vtkFloatArray.h"
#include "vtkObject.h"
#include "vtkObjectFactory.h"
#include "vtkRect.h"
#include "vtkRendererCollection.h"
#include "vtkTypeUInt8Array.h"
#include "vtkUnsignedCharArray.h"
#include "vtkWGPUContext.h"
#include "vtkWebGPUClearPass.h"
#include "vtkWebGPUInternalsBindGroup.h"
#include "vtkWebGPUInternalsBindGroupLayout.h"
#include "vtkWebGPUInternalsCallbacks.h"
#include "vtkWebGPUInternalsPipelineLayout.h"
#include "vtkWebGPUInternalsRenderPassCreateInfo.h"
#include "vtkWebGPUInternalsRenderPipelineDescriptor.h"
#include "vtkWebGPUInternalsShaderModule.h"
#include "vtkWebGPURenderer.h"

#include <exception>
#include <sstream>

#if defined(__EMSCRIPTEN__)
#include "emscripten/version.h"
#endif

VTK_ABI_NAMESPACE_BEGIN

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
  desc.NumBytesPerRow = vtkWGPUContext::Align(colorTexture.GetWidth() * 4, 256);
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
}

//------------------------------------------------------------------------------
vtkWebGPURenderWindow::~vtkWebGPURenderWindow() = default;

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::PreferHighPerformanceAdapter()
{
  this->PowerPreference = wgpu::PowerPreference::HighPerformance;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::PreferLowPowerAdapter()
{
  this->PowerPreference = wgpu::PowerPreference::LowPower;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::SetBackendTypeToD3D11()
{
  this->RenderingBackendType = wgpu::BackendType::D3D11;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::SetBackendTypeToD3D12()
{
  this->RenderingBackendType = wgpu::BackendType::D3D12;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::SetBackendTypeToMetal()
{
  this->RenderingBackendType = wgpu::BackendType::Metal;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::SetBackendTypeToVulkan()
{
  this->RenderingBackendType = wgpu::BackendType::Vulkan;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::SetBackendTypeToOpenGL()
{
  this->RenderingBackendType = wgpu::BackendType::OpenGL;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::SetBackendTypeToOpenGLES()
{
  this->RenderingBackendType = wgpu::BackendType::OpenGLES;
}

//------------------------------------------------------------------------------
std::string vtkWebGPURenderWindow::GetBackendTypeAsString()
{
  wgpu::AdapterProperties properties = {};
  this->Adapter.GetProperties(&properties);
  switch (properties.backendType)
  {
    case wgpu::BackendType::Null:
      return "Null";
    case wgpu::BackendType::WebGPU:
      return "WebGPU";
    case wgpu::BackendType::D3D11:
      return "D3D11";
    case wgpu::BackendType::D3D12:
      return "D3D12";
    case wgpu::BackendType::Metal:
      return "Metal";
    case wgpu::BackendType::Vulkan:
      return "Vulkan";
    case wgpu::BackendType::OpenGL:
      return "OpenGL";
    case wgpu::BackendType::OpenGLES:
      return "OpenGL ES";
    case wgpu::BackendType::Undefined:
      break;
  }
  return "Unknown";
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
bool vtkWebGPURenderWindow::WGPUInit()
{
  vtkDebugMacro(<< __func__ << " WGPUInitialized=" << this->WGPUInitialized);
  vtkWGPUContext::LogAvailableAdapters();
  ///@{ TODO: CLEAN UP DEVICE ACQUISITION
  // for emscripten, the glue code is expected to pre-initialize an instance, adapter and a device.
  wgpu::RequestAdapterOptions options;
  options.backendType = this->RenderingBackendType;
  options.powerPreference = this->PowerPreference;
  this->Adapter = vtkWGPUContext::RequestAdapter(options);

  wgpu::DeviceDescriptor deviceDescriptor = {};
  deviceDescriptor.label = "vtkWebGPURenderWindow::WGPUInit";
  deviceDescriptor.deviceLostCallback = &vtkWebGPUInternalsCallbacks::DeviceLostCallback;
  deviceDescriptor.deviceLostUserdata = this;
  ///@{ TODO: Populate feature requests
  // ...
  ///@}
  ///@{ TODO: Populate limit requests
  // ...
  ///@}
  this->Device = vtkWGPUContext::RequestDevice(this->Adapter, deviceDescriptor);
  if (!this->Device)
  {
    vtkErrorMacro(<< "Failed to acquire device");
    return false;
  }
  // install error handler
  this->Device.SetUncapturedErrorCallback(vtkWebGPUInternalsCallbacks::PrintWGPUError, this);
  ///@}
  // change the window name if it's the default "Visualization Toolkit"
  // to "Visualization Toolkit " + " Backend name"
  if (this->WindowName == std::string("Visualization Toolkit"))
  {
    std::string windowNameWithBackend = this->MakeDefaultWindowNameWithBackend();
    this->SetWindowName(windowNameWithBackend.c_str());
  }
  return true;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::WGPUFinalize()
{
  vtkDebugMacro(<< __func__ << " WGPUInitialized=" << this->WGPUInitialized);
  this->DestroyDepthStencilTexture();
  this->DestroySwapChain();
  this->Device = nullptr;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::Render()
{
  vtkDebugMacro(<< __func__);

  this->Superclass::Render();
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
void vtkWebGPURenderWindow::CreateSwapChain()
{
  vtkDebugMacro(<< __func__ << '(' << this->Size[0] << ',' << this->Size[1] << ')');

  this->SwapChain.Width = this->Size[0];
  this->SwapChain.Height = this->Size[1];

  wgpu::SwapChainDescriptor swapChainDescriptor = {};
  swapChainDescriptor.usage = wgpu::TextureUsage::RenderAttachment;
  swapChainDescriptor.format = this->GetPreferredSwapChainTextureFormat();
  swapChainDescriptor.width = this->SwapChain.Width;
  swapChainDescriptor.height = this->SwapChain.Height;
  swapChainDescriptor.presentMode = wgpu::PresentMode::Fifo;

  this->SwapChain.TexFormat = swapChainDescriptor.format;
  this->SwapChain.Instance = this->Device.CreateSwapChain(this->Surface, &swapChainDescriptor);
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
  textureDesc.usage = wgpu::TextureUsage::RenderAttachment;

  // view
  wgpu::TextureViewDescriptor textureViewDesc;
  textureViewDesc.dimension = wgpu::TextureViewDimension::e2D;
  textureViewDesc.format = textureDesc.format;
  textureViewDesc.baseMipLevel = 0;
  textureViewDesc.mipLevelCount = 1;
  textureViewDesc.baseArrayLayer = 0;
  textureViewDesc.arrayLayerCount = 1;

  this->DepthStencil.Texture = this->Device.CreateTexture(&textureDesc);
  this->DepthStencil.View = this->DepthStencil.Texture.CreateView(&textureViewDesc);
  this->DepthStencil.Format = textureDesc.format;
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
    wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::CopyDst;
  textureDesc.viewFormatCount = 0;
  textureDesc.viewFormats = nullptr;

  this->ColorAttachment.Texture = this->Device.CreateTexture(&textureDesc);

  // view
  wgpu::TextureViewDescriptor textureViewDesc;
  textureViewDesc.dimension = wgpu::TextureViewDimension::e2D;
  textureViewDesc.format = textureDesc.format;
  textureViewDesc.baseMipLevel = 0;
  textureViewDesc.mipLevelCount = 1;
  textureViewDesc.baseArrayLayer = 0;
  textureViewDesc.arrayLayerCount = 1;

  this->ColorAttachment.View = this->ColorAttachment.Texture.CreateView(&textureViewDesc);
  this->ColorAttachment.Format = textureDesc.format;

  const auto alignedWidth =
    vtkWGPUContext::Align(4 * this->ColorAttachment.Texture.GetWidth(), 256);

  wgpu::BufferDescriptor buffDesc;
  buffDesc.label = "Offscreen buffer";
  buffDesc.mappedAtCreation = false;
  buffDesc.size = this->ColorAttachment.Texture.GetHeight() * alignedWidth;
  buffDesc.usage = wgpu::BufferUsage::MapRead | wgpu::BufferUsage::CopyDst;

  this->ColorAttachment.OffscreenBuffer = this->Device.CreateBuffer(&buffDesc);
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::DestroyOffscreenColorAttachments()
{
  this->ColorAttachment.OffscreenBuffer.Destroy();
  this->ColorAttachment.OffscreenBuffer = nullptr;
  this->ColorAttachment.View = nullptr;
  this->ColorAttachment.Texture = nullptr;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::CreateFSQGraphicsPipeline()
{
  wgpu::BindGroupLayout bgl = vtkWebGPUInternalsBindGroupLayout::MakeBindGroupLayout(this->Device,
    {
      // clang-format off
    { 0, wgpu::ShaderStage::Fragment, wgpu::TextureSampleType::Float, wgpu::TextureViewDimension::e2D, /*multiSampled=*/false }
      // clang-format on
    });
  bgl.SetLabel("FSQ bind group layout");

  wgpu::PipelineLayout pipelineLayout =
    vtkWebGPUInternalsPipelineLayout::MakeBasicPipelineLayout(this->Device, &bgl);
  pipelineLayout.SetLabel("FSQ graphics pipeline layout");

  this->FSQ.BindGroup = vtkWebGPUInternalsBindGroup::MakeBindGroup(this->Device, bgl,
    {
      // clang-formt off
      { 0, this->ColorAttachment.View }
      // clang-format on
    });

  wgpu::ShaderModule shaderModule = vtkWebGPUInternalsShaderModule::CreateFromWGSL(this->Device, R"(
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
      let color = textureLoad(fsqTexture, vec2<i32>(fragment.position.xy), 0).rgba;
      return vec4<f32>(color);
    }
  )");

  vtkWebGPUInternalsRenderPipelineDescriptor pipelineDesc;
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

  this->FSQ.Pipeline = this->Device.CreateRenderPipeline(&pipelineDesc);
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::DestroyFSQGraphicsPipeline()
{
  this->FSQ.BindGroup = nullptr;
  this->FSQ.Pipeline = nullptr;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::RenderOffscreenTexture()
{
  // prepare the offscreen texture for presentation.
  this->SwapChain.Framebuffer = this->SwapChain.Instance.GetCurrentTextureView();

  vtkWebGPUInternalsRenderPassDescriptor renderPassDescriptor({ this->SwapChain.Framebuffer });
  renderPassDescriptor.label = "Render offscreen texture";

  for (auto& colorAttachment : renderPassDescriptor.ColorAttachments)
  {
    colorAttachment.clearValue.r = 0.0;
    colorAttachment.clearValue.g = 0.0;
    colorAttachment.clearValue.b = 0.0;
    colorAttachment.clearValue.a = 1.0f;
  }
  auto encoder = this->NewRenderPass(renderPassDescriptor);
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
    vtkWGPUContext::Align(4 * this->ColorAttachment.Texture.GetWidth(), 256);
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
  wgpu::Queue queue = this->Device.GetQueue();
  queue.Submit(count, buffers);
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::Start()
{
  int* size = this->GetSize();
  vtkDebugMacro(<< __func__ << '(' << size[0] << ',' << size[1] << ')');
  this->Size[0] = (size[0] > 0 ? size[0] : 300);
  this->Size[1] = (size[1] > 0 ? size[1] : 300);

  if (!this->WGPUInitialized)
  {
    this->WGPUInitialized = this->Initialize(); // calls WGPUInit after surface is created.
    this->CreateSwapChain();
    this->CreateOffscreenColorAttachments();
    this->CreateDepthStencilTexture();
    this->CreateFSQGraphicsPipeline();
  }
  else if (this->Size[0] != this->SwapChain.Width || this->Size[1] != this->SwapChain.Height)
  {
    // Recreate if size changed
    this->DestroyFSQGraphicsPipeline();
    this->DestroyDepthStencilTexture();
    this->DestroyOffscreenColorAttachments();
    this->DestroySwapChain();
    this->CreateSwapChain();
    this->CreateOffscreenColorAttachments();
    this->CreateDepthStencilTexture();
    this->CreateFSQGraphicsPipeline();
  }

  if (!this->WGPUInitialized)
  {
    vtkErrorMacro(<< "Failed to initialize WebGPU!");
  }

  wgpu::CommandEncoderDescriptor encDesc = {};
  std::stringstream label;
  encDesc.label = "vtkWebGPURenderWindow::Start";

  this->CommandEncoder = this->Device.CreateCommandEncoder(&encDesc);
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::Frame()
{
  vtkDebugMacro(<< __func__);
  this->Superclass::Frame();

  this->RenderOffscreenTexture();

  wgpu::CommandBufferDescriptor cmdBufDesc = {};
  wgpu::CommandBuffer cmdBuffer = this->CommandEncoder.Finish(&cmdBufDesc);

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
  // This lets the implementation execute all callbacks so that validation errors are output in the
  // console.
  vtkWGPUContext::WaitABit();
#endif
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::End()
{
  vtkDebugMacro(<< __func__);

  // If user called SetPixelData or it's variant, source our offscreen texture from that data.
  if (this->StagingPixelData.Buffer.Get() != nullptr)
  {
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
        assert(mapped != nullptr);
        // allocate sufficient space on host.
        ctx->dst->SetNumberOfValues(ctx->size);
        // These are plain bytes. GetABCDPixelData() functions know how to interpret them.
        std::copy(mapped, mapped + ctx->size, ctx->dst->GetPointer(0));
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
  (void)front;
  (void)right;
  const int nComp = 3;
  int width = (x2 - x) + 1;
  int height = (y2 - y) + 1;
  int bytesPerRow = vtkWGPUContext::Align(width * nComp, 256);
  int size = bytesPerRow * height;

  wgpu::BufferDescriptor desc;
  desc.mappedAtCreation = true;
  desc.label = "Staging buffer for SetPixelData";
  desc.size = size;
  desc.usage = wgpu::BufferUsage::CopySrc;

  this->StagingPixelData.Buffer = this->Device.CreateBuffer(&desc);
  auto mapped =
    reinterpret_cast<unsigned char*>(this->StagingPixelData.Buffer.GetMappedRange(0, size));
  assert(mapped != nullptr);
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
  (void)front;
  (void)blend;
  (void)right;
  const int nComp = 4;
  int width = (x2 - x) + 1;
  int height = (y2 - y) + 1;
  int bytesPerRow = vtkWGPUContext::Align(width * nComp, 256);
  int size = bytesPerRow * height;

  wgpu::BufferDescriptor desc;
  desc.mappedAtCreation = true;
  desc.label = "Staging buffer for SetRGBAPixelData";
  desc.size = size;
  desc.usage = wgpu::BufferUsage::CopySrc;

  this->StagingPixelData.Buffer = this->Device.CreateBuffer(&desc);
  auto mapped =
    reinterpret_cast<unsigned char*>(this->StagingPixelData.Buffer.GetMappedRange(0, size));
  assert(mapped != nullptr);
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
  (void)front;
  (void)blend;
  (void)right;
  const int nComp = 4;
  int width = (x2 - x) + 1;
  int height = (y2 - y) + 1;
  int bytesPerRow = vtkWGPUContext::Align(width * nComp, 256);
  int size = bytesPerRow * height;

  wgpu::BufferDescriptor desc;
  desc.mappedAtCreation = true;
  desc.label = "Staging buffer for SetRGBACharPixelData";
  desc.size = size;
  desc.usage = wgpu::BufferUsage::CopySrc;

  this->StagingPixelData.Buffer = this->Device.CreateBuffer(&desc);
  auto mapped =
    reinterpret_cast<unsigned char*>(this->StagingPixelData.Buffer.GetMappedRange(0, size));
  assert(mapped != nullptr);
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
  bool done = false;
  this->Device.GetQueue().OnSubmittedWorkDone(
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
    vtkWGPUContext::WaitABit();
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
void vtkWebGPURenderWindow::ReleaseGraphicsResources(vtkWindow*) {}

VTK_ABI_NAMESPACE_END
