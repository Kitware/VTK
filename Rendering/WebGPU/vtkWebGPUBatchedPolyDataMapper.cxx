// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUBatchedPolyDataMapper.h"
#include "vtkColorTransferFunction.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkLookupTable.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkWebGPUActor.h"
#include "vtkWebGPUCellToPrimitiveConverter.h"
#include "vtkWebGPUPolyDataMapper.h"
#include "vtkWebGPURenderPipelineCache.h"
#include "vtkWebGPURenderWindow.h"
#include "vtkWebGPURenderer.h"

#include "Private/vtkWebGPUBindGroupInternals.h"
#include "Private/vtkWebGPUBindGroupLayoutInternals.h"

#include <cstddef>
#include <sstream>

namespace
{
template <typename T>
class ScopedValueRollback
{
public:
  ScopedValueRollback(T& value, T newValue)
  {
    Value = value;
    Pointer = &value;
    *Pointer = newValue;
  }
  ~ScopedValueRollback() { *Pointer = Value; }

private:
  T* Pointer = nullptr;
  T Value;
};

} // end anonymous namespace

VTK_ABI_NAMESPACE_BEGIN

#define SCOPED_ROLLBACK(type, varName)                                                             \
  ScopedValueRollback<type> saver_##varName(this->varName, batchElement->varName)

#define SCOPED_ROLLBACK_CUSTOM_VARIABLE(type, varName, newVarName)                                 \
  ScopedValueRollback<type> saver_##varName(this->varName, newVarName)

#define SCOPED_ROLLBACK_ARRAY_ELEMENT(type, varName, idx)                                          \
  ScopedValueRollback<type> saver_##varName##idx(this->varName[idx], batchElement->varName[idx])

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkWebGPUBatchedPolyDataMapper);

//------------------------------------------------------------------------------
vtkWebGPUBatchedPolyDataMapper::vtkWebGPUBatchedPolyDataMapper()
{
  // force static
  this->Static = true;
}

//------------------------------------------------------------------------------
vtkWebGPUBatchedPolyDataMapper::~vtkWebGPUBatchedPolyDataMapper() = default;

//------------------------------------------------------------------------------
void vtkWebGPUBatchedPolyDataMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Parent: " << this->Parent << '\n';
}

//------------------------------------------------------------------------------
void vtkWebGPUBatchedPolyDataMapper::AddBatchElement(unsigned int flatIndex, BatchElement&& element)
{
  auto address = reinterpret_cast<std::uintptr_t>(element.PolyData);
  auto found = this->VTKPolyDataToBatchElement.find(address);

  this->FlatIndexToPolyData[flatIndex] = address;

  if (found == this->VTKPolyDataToBatchElement.end())
  {
    this->VTKPolyDataToBatchElement[address] =
      std::unique_ptr<BatchElement>(new BatchElement(std::move(element)));
    this->VTKPolyDataToBatchElement[address]->Marked = true;
    this->Modified();
  }
  else
  {
    auto& batchElement = found->second;
    batchElement->FlatIndex = flatIndex;
    batchElement->Marked = true;
  }
}

//------------------------------------------------------------------------------
vtkCompositePolyDataMapperDelegator::BatchElement* vtkWebGPUBatchedPolyDataMapper::GetBatchElement(
  vtkPolyData* polydata)
{
  auto address = reinterpret_cast<std::uintptr_t>(polydata);
  auto found = this->VTKPolyDataToBatchElement.find(address);
  if (found != this->VTKPolyDataToBatchElement.end())
  {
    return found->second.get();
  }
  return nullptr;
}

//------------------------------------------------------------------------------
void vtkWebGPUBatchedPolyDataMapper::ClearBatchElements()
{
  this->VTKPolyDataToBatchElement.clear();
  this->FlatIndexToPolyData.clear();
  this->Modified();
}

//------------------------------------------------------------------------------
std::vector<vtkPolyData*> vtkWebGPUBatchedPolyDataMapper::GetRenderedList() const
{
  std::vector<vtkPolyData*> result;
  result.reserve(this->VTKPolyDataToBatchElement.size());
  for (const auto& iter : this->VTKPolyDataToBatchElement)
  {
    result.emplace_back(iter.second->PolyData);
  }
  return result;
}

//------------------------------------------------------------------------------
void vtkWebGPUBatchedPolyDataMapper::SetParent(vtkCompositePolyDataMapper* parent)
{
  this->Parent = parent;
  this->SetInputDataObject(0, parent->GetInputDataObject(0, 0));
}

//------------------------------------------------------------------------------
void vtkWebGPUBatchedPolyDataMapper::RenderPiece(vtkRenderer* renderer, vtkActor* actor)
{
  vtkLogScopeFunction(TRACE);
  if (this->VTKPolyDataToBatchElement.empty())
  {
    vtkWarningMacro(<< "No batch elements!");
    return;
  }

  auto* wgpuRenderer = vtkWebGPURenderer::SafeDownCast(renderer);
  vtkLog(TRACE,
    "RenderPiece for actor: " << actor << " in renderer: " << renderer
                              << " in stage: " << static_cast<int>(wgpuRenderer->GetRenderStage()));
  switch (wgpuRenderer->GetRenderStage())
  {
    case vtkWebGPURenderer::RenderStageEnum::SyncDeviceResources:
    {
      if (this->GetMTime() > this->ResourcesSyncTimeStamp)
      {
        vtkLogScopeF(TRACE, "SyncDeviceResources");
        // See the override methods - UpdateMeshGeometryBuffers and UpdateMeshTopologyBuffers
        this->Superclass::RenderPiece(renderer, actor);
        this->ResourcesSyncTimeStamp.Modified();
      }
      break;
    }
    case vtkWebGPURenderer::RenderStageEnum::RecordingCommands:
    {
      vtkLogScopeF(TRACE, "RecordingCommands");
      this->MeshAttributeDynamicOffsets.resize(1);
      if (wgpuRenderer->GetUseRenderBundles())
      {
        this->CurrentDrawMeshId = 0;
        for (auto& iter : this->VTKPolyDataToBatchElement)
        {
          if (!iter.second->Visibility)
          {
            continue;
          }
          this->MeshAttributeDynamicOffsets[0] =
            static_cast<std::uint32_t>(this->CurrentDrawMeshId) *
            this->CompositeDataPropertyStorage.BindingSize;
          this->RecordDrawCommands(renderer, actor, wgpuRenderer->GetRenderBundleEncoder());
          this->CurrentDrawMeshId++;
        }
      }
      else
      {
        this->CurrentDrawMeshId = 0;
        for (auto& iter : this->VTKPolyDataToBatchElement)
        {
          if (!iter.second->Visibility)
          {
            continue;
          }
          this->MeshAttributeDynamicOffsets[0] =
            static_cast<std::uint32_t>(this->CurrentDrawMeshId) *
            this->CompositeDataPropertyStorage.BindingSize;
          this->RecordDrawCommands(renderer, actor, wgpuRenderer->GetRenderPassEncoder());
          this->CurrentDrawMeshId++;
        }
      }
      break;
    }
    default:
      break;
  }
}

//------------------------------------------------------------------------------
bool vtkWebGPUBatchedPolyDataMapper::ShouldReleaseGraphicsResourcesOnSync()
{
  return (this->GetMTime() > this->ResourcesSyncTimeStamp);
}

//------------------------------------------------------------------------------
bool vtkWebGPUBatchedPolyDataMapper::EnsureInput()
{
  return !this->VTKPolyDataToBatchElement.empty();
}

//------------------------------------------------------------------------------
void vtkWebGPUBatchedPolyDataMapper::DeducePointCellAttributeAvailability()
{
  vtkLogScopeFunction(TRACE);
  std::array<bool, POINT_NB_ATTRIBUTES> prevHasPointAttributes;
  std::array<bool, CELL_NB_ATTRIBUTES> prevHasCellAttributes;
  int elementId = 0;
  for (const auto& iter : this->VTKPolyDataToBatchElement)
  {
    const auto& batchElement = iter.second;
    if (!batchElement->Visibility)
    {
      continue;
    }
    // Get rid of old texture color coordinates if any
    if (this->ColorCoordinates)
    {
      this->ColorCoordinates->UnRegister(this);
      this->ColorCoordinates = nullptr;
    }
    // Get rid of old texture color coordinates if any
    if (this->Colors)
    {
      this->Colors->UnRegister(this);
      this->Colors = nullptr;
    }
    SCOPED_ROLLBACK(int, ColorMode);
    SCOPED_ROLLBACK(int, ScalarMode);
    SCOPED_ROLLBACK(int, ArrayAccessMode);
    SCOPED_ROLLBACK(int, ArrayComponent);
    SCOPED_ROLLBACK(int, ArrayId);
    SCOPED_ROLLBACK_CUSTOM_VARIABLE(char*, ArrayName,
      static_cast<char*>(
        batchElement->ArrayName.empty() ? nullptr : &batchElement->ArrayName.front()));
    SCOPED_ROLLBACK(vtkIdType, FieldDataTupleId);
    SCOPED_ROLLBACK(vtkTypeBool, ScalarVisibility);
    SCOPED_ROLLBACK(vtkTypeBool, UseLookupTableScalarRange);
    SCOPED_ROLLBACK(vtkTypeBool, InterpolateScalarsBeforeMapping);
    SCOPED_ROLLBACK_ARRAY_ELEMENT(double, ScalarRange, 0);
    SCOPED_ROLLBACK_ARRAY_ELEMENT(double, ScalarRange, 1);
    this->CurrentInput = batchElement->PolyData;
    this->Superclass::DeducePointCellAttributeAvailability();
    if (elementId > 0)
    {
      for (int i = 0; i < CELL_NB_ATTRIBUTES; ++i)
      {
        if (this->HasCellAttributes[i] != prevHasCellAttributes[i])
        {
          vtkWarningMacro(<< "Detected non-uniform cell attributes within the batch! "
                             "Rendering will fail.");
        }
      }
      for (int i = 0; i < POINT_NB_ATTRIBUTES; ++i)
      {
        if (this->HasPointAttributes[i] != prevHasPointAttributes[i])
        {
          vtkWarningMacro(<< "Detected non-uniform point attributes within the batch! "
                             "Rendering will fail.");
        }
      }
    }
    std::copy_n(this->HasCellAttributes, CELL_NB_ATTRIBUTES, prevHasCellAttributes.begin());
    std::copy_n(this->HasPointAttributes, POINT_NB_ATTRIBUTES, prevHasPointAttributes.begin());
    ++elementId;
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUBatchedPolyDataMapper::UpdateMeshGeometryBuffers(
  vtkWebGPUConfiguration* wgpuConfiguration)
{
  vtkLogScopeFunction(TRACE);
  if (!this->AllocateCompositeDataPropertyStorageBuffer(wgpuConfiguration))
  {
    vtkErrorMacro(<< "Unable to render. Failed to allocate buffers for composite data unfiorms.");
    return;
  }
  if (this->MinStorageBufferOffsetAlignment == 0)
  {
    vtkErrorMacro(<< "MinStorageBufferOffsetAlignment = 0. "
                     "AllocateCompositeDataPropertyStorageBuffer was not called!");
    return;
  }
  if (this->CompositeDataPropertyStorage.Buffer == nullptr)
  {
    if (this->VTKPolyDataToBatchElement.empty())
    {
      return;
    }
    else
    {
      vtkErrorMacro("CompositeDataPropertyStorage.Buffer = nullptr. "
                    "AllocateCompositeDataPropertyStorageBuffer was not called!");
      return;
    }
  }
  std::uint64_t iMesh = 0;
  vtkTypeUInt32 cellIdOffsetForSelector = 0;
  vtkTypeUInt32 cellIdOffsetForVerts = 0, cellIdOffsetForLines = 0, cellIdOffsetForPolys = 0;
  for (auto& iter : this->VTKPolyDataToBatchElement)
  {
    const auto& batchElement = iter.second;
    if (!batchElement->Visibility)
    {
      continue;
    }
    // Set the variables that affect scalar coloring of the current block.
    SCOPED_ROLLBACK(int, ColorMode);
    SCOPED_ROLLBACK(int, ScalarMode);
    SCOPED_ROLLBACK(int, ArrayAccessMode);
    SCOPED_ROLLBACK(int, ArrayComponent);
    SCOPED_ROLLBACK(int, ArrayId);
    SCOPED_ROLLBACK_CUSTOM_VARIABLE(char*, ArrayName,
      static_cast<char*>(
        batchElement->ArrayName.empty() ? nullptr : &batchElement->ArrayName.front()));
    SCOPED_ROLLBACK(vtkIdType, FieldDataTupleId);
    SCOPED_ROLLBACK(vtkTypeBool, ScalarVisibility);
    SCOPED_ROLLBACK(vtkTypeBool, UseLookupTableScalarRange);
    SCOPED_ROLLBACK(vtkTypeBool, InterpolateScalarsBeforeMapping);
    SCOPED_ROLLBACK_ARRAY_ELEMENT(double, ScalarRange, 0);
    SCOPED_ROLLBACK_ARRAY_ELEMENT(double, ScalarRange, 1);
    this->CurrentInput = batchElement->PolyData;
    // Get rid of old texture color coordinates if any
    if (this->ColorCoordinates)
    {
      this->ColorCoordinates->UnRegister(this);
      this->ColorCoordinates = nullptr;
    }
    // Get rid of old texture color coordinates if any
    if (this->Colors)
    {
      this->Colors->UnRegister(this);
      this->Colors = nullptr;
    }
    this->Superclass::UpdateMeshGeometryBuffers(wgpuConfiguration);
    // If requested, color partial / missing arrays with NaN color.
    bool useNanColor = false;
    double nanColor[4] = { -1., -1., -1., -1 };
    if (this->Parent->GetColorMissingArraysWithNanColor() && this->ScalarVisibility)
    {
      int cellFlag = 0;
      vtkAbstractArray* scalars = vtkAbstractMapper::GetAbstractScalars(batchElement->PolyData,
        this->ScalarMode, this->ArrayAccessMode, this->ArrayId, this->ArrayName, cellFlag);
      if (scalars == nullptr)
      {
        vtkLookupTable* lut = vtkLookupTable::SafeDownCast(this->GetLookupTable());
        vtkColorTransferFunction* ctf =
          lut ? nullptr : vtkColorTransferFunction::SafeDownCast(this->GetLookupTable());
        if (lut)
        {
          lut->GetNanColor(nanColor);
          useNanColor = true;
        }
        else if (ctf)
        {
          ctf->GetNanColor(nanColor);
          useNanColor = true;
        }
      }
    }
    CompositeDataProperties properties;
    properties.ApplyOverrideColors = useNanColor || batchElement->OverridesColor;
    properties.Opacity = batchElement->Opacity;
    properties.CompositeId = batchElement->FlatIndex;
    properties.Pickable = batchElement->Pickability;
    if (useNanColor)
    {
      std::copy_n(nanColor, 3, properties.Ambient);
      std::copy_n(nanColor, 3, properties.Diffuse);
    }
    else
    {
      std::copy_n(batchElement->AmbientColor.GetData(), 3, properties.Ambient);
      std::copy_n(batchElement->DiffuseColor.GetData(), 3, properties.Diffuse);
    }
    properties.CellIdOffsetForVerts = cellIdOffsetForVerts;
    cellIdOffsetForVerts +=
      this->CurrentInput->GetNumberOfLines() + this->CurrentInput->GetNumberOfPolys();

    cellIdOffsetForLines += this->CurrentInput->GetNumberOfVerts();
    properties.CellIdOffsetForLines = cellIdOffsetForLines;
    cellIdOffsetForLines += this->CurrentInput->GetNumberOfPolys();

    cellIdOffsetForPolys +=
      (this->CurrentInput->GetNumberOfVerts() + this->CurrentInput->GetNumberOfLines());
    properties.CellIdOffsetForPolys = cellIdOffsetForPolys;
    properties.CellIdOffsetForSelector = cellIdOffsetForSelector;
    cellIdOffsetForSelector += this->CurrentInput->GetNumberOfCells();
    wgpuConfiguration->WriteBuffer(this->CompositeDataPropertyStorage.Buffer,
      iMesh * this->CompositeDataPropertyStorage.BindingSize, &properties, sizeof(properties),
      "Write composite properties");
    iMesh++;
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUBatchedPolyDataMapper::UpdateMeshTopologyBuffers(
  vtkWebGPUConfiguration* wgpuConfiguration, vtkProperty* displayProperty)
{
  vtkLogScopeFunction(TRACE);
  const auto batchSize = this->VTKPolyDataToBatchElement.size();
  std::vector<vtkPolyData*> meshes;
  meshes.reserve(batchSize);
  for (const auto& iter : this->VTKPolyDataToBatchElement)
  {
    if (!iter.second->Visibility)
    {
      continue;
    }
    meshes.emplace_back(iter.second->PolyData);
  }
  std::vector<std::pair<vtkTypeUInt32, vtkTypeUInt32>>*
    vertexOffsetsAndCounts[vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES];
  std::array<WGPUBuffer, vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES>
    connectivityBuffers;
  std::array<WGPUBuffer, vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES>
    cellIdBuffers;
  std::array<WGPUBuffer, vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES>
    edgeArrayBuffers;

  for (int i = 0; i < vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES; ++i)
  {
    auto& bgInfo = this->TopologyBindGroupInfos[i];
    bgInfo.VertexOffsetAndCounts.resize(meshes.size());
    vertexOffsetsAndCounts[i] = &(bgInfo.VertexOffsetAndCounts);
    connectivityBuffers[i] = bgInfo.ConnectivityBuffer;
    cellIdBuffers[i] = bgInfo.CellIdBuffer;
    edgeArrayBuffers[i] = bgInfo.EdgeArrayBuffer;
  }
  bool updateTopologyBindGroup = this->CellConverter->DispatchMeshesToPrimitiveComputePipeline(
    wgpuConfiguration, meshes, displayProperty->GetRepresentation(), vertexOffsetsAndCounts,
    connectivityBuffers, cellIdBuffers, edgeArrayBuffers);
  // Handle vertex visibility.
  if (displayProperty->GetVertexVisibility() &&
    // avoids dispatching the cell-to-vertex pipeline again.
    displayProperty->GetRepresentation() != VTK_POINTS)
  {
    // dispatch compute pipeline that extracts cell vertices.
    updateTopologyBindGroup |=
      this->CellConverter->DispatchMeshesToPrimitiveComputePipeline(wgpuConfiguration, meshes,
        VTK_POINTS, vertexOffsetsAndCounts, connectivityBuffers, cellIdBuffers, edgeArrayBuffers);
  }
  // Rebuild topology bind group if required (when VertexCount > 0)
  for (int i = 0; i < vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES; ++i)
  {
    const auto topologySourceType = vtkWebGPUCellToPrimitiveConverter::TopologySourceType(i);
    auto& bgInfo = this->TopologyBindGroupInfos[i];
    bgInfo.VertexCount = 0; // store total vertex count for this batch
    for (const auto& pair : bgInfo.VertexOffsetAndCounts)
    {
      bgInfo.VertexCount += pair.second;
    }
    // setup bind group
    if (updateTopologyBindGroup && bgInfo.VertexCount > 0)
    {
      const std::string& label = this->GetObjectDescription() + "-" +
        vtkWebGPUCellToPrimitiveConverter::GetTopologySourceTypeAsString(topologySourceType);
      bgInfo.BindGroup =
        this->CreateTopologyBindGroup(wgpuConfiguration->GetDevice(), label, topologySourceType);
      this->RebuildGraphicsPipelines = true;
    }
    if (bgInfo.VertexCount == 0)
    {
      if (bgInfo.ConnectivityBuffer)
      {
        wgpu::Buffer(bgInfo.ConnectivityBuffer).Destroy();
        bgInfo.ConnectivityBuffer = nullptr;
      }
      if (bgInfo.CellIdBuffer)
      {
        wgpu::Buffer(bgInfo.CellIdBuffer).Destroy();
        bgInfo.CellIdBuffer = nullptr;
      }
      if (bgInfo.EdgeArrayBuffer)
      {
        wgpu::Buffer(bgInfo.EdgeArrayBuffer).Destroy();
        bgInfo.EdgeArrayBuffer = nullptr;
      }
      if (bgInfo.BindGroup != nullptr)
      {
        bgInfo.BindGroup = nullptr;
        this->RebuildGraphicsPipelines = true;
      }
    }
  }
}

//------------------------------------------------------------------------------
unsigned long vtkWebGPUBatchedPolyDataMapper::GetPointAttributeByteSize(
  vtkWebGPUPolyDataMapper::PointDataAttributes attribute)
{
  vtkLogScopeFunction(TRACE);
  unsigned long result = 0;
  for (const auto& iter : this->VTKPolyDataToBatchElement)
  {
    const auto& batchElement = iter.second;
    if (!batchElement->Visibility)
    {
      continue;
    }
    // Get rid of old texture color coordinates if any
    if (this->ColorCoordinates)
    {
      this->ColorCoordinates->UnRegister(this);
      this->ColorCoordinates = nullptr;
    }
    // Get rid of old texture color coordinates if any
    if (this->Colors)
    {
      this->Colors->UnRegister(this);
      this->Colors = nullptr;
    }
    SCOPED_ROLLBACK(int, ColorMode);
    SCOPED_ROLLBACK(int, ScalarMode);
    SCOPED_ROLLBACK(int, ArrayAccessMode);
    SCOPED_ROLLBACK(int, ArrayComponent);
    SCOPED_ROLLBACK(int, ArrayId);
    SCOPED_ROLLBACK_CUSTOM_VARIABLE(char*, ArrayName,
      static_cast<char*>(
        batchElement->ArrayName.empty() ? nullptr : &batchElement->ArrayName.front()));
    SCOPED_ROLLBACK(vtkIdType, FieldDataTupleId);
    SCOPED_ROLLBACK(vtkTypeBool, ScalarVisibility);
    SCOPED_ROLLBACK(vtkTypeBool, UseLookupTableScalarRange);
    SCOPED_ROLLBACK(vtkTypeBool, InterpolateScalarsBeforeMapping);
    SCOPED_ROLLBACK_ARRAY_ELEMENT(double, ScalarRange, 0);
    SCOPED_ROLLBACK_ARRAY_ELEMENT(double, ScalarRange, 1);
    this->CurrentInput = iter.second->PolyData;
    int cellFlag = 0;
    // initializes this->Colors and this->ColorCoordinates
    this->MapScalars(this->CurrentInput, 1.0, cellFlag);
    result += this->Superclass::GetPointAttributeByteSize(attribute);
  }
  return result;
}

//------------------------------------------------------------------------------
unsigned long vtkWebGPUBatchedPolyDataMapper::GetCellAttributeByteSize(
  vtkWebGPUPolyDataMapper::CellDataAttributes attribute)
{
  vtkLogScopeFunction(TRACE);
  unsigned long result = 0;
  for (const auto& iter : this->VTKPolyDataToBatchElement)
  {
    const auto& batchElement = iter.second;
    if (!batchElement->Visibility)
    {
      continue;
    }
    // Get rid of old texture color coordinates if any
    if (this->ColorCoordinates)
    {
      this->ColorCoordinates->UnRegister(this);
      this->ColorCoordinates = nullptr;
    }
    // Get rid of old texture color coordinates if any
    if (this->Colors)
    {
      this->Colors->UnRegister(this);
      this->Colors = nullptr;
    }
    SCOPED_ROLLBACK(int, ColorMode);
    SCOPED_ROLLBACK(int, ScalarMode);
    SCOPED_ROLLBACK(int, ArrayAccessMode);
    SCOPED_ROLLBACK(int, ArrayComponent);
    SCOPED_ROLLBACK(int, ArrayId);
    SCOPED_ROLLBACK_CUSTOM_VARIABLE(char*, ArrayName,
      static_cast<char*>(
        batchElement->ArrayName.empty() ? nullptr : &batchElement->ArrayName.front()));
    SCOPED_ROLLBACK(vtkIdType, FieldDataTupleId);
    SCOPED_ROLLBACK(vtkTypeBool, ScalarVisibility);
    SCOPED_ROLLBACK(vtkTypeBool, UseLookupTableScalarRange);
    SCOPED_ROLLBACK(vtkTypeBool, InterpolateScalarsBeforeMapping);
    SCOPED_ROLLBACK_ARRAY_ELEMENT(double, ScalarRange, 0);
    SCOPED_ROLLBACK_ARRAY_ELEMENT(double, ScalarRange, 1);
    this->CurrentInput = iter.second->PolyData;
    int cellFlag = 0;
    // initializes this->Colors and this->ColorCoordinates
    this->MapScalars(this->CurrentInput, 1.0, cellFlag);
    result += this->Superclass::GetCellAttributeByteSize(attribute);
  }
  return result;
}

//------------------------------------------------------------------------------
std::vector<WGPUBindGroupLayoutEntry>
vtkWebGPUBatchedPolyDataMapper::GetMeshBindGroupLayoutEntries()
{
  // extend superclass bindings with additional entry for `Mesh` buffer.
  auto entries = this->Superclass::GetMeshBindGroupLayoutEntries();
  std::uint32_t bindingId = static_cast<std::uint32_t>(entries.size());

  // clang-format off
  WGPUBindGroupLayoutEntry meshEntry = vtkWebGPUBindGroupLayoutInternals::LayoutEntryInitializationHelper{
    bindingId++,
    static_cast<WGPUShaderStage>(wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment),
    static_cast<WGPUBufferBindingType>(wgpu::BufferBindingType::ReadOnlyStorage),
    true,
    vtkWebGPUConfiguration::Align(sizeof(CompositeDataProperties), this->MinStorageBufferOffsetAlignment)
  };
  entries.push_back(meshEntry);
  // clang-format on
  return entries;
}

//------------------------------------------------------------------------------
std::vector<WGPUBindGroupEntry> vtkWebGPUBatchedPolyDataMapper::GetMeshBindGroupEntries()
{
  // extend superclass bindings with additional entry for `Mesh` buffer.
  auto entries = this->Superclass::GetMeshBindGroupEntries();
  std::uint32_t bindingId = static_cast<std::uint32_t>(entries.size());
  // clang-format off
  auto bindingInit = vtkWebGPUBindGroupInternals::BindingInitializationHelper{
    bindingId++,
    this->CompositeDataPropertyStorage.Buffer,
    0,
    this->CompositeDataPropertyStorage.BindingSize
  };
  // clang-format on
  entries.push_back(bindingInit.GetAsBinding());
  return entries;
}

//------------------------------------------------------------------------------
std::vector<WGPUBindGroupLayoutEntry>
vtkWebGPUBatchedPolyDataMapper::GetTopologyBindGroupLayoutEntries(
  bool homogeneousCellSize, bool useEdgeArray)
{
  if (homogeneousCellSize)
  {
    std::vector<WGPUBindGroupLayoutEntry> entries;
    std::uint32_t bindingId = 0;

    // clang-format off
    WGPUBindGroupLayoutEntry layoutEntry = vtkWebGPUBindGroupLayoutInternals::LayoutEntryInitializationHelper{
      bindingId++,
      static_cast<WGPUShaderStage>(wgpu::ShaderStage::Vertex),
      static_cast<WGPUBufferBindingType>(wgpu::BufferBindingType::ReadOnlyStorage),
    };
    entries.push_back(layoutEntry);
    // clang-format on
    return entries;
  }
  else
  {
    return this->Superclass::GetTopologyBindGroupLayoutEntries(homogeneousCellSize, useEdgeArray);
  }
}

//------------------------------------------------------------------------------
std::vector<WGPUBindGroupEntry> vtkWebGPUBatchedPolyDataMapper::GetTopologyBindGroupEntries(
  vtkWebGPUCellToPrimitiveConverter::TopologySourceType topologySourceType,
  bool homogeneousCellSize, bool useEdgeArray)
{
  if (homogeneousCellSize)
  {
    std::vector<WGPUBindGroupEntry> entries;
    std::uint32_t bindingId = 0;
    const auto& info = this->TopologyBindGroupInfos[topologySourceType];
    // connectivity
    const auto connectivityBindingInit = vtkWebGPUBindGroupInternals::BindingInitializationHelper{
      bindingId++,
      info.ConnectivityBuffer,
    };
    entries.push_back(connectivityBindingInit.GetAsBinding());
    return entries;
  }
  else
  {
    return this->Superclass::GetTopologyBindGroupEntries(
      topologySourceType, homogeneousCellSize, useEdgeArray);
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUBatchedPolyDataMapper::ReplaceShaderCustomDef(
  GraphicsPipelineType vtkNotUsed(pipelineType), vtkWebGPURenderer* vtkNotUsed(wgpuRenderer),
  vtkWebGPUActor* vtkNotUsed(wgpuActor), std::string& vss, std::string& fss)
{
  const std::string code = R"(struct CompositeDataProperties
{
  apply_override_colors: u32,
  opacity: f32,
  composite_id: u32,
  pickable: u32,
  ambient: vec3<f32>,
  cell_id_offset_for_verts: u32,
  diffuse: vec3<f32>,
  cell_id_offset_for_lines: u32,
  cell_id_offset_for_polys: u32,
  cell_id_offset_for_selector: u32
};)";
  vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::Custom::Def", code,
    /*all=*/false);
  vtkWebGPURenderPipelineCache::Substitute(fss, "//VTK::Custom::Def", code,
    /*all=*/false);
}

//------------------------------------------------------------------------------
void vtkWebGPUBatchedPolyDataMapper::ReplaceShaderCustomBindings(
  GraphicsPipelineType vtkNotUsed(pipelineType), vtkWebGPURenderer* vtkNotUsed(wgpuRenderer),
  vtkWebGPUActor* vtkNotUsed(wgpuActor), std::string& vss, std::string& fss)
{
  auto& bindingId = this->NumberOfBindings[GROUP_MESH];
  std::stringstream codeStream;
  codeStream << "@group(" << GROUP_MESH << ") @binding(" << bindingId++
             << ") var<storage, read> composite_data_properties: CompositeDataProperties;";
  vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::Custom::Bindings", codeStream.str(),
    /*all=*/false);
  vtkWebGPURenderPipelineCache::Substitute(fss, "//VTK::Custom::Bindings", codeStream.str(),
    /*all=*/false);
}

//------------------------------------------------------------------------------
void vtkWebGPUBatchedPolyDataMapper::ReplaceVertexShaderCellId(GraphicsPipelineType pipelineType,
  vtkWebGPURenderer* vtkNotUsed(renderer), vtkWebGPUActor* vtkNotUsed(actor), std::string& vss)
{
  switch (pipelineType)
  {
    case GFX_PIPELINE_POINTS_HOMOGENEOUS_CELL_SIZE:
    case GFX_PIPELINE_POINTS_SHAPED_HOMOGENEOUS_CELL_SIZE:
      vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::CellId::Impl",
        R"(let cell_id = primitive_id + composite_data_properties.cell_id_offset_for_verts;)",
        /*all=*/true);
      break;
    case GFX_PIPELINE_LINES_HOMOGENEOUS_CELL_SIZE:
    case GFX_PIPELINE_LINES_THICK_HOMOGENEOUS_CELL_SIZE:
    case GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN_HOMOGENEOUS_CELL_SIZE:
    case GFX_PIPELINE_LINES_MITER_JOIN_HOMOGENEOUS_CELL_SIZE:
      vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::CellId::Impl",
        R"(let cell_id = primitive_id + composite_data_properties.cell_id_offset_for_lines;)",
        /*all=*/true);
      break;
    case GFX_PIPELINE_TRIANGLES_HOMOGENEOUS_CELL_SIZE:
      vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::CellId::Impl",
        R"(let cell_id = primitive_id + composite_data_properties.cell_id_offset_for_polys;)",
        /*all=*/true);
      break;
    case GFX_PIPELINE_POINTS:
    case GFX_PIPELINE_POINTS_SHAPED:
      vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::CellId::Impl",
        R"(let cell_id = cell_ids[primitive_id] + composite_data_properties.cell_id_offset_for_verts;)",
        /*all=*/true);
      break;
    case GFX_PIPELINE_LINES:
    case GFX_PIPELINE_LINES_THICK:
    case GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN:
    case GFX_PIPELINE_LINES_MITER_JOIN:
      vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::CellId::Impl",
        R"(let cell_id = cell_ids[primitive_id] + composite_data_properties.cell_id_offset_for_lines;)",
        /*all=*/true);
      break;
    case GFX_PIPELINE_TRIANGLES:
      vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::CellId::Impl",
        R"(let cell_id = cell_ids[primitive_id] + composite_data_properties.cell_id_offset_for_polys;)",
        /*all=*/true);
      break;
    default:
      break;
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUBatchedPolyDataMapper::ReplaceVertexShaderPicking(
  GraphicsPipelineType vtkNotUsed(pipelineType), vtkWebGPURenderer* vtkNotUsed(wgpuRenderer),
  vtkWebGPUActor* vtkNotUsed(wgpuActor), std::string& vss)
{
  vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::Picking::Impl",
    R"(if (composite_data_properties.pickable == 1u)
  {
    // Write indices
    output.cell_id = cell_id;
    output.prop_id = actor.color_options.id;
    output.composite_id = composite_data_properties.composite_id;
    output.process_id = 0;
  })",
    /*all=*/true);
}

//------------------------------------------------------------------------------
void vtkWebGPUBatchedPolyDataMapper::ReplaceFragmentShaderColors(GraphicsPipelineType pipelineType,
  vtkWebGPURenderer* wgpuRenderer, vtkWebGPUActor* wgpuActor, std::string& fss)
{
  if (this->HasCellAttributes[CELL_COLORS] || this->HasPointAttributes[POINT_COLORS] ||
    this->HasPointAttributes[POINT_COLOR_UVS])
  {
    vtkWebGPURenderPipelineCache::Substitute(fss, "//VTK::Colors::Impl",
      R"(//VTK::Colors::Impl
  if (composite_data_properties.apply_override_colors == 1u)
  {
    ambient_color = composite_data_properties.ambient.rgb;
    diffuse_color = composite_data_properties.diffuse.rgb;
    opacity = composite_data_properties.opacity;
  })",
      /*all=*/false);
  }
  else
  {
    switch (pipelineType)
    {
      case GFX_PIPELINE_POINTS:
      case GFX_PIPELINE_POINTS_HOMOGENEOUS_CELL_SIZE:
      case GFX_PIPELINE_POINTS_SHAPED:
      case GFX_PIPELINE_POINTS_SHAPED_HOMOGENEOUS_CELL_SIZE:
        break;
      default:
        vtkWebGPURenderPipelineCache::Substitute(fss, "//VTK::Colors::Impl",
          R"(//VTK::Colors::Impl
    ambient_color = composite_data_properties.ambient.rgb;
    diffuse_color = composite_data_properties.diffuse.rgb;
    opacity = composite_data_properties.opacity;)",
          /*all=*/false);
        break;
    }
  }
  this->Superclass::ReplaceFragmentShaderColors(pipelineType, wgpuRenderer, wgpuActor, fss);
}

//------------------------------------------------------------------------------
void vtkWebGPUBatchedPolyDataMapper::ReplaceFragmentShaderPicking(
  GraphicsPipelineType vtkNotUsed(pipelineType), vtkWebGPURenderer* vtkNotUsed(wgpuRenderer),
  vtkWebGPUActor* vtkNotUsed(wgpuActor), std::string& fss)
{
  vtkWebGPURenderPipelineCache::Substitute(fss, "//VTK::Picking::Impl",
    R"(if (composite_data_properties.pickable == 1u)
  {
    output.ids.x = vertex.cell_id + 1 - composite_data_properties.cell_id_offset_for_selector;
    output.ids.y = vertex.prop_id + 1;
    output.ids.z = vertex.composite_id + 1;
    output.ids.w = vertex.process_id + 1;
  })",
    /*all=*/true);
}

//------------------------------------------------------------------------------
vtkWebGPUPolyDataMapper::DrawCallArgs vtkWebGPUBatchedPolyDataMapper::GetDrawCallArgs(
  GraphicsPipelineType pipelineType,
  vtkWebGPUCellToPrimitiveConverter::TopologySourceType topologySourceType)
{
  const auto& bgInfo = this->TopologyBindGroupInfos[topologySourceType];
  if (this->CurrentDrawMeshId >= bgInfo.VertexOffsetAndCounts.size())
  {
    return {};
  }
  const auto& [vertexOffset, vertexCount] = bgInfo.VertexOffsetAndCounts[this->CurrentDrawMeshId];
  switch (topologySourceType)
  {
    case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_VERTS:
    case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_LINE_POINTS:
    case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGON_POINTS:
      if (pipelineType == GFX_PIPELINE_POINTS ||
        pipelineType == GFX_PIPELINE_POINTS_HOMOGENEOUS_CELL_SIZE)
      {
        return { /*VertexOffset=*/vertexOffset, /*vertexCount=*/vertexCount, /*InstanceOffset=*/0,
          /*instanceCount=*/1 };
      }
      if (pipelineType == GFX_PIPELINE_POINTS_SHAPED ||
        pipelineType == GFX_PIPELINE_POINTS_SHAPED_HOMOGENEOUS_CELL_SIZE)
      {
        return { /*VertexOffset=*/0, /*vertexCount=*/4, /*InstanceOffset=*/vertexOffset,
          /*instanceCount=*/vertexCount };
      }
      break;
    case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_LINES:
    case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGON_EDGES:
      if (pipelineType == GFX_PIPELINE_LINES ||
        pipelineType == GFX_PIPELINE_LINES_HOMOGENEOUS_CELL_SIZE)
      {
        return { /*VertexOffset=*/vertexOffset, /*vertexCount=*/vertexCount, /*InstanceOffset=*/0,
          /*instanceCount=*/1 };
      }
      if (pipelineType == GFX_PIPELINE_LINES_THICK ||
        pipelineType == GFX_PIPELINE_LINES_THICK_HOMOGENEOUS_CELL_SIZE)
      {
        return { /*VertexOffset=*/0, /*vertexCount=*/4, /*InstanceOffset=*/vertexOffset / 2,
          /*instanceCount=*/vertexCount / 2 };
      }
      if (pipelineType == GFX_PIPELINE_LINES_MITER_JOIN ||
        pipelineType == GFX_PIPELINE_LINES_MITER_JOIN_HOMOGENEOUS_CELL_SIZE)
      {
        return { /*VertexOffset=*/0, /*vertexCount=*/4, /*InstanceOffset=*/vertexOffset / 2,
          /*instanceCount=*/vertexCount / 2 };
      }
      if (pipelineType == GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN ||
        pipelineType == GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN_HOMOGENEOUS_CELL_SIZE)
      {
        return { /*VertexOffset=*/0, /*vertexCount=*/36, /*InstanceOffset=*/vertexOffset / 2,
          /*instanceCount=*/vertexCount / 2 };
      }
      break;
    case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGONS:
      return { /*VertexOffset=*/vertexOffset, /*vertexCount=*/vertexCount, /*InstanceOffset=*/0,
        /*instanceCount=*/1 };
    case vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES:
    default:
      break;
  }
  return {};
}

//------------------------------------------------------------------------------
vtkWebGPUPolyDataMapper::DrawCallArgs
vtkWebGPUBatchedPolyDataMapper::GetDrawCallArgsForDrawingVertices(
  vtkWebGPUCellToPrimitiveConverter::TopologySourceType topologySourceType)
{
  const auto& bgInfo = this->TopologyBindGroupInfos[topologySourceType];
  if (this->CurrentDrawMeshId >= bgInfo.VertexOffsetAndCounts.size())
  {
    return {};
  }
  const auto& [vertexOffset, vertexCount] = bgInfo.VertexOffsetAndCounts[this->CurrentDrawMeshId];
  return { /*VertexOffset=*/0, /*VertexCount=*/4, /*InstanceOffset=*/vertexOffset,
    /*InstanceCount=*/vertexCount };
}

//------------------------------------------------------------------------------
int vtkWebGPUBatchedPolyDataMapper::CanUseTextureMapForColoring(vtkDataObject*)
{
  if (!this->InterpolateScalarsBeforeMapping)
  {
    return 0; // user doesn't want us to use texture maps at all.
  }
  int cellFlag = 0;
  vtkScalarsToColors* scalarsLookupTable = nullptr;
  for (auto& iter : this->VTKPolyDataToBatchElement)
  {
    auto polydata = iter.second->PolyData;
    vtkDataArray* scalars = vtkAbstractMapper::GetScalars(
      polydata, this->ScalarMode, this->ArrayAccessMode, this->ArrayId, this->ArrayName, cellFlag);

    if (scalars)
    {
      if (cellFlag)
      {
        return 0;
      }
      // Don't use texture if direct coloring using RGB unsigned chars was requested.
      if ((this->ColorMode == VTK_COLOR_MODE_DEFAULT &&
            vtkArrayDownCast<vtkUnsignedCharArray>(scalars)) ||
        this->ColorMode == VTK_COLOR_MODE_DIRECT_SCALARS)
      {
        // Don't use texture if direct coloring using RGB unsigned chars is
        // requested.
        return 0;
      }

      if (scalarsLookupTable && scalars->GetLookupTable() &&
        (scalarsLookupTable != scalars->GetLookupTable()))
      {
        // Two datasets are requesting different lookup tables to color with.
        // We don't handle this case right now for composite datasets.
        return 0;
      }
      if (scalars->GetLookupTable())
      {
        scalarsLookupTable = scalars->GetLookupTable();
      }
    }
  }

  if ((scalarsLookupTable && scalarsLookupTable->GetIndexedLookup()) ||
    (!scalarsLookupTable && this->LookupTable && this->LookupTable->GetIndexedLookup()))
  {
    return 0;
  }

  return 1;
}

//------------------------------------------------------------------------------
bool vtkWebGPUBatchedPolyDataMapper::AllocateCompositeDataPropertyStorageBuffer(
  vtkWebGPUConfiguration* wgpuConfiguration)
{
  const auto batchSize = this->VTKPolyDataToBatchElement.size();
  // cache the minimum required alignment for storage buffer
  if (this->MinStorageBufferOffsetAlignment == 0)
  {
    const auto& device = wgpuConfiguration->GetDevice();
    wgpu::Limits limits{};
    device.GetLimits(&limits);
    this->MinStorageBufferOffsetAlignment = limits.minStorageBufferOffsetAlignment;
  }
  const auto bindingSize = wgpuConfiguration->Align(
    sizeof(CompositeDataProperties), this->MinStorageBufferOffsetAlignment);
  const auto bufferSize = batchSize * bindingSize;
  if (this->CompositeDataPropertyStorage.Size != bufferSize)
  {
    if (this->CompositeDataPropertyStorage.Buffer)
    {
      this->CompositeDataPropertyStorage.Buffer.Destroy();
      this->CompositeDataPropertyStorage.Size = 0;
    }
    const std::string label = "composite_data_property-" + this->GetObjectDescription();
    wgpu::BufferDescriptor desc = {};
    desc.label = label.c_str();
    desc.mappedAtCreation = false;
    desc.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;
    desc.size = bufferSize;
    this->CompositeDataPropertyStorage.Buffer = wgpuConfiguration->CreateBuffer(desc);
    this->CompositeDataPropertyStorage.Size = bufferSize;
    this->CompositeDataPropertyStorage.BindingSize = static_cast<std::uint32_t>(bindingSize);
    this->ResourcesSyncTimeStamp = vtkTimeStamp();
    this->RebuildGraphicsPipelines = true;
  }
  return (this->CompositeDataPropertyStorage.Buffer != nullptr);
}

//------------------------------------------------------------------------------
void vtkWebGPUBatchedPolyDataMapper::UnmarkBatchElements()
{
  for (auto& iter : this->VTKPolyDataToBatchElement)
  {
    auto& batchElement = iter.second;
    batchElement->Marked = false;
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUBatchedPolyDataMapper::ClearUnmarkedBatchElements()
{
  for (auto iter = this->VTKPolyDataToBatchElement.begin();
       iter != this->VTKPolyDataToBatchElement.end();)
  {
    if (!iter->second->Marked)
    {
      this->VTKPolyDataToBatchElement.erase(iter++);
      this->Modified();
    }
    else
    {
      ++iter;
    }
  }
}

//------------------------------------------------------------------------------
vtkMTimeType vtkWebGPUBatchedPolyDataMapper::GetMTime()
{
  if (this->Parent)
  {
    return std::max(this->Superclass::GetMTime(), this->Parent->GetMTime());
  }
  else
  {
    return this->Superclass::GetMTime();
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUBatchedPolyDataMapper::ReleaseGraphicsResources(vtkWindow* w)
{
  this->CompositeDataPropertyStorage = {};
  this->ResourcesSyncTimeStamp = vtkTimeStamp();
  this->MinStorageBufferOffsetAlignment = 0;
  this->Superclass::ReleaseGraphicsResources(w);
}

VTK_ABI_NAMESPACE_END
