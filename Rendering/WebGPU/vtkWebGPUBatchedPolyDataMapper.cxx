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
#include "vtkWebGPURenderWindow.h"
#include "vtkWebGPURenderer.h"

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
  auto* wgpuActor = vtkWebGPUActor::SafeDownCast(actor);
  auto* wgpuConfiguration = wgpuRenderWindow->GetWGPUConfiguration();
  const auto& device = wgpuRenderWindow->GetDevice();

  auto& batchElement = *(this->VTKPolyDataToBatchElement.begin()->second);
  if (this->LastBlockVisibility != batchElement.Visibility)
  {
    wgpuActor->SetBundleInvalidated(true);
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

  // write to the `OverrideColorDescriptor` portion of the `MeshAttributeDescriptor` buffer only if
  // colors/opacity per block changed.
  if ((this->Parent->GetMTime() > this->OverrideColorUploadTimestamp) ||
    (this->LastUseNanColor != useNanColor))
  {
    if (useNanColor)
    {
      this->WriteOverrideColorBuffer(wgpuConfiguration, true, batchElement.Opacity,
        vtkColor3d{ nanColor }, vtkColor3d{ nanColor });
    }
    else
    {
      this->WriteOverrideColorBuffer(wgpuConfiguration, batchElement.OverridesColor,
        batchElement.Opacity, batchElement.AmbientColor, batchElement.DiffuseColor);
    }
  }
  this->LastUseNanColor = useNanColor;
}

//------------------------------------------------------------------------------
void vtkWebGPUBatchedPolyDataMapper::WriteOverrideColorBuffer(
  vtkSmartPointer<vtkWebGPUConfiguration> wgpuConfiguration, bool applyOverrides,
  double overrideOpacity, const vtkColor3d& overrideAmbientColor,
  const vtkColor3d& overrideDiffuseColor)
{
  OverrideColorDescriptor overrideColorDescriptor = {};
  overrideColorDescriptor.ApplyOverrideColors = applyOverrides ? 1 : 0;
  overrideColorDescriptor.Opacity = overrideOpacity;
  for (int i = 0; i < 3; ++i)
  {
    overrideColorDescriptor.Ambient[i] = overrideAmbientColor[i];
    overrideColorDescriptor.Diffuse[i] = overrideDiffuseColor[i];
  }
  if (this->AttributeDescriptorBuffer != nullptr)
  {
    wgpuConfiguration->WriteBuffer(this->AttributeDescriptorBuffer,
      offsetof(MeshAttributeDescriptor, OverrideColors), &overrideColorDescriptor,
      sizeof(overrideColorDescriptor));
  }
  this->OverrideColorUploadTimestamp.Modified();
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
VTK_ABI_NAMESPACE_END
