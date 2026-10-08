// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUContextDevice2D.h"
#include "Private/vtkWebGPUContextDevice2DPrivate.h"
#include "Private/vtkWebGPUHelpersPrivate.h"

#include "ContextDevice2D.h"
#include "Private/vtkWebGPUProcDispatch.h"
#include "Private/vtkWebGPURenderPipelineDescriptorInternals.h"
#include "vtkAbstractMapper.h"
#include "vtkBrush.h"
#include "vtkFloatArray.h"
#include "vtkFreeTypeTools.h"
#include "vtkImageData.h"
#include "vtkMarkerUtilities.h"
#include "vtkMath.h"
#include "vtkMathTextUtilities.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkPolyData.h"
#include "vtkSetGet.h"
#include "vtkTextProperty.h"
#include "vtkTransform.h"
#include "vtkUnsignedCharArray.h"
#include "vtkViewport.h"
#include "vtkWebGPURenderer.h"
#include "webgpu/webgpu.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

namespace
{
const WGPUStringView kTCoordsKey = { "haveTCoords", 11 };
const WGPUStringView kLinesKey = { "haveLines", 9 };
const WGPUStringView kVertexEntryPoint = { "ContextDevice2DVertex", 21 };
const WGPUStringView kFragmentEntryPoint = { "ContextDevice2DFragment", 23 };
constexpr float inv255 = 1.0f / 255.0f;

constexpr std::size_t FLOATS_PER_VERTEX = 8;
constexpr std::size_t INITIAL_VERTEX_CAPACITY = 10000;

}

vtkStandardNewMacro(vtkWebGPUContextDevice2D);

//------------------------------------------------------------------------------
vtkWebGPUContextDevice2D::vtkWebGPUContextDevice2D()
{
  this->Storage = new vtkWebGPUContextDevice2D::Private();

  this->ProjectionMatrix = vtkTransform::New();
  this->ModelMatrix = vtkTransform::New();
}

//------------------------------------------------------------------------------
vtkWebGPUContextDevice2D::~vtkWebGPUContextDevice2D()
{
  delete this->Storage;
  this->Storage = nullptr;

  this->ProjectionMatrix->Delete();
  this->ModelMatrix->Delete();
}

//------------------------------------------------------------------------------
void vtkWebGPUContextDevice2D::Begin(vtkViewport* viewport)
{
  this->RenderWindow = vtkWebGPURenderWindow::SafeDownCast(viewport->GetVTKWindow());
  this->Renderer = vtkRenderer::SafeDownCast(viewport);
  if (!this->RenderWindow || !this->Renderer)
  {
    vtkErrorMacro("vtkWebGPUContextDevice2D requires a vtkWebGPURenderWindow and vtkRenderer.");
    return;
  }

  int* size = viewport->GetSize();

  this->Storage->Config = this->RenderWindow->GetWGPUConfiguration();
  this->Storage->PipelineCache = this->RenderWindow->GetWGPUPipelineCache();

  if (!this->Storage->Config || !this->Storage->Config->GetDevice())
  {
    vtkErrorMacro("WebGPU Configuration or Device is not ready.");
    return;
  }

  if (!this->Storage->MatrixUniformBuffer)
  {
    if (!this->InitializeDeviceResources())
    {
      vtkErrorMacro("WebGPU ContextDevice2D failed to initialize");
    }
    else
    {
      vtkDebugMacro("WebGPU ContextDevice2D initialize");
    }
  }

  this->ProjectionMatrix->Push();
  this->ProjectionMatrix->Identity();
  this->ModelMatrix->Push();
  this->ModelMatrix->Identity();

  double offset = 0.5;
  double xmin = offset;
  double xmax = size[0] + offset - 1.0;
  double ymin = offset;
  double ymax = size[1] + offset - 1.0;
  double znear = -2000;
  double zfar = 2000;

  vtkNew<vtkMatrix4x4> matrix;
  matrix->Identity();

  matrix->SetElement(0, 0, 2.0 / (xmax - xmin));
  matrix->SetElement(1, 1, 2.0 / (ymax - ymin));
  matrix->SetElement(2, 2, -1.0 / (zfar - znear));

  matrix->SetElement(0, 3, -(xmin + xmax) / (xmax - xmin));
  matrix->SetElement(1, 3, -(ymin + ymax) / (ymax - ymin));
  matrix->SetElement(2, 3, -znear / (zfar - znear));

  this->ProjectionMatrix->SetMatrix(matrix);

  this->UpdateCameraUniforms();

  this->Storage->VertexAllocator.Reset();

  this->Storage->BatchedVertices.clear();
  this->Storage->BatchedVertices.reserve(INITIAL_VERTEX_CAPACITY * FLOATS_PER_VERTEX);
  this->Storage->CurrentBatchPipeline = nullptr;
  this->Storage->CurrentBatchBindGroup = nullptr;

  this->InRender = true;
}

//------------------------------------------------------------------------------
void vtkWebGPUContextDevice2D::ReleaseGraphicsResources(vtkWindow* /*window*/)
{
  if (!this->Storage)
  {
    return;
  }

  if (this->Storage->MatrixUniformBuffer)
  {
    wgpuBufferRelease(this->Storage->MatrixUniformBuffer);
    this->Storage->MatrixUniformBuffer = nullptr;
  }
  if (this->Storage->ColorBuffer)
  {
    wgpuBufferRelease(this->Storage->ColorBuffer);
    this->Storage->ColorBuffer = nullptr;
  }

  if (this->Storage->DummyTextureView)
  {
    wgpuTextureViewRelease(this->Storage->DummyTextureView);
    this->Storage->DummyTextureView = nullptr;
  }
  if (this->Storage->DummyTexture)
  {
    wgpuTextureRelease(this->Storage->DummyTexture);
    this->Storage->DummyTexture = nullptr;
  }
  if (this->Storage->DefaultSampler)
  {
    wgpuSamplerRelease(this->Storage->DefaultSampler);
    this->Storage->DefaultSampler = nullptr;
  }

  if (this->Storage->TransformBindGroup)
  {
    wgpuBindGroupRelease(this->Storage->TransformBindGroup);
    this->Storage->TransformBindGroup = nullptr;
  }
  if (this->Storage->BindGroupLayout)
  {
    wgpuBindGroupLayoutRelease(this->Storage->BindGroupLayout);
    this->Storage->BindGroupLayout = nullptr;
  }
  if (this->Storage->PipelineLayout)
  {
    wgpuPipelineLayoutRelease(this->Storage->PipelineLayout);
    this->Storage->PipelineLayout = nullptr;
  }

  this->Storage->ActiveTextureBindGroup = nullptr;
  this->Storage->CurrentBatchBindGroup = nullptr;
  this->Storage->CurrentBatchPipeline = nullptr;

  for (auto& pair : this->Storage->ImageCache)
  {
    if (pair.second.BindGroup)
      wgpuBindGroupRelease(pair.second.BindGroup);
    if (pair.second.View)
      wgpuTextureViewRelease(pair.second.View);
    if (pair.second.Texture)
      wgpuTextureRelease(pair.second.Texture);
    if (pair.second.Sampler)
      wgpuSamplerRelease(pair.second.Sampler);
  }
  this->Storage->ImageCache.clear();
  this->Storage->TextImageCache.clear();

  this->Storage->VertexAllocator.Reset();
  this->Storage->BatchedVertices.clear();
}

//------------------------------------------------------------------------------
void vtkWebGPUContextDevice2D::End()
{
  if (!this->InRender)
    return;
  this->Flush();
  this->ProjectionMatrix->Pop();
  this->ModelMatrix->Pop();

  this->RenderWindow = nullptr;
  this->InRender = false;
}

//------------------------------------------------------------------------------
void vtkWebGPUContextDevice2D::Flush()
{
  if (!this->InRender || !this->Storage || this->Storage->BatchedVertices.empty())
  {
    return;
  }

  vtkWebGPURenderer* wgpuRenderer = vtkWebGPURenderer::SafeDownCast(this->Renderer);
  if (!wgpuRenderer)
    return;

  WGPURenderPassEncoder passEncoder = wgpuRenderer->GetRenderPassEncoder();
  if (!passEncoder)
    return;

  uint64_t sizeInBytes = this->Storage->BatchedVertices.size() * sizeof(float);
  uint64_t vboOffset = this->Storage->VertexAllocator.Allocate(
    this->Storage->Config, sizeInBytes, WGPUBufferUsage_Vertex);

  WGPUBuffer currentVBO = this->Storage->VertexAllocator.GetBuffer();

  this->Storage->Config->WriteBuffer(
    currentVBO, vboOffset, this->Storage->BatchedVertices.data(), sizeInBytes, "Batch Upload");

  this->ApplyScissorRect(passEncoder);
  wgpuRenderPassEncoderSetPipeline(passEncoder, this->Storage->CurrentBatchPipeline);
  wgpuRenderPassEncoderSetBindGroup(
    passEncoder, 0, this->Storage->CurrentBatchBindGroup, 0, nullptr);
  wgpuRenderPassEncoderSetVertexBuffer(passEncoder, 0, currentVBO, vboOffset, sizeInBytes);

  uint32_t vertexCount = static_cast<uint32_t>(this->Storage->BatchedVertices.size() / 8);
  wgpuRenderPassEncoderDraw(passEncoder, vertexCount, 1, 0, 0);

  this->Storage->BatchedVertices.clear();
}

//------------------------------------------------------------------------------
bool vtkWebGPUContextDevice2D::InitializeDeviceResources()
{
  if (!this->Storage || !this->Storage->Config || !this->Storage->PipelineCache)
  {
    return false;
  }

  // Init Uniform

  const std::string label = "CameraUniforms-" + this->GetObjectDescription();
  this->Storage->MatrixUniformBuffer = this->Storage->Config->CreateBuffer(128,
    static_cast<WGPUBufferUsage>(WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst), false,
    label.c_str());

  const auto& device = this->Storage->Config->GetDevice();
  if (!device)
  {
    vtkErrorMacro("Failed to get device");
    return false;
  }

  WGPUExtent3D extents = { 1, 1, 1 };
  this->Storage->DummyTexture = this->Storage->Config->CreateTexture(extents,
    WGPUTextureDimension_2D, WGPUTextureFormat_RGBA8Unorm,
    static_cast<WGPUTextureUsage>(WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst), 1,
    "Context2D Dummy Texture");

  this->InitializeTexture();

  vtkDebugMacro("INIT vtkWebGPUContextDevice2D");

  std::array<WGPUBindGroupLayoutEntry, 3> layoutEntries = {};

  layoutEntries[0].nextInChain = nullptr;
  layoutEntries[0].binding = 0;
  layoutEntries[0].visibility = WGPUShaderStage_Vertex;
  layoutEntries[0].buffer.nextInChain = nullptr;
  layoutEntries[0].buffer.type = WGPUBufferBindingType_Uniform;
  layoutEntries[0].buffer.hasDynamicOffset = false;
  layoutEntries[0].buffer.minBindingSize = 128;

  layoutEntries[1].binding = 1;
  layoutEntries[1].visibility = WGPUShaderStage_Fragment;
  layoutEntries[1].texture.sampleType = WGPUTextureSampleType_Float;
  layoutEntries[1].texture.viewDimension = WGPUTextureViewDimension_2D;

  layoutEntries[2].binding = 2;
  layoutEntries[2].visibility = WGPUShaderStage_Fragment;
  layoutEntries[2].sampler.type = WGPUSamplerBindingType_Filtering;

  WGPUBindGroupLayoutDescriptor layoutDesc{};
  layoutDesc.nextInChain = nullptr;
  layoutDesc.label = vtkWebGPUMakeStringView("Context2D Uniform Layout Descriptor");
  layoutDesc.entryCount = 3;
  layoutDesc.entries = layoutEntries.data();

  WGPUBindGroupLayout bindGroupLayout = wgpuDeviceCreateBindGroupLayout(device, &layoutDesc);
  if (!bindGroupLayout)
  {
    vtkErrorMacro("Failed to explicitly allocate raw WGPUBindGroupLayout container.");
    return false;
  }
  this->Storage->BindGroupLayout = bindGroupLayout;

  WGPUPipelineLayoutDescriptor pipelineLayoutDesc = WGPU_PIPELINE_LAYOUT_DESCRIPTOR_INIT;
  pipelineLayoutDesc.bindGroupLayoutCount = 1;
  pipelineLayoutDesc.bindGroupLayouts = &bindGroupLayout;
  pipelineLayoutDesc.label = WGPUStringView{ "Context2DPipelineLayout", WGPU_STRLEN };
  this->Storage->PipelineLayout = wgpuDeviceCreatePipelineLayout(device, &pipelineLayoutDesc);

  std::array<WGPUVertexAttribute, 3> vertexAttributes = {};
  vertexAttributes[0].shaderLocation = 0;
  vertexAttributes[0].format = WGPUVertexFormat_Float32x2;
  vertexAttributes[0].offset = 0;

  vertexAttributes[1].shaderLocation = 1;
  vertexAttributes[1].format = WGPUVertexFormat_Float32x4;
  vertexAttributes[1].offset = sizeof(float) * 2;

  vertexAttributes[2].shaderLocation = 2;
  vertexAttributes[2].format = WGPUVertexFormat_Float32x2;
  vertexAttributes[2].offset = sizeof(float) * 6;

  WGPUVertexBufferLayout vertexBufferLayout{};
  vertexBufferLayout.arrayStride = sizeof(float) * 8;
  vertexBufferLayout.attributeCount = static_cast<uint32_t>(vertexAttributes.size());
  vertexBufferLayout.attributes = vertexAttributes.data();
  vertexBufferLayout.stepMode = WGPUVertexStepMode_Vertex;

  this->Storage->VertexBufferLayout = vertexBufferLayout;

  this->Storage->TrianglePipeline = this->CreatePipelineWithConstants(0.0f, 0.0f);
  this->Storage->LinePipeline =
    this->CreatePipelineWithConstants(0.0f, 1.0f, WGPUPrimitiveTopology_LineList);
  this->Storage->TexturedPipeline =
    this->CreatePipelineWithConstants(1.0f, 0.0f, WGPUPrimitiveTopology_TriangleList);

  // Bind Group
  std::array<WGPUBindGroupEntry, 3> bgEntries = {};
  bgEntries[0].nextInChain = nullptr;
  bgEntries[0].binding = 0;
  bgEntries[0].buffer = this->Storage->MatrixUniformBuffer;
  bgEntries[0].offset = 0;
  bgEntries[0].size = 128;

  bgEntries[1].binding = 1;
  bgEntries[1].textureView = this->Storage->DummyTextureView;

  bgEntries[2].binding = 2;
  bgEntries[2].sampler = this->Storage->DefaultSampler;

  WGPUBindGroupDescriptor bindGroupDesc{};
  bindGroupDesc.nextInChain = nullptr;
  bindGroupDesc.label = vtkWebGPUMakeStringView("Context2D Shared TransformBindGroup");
  bindGroupDesc.layout = bindGroupLayout;
  bindGroupDesc.entryCount = 3;
  bindGroupDesc.entries = bgEntries.data();

  WGPUBindGroup rawBindGroup = wgpuDeviceCreateBindGroup(device, &bindGroupDesc);
  if (!rawBindGroup)
  {
    vtkErrorMacro("Failed to instantiate native WGPUBindGroup instance.");
    return false;
  }
  this->Storage->TransformBindGroup = rawBindGroup;

  return true;
}

//------------------------------------------------------------------------------
void vtkWebGPUContextDevice2D::InitializeTexture()
{
  const auto& device = this->Storage->Config->GetDevice();
  if (!device)
  {
    vtkErrorMacro("Failed to get device");
    return;
  }
  WGPUExtent3D extents = { 1, 1, 1 };
  this->Storage->DummyTexture = this->Storage->Config->CreateTexture(extents,
    WGPUTextureDimension_2D, WGPUTextureFormat_RGBA8Unorm,
    static_cast<WGPUTextureUsage>(WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst), 1,
    "Context2D Dummy Texture");

  this->Storage->DummyTextureView =
    this->Storage->Config->CreateView(this->Storage->DummyTexture, WGPUTextureViewDimension_2D,
      WGPUTextureAspect_All, WGPUTextureFormat_RGBA8Unorm, 0, 1, "Context2D Dummy Texture View");

  WGPUSamplerDescriptor samplerDesc = WGPU_SAMPLER_DESCRIPTOR_INIT;
  samplerDesc.addressModeU = WGPUAddressMode_ClampToEdge;
  samplerDesc.addressModeV = WGPUAddressMode_ClampToEdge;
  samplerDesc.addressModeW = WGPUAddressMode_ClampToEdge;
  samplerDesc.magFilter = WGPUFilterMode_Linear;
  samplerDesc.minFilter = WGPUFilterMode_Linear;
  samplerDesc.mipmapFilter = WGPUMipmapFilterMode_Linear;
  samplerDesc.maxAnisotropy = 1;

  this->Storage->DefaultSampler = wgpuDeviceCreateSampler(device, &samplerDesc);
}

//------------------------------------------------------------------------------
WGPURenderPipeline vtkWebGPUContextDevice2D::CreatePipelineWithConstants(
  float haveTCoordsVal, float haveLinesVal, WGPUPrimitiveTopology topology)
{
  const auto& device = this->Storage->Config->GetDevice();
  if (!device)
  {
    return nullptr;
  }

  WGPUTextureFormat colorFormat = this->RenderWindow->GetOffscreenColorTextureFormat();

  std::array<WGPUConstantEntry, 2> constants = {
    {
      { nullptr, kTCoordsKey, haveTCoordsVal },
      { nullptr, kLinesKey, haveLinesVal },
    },
  };

  std::array<WGPUColorTargetState, 2> targets = {};
  targets[0].format = colorFormat;
  targets[0].writeMask = WGPUColorWriteMask_All;

  targets[1].format = RenderWindow->GetPreferredSelectorIdsTextureFormat();
  targets[1].writeMask = WGPUColorWriteMask_All;

  WGPUDepthStencilState depthStencil = {};
  depthStencil.format = WGPUTextureFormat_Depth24PlusStencil8;
  depthStencil.depthWriteEnabled = WGPUOptionalBool_False;
  depthStencil.depthCompare = WGPUCompareFunction_LessEqual;

  vtkWebGPURenderPipelineDescriptorInternals rpDesc;
  rpDesc.label = WGPUStringView{ "Context2D Render Pipeline", WGPU_STRLEN };
  rpDesc.layout = this->Storage->PipelineLayout;

  rpDesc.vertex.entryPoint = kVertexEntryPoint;
  rpDesc.vertex.bufferCount = 1;
  rpDesc.vertex.buffers = &this->Storage->VertexBufferLayout;
  rpDesc.vertex.constantCount = static_cast<uint32_t>(constants.size());
  rpDesc.vertex.constants = constants.data();

  static WGPUBlendState blendState = {};
  blendState.color.srcFactor = WGPUBlendFactor_SrcAlpha;
  blendState.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
  blendState.color.operation = WGPUBlendOperation_Add;
  blendState.alpha.srcFactor = WGPUBlendFactor_Zero;
  blendState.alpha.dstFactor = WGPUBlendFactor_One;
  blendState.alpha.operation = WGPUBlendOperation_Add;

  rpDesc.cTargets[0].blend = &blendState;

  rpDesc.cFragment.entryPoint = kFragmentEntryPoint;
  rpDesc.cFragment.targetCount = 2;
  rpDesc.cFragment.constantCount = static_cast<uint32_t>(constants.size());
  rpDesc.cFragment.constants = constants.data();
  rpDesc.cTargets[0].format = colorFormat;
  rpDesc.cTargets[0].writeMask = WGPUColorWriteMask_All;
  rpDesc.cTargets[1].format = RenderWindow->GetPreferredSelectorIdsTextureFormat();
  rpDesc.cTargets[1].writeMask = WGPUColorWriteMask_All;
  rpDesc.DisableBlending(1);

  rpDesc.primitive.topology = topology;
  rpDesc.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
  rpDesc.primitive.frontFace = WGPUFrontFace_CCW;
  rpDesc.primitive.cullMode = WGPUCullMode_None;

  rpDesc.depthStencil = &depthStencil;
  rpDesc.multisample.count = 1;
  rpDesc.multisample.mask = 0xFFFFFFFF;

  std::string key = this->Storage->PipelineCache->GetPipelineKey(&rpDesc, ContextDevice2D);
  vtkDebugMacro("Get Pipeline Key = " << key);
  this->Storage->PipelineCache->CreateRenderPipeline(&rpDesc, this->RenderWindow, ContextDevice2D);
  return this->Storage->PipelineCache->GetRenderPipeline(key);
}

//------------------------------------------------------------------------------
void vtkWebGPUContextDevice2D::UpdateCameraUniforms()
{
  this->Flush();
  const double* projData = this->ProjectionMatrix->GetMatrix()->GetData();
  const double* modelData = this->ModelMatrix->GetMatrix()->GetData();

  std::array<float, 32> gpuBuffer;

  for (int row = 0; row < 4; ++row)
  {
    for (int col = 0; col < 4; ++col)
    {
      int vtkIdx = row * 4 + col;
      int wgpuIdx = col * 4 + row;

      gpuBuffer[wgpuIdx] = static_cast<float>(projData[vtkIdx]);

      gpuBuffer[wgpuIdx + 16] = static_cast<float>(modelData[vtkIdx]);
    }
  }
  this->Storage->Config->WriteBuffer(
    this->Storage->MatrixUniformBuffer, 0, gpuBuffer.data(), gpuBuffer.size() * sizeof(float));
}

//------------------------------------------------------------------------------
void vtkWebGPUContextDevice2D::UpdatePaintUniforms(const unsigned char color[4])
{
  if (!this->Storage || !this->Storage->ColorBuffer || !this->Storage->Config)
  {
    return;
  }

  std::array<float, 4> normalizedColor;
  normalizedColor[0] = static_cast<float>(color[0]) / 255.0f; // Red
  normalizedColor[1] = static_cast<float>(color[1]) / 255.0f; // Green
  normalizedColor[2] = static_cast<float>(color[2]) / 255.0f; // Blue
  normalizedColor[3] = static_cast<float>(color[3]) / 255.0f; // Alpha

  this->Storage->Config->WriteBuffer(this->Storage->ColorBuffer, 0, normalizedColor.data(),
    normalizedColor.size() * sizeof(float), "ContextDevice2D Color Update");
}

//------------------------------------------------------------------------------
void vtkWebGPUContextDevice2D::DrawPoly(float* points, int n, unsigned char* colors, int nc_comps)
{
  if (!this->InRender || !points || n <= 0)
  {
    return;
  }

  WGPURenderPipeline reqPipeline = this->Storage->TrianglePipeline;
  WGPUBindGroup reqBindGroup = this->Storage->TransformBindGroup;

  if (this->Storage->CurrentBatchPipeline != reqPipeline ||
    this->Storage->CurrentBatchBindGroup != reqBindGroup)
  {
    this->Flush();
    this->Storage->CurrentBatchPipeline = reqPipeline;
    this->Storage->CurrentBatchBindGroup = reqBindGroup;
  }

  unsigned char penColor[4];
  this->Pen->GetColor(penColor);
  penColor[3] = this->Pen->GetOpacity();

  size_t startIdx = this->Storage->BatchedVertices.size();
  this->Storage->BatchedVertices.resize(startIdx + (n * 8));
  float* vertexData = &this->Storage->BatchedVertices[startIdx];

  for (int i = 0; i < n; ++i)
  {
    int base = i * 8;
    vertexData[base + 0] = points[i * 2 + 0];
    vertexData[base + 1] = points[i * 2 + 1];

    if (colors != nullptr)
    {
      int start = i * nc_comps;
      vertexData[base + 2] = (nc_comps > 0) ? colors[start + 0] * inv255 : 0.0f;
      vertexData[base + 3] = (nc_comps > 1) ? colors[start + 1] * inv255 : 0.0f;
      vertexData[base + 4] = (nc_comps > 2) ? colors[start + 2] * inv255 : 0.0f;
      vertexData[base + 5] = (nc_comps > 3) ? colors[start + 3] * inv255 : 1.0f;
    }
    else
    {
      vertexData[base + 2] = penColor[0] * inv255;
      vertexData[base + 3] = penColor[1] * inv255;
      vertexData[base + 4] = penColor[2] * inv255;
      vertexData[base + 5] = penColor[3] * inv255;
    }

    vertexData[base + 6] = 0.0f; // TCoord X
    vertexData[base + 7] = 0.0f; // TCoord Y
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUContextDevice2D::DrawLines(float* points, int n, unsigned char* colors, int nc)
{
  if (!this->InRender || !points || n <= 0)
  {
    return;
  }

  if (this->Pen->GetLineType() == vtkPen::NO_PEN)
  {
    return;
  }

  if (!colors && this->Pen->GetColorObject().GetAlpha() == 0)
  {
    return;
  }

  unsigned char penColor[4];
  this->Pen->GetColor(penColor);
  penColor[3] = this->Pen->GetOpacity();
  this->UpdatePaintUniforms(penColor);

  // Compute accumulated distances for stippling/dashing
  double* scale = this->ModelMatrix->GetScale();
  std::vector<float> distances(n, 0.0f);
  float totalDistance = 0.0f;
  for (int i = 1; i < n; i++)
  {
    float xDel = scale[0] * (points[i * 2] - points[i * 2 - 2]);
    float yDel = scale[1] * (points[i * 2 + 1] - points[i * 2 - 1]);
    totalDistance += std::sqrt(xDel * xDel + yDel * yDel);
    distances[i] = totalDistance;
  }

  if (this->Pen->GetWidth() > 1.0f)
  {
    this->DrawThickLines(points, n, colors, nc, distances);
  }
  else
  {
    this->DrawThinLines(points, n, colors, nc, distances);
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUContextDevice2D::DrawThickLines(
  float* points, int n, unsigned char* colors, int nc, const std::vector<float>& distances)
{
  WGPURenderPipeline reqPipeline = this->Storage->TrianglePipeline;
  WGPUBindGroup reqBindGroup = this->Storage->TransformBindGroup;

  if (this->Storage->CurrentBatchPipeline != reqPipeline ||
    this->Storage->CurrentBatchBindGroup != reqBindGroup)
  {
    this->Flush();
    this->Storage->CurrentBatchPipeline = reqPipeline;
    this->Storage->CurrentBatchBindGroup = reqBindGroup;
  }

  unsigned char penColor[4];
  this->Pen->GetColor(penColor);
  penColor[3] = this->Pen->GetOpacity();

  double* scale = this->ModelMatrix->GetScale();
  size_t startIdx = this->Storage->BatchedVertices.size();

  float hwidth = this->Pen->GetWidth() / 2.0f;
  int vertexCount = (n / 2) * 6;
  this->Storage->BatchedVertices.resize(startIdx + (vertexCount * 8));
  float* vertexData = &this->Storage->BatchedVertices[startIdx];

  for (int i = 0; i < n - 1; i += 2)
  {
    vtkVector2f dir((points[i * 2 + 2] - points[i * 2]) * scale[0],
      (points[i * 2 + 3] - points[i * 2 + 1]) * scale[1]);

    vtkVector2f norm(-dir.GetY(), dir.GetX());
    norm.Normalize();
    norm.SetX(hwidth * norm.GetX() / scale[0]);
    norm.SetY(hwidth * norm.GetY() / scale[1]);

    const std::array<float, 6> px = {
      points[i * 2] + norm.GetX(),
      points[i * 2] - norm.GetX(),
      points[i * 2 + 2] - norm.GetX(),
      points[i * 2] + norm.GetX(),
      points[i * 2 + 2] - norm.GetX(),
      points[i * 2 + 2] + norm.GetX(),
    };

    const std::array<float, 6> py = {
      points[i * 2 + 1] + norm.GetY(),
      points[i * 2 + 1] - norm.GetY(),
      points[i * 2 + 3] - norm.GetY(),
      points[i * 2 + 1] + norm.GetY(),
      points[i * 2 + 3] - norm.GetY(),
      points[i * 2 + 3] + norm.GetY(),
    };

    const std::array<int, 6> pIdx = { i, i, i + 1, i, i + 1, i + 1 };

    for (int v = 0; v < 6; ++v)
    {
      int base = ((i / 2) * 6 + v) * 8;
      vertexData[base + 0] = px[v];
      vertexData[base + 1] = py[v];

      if (colors != nullptr)
      {
        int start = pIdx[v] * nc;
        vertexData[base + 2] = (nc > 0) ? colors[start + 0] * inv255 : 0.0f;
        vertexData[base + 3] = (nc > 1) ? colors[start + 1] * inv255 : 0.0f;
        vertexData[base + 4] = (nc > 2) ? colors[start + 2] * inv255 : 0.0f;
        vertexData[base + 5] = (nc > 3) ? colors[start + 3] * inv255 : 1.0f;
      }
      else
      {
        vertexData[base + 2] = penColor[0] * inv255;
        vertexData[base + 3] = penColor[1] * inv255;
        vertexData[base + 4] = penColor[2] * inv255;
        vertexData[base + 5] = penColor[3] * inv255;
      }

      vertexData[base + 6] = distances[pIdx[v]]; // Distance for stipple
      vertexData[base + 7] = 0.0f;
    }
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUContextDevice2D::DrawThinLines(
  float* points, int n, unsigned char* colors, int nc, const std::vector<float>& distances)
{
  WGPURenderPipeline reqPipeline = this->Storage->LinePipeline;
  WGPUBindGroup reqBindGroup = this->Storage->TransformBindGroup;

  if (this->Storage->CurrentBatchPipeline != reqPipeline ||
    this->Storage->CurrentBatchBindGroup != reqBindGroup)
  {
    this->Flush();
    this->Storage->CurrentBatchPipeline = reqPipeline;
    this->Storage->CurrentBatchBindGroup = reqBindGroup;
  }

  unsigned char penColor[4];
  this->Pen->GetColor(penColor);
  penColor[3] = this->Pen->GetOpacity();

  size_t startIdx = this->Storage->BatchedVertices.size();
  this->Storage->BatchedVertices.resize(startIdx + (n * 8));
  float* vertexData = &this->Storage->BatchedVertices[startIdx];

  for (int i = 0; i < n; ++i)
  {
    int base = i * 8;
    vertexData[base + 0] = points[i * 2];
    vertexData[base + 1] = points[i * 2 + 1];

    if (colors != nullptr)
    {
      int start = i * nc;
      vertexData[base + 2] = (nc > 0) ? colors[start + 0] * inv255 : 0.0f;
      vertexData[base + 3] = (nc > 1) ? colors[start + 1] * inv255 : 0.0f;
      vertexData[base + 4] = (nc > 2) ? colors[start + 2] * inv255 : 0.0f;
      vertexData[base + 5] = (nc > 3) ? colors[start + 3] * inv255 : 1.0f;
    }
    else
    {
      vertexData[base + 2] = penColor[0] * inv255;
      vertexData[base + 3] = penColor[1] * inv255;
      vertexData[base + 4] = penColor[2] * inv255;
      vertexData[base + 5] = penColor[3] * inv255;
    }

    vertexData[base + 6] = distances[i];
    vertexData[base + 7] = 0.0f;
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUContextDevice2D::DrawPoints(float* f, int n, unsigned char* colors, int nc_comps)
{
  if (!this->InRender || !f || n <= 0)
  {
    return;
  }

  WGPURenderPipeline reqPipeline = this->Storage->TrianglePipeline;
  WGPUBindGroup reqBindGroup = this->Storage->TransformBindGroup;

  if (this->Storage->CurrentBatchPipeline != reqPipeline ||
    this->Storage->CurrentBatchBindGroup != reqBindGroup)
  {
    this->Flush();
    this->Storage->CurrentBatchPipeline = reqPipeline;
    this->Storage->CurrentBatchBindGroup = reqBindGroup;
  }

  unsigned char penColor[4];
  this->Pen->GetColor(penColor);
  penColor[3] = this->Pen->GetOpacity();

  int vertexCount = n * 6;
  size_t startIdx = this->Storage->BatchedVertices.size();
  this->Storage->BatchedVertices.resize(startIdx + (vertexCount * 8));
  float* vertexData = &this->Storage->BatchedVertices[startIdx];

  double* scale = this->ModelMatrix->GetScale();
  float pointSize = this->Pen->GetWidth() > 0.0f ? this->Pen->GetWidth() : 1.0f;
  float hwX = (pointSize / 2.0f) / scale[0];
  float hwY = (pointSize / 2.0f) / scale[1];

  for (int i = 0; i < n; ++i)
  {
    float cx = f[i * 2];
    float cy = f[i * 2 + 1];

    const std::array<float, 6> px = { cx - hwX, cx - hwX, cx + hwX, cx - hwX, cx + hwX, cx + hwX };
    const std::array<float, 6> py = { cy + hwY, cy - hwY, cy + hwY, cy - hwY, cy - hwY, cy + hwY };

    for (int v = 0; v < 6; ++v)
    {
      int base = (i * 6 + v) * 8;
      vertexData[base + 0] = px[v];
      vertexData[base + 1] = py[v];

      if (colors != nullptr)
      {
        int start = i * nc_comps;
        vertexData[base + 2] = (nc_comps > 0) ? colors[start + 0] * inv255 : 0.0f;
        vertexData[base + 3] = (nc_comps > 1) ? colors[start + 1] * inv255 : 0.0f;
        vertexData[base + 4] = (nc_comps > 2) ? colors[start + 2] * inv255 : 0.0f;
        vertexData[base + 5] = (nc_comps > 3) ? colors[start + 3] * inv255 : 1.0f;
      }
      else
      {
        vertexData[base + 2] = penColor[0] * inv255;
        vertexData[base + 3] = penColor[1] * inv255;
        vertexData[base + 4] = penColor[2] * inv255;
        vertexData[base + 5] = penColor[3] * inv255;
      }

      vertexData[base + 6] = 0.0f; // TCoord X (not used for plain points)
      vertexData[base + 7] = 0.0f; // TCoord Y
    }
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUContextDevice2D::DrawPoints(
  vtkDataArray* positions, vtkUnsignedCharArray* colors, std::uintptr_t /*cacheIdentifier*/)
{
  if (!positions)
  {
    return;
  }

  int n = positions->GetNumberOfTuples();
  vtkFloatArray* floatPos = vtkFloatArray::SafeDownCast(positions);
  unsigned char* c = colors ? colors->GetPointer(0) : nullptr;
  int nc_comps = colors ? colors->GetNumberOfComponents() : 0;
  if (floatPos)
  {
    float* f = floatPos->GetPointer(0);
    this->DrawPoints(f, n, c, nc_comps);
    return;
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUContextDevice2D::DrawPointSprites(
  vtkImageData* image, float* f, int n, unsigned char* colors, int nc_comps)
{
  if (!this->InRender || !f || n <= 0 || !image)
    return;

  this->SetTexture(image, 0);

  WGPURenderPipeline reqPipeline = this->Storage->TexturedPipeline;
  WGPUBindGroup reqBindGroup = this->Storage->ActiveTextureBindGroup
    ? this->Storage->ActiveTextureBindGroup
    : this->Storage->TransformBindGroup;

  if (this->Storage->CurrentBatchPipeline != reqPipeline ||
    this->Storage->CurrentBatchBindGroup != reqBindGroup)
  {
    this->Flush();
    this->Storage->CurrentBatchPipeline = reqPipeline;
    this->Storage->CurrentBatchBindGroup = reqBindGroup;
  }

  unsigned char penColor[4];
  this->Pen->GetColor(penColor);
  penColor[3] = this->Pen->GetOpacity();

  int vertexCount = n * 6;
  size_t startIdx = this->Storage->BatchedVertices.size();
  this->Storage->BatchedVertices.resize(startIdx + (vertexCount * 8));
  float* vertexData = &this->Storage->BatchedVertices[startIdx];

  double* scale = this->ModelMatrix->GetScale();

  int dims[3];
  image->GetDimensions(dims);
  float spriteWidth = (this->Pen->GetWidth() > 1.0f) ? this->Pen->GetWidth() : dims[0];
  float spriteHeight = (this->Pen->GetWidth() > 1.0f) ? this->Pen->GetWidth() : dims[1];

  float hwX = (spriteWidth / 2.0f) / scale[0];
  float hwY = (spriteHeight / 2.0f) / scale[1];

  for (int i = 0; i < n; ++i)
  {
    float cx = f[i * 2];
    float cy = f[i * 2 + 1];

    // Quad logic with UV texture coordinates
    float px[6] = { cx - hwX, cx - hwX, cx + hwX, cx - hwX, cx + hwX, cx + hwX };
    float py[6] = { cy + hwY, cy - hwY, cy + hwY, cy - hwY, cy - hwY, cy + hwY };

    // UVs: BL(0,1), TL(0,0), TR(1,0), BL(0,1), TR(1,0), BR(1,1)
    constexpr std::array<float, 6> u = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f };
    constexpr std::array<float, 6> v = { 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f };

    for (int vIdx = 0; vIdx < 6; ++vIdx)
    {
      int base = (i * 6 + vIdx) * 8;
      vertexData[base + 0] = px[vIdx];
      vertexData[base + 1] = py[vIdx];

      if (colors != nullptr)
      {
        int start = i * nc_comps;
        vertexData[base + 2] = (nc_comps > 0) ? colors[start + 0] * inv255 : 0.0f;
        vertexData[base + 3] = (nc_comps > 1) ? colors[start + 1] * inv255 : 0.0f;
        vertexData[base + 4] = (nc_comps > 2) ? colors[start + 2] * inv255 : 0.0f;
        vertexData[base + 5] = (nc_comps > 3) ? colors[start + 3] * inv255 : 1.0f;
      }
      else
      {
        vertexData[base + 2] = penColor[0] * inv255;
        vertexData[base + 3] = penColor[1] * inv255;
        vertexData[base + 4] = penColor[2] * inv255;
        vertexData[base + 5] = penColor[3] * inv255;
      }

      vertexData[base + 6] = u[vIdx]; // TCoord X
      vertexData[base + 7] = v[vIdx]; // TCoord Y
    }
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUContextDevice2D::DrawPointSprites(
  vtkImageData* image, vtkDataArray* positions, vtkUnsignedCharArray* colors, std::uintptr_t)
{
  if (!positions)
    return;
  int n = positions->GetNumberOfTuples();
  vtkFloatArray* floatPos = vtkFloatArray::SafeDownCast(positions);
  unsigned char* c = colors ? colors->GetPointer(0) : nullptr;
  int nc_comps = colors ? colors->GetNumberOfComponents() : 0;

  if (floatPos)
  {
    this->DrawPointSprites(image, floatPos->GetPointer(0), n, c, nc_comps);
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUContextDevice2D::DrawImage(const vtkRectf& rect, vtkImageData* image)
{
  if (!this->InRender || !image)
    return;

  vtkDebugMacro("DrawImage Called");
  this->SetTexture(image, 0);

  WGPURenderPipeline reqPipeline = this->Storage->TexturedPipeline;
  WGPUBindGroup reqBindGroup = this->Storage->ActiveTextureBindGroup
    ? this->Storage->ActiveTextureBindGroup
    : this->Storage->TransformBindGroup;

  if (this->Storage->CurrentBatchPipeline != reqPipeline ||
    this->Storage->CurrentBatchBindGroup != reqBindGroup)
  {
    this->Flush();
    this->Storage->CurrentBatchPipeline = reqPipeline;
    this->Storage->CurrentBatchBindGroup = reqBindGroup;
  }

  float x = rect.GetX();
  float y = rect.GetY();
  float w = rect.GetWidth();
  float h = rect.GetHeight();

  size_t startIdx = this->Storage->BatchedVertices.size();
  this->Storage->BatchedVertices.resize(startIdx + (6 * 8));
  float* vertexData = &this->Storage->BatchedVertices[startIdx];

  const std::array<float, 48> quadData = { x, y + h, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, x, y, 1.0f,
    1.0f, 1.0f, 1.0f, 0.0f, 0.0f, x + w, y, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, x, y + h, 1.0f,
    1.0f, 1.0f, 1.0f, 0.0f, 1.0f, x + w, y, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, x + w, y + h, 1.0f,
    1.0f, 1.0f, 1.0f, 1.0f, 1.0f };

  std::copy(quadData.begin(), quadData.end(), vertexData);
}

//------------------------------------------------------------------------------
void vtkWebGPUContextDevice2D::DrawImage(float p[2], float scale, vtkImageData* image)
{
  if (!image)
    return;
  int dims[3];
  image->GetDimensions(dims);
  float w = dims[0] * scale;
  float h = dims[1] * scale;
  this->DrawImage(vtkRectf(p[0], p[1], w, h), image);
}

//------------------------------------------------------------------------------
void vtkWebGPUContextDevice2D::DrawMarkers(
  int shape, bool highlight, float* f, int n, unsigned char* colors, int nc_comps)
{
  int size = static_cast<int>(this->Pen->GetWidth());
  if (size <= 0)
    size = 7;

  if (highlight)
  {
    size += 2;
  }

  vtkNew<vtkImageData> markerImage;
  vtkMarkerUtilities::GenerateMarker(markerImage, shape, size);

  this->DrawPointSprites(markerImage, f, n, colors, nc_comps);
}

//------------------------------------------------------------------------------
void vtkWebGPUContextDevice2D::DrawMarkers(
  int shape, bool highlight, vtkDataArray* positions, vtkUnsignedCharArray* colors, std::uintptr_t)
{
  if (!positions)
    return;
  int n = positions->GetNumberOfTuples();
  vtkFloatArray* floatPos = vtkFloatArray::SafeDownCast(positions);
  unsigned char* c = colors ? colors->GetPointer(0) : nullptr;
  int nc_comps = colors ? colors->GetNumberOfComponents() : 0;

  if (floatPos)
  {
    this->DrawMarkers(shape, highlight, floatPos->GetPointer(0), n, c, nc_comps);
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUContextDevice2D::DrawPolygon(float* f, int numPoint)
{
  this->DrawColoredPolygon(f, numPoint, nullptr, 0);
}

//------------------------------------------------------------------------------
void vtkWebGPUContextDevice2D::DrawColoredPolygon(
  float* f, int n, unsigned char* colors, int nc_comps)
{
  if (n < 3)
    return;

  int numTriangles = n - 2;
  int numVertices = numTriangles * 3;

  std::vector<float> triPoints(numVertices * 2);
  std::vector<unsigned char> triColors;

  if (colors)
  {
    triColors.resize(numVertices * nc_comps);
  }

  int vIdx = 0;
  for (int i = 0; i < numTriangles; ++i)
  {
    // Vertex 0 (Center/Pivot)
    triPoints[vIdx * 2] = f[0];
    triPoints[vIdx * 2 + 1] = f[1];
    if (colors)
      std::copy(colors, colors + nc_comps, &triColors[vIdx * nc_comps]);
    vIdx++;

    // Vertex i + 1
    triPoints[vIdx * 2] = f[(i + 1) * 2];
    triPoints[vIdx * 2 + 1] = f[(i + 1) * 2 + 1];
    if (colors)
      std::copy(colors + (i + 1) * nc_comps, colors + (i + 1) * nc_comps + nc_comps,
        &triColors[vIdx * nc_comps]);
    vIdx++;

    // Vertex i + 2
    triPoints[vIdx * 2] = f[(i + 2) * 2];
    triPoints[vIdx * 2 + 1] = f[(i + 2) * 2 + 1];
    if (colors)
      std::copy(colors + (i + 2) * nc_comps, colors + (i + 2) * nc_comps + nc_comps,
        &triColors[vIdx * nc_comps]);
    vIdx++;
  }

  this->DrawPoly(triPoints.data(), numVertices, colors ? triColors.data() : nullptr, nc_comps);
}

//------------------------------------------------------------------------------
void vtkWebGPUContextDevice2D::DrawQuad(float* f, int n)
{
  this->DrawPolygon(f, n);
}

//------------------------------------------------------------------------------
void vtkWebGPUContextDevice2D::DrawQuadStrip(float* f, int n)
{
  if (n < 4)
    return;

  // Convert QuadStrip to Triangle List
  int numQuads = (n / 2) - 1;
  int numVertices = numQuads * 6;
  std::vector<float> triPoints(numVertices * 2);

  int vIdx = 0;
  for (int i = 0; i < numQuads; ++i)
  {
    int base = i * 4; // 2 points per column
    // Triangle 1
    triPoints[vIdx++] = f[base + 0];
    triPoints[vIdx++] = f[base + 1];
    triPoints[vIdx++] = f[base + 2];
    triPoints[vIdx++] = f[base + 3];
    triPoints[vIdx++] = f[base + 4];
    triPoints[vIdx++] = f[base + 5];
    // Triangle 2
    triPoints[vIdx++] = f[base + 2];
    triPoints[vIdx++] = f[base + 3];
    triPoints[vIdx++] = f[base + 4];
    triPoints[vIdx++] = f[base + 5];
    triPoints[vIdx++] = f[base + 6];
    triPoints[vIdx++] = f[base + 7];
  }
  this->DrawPoly(triPoints.data(), numVertices, nullptr, 0);
}

//------------------------------------------------------------------------------
void vtkWebGPUContextDevice2D::DrawEllipseWedge(float x, float y, float outRx, float outRy,
  float inRx, float inRy, float startAngle, float stopAngle)
{
  constexpr int numSegments = 36;
  const float startRad = startAngle * vtkMath::Pi() / 180.0f;
  const float stopRad = stopAngle * vtkMath::Pi() / 180.0f;
  const float step = (stopRad - startRad) / numSegments;

  std::vector<float> points;

  if (inRx == 0.0f && inRy == 0.0f)
  {
    points.reserve((numSegments + 2) * 2);

    points.push_back(x);
    points.push_back(y);

    for (int i = 0; i <= numSegments; ++i)
    {
      const float angle = startRad + (i * step);
      points.push_back(x + outRx * std::cos(angle));
      points.push_back(y + outRy * std::sin(angle));
    }

    this->DrawPolygon(points.data(), static_cast<int>(points.size() / 2));
  }
  else
  {
    points.reserve((numSegments + 1) * 4);

    for (int i = 0; i <= numSegments; ++i)
    {
      const float angle = startRad + (i * step);
      const float cosA = std::cos(angle);
      const float sinA = std::sin(angle);

      points.push_back(x + inRx * cosA);
      points.push_back(y + inRy * sinA);

      points.push_back(x + outRx * cosA);
      points.push_back(y + outRy * sinA);
    }

    this->DrawQuadStrip(points.data(), static_cast<int>(points.size() / 2));
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUContextDevice2D::DrawEllipticArc(
  float x, float y, float rX, float rY, float startAngle, float stopAngle)
{
  int numSegments = 36;
  std::vector<float> points;

  float startRad = startAngle * vtkMath::Pi() / 180.0f;
  float stopRad = stopAngle * vtkMath::Pi() / 180.0f;
  float step = (stopRad - startRad) / numSegments;

  for (int i = 0; i <= numSegments; ++i)
  {
    float angle = startRad + (i * step);
    points.push_back(x + rX * std::cos(angle));
    points.push_back(y + rY * std::sin(angle));
  }

  this->DrawLines(points.data(), static_cast<int>(points.size() / 2));
}

//------------------------------------------------------------------------------
void vtkWebGPUContextDevice2D::DrawString(float* point, const vtkStdString& string)
{
  if (!this->InRender || string.empty())
    return;

  vtkFreeTypeTools* freeType = vtkFreeTypeTools::GetInstance();
  if (!freeType || !this->TextProp)
    return;

  double rgb[3];
  this->TextProp->GetColor(rgb);
  double opacity = this->TextProp->GetOpacity();

  std::ostringstream keyBuilder;
  keyBuilder << string << "_" << this->TextProp->GetFontFamily() << "_"
             << this->TextProp->GetFontSize() << "_" << this->TextProp->GetBold() << "_"
             << this->TextProp->GetItalic() << "_" << rgb[0] << "_" << rgb[1] << "_" << rgb[2]
             << "_" << opacity;

  std::string cacheKey = keyBuilder.str();

  int dpi = this->RenderWindow ? this->RenderWindow->GetDPI() : 72;

  auto it = this->Storage->TextImageCache.find(cacheKey);
  if (it == this->Storage->TextImageCache.end())
  {
    Private::CachedTextData newText;
    newText.Image = vtkSmartPointer<vtkImageData>::New();

    if (!freeType->RenderString(this->TextProp, string, dpi, newText.Image))
    {
      return;
    }
    freeType->GetBoundingBox(this->TextProp, string, dpi, newText.BBox);

    this->Storage->TextImageCache[cacheKey] = newText;
    it = this->Storage->TextImageCache.find(cacheKey);
  }

  vtkImageData* textImage = it->second.Image;
  if (textImage->GetNumberOfPoints() == 0)
    return;

  int dims[3];
  textImage->GetDimensions(dims);
  float width = static_cast<float>(dims[0]);
  float height = static_cast<float>(dims[1]);

  float x = point[0] + static_cast<float>(it->second.BBox[0]);
  float y = point[1] + static_cast<float>(it->second.BBox[2]);

  vtkRectf rect(x, y, width, height);
  this->DrawImage(rect, textImage);
}

void vtkWebGPUContextDevice2D::ComputeStringBounds(const vtkStdString& string, float bounds[4])
{
  vtkFreeTypeTools* freeType = vtkFreeTypeTools::GetInstance();
  if (!freeType || !this->TextProp || string.empty())
  {
    bounds[0] = bounds[1] = bounds[2] = bounds[3] = 0.0f;
    return;
  }

  int dpi = this->RenderWindow ? this->RenderWindow->GetDPI() : 72;

  int oldJustification = this->TextProp->GetJustification();
  this->TextProp->SetJustification(VTK_TEXT_LEFT);

  int bbox[4]; // [xMin, xMax, yMin, yMax]
  bool success = freeType->GetBoundingBox(this->TextProp, string, dpi, bbox);

  this->TextProp->SetJustification(oldJustification);

  if (success)
  {
    bounds[0] = static_cast<float>(bbox[0]);           // X offset
    bounds[1] = static_cast<float>(bbox[2]);           // Y offset
    bounds[2] = static_cast<float>(bbox[1] - bbox[0]); // Width
    bounds[3] = static_cast<float>(bbox[3] - bbox[2]); // Height
  }
  else
  {
    bounds[0] = bounds[1] = bounds[2] = bounds[3] = 0.0f;
  }
}

void vtkWebGPUContextDevice2D::ComputeJustifiedStringBounds(const char* string, float bounds[4])
{
  this->ComputeStringBounds(string, bounds);

  if (!this->TextProp)
  {
    return;
  }

  int justification = this->TextProp->GetJustification();
  if (justification == VTK_TEXT_CENTERED)
  {
    bounds[0] = -0.5f * bounds[2];
  }
  else if (justification == VTK_TEXT_RIGHT)
  {
    bounds[0] = -bounds[2];
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUContextDevice2D::DrawMathTextString(float point[2], const vtkStdString& string)
{
  if (!this->InRender || string.empty())
    return;

  vtkMathTextUtilities* mathText = vtkMathTextUtilities::GetInstance();
  if (!mathText || !this->TextProp)
  {
    this->DrawString(point, string);
    return;
  }

  int dpi = this->RenderWindow ? this->RenderWindow->GetDPI() : 72;

  vtkNew<vtkImageData> textImage;
  if (!mathText->RenderString(string.c_str(), textImage, this->TextProp, dpi))
  {
    return;
  }

  if (textImage->GetNumberOfPoints() == 0)
    return;

  int dims[3];
  textImage->GetDimensions(dims);
  float width = static_cast<float>(dims[0]);
  float height = static_cast<float>(dims[1]);

  int bbox[4];
  mathText->GetBoundingBox(this->TextProp, string.c_str(), dpi, bbox);

  float x = point[0] + static_cast<float>(bbox[0]);
  float y = point[1] + static_cast<float>(bbox[2]);

  vtkRectf rect(x, y, width, height);
  this->DrawImage(rect, textImage);
}

//------------------------------------------------------------------------------
void vtkWebGPUContextDevice2D::DrawPolyData(
  float offset[2], float scale, vtkPolyData* poly, vtkUnsignedCharArray* colors, int scalarMode)
{
  if (!poly || !poly->GetPoints())
  {
    return;
  }

  int nc_comps = colors ? colors->GetNumberOfComponents() : 0;
  unsigned char* colorPtr = colors ? colors->GetPointer(0) : nullptr;

  vtkIdType numPoints = poly->GetNumberOfPoints();
  std::vector<float> pts(numPoints * 2);
  for (vtkIdType i = 0; i < numPoints; ++i)
  {
    double p[3];
    poly->GetPoint(i, p);

    pts[i * 2] = (static_cast<float>(p[0]) + offset[0]) * scale;
    pts[i * 2 + 1] = (static_cast<float>(p[1]) + offset[1]) * scale;
  }

  vtkIdType cellId = poly->GetNumberOfVerts();

  vtkIdType maxCellSize = 0;
  if (poly->GetLines()->GetNumberOfCells() > 0)
    maxCellSize = std::max<vtkIdType>(maxCellSize, poly->GetLines()->GetMaxCellSize());
  if (poly->GetPolys()->GetNumberOfCells() > 0)
    maxCellSize = std::max<vtkIdType>(maxCellSize, poly->GetPolys()->GetMaxCellSize());

  std::vector<float> cellPts;
  std::vector<unsigned char> cellColors;
  cellPts.reserve(maxCellSize * 2);
  if (colors)
  {
    cellColors.reserve(maxCellSize * nc_comps);
  }

  if (poly->GetLines()->GetNumberOfCells() > 0)
  {
    vtkCellArray* lines = poly->GetLines();
    vtkIdType npts;
    const vtkIdType* pts_indices;

    for (lines->InitTraversal(); lines->GetNextCell(npts, pts_indices); ++cellId)
    {
      cellPts.resize(npts * 2);
      if (colors)
      {
        cellColors.resize(npts * nc_comps);
      }

      for (vtkIdType i = 0; i < npts; ++i)
      {
        cellPts[i * 2] = pts[pts_indices[i] * 2];
        cellPts[i * 2 + 1] = pts[pts_indices[i] * 2 + 1];

        if (colors)
        {
          if (scalarMode == VTK_SCALAR_MODE_USE_POINT_DATA)
          {
            for (int c = 0; c < nc_comps; ++c)
            {
              cellColors[i * nc_comps + c] = colorPtr[pts_indices[i] * nc_comps + c];
            }
          }
          else if (scalarMode == VTK_SCALAR_MODE_USE_CELL_DATA)
          {
            for (int c = 0; c < nc_comps; ++c)
            {
              cellColors[i * nc_comps + c] = colorPtr[cellId * nc_comps + c];
            }
          }
        }
      }

      this->DrawLines(
        cellPts.data(), static_cast<int>(npts), colors ? cellColors.data() : nullptr, nc_comps);
    }
  }

  if (poly->GetPolys()->GetNumberOfCells() > 0)
  {
    vtkCellArray* polys = poly->GetPolys();
    vtkIdType npts;
    const vtkIdType* pts_indices;

    for (polys->InitTraversal(); polys->GetNextCell(npts, pts_indices); ++cellId)
    {
      cellPts.resize(npts * 2);
      if (colors)
      {
        cellColors.resize(npts * nc_comps);
      }

      for (vtkIdType i = 0; i < npts; ++i)
      {
        cellPts[i * 2] = pts[pts_indices[i] * 2];
        cellPts[i * 2 + 1] = pts[pts_indices[i] * 2 + 1];

        if (colors)
        {
          if (scalarMode == VTK_SCALAR_MODE_USE_POINT_DATA)
          {
            for (int c = 0; c < nc_comps; ++c)
            {
              cellColors[i * nc_comps + c] = colorPtr[pts_indices[i] * nc_comps + c];
            }
          }
          else if (scalarMode == VTK_SCALAR_MODE_USE_CELL_DATA)
          {
            for (int c = 0; c < nc_comps; ++c)
            {
              cellColors[i * nc_comps + c] = colorPtr[cellId * nc_comps + c];
            }
          }
        }
      }

      if (colors)
      {
        this->DrawColoredPolygon(
          cellPts.data(), static_cast<int>(npts), cellColors.data(), nc_comps);
      }
      else
      {
        this->DrawPolygon(cellPts.data(), static_cast<int>(npts));
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUContextDevice2D::SetColor4(unsigned char color[4])
{
  this->Pen->SetColor(color[0], color[1], color[2], color[3]);
  this->Brush->SetColor(color[0], color[1], color[2], color[3]);
  if (this->InRender)
  {
    this->UpdatePaintUniforms(color);
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUContextDevice2D::SetColor(unsigned char color[3])
{
  unsigned char color4[4] = { color[0], color[1], color[2], 255 };
  this->SetColor4(color4);
}

//------------------------------------------------------------------------------
void vtkWebGPUContextDevice2D::SetTexture(vtkImageData* image, int properties)
{
  if (!image)
  {
    this->Storage->ActiveTextureBindGroup = nullptr;
    vtkErrorMacro(
      "CRITICAL: ActiveTextureBindGroup is NULL in DrawImage! Falling back to dummy texture.");
    return;
  }

  const auto& device = this->Storage->Config->GetDevice();
  if (!device)
    return;

  vtkDebugMacro("SetTexture Called");

  auto it = this->Storage->ImageCache.find(image);
  if (it != this->Storage->ImageCache.end())
  {
    if (it->second.ModifiedTime == image->GetMTime() && it->second.Properties == properties)
    {
      this->Storage->ActiveTextureBindGroup = it->second.BindGroup;
      return;
    }
    else
    {
      this->Flush();

      if (it->second.BindGroup)
        wgpuBindGroupRelease(it->second.BindGroup);
      if (it->second.View)
        wgpuTextureViewRelease(it->second.View);
      if (it->second.Texture)
        wgpuTextureRelease(it->second.Texture);
      if (it->second.Sampler)
        wgpuSamplerRelease(it->second.Sampler);
    }
  }

  int dims[3];
  image->GetDimensions(dims);
  int width = dims[0];
  int height = dims[1];

  if (width <= 0 || height <= 0)
    return;

  WGPUExtent3D extents = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
  WGPUTexture wgpuTex = this->Storage->Config->CreateTexture(extents, WGPUTextureDimension_2D,
    WGPUTextureFormat_RGBA8Unorm,
    static_cast<WGPUTextureUsage>(WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst), 1,
    "Context2D Image Texture");

  void* rawScalars = image->GetScalarPointer();
  if (rawScalars)
  {
    int numComps = image->GetNumberOfScalarComponents();
    const void* uploadData = rawScalars;
    std::vector<unsigned char> paddedData;

    if (numComps == 3)
    {
      int numPixels = width * height;
      paddedData.resize(numPixels * 4);
      const unsigned char* inData = static_cast<const unsigned char*>(rawScalars);

      for (int i = 0; i < numPixels; ++i)
      {
        paddedData[i * 4 + 0] = inData[i * 3 + 0]; // R
        paddedData[i * 4 + 1] = inData[i * 3 + 1]; // G
        paddedData[i * 4 + 2] = inData[i * 3 + 2]; // B
        paddedData[i * 4 + 3] = 255;               // A (Opaque)
      }
      uploadData = paddedData.data();
    }
    else if (numComps != 4)
    {
      vtkWarningMacro("Unsupported number of components for WebGPU texture: " << numComps);
      return;
    }

    WGPUTexelCopyTextureInfo destination{};
    destination.texture = wgpuTex;
    destination.mipLevel = 0;
    destination.origin = { 0, 0, 0 };
    destination.aspect = WGPUTextureAspect_All;

    WGPUTexelCopyBufferLayout sourceLayout{};
    sourceLayout.offset = 0;
    sourceLayout.bytesPerRow = static_cast<uint32_t>(width * 4);
    sourceLayout.rowsPerImage = static_cast<uint32_t>(height);

    WGPUExtent3D writeSize = extents;

    WGPUQueue queue = wgpuDeviceGetQueue(device);
    wgpuQueueWriteTexture(
      queue, &destination, uploadData, width * height * 4, &sourceLayout, &writeSize);
  }

  WGPUTextureView texView = this->Storage->Config->CreateView(wgpuTex, WGPUTextureViewDimension_2D,
    WGPUTextureAspect_All, WGPUTextureFormat_RGBA8Unorm, 0, 1, "Context2D Image Texture View");

  bool linearInterpolation = (properties & vtkContextDevice2D::Linear);
  bool repeatTexture = (properties & vtkContextDevice2D::Repeat);

  WGPUSamplerDescriptor samplerDesc{};
  samplerDesc.label = vtkWebGPUMakeStringView("Context2D Image Sampler");

  WGPUFilterMode filterMode = linearInterpolation ? WGPUFilterMode_Linear : WGPUFilterMode_Nearest;
  samplerDesc.magFilter = filterMode;
  samplerDesc.minFilter = filterMode;
  samplerDesc.mipmapFilter = WGPUMipmapFilterMode_Nearest;

  WGPUAddressMode addressMode =
    repeatTexture ? WGPUAddressMode_Repeat : WGPUAddressMode_ClampToEdge;
  samplerDesc.addressModeU = addressMode;
  samplerDesc.addressModeV = addressMode;
  samplerDesc.addressModeW = addressMode;

  samplerDesc.lodMinClamp = 0.0f;
  samplerDesc.lodMaxClamp = 32.0f;
  samplerDesc.maxAnisotropy = 1;

  WGPUSampler wgpuSampler = wgpuDeviceCreateSampler(device, &samplerDesc);

  std::array<WGPUBindGroupEntry, 3> bgEntries = {};

  bgEntries[0].binding = 0;
  bgEntries[0].buffer = this->Storage->MatrixUniformBuffer;
  bgEntries[0].offset = 0;
  bgEntries[0].size = 128;
  bgEntries[1].binding = 1;
  bgEntries[1].textureView = texView;
  bgEntries[2].binding = 2;
  bgEntries[2].sampler = wgpuSampler;

  WGPUBindGroupDescriptor bindGroupDesc{};
  bindGroupDesc.label = vtkWebGPUMakeStringView("Context2D Image BindGroup");
  bindGroupDesc.layout = this->Storage->BindGroupLayout;
  bindGroupDesc.entryCount = 3;
  bindGroupDesc.entries = bgEntries.data();

  WGPUBindGroup rawBindGroup = wgpuDeviceCreateBindGroup(device, &bindGroupDesc);

  Private::CachedTexture cacheEntry;
  cacheEntry.Texture = wgpuTex;
  cacheEntry.View = texView;
  cacheEntry.BindGroup = rawBindGroup;
  cacheEntry.Sampler = wgpuSampler;
  cacheEntry.ModifiedTime = image->GetMTime();
  cacheEntry.Properties = properties;
  cacheEntry.Width = width;
  cacheEntry.Height = height;

  this->Storage->ImageCache[image] = cacheEntry;
  this->Storage->ActiveTextureBindGroup = rawBindGroup;
}

//------------------------------------------------------------------------------
void vtkWebGPUContextDevice2D::SetPointSize(float size)
{
  this->Pen->SetWidth(size);
}

//------------------------------------------------------------------------------
void vtkWebGPUContextDevice2D::SetLineWidth(float width)
{
  this->Pen->SetWidth(width);
}

//------------------------------------------------------------------------------
void vtkWebGPUContextDevice2D::SetLineType(int type)
{
  this->Pen->SetLineType(type);
}

//------------------------------------------------------------------------------
void vtkWebGPUContextDevice2D::SetClipping(int* rect)
{
  if (!rect)
    return;
  this->Flush();

  this->Storage->ClipRect[0] = rect[0];
  this->Storage->ClipRect[1] = rect[1];
  this->Storage->ClipRect[2] = rect[2];
  this->Storage->ClipRect[3] = rect[3];
}

//------------------------------------------------------------------------------
void vtkWebGPUContextDevice2D::EnableClipping(bool enable)
{
  if (this->Storage->ClippingEnabled != enable)
  {
    this->Flush();
    this->Storage->ClippingEnabled = enable;
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUContextDevice2D::MultiplyMatrix(vtkMatrix3x3* m)
{
  if (!m)
    return;

  vtkNew<vtkMatrix4x4> mat4;
  mat4->Identity();

  mat4->SetElement(0, 0, m->GetElement(0, 0));
  mat4->SetElement(0, 1, m->GetElement(0, 1));
  mat4->SetElement(0, 3, m->GetElement(0, 2)); // Offset X

  mat4->SetElement(1, 0, m->GetElement(1, 0));
  mat4->SetElement(1, 1, m->GetElement(1, 1));
  mat4->SetElement(1, 3, m->GetElement(1, 2)); // Offset Y

  this->ModelMatrix->Concatenate(mat4);
  this->UpdateCameraUniforms();
}

//------------------------------------------------------------------------------
void vtkWebGPUContextDevice2D::SetMatrix(vtkMatrix3x3* m)
{
  if (!m)
    return;

  vtkNew<vtkMatrix4x4> mat4;
  mat4->Identity();

  mat4->SetElement(0, 0, m->GetElement(0, 0));
  mat4->SetElement(0, 1, m->GetElement(0, 1));
  mat4->SetElement(0, 3, m->GetElement(0, 2)); // Offset X

  mat4->SetElement(1, 0, m->GetElement(1, 0));
  mat4->SetElement(1, 1, m->GetElement(1, 1));
  mat4->SetElement(1, 3, m->GetElement(1, 2)); // Offset Y

  this->ModelMatrix->SetMatrix(mat4);
  this->UpdateCameraUniforms();
}

//------------------------------------------------------------------------------
void vtkWebGPUContextDevice2D::GetMatrix(vtkMatrix3x3* m)
{
  if (!m)
    return;

  vtkMatrix4x4* mat4 = this->ModelMatrix->GetMatrix();

  m->SetElement(0, 0, mat4->GetElement(0, 0));
  m->SetElement(0, 1, mat4->GetElement(0, 1));
  m->SetElement(0, 2, mat4->GetElement(0, 3));

  m->SetElement(1, 0, mat4->GetElement(1, 0));
  m->SetElement(1, 1, mat4->GetElement(1, 1));
  m->SetElement(1, 2, mat4->GetElement(1, 3));

  m->SetElement(2, 0, 0.0);
  m->SetElement(2, 1, 0.0);
  m->SetElement(2, 2, 1.0);
}

//------------------------------------------------------------------------------
void vtkWebGPUContextDevice2D::PushMatrix()
{
  this->ModelMatrix->Push();
  this->UpdateCameraUniforms();
}

//------------------------------------------------------------------------------
void vtkWebGPUContextDevice2D::PopMatrix()
{
  this->ModelMatrix->Pop();
  this->UpdateCameraUniforms();
}

//------------------------------------------------------------------------------
void vtkWebGPUContextDevice2D::ApplyScissorRect(WGPURenderPassEncoder passEncoder)
{
  if (!passEncoder || !this->RenderWindow)
    return;

  int* size = this->RenderWindow->GetSize();
  uint32_t x = 0;
  uint32_t y = 0;
  uint32_t width = static_cast<uint32_t>(size[0]);
  uint32_t height = static_cast<uint32_t>(size[1]);

  if (this->Storage->ClippingEnabled)
  {
    x = static_cast<uint32_t>(std::max(0, this->Storage->ClipRect[0]));
    y = static_cast<uint32_t>(std::max(0, this->Storage->ClipRect[1]));

    int clampedWidth = this->Storage->ClipRect[2];
    int clampedHeight = this->Storage->ClipRect[3];

    if (static_cast<int>(x) + clampedWidth > size[0])
    {
      clampedWidth = size[0] - static_cast<int>(x);
    }
    if (static_cast<int>(y) + clampedHeight > size[1])
    {
      clampedHeight = size[1] - static_cast<int>(y);
    }

    width = static_cast<uint32_t>(std::max(0, clampedWidth));
    height = static_cast<uint32_t>(std::max(0, clampedHeight));
  }

  wgpuRenderPassEncoderSetScissorRect(passEncoder, x, y, width, height);
}

//------------------------------------------------------------------------------
void vtkWebGPUContextDevice2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

VTK_ABI_NAMESPACE_END
