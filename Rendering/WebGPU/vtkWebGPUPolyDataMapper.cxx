/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWebGPUPolyDataMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkWebGPUPolyDataMapper.h"
#include "vtkArrayDispatch.h"
#include "vtkCellArray.h"
#include "vtkCellArrayIterator.h"
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkMath.h"
#include "vtkMatrix3x3.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSetGet.h"
#include "vtkType.h"
#include "vtkTypeFloat32Array.h"
#include "vtkTypeUInt32Array.h"
#include "vtkUnsignedCharArray.h"
#include "vtkWGPUContext.h"
#include "vtkWebGPUActor.h"
#include "vtkWebGPUCamera.h"
#include "vtkWebGPUInternalsBindGroup.h"
#include "vtkWebGPUInternalsBindGroupLayout.h"
#include "vtkWebGPUInternalsBuffer.h"
#include "vtkWebGPUInternalsPipelineLayout.h"
#include "vtkWebGPUInternalsRenderPipelineDescriptor.h"
#include "vtkWebGPUInternalsShaderModule.h"
#include "vtkWebGPURenderWindow.h"
#include "vtkWebGPURenderer.h"

#include "PolyData.h"

#include <algorithm>
#include <numeric>
#include <type_traits>

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkWebGPUPolyDataMapper);

//------------------------------------------------------------------------------
vtkWebGPUPolyDataMapper::vtkWebGPUPolyDataMapper() = default;

//------------------------------------------------------------------------------
vtkWebGPUPolyDataMapper::~vtkWebGPUPolyDataMapper() = default;

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::PrintSelf(ostream& os, vtkIndent indent) {}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::RenderPiece(vtkRenderer* renderer, vtkActor* actor)
{
  auto wgpuRenWin = vtkWebGPURenderWindow::SafeDownCast(renderer->GetRenderWindow());
  // Note for emscripten: the indirection to js get time now is a bit costly. it can quickly add up
  // for really large number of actors. However, vtkRenderWindow caps it to 5 times per second. the
  // cost of this check abort is about 0.2ms per call in emscripten. So, 1 millisecond is the
  // guaranteed cost per number of frames rendered in a second.
  if (wgpuRenWin->CheckAbortStatus())
  {
    return;
  }

  const auto device = wgpuRenWin->GetDevice();
  auto wgpuActor = reinterpret_cast<vtkWebGPUActor*>(actor);
  auto wgpuRenderer = reinterpret_cast<vtkWebGPURenderer*>(renderer);

  using RenderType = vtkWebGPUActor::MapperRenderType;
  auto renderType = wgpuActor->GetMapperRenderType();
  switch (renderType)
  {
    case RenderType::UpdateBuffers:
    {
      if (!this->InitializedPipeline)
      {
        this->SetupPipelineLayout(device, renderer, actor);
        this->SetupGraphicsPipeline(device, renderer, actor);
        this->InitializedPipeline = true;
      }
      this->UpdateMeshGeometryBuffers(device, actor);
      this->UpdateMeshIndexBuffers(device);
      this->SetupBindGroups(device, renderer);
      break;
    }
    case RenderType::RenderPassEncode:
    {
      this->EncodeRenderCommands(renderer, actor);
    }
    case RenderType::None:
    default:
      break;
  }
}

void vtkWebGPUPolyDataMapper::EncodeRenderCommands(vtkRenderer* renderer, vtkActor* actor)
{
  auto wgpuRenderer = reinterpret_cast<vtkWebGPURenderer*>(renderer);

  wgpu::RenderPassEncoder passEncoder = wgpuRenderer->GetRenderPassEncoder();
  passEncoder.PushDebugGroup("vtkWebGPUPolyDataMapper::EncodeRenderCommands");
  passEncoder.SetBindGroup(2, this->MeshAttributeBindGroup);

  {
    uint32_t vcFactor[3] = { 1, 1, 1 };
    uint32_t instanceCount[3] = { 1, 1, 1 };
    const int representation = actor->GetProperty()->GetRepresentation();
    switch (representation)
    {
      case VTK_POINTS:
        // clang-format off
      vcFactor[0] = 6; // A VTK_POINT is represented as a point using 2 triangles.
      vcFactor[1] = 6; // A VTK_LINE is represented as 2 vertices using 2 triangles for each vertex of the line, overall 2*2=4 triangles are used
      vcFactor[2] = 6; // A VTK_TRIANGLE is represented as 3 vertces using 2 triangles for each vertex of the triangle, overall 3*2=6 triangles are used.
      break;
    case VTK_WIREFRAME:
      vcFactor[0] = 0; // A VTK_POINT cannot be represented as a wireframe!
      vcFactor[1] = 1; // A VTK_LINE is represented with wireframe using 1 line and some number of instances.
      instanceCount[1] = 2 * vtkMath::Ceil(actor->GetProperty()->GetLineWidth());
      vcFactor[2] = 1; // A VTK_TRIANGLE is represented with wireframe using 1 triangle without the interior region. shader discards interior fragments.
      break;
    case VTK_SURFACE:
    default:
      vcFactor[0] = 6; // A VTK_POINT is represented as a surface using 2 triangles.
      vcFactor[1] = 1; // A VTK_LINE is represented with wireframe using 1 line and some number of instances.
      instanceCount[1] = 2 * vtkMath::Ceil(actor->GetProperty()->GetLineWidth());
      vcFactor[2] = 1; // A VTK_TRIANGLE is represented as a surface using 1 triangle.
      break;
        // clang-format on
    }
    if (this->PointPrimitiveBGInfo.PipelineID != wgpuRenderer->GetCurrentPipelineID() &&
      this->PointPrimitiveBGInfo.VertexCount > 0)
    {
#ifndef NDEBUG
      passEncoder.PushDebugGroup("VTK_POINT");
#endif
      passEncoder.SetBindGroup(3, this->PointPrimitiveBGInfo.BindGroup);
      passEncoder.Draw(this->PointPrimitiveBGInfo.VertexCount * vcFactor[0], instanceCount[0]);
#ifndef NDEBUG
      passEncoder.PopDebugGroup();
#endif
    }
    else if (this->LinePrimitiveBGInfo.PipelineID != wgpuRenderer->GetCurrentPipelineID() &&
      this->LinePrimitiveBGInfo.VertexCount > 0)
    {
#ifndef NDEBUG
      passEncoder.PushDebugGroup("VTK_LINE");
#endif
      passEncoder.SetBindGroup(3, this->LinePrimitiveBGInfo.BindGroup);
      passEncoder.Draw(this->LinePrimitiveBGInfo.VertexCount * vcFactor[1], instanceCount[1]);
#ifndef NDEBUG
      passEncoder.PopDebugGroup();
#endif
    }
    else if (this->TrianglePrimitiveBGInfo.PipelineID != wgpuRenderer->GetCurrentPipelineID() &&
      this->TrianglePrimitiveBGInfo.VertexCount > 0)
    {
#ifndef NDEBUG
      passEncoder.PushDebugGroup("VTK_TRIANGLE");
#endif
      passEncoder.SetBindGroup(3, this->TrianglePrimitiveBGInfo.BindGroup);
      passEncoder.Draw(this->TrianglePrimitiveBGInfo.VertexCount * vcFactor[2], instanceCount[2]);
#ifndef NDEBUG
      passEncoder.PopDebugGroup();
#endif
    }
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::SetupPipelineLayout(
  const wgpu::Device& device, vtkRenderer* renderer, vtkActor* actor)
{
  this->MeshAttributeBindGroupLayout =
    vtkWebGPUInternalsBindGroupLayout::MakeBindGroupLayout(device,
      {
        // clang-format off
        // MeshAttributeArrayDescriptor
        { 0, wgpu::ShaderStage::Vertex, wgpu::BufferBindingType::Uniform },
        // point_data
        { 1, wgpu::ShaderStage::Vertex, wgpu::BufferBindingType::ReadOnlyStorage },
        // cell_data
        { 2, wgpu::ShaderStage::Vertex, wgpu::BufferBindingType::ReadOnlyStorage }
        // clang-format on
      });
  this->MeshAttributeBindGroupLayout.SetLabel("MeshAttributeBindGroupLayout");
  this->PrimitiveBindGroupLayout = vtkWebGPUInternalsBindGroupLayout::MakeBindGroupLayout(device,
    {
      // clang-format off
        // Primitive size
        { 0, wgpu::ShaderStage::Vertex, wgpu::BufferBindingType::Uniform },
        // topology
        { 1, wgpu::ShaderStage::Vertex, wgpu::BufferBindingType::ReadOnlyStorage },
      // clang-format on
    });

  this->PrimitiveBindGroupLayout.SetLabel("PrimitiveBindGroupLayout");

  // create pipeline layout.
  ///@{ TODO: Can the mappers simply keep track of bindgroup layouts and let renderer control
  /// per-mapper pipeline creation?
  auto wgpuRenderer = reinterpret_cast<vtkWebGPURenderer*>(renderer);
  assert(wgpuRenderer != nullptr);
  auto wgpuActor = reinterpret_cast<vtkWebGPUActor*>(actor);
  assert(wgpuActor != nullptr);
  std::vector<wgpu::BindGroupLayout> bgls;
  wgpuRenderer->PopulateBindgroupLayouts(bgls);
  bgls.emplace_back(this->MeshAttributeBindGroupLayout);
  bgls.emplace_back(this->PrimitiveBindGroupLayout);
  this->PipelineLayout = vtkWebGPUInternalsPipelineLayout::MakePipelineLayout(device, bgls);
  ///@}
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::SetupBindGroups(const wgpu::Device& device, vtkRenderer* renderer)
{
  if (!this->MeshAttributeBindGroup.Get())
  {
    this->MeshAttributeBindGroup =
      vtkWebGPUInternalsBindGroup::MakeBindGroup(device, this->MeshAttributeBindGroupLayout,
        {
          // clang-format off
          { 0, this->AttributeDescriptorBuffer, 0},
          { 1, this->MeshSSBO.Point.Buffer, 0},
          { 2, this->MeshSSBO.Cell.Buffer, 0}
          // clang-format on
        });
    this->MeshAttributeBindGroup.SetLabel("MeshAttributeBindGroup");
  }

  if (!this->UpdatedPrimitiveSizes)
  {
    vtkTypeUInt32 primitiveSizes[3] = { 1, 2, 3 };
    this->PointPrimitiveBGInfo.PrimitiveSizeBuffer =
      vtkWebGPUInternalsBuffer::Upload(device, 0, &primitiveSizes[0], sizeof(vtkTypeUInt32),
        wgpu::BufferUsage::Uniform, "Primitive size for VTK_POINT");
    this->LinePrimitiveBGInfo.PrimitiveSizeBuffer =
      vtkWebGPUInternalsBuffer::Upload(device, 0, &primitiveSizes[1], sizeof(vtkTypeUInt32),
        wgpu::BufferUsage::Uniform, "Primitive size for VTK_LINE");
    this->TrianglePrimitiveBGInfo.PrimitiveSizeBuffer =
      vtkWebGPUInternalsBuffer::Upload(device, 0, &primitiveSizes[2], sizeof(vtkTypeUInt32),
        wgpu::BufferUsage::Uniform, "Primitive size for VTK_TRIANGLE");
    this->UpdatedPrimitiveSizes = true;
  }

  if (!this->PointPrimitiveBGInfo.BindGroup.Get() && this->PointPrimitiveBGInfo.VertexCount > 0)
  {
    this->PointPrimitiveBGInfo.BindGroup =
      vtkWebGPUInternalsBindGroup::MakeBindGroup(device, this->PrimitiveBindGroupLayout,
        {
          // clang-format off
          { 0, this->PointPrimitiveBGInfo.PrimitiveSizeBuffer, 0},
          { 1, this->PointPrimitiveBGInfo.Buffer, 0},
          // clang-format on
        });
    this->PointPrimitiveBGInfo.BindGroup.SetLabel("PointPrimitiveBGInfo.BindGroup");
  }
  if (!this->LinePrimitiveBGInfo.BindGroup.Get() && this->LinePrimitiveBGInfo.VertexCount > 0)
  {
    this->LinePrimitiveBGInfo.BindGroup =
      vtkWebGPUInternalsBindGroup::MakeBindGroup(device, this->PrimitiveBindGroupLayout,
        {
          // clang-format off
          { 0, this->LinePrimitiveBGInfo.PrimitiveSizeBuffer, 0},
          { 1, this->LinePrimitiveBGInfo.Buffer, 0},
          // clang-format on
        });
    this->LinePrimitiveBGInfo.BindGroup.SetLabel("LinePrimitiveBGInfo.BindGroup");
  }
  if (!this->TrianglePrimitiveBGInfo.BindGroup.Get() &&
    this->TrianglePrimitiveBGInfo.VertexCount > 0)
  {
    this->TrianglePrimitiveBGInfo.BindGroup =
      vtkWebGPUInternalsBindGroup::MakeBindGroup(device, this->PrimitiveBindGroupLayout,
        {
          // clang-format off
          { 0, this->TrianglePrimitiveBGInfo.PrimitiveSizeBuffer, 0},
          { 1, this->TrianglePrimitiveBGInfo.Buffer, 0},
          // clang-format on
        });
    this->TrianglePrimitiveBGInfo.BindGroup.SetLabel("TrianglePrimitiveBGInfo.BindGroup");
  }
}

//------------------------------------------------------------------------------
unsigned long vtkWebGPUPolyDataMapper::GetExactPointBufferSize()
{
  unsigned long result = 0;
  // positions
  result += this->CurrentInput->GetNumberOfPoints() * 3 * sizeof(vtkTypeFloat32);
  // point colors
  result += this->HasPointColors ? this->Colors->GetDataSize() * sizeof(vtkTypeFloat32) : 0;
  // point normals
  result += this->HasPointNormals
    ? this->CurrentInput->GetPointData()->GetNormals()->GetNumberOfValues() * sizeof(vtkTypeFloat32)
    : 0;
  // point tangents
  result += this->HasPointTangents
    ? this->CurrentInput->GetPointData()->GetTangents()->GetNumberOfValues() *
      sizeof(vtkTypeFloat32)
    : 0;
  // uvs
  result += this->HasPointUVs
    ? this->CurrentInput->GetPointData()->GetTCoords()->GetNumberOfValues() * sizeof(vtkTypeFloat32)
    : 0;
  result = vtkWGPUContext::Align(result, 32);
  vtkDebugMacro(<< __func__ << "=" << result);
  return result;
}

//------------------------------------------------------------------------------
unsigned long vtkWebGPUPolyDataMapper::GetExactCellBufferSize()
{
  unsigned long result = 0;
  this->EdgeArrayCount = 0;

  // cell colors
  result += this->HasCellColors ? this->Colors->GetDataSize() * sizeof(vtkTypeFloat32) : 0;
  // cell normals
  result += this->HasCellNormals
    ? this->CurrentInput->GetCellData()->GetNormals()->GetDataSize() * sizeof(vtkTypeFloat32)
    : 0;
  // edge array
  auto polysIter = vtk::TakeSmartPointer(this->CurrentInput->GetPolys()->NewIterator());
  for (polysIter->GoToFirstCell(); !polysIter->IsDoneWithTraversal(); polysIter->GoToNextCell())
  {
    const vtkIdType* pts = nullptr;
    vtkIdType npts = 0;
    polysIter->GetCurrentCell(npts, pts);
    result += (npts - 2) * sizeof(vtkTypeFloat32);
    this->EdgeArrayCount += (npts - 2);
  }
  if (this->CurrentInput->GetPolys()->GetNumberOfCells() == 0)
  {
    result += sizeof(vtkTypeFloat32);
  }
  result = vtkWGPUContext::Align(result, 32);
  vtkDebugMacro(<< __func__ << "=" << result);
  return result;
}

//------------------------------------------------------------------------------
std::vector<unsigned long> vtkWebGPUPolyDataMapper::GetExactConnecitivityBufferSizes()
{
  unsigned long result = 0;
  std::vector<unsigned long> results;
  this->PointPrimitiveBGInfo.VertexCount = 0;
  this->LinePrimitiveBGInfo.VertexCount = 0;
  this->TrianglePrimitiveBGInfo.VertexCount = 0;

  const vtkIdType* pts = nullptr;
  vtkIdType npts = 0;
  {
    result = 0;
    auto vertsIter = vtk::TakeSmartPointer(this->CurrentInput->GetVerts()->NewIterator());
    for (vertsIter->GoToFirstCell(); !vertsIter->IsDoneWithTraversal(); vertsIter->GoToNextCell())
    {
      vertsIter->GetCurrentCell(npts, pts);
      this->PointPrimitiveBGInfo.VertexCount += npts;
      // the first '2' is to count these twice. once for cell_ids and once more for point_ids
      result += (2 * npts * sizeof(vtkTypeUInt32));
    }
    results.emplace_back(result);
  }

  {
    result = 0;
    auto linesIter = vtk::TakeSmartPointer(this->CurrentInput->GetLines()->NewIterator());
    for (linesIter->GoToFirstCell(); !linesIter->IsDoneWithTraversal(); linesIter->GoToNextCell())
    {
      linesIter->GetCurrentCell(npts, pts);
      const int numSubLines = npts - 1;
      this->LinePrimitiveBGInfo.VertexCount += numSubLines * 2;
      // the first '2' is to count these twice. once for cell_ids and once more for point_ids
      result += (2 * numSubLines * 2 * sizeof(vtkTypeUInt32));
    }
    results.emplace_back(result);
  }

  {
    result = 0;
    auto polysIter = vtk::TakeSmartPointer(this->CurrentInput->GetPolys()->NewIterator());
    for (polysIter->GoToFirstCell(); !polysIter->IsDoneWithTraversal(); polysIter->GoToNextCell())
    {
      polysIter->GetCurrentCell(npts, pts);
      const int numSubTriangles = npts - 2;
      this->TrianglePrimitiveBGInfo.VertexCount += numSubTriangles * 3;
      // the first '2' is to count these twice. once for cell_ids and once more for point_ids
      result += (2 * numSubTriangles * 3 * sizeof(vtkTypeUInt32));
    }
    auto stripsIter = vtk::TakeSmartPointer(this->CurrentInput->GetStrips()->NewIterator());
    for (stripsIter->GoToFirstCell(); !stripsIter->IsDoneWithTraversal();
         stripsIter->GoToNextCell())
    {
      stripsIter->GetCurrentCell(npts, pts);
      const int numSubTriangles = npts - 1;
      this->TrianglePrimitiveBGInfo.VertexCount += numSubTriangles * 3;
      // the first '2' is to count these twice. once for cell_ids and once more for point_ids
      result += (2 * numSubTriangles * 3 * sizeof(vtkTypeUInt32));
    }
    results.emplace_back(result);
  }

  for (auto& result : results)
  {
    vtkDebugMacro(<< __func__ << "=" << result);
  }
  vtkDebugMacro(<< __func__ << "=" << this->PointPrimitiveBGInfo.VertexCount);
  vtkDebugMacro(<< __func__ << "=" << this->LinePrimitiveBGInfo.VertexCount);
  vtkDebugMacro(<< __func__ << "=" << this->TrianglePrimitiveBGInfo.VertexCount);
  return results;
}

namespace
{
template <typename DestT>
struct WriteTypedArray
{
  std::size_t Offset = 0;
  void* Dst = nullptr;
  float Denominator = 1.0;

  template <typename SrcArrayT>
  void operator()(SrcArrayT* array)
  {
    if (array == nullptr || this->Dst == nullptr)
    {
      return;
    }
    DestT* dst = reinterpret_cast<DestT*>(this->Dst);
    const auto values = vtk::DataArrayValueRange(array);
    for (const auto& value : values)
    {
      *dst++ = value / this->Denominator;
      this->Offset += sizeof(DestT);
      this->Dst = dst;
    }
  }
};
}

//------------------------------------------------------------------------------
bool vtkWebGPUPolyDataMapper::UpdateMeshGeometryBuffers(const wgpu::Device& device, vtkActor* actor)
{
  bool bufferModified = false;
  if (this->CachedInput == nullptr)
  {
    vtkDebugMacro(<< "No cached input.");
    this->InvokeEvent(vtkCommand::StartEvent, nullptr);
    if (!this->Static)
    {
      this->GetInputAlgorithm()->Update();
    }
    this->InvokeEvent(vtkCommand::EndEvent, nullptr);
  }
  else
  {
    this->CurrentInput = this->CachedInput;
  }
  if (this->CurrentInput == nullptr)
  {
    vtkErrorMacro(<< "No input!");
    return false;
  }

  // if there are no points then we are done
  if (!this->CurrentInput->GetPoints())
  {
    return false;
  }

  bool updateGeometry =
    this->CurrentInput->GetPoints()->GetMTime() > this->PointCellAttributesBuildTimestamp ||
    this->CurrentInput->GetPointData()->GetMTime() > this->PointCellAttributesBuildTimestamp ||
    this->CurrentInput->GetCellData()->GetMTime() > this->PointCellAttributesBuildTimestamp ||
    this->LastScalarVisibility != this->ScalarVisibility ||
    this->LastScalarMode != this->ScalarMode || this->LastColors != this->Colors;

  if (!updateGeometry)
  {
    return false;
  }

  this->HasCellNormals = this->CurrentInput->GetCellData()->GetNormals() != nullptr;
  this->HasPointNormals = this->CurrentInput->GetPointData()->GetNormals() != nullptr;
  this->HasPointTangents = this->CurrentInput->GetPointData()->GetTangents();
  this->HasPointUVs = this->CurrentInput->GetPointData()->GetTCoords();
  // For vertex coloring, this sets this->Colors as side effect.
  // For texture map coloring, this sets ColorCoordinates
  // and ColorTextureMap as a side effect.
  this->MapScalars(1.0);
  this->HasPointColors = false;
  this->HasCellColors = false;
  auto shadeType = vtkWebGPUActor::ShadingTypeEnum::Global;
  if (this->Colors != nullptr && this->Colors->GetNumberOfValues() > 0)
  {
    // we've point scalars mapped to colors.
    shadeType = vtkWebGPUActor::ShadingTypeEnum::Smooth;
    this->HasPointColors = true;
  }
  // check for cell scalars
  if (this->ScalarVisibility)
  {
    // We must figure out how the scalars should be mapped to the polydata.
    if ((this->ScalarMode == VTK_SCALAR_MODE_USE_CELL_DATA ||
          this->ScalarMode == VTK_SCALAR_MODE_USE_CELL_FIELD_DATA ||
          this->ScalarMode == VTK_SCALAR_MODE_USE_FIELD_DATA ||
          !this->CurrentInput->GetPointData()->GetScalars()) &&
      this->ScalarMode != VTK_SCALAR_MODE_USE_POINT_FIELD_DATA && this->Colors &&
      this->Colors->GetNumberOfTuples() > 0)
    {
      shadeType = vtkWebGPUActor::ShadingTypeEnum::Flat;
      this->HasCellColors = true;
    }
  }
  auto wgpuActor = reinterpret_cast<vtkWebGPUActor*>(actor);
  wgpuActor->SetShadingType(shadeType);
  this->LastColors = this->Colors;
  this->LastScalarMode = this->ScalarMode;
  this->LastScalarVisibility = this->ScalarVisibility;
  ///@{ TODO:
  // // If we are coloring by texture, then load the texture map.
  // if (this->ColorTextureMap)
  // {
  //   if (this->InternalColorTexture == nullptr)
  //   {
  //     this->InternalColorTexture = vtkOpenGLTexture::New();
  //     this->InternalColorTexture->RepeatOff();
  //   }
  //   this->InternalColorTexture->SetInputData(this->ColorTextureMap);
  // }
  ///@}

  MeshAttributeDescriptor meshAttrDescriptor;

  vtkPointData* pointData = this->CurrentInput->GetPointData();
  vtkDataArray* points = this->CurrentInput->GetPoints()->GetData();
  vtkDataArray* colors = this->HasPointColors ? vtkDataArray::SafeDownCast(this->Colors) : nullptr;
  vtkDataArray* normals = pointData->GetNormals();
  vtkDataArray* tangents = pointData->GetTangents();
  vtkDataArray* uvs = pointData->GetTCoords();

  if (this->MeshSSBO.Point.Buffer.Get() != nullptr)
  {
    this->MeshSSBO.Point.Buffer.Destroy();
  }
  if (this->MeshSSBO.Cell.Buffer.Get() != nullptr)
  {
    this->MeshSSBO.Cell.Buffer.Destroy();
  }

  wgpu::BufferDescriptor pointBufDescriptor;
  pointBufDescriptor.size = this->GetExactPointBufferSize();
  pointBufDescriptor.label = "Upload point buffer";
  pointBufDescriptor.mappedAtCreation = true;
  pointBufDescriptor.usage = wgpu::BufferUsage::Storage;
  this->MeshSSBO.Point.Buffer = device.CreateBuffer(&pointBufDescriptor);

  ::WriteTypedArray<vtkTypeFloat32> f32Writer;

  f32Writer.Denominator = 1.0;
  f32Writer.Offset = 0;
  using DispatchT = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::AllTypes>;

  meshAttrDescriptor.Positions.Start = 0;
  void* mapped =
    this->MeshSSBO.Point.Buffer.GetMappedRange(f32Writer.Offset, pointBufDescriptor.size);
  assert(mapped != nullptr);
  f32Writer.Dst = mapped;
  if (!DispatchT::Execute(points, f32Writer))
  {
    f32Writer(points);
  }
  meshAttrDescriptor.Positions.NumComponents = points->GetNumberOfComponents();
  meshAttrDescriptor.Positions.NumTuples = points->GetNumberOfTuples();
  vtkDebugMacro(<< "[Positions] "
                << "+ " << f32Writer.Offset << " bytes ");

  if (this->HasPointColors)
  {
    f32Writer.Denominator = 255.0f;
    meshAttrDescriptor.Colors.Start = f32Writer.Offset / sizeof(vtkTypeFloat32);
    if (!DispatchT::Execute(colors, f32Writer))
    {
      f32Writer(colors);
    }
    f32Writer.Denominator = 1.0f;
    meshAttrDescriptor.Colors.NumComponents = colors->GetNumberOfComponents();
    meshAttrDescriptor.Colors.NumTuples = colors->GetNumberOfTuples();
    vtkDebugMacro(<< "[Colors] "
                  << "+ " << f32Writer.Offset << " bytes ");
  }
  if (this->HasPointNormals)
  {
    meshAttrDescriptor.Normals.Start = f32Writer.Offset / sizeof(vtkTypeFloat32);
    if (!DispatchT::Execute(normals, f32Writer))
    {
      f32Writer(normals);
    }
    meshAttrDescriptor.Normals.NumComponents = normals->GetNumberOfComponents();
    meshAttrDescriptor.Normals.NumTuples = normals->GetNumberOfTuples();
    vtkDebugMacro(<< "[Normals] "
                  << "+ " << f32Writer.Offset << " bytes ");
  }
  if (this->HasPointTangents)
  {
    meshAttrDescriptor.Tangents.Start = f32Writer.Offset / sizeof(vtkTypeFloat32);
    if (!DispatchT::Execute(tangents, f32Writer))
    {
      f32Writer(tangents);
    }
    meshAttrDescriptor.Tangents.NumComponents = tangents->GetNumberOfComponents();
    meshAttrDescriptor.Tangents.NumTuples = tangents->GetNumberOfTuples();
    vtkDebugMacro(<< "[Tangents] "
                  << "+ " << f32Writer.Offset << " bytes ");
  }
  if (this->HasPointUVs)
  {
    meshAttrDescriptor.UVs.Start = f32Writer.Offset / sizeof(vtkTypeFloat32);
    if (!DispatchT::Execute(uvs, f32Writer))
    {
      f32Writer(uvs);
    }
    meshAttrDescriptor.UVs.NumComponents = uvs->GetNumberOfComponents();
    meshAttrDescriptor.UVs.NumTuples = uvs->GetNumberOfTuples();
    vtkDebugMacro(<< "[UVs] "
                  << "+ " << f32Writer.Offset << " bytes ");
  }
  mapped = nullptr;
  f32Writer.Dst = nullptr;
  this->MeshSSBO.Point.Buffer.Unmap();

  wgpu::BufferDescriptor cellBufDescriptor;
  cellBufDescriptor.size = this->GetExactCellBufferSize();
  cellBufDescriptor.label = "Upload cell buffer";
  cellBufDescriptor.mappedAtCreation = true;
  cellBufDescriptor.usage = wgpu::BufferUsage::Storage;
  this->MeshSSBO.Cell.Buffer = device.CreateBuffer(&cellBufDescriptor);

  f32Writer.Denominator = 1.0;
  f32Writer.Offset = 0;

  vtkCellData* cellData = this->CurrentInput->GetCellData();
  vtkDataArray* cellColors =
    this->HasCellColors ? vtkDataArray::SafeDownCast(this->Colors) : nullptr;
  vtkDataArray* cellNormals = this->HasCellNormals ? cellData->GetNormals() : nullptr;

  meshAttrDescriptor.CellEdgeArray.Start = f32Writer.Offset / sizeof(vtkTypeFloat32);
  mapped = this->MeshSSBO.Cell.Buffer.GetMappedRange(f32Writer.Offset, cellBufDescriptor.size);
  assert(mapped != nullptr);
  // edge array
  auto polysIter = vtk::TakeSmartPointer(this->CurrentInput->GetPolys()->NewIterator());
  auto* dst = reinterpret_cast<vtkTypeFloat32*>(mapped);
  for (polysIter->GoToFirstCell(); !polysIter->IsDoneWithTraversal(); polysIter->GoToNextCell())
  {
    const vtkIdType* pts = nullptr;
    vtkIdType npts = 0;
    polysIter->GetCurrentCell(npts, pts);
    for (vtkIdType i = 1; i < npts - 1; ++i)
    {
      vtkTypeFloat32 val = npts == 3 ? -1 : i == 1 ? 2 : i == npts - 2 ? 0 : 1;
      *dst++ = val;
      f32Writer.Offset += sizeof(vtkTypeFloat32);
      f32Writer.Dst = dst;
    }
  }
  meshAttrDescriptor.CellEdgeArray.NumComponents = 1;
  meshAttrDescriptor.CellEdgeArray.NumTuples = this->EdgeArrayCount;
  vtkDebugMacro(<< "[Cell edge array] "
                << "+ " << f32Writer.Offset << " bytes ");

  if (this->HasCellColors)
  {
    meshAttrDescriptor.CellColors.Start = f32Writer.Offset / sizeof(vtkTypeFloat32);
    f32Writer.Denominator = 255.0f;
    if (!DispatchT::Execute(cellColors, f32Writer))
    {
      f32Writer(cellColors);
    }
    f32Writer.Denominator = 1.0f;
    meshAttrDescriptor.CellColors.NumComponents = cellColors->GetNumberOfComponents();
    meshAttrDescriptor.CellColors.NumTuples = cellColors->GetNumberOfTuples();
    vtkDebugMacro(<< "[Cell colors] "
                  << "+ " << f32Writer.Offset << " bytes ");
  }
  if (this->HasCellNormals)
  {
    meshAttrDescriptor.CellNormals.Start = f32Writer.Offset / sizeof(vtkTypeFloat32);
    if (!DispatchT::Execute(cellNormals, f32Writer))
    {
      f32Writer(cellNormals);
    }
    meshAttrDescriptor.CellNormals.NumComponents = cellNormals->GetNumberOfComponents();
    meshAttrDescriptor.CellNormals.NumTuples = cellNormals->GetNumberOfTuples();
    vtkDebugMacro(<< "[Cell normals] "
                  << "+ " << f32Writer.Offset << " bytes ");
  }
  this->MeshSSBO.Cell.Buffer.Unmap();

  this->AttributeDescriptorBuffer = vtkWebGPUInternalsBuffer::Upload(device, 0, &meshAttrDescriptor,
    sizeof(meshAttrDescriptor), wgpu::BufferUsage::Uniform, "Mesh attribute descriptor");

  using DMEnum = vtkWebGPUActor::DirectionalMaskEnum;
  vtkTypeUInt32 dirMask = DMEnum::NoNormals;
  dirMask = this->HasPointNormals ? DMEnum::PointNormals : 0;
  dirMask |= this->HasPointTangents ? DMEnum::PointTangents : dirMask;
  dirMask |= this->HasCellNormals ? DMEnum::CellNormals : dirMask;
  if (dirMask == 0)
  {
    dirMask = DMEnum::NoNormals;
  }
  wgpuActor->SetDirectionalMaskType(dirMask);

  this->PointCellAttributesBuildTimestamp.Modified();
  vtkDebugMacro(<< __func__ << " bufferModifiedTime=" << this->PointCellAttributesBuildTimestamp);
  return true;
}

//------------------------------------------------------------------------------
bool vtkWebGPUPolyDataMapper::UpdateMeshIndexBuffers(const wgpu::Device& device)
{
  bool updateIndices = this->CurrentInput->GetMeshMTime() > this->Primitive2CellIDsBuildTimestamp;
  if (!updateIndices)
  {
    return false;
  }

  vtkTypeUInt32 cellCount = 0;

  const auto sizes = this->GetExactConnecitivityBufferSizes();

  const vtkIdType* pts = nullptr;
  vtkIdType npts = 0;
  {
    vtkCellArray* verts = this->CurrentInput->GetVerts();
    if (verts->GetNumberOfCells() > 0)
    {
      if (this->PointPrimitiveBGInfo.Buffer.Get() != nullptr)
      {
        this->PointPrimitiveBGInfo.Buffer.Destroy();
      }
      // point primitives.
      wgpu::BufferDescriptor topoBufDescriptor;
      topoBufDescriptor.size = sizes[0];
      topoBufDescriptor.label = "Upload vtkPolyData::Verts";
      topoBufDescriptor.mappedAtCreation = true;
      topoBufDescriptor.usage = wgpu::BufferUsage::Storage;

      this->PointPrimitiveBGInfo.Buffer = device.CreateBuffer(&topoBufDescriptor);
      void* mapped = this->PointPrimitiveBGInfo.Buffer.GetMappedRange(0, sizes[0]);
      auto* mappedAsU32 = reinterpret_cast<vtkTypeUInt32*>(mapped);
      vtkSmartPointer<vtkCellArrayIterator> vertsIter = vtk::TakeSmartPointer(verts->NewIterator());
      for (vertsIter->GoToFirstCell(); !vertsIter->IsDoneWithTraversal();
           vertsIter->GoToNextCell(), ++cellCount)
      {
        vertsIter->GetCurrentCell(npts, pts);
        for (int i = 0; i < npts; ++i)
        {
          *mappedAsU32++ = cellCount;
          *mappedAsU32++ = pts[i];
        }
      }
      this->PointPrimitiveBGInfo.Buffer.Unmap();
    }
    vtkDebugMacro(<< "[Verts] "
                  << "+ " << sizes[0] << " bytes ");
  }
  {
    vtkCellArray* lines = this->CurrentInput->GetLines();
    if (lines->GetNumberOfCells() > 0)
    {
      if (this->LinePrimitiveBGInfo.Buffer.Get() != nullptr)
      {
        this->LinePrimitiveBGInfo.Buffer.Destroy();
      }
      // line primitives.
      wgpu::BufferDescriptor topoBufDescriptor;
      topoBufDescriptor.size = sizes[1];
      topoBufDescriptor.label = "Upload vtkPolyData::Lines";
      topoBufDescriptor.mappedAtCreation = true;
      topoBufDescriptor.usage = wgpu::BufferUsage::Storage;

      this->LinePrimitiveBGInfo.Buffer = device.CreateBuffer(&topoBufDescriptor);
      void* mapped = this->LinePrimitiveBGInfo.Buffer.GetMappedRange(0, sizes[1]);
      auto* mappedAsU32 = reinterpret_cast<vtkTypeUInt32*>(mapped);
      vtkSmartPointer<vtkCellArrayIterator> linesIter = vtk::TakeSmartPointer(lines->NewIterator());
      for (linesIter->GoToFirstCell(); !linesIter->IsDoneWithTraversal();
           linesIter->GoToNextCell(), ++cellCount)
      {
        linesIter->GetCurrentCell(npts, pts);
        const int numSubLines = npts - 1;
        for (int i = 0; i < numSubLines; ++i)
        {
          *mappedAsU32++ = cellCount;
          *mappedAsU32++ = pts[i];
          *mappedAsU32++ = cellCount;
          *mappedAsU32++ = pts[i + 1];
        }
      }
      this->LinePrimitiveBGInfo.Buffer.Unmap();
    }
    vtkDebugMacro(<< "[Lines] "
                  << "+ " << sizes[1] << " bytes ");
  }
  {
    vtkCellArray* polys = this->CurrentInput->GetPolys();
    vtkCellArray* strips = this->CurrentInput->GetStrips();
    if (polys->GetNumberOfCells() + strips->GetNumberOfCells() > 0)
    {
      if (this->TrianglePrimitiveBGInfo.Buffer.Get() != nullptr)
      {
        this->TrianglePrimitiveBGInfo.Buffer.Destroy();
      }
      // triangle primitives.
      wgpu::BufferDescriptor topoBufDescriptor;
      topoBufDescriptor.size = sizes[2];
      topoBufDescriptor.label = "Upload vtkPolyData::{Tris,Strips}";
      topoBufDescriptor.mappedAtCreation = true;
      topoBufDescriptor.usage = wgpu::BufferUsage::Storage;

      this->TrianglePrimitiveBGInfo.Buffer = device.CreateBuffer(&topoBufDescriptor);
      void* mapped = this->TrianglePrimitiveBGInfo.Buffer.GetMappedRange(0, sizes[2]);
      auto* mappedAsU32 = reinterpret_cast<vtkTypeUInt32*>(mapped);
      vtkSmartPointer<vtkCellArrayIterator> polysIter = vtk::TakeSmartPointer(polys->NewIterator());
      for (polysIter->GoToFirstCell(); !polysIter->IsDoneWithTraversal();
           polysIter->GoToNextCell(), ++cellCount)
      {
        polysIter->GetCurrentCell(npts, pts);
        const int numSubTriangles = npts - 2;
        for (int i = 0; i < numSubTriangles; ++i)
        {
          *mappedAsU32++ = cellCount;
          *mappedAsU32++ = pts[0];
          *mappedAsU32++ = cellCount;
          *mappedAsU32++ = pts[i + 1];
          *mappedAsU32++ = cellCount;
          *mappedAsU32++ = pts[i + 2];
        }
      }
      vtkSmartPointer<vtkCellArrayIterator> stripsIter =
        vtk::TakeSmartPointer(strips->NewIterator());
      for (stripsIter->GoToFirstCell(); !stripsIter->IsDoneWithTraversal();
           stripsIter->GoToNextCell(), ++cellCount)
      {
        stripsIter->GetCurrentCell(npts, pts);
        *mappedAsU32++ = cellCount;
        *mappedAsU32++ = pts[0];
        *mappedAsU32++ = cellCount;
        *mappedAsU32++ = pts[1];
        *mappedAsU32++ = cellCount;
        *mappedAsU32++ = pts[2];
        for (int i = 2; i < npts; ++i)
        {
          *mappedAsU32++ = cellCount;
          *mappedAsU32++ = pts[i - 2];
          *mappedAsU32++ = cellCount;
          *mappedAsU32++ = pts[i - 1];
          *mappedAsU32++ = cellCount;
          *mappedAsU32++ = pts[i];
        }
      }
      this->TrianglePrimitiveBGInfo.Buffer.Unmap();
    }
  }
  vtkDebugMacro(<< "[Triangles] "
                << "+ " << sizes[2] << " bytes ");
  this->Primitive2CellIDsBuildTimestamp.Modified();
  vtkDebugMacro(<< __func__ << " bufferModifiedTime=" << this->Primitive2CellIDsBuildTimestamp);
  return true;
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::SetupGraphicsPipeline(
  const wgpu::Device& device, vtkRenderer* renderer, vtkActor* actor)
{
  // build shaders if needed.
  wgpu::ShaderModule shaderModule =
    vtkWebGPUInternalsShaderModule::CreateFromWGSL(device, PolyData);
  auto wgpuRenWin = vtkWebGPURenderWindow::SafeDownCast(renderer->GetRenderWindow());
  auto wgpuRenderer = reinterpret_cast<vtkWebGPURenderer*>(renderer);

  vtkWebGPUInternalsRenderPipelineDescriptor descriptor;
  descriptor.layout = this->PipelineLayout;
  descriptor.vertex.module = shaderModule;
  descriptor.vertex.entryPoint = "vertexMain";
  descriptor.vertex.bufferCount = 0;
  descriptor.cFragment.module = shaderModule;
  descriptor.cFragment.entryPoint = "fragmentMain";
  descriptor.cTargets[0].format = wgpuRenWin->GetPreferredSwapChainTextureFormat();
  ///@{ TODO: Only for valid depth stencil formats
  auto depthState = descriptor.EnableDepthStencil(wgpuRenWin->GetDepthStencilFormat());
  depthState->depthWriteEnabled = true;
  depthState->depthCompare = wgpu::CompareFunction::Less;

  const int representation = actor->GetProperty()->GetRepresentation();
  const std::string reprAsStr = actor->GetProperty()->GetRepresentationAsString();
  ///@}
  // create pipeline for VTK_POINT primitive
  {
    std::string info = "primitive=VTK_POINT;representation=" + reprAsStr;
    descriptor.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
    this->PointPrimitiveBGInfo.PipelineID =
      wgpuRenderer->InsertRenderPipeline(this, actor, descriptor, info);
  }
  // create pipeline for VTK_LINE primitive
  {
    std::string info = "primitive=VTK_LINE;representation=" + reprAsStr;
    descriptor.primitive.topology = representation == VTK_POINTS
      ? wgpu::PrimitiveTopology::TriangleList
      : wgpu::PrimitiveTopology::LineList;
    this->LinePrimitiveBGInfo.PipelineID =
      wgpuRenderer->InsertRenderPipeline(this, actor, descriptor, info);
  }
  // create pipeline for VTK_TRIANGLE primitive
  {
    std::string info = "primitive=VTK_TRIANGLE;representation=" + reprAsStr;
    descriptor.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
    this->TrianglePrimitiveBGInfo.PipelineID =
      wgpuRenderer->InsertRenderPipeline(this, actor, descriptor, info);
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::ReleaseGraphicsResources(vtkWindow*) {}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::ShallowCopy(vtkAbstractMapper* m) {}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::MapDataArrayToVertexAttribute(const char* vertexAttributeName,
  const char* dataArrayName, int fieldAssociation, int componentno /*=-1*/)
{
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::MapDataArrayToMultiTextureAttribute(
  const char* tname, const char* dataArrayName, int fieldAssociation, int componentno /*=-1*/)
{
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::RemoveVertexAttributeMapping(const char* vertexAttributeName) {}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::RemoveAllVertexAttributeMappings() {}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::ProcessSelectorPixelBuffers(
  vtkHardwareSelector* sel, std::vector<unsigned int>& pixeloffsets, vtkProp* prop)
{
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::ComputeBounds()
{
  this->CachedInput = this->GetInput();
  this->InvokeEvent(vtkCommand::StartEvent, nullptr);
  if (!this->Static)
  {
    this->GetInputAlgorithm()->Update();
  }
  this->InvokeEvent(vtkCommand::EndEvent, nullptr);
  if (!this->CachedInput)
  {
    vtkMath::UninitializeBounds(this->Bounds);
    return;
  }
  this->CachedInput->GetCellsBounds(this->Bounds);
}

VTK_ABI_NAMESPACE_END
