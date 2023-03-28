/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWebGPURenderWindow.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkWebGPURenderWindow.h"
#include "vtkObject.h"
#include "vtkObjectFactory.h"
#include "vtkUnsignedCharArray.h"
#include "vtkWGPUContext.h"
#include "vtkWebGPUClearPass.h"
#include "vtkWebGPUInternalsBindGroup.h"
#include "vtkWebGPUInternalsBindGroupLayout.h"
#include "vtkWebGPUInternalsPipelineLayout.h"
#include "vtkWebGPUInternalsRenderPassCreateInfo.h"
#include "vtkWebGPUInternalsRenderPipelineDescriptor.h"
#include "vtkWebGPUInternalsShaderModule.h"

#include <exception>
#include <sstream>

VTK_ABI_NAMESPACE_BEGIN

namespace
{
void print_wgpu_error(WGPUErrorType error_type, const char* message, void* self)
{
  const char* error_type_lbl = "";
  switch (error_type)
  {
    case WGPUErrorType_Validation:
      error_type_lbl = "Validation";
      break;
    case WGPUErrorType_OutOfMemory:
      error_type_lbl = "Out of memory";
      break;
    case WGPUErrorType_Unknown:
      error_type_lbl = "Unknown";
      break;
    case WGPUErrorType_DeviceLost:
      error_type_lbl = "Device lost";
      break;
    default:
      error_type_lbl = "Unknown";
  }
  vtkErrorWithObjectMacro(
    reinterpret_cast<vtkObject*>(self), << error_type_lbl << "Error: " << message);
}
void device_lost_callback(WGPUDeviceLostReason reason, char const* message, void* self)
{

  const char* reason_type_lbl = "";
  switch (reason)
  {
    case WGPUDeviceLostReason_Destroyed:
      reason_type_lbl = "Destroyed";
      break;
    case WGPUDeviceLostReason_Undefined:
      reason_type_lbl = "Undefined";
      break;
    default:
      reason_type_lbl = "Unknown";
  }
  vtkErrorWithObjectMacro(
    reinterpret_cast<vtkObject*>(self), << reason_type_lbl << "Device lost! Reason : " << message);
}
}

//------------------------------------------------------------------------------
vtkWebGPURenderWindow::vtkWebGPURenderWindow() = default;

//------------------------------------------------------------------------------
vtkWebGPURenderWindow::~vtkWebGPURenderWindow() = default;

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::PrintSelf(ostream& os, vtkIndent indent) {}

//------------------------------------------------------------------------------
bool vtkWebGPURenderWindow::WGPUInit()
{
  vtkDebugMacro(<< __func__ << " WGPUInitialized=" << this->WGPUInitialized);
  vtkWGPUContext::LogAvailableAdapters();
  ///@{ TODO: CLEAN UP DEVICE ACQUISITION
  // for emscripten, the glue code is expected to pre-initialize an instance, adapter and a device.
  wgpu::RequestAdapterOptions options;
  options.powerPreference = this->PowerPreference;
  this->Adapter = vtkWGPUContext::RequestAdapter(options);
  wgpu::DeviceDescriptor deviceDescriptor = {};
  deviceDescriptor.label = "vtkWebGPURenderWindow::WGPUInit";
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
  this->Device.SetUncapturedErrorCallback(&::print_wgpu_error, this);
  this->Device.SetDeviceLostCallback(&::device_lost_callback, this);
  ///@}
  return true;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::WGPUFinalize()
{
  vtkDebugMacro(<< __func__ << " WGPUInitialized=" << this->WGPUInitialized);
  this->DestroyDepthStencilTexture();
  this->DestroySwapChain();
  this->Device.Release();
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::Render()
{
  vtkDebugMacro(<< __func__);
  this->Superclass::Render();
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
  label << "vtkWebGPURenderWindow::" << __func__ << '(' << size[0] << ',' << size[1] << ')';
  encDesc.label = label.str().c_str();

  this->CommandEncoder = this->Device.CreateCommandEncoder(&encDesc);
  this->CommandEncoder.InsertDebugMarker(__func__);
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
  this->SwapChain.Instance.Release();
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
  this->DepthStencil.View.Release();
  this->DepthStencil.Texture.Release();
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::CreateOffscreenColorAttachments()
{
  // must match swapchain's dimensions as we'll eventually sample from this, when the time is right.
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
    wgpu::TextureUsage::CopySrc;
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

  auto alignedWidth = this->ColorAttachment.Texture.GetWidth() * 4;
  alignedWidth = (alignedWidth + 255) & (-256);

  wgpu::BufferDescriptor buffDesc;
  buffDesc.label = "Offscreen buffer";
  buffDesc.mappedAtCreation = false;
  buffDesc.size = this->ColorAttachment.Texture.GetHeight() * alignedWidth;
  buffDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead;

  this->ColorAttachment.OffscreenBuffer = this->Device.CreateBuffer(&buffDesc);
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::DestroyOffscreenColorAttachments()
{
  this->ColorAttachment.View.Release();
  this->ColorAttachment.Texture.Release();
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
  this->FSQ.BindGroup.Release();
  this->FSQ.Pipeline.Release();
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::FlushCommandBuffers(vtkTypeUInt32 count, wgpu::CommandBuffer* buffers)
{
  vtkDebugMacro(<< __func__ << "count=" << count);
  wgpu::Queue queue = this->Device.GetQueue();
  queue.Submit(count, buffers);
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::Frame()
{
  vtkDebugMacro(<< __func__);
  this->Superclass::Frame();
  // On web, html5 `requestAnimateFrame` takes care of presentation.
#ifndef __EMSCRIPTEN__
  this->SwapChain.Instance.Present();
#endif
  this->SwapChain.Framebuffer.Release();
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::End()
{
  vtkDebugMacro(<< __func__);
  this->CommandEncoder.InsertDebugMarker(__func__);

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
  encoder.SetLabel("Encoding offscreen texture render commands");
  encoder.SetViewport(0, 0, this->SwapChain.Width, this->SwapChain.Height, 0.0, 1.0);
  encoder.SetScissorRect(0, 0, this->SwapChain.Width, this->SwapChain.Height);
  // set fsq pipeline
  encoder.PushDebugGroup("FSQ Render");
  encoder.SetPipeline(this->FSQ.Pipeline);
  // bind fsq group
  encoder.SetBindGroup(0, this->FSQ.BindGroup);
  // draw triangle strip
  encoder.Draw(4);
  encoder.PopDebugGroup();
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
  textureDataLayout.bytesPerRow = 4 * this->ColorAttachment.Texture.GetWidth();
  textureDataLayout.bytesPerRow = (textureDataLayout.bytesPerRow + 255) & (-256);
  textureDataLayout.rowsPerImage = this->ColorAttachment.Texture.GetHeight();

  wgpu::ImageCopyBuffer copyDst;
  copyDst.buffer = this->ColorAttachment.OffscreenBuffer;
  copyDst.layout = textureDataLayout;

  this->CommandEncoder.PushDebugGroup("Copy color attachment to offscreen buffer");
  this->CommandEncoder.CopyTextureToBuffer(&copySrc, &copyDst, &srcExtent);
  this->CommandEncoder.PopDebugGroup();

  wgpu::CommandBufferDescriptor cmdBufDesc = {};
  wgpu::CommandBuffer cmdBuffer = this->CommandEncoder.Finish(&cmdBufDesc);

  this->CommandEncoder.Release();

  this->FlushCommandBuffers(1, &cmdBuffer);
#ifndef NDEBUG
  // This lets the implementation execute all callbacks so that validation errors are output in the
  // console.
  vtkWGPUContext::WaitABit();
#endif
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
  int x, int y, int x2, int y2, int front, int right)
{
}

//------------------------------------------------------------------------------
int vtkWebGPURenderWindow::GetPixelData(
  int x, int y, int x2, int y2, int front, vtkUnsignedCharArray* data, int right)
{
  // struct MappingContext
  // {
  //   vtkUnsignedCharArray* data;
  //   vtkWebGPURenderWindow* self;
  //   std::atomic<bool> finished{ false };
  // } mapCtx;
  // mapCtx.data = data;
  // mapCtx.self = this;
  // this->ColorAttachment.OffscreenBuffer.MapAsync(
  //   wgpu::MapMode::Read, 0, WGPU_WHOLE_MAP_SIZE,
  //   [](WGPUBufferMapAsyncStatus status, void* userdata) {
  //     auto mapCtx = reinterpret_cast<MappingContext*>(userdata);
  //     std::cout << "Mapped !";
  //     if (status == WGPUBufferMapAsyncStatus_Success)
  //     {
  //       auto mapped =
  //         mapCtx->self->ColorAttachment.OffscreenBuffer.GetConstMappedRange(0,
  //         WGPU_WHOLE_MAP_SIZE);
  //       auto mappedAsU8 = reinterpret_cast<const vtkTypeUInt8*>(mapped);
  //       int size = mapCtx->self->ColorAttachment.OffscreenBuffer.GetSize();
  //       mapCtx->data->SetNumberOfValues(mapCtx->self->ColorAttachment.OffscreenBuffer.GetSize());
  //       std::copy(
  //         mappedAsU8, mappedAsU8 + mapCtx->data->GetNumberOfValues(), mapCtx->data->Begin());
  //       mapCtx->self->ColorAttachment.OffscreenBuffer.Unmap();
  //     }
  //     mapCtx->finished = true;
  //   },
  //   &mapCtx);
  return 0;
}

//------------------------------------------------------------------------------
int vtkWebGPURenderWindow::SetPixelData(
  int x, int y, int x2, int y2, unsigned char* data, int front, int right)
{
}

//------------------------------------------------------------------------------
int vtkWebGPURenderWindow::SetPixelData(
  int x, int y, int x2, int y2, vtkUnsignedCharArray* data, int front, int right)
{
}

//------------------------------------------------------------------------------
float* vtkWebGPURenderWindow::GetRGBAPixelData(
  int x, int y, int x2, int y2, int front, int right /*=0*/)
{
}

//------------------------------------------------------------------------------
int vtkWebGPURenderWindow::GetRGBAPixelData(
  int x, int y, int x2, int y2, int front, vtkFloatArray* data, int right /*=0*/)
{
}

//------------------------------------------------------------------------------
int vtkWebGPURenderWindow::SetRGBAPixelData(
  int x, int y, int x2, int y2, float* data, int front, int blend /*=0*/, int right /*=0*/)
{
}

//------------------------------------------------------------------------------
int vtkWebGPURenderWindow::SetRGBAPixelData(
  int x, int y, int x2, int y2, vtkFloatArray* data, int front, int blend /*=0*/, int right /*=0*/)
{
}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::ReleaseRGBAPixelData(float* data) {}
unsigned char* vtkWebGPURenderWindow::GetRGBACharPixelData(
  int x, int y, int x2, int y2, int front, int right /*=0*/)
{
}

//------------------------------------------------------------------------------
int vtkWebGPURenderWindow::GetRGBACharPixelData(
  int x, int y, int x2, int y2, int front, vtkUnsignedCharArray* data, int right /*=0*/)
{
}

//------------------------------------------------------------------------------
int vtkWebGPURenderWindow::SetRGBACharPixelData(
  int x, int y, int x2, int y2, unsigned char* data, int front, int blend /*=0*/, int right /*=0*/)
{
}

//------------------------------------------------------------------------------
int vtkWebGPURenderWindow::SetRGBACharPixelData(int x, int y, int x2, int y2,
  vtkUnsignedCharArray* data, int front, int blend /*=0*/, int right /*=0*/)
{
}

//------------------------------------------------------------------------------
float* vtkWebGPURenderWindow::GetZbufferData(int x1, int y1, int x2, int y2) {}

//------------------------------------------------------------------------------
int vtkWebGPURenderWindow::GetZbufferData(int x1, int y1, int x2, int y2, float* z) {}

//------------------------------------------------------------------------------
int vtkWebGPURenderWindow::GetZbufferData(int x1, int y1, int x2, int y2, vtkFloatArray* buffer) {}

//------------------------------------------------------------------------------
int vtkWebGPURenderWindow::SetZbufferData(int x1, int y1, int x2, int y2, float* buffer) {}

//------------------------------------------------------------------------------
int vtkWebGPURenderWindow::SetZbufferData(int x1, int y1, int x2, int y2, vtkFloatArray* buffer) {}

//------------------------------------------------------------------------------
int vtkWebGPURenderWindow::GetColorBufferSizes(int* rgba) {}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::WaitForCompletion()
{
  bool done = false;
  this->Device.GetQueue().OnSubmittedWorkDone(
    0u, [](WGPUQueueWorkDoneStatus, void* userdata) { *static_cast<bool*>(userdata) = true; },
    &done);
  while (!done)
  {
    vtkWGPUContext::WaitABit();
  }
}

//------------------------------------------------------------------------------
int vtkWebGPURenderWindow::SupportsOpenGL() {}

//------------------------------------------------------------------------------
const char* vtkWebGPURenderWindow::ReportCapabilities() {}

//------------------------------------------------------------------------------
bool vtkWebGPURenderWindow::InitializeFromCurrentContext() {}

//------------------------------------------------------------------------------
void vtkWebGPURenderWindow::ReleaseGraphicsResources(vtkWindow*) {}

VTK_ABI_NAMESPACE_END
