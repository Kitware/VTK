// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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
#include "vtkTypeFloat32Array.h"
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
void vtkWebGPUPolyDataMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

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
      // update (i.e, create and write) GPU buffers if the data is outdated.
      bool buffersRecreated = false;
      buffersRecreated |= this->UpdateMeshGeometryBuffers(device, actor);
      buffersRecreated |= this->UpdateMeshIndexBuffers(device);
      // setup pipeline
      if (!this->InitializedPipeline)
      {
        this->SetupPipelineLayout(device, renderer, actor);
        this->SetupGraphicsPipeline(device, renderer, actor);
        this->InitializedPipeline = true;
      }
      // bind groups must be recreated if any of the buffers at the bindpoints were recreated.
      if (buffersRecreated)
      {
        this->SetupBindGroups(device, renderer);
      }
      // any previously built command buffer is no longer valid, since bind groups were re-created
      wgpuActor->SetMapperRenderPipelineOutdated(buffersRecreated);
      break;
    }
    case RenderType::RenderPassEncode:
      this->EncodeRenderCommands(renderer, actor, wgpuRenderer->GetRenderPassEncoder());
      break;
    case RenderType::RenderBundleEncode:
      this->EncodeRenderCommands(renderer, actor, wgpuActor->GetRenderBundleEncoder());
      break;
    case RenderType::None:
    default:
      break;
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::EncodeRenderCommands(
  vtkRenderer*, vtkActor* actor, const wgpu::RenderPassEncoder& passEncoder)
{
  passEncoder.SetBindGroup(2, this->MeshAttributeBindGroup);

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
  if (this->PointPrimitiveBGInfo.Pipeline.Get() != nullptr &&
    this->PointPrimitiveBGInfo.VertexCount > 0)
  {
#ifndef NDEBUG
    passEncoder.PushDebugGroup("VTK_POINT");
#endif
    passEncoder.SetPipeline(this->PointPrimitiveBGInfo.Pipeline);
    passEncoder.SetBindGroup(3, this->PointPrimitiveBGInfo.BindGroup);
    passEncoder.Draw(this->PointPrimitiveBGInfo.VertexCount * vcFactor[0], instanceCount[0]);
#ifndef NDEBUG
    passEncoder.PopDebugGroup();
#endif
  }
  else if (this->LinePrimitiveBGInfo.Pipeline.Get() != nullptr &&
    this->LinePrimitiveBGInfo.VertexCount > 0)
  {
#ifndef NDEBUG
    passEncoder.PushDebugGroup("VTK_LINE");
#endif
    passEncoder.SetPipeline(this->LinePrimitiveBGInfo.Pipeline);
    passEncoder.SetBindGroup(3, this->LinePrimitiveBGInfo.BindGroup);
    passEncoder.Draw(this->LinePrimitiveBGInfo.VertexCount * vcFactor[1], instanceCount[1]);
#ifndef NDEBUG
    passEncoder.PopDebugGroup();
#endif
  }
  else if (this->TrianglePrimitiveBGInfo.Pipeline.Get() != nullptr &&
    this->TrianglePrimitiveBGInfo.VertexCount > 0)
  {
#ifndef NDEBUG
    passEncoder.PushDebugGroup("VTK_TRIANGLE");
#endif
    passEncoder.SetPipeline(this->TrianglePrimitiveBGInfo.Pipeline);
    passEncoder.SetBindGroup(3, this->TrianglePrimitiveBGInfo.BindGroup);
    passEncoder.Draw(this->TrianglePrimitiveBGInfo.VertexCount * vcFactor[2], instanceCount[2]);
#ifndef NDEBUG
    passEncoder.PopDebugGroup();
#endif
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::EncodeRenderCommands(
  vtkRenderer*, vtkActor* actor, const wgpu::RenderBundleEncoder& bundleEncoder)
{
  bundleEncoder.SetBindGroup(2, this->MeshAttributeBindGroup);

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
  if (this->PointPrimitiveBGInfo.Pipeline.Get() != nullptr &&
    this->PointPrimitiveBGInfo.VertexCount > 0)
  {
#ifndef NDEBUG
    bundleEncoder.PushDebugGroup("VTK_POINT");
#endif
    bundleEncoder.SetPipeline(this->PointPrimitiveBGInfo.Pipeline);
    bundleEncoder.SetBindGroup(3, this->PointPrimitiveBGInfo.BindGroup);
    bundleEncoder.Draw(this->PointPrimitiveBGInfo.VertexCount * vcFactor[0], instanceCount[0]);
#ifndef NDEBUG
    bundleEncoder.PopDebugGroup();
#endif
  }
  else if (this->LinePrimitiveBGInfo.Pipeline.Get() != nullptr &&
    this->LinePrimitiveBGInfo.VertexCount > 0)
  {
#ifndef NDEBUG
    bundleEncoder.PushDebugGroup("VTK_LINE");
#endif
    bundleEncoder.SetPipeline(this->LinePrimitiveBGInfo.Pipeline);
    bundleEncoder.SetBindGroup(3, this->LinePrimitiveBGInfo.BindGroup);
    bundleEncoder.Draw(this->LinePrimitiveBGInfo.VertexCount * vcFactor[1], instanceCount[1]);
#ifndef NDEBUG
    bundleEncoder.PopDebugGroup();
#endif
  }
  else if (this->TrianglePrimitiveBGInfo.Pipeline.Get() != nullptr &&
    this->TrianglePrimitiveBGInfo.VertexCount > 0)
  {
#ifndef NDEBUG
    bundleEncoder.PushDebugGroup("VTK_TRIANGLE");
#endif
    bundleEncoder.SetPipeline(this->TrianglePrimitiveBGInfo.Pipeline);
    bundleEncoder.SetBindGroup(3, this->TrianglePrimitiveBGInfo.BindGroup);
    bundleEncoder.Draw(this->TrianglePrimitiveBGInfo.VertexCount * vcFactor[2], instanceCount[2]);
#ifndef NDEBUG
    bundleEncoder.PopDebugGroup();
#endif
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::SetupPipelineLayout(
  const wgpu::Device& device, vtkRenderer* renderer, vtkActor*)
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
  std::vector<wgpu::BindGroupLayout> bgls;
  wgpuRenderer->PopulateBindgroupLayouts(bgls);
  bgls.emplace_back(this->MeshAttributeBindGroupLayout);
  bgls.emplace_back(this->PrimitiveBindGroupLayout);
  this->PipelineLayout = vtkWebGPUInternalsPipelineLayout::MakePipelineLayout(device, bgls);
  ///@}
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::SetupBindGroups(const wgpu::Device& device, vtkRenderer*)
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
unsigned long vtkWebGPUPolyDataMapper::GetPointAttributeByteSize(
  vtkWebGPUPolyDataMapper::PointDataAttributes attribute)
{
  switch (attribute)
  {
    case PointDataAttributes::POINT_POSITIONS:
      return this->CurrentInput->GetNumberOfPoints() * 3 * sizeof(vtkTypeFloat32);

    case PointDataAttributes::POINT_COLORS:
      return this->HasPointColors ? this->Colors->GetDataSize() * sizeof(vtkTypeFloat32) : 0;

    case PointDataAttributes::POINT_NORMALS:
      if (this->HasPointNormals)
      {
        return this->CurrentInput->GetPointData()->GetNormals()->GetNumberOfValues() *
          sizeof(vtkTypeFloat32);
      }

      break;

    case PointDataAttributes::POINT_TANGENTS:
      if (this->HasPointTangents)
      {
        return this->CurrentInput->GetPointData()->GetTangents()->GetNumberOfValues() *
          sizeof(vtkTypeFloat32);
      }

      break;

    case PointDataAttributes::POINT_UVS:
      if (this->HasPointUVs)
      {
        return this->CurrentInput->GetPointData()->GetTCoords()->GetNumberOfValues() *
          sizeof(vtkTypeFloat32);
      }

      break;

    default:
      break;
  }

  return 0;
}

//------------------------------------------------------------------------------
unsigned long vtkWebGPUPolyDataMapper::GetCellAttributeByteSize(
  vtkWebGPUPolyDataMapper::CellDataAttributes attribute)
{
  switch (attribute)
  {
    case CellDataAttributes::CELL_COLORS:
      if (this->HasCellColors)
      {
        return this->Colors->GetDataSize() * sizeof(vtkTypeFloat32);
      }

      break;

    case CellDataAttributes::CELL_NORMALS:
      if (this->HasCellNormals)
      {
        return this->CurrentInput->GetCellData()->GetNormals()->GetDataSize() *
          sizeof(vtkTypeFloat32);
      }

      break;

    case CellDataAttributes::CELL_EDGES:
    {
      unsigned long size = 0;
      this->EdgeArrayCount = 0;

      auto polysIter = vtk::TakeSmartPointer(this->CurrentInput->GetPolys()->NewIterator());
      for (polysIter->GoToFirstCell(); !polysIter->IsDoneWithTraversal(); polysIter->GoToNextCell())
      {
        const vtkIdType* pts = nullptr;
        vtkIdType npts = 0;
        polysIter->GetCurrentCell(npts, pts);
        size += (npts - 2) * sizeof(vtkTypeFloat32);
        this->EdgeArrayCount += (npts - 2);
      }

      if (this->CurrentInput->GetPolys()->GetNumberOfCells() == 0)
      {
        size += sizeof(vtkTypeFloat32);
      }

      return size;
    }

    default:
      break;
  }

  return 0;
}

//------------------------------------------------------------------------------
unsigned long vtkWebGPUPolyDataMapper::GetPointAttributeElementSize(
  vtkWebGPUPolyDataMapper::PointDataAttributes attribute)
{
  switch (attribute)
  {
    case PointDataAttributes::POINT_POSITIONS:
      return 3 * sizeof(vtkTypeFloat32);

    case PointDataAttributes::POINT_COLORS:
      if (this->HasPointColors)
      {
        return vtkDataArray::SafeDownCast(this->Colors)->GetNumberOfComponents() *
          sizeof(vtkTypeFloat32);
      }

      break;

    case PointDataAttributes::POINT_NORMALS:
      if (this->HasPointNormals)
      {
        return this->CurrentInput->GetPointData()->GetNormals()->GetNumberOfComponents() *
          sizeof(vtkTypeFloat32);
      }

      break;

    case PointDataAttributes::POINT_TANGENTS:
      if (this->HasPointTangents)
      {
        return this->CurrentInput->GetPointData()->GetTangents()->GetNumberOfComponents() *
          sizeof(vtkTypeFloat32);
      }

      break;

    case PointDataAttributes::POINT_UVS:
      if (this->HasPointUVs)
      {
        return this->CurrentInput->GetPointData()->GetTCoords()->GetNumberOfComponents() *
          sizeof(vtkTypeFloat32);
      }

      break;

    default:
      break;
  }

  return 0;
}

//------------------------------------------------------------------------------
unsigned long vtkWebGPUPolyDataMapper::GetCellAttributeElementSize(
  vtkWebGPUPolyDataMapper::CellDataAttributes attribute)
{
  switch (attribute)
  {
    case CellDataAttributes::CELL_COLORS:
      if (this->HasCellColors)
      {
        return vtkDataArray::SafeDownCast(this->Colors)->GetNumberOfComponents() *
          sizeof(vtkTypeFloat32);
      }

      break;

    case CellDataAttributes::CELL_NORMALS:
      if (this->HasCellNormals)
      {
        return this->CurrentInput->GetCellData()->GetNormals()->GetNumberOfComponents() *
          sizeof(vtkTypeFloat32);
      }

      break;

    case CellDataAttributes::CELL_EDGES:
      return sizeof(float);

    default:
      break;
  }

  return 0;
}

//------------------------------------------------------------------------------
vtkIdType vtkWebGPUPolyDataMapper::GetPointAttributeByteOffset(
  vtkWebGPUPolyDataMapper::PointDataAttributes attribute)
{
  vtkIdType accumulatedOffset = 0;

  for (int attributeIndex = 0; attributeIndex <= PointDataAttributes::POINT_NB_ATTRIBUTES;
       attributeIndex++)
  {
    PointDataAttributes attributeInOrder = this->PointDataAttributesOrder[attributeIndex];
    if (attributeInOrder != attribute)
    {
      accumulatedOffset +=
        this->GetPointAttributeByteSize(static_cast<PointDataAttributes>(attributeInOrder));
    }
    else
    {
      break;
    }
  }

  return accumulatedOffset;
}

//------------------------------------------------------------------------------
vtkIdType vtkWebGPUPolyDataMapper::GetCellAttributeByteOffset(
  vtkWebGPUPolyDataMapper::CellDataAttributes attribute)
{
  vtkIdType accumulatedOffset = 0;

  for (int attributeIndex = 0; attributeIndex <= CellDataAttributes::CELL_NB_ATTRIBUTES;
       attributeIndex++)
  {
    CellDataAttributes attributeInOrder = this->CellDataAttributesOrder[attributeIndex];
    if (attributeInOrder != attribute)
    {
      accumulatedOffset +=
        this->GetCellAttributeByteSize(static_cast<CellDataAttributes>(attributeInOrder));
    }
    else
    {
      break;
    }
  }

  return accumulatedOffset;
}

//------------------------------------------------------------------------------
unsigned long vtkWebGPUPolyDataMapper::GetExactPointBufferSize()
{
  unsigned long result = 0;

  result += this->GetPointAttributeByteSize(PointDataAttributes::POINT_POSITIONS);
  result += this->GetPointAttributeByteSize(PointDataAttributes::POINT_COLORS);
  result += this->GetPointAttributeByteSize(PointDataAttributes::POINT_NORMALS);
  result += this->GetPointAttributeByteSize(PointDataAttributes::POINT_TANGENTS);
  result += this->GetPointAttributeByteSize(PointDataAttributes::POINT_UVS);

  result = vtkWGPUContext::Align(result, 32);
  vtkDebugMacro(<< __func__ << "=" << result);
  return result;
}

//------------------------------------------------------------------------------
unsigned long vtkWebGPUPolyDataMapper::GetExactCellBufferSize()
{
  unsigned long result = 0;

  result += this->GetCellAttributeByteSize(CellDataAttributes::CELL_COLORS);
  result += this->GetCellAttributeByteSize(CellDataAttributes::CELL_NORMALS);
  result += this->GetCellAttributeByteSize(CellDataAttributes::CELL_EDGES);

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

  vtkDebugMacro(<< __func__ << "[verts]=" << this->PointPrimitiveBGInfo.VertexCount);
  vtkDebugMacro(<< __func__ << "[lines]=" << this->LinePrimitiveBGInfo.VertexCount);
  vtkDebugMacro(<< __func__ << "[polys]=" << this->TrianglePrimitiveBGInfo.VertexCount);
  return results;
}

namespace
{
template <typename DestT>
struct WriteTypedArray
{
  std::size_t Offset = 0;
  const wgpu::Buffer& DstBuffer;
  const wgpu::Device& Device;
  float Denominator = 1.0;

  template <typename SrcArrayT>
  void operator()(SrcArrayT* array)
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
    this->Device.GetQueue().WriteBuffer(this->DstBuffer, this->Offset, data->GetPointer(0), nbytes);
    this->Offset += nbytes;
  }
};
}

//------------------------------------------------------------------------------
vtkTypeFloat32Array* vtkWebGPUPolyDataMapper::ComputeEdgeArray(vtkCellArray* polys)
{
  auto polysIter = vtk::TakeSmartPointer(polys->NewIterator());
  auto edgeArray = vtkTypeFloat32Array::New();
  for (polysIter->GoToFirstCell(); !polysIter->IsDoneWithTraversal(); polysIter->GoToNextCell())
  {
    const vtkIdType* pts = nullptr;
    vtkIdType npts = 0;
    polysIter->GetCurrentCell(npts, pts);
    for (vtkIdType i = 1; i < npts - 1; ++i)
    {
      vtkTypeFloat32 val = npts == 3 ? -1 : i == 1 ? 2 : i == npts - 2 ? 0 : 1;
      edgeArray->InsertNextValue(val);
    }
  }
  return edgeArray;
}

//------------------------------------------------------------------------------
bool vtkWebGPUPolyDataMapper::UpdateMeshGeometryBuffers(const wgpu::Device& device, vtkActor* actor)
{
  if (this->CachedInput == nullptr)
  {
    vtkDebugMacro(<< "No cached input.");
    this->InvokeEvent(vtkCommand::StartEvent, nullptr);
    if (!this->Static)
    {
      this->GetInputAlgorithm()->Update();
    }
    this->CachedInput = this->CurrentInput = this->GetInput();
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
  vtkDataArray* pointPositions = this->CurrentInput->GetPoints()->GetData();
  vtkDataArray* pointColors =
    this->HasPointColors ? vtkDataArray::SafeDownCast(this->Colors) : nullptr;
  vtkDataArray* pointNormals = pointData->GetNormals();
  vtkDataArray* pointTangents = pointData->GetTangents();
  vtkDataArray* pointUvs = pointData->GetTCoords();

  using DispatchT = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::AllTypes>;
  if (this->MeshSSBO.Point.Buffer.Get() != nullptr)
  {
    this->MeshSSBO.Point.Buffer.Destroy();
  }

  wgpu::BufferDescriptor pointBufDescriptor{};
  pointBufDescriptor.size = this->GetExactPointBufferSize();
  pointBufDescriptor.label = "Upload point buffer";
  pointBufDescriptor.mappedAtCreation = false;
  pointBufDescriptor.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;
  this->MeshSSBO.Point.Buffer = device.CreateBuffer(&pointBufDescriptor);

  ::WriteTypedArray<vtkTypeFloat32> pointDataWriter{ 0, this->MeshSSBO.Point.Buffer, device, 1. };

  pointDataWriter.Denominator = 1.0;
  pointDataWriter.Offset = 0;
  for (int attributeIndex = 0; attributeIndex < PointDataAttributes::POINT_NB_ATTRIBUTES;
       attributeIndex++)
  {
    switch (PointDataAttributesOrder[attributeIndex])
    {
      case PointDataAttributes::POINT_POSITIONS:
        meshAttrDescriptor.Positions.Start = pointDataWriter.Offset / sizeof(vtkTypeFloat32);

        if (!DispatchT::Execute(pointPositions, pointDataWriter))
        {
          pointDataWriter(pointPositions);
        }
        meshAttrDescriptor.Positions.NumComponents = pointPositions->GetNumberOfComponents();
        meshAttrDescriptor.Positions.NumTuples = pointPositions->GetNumberOfTuples();
        vtkDebugMacro(<< "[Positions] "
                      << "-- " << pointDataWriter.Offset << " bytes ");

        break;

      case PointDataAttributes::POINT_COLORS:
        pointDataWriter.Denominator = 255.0f;
        meshAttrDescriptor.Colors.Start = pointDataWriter.Offset / sizeof(vtkTypeFloat32);

        if (!DispatchT::Execute(pointColors, pointDataWriter))
        {
          pointDataWriter(pointColors);
        }
        pointDataWriter.Denominator = 1.0f;
        meshAttrDescriptor.Colors.NumComponents =
          pointColors ? pointColors->GetNumberOfComponents() : 0;
        meshAttrDescriptor.Colors.NumTuples = pointColors ? pointColors->GetNumberOfTuples() : 0;
        vtkDebugMacro(<< "[Colors] "
                      << "-- " << pointDataWriter.Offset << " bytes ");

        break;

      case PointDataAttributes::POINT_NORMALS:
        meshAttrDescriptor.Normals.Start = pointDataWriter.Offset / sizeof(vtkTypeFloat32);
        if (!DispatchT::Execute(pointNormals, pointDataWriter))
        {
          pointDataWriter(pointNormals);
        }
        meshAttrDescriptor.Normals.NumComponents =
          pointNormals ? pointNormals->GetNumberOfComponents() : 0;
        meshAttrDescriptor.Normals.NumTuples = pointNormals ? pointNormals->GetNumberOfTuples() : 0;
        vtkDebugMacro(<< "[Normals] "
                      << "-- " << pointDataWriter.Offset << " bytes ");
        break;

      case PointDataAttributes::POINT_TANGENTS:
        meshAttrDescriptor.Tangents.Start = pointDataWriter.Offset / sizeof(vtkTypeFloat32);
        if (!DispatchT::Execute(pointTangents, pointDataWriter))
        {
          pointDataWriter(pointTangents);
        }
        meshAttrDescriptor.Tangents.NumComponents =
          pointTangents ? pointTangents->GetNumberOfComponents() : 0;
        meshAttrDescriptor.Tangents.NumTuples =
          pointTangents ? pointTangents->GetNumberOfTuples() : 0;
        vtkDebugMacro(<< "[Tangents] "
                      << "-- " << pointDataWriter.Offset << " bytes ");
        break;

      case PointDataAttributes::POINT_UVS:
        meshAttrDescriptor.UVs.Start = pointDataWriter.Offset / sizeof(vtkTypeFloat32);
        if (!DispatchT::Execute(pointUvs, pointDataWriter))
        {
          pointDataWriter(pointUvs);
        }
        meshAttrDescriptor.UVs.NumComponents = pointUvs ? pointUvs->GetNumberOfComponents() : 0;
        meshAttrDescriptor.UVs.NumTuples = pointUvs ? pointUvs->GetNumberOfTuples() : 0;
        vtkDebugMacro(<< "[UVs] "
                      << "-- " << pointDataWriter.Offset << " bytes ");
        break;

      default:
        break;
    }
  }

  if (this->MeshSSBO.Cell.Buffer.Get() != nullptr)
  {
    this->MeshSSBO.Cell.Buffer.Destroy();
  }
  wgpu::BufferDescriptor cellBufDescriptor{};
  cellBufDescriptor.size = this->GetExactCellBufferSize();
  cellBufDescriptor.label = "Upload cell buffer";
  cellBufDescriptor.mappedAtCreation = false;
  cellBufDescriptor.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;
  this->MeshSSBO.Cell.Buffer = device.CreateBuffer(&cellBufDescriptor);

  ::WriteTypedArray<vtkTypeFloat32> cellBufWriter{ 0, this->MeshSSBO.Cell.Buffer, device, 1. };

  vtkCellData* cellData = this->CurrentInput->GetCellData();
  vtkDataArray* cellColors =
    this->HasCellColors ? vtkDataArray::SafeDownCast(this->Colors) : nullptr;
  vtkDataArray* cellNormals = this->HasCellNormals ? cellData->GetNormals() : nullptr;

  for (int attribute_index = 0; attribute_index < CellDataAttributes::CELL_NB_ATTRIBUTES;
       attribute_index++)
  {
    switch (CellDataAttributesOrder[attribute_index])
    {
      case CellDataAttributes::CELL_EDGES:
      {
        auto edgeArray =
          vtk::TakeSmartPointer(this->ComputeEdgeArray(this->CurrentInput->GetPolys()));
        meshAttrDescriptor.CellEdgeArray.Start = cellBufWriter.Offset / sizeof(vtkTypeFloat32);
        if (!DispatchT::Execute(edgeArray, cellBufWriter))
        {
          cellBufWriter(edgeArray.Get());
        }
        meshAttrDescriptor.CellEdgeArray.NumComponents = 1;
        meshAttrDescriptor.CellEdgeArray.NumTuples = this->EdgeArrayCount;
        vtkDebugMacro(<< "[Cell edge array] "
                      << "-- " << cellBufWriter.Offset << " bytes ");
        break;
      }

      case CellDataAttributes::CELL_COLORS:
      {
        meshAttrDescriptor.CellColors.Start = cellBufWriter.Offset / sizeof(vtkTypeFloat32);
        cellBufWriter.Denominator = 255.0f;
        if (!DispatchT::Execute(cellColors, cellBufWriter))
        {
          cellBufWriter(cellColors);
        }
        cellBufWriter.Denominator = 1.0f;
        meshAttrDescriptor.CellColors.NumComponents =
          cellColors ? cellColors->GetNumberOfComponents() : 0;
        meshAttrDescriptor.CellColors.NumTuples = cellColors ? cellColors->GetNumberOfTuples() : 0;
        vtkDebugMacro(<< "[Cell colors] "
                      << "-- " << cellBufWriter.Offset << " bytes ");

        break;
      }

      case CellDataAttributes::CELL_NORMALS:
      {
        meshAttrDescriptor.CellNormals.Start = cellBufWriter.Offset / sizeof(vtkTypeFloat32);
        if (!DispatchT::Execute(cellNormals, cellBufWriter))
        {
          cellBufWriter(cellNormals);
        }
        meshAttrDescriptor.CellNormals.NumComponents =
          cellNormals ? cellNormals->GetNumberOfComponents() : 0;
        meshAttrDescriptor.CellNormals.NumTuples =
          cellNormals ? cellNormals->GetNumberOfTuples() : 0;
        vtkDebugMacro(<< "[Cell normals] "
                      << "-- " << cellBufWriter.Offset << " bytes ");

        break;
      }

      default:
        break;
    }
  }

  {

    this->AttributeDescriptorBuffer =
      vtkWebGPUInternalsBuffer::Upload(device, 0, &meshAttrDescriptor, sizeof(meshAttrDescriptor),
        wgpu::BufferUsage::Uniform, "Mesh attribute descriptor");

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
      vtkNew<vtkTypeUInt32Array> indices;
      indices->Allocate(sizes[0]);
      vtkSmartPointer<vtkCellArrayIterator> vertsIter = vtk::TakeSmartPointer(verts->NewIterator());
      for (vertsIter->GoToFirstCell(); !vertsIter->IsDoneWithTraversal();
           vertsIter->GoToNextCell(), ++cellCount)
      {
        vertsIter->GetCurrentCell(npts, pts);
        for (int i = 0; i < npts; ++i)
        {
          indices->InsertNextValue(cellCount);
          indices->InsertNextValue(pts[i]);
        }
      }
      const std::size_t sizeBytes = indices->GetDataSize() * indices->GetDataTypeSize();
      this->PointPrimitiveBGInfo.Buffer = vtkWebGPUInternalsBuffer::Upload(device, 0,
        indices->GetPointer(0), sizeBytes, wgpu::BufferUsage::Storage, "Upload vtkPolyData::Verts");
    }
    vtkDebugMacro(<< "[Verts] "
                  << "-- " << sizes[0] << " bytes ");
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
      vtkNew<vtkTypeUInt32Array> indices;
      indices->Allocate(sizes[1]);
      vtkSmartPointer<vtkCellArrayIterator> linesIter = vtk::TakeSmartPointer(lines->NewIterator());
      for (linesIter->GoToFirstCell(); !linesIter->IsDoneWithTraversal();
           linesIter->GoToNextCell(), ++cellCount)
      {
        linesIter->GetCurrentCell(npts, pts);
        const int numSubLines = npts - 1;
        for (int i = 0; i < numSubLines; ++i)
        {
          indices->InsertNextValue(cellCount);
          indices->InsertNextValue(pts[i]);
          indices->InsertNextValue(cellCount);
          indices->InsertNextValue(pts[i + 1]);
        }
      }
      const std::size_t sizeBytes = indices->GetDataSize() * indices->GetDataTypeSize();
      this->LinePrimitiveBGInfo.Buffer = vtkWebGPUInternalsBuffer::Upload(device, 0,
        indices->GetPointer(0), sizeBytes, wgpu::BufferUsage::Storage, "Upload vtkPolyData::Lines");
    }
    vtkDebugMacro(<< "[Lines] "
                  << "-- " << sizes[1] << " bytes ");
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
      vtkNew<vtkTypeUInt32Array> indices;
      indices->Allocate(sizes[2]);
      vtkSmartPointer<vtkCellArrayIterator> polysIter = vtk::TakeSmartPointer(polys->NewIterator());
      for (polysIter->GoToFirstCell(); !polysIter->IsDoneWithTraversal();
           polysIter->GoToNextCell(), ++cellCount)
      {
        polysIter->GetCurrentCell(npts, pts);
        const int numSubTriangles = npts - 2;
        for (int i = 0; i < numSubTriangles; ++i)
        {
          indices->InsertNextValue(cellCount);
          indices->InsertNextValue(pts[0]);
          indices->InsertNextValue(cellCount);
          indices->InsertNextValue(pts[i + 1]);
          indices->InsertNextValue(cellCount);
          indices->InsertNextValue(pts[i + 2]);
        }
      }
      vtkSmartPointer<vtkCellArrayIterator> stripsIter =
        vtk::TakeSmartPointer(strips->NewIterator());
      for (stripsIter->GoToFirstCell(); !stripsIter->IsDoneWithTraversal();
           stripsIter->GoToNextCell(), ++cellCount)
      {
        stripsIter->GetCurrentCell(npts, pts);
        indices->InsertNextValue(cellCount);
        indices->InsertNextValue(pts[0]);
        indices->InsertNextValue(cellCount);
        indices->InsertNextValue(pts[1]);
        indices->InsertNextValue(cellCount);
        indices->InsertNextValue(pts[2]);
        for (int i = 2; i < npts; ++i)
        {
          indices->InsertNextValue(cellCount);
          indices->InsertNextValue(pts[i - 2]);
          indices->InsertNextValue(cellCount);
          indices->InsertNextValue(pts[i - 1]);
          indices->InsertNextValue(cellCount);
          indices->InsertNextValue(pts[i]);
        }
      }
      const std::size_t sizeBytes = indices->GetDataSize() * indices->GetDataTypeSize();
      this->TrianglePrimitiveBGInfo.Buffer =
        vtkWebGPUInternalsBuffer::Upload(device, 0, indices->GetPointer(0), sizeBytes,
          wgpu::BufferUsage::Storage, "Upload vtkPolyData::{Tris,Strips}");
    }
  }
  vtkDebugMacro(<< "[Triangles] "
                << "-- " << sizes[2] << " bytes ");
  this->Primitive2CellIDsBuildTimestamp.Modified();
  vtkDebugMacro(<< __func__ << " bufferModifiedTime=" << this->Primitive2CellIDsBuildTimestamp);
  return true;
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::SetupGraphicsPipeline(
  const wgpu::Device& device, vtkRenderer* renderer, vtkActor* actor)
{
  auto wgpuRenWin = vtkWebGPURenderWindow::SafeDownCast(renderer->GetRenderWindow());
  auto wgpuRenderer = reinterpret_cast<vtkWebGPURenderer*>(renderer);
  // build shaders if needed.
  wgpu::ShaderModule shaderModule = wgpuRenderer->HasShaderCache(PolyData);
  if (shaderModule == nullptr)
  {
    shaderModule = vtkWebGPUInternalsShaderModule::CreateFromWGSL(device, PolyData);
    wgpuRenderer->InsertShader(PolyData, shaderModule);
  }

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

  if (actor->GetProperty()->GetBackfaceCulling())
  {
    descriptor.primitive.cullMode = wgpu::CullMode::Back;
  }
  else if (actor->GetProperty()->GetFrontfaceCulling())
  {
    descriptor.primitive.cullMode = wgpu::CullMode::Front;
  }

  if (this->PointPrimitiveBGInfo.VertexCount > 0)
  {
    std::string info = "primitive=VTK_POINT;representation=" + reprAsStr;
    descriptor.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
    this->PointPrimitiveBGInfo.Pipeline = device.CreateRenderPipeline(&descriptor);
  }

  if (this->LinePrimitiveBGInfo.VertexCount > 0)
  {
    std::string info = "primitive=VTK_LINE;representation=" + reprAsStr;
    descriptor.primitive.topology = representation == VTK_POINTS
      ? wgpu::PrimitiveTopology::TriangleList
      : wgpu::PrimitiveTopology::LineList;
    this->LinePrimitiveBGInfo.Pipeline = device.CreateRenderPipeline(&descriptor);
  }

  if (this->TrianglePrimitiveBGInfo.VertexCount > 0)
  {
    std::string info = "primitive=VTK_TRIANGLE;representation=" + reprAsStr;
    descriptor.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
    this->TrianglePrimitiveBGInfo.Pipeline = device.CreateRenderPipeline(&descriptor);
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::ReleaseGraphicsResources(vtkWindow*) {}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::ShallowCopy(vtkAbstractMapper*) {}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::MapDataArrayToVertexAttribute(const char*, const char*, int, int) {}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::MapDataArrayToMultiTextureAttribute(
  const char*, const char*, int, int)
{
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::RemoveVertexAttributeMapping(const char*) {}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::RemoveAllVertexAttributeMappings() {}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::ProcessSelectorPixelBuffers(
  vtkHardwareSelector*, std::vector<unsigned int>&, vtkProp*)
{
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkWebGPUComputeRenderBuffer>
vtkWebGPUPolyDataMapper::AcquirePointAttributeComputeRenderBuffer(PointDataAttributes attribute,
  int bufferGroup, int bufferBinding, int uniformsGroup, int uniformsBinding)
{
  vtkSmartPointer<vtkWebGPUComputeRenderBuffer> renderBuffer =
    vtkSmartPointer<vtkWebGPUComputeRenderBuffer>::New();

  std::stringstream label;
  label << "Compute render buffer with point attribute " << static_cast<int>(attribute)
        << " and group/binding/uniformGroup/uniformBinding: " << bufferGroup << "/" << bufferBinding
        << "/" << uniformsGroup << "/" << uniformsBinding;

  renderBuffer->SetPointBufferAttribute(attribute);
  renderBuffer->SetCellBufferAttribute(CellDataAttributes::CELL_UNDEFINED);
  renderBuffer->SetGroup(bufferGroup);
  renderBuffer->SetBinding(bufferBinding);
  renderBuffer->SetRenderUniformsGroup(uniformsGroup);
  renderBuffer->SetRenderUniformsBinding(uniformsBinding);
  renderBuffer->SetLabel(label.str());

  this->NotSetupComputeRenderBuffers.insert(renderBuffer);

  return renderBuffer;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkWebGPUComputeRenderBuffer>
vtkWebGPUPolyDataMapper::AcquireCellAttributeComputeRenderBuffer(CellDataAttributes attribute,
  int bufferGroup, int bufferBinding, int uniformsGroup, int uniformsBinding)
{
  vtkSmartPointer<vtkWebGPUComputeRenderBuffer> renderBuffer =
    vtkSmartPointer<vtkWebGPUComputeRenderBuffer>::New();

  std::stringstream label;
  label << "Compute render buffer with cell attribute " << static_cast<int>(attribute)
        << " and group/binding/uniformGroup/uniformBinding: " << bufferGroup << "/" << bufferBinding
        << "/" << uniformsGroup << "/" << uniformsBinding;

  renderBuffer->SetPointBufferAttribute(PointDataAttributes::POINT_UNDEFINED);
  renderBuffer->SetCellBufferAttribute(attribute);
  renderBuffer->SetGroup(bufferGroup);
  renderBuffer->SetBinding(bufferBinding);
  renderBuffer->SetRenderUniformsGroup(uniformsGroup);
  renderBuffer->SetRenderUniformsBinding(uniformsBinding);
  renderBuffer->SetLabel(label.str());

  this->NotSetupComputeRenderBuffers.insert(renderBuffer);

  return renderBuffer;
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
