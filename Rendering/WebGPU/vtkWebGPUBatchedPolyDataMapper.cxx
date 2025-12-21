// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUBatchedPolyDataMapper.h"
#include "vtkColorTransferFunction.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkLookupTable.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkWebGPUActor.h"
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
  ScopedValueRollback<type> saver_##varName(this->varName, batchElement.varName)

#define SCOPED_ROLLBACK_CUSTOM_VARIABLE(type, varName, newVarName)                                 \
  ScopedValueRollback<type> saver_##varName(this->varName, newVarName)

#define SCOPED_ROLLBACK_ARRAY_ELEMENT(type, varName, idx)                                          \
  ScopedValueRollback<type> saver_##varName##idx(this->varName[idx], batchElement.varName[idx])

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
  os << indent << "OverrideColorUploadTimestamp: " << this->OverrideColorUploadTimestamp << '\n';
  os << indent << "LastBlockVisibility: " << this->LastBlockVisibility << '\n';
  os << indent << "LastUseNanColor: " << this->LastUseNanColor << '\n';
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
  if (this->VTKPolyDataToBatchElement.empty())
  {
    vtkWarningMacro(<< "No batch elements!");
    return;
  }

  auto* wgpuRenderWindow = vtkWebGPURenderWindow::SafeDownCast(renderer->GetRenderWindow());
  auto* wgpuRenderer = vtkWebGPURenderer::SafeDownCast(renderer);
  auto* wgpuConfiguration = wgpuRenderWindow->GetWGPUConfiguration();
  const auto& device = wgpuRenderWindow->GetDevice();

  auto& batchElement = *(this->VTKPolyDataToBatchElement.begin()->second);
  if (this->LastBlockVisibility != batchElement.Visibility)
  {
    wgpuRenderer->InvalidateBundle();
  }
  this->LastBlockVisibility = batchElement.Visibility;

  if (!batchElement.Visibility)
  {
    this->ReleaseGraphicsResources(wgpuRenderWindow);
    return;
  }

  // Set the variables that affect scalar coloring of the current block.
  SCOPED_ROLLBACK(int, ColorMode);
  SCOPED_ROLLBACK(int, ScalarMode);
  SCOPED_ROLLBACK(int, ArrayAccessMode);
  SCOPED_ROLLBACK(int, ArrayComponent);
  SCOPED_ROLLBACK(int, ArrayId);
  SCOPED_ROLLBACK_CUSTOM_VARIABLE(char*, ArrayName,
    static_cast<char*>(batchElement.ArrayName.empty() ? nullptr : &batchElement.ArrayName.front()));
  SCOPED_ROLLBACK(vtkIdType, FieldDataTupleId);
  SCOPED_ROLLBACK(vtkTypeBool, ScalarVisibility);
  SCOPED_ROLLBACK(vtkTypeBool, UseLookupTableScalarRange);
  SCOPED_ROLLBACK(vtkTypeBool, InterpolateScalarsBeforeMapping);
  SCOPED_ROLLBACK_ARRAY_ELEMENT(double, ScalarRange, 0);
  SCOPED_ROLLBACK_ARRAY_ELEMENT(double, ScalarRange, 1);

  this->CachedInput = this->CurrentInput = batchElement.PolyData;
  const std::string label = "CompositeDataProperties-" + this->CurrentInput->GetObjectDescription();
  if (this->CompositeDataPropertiesBuffer == nullptr)
  {
    const auto alignedSize = vtkWebGPUConfiguration::Align(sizeof(CompositeDataProperties), 16);
    this->CompositeDataPropertiesBuffer = wgpuConfiguration->CreateBuffer(alignedSize,
      wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst,
      /*mappedAtCreation=*/false, label.c_str());
    // Rebuild pipeline and bindgroups when buffer is re-created.
    this->RebuildGraphicsPipelines = true;
  }
  this->Superclass::RenderPiece(renderer, actor);

  // If requested, color partial / missing arrays with NaN color.
  bool useNanColor = false;
  double nanColor[4] = { -1., -1., -1., -1 };
  if (this->Parent->GetColorMissingArraysWithNanColor() && this->ScalarVisibility)
  {
    int cellFlag = 0;
    vtkAbstractArray* scalars = vtkAbstractMapper::GetAbstractScalars(batchElement.PolyData,
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

  // write to the `OverrideColorDescriptor` portion of the `Mesh` buffer only if
  // colors/opacity/pickability per block changed.
  if ((this->Parent->GetMTime() > this->OverrideColorUploadTimestamp) ||
    (this->LastUseNanColor != useNanColor))
  {
    if (useNanColor)
    {
      this->UploadCompositeDataProperties(wgpuConfiguration, true, batchElement.Opacity,
        vtkColor3d{ nanColor }, vtkColor3d{ nanColor }, batchElement.FlatIndex,
        batchElement.Pickability);
    }
    else
    {
      this->UploadCompositeDataProperties(wgpuConfiguration, batchElement.OverridesColor,
        batchElement.Opacity, batchElement.AmbientColor, batchElement.DiffuseColor,
        batchElement.FlatIndex, batchElement.Pickability);
    }
  }
  this->LastUseNanColor = useNanColor;
}

//------------------------------------------------------------------------------
std::vector<wgpu::BindGroupLayoutEntry>
vtkWebGPUBatchedPolyDataMapper::GetMeshBindGroupLayoutEntries()
{
  // extend superclass bindings with additional entry for `Mesh` buffer.
  auto entries = this->Superclass::GetMeshBindGroupLayoutEntries();
  std::uint32_t bindingId = entries.size();

  entries.emplace_back(
    vtkWebGPUBindGroupLayoutInternals::LayoutEntryInitializationHelper{ bindingId++,
      wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment, wgpu::BufferBindingType::Uniform });
  return entries;
}

//------------------------------------------------------------------------------
std::vector<wgpu::BindGroupEntry> vtkWebGPUBatchedPolyDataMapper::GetMeshBindGroupEntries()
{
  // extend superclass bindings with additional entry for `Mesh` buffer.
  auto entries = this->Superclass::GetMeshBindGroupEntries();
  std::uint32_t bindingId = entries.size();

  auto bindingInit = vtkWebGPUBindGroupInternals::BindingInitializationHelper{ bindingId++,
    this->CompositeDataPropertiesBuffer, 0 };
  entries.emplace_back(bindingInit.GetAsBinding());
  return entries;
}

//------------------------------------------------------------------------------
void vtkWebGPUBatchedPolyDataMapper::UploadCompositeDataProperties(
  vtkSmartPointer<vtkWebGPUConfiguration> wgpuConfiguration, bool applyOverrides,
  double overrideOpacity, const vtkColor3d& overrideAmbientColor,
  const vtkColor3d& overrideDiffuseColor, vtkTypeUInt32 composite_id, bool pickable)
{
  if (this->CompositeDataPropertiesBuffer != nullptr)
  {
    vtkTypeUInt32 applyOverrideColors = applyOverrides ? 1 : 0;
    vtkTypeFloat32 opacity = overrideOpacity;
    vtkTypeFloat32 ambient_color[3];
    vtkTypeFloat32 diffuse_color[3];
    for (int i = 0; i < 3; ++i)
    {
      ambient_color[i] = overrideAmbientColor[i];
      diffuse_color[i] = overrideDiffuseColor[i];
    }
    wgpuConfiguration->WriteBuffer(this->CompositeDataPropertiesBuffer,
      offsetof(CompositeDataProperties, ApplyOverrideColors), &applyOverrideColors,
      sizeof(vtkTypeUInt32), "CompositeDataProperties.ApplyOverrideColors");

    wgpuConfiguration->WriteBuffer(this->CompositeDataPropertiesBuffer,
      offsetof(CompositeDataProperties, Opacity), &opacity, sizeof(vtkTypeFloat32),
      "CompositeDataProperties.Opacity");

    wgpuConfiguration->WriteBuffer(this->CompositeDataPropertiesBuffer,
      offsetof(CompositeDataProperties, CompositeId), &composite_id, sizeof(vtkTypeUInt32),
      "CompositeDataProperties.CompositeId");

    wgpuConfiguration->WriteBuffer(this->CompositeDataPropertiesBuffer,
      offsetof(CompositeDataProperties, Ambient), &ambient_color, sizeof(ambient_color),
      "CompositeDataProperties.Ambient");

    wgpuConfiguration->WriteBuffer(this->CompositeDataPropertiesBuffer,
      offsetof(CompositeDataProperties, Diffuse), &diffuse_color, sizeof(diffuse_color),
      "CompositeDataProperties.Diffuse");

    vtkTypeUInt32 pickableAsUInt32 = pickable ? 1u : 0u;
    wgpuConfiguration->WriteBuffer(this->CompositeDataPropertiesBuffer,
      offsetof(CompositeDataProperties, Pickable), &pickableAsUInt32, sizeof(pickableAsUInt32),
      "CompositeDataProperties.Pickable");
  }
  this->OverrideColorUploadTimestamp.Modified();
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
  pad: u32,
  diffuse: vec3<f32>,
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
             << ") var<uniform> composite_data_properties: CompositeDataProperties;";
  vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::Custom::Bindings", codeStream.str(),
    /*all=*/false);
  vtkWebGPURenderPipelineCache::Substitute(fss, "//VTK::Custom::Bindings", codeStream.str(),
    /*all=*/false);
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
  vtkWebGPURenderPipelineCache::Substitute(fss, "//VTK::Colors::Impl",
    R"(//VTK::Colors::Impl
  if (composite_data_properties.apply_override_colors == 1u)
  {
    ambient_color = composite_data_properties.ambient.rgb;
    diffuse_color = composite_data_properties.diffuse.rgb;
    opacity = composite_data_properties.opacity;
  })",
    /*all=*/false);
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
    output.ids.x = vertex.cell_id + 1;
    output.ids.y = vertex.prop_id + 1;
    output.ids.z = vertex.composite_id + 1;
    output.ids.w = vertex.process_id + 1;
  })",
    /*all=*/true);
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
  this->CompositeDataPropertiesBuffer = nullptr;
  this->Superclass::ReleaseGraphicsResources(w);
}

VTK_ABI_NAMESPACE_END
