// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "Private/vtkWebGPUPointCloudMapperInternals.h"
#include "PointCloudMapperCopyDepthFromWindow.h"
#include "PointCloudMapperCopyDepthToWindow.h"
#include "PointCloudMapperShader.h"
#include "Private/vtkWebGPUBindGroupInternals.h"
#include "Private/vtkWebGPUBindGroupLayoutInternals.h"
#include "Private/vtkWebGPUBufferInternals.h"
#include "Private/vtkWebGPUComputePassInternals.h"
#include "Private/vtkWebGPUPipelineLayoutInternals.h"
#include "Private/vtkWebGPURenderPassDescriptorInternals.h"
#include "Private/vtkWebGPURenderPipelineDescriptorInternals.h"
#include "Private/vtkWebGPUShaderModuleInternals.h"
#include "vtkCamera.h"
#include "vtkMatrix4x4.h"
#include "vtkPointData.h"
#include "vtkRenderer.h"
#include "vtkWebGPUCommandEncoderDebugGroup.h"
#include "vtkWebGPUComputeBuffer.h"
#include "vtkWebGPUComputePass.h"
#include "vtkWebGPUComputePipeline.h"
#include "vtkWebGPURenderWindow.h"

#include <vector>

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkWebGPUPointCloudMapperInternals);

//------------------------------------------------------------------------------
vtkWebGPUPointCloudMapperInternals::vtkWebGPUPointCloudMapperInternals() {}

//------------------------------------------------------------------------------
vtkWebGPUPointCloudMapperInternals::~vtkWebGPUPointCloudMapperInternals() {}

//------------------------------------------------------------------------------
void vtkWebGPUPointCloudMapperInternals::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "Compute pipeline: ";
  this->ComputePipeline->PrintSelf(os, indent);

  os << indent << "Copy depth pass: ";
  this->CopyDepthPass->PrintSelf(os, indent);

  os << indent << "Render point pass: ";
  this->RenderPointsPass->PrintSelf(os, indent);

  os << indent << "Point depth buffer: ";
  this->PointDepthBuffer->PrintSelf(os, indent);

  os << indent << "PointBufferIndex: " << this->PointBufferIndex << std::endl;
  os << indent << "PointColorBufferIndex: " << this->PointColorBufferIndex << std::endl;

  os << indent << "PointDepthBufferIndex: " << this->PointDepthBufferIndex << std::endl;
  os << indent << "CameraVPBufferIndex: " << this->CameraVPBufferIndex << std::endl;
  os << indent << "FrameBufferRenderTextureIndex: " << this->FrameBufferRenderTextureIndex
     << std::endl;
}

//------------------------------------------------------------------------------
void vtkWebGPUPointCloudMapperInternals::SetMapper(vtkWebGPUComputePointCloudMapper* mapper)
{
  this->ParentMapper = mapper;
}

//------------------------------------------------------------------------------
vtkWebGPURenderWindow* vtkWebGPUPointCloudMapperInternals::GetRendererRenderWindow(
  vtkRenderer* renderer)
{
  vtkRenderWindow* renderWindow = renderer->GetRenderWindow();
  vtkWebGPURenderWindow* wgpuRenderWindow = vtkWebGPURenderWindow::SafeDownCast(renderWindow);

  if (wgpuRenderWindow == nullptr)
  {
    vtkErrorWithObjectMacro(this,
      "The renderer given in GetRendererRenderWindow doesn't "
      "belong to a WebGPURenderWindow.");

    return nullptr;
  }

  if (!wgpuRenderWindow->GetInitialized())
  {
    vtkErrorWithObjectMacro(this,
      "The render window of the given renderer in GetRendererRenderWindow "
      "hasn't been initialized. Did you forget to call vtkRenderWindow::Initialize()?");

    return nullptr;
  }

  return wgpuRenderWindow;
}

//------------------------------------------------------------------------------
void vtkWebGPUPointCloudMapperInternals::UpdateRenderWindowDepthBuffer(vtkRenderer* renderer)
{
  vtkWebGPURenderWindow* wgpuRenderWindow = this->GetRendererRenderWindow(renderer);
  if (wgpuRenderWindow == nullptr)
  {
    return;
  }

  if (this->CopyDepthBufferPipeline.Pipeline == nullptr)
  {
    this->CreateCopyDepthBufferRenderPipeline(wgpuRenderWindow);
  }

  this->CopyDepthBufferToRenderWindow(wgpuRenderWindow);
}

//------------------------------------------------------------------------------
void vtkWebGPUPointCloudMapperInternals::CreateCopyDepthBufferRenderPipeline(
  vtkWebGPURenderWindow* wgpuRenderWindow)
{
  wgpu::Device device = wgpuRenderWindow->GetDevice();

  // Creating the buffer that will hold the width of the framebuffer for the fragment shader that
  // copies the point depth buffer into the depth buffer of the render window
  this->CopyDepthBufferPipeline.FramebufferWidthUniformBuffer =
    wgpuRenderWindow->GetWGPUConfiguration()->CreateBuffer(sizeof(unsigned int),
      wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform, false,
      "Point cloud mapper - Copy depth to RenderWindow - Framebuffer width uniform buffer");

  wgpu::BindGroupLayout bgl = vtkWebGPUBindGroupLayoutInternals::MakeBindGroupLayout(device,
    {
      { 0, wgpu::ShaderStage::Fragment, wgpu::BufferBindingType::ReadOnlyStorage },
      { 1, wgpu::ShaderStage::Fragment, wgpu::BufferBindingType::Uniform },
    });
  bgl.SetLabel("FSQ bind group layout");

  wgpu::PipelineLayout pipelineLayout =
    vtkWebGPUPipelineLayoutInternals::MakeBasicPipelineLayout(device, &bgl);
  pipelineLayout.SetLabel("FSQ graphics pipeline layout");

  auto bufferStorage = this->CopyDepthPass->Internals->BufferStorage;
  this->CopyDepthBufferPipeline.BindGroup = vtkWebGPUBindGroupInternals::MakeBindGroup(device, bgl,
    {
      { 0, bufferStorage->GetWGPUBuffer(this->PointDepthBufferIndex) },
      { 1, this->CopyDepthBufferPipeline.FramebufferWidthUniformBuffer },
    });

  wgpu::ShaderModule shaderModule =
    vtkWebGPUShaderModuleInternals::CreateFromWGSL(device, PointCloudMapperCopyDepthToWindow);

  vtkWebGPURenderPipelineDescriptorInternals pipelineDesc;
  pipelineDesc.label = "Point cloud mapper - Copy point depth buffer graphics pipeline description";
  pipelineDesc.layout = pipelineLayout;
  pipelineDesc.vertex.module = shaderModule;
  pipelineDesc.vertex.entryPoint = "vertexMain";
  pipelineDesc.vertex.bufferCount = 0;
  pipelineDesc.cFragment.module = shaderModule;
  pipelineDesc.cFragment.entryPoint = "fragmentMain";
  // We are not going to use the color target but Dawn needs it
  pipelineDesc.cFragment.targetCount = 1;
  pipelineDesc.cTargets[0].format = wgpuRenderWindow->GetPreferredSurfaceTextureFormat();
  // Not writing to the color attachment
  pipelineDesc.cTargets[0].writeMask = wgpu::ColorWriteMask::None;

  // Enabling the depth buffer
  wgpu::DepthStencilState* depthState = pipelineDesc.EnableDepthStencil();
  depthState->depthWriteEnabled = true;
  depthState->depthCompare = wgpu::CompareFunction::Less;
  pipelineDesc.primitive.topology = wgpu::PrimitiveTopology::TriangleStrip;

  this->CopyDepthBufferPipeline.Pipeline = device.CreateRenderPipeline(&pipelineDesc);
}

//------------------------------------------------------------------------------
void vtkWebGPUPointCloudMapperInternals::CopyDepthBufferToRenderWindow(
  vtkWebGPURenderWindow* wgpuRenderWindow)
{
  std::vector<wgpu::TextureView> colorAttachment;
  colorAttachment.push_back(wgpuRenderWindow->GetOffscreenColorAttachmentView());

  vtkWebGPURenderPassDescriptorInternals renderPassDescriptor(colorAttachment,
    wgpuRenderWindow->GetDepthStencilView(),
    /* Don't clear the color/depth buffer with this pass */ false);
  // Discarding anything that we write to the color attachment
  renderPassDescriptor.ColorAttachments[0].storeOp = wgpu::StoreOp::Store;

  int* windowSize = wgpuRenderWindow->GetSize();

  wgpu::Device device = wgpuRenderWindow->GetDevice();
  wgpuRenderWindow->GetWGPUConfiguration()->WriteBuffer(
    this->CopyDepthBufferPipeline.FramebufferWidthUniformBuffer, 0, (uint8_t*)&windowSize[0],
    sizeof(unsigned int));

  wgpu::CommandEncoderDescriptor encDesc;
  encDesc.label = "vtkWebGPURenderWindow::CommandEncoder";
  wgpu::CommandEncoder commandEncoder = device.CreateCommandEncoder(&encDesc);

  auto encoder = commandEncoder.BeginRenderPass(&renderPassDescriptor);
  encoder.SetLabel("Point cloud mapper - Encode copy point depth buffer to render window");
  encoder.SetViewport(0, 0, windowSize[0], windowSize[1], 0.0, 1.0);
  encoder.SetScissorRect(0, 0, windowSize[0], windowSize[1]);
  {
    vtkScopedEncoderDebugGroup(
      encoder, "Point cloud mapper - Copy point depth buffer to render window");
    encoder.SetPipeline(this->CopyDepthBufferPipeline.Pipeline);
    encoder.SetBindGroup(0, this->CopyDepthBufferPipeline.BindGroup);
    encoder.Draw(4);
  }
  encoder.End();

  wgpu::CommandBufferDescriptor cmdBufDesc;
  wgpu::CommandBuffer cmdBuffer = commandEncoder.Finish(&cmdBufDesc);
  wgpuRenderWindow->FlushCommandBuffers(1, &cmdBuffer);
}

//------------------------------------------------------------------------------
void vtkWebGPUPointCloudMapperInternals::Initialize(vtkRenderer* renderer)
{
  if (this->Initialized)
  {
    // Already initialized

    return;
  }

  this->ComputePipeline.TakeReference(vtkWebGPUComputePipeline::New());

  this->CopyDepthPass = this->ComputePipeline->CreateComputePass();
  this->CopyDepthPass->SetShaderSource(PointCloudMapperCopyDepthFromWindow);
  this->CopyDepthPass->SetShaderEntryPoint("computeMain");

  this->RenderPointsPass = this->ComputePipeline->CreateComputePass();
  this->RenderPointsPass->SetShaderSource(PointCloudMapperShader);
  this->RenderPointsPass->SetShaderEntryPoint("pointCloudRenderEntryPoint");

  this->UseRenderWindowDevice(renderer);
  this->InitializeDepthCopyPass(renderer);
  this->InitializePointRenderPass(renderer);

  this->Initialized = true;
}

//------------------------------------------------------------------------------
void vtkWebGPUPointCloudMapperInternals::Update(vtkRenderer* renderer)
{
  this->ResizeToRenderWindow(renderer);
}

//------------------------------------------------------------------------------
void vtkWebGPUPointCloudMapperInternals::UseRenderWindowDevice(vtkRenderer* renderer)
{
  vtkWebGPURenderWindow* wgpuRenderWindow = this->GetRendererRenderWindow(renderer);
  if (wgpuRenderWindow == nullptr)
  {
    return;
  }

  this->ComputePipeline->SetWGPUConfiguration(wgpuRenderWindow->GetWGPUConfiguration());
}

//------------------------------------------------------------------------------
void vtkWebGPUPointCloudMapperInternals::ResizeToRenderWindow(vtkRenderer* renderer)
{
  vtkWebGPURenderWindow* wgpuRenderWindow = this->GetRendererRenderWindow(renderer);
  if (wgpuRenderWindow == nullptr)
  {
    return;
  }

  int* windowSize = wgpuRenderWindow->GetSize();

  if (this->CopyDepthPass->GetBufferByteSize(this->PointDepthBufferIndex) ==
    windowSize[0] * windowSize[1] * sizeof(unsigned int))
  {
    // Nothing to resize

    return;
  }

  this->CopyDepthPass->ResizeBuffer(
    this->PointDepthBufferIndex, windowSize[0] * windowSize[1] * sizeof(unsigned int));

  int nbGroupsX = std::ceil(windowSize[0] / 8.0f);
  int nbGroupsY = std::ceil(windowSize[1] / 8.0f);
  this->CopyDepthPass->SetWorkgroups(nbGroupsX, nbGroupsY, 1);
}

//------------------------------------------------------------------------------
void vtkWebGPUPointCloudMapperInternals::InitializeDepthCopyPass(vtkRenderer* renderer)
{
  vtkWebGPURenderWindow* wgpuRenderWindow = this->GetRendererRenderWindow(renderer);
  if (wgpuRenderWindow == nullptr)
  {
    return;
  }

  int* windowSize = wgpuRenderWindow->GetSize();

  vtkSmartPointer<vtkWebGPUComputeRenderTexture> depthBufferRenderTexture =
    wgpuRenderWindow->AcquireDepthBufferRenderTexture();
  int depthBufferRenderTextureIndex =
    this->CopyDepthPass->AddRenderTexture(depthBufferRenderTexture);

  vtkSmartPointer<vtkWebGPUComputeTextureView> depthBufferTextureView =
    this->CopyDepthPass->CreateTextureView(depthBufferRenderTextureIndex);
  depthBufferTextureView->SetGroup(0);
  depthBufferTextureView->SetBinding(0);
  depthBufferTextureView->SetAspect(vtkWebGPUComputeTextureView::TextureViewAspect::ASPECT_DEPTH);
  depthBufferTextureView->SetMode(vtkWebGPUComputeTextureView::TextureViewMode::READ_ONLY);
  depthBufferTextureView->SetFormat(vtkWebGPUComputeTexture::DEPTH_24_PLUS);
  depthBufferTextureView->SetLabel("Point cloud mapper - point render pass - depth buffer");
  this->CopyDepthPass->AddTextureView(depthBufferTextureView);

  this->PointDepthBuffer = vtkSmartPointer<vtkWebGPUComputeBuffer>::New();
  this->PointDepthBuffer->SetGroup(0);
  this->PointDepthBuffer->SetBinding(1);
  this->PointDepthBuffer->SetByteSize(windowSize[0] * windowSize[1] * sizeof(unsigned int));
  this->PointDepthBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::READ_WRITE_COMPUTE_STORAGE);
  this->PointDepthBufferIndex = this->CopyDepthPass->AddBuffer(this->PointDepthBuffer);

  int nbGroupsX = std::ceil(windowSize[0] / 8.0f);
  int nbGroupsY = std::ceil(windowSize[1] / 8.0f);
  this->CopyDepthPass->SetWorkgroups(nbGroupsX, nbGroupsY, 1);
  this->CopyDepthPass->SetLabel("Point cloud mapper - Depth buffer copy pass");
}

//------------------------------------------------------------------------------
void vtkWebGPUPointCloudMapperInternals::InitializePointRenderPass(vtkRenderer* renderer)
{
  vtkWebGPURenderWindow* wgpuRenderWindow = this->GetRendererRenderWindow(renderer);
  if (wgpuRenderWindow == nullptr)
  {
    return;
  }

  auto colorFramebuffer = wgpuRenderWindow->AcquireFramebufferRenderTexture();

  vtkSmartPointer<vtkWebGPUComputeBuffer> pointBuffer;
  pointBuffer = vtkSmartPointer<vtkWebGPUComputeBuffer>::New();
  pointBuffer->SetGroup(0);
  pointBuffer->SetBinding(0);
  // Will be resized when the polydata will be set on this point cloud renderer
  pointBuffer->SetByteSize(1);
  pointBuffer->SetDataType(vtkWebGPUComputeBuffer::BufferDataType::STD_VECTOR);
  pointBuffer->SetLabel("Point cloud mapper - point render pass - point buffer");
  pointBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::READ_ONLY_COMPUTE_STORAGE);
  this->PointBufferIndex = this->RenderPointsPass->AddBuffer(pointBuffer);

  // Binding (0, 1)
  this->RenderPointsPass->AddBuffer(this->PointDepthBuffer);

  vtkSmartPointer<vtkWebGPUComputeBuffer> pointColorBuffer;
  pointColorBuffer = vtkSmartPointer<vtkWebGPUComputeBuffer>::New();
  pointColorBuffer->SetGroup(0);
  pointColorBuffer->SetBinding(2);
  pointColorBuffer->SetDataType(vtkWebGPUComputeBuffer::BufferDataType::STD_VECTOR);
  // Dummy size. Will be resized when setting the polydata
  pointColorBuffer->SetByteSize(4);
  pointColorBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::READ_ONLY_COMPUTE_STORAGE);
  pointColorBuffer->SetLabel("Point cloud mapper - point render pass - point color buffer");
  this->PointColorBufferIndex = this->RenderPointsPass->AddBuffer(pointColorBuffer);

  int framebufferIndex = this->RenderPointsPass->AddRenderTexture(colorFramebuffer);
  vtkSmartPointer<vtkWebGPUComputeTextureView> framebufferTextureView =
    this->RenderPointsPass->CreateTextureView(framebufferIndex);
  framebufferTextureView->SetGroup(0);
  framebufferTextureView->SetBinding(3);
  framebufferTextureView->SetFormat(vtkWebGPUComputeTexture::TextureFormat::BGRA8_UNORM);
  framebufferTextureView->SetMode(vtkWebGPUComputeTextureView::TextureViewMode::WRITE_ONLY_STORAGE);
  framebufferTextureView->SetAspect(vtkWebGPUComputeTextureView::TextureViewAspect::ASPECT_ALL);
  framebufferTextureView->SetLabel("Point cloud mapper - point render pass - color framebuffer");
  this->RenderPointsPass->AddTextureView(framebufferTextureView);

  vtkSmartPointer<vtkWebGPUComputeBuffer> cameraVPBuffer;
  cameraVPBuffer = vtkSmartPointer<vtkWebGPUComputeBuffer>::New();
  cameraVPBuffer->SetGroup(0);
  cameraVPBuffer->SetBinding(4);
  cameraVPBuffer->SetByteSize(sizeof(float) * 4 * 4); // 4x4 matrix
  cameraVPBuffer->SetLabel(
    "Compute point cloud renderer - Point render pass - Camera view-projection matrix buffer");
  cameraVPBuffer->SetDataType(vtkWebGPUComputeBuffer::BufferDataType::STD_VECTOR);
  cameraVPBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::UNIFORM_BUFFER);

  this->CameraVPBufferIndex = this->RenderPointsPass->AddBuffer(cameraVPBuffer);
  this->RenderPointsPass->SetLabel("Point cloud mapper - Render point pass pass");
}

//------------------------------------------------------------------------------
void vtkWebGPUPointCloudMapperInternals::UploadPointsToGPU()
{
  vtkPoints* points = this->CachedInput->GetPoints();
  vtkMTimeType pointsMTime = points->GetMTime();
  if (points->GetMTime() <= this->LastPointsMTime)
  {
    // Nothing to upload, already up to date

    return;
  }

  this->LastPointsMTime = pointsMTime;

  std::vector<float> floatPointsData(points->GetNumberOfPoints() * 3);
  for (vtkIdType i = 0; i < points->GetNumberOfPoints(); i++)
  {
    double* pointDouble = points->GetPoint(i);

    // Casting the coordinates of the point into float because WebGPU can only handle 32-bit
    // floating points types
    floatPointsData[i * 3 + 0] = static_cast<float>(pointDouble[0]);
    floatPointsData[i * 3 + 1] = static_cast<float>(pointDouble[1]);
    floatPointsData[i * 3 + 2] = static_cast<float>(pointDouble[2]);
  }

  // The compute shader uses workgroups of size (256, 1, 1)
  // The maximum number of workgroups on one dimensions is 65535
  // as per the spec:
  // https://www.w3.org/TR/webgpu/#dom-supported-limits-maxcomputeworkgroupsperdimension
  //
  // If 65535 workgroups of size 256 isn't enough to have one thread per point (that's
  // a maximum of 16776960 thread per compute invocation), then 1 single thread will render
  // multiple points. This logic is handled in the shader.
  int groupsX =
    static_cast<int>(std::min(65535.0f, std::ceil(points->GetNumberOfPoints() / 256.0f)));
  this->RenderPointsPass->SetWorkgroups(groupsX, 1, 1);
  this->RenderPointsPass->ResizeBuffer(
    this->PointBufferIndex, points->GetNumberOfPoints() * sizeof(float) * 3);
  this->RenderPointsPass->UpdateBufferData(this->PointBufferIndex, floatPointsData);
}

//------------------------------------------------------------------------------
void vtkWebGPUPointCloudMapperInternals::UploadColorsToGPU()
{
  vtkMTimeType pointDataMTime = this->CachedInput->GetPointData()->GetMTime();
  if (pointDataMTime <= this->LastPointDataMTime)
  {
    // Nothing to upload, already up to date

    return;
  }

  this->LastPointDataMTime = pointDataMTime;

  vtkUnsignedCharArray* pointColors = this->ParentMapper->MapScalars(1.0);

  if (pointColors != nullptr && pointColors->GetNumberOfValues() > 0)
  {
    // If we DO have colors to upload

    // Resizing to hold unsigned char type colors (the documentation of MapScalars() guarantees that
    // this->Colors contains unsigned char data)
    this->RenderPointsPass->ResizeBuffer(
      this->PointColorBufferIndex, sizeof(unsigned char) * pointColors->GetNumberOfValues());
    this->RenderPointsPass->UpdateBufferData(this->PointColorBufferIndex, pointColors);
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUPointCloudMapperInternals::UploadCameraVPMatrix(vtkRenderer* renderer)
{
  vtkCamera* camera = renderer->GetActiveCamera();

  vtkMatrix4x4* viewMatrix = camera->GetModelViewTransformMatrix();
  vtkMatrix4x4* projectionMatrix =
    camera->GetProjectionTransformMatrix(renderer->GetTiledAspectRatio(), -1, 1);
  vtkNew<vtkMatrix4x4> viewProj;
  vtkMatrix4x4::Multiply4x4(projectionMatrix, viewMatrix, viewProj);
  // WebGPU uses column major matrices but VTK is row major
  viewProj->Transpose();

  // Getting the matrix data as floats
  std::vector<float> matrixData(16);
  for (std::size_t i = 0; i < 16; i++)
  {
    matrixData[i] = static_cast<float>(viewProj->GetData()[i]);
  }

  this->RenderPointsPass->UpdateBufferData(this->CameraVPBufferIndex, matrixData);
}

VTK_ABI_NAMESPACE_END
