// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor2D.h"
#include "vtkArrayDispatch.h"
#include "vtkDataArray.h"
#include "vtkMatrix4x4.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty2D.h"
#include "vtkWebGPUCellToPrimitiveConverter.h"
#include "vtkWebGPUCommandEncoderDebugGroup.h"
#include "vtkWebGPUComputePass.h"
#include "vtkWebGPUPolyDataMapper2D.h"
#include "vtkWebGPURenderWindow.h"
#include "vtkWebGPURenderer.h"

#include "PolyData2D.h"

#include "Private/vtkWebGPUBindGroupInternals.h"
#include "Private/vtkWebGPUBindGroupLayoutInternals.h"
#include "Private/vtkWebGPUPipelineLayoutInternals.h"
#include "Private/vtkWebGPUPolyDataMapper2DInternals.h"
#include "Private/vtkWebGPURenderPipelineDescriptorInternals.h"

VTK_ABI_NAMESPACE_BEGIN

namespace
{
template <typename DestT>
struct WriteTypedArray
{
  std::size_t ByteOffset = 0;
  const wgpu::Buffer& DstBuffer;
  vtkSmartPointer<vtkWebGPUConfiguration> WGPUConfiguration;
  float Denominator = 1.0;

  template <typename SrcArrayT>
  void operator()(SrcArrayT* array, const char* description)
  {
    if (array == nullptr || this->DstBuffer.Get() == nullptr)
    {
      return;
    }
    const auto values = vtk::DataArrayValueRange(array);
    vtkNew<vtkAOSDataArrayTemplate<DestT>> data;
    for (const auto& value : values)
    {
      data->InsertNextValue(value / this->Denominator);
    }
    const std::size_t nbytes = data->GetNumberOfValues() * sizeof(DestT);
    this->WGPUConfiguration->WriteBuffer(
      this->DstBuffer, this->ByteOffset, data->GetPointer(0), nbytes, description);
    this->ByteOffset += nbytes;
  }
};
}

//------------------------------------------------------------------------------
vtkWebGPUPolyDataMapper2DInternals::vtkWebGPUPolyDataMapper2DInternals() = default;

//------------------------------------------------------------------------------
vtkWebGPUPolyDataMapper2DInternals::~vtkWebGPUPolyDataMapper2DInternals() = default;

//------------------------------------------------------------------------------
wgpu::BindGroupLayout vtkWebGPUPolyDataMapper2DInternals::CreateMeshAttributeBindGroupLayout(
  const wgpu::Device& device, const std::string& label)
{
  return vtkWebGPUBindGroupLayoutInternals::MakeBindGroupLayout(device,
    {
      // clang-format off
      // state
      { 0, wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment, wgpu::BufferBindingType::ReadOnlyStorage },
      // mesh_descriptor
      { 1, wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment, wgpu::BufferBindingType::ReadOnlyStorage },
      // mesh_data
      { 2, wgpu::ShaderStage::Vertex, wgpu::BufferBindingType::ReadOnlyStorage },
      // clang-format on
    },
    label);
}

//------------------------------------------------------------------------------
wgpu::BindGroupLayout vtkWebGPUPolyDataMapper2DInternals::CreateTopologyBindGroupLayout(
  const wgpu::Device& device, const std::string& label)
{
  return vtkWebGPUBindGroupLayoutInternals::MakeBindGroupLayout(device,
    {
      // clang-format off
      // topology
      { 0, wgpu::ShaderStage::Vertex, wgpu::BufferBindingType::ReadOnlyStorage },
      // clang-format on
    },
    label);
}

//------------------------------------------------------------------------------
const char* vtkWebGPUPolyDataMapper2DInternals::GetGraphicsPipelineTypeAsString(
  GraphicsPipeline2DType graphicsPipelineType)
{
  switch (graphicsPipelineType)
  {
    case GFX_PIPELINE_2D_POINTS:
      return "GFX_PIPELINE_POINTS";
    case GFX_PIPELINE_2D_LINES:
      return "GFX_PIPELINE_LINES";
    case GFX_PIPELINE_2D_TRIANGLES:
      return "GFX_PIPELINE_2D_TRIANGLES";
    default:
      return "";
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper2DInternals::ReleaseGraphicsResources(vtkWindow* w)
{
  this->CellConverter->ReleaseGraphicsResources(w);
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper2DInternals::UpdateBuffers(
  vtkViewport* viewport, vtkActor2D* actor, vtkWebGPUPolyDataMapper2D* mapper)
{
  vtkPolyData* input = mapper->GetInput();
  if (input == nullptr)
  {
    vtkErrorWithObjectMacro(mapper, << "No input!");
    return;
  }
  mapper->GetInputAlgorithm()->Update();
  vtkIdType numPts = input->GetNumberOfPoints();
  if (numPts == 0)
  {
    vtkDebugWithObjectMacro(mapper, << "No points!");
    return;
  }
  if (mapper->LookupTable == nullptr)
  {
    mapper->CreateDefaultLookupTable();
  }

  auto* wgpuRenderer = vtkWebGPURenderer::SafeDownCast(viewport);
  auto* wgpuRenderWindow = vtkWebGPURenderWindow::SafeDownCast(viewport->GetVTKWindow());
  auto* wgpuConfiguration = wgpuRenderWindow->GetWGPUConfiguration();

  bool recreateMeshBindGroup = false;
  if (this->Mapper2DStateData.Buffer == nullptr)
  {
    const auto label = "Mapper2DState-" + input->GetObjectDescription();
    this->Mapper2DStateData.Buffer = wgpuConfiguration->CreateBuffer(sizeof(Mapper2DState),
      wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage, false, label.c_str());
    this->Mapper2DStateData.Size = sizeof(Mapper2DState);
    const auto& device = wgpuConfiguration->GetDevice();
    recreateMeshBindGroup = true;
  }
  if ((this->Mapper2DStateData.BuildTimeStamp < actor->GetProperty()->GetMTime()) ||
    (this->Mapper2DStateData.BuildTimeStamp < actor->GetPositionCoordinate()->GetMTime()) ||
    (this->Mapper2DStateData.BuildTimeStamp < viewport->GetMTime()) ||
    (this->Mapper2DStateData.BuildTimeStamp < viewport->GetVTKWindow()->GetMTime()))
  {
    // Get the position of the actor
    int size[2];
    size[0] = viewport->GetSize()[0];
    size[1] = viewport->GetSize()[1];

    double* vport = viewport->GetViewport();
    int* actorPos = actor->GetPositionCoordinate()->GetComputedViewportValue(viewport);

    // get window info
    double* tileViewPort = viewport->GetVTKWindow()->GetTileViewport();
    double visVP[4];
    visVP[0] = (vport[0] >= tileViewPort[0]) ? vport[0] : tileViewPort[0];
    visVP[1] = (vport[1] >= tileViewPort[1]) ? vport[1] : tileViewPort[1];
    visVP[2] = (vport[2] <= tileViewPort[2]) ? vport[2] : tileViewPort[2];
    visVP[3] = (vport[3] <= tileViewPort[3]) ? vport[3] : tileViewPort[3];
    if (visVP[0] >= visVP[2])
    {
      return;
    }
    if (visVP[1] >= visVP[3])
    {
      return;
    }
    size[0] = static_cast<int>(std::round(size[0] * (visVP[2] - visVP[0]) / (vport[2] - vport[0])));
    size[1] = static_cast<int>(std::round(size[1] * (visVP[3] - visVP[1]) / (vport[3] - vport[1])));

    const int* winSize = viewport->GetVTKWindow()->GetSize();

    int xoff = static_cast<int>(actorPos[0] - (visVP[0] - vport[0]) * winSize[0]);
    int yoff = static_cast<int>(actorPos[1] - (visVP[1] - vport[1]) * winSize[1]);

    // set ortho projection
    float left = -xoff;
    float right = -xoff + size[0];
    float bottom = -yoff;
    float top = -yoff + size[1];

    // it's an error to call glOrtho with
    // either left==right or top==bottom
    if (left == right)
    {
      right = left + 1.0;
    }
    if (bottom == top)
    {
      top = bottom + 1.0;
    }

    // compute the combined ModelView matrix and send it down to save time in the shader
    this->WCVCMatrix->Identity();
    this->WCVCMatrix->SetElement(0, 0, 2.0 / (right - left));
    this->WCVCMatrix->SetElement(1, 1, -2.0 / (top - bottom));
    this->WCVCMatrix->SetElement(0, 3, -1.0 * (right + left) / (right - left));
    this->WCVCMatrix->SetElement(1, 3, 1.0 * (top + bottom) / (top - bottom));
    this->WCVCMatrix->SetElement(2, 2, 0.0);
    this->WCVCMatrix->SetElement(
      2, 3, actor->GetProperty()->GetDisplayLocation() == VTK_FOREGROUND_LOCATION ? 0.0 : 1.0);
    this->WCVCMatrix->SetElement(3, 3, 1.0);
    // transpose and convert from double to float in one nested loop.
    for (int i = 0; i < 4; ++i)
    {
      for (int j = 0; j < 4; ++j)
      {
        // transpose because, shader will interpret it in a column-major order.
        this->State.WCVCMatrix[j][i] = this->WCVCMatrix->GetElement(i, j);
      }
    }
    std::array<double, 3> color = {};
    actor->GetProperty()->GetColor(color.data());
    std::copy(color.begin(), color.end(), this->State.Color);

    this->State.Color[3] = actor->GetProperty()->GetOpacity();

    this->State.PointSize = actor->GetProperty()->GetPointSize();

    this->State.LineWidth = actor->GetProperty()->GetLineWidth();

    this->State.Flags = (this->UseCellScalarMapping ? 1 : 0);
    this->State.Flags |= ((this->UsePointScalarMapping ? 1 : 0) << 1);
    wgpuConfiguration->WriteBuffer(
      this->Mapper2DStateData.Buffer, 0, &(this->State), sizeof(Mapper2DState), "Mapper2DState");
  }

  if (this->MeshData.BuildTimeStamp < mapper->GetMTime() ||
    this->MeshData.BuildTimeStamp < actor->GetMTime() ||
    this->MeshData.BuildTimeStamp < input->GetMTime() ||
    (mapper->TransformCoordinate &&
      (this->MeshData.BuildTimeStamp < viewport->GetMTime() ||
        this->MeshData.BuildTimeStamp < viewport->GetVTKWindow()->GetMTime())))
  {
    // update point data buffer.
    mapper->MapScalars(actor->GetProperty()->GetOpacity());
    if (mapper->Colors != nullptr && mapper->Colors->GetNumberOfValues() > 0)
    {
      this->UsePointScalarMapping = true;
    }
    if (mapper->ScalarVisibility)
    {
      // We must figure out how the scalars should be mapped to the polydata.
      if ((mapper->ScalarMode == VTK_SCALAR_MODE_USE_CELL_DATA ||
            mapper->ScalarMode == VTK_SCALAR_MODE_USE_CELL_FIELD_DATA ||
            mapper->ScalarMode == VTK_SCALAR_MODE_USE_FIELD_DATA ||
            !input->GetPointData()->GetScalars()) &&
        mapper->ScalarMode != VTK_SCALAR_MODE_USE_POINT_FIELD_DATA && mapper->Colors)
      {
        this->UseCellScalarMapping = true;
        this->UsePointScalarMapping = false;
      }
    }
    this->State.Flags = (this->UseCellScalarMapping ? 1 : 0);
    this->State.Flags |= ((this->UsePointScalarMapping ? 1 : 0) << 1);
    wgpuConfiguration->WriteBuffer(
      this->Mapper2DStateData.Buffer, 0, &(this->State), sizeof(Mapper2DState), "Mapper2DState");
    vtkDataArray* pointPositions = input->GetPoints()->GetData();
    // Transform the points, if necessary
    if (mapper->TransformCoordinate)
    {
      if (!this->TransformedPoints)
      {
        this->TransformedPoints = vtk::TakeSmartPointer(vtkPoints::New());
      }
      this->TransformedPoints->SetNumberOfPoints(numPts);
      for (vtkIdType j = 0; j < numPts; j++)
      {
        mapper->TransformCoordinate->SetValue(pointPositions->GetTuple(j));
        if (mapper->TransformCoordinateUseDouble)
        {
          double* dtmp = mapper->TransformCoordinate->GetComputedDoubleViewportValue(viewport);
          this->TransformedPoints->SetPoint(j, dtmp[0], dtmp[1], 0.0);
        }
        else
        {
          int* itmp = mapper->TransformCoordinate->GetComputedViewportValue(viewport);
          this->TransformedPoints->SetPoint(j, itmp[0], itmp[1], 0.0);
        }
      }
      pointPositions = this->TransformedPoints->GetData();
    }
    vtkDataArray* pointColors =
      this->UsePointScalarMapping ? vtkDataArray::SafeDownCast(mapper->Colors) : nullptr;
    vtkDataArray* pointUVs = input->GetPointData()->GetTCoords();
    vtkDataArray* cellColors =
      this->UseCellScalarMapping ? vtkDataArray::SafeDownCast(mapper->Colors) : nullptr;
    std::size_t requiredBufferSize = 0;
    for (auto* array : { pointPositions, pointUVs, pointColors, cellColors })
    {
      if (array != nullptr)
      {
        requiredBufferSize += sizeof(vtkTypeFloat32) * array->GetDataSize();
      }
    }
    if (requiredBufferSize != this->MeshData.Size)
    {
      this->MeshData.Buffer = nullptr;
    }

    if (this->MeshData.Buffer == nullptr)
    {
      recreateMeshBindGroup = true;
      const auto label = "MeshAttributes-" + input->GetObjectDescription();
      this->MeshData.Buffer = wgpuConfiguration->CreateBuffer(requiredBufferSize,
        wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage, false, label.c_str());
      using DispatchT = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::AllTypes>;
      ::WriteTypedArray<vtkTypeFloat32> meshDataWriter{ 0, this->MeshData.Buffer, wgpuConfiguration,
        1. };

      // Upload positions
      this->MeshArraysDescriptor.Positions.Start =
        meshDataWriter.ByteOffset / sizeof(vtkTypeFloat32);
      if (!DispatchT::Execute(pointPositions, meshDataWriter, "Positions"))
      {
        meshDataWriter(pointPositions, "Positions");
      }
      this->MeshArraysDescriptor.Positions.NumComponents = pointPositions->GetNumberOfComponents();
      this->MeshArraysDescriptor.Positions.NumTuples = pointPositions->GetNumberOfTuples();
      // Upload point UVs
      if (pointUVs)
      {
        this->MeshArraysDescriptor.UVs.Start = meshDataWriter.ByteOffset / sizeof(vtkTypeFloat32);
        if (!DispatchT::Execute(pointUVs, meshDataWriter, "UVs"))
        {
          meshDataWriter(pointUVs, "UVs");
        }
        this->MeshArraysDescriptor.UVs.NumComponents = pointUVs->GetNumberOfComponents();
        this->MeshArraysDescriptor.UVs.NumTuples = pointUVs->GetNumberOfTuples();
      }
      // Upload point colors
      if (pointColors && this->UsePointScalarMapping)
      {
        meshDataWriter.Denominator = 255.0;
        this->MeshArraysDescriptor.Colors.Start =
          meshDataWriter.ByteOffset / sizeof(vtkTypeFloat32);
        if (!DispatchT::Execute(pointColors, meshDataWriter, "PointColors"))
        {
          meshDataWriter(pointColors, "PointColors");
        }
        meshDataWriter.Denominator = 1.0;
        this->MeshArraysDescriptor.Colors.NumComponents = pointColors->GetNumberOfComponents();
        this->MeshArraysDescriptor.Colors.NumTuples = pointColors->GetNumberOfTuples();
      }
      // Upload cell colors
      else if (cellColors && this->UseCellScalarMapping)
      {
        meshDataWriter.Denominator = 255.0;
        this->MeshArraysDescriptor.Colors.Start =
          meshDataWriter.ByteOffset / sizeof(vtkTypeFloat32);
        if (!DispatchT::Execute(cellColors, meshDataWriter, "CellColors"))
        {
          meshDataWriter(cellColors, "CellColors");
        }
        meshDataWriter.Denominator = 1.0;
        this->MeshArraysDescriptor.Colors.NumComponents = cellColors->GetNumberOfComponents();
        this->MeshArraysDescriptor.Colors.NumTuples = cellColors->GetNumberOfTuples();
      }
      this->MeshData.Size = requiredBufferSize;
    }
    const std::string meshAttrDescriptorLabel =
      "MeshAttributeDescriptor-" + input->GetObjectDescription();
    if (this->AttributeDescriptorData.Buffer == nullptr)
    {
      recreateMeshBindGroup = true;
      this->AttributeDescriptorData.Buffer = wgpuConfiguration->CreateBuffer(
        sizeof(this->MeshArraysDescriptor), wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage,
        false, meshAttrDescriptorLabel.c_str());
      this->AttributeDescriptorData.Size = sizeof(this->MeshArraysDescriptor);
    }
    wgpuConfiguration->WriteBuffer(this->AttributeDescriptorData.Buffer, 0,
      &this->MeshArraysDescriptor, sizeof(this->MeshArraysDescriptor),
      meshAttrDescriptorLabel.c_str());
    this->AttributeDescriptorData.BuildTimeStamp.Modified();
    this->MeshData.BuildTimeStamp.Modified();

    if (recreateMeshBindGroup)
    {
      const auto& device = wgpuConfiguration->GetDevice();
      auto layout =
        this->CreateMeshAttributeBindGroupLayout(device, "MeshAttributeBindGroup_LAYOUT");
      this->MeshAttributeBindGroup = vtkWebGPUBindGroupInternals::MakeBindGroup(device, layout,
        {
          // clang-format off
        { 0, this->Mapper2DStateData.Buffer, 0 },
        { 1, this->AttributeDescriptorData.Buffer, 0 },
        { 2, this->MeshData.Buffer, 0 },
          // clang-format on
        },
        "MeshAttributeBindGroup");
      this->RebuildGraphicsPipelines = true;
      // Invalidate render bundle because bindgroup was recreated.
      wgpuRenderer->InvalidateBundle();
    }
  }

  vtkTypeUInt32* vertexCounts[vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES];
  wgpu::Buffer* topologyBuffers[vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES];
  wgpu::Buffer* edgeArrayBuffers[vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES];
  for (int i = 0; i < vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES; ++i)
  {
    auto& bgInfo = this->TopologyBindGroupInfos[i];
    vertexCounts[i] = &(bgInfo.VertexCount);
    topologyBuffers[i] = &(bgInfo.TopologyBuffer);
    edgeArrayBuffers[i] = nullptr;
  }
  bool updateTopologyBindGroup = this->CellConverter->DispatchMeshToPrimitiveComputePipeline(
    wgpuConfiguration, input, VTK_SURFACE, vertexCounts, topologyBuffers, edgeArrayBuffers);

  // Rebuild topology bind group if required (when VertexCount > 0)
  for (int i = 0; i < vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES; ++i)
  {
    const auto topologySourceType = vtkWebGPUCellToPrimitiveConverter::TopologySourceType(i);
    auto& bgInfo = this->TopologyBindGroupInfos[i];
    // setup bind group
    if (updateTopologyBindGroup && bgInfo.VertexCount > 0)
    {
      const std::string& label =
        vtkWebGPUCellToPrimitiveConverter::GetTopologySourceTypeAsString(topologySourceType);
      const auto& device = wgpuConfiguration->GetDevice();
      auto layout = this->CreateTopologyBindGroupLayout(device, "TopologyBindGroup_LAYOUT");
      bgInfo.BindGroup = vtkWebGPUBindGroupInternals::MakeBindGroup(device, layout,
        {
          // clang-format off
        { 0, bgInfo.TopologyBuffer, 0}
          // clang-format on
        },
        "TopologyBindGroup");
    }
    else if (bgInfo.VertexCount == 0)
    {
      if (bgInfo.TopologyBuffer)
      {
        bgInfo.TopologyBuffer.Destroy();
        bgInfo.TopologyBuffer = nullptr;
      }
      bgInfo.BindGroup = nullptr;
    }
    this->RebuildGraphicsPipelines = true;
  }

  if (this->RebuildGraphicsPipelines)
  {
    const auto& device = wgpuConfiguration->GetDevice();
    auto* wgpuPipelineCache = wgpuRenderWindow->GetWGPUPipelineCache();

    vtkWebGPURenderPipelineDescriptorInternals descriptor;
    descriptor.vertex.bufferCount = 0;
    descriptor.cFragment.entryPoint = "fragmentMain";
    descriptor.EnableBlending(0);
    descriptor.cTargets[0].format = wgpuRenderWindow->GetPreferredSurfaceTextureFormat();
    ///@{ TODO: Only for valid depth stencil formats
    auto depthState = descriptor.EnableDepthStencil(wgpuRenderWindow->GetDepthStencilFormat());
    depthState->depthWriteEnabled = true;
    depthState->depthCompare = wgpu::CompareFunction::Less;
    ///@}
    // Prepare selection ids output.
    descriptor.cTargets[1].format = wgpuRenderWindow->GetPreferredSelectorIdsTextureFormat();
    descriptor.cFragment.targetCount++;
    descriptor.DisableBlending(1);

    // Update local parameters that decide whether a pipeline must be rebuilt.
    this->RebuildGraphicsPipelines = false;
    descriptor.primitive.cullMode = wgpu::CullMode::None;

    descriptor.layout = vtkWebGPUPipelineLayoutInternals::MakePipelineLayout(device,
      { this->CreateMeshAttributeBindGroupLayout(device, "MeshAttributeBindGroupLayout"),
        this->CreateTopologyBindGroupLayout(device, "TopologyBindGroupLayout") },
      "vtkPolyDataMapper2DPipelineLayout");

    for (int i = 0; i < GraphicsPipeline2DType::NUM_GFX_PIPELINE_2D_NB_TYPES; ++i)
    {
      descriptor.vertex.entryPoint = this->VertexShaderEntryPoints[i].c_str();
      descriptor.label =
        this->GetGraphicsPipelineTypeAsString(static_cast<GraphicsPipeline2DType>(i));
      descriptor.primitive.topology = this->GraphicsPipeline2DPrimitiveTypes[i];
      // generate a unique key for the pipeline descriptor and shader source pointer
      this->GraphicsPipeline2DKeys[i] = wgpuPipelineCache->GetPipelineKey(&descriptor, PolyData2D);
      // create a pipeline if it does not already exist
      if (wgpuPipelineCache->GetRenderPipeline(this->GraphicsPipeline2DKeys[i]) == nullptr)
      {
        wgpuPipelineCache->CreateRenderPipeline(&descriptor, wgpuRenderer, PolyData2D);
      }
    }
    // Invalidate render bundle because pipeline was recreated.
    wgpuRenderer->InvalidateBundle();
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper2DInternals::RecordDrawCommands(
  vtkViewport* viewport, const wgpu::RenderPassEncoder& encoder)
{
  auto* wgpuRenderWindow = vtkWebGPURenderWindow::SafeDownCast(viewport->GetVTKWindow());
  auto* wgpuPipelineCache = wgpuRenderWindow->GetWGPUPipelineCache();

  encoder.SetBindGroup(0, this->MeshAttributeBindGroup);
  for (const auto& pipelineMapping : this->PipelineBindGroupCombos)
  {
    const auto& pipelineType = pipelineMapping.first;
    const auto& pipelineKey = this->GraphicsPipeline2DKeys[pipelineType];
    encoder.SetPipeline(wgpuPipelineCache->GetRenderPipeline(pipelineKey));
    const auto& pipelineLabel = this->GetGraphicsPipelineTypeAsString(pipelineType);
    vtkScopedEncoderDebugGroup(encoder, pipelineLabel);
    const auto topologySourceType = pipelineMapping.second;

    const auto& bgInfo = this->TopologyBindGroupInfos[topologySourceType];
    if (bgInfo.VertexCount == 0)
    {
      continue;
    }
    encoder.SetBindGroup(1, bgInfo.BindGroup);
    const auto topologyBGInfoName =
      vtkWebGPUCellToPrimitiveConverter::GetTopologySourceTypeAsString(topologySourceType);
    vtkScopedEncoderDebugGroup(encoder, topologyBGInfoName);
    switch (topologySourceType)
    {
      case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_VERTS:
        encoder.Draw(/*vertexCount=*/4, /*instanceCount=*/bgInfo.VertexCount);
        break;
      case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_LINES:
        encoder.Draw(/*vertexCount=*/4, /*instanceCount=*/bgInfo.VertexCount / 2);
        break;
      case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGONS:
        encoder.Draw(/*vertexCount=*/bgInfo.VertexCount, /*instanceCount=*/1);
        break;
      case vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES:
      default:
        break;
    }
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper2DInternals::RecordDrawCommands(
  vtkViewport* viewport, const wgpu::RenderBundleEncoder& encoder)
{
  auto* wgpuRenderWindow = vtkWebGPURenderWindow::SafeDownCast(viewport->GetVTKWindow());
  auto* wgpuPipelineCache = wgpuRenderWindow->GetWGPUPipelineCache();

  encoder.SetBindGroup(0, this->MeshAttributeBindGroup);
  for (const auto& pipelineMapping : this->PipelineBindGroupCombos)
  {
    const auto& pipelineType = pipelineMapping.first;
    const auto& pipelineKey = this->GraphicsPipeline2DKeys[pipelineType];
    encoder.SetPipeline(wgpuPipelineCache->GetRenderPipeline(pipelineKey));
    const auto& pipelineLabel = this->GetGraphicsPipelineTypeAsString(pipelineType);
    vtkScopedEncoderDebugGroup(encoder, pipelineLabel);
    const auto topologySourceType = pipelineMapping.second;

    const auto& bgInfo = this->TopologyBindGroupInfos[topologySourceType];
    if (bgInfo.VertexCount == 0)
    {
      continue;
    }
    encoder.SetBindGroup(1, bgInfo.BindGroup);
    const auto topologyBGInfoName =
      vtkWebGPUCellToPrimitiveConverter::GetTopologySourceTypeAsString(topologySourceType);
    vtkScopedEncoderDebugGroup(encoder, topologyBGInfoName);
    switch (topologySourceType)
    {
      case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_VERTS:
        encoder.Draw(/*vertexCount=*/4, /*instanceCount=*/bgInfo.VertexCount);
        break;
      case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_LINES:
        encoder.Draw(/*vertexCount=*/4, /*instanceCount=*/bgInfo.VertexCount / 2);
        break;
      case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGONS:
        encoder.Draw(/*vertexCount=*/bgInfo.VertexCount, /*instanceCount=*/1);
        break;
      case vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES:
      default:
        break;
    }
  }
}

VTK_ABI_NAMESPACE_END
