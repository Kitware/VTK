// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Hide VTK_DEPRECATED_IN_9_3_0() warnings for this class.
#define VTK_DEPRECATION_LEVEL 0
#include "vtkLegacy.h"

#include "vtkCompositePolyDataMapper.h"

#include "vtkActor.h"
#include "vtkCellData.h"
#include "vtkCommand.h"
#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositePolyDataMapperDelegator.h"
#include "vtkDataObjectTree.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkDataObjectTreeRange.h"
#include "vtkExecutive.h"
#include "vtkGarbageCollector.h"
#include "vtkInformation.h"
#include "vtkMapper.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkScalarsToColors.h"
#include "vtkTexture.h"

#include <map>
#include <stack>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkCompositePolyDataMapper);

class vtkCompositePolyDataMapper::vtkInternals
{
public:
  struct RenderBlockState
  {
    std::stack<double> Opacity;
    std::stack<bool> Visibility;
    std::stack<bool> Pickability;
    std::stack<vtkColor3d> AmbientColor;
    std::stack<vtkColor3d> DiffuseColor;
    std::stack<vtkColor3d> SpecularColor;
    std::stack<vtkColor3d> SelectionColor;
    std::stack<double> SelectionOpacity;
    std::stack<bool> ScalarVisibility;
    std::stack<bool> UseLookupTableScalarRange;
    std::stack<bool> InterpolateScalarsBeforeMapping;
    std::stack<int> ColorMode;
    std::stack<int> ScalarMode;
    std::stack<int> ArrayAccessMode;
    std::stack<int> ArrayComponent;
    std::stack<int> ArrayId;
    std::stack<std::string> ArrayName;
    std::stack<vtkIdType> FieldDataTupleId;
    std::stack<vtkVector2d> ScalarRange;
    std::stack<vtkSmartPointer<vtkScalarsToColors>> LookupTable;
  };
  RenderBlockState BlockState;

  std::vector<vtkPolyData*> RenderedList;

  /**
   * Mappers created per block along with time of creation.
   */
  std::map<vtkPolyDataMapper::MapperHashType, vtkSmartPointer<vtkCompositePolyDataMapperDelegator>>
    BatchedDelegators;
};

//------------------------------------------------------------------------------
vtkCompositePolyDataMapper::vtkCompositePolyDataMapper()
  : Internals(std::unique_ptr<vtkInternals>(new vtkInternals()))
{
}

//------------------------------------------------------------------------------
vtkCompositePolyDataMapper::~vtkCompositePolyDataMapper()
{
  this->SetPointIdArrayName(nullptr);
  this->SetCellIdArrayName(nullptr);
  this->SetProcessIdArrayName(nullptr);
  this->SetCompositeIdArrayName(nullptr);
}

//------------------------------------------------------------------------------
// Specify the type of data this mapper can handle. If we are
// working with a regular (not hierarchical) pipeline, then we
// need vtkPolyData. For composite data pipelines, then
// vtkCompositeDataSet is required, and we'll check when
// building our structure whether all the part of the composite
// data set are polydata.
int vtkCompositePolyDataMapper::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  return 1;
}

//------------------------------------------------------------------------------
vtkExecutive* vtkCompositePolyDataMapper::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}

//------------------------------------------------------------------------------
// Looks at each DataSet and finds the union of all the bounds
void vtkCompositePolyDataMapper::ComputeBounds()
{
  vtkDataObjectTree* input = vtkDataObjectTree::SafeDownCast(this->GetInputDataObject(0, 0));

  // If we don't have hierarchical data, test to see if we have
  // plain old polydata. In this case, the bounds are simply
  // the bounds of the input polydata.
  if (!input)
  {
    this->Superclass::ComputeBounds();
    return;
  }

  if (input->GetMTime() < this->BoundsMTime && this->GetMTime() < this->BoundsMTime)
  {
    return;
  }

  // computing bounds with only visible blocks
  vtkCompositeDataDisplayAttributes::ComputeVisibleBounds(
    this->CompositeAttributes, input, this->Bounds);
  this->BoundsMTime.Modified();
}

//------------------------------------------------------------------------------
double* vtkCompositePolyDataMapper::GetBounds()
{
  if (!this->GetExecutive()->GetInputData(0, 0))
  {
    vtkMath::UninitializeBounds(this->Bounds);
    return this->Bounds;
  }
  else
  {
    this->Update();

    // only compute bounds when the input data has changed
    vtkCompositeDataPipeline* executive =
      vtkCompositeDataPipeline::SafeDownCast(this->GetExecutive());
    if (executive->GetPipelineMTime() > this->BoundsMTime.GetMTime())
    {
      this->ComputeBounds();
    }

    return this->Bounds;
  }
}

//------------------------------------------------------------------------------
void vtkCompositePolyDataMapper::ShallowCopy(vtkAbstractMapper* mapper)
{
  vtkCompositePolyDataMapper* cpdm = vtkCompositePolyDataMapper::SafeDownCast(mapper);
  if (cpdm != nullptr)
  {
    this->SetCompositeDataDisplayAttributes(cpdm->GetCompositeDataDisplayAttributes());
    this->SetColorMissingArraysWithNanColor(cpdm->GetColorMissingArraysWithNanColor());
    this->SetCellIdArrayName(cpdm->GetCellIdArrayName());
    this->SetCompositeIdArrayName(cpdm->GetCompositeIdArrayName());
    this->SetPointIdArrayName(cpdm->GetPointIdArrayName());
    this->SetProcessIdArrayName(cpdm->GetProcessIdArrayName());
  }
  // Now do superclass
  this->vtkPolyDataMapper::ShallowCopy(mapper);
}

//------------------------------------------------------------------------------
void vtkCompositePolyDataMapper::ReleaseGraphicsResources(vtkWindow* win)
{
  auto& internals = (*this->Internals);
  for (auto& iter : internals.BatchedDelegators)
  {
    auto& delegator = iter.second;
    delegator->GetDelegate()->ReleaseGraphicsResources(win);
  }
  internals.BatchedDelegators.clear();
  this->Modified();
  this->Superclass::ReleaseGraphicsResources(win);
}

//------------------------------------------------------------------------------
void vtkCompositePolyDataMapper::SetVBOShiftScaleMethod(int method)
{
  if (this->ShiftScaleMethod == method)
  {
    return;
  }
  this->ShiftScaleMethod = method;

  this->Superclass::SetVBOShiftScaleMethod(method);

  auto& internals = (*this->Internals);
  for (auto& iter : internals.BatchedDelegators)
  {
    auto& delegator = iter.second;
    delegator->GetDelegate()->SetVBOShiftScaleMethod(method);
  }
}

//------------------------------------------------------------------------------
void vtkCompositePolyDataMapper::SetPauseShiftScale(bool pauseShiftScale)
{
  if (pauseShiftScale == this->PauseShiftScale)
  {
    return;
  }

  this->Superclass::SetPauseShiftScale(pauseShiftScale);
  auto& internals = (*this->Internals);
  for (auto& iter : internals.BatchedDelegators)
  {
    auto& delegator = iter.second;
    delegator->GetDelegate()->SetPauseShiftScale(pauseShiftScale);
  }
}

//------------------------------------------------------------------------------
void vtkCompositePolyDataMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkCompositePolyDataMapperDelegator* vtkCompositePolyDataMapper::CreateADelegator()
{
  auto delegator = vtkCompositePolyDataMapperDelegator::New();
  return delegator;
}

//------------------------------------------------------------------------------
vtkDataObjectTreeIterator* vtkCompositePolyDataMapper::MakeAnIterator(vtkCompositeDataSet* dataset)
{
  auto iter = vtkDataObjectTreeIterator::New();
  iter->SetDataSet(dataset);
  iter->SkipEmptyNodesOn();
  iter->VisitOnlyLeavesOn();
  return iter;
}

//------------------------------------------------------------------------------
// simple tests, the mapper is tolerant of being
// called both on opaque and translucent
bool vtkCompositePolyDataMapper::HasOpaqueGeometry()
{
  return true;
}

//------------------------------------------------------------------------------
// look at children
bool vtkCompositePolyDataMapper::HasTranslucentPolygonalGeometry()
{
  // Make sure that we have been properly initialized.
  if (this->GetInputAlgorithm() == nullptr)
  {
    return false;
  }

  if (!this->Static)
  {
    this->InvokeEvent(vtkCommand::StartEvent, nullptr);
    this->GetInputAlgorithm()->Update();
    this->InvokeEvent(vtkCommand::EndEvent, nullptr);
  }

  auto input = this->GetInputDataObject(0, 0);
  if (input == nullptr)
  {
    return false;
  }

  // rebuild the render values if needed
  vtkCompositeDataDisplayAttributes* cda = this->GetCompositeDataDisplayAttributes();
  vtkScalarsToColors* lut = this->ScalarVisibility ? this->GetLookupTable() : nullptr;

  this->TempState.Clear();
  this->TempState.Append(cda ? cda->GetMTime() : 0, "cda mtime");
  this->TempState.Append(lut ? lut->GetMTime() : 0, "lut mtime");
  this->TempState.Append(input->GetMTime(), "input mtime");
  if (this->TranslucentState != this->TempState)
  {
    this->TranslucentState = this->TempState;
    if (lut)
    {
      // Ensure that the lookup table is built
      lut->Build();
    }

    // Push base-values on the state stack.
    unsigned int flat_index = 0;
    this->HasTranslucentGeometry = this->RecursiveHasTranslucentGeometry(input, flat_index);
  }

  return this->HasTranslucentGeometry;
}

//------------------------------------------------------------------------------
void vtkCompositePolyDataMapper::Render(vtkRenderer* renderer, vtkActor* actor)
{
  auto& internals = (*this->Internals);
  internals.RenderedList.clear();
  // Make sure that we have been properly initialized.
  if (renderer->GetRenderWindow()->CheckAbortStatus())
  {
    return;
  }
  if (this->GetInputAlgorithm() == nullptr)
  {
    return;
  }
  if (!this->Static)
  {
    this->InvokeEvent(vtkCommand::StartEvent, nullptr);
    this->GetInputAlgorithm()->Update();
    this->InvokeEvent(vtkCommand::EndEvent, nullptr);
  }
  auto input = this->GetInputDataObject(0, 0);
  if (input == nullptr)
  {
    vtkErrorMacro(<< "No input!");
  }

  // The first step is to gather up the polydata based on their
  // signatures (aka have normals, have scalars etc)
  // At a high-level, the following code visits every polydata in this composite dataset,
  // creates/reuses an existing polydata mapper based on a hash string.
  if (this->DelegatorMTime < input->GetMTime() || this->DelegatorMTime < this->GetMTime())
  {
    this->DelegatorMTime.Modified();
  }

  // rebuild the render values if needed.
  this->TempState.Clear();
  this->TempState.Append(actor->GetProperty()->GetMTime(), "actor mtime");
  this->TempState.Append(this->GetMTime(), "this mtime");
  this->TempState.Append(this->DelegatorMTime, "delegator mtime");
  this->TempState.Append(
    actor->GetTexture() ? actor->GetTexture()->GetMTime() : 0, "texture mtime");

  if (this->RenderValuesState != this->TempState)
  {
    this->RenderValuesState = this->TempState;
    auto property = actor->GetProperty();
    if (auto lut = this->GetLookupTable())
    {
      lut->Build();
    }
    auto selColor = property->GetSelectionColor();

    // unmark old delegators
    for (auto& iter : internals.BatchedDelegators)
    {
      auto& delegator = iter.second;
      delegator->Unmark();
    }

    // Push base-values on the state stack.
    internals.BlockState.Visibility.push(true);
    internals.BlockState.Pickability.push(true);
    internals.BlockState.Opacity.push(property->GetOpacity());
    internals.BlockState.AmbientColor.emplace(property->GetAmbientColor());
    internals.BlockState.DiffuseColor.emplace(property->GetDiffuseColor());
    internals.BlockState.SpecularColor.emplace(property->GetSpecularColor());
    internals.BlockState.SelectionColor.emplace(selColor);
    internals.BlockState.SelectionOpacity.push(selColor[3]);
    internals.BlockState.ScalarMode.push(this->ScalarMode);
    internals.BlockState.ArrayAccessMode.push(this->ArrayAccessMode);
    internals.BlockState.ArrayComponent.push(this->ArrayComponent);
    internals.BlockState.ArrayId.push(this->ArrayId);
    internals.BlockState.ArrayName.emplace(this->ArrayName);
    internals.BlockState.FieldDataTupleId.push(this->FieldDataTupleId);
    internals.BlockState.ScalarVisibility.push(this->ScalarVisibility);
    internals.BlockState.UseLookupTableScalarRange.push(this->UseLookupTableScalarRange);
    internals.BlockState.InterpolateScalarsBeforeMapping.push(
      this->InterpolateScalarsBeforeMapping);
    internals.BlockState.ColorMode.push(this->ColorMode);
    internals.BlockState.ScalarRange.emplace(this->ScalarRange[0], this->ScalarRange[1]);
    internals.BlockState.LookupTable.push(this->GetLookupTable());

    {
      unsigned int flatIndex = 0;
      this->BuildRenderValues(renderer, actor, input, flatIndex);
    }

    // Pop base-values from the state stack.
    internals.BlockState.Visibility.pop();
    internals.BlockState.Pickability.pop();
    internals.BlockState.Opacity.pop();
    internals.BlockState.AmbientColor.pop();
    internals.BlockState.DiffuseColor.pop();
    internals.BlockState.SpecularColor.pop();
    internals.BlockState.SelectionColor.pop();
    internals.BlockState.SelectionOpacity.pop();
    internals.BlockState.ScalarMode.pop();
    internals.BlockState.ArrayAccessMode.pop();
    internals.BlockState.ArrayComponent.pop();
    internals.BlockState.ArrayId.pop();
    internals.BlockState.ArrayName.pop();
    internals.BlockState.FieldDataTupleId.pop();
    internals.BlockState.ScalarVisibility.pop();
    internals.BlockState.UseLookupTableScalarRange.pop();
    internals.BlockState.InterpolateScalarsBeforeMapping.pop();
    internals.BlockState.ColorMode.pop();
    internals.BlockState.ScalarRange.pop();
    internals.BlockState.LookupTable.pop();

    // delete unused old helpers/data
    for (auto iter = internals.BatchedDelegators.begin();
         iter != internals.BatchedDelegators.end();)
    {
      iter->second->ClearUnmarkedBatchElements();
      if (!iter->second->GetMarked())
      {
        iter->second->GetDelegate()->ReleaseGraphicsResources(renderer->GetVTKWindow());
        internals.BatchedDelegators.erase(iter++);
      }
      else
      {
        ++iter;
      }
    }
  }

  std::vector<vtkSmartPointer<vtkCompositePolyDataMapperDelegator>> delegators;
  delegators.reserve(internals.BatchedDelegators.size());
  for (const auto& pair : internals.BatchedDelegators)
  {
    delegators.emplace_back(pair.second);
  }
  this->PreRender(delegators, renderer, actor);
  for (auto& iter : internals.BatchedDelegators)
  {
    auto& delegator = iter.second;
    delegator->GetDelegate()->RenderPiece(renderer, actor);

    for (auto& polydata : delegator->GetRenderedList())
    {
      internals.RenderedList.emplace_back(polydata);
    }
  }
  this->PostRender(delegators, renderer, actor);
}

//------------------------------------------------------------------------------
vtkCompositePolyDataMapper::MapperHashType vtkCompositePolyDataMapper::InsertPolyData(
  vtkPolyData* polydata, const unsigned int& flatIndex)
{
  if (polydata == nullptr)
  {
    vtkDebugMacro(<< "DataObject at flatIndex=" << flatIndex
                  << " is not a vtkPolyData or a vtkPolyData derived instance!");
    return std::numeric_limits<MapperHashType>::max();
  }
  if (polydata->GetPoints() == nullptr || !polydata->GetNumberOfPoints())
  {
    vtkDebugMacro(<< "vtkPolyData at flatIndex=" << flatIndex
                  << " does not have points. It will not be rendered.");
    return std::numeric_limits<MapperHashType>::max();
  }
  auto& internals = (*this->Internals);
  const auto hash = this->GenerateHash(polydata);
  // Find a mapper. If it doesn't exist, a new one is created.
  internals.BatchedDelegators.emplace(hash, nullptr);
  auto& delegator = internals.BatchedDelegators.at(hash);
  if (delegator == nullptr)
  {
    delegator.TakeReference(this->CreateADelegator());
    delegator->SetParent(this);
  }
  delegator->ShallowCopy(this);
  delegator->Mark();

  auto createBatchElement =
    [](vtkPolyData* _polydata,
      unsigned int _flatIndex) -> vtkCompositePolyDataMapperDelegator::BatchElement {
    vtkCompositePolyDataMapperDelegator::BatchElement element;
    element.PolyData = _polydata;
    element.FlatIndex = _flatIndex;
    return element;
  };

  delegator->Insert(createBatchElement(polydata, flatIndex));
  return hash;
}

//------------------------------------------------------------------------------
void vtkCompositePolyDataMapper::BuildRenderValues(
  vtkRenderer* renderer, vtkActor* actor, vtkDataObject* dobj, unsigned int& flatIndex)
{
  auto& internals = (*this->Internals);
  // Push overridden attributes onto the stack.
  // Keep track of attributes that were pushed so that they can be popped after they're applied to
  // the batch element.
  vtkCompositeDataDisplayAttributes* cda = this->GetCompositeDataDisplayAttributes();
  bool overrides_visibility = (cda && cda->HasBlockVisibility(dobj));
  if (overrides_visibility)
  {
    internals.BlockState.Visibility.push(cda->GetBlockVisibility(dobj));
  }
  bool overrides_pickability = (cda && cda->HasBlockPickability(dobj));
  if (overrides_pickability)
  {
    internals.BlockState.Pickability.push(cda->GetBlockPickability(dobj));
  }

  bool overrides_opacity = (cda && cda->HasBlockOpacity(dobj));
  if (overrides_opacity)
  {
    internals.BlockState.Opacity.push(cda->GetBlockOpacity(dobj));
  }

  bool overrides_color = (cda && cda->HasBlockColor(dobj));
  if (overrides_color)
  {
    vtkColor3d color = cda->GetBlockColor(dobj);
    internals.BlockState.AmbientColor.push(color);
    internals.BlockState.DiffuseColor.push(color);
    internals.BlockState.SpecularColor.push(color);
  }

  bool overrides_scalar_mode = (cda && cda->HasBlockScalarMode(dobj));
  if (overrides_scalar_mode)
  {
    internals.BlockState.ScalarMode.push(cda->GetBlockScalarMode(dobj));
  }

  bool overrides_scalar_array_access_mode = (cda && cda->HasBlockArrayAccessMode(dobj));
  if (overrides_scalar_array_access_mode)
  {
    internals.BlockState.ArrayAccessMode.push(cda->GetBlockArrayAccessMode(dobj));
  }

  bool overrides_scalar_array_component = (cda && cda->HasBlockArrayComponent(dobj));
  if (overrides_scalar_array_component)
  {
    internals.BlockState.ArrayComponent.push(cda->GetBlockArrayComponent(dobj));
  }

  bool overrides_scalar_array_id = (cda && cda->HasBlockArrayId(dobj));
  if (overrides_scalar_array_id)
  {
    internals.BlockState.ArrayId.push(cda->GetBlockArrayId(dobj));
  }

  bool overrides_field_tuple_id = (cda && cda->HasBlockFieldDataTupleId(dobj));
  if (overrides_field_tuple_id)
  {
    internals.BlockState.FieldDataTupleId.push(cda->GetBlockFieldDataTupleId(dobj));
  }

  bool overrides_scalar_array_name = (cda && cda->HasBlockArrayName(dobj));
  if (overrides_scalar_array_name)
  {
    internals.BlockState.ArrayName.push(cda->GetBlockArrayName(dobj));
  }

  bool overrides_scalar_visibility = (cda && cda->HasBlockScalarVisibility(dobj));
  if (overrides_scalar_visibility)
  {
    internals.BlockState.ScalarVisibility.push(cda->GetBlockScalarVisibility(dobj));
  }

  bool overrides_use_lookup_table_scalar_range =
    (cda && cda->HasBlockUseLookupTableScalarRange(dobj));
  if (overrides_use_lookup_table_scalar_range)
  {
    internals.BlockState.UseLookupTableScalarRange.push(
      cda->GetBlockUseLookupTableScalarRange(dobj));
  }

  bool overrides_interpolate_scalars_before_mapping =
    (cda && cda->HasBlockInterpolateScalarsBeforeMapping(dobj));
  if (overrides_interpolate_scalars_before_mapping)
  {
    internals.BlockState.InterpolateScalarsBeforeMapping.push(
      cda->GetBlockInterpolateScalarsBeforeMapping(dobj));
  }

  bool overrides_color_mode = (cda && cda->HasBlockColorMode(dobj));
  if (overrides_color_mode)
  {
    internals.BlockState.ColorMode.push(cda->GetBlockColorMode(dobj));
  }

  bool overrides_scalar_range = (cda && cda->HasBlockScalarRange(dobj));
  if (overrides_scalar_range)
  {
    internals.BlockState.ScalarRange.push(cda->GetBlockScalarRange(dobj));
  }

  bool overrides_lookup_table = (cda && cda->HasBlockLookupTable(dobj));
  if (overrides_lookup_table)
  {
    internals.BlockState.LookupTable.push(cda->GetBlockLookupTable(dobj));
  }

  // Advance flat-index. After this point, flatIndex no longer points to this
  // block.
  const auto originalFlatIndex = flatIndex;
  flatIndex++;

  bool textureOpaque = true;
  if (actor->GetTexture() != nullptr && actor->GetTexture()->IsTranslucent())
  {
    textureOpaque = false;
  }

  if (auto dObjTree = vtkDataObjectTree::SafeDownCast(dobj))
  {
    using Opts = vtk::DataObjectTreeOptions;
    for (vtkDataObject* child : vtk::Range(dObjTree, Opts::None))
    {
      if (!child)
      {
        ++flatIndex;
      }
      else
      {
        this->BuildRenderValues(renderer, actor, child, flatIndex);
      }
    }
  }
  else if (auto polydata = vtkPolyData::SafeDownCast(dobj))
  {
    // The prototype mapper is a placeholder mapper that doesn't have inputs. It relies on object
    // factory overrides to facilitate hash computation using the underlying graphics implementation
    // of vtkPolydataMapper. Prepare the prototype mapper with exact scalar mapping attributes, so
    // that hash computation is as accurate as possible.
    this->PrototypeMapper->SetScalarMode(internals.BlockState.ScalarMode.top());
    this->PrototypeMapper->SetArrayAccessMode(internals.BlockState.ArrayAccessMode.top());
    this->PrototypeMapper->SetArrayComponent(internals.BlockState.ArrayComponent.top());
    this->PrototypeMapper->SetArrayId(internals.BlockState.ArrayId.top());
    this->PrototypeMapper->SetArrayName(internals.BlockState.ArrayName.top().c_str());
    this->PrototypeMapper->SetFieldDataTupleId(internals.BlockState.FieldDataTupleId.top());
    this->PrototypeMapper->SetScalarVisibility(internals.BlockState.ScalarVisibility.top());
    this->PrototypeMapper->SetColorMode(internals.BlockState.ColorMode.top());
    this->PrototypeMapper->SetUseLookupTableScalarRange(
      internals.BlockState.UseLookupTableScalarRange.top());
    this->PrototypeMapper->SetInterpolateScalarsBeforeMapping(
      internals.BlockState.InterpolateScalarsBeforeMapping.top());
    this->PrototypeMapper->SetScalarRange(internals.BlockState.ScalarRange.top().GetData());
    this->PrototypeMapper->SetLookupTable(internals.BlockState.LookupTable.top());

    const auto hash = this->InsertPolyData(polydata, originalFlatIndex);
    if (hash == std::numeric_limits<MapperHashType>::max())
    {
      return;
    }
    vtkDebugMacro(<< "Inserted " << polydata << " at " << hash);
    const auto& delegator = internals.BatchedDelegators[hash];
    // because it was incremented few lines above.
    if (auto inputItem = delegator->Get(polydata))
    {
      // Capture the display attributes in the batch element.
      inputItem->Opacity = internals.BlockState.Opacity.top();
      inputItem->Visibility = internals.BlockState.Visibility.top();
      inputItem->Pickability = internals.BlockState.Pickability.top();
      inputItem->AmbientColor = internals.BlockState.AmbientColor.top();
      inputItem->DiffuseColor = internals.BlockState.DiffuseColor.top();
      inputItem->SelectionColor = internals.BlockState.SelectionColor.top();
      inputItem->SelectionOpacity = internals.BlockState.SelectionOpacity.top();
      inputItem->OverridesColor = (internals.BlockState.AmbientColor.size() > 1);
      inputItem->IsOpaque = (inputItem->Opacity >= 1.0) ? textureOpaque : false;
      inputItem->ScalarMode = internals.BlockState.ScalarMode.top();
      inputItem->ArrayAccessMode = internals.BlockState.ArrayAccessMode.top();
      inputItem->ArrayComponent = internals.BlockState.ArrayComponent.top();
      inputItem->ArrayId = internals.BlockState.ArrayId.top();
      inputItem->ArrayName = internals.BlockState.ArrayName.top();
      inputItem->FieldDataTupleId = internals.BlockState.FieldDataTupleId.top();
      inputItem->ScalarVisibility = internals.BlockState.ScalarVisibility.top();
      inputItem->ColorMode = internals.BlockState.ColorMode.top();
      inputItem->UseLookupTableScalarRange = internals.BlockState.UseLookupTableScalarRange.top();
      inputItem->InterpolateScalarsBeforeMapping =
        internals.BlockState.InterpolateScalarsBeforeMapping.top();
      inputItem->ScalarRange.Set(
        internals.BlockState.ScalarRange.top()[0], internals.BlockState.ScalarRange.top()[1]);
      inputItem->LookupTable = internals.BlockState.LookupTable.top();

      // Apply these on the delegate. These attributes are batch invariants.
      auto delegate = delegator->GetDelegate();
      delegate->SetInterpolateScalarsBeforeMapping(inputItem->InterpolateScalarsBeforeMapping);
      delegate->SetLookupTable(inputItem->LookupTable);

      // if we think it is opaque check the scalars
      if (inputItem->IsOpaque && inputItem->ScalarVisibility) // inputItem->ScalarVisibility
      {
        vtkScalarsToColors* lut = inputItem->LookupTable; // inputItem->LookupTable
        // ensure table is built
        lut->Build();
        int cellFlag;
        vtkDataArray* scalars =
          vtkCompositePolyDataMapper::GetScalars(polydata, inputItem->ScalarMode,
            inputItem->ArrayAccessMode, inputItem->ArrayId, inputItem->ArrayName.c_str(), cellFlag);

        unsigned char ghostsToSkip;
        vtkUnsignedCharArray* ghosts =
          vtkAbstractMapper::GetGhostArray(polydata, inputItem->ScalarMode, ghostsToSkip);

        if (!lut->IsOpaque(scalars, inputItem->ColorMode, inputItem->ArrayComponent, ghosts,
              ghostsToSkip)) // inputItem->ColorMode
        {
          inputItem->IsOpaque = false;
        }
      }
    }
  }
  else
  {
    vtkErrorMacro(<< "Expected a vtkDataObjectTree or vtkPolyData input. Got "
                  << dobj->GetClassName());
  }
  // Pop overridden attributes from the stack.
  if (overrides_scalar_mode)
  {
    internals.BlockState.ScalarMode.pop();
  }
  if (overrides_scalar_array_access_mode)
  {
    internals.BlockState.ArrayAccessMode.pop();
  }
  if (overrides_scalar_array_component)
  {
    internals.BlockState.ArrayComponent.pop();
  }
  if (overrides_scalar_array_id)
  {
    internals.BlockState.ArrayId.pop();
  }
  if (overrides_field_tuple_id)
  {
    internals.BlockState.FieldDataTupleId.pop();
  }
  if (overrides_scalar_array_name)
  {
    internals.BlockState.ArrayName.pop();
  }
  if (overrides_color)
  {
    internals.BlockState.AmbientColor.pop();
    internals.BlockState.DiffuseColor.pop();
    internals.BlockState.SpecularColor.pop();
  }
  if (overrides_opacity)
  {
    internals.BlockState.Opacity.pop();
  }
  if (overrides_pickability)
  {
    internals.BlockState.Pickability.pop();
  }
  if (overrides_visibility)
  {
    internals.BlockState.Visibility.pop();
  }
  if (overrides_scalar_visibility)
  {
    internals.BlockState.ScalarVisibility.pop();
  }
  if (overrides_color_mode)
  {
    internals.BlockState.ColorMode.pop();
  }
  if (overrides_use_lookup_table_scalar_range)
  {
    internals.BlockState.UseLookupTableScalarRange.pop();
  }
  if (overrides_interpolate_scalars_before_mapping)
  {
    internals.BlockState.InterpolateScalarsBeforeMapping.pop();
  }
  if (overrides_scalar_range)
  {
    internals.BlockState.ScalarRange.pop();
  }
  if (overrides_lookup_table)
  {
    internals.BlockState.LookupTable.pop();
  }
}

//------------------------------------------------------------------------------
bool vtkCompositePolyDataMapper::RecursiveHasTranslucentGeometry(
  vtkDataObject* dobj, unsigned int& flat_index)
{
  vtkCompositeDataDisplayAttributes* cda = this->GetCompositeDataDisplayAttributes();
  bool overrides_opacity = (cda && cda->HasBlockOpacity(dobj));
  if (overrides_opacity)
  {
    if (cda->GetBlockOpacity(dobj) < 1.0)
    {
      return true;
    }
  }

  // Advance flat-index. After this point, flat_index no longer points to this
  // block.
  flat_index++;

  if (auto dObjTree = vtkDataObjectTree::SafeDownCast(dobj))
  {
    using Opts = vtk::DataObjectTreeOptions;
    for (vtkDataObject* child : vtk::Range(dObjTree, Opts::None))
    {
      if (!child)
      {
        ++flat_index;
      }
      else
      {
        if (this->RecursiveHasTranslucentGeometry(child, flat_index))
        {
          return true;
        }
      }
    }
    return false;
  }
  else
  {
    bool overrides_visibility = (cda && cda->HasBlockVisibility(dobj));
    if (overrides_visibility)
    {
      if (!cda->GetBlockVisibility(dobj))
      {
        return false;
      }
    }
    int scalarMode = this->ScalarMode;
    int arrayAccessMode = this->ArrayAccessMode;
    int arrayComponent = this->ArrayComponent;
    int arrayId = this->ArrayId;
    std::string arrayName = this->ArrayName;
    bool scalarVisibility = this->ScalarVisibility;
    int colorMode = this->ColorMode;
    vtkScalarsToColors* lut = this->GetLookupTable();

    bool overrides_scalar_mode = (cda && cda->HasBlockArrayAccessMode(dobj));
    if (overrides_scalar_mode)
    {
      scalarMode = cda->GetBlockArrayAccessMode(dobj);
    }

    bool overrides_scalar_array_access_mode = (cda && cda->HasBlockArrayAccessMode(dobj));
    if (overrides_scalar_array_access_mode)
    {
      arrayAccessMode = cda->GetBlockArrayAccessMode(dobj);
    }

    bool overrides_scalar_array_component = (cda && cda->HasBlockArrayComponent(dobj));
    if (overrides_scalar_array_component)
    {
      arrayComponent = cda->GetBlockArrayComponent(dobj);
    }

    bool overrides_scalar_array_id = (cda && cda->HasBlockArrayId(dobj));
    if (overrides_scalar_array_id)
    {
      arrayId = cda->GetBlockArrayId(dobj);
    }

    bool overrides_scalar_array_name = (cda && cda->HasBlockArrayName(dobj));
    if (overrides_scalar_array_name)
    {
      arrayName = cda->GetBlockArrayName(dobj);
    }

    bool overrides_scalar_visibility = (cda && cda->HasBlockScalarVisibility(dobj));
    if (overrides_scalar_visibility)
    {
      scalarVisibility = cda->GetBlockScalarVisibility(dobj);
    }

    bool overrides_color_mode = (cda && cda->HasBlockColorMode(dobj));
    if (overrides_color_mode)
    {
      colorMode = cda->GetBlockColorMode(dobj);
    }

    bool overrides_lookup_table = (cda && cda->HasBlockLookupTable(dobj));
    if (overrides_lookup_table)
    {
      lut = cda->GetBlockLookupTable(dobj);
      lut->Build();
    }

    vtkPolyData* pd = vtkPolyData::SafeDownCast(dobj);

    // if we think it is opaque check the scalars
    if (scalarVisibility)
    {
      int cellFlag;
      vtkDataArray* scalars = vtkCompositePolyDataMapper::GetScalars(
        pd, scalarMode, arrayAccessMode, arrayId, arrayName.c_str(), cellFlag);

      unsigned char ghostsToSkip;
      vtkUnsignedCharArray* ghosts = vtkAbstractMapper::GetGhostArray(pd, scalarMode, ghostsToSkip);

      if (lut->IsOpaque(scalars, colorMode, arrayComponent, ghosts, ghostsToSkip) == 0)
      {
        return true;
      }
    }
  }

  return false;
}

//------------------------------------------------------------------------------
vtkPolyDataMapper::MapperHashType vtkCompositePolyDataMapper::GenerateHash(vtkPolyData* polydata)
{
  return this->PrototypeMapper->GenerateHash(polydata);
}

//------------------------------------------------------------------------------
void vtkCompositePolyDataMapper::SetCompositeDataDisplayAttributes(
  vtkCompositeDataDisplayAttributes* attributes)
{
  if (this->CompositeAttributes != attributes)
  {
    this->CompositeAttributes = attributes;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
vtkCompositeDataDisplayAttributes* vtkCompositePolyDataMapper::GetCompositeDataDisplayAttributes()
{
  return this->CompositeAttributes;
}

//------------------------------------------------------------------------------
void vtkCompositePolyDataMapper::SetBlockVisibility(unsigned int index, bool visible)
{
  if (this->CompositeAttributes)
  {
    auto dataObj =
      vtkCompositeDataDisplayAttributes::DataObjectFromIndex(index, this->GetInputDataObject(0, 0));
    if (dataObj)
    {
      this->CompositeAttributes->SetBlockVisibility(dataObj, visible);
      this->Modified();
    }
  }
}

//------------------------------------------------------------------------------
bool vtkCompositePolyDataMapper::GetBlockVisibility(unsigned int index)
{
  if (this->CompositeAttributes)
  {
    auto dataObj =
      vtkCompositeDataDisplayAttributes::DataObjectFromIndex(index, this->GetInputDataObject(0, 0));
    if (dataObj)
    {
      return this->CompositeAttributes->GetBlockVisibility(dataObj);
    }
  }

  return true;
}

//------------------------------------------------------------------------------
void vtkCompositePolyDataMapper::RemoveBlockVisibility(unsigned int index)
{
  if (this->CompositeAttributes)
  {
    auto dataObj =
      vtkCompositeDataDisplayAttributes::DataObjectFromIndex(index, this->GetInputDataObject(0, 0));
    if (dataObj)
    {
      this->CompositeAttributes->RemoveBlockVisibility(dataObj);
      this->Modified();
    }
  }
}

//------------------------------------------------------------------------------
void vtkCompositePolyDataMapper::RemoveBlockVisibilities()
{
  if (this->CompositeAttributes)
  {
    this->CompositeAttributes->RemoveBlockVisibilities();
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkCompositePolyDataMapper::SetBlockColor(unsigned int index, const double color[3])
{
  if (this->CompositeAttributes)
  {
    auto dataObj =
      vtkCompositeDataDisplayAttributes::DataObjectFromIndex(index, this->GetInputDataObject(0, 0));

    if (dataObj)
    {
      this->CompositeAttributes->SetBlockColor(dataObj, color);
      this->Modified();
    }
  }
}

//------------------------------------------------------------------------------
void vtkCompositePolyDataMapper::GetBlockColor(unsigned int index, double color[3])
{
  if (this->CompositeAttributes)
  {
    unsigned int start_index = 0;
    auto dataObj = vtkCompositeDataDisplayAttributes::DataObjectFromIndex(
      index, this->GetInputDataObject(0, 0), start_index);
    if (dataObj)
    {
      this->CompositeAttributes->GetBlockColor(dataObj, color);
    }
  }
  else
  {
    color[0] = 1.0;
    color[1] = 1.0;
    color[2] = 1.0;
  }
}

//------------------------------------------------------------------------------
double* vtkCompositePolyDataMapper::GetBlockColor(unsigned int index)
{
  VTK_LEGACY_REPLACED_BODY(double* vtkCompositePolyDataMapper::GetBlockColor(unsigned int index),
    "VTK 9.3", void vtkCompositePolyDataMapper::GetBlockColor(unsigned int index, double color[3]));
  static double white[3] = { 1.0, 1.0, 1.0 };

  if (this->CompositeAttributes)
  {
    unsigned int start_index = 0;
    auto dataObj = vtkCompositeDataDisplayAttributes::DataObjectFromIndex(
      index, this->GetInputDataObject(0, 0), start_index);
    if (dataObj)
    {
      this->CompositeAttributes->GetBlockColor(dataObj, this->ColorResult.data());
    }

    return this->ColorResult.data();
  }
  else
  {
    return white;
  }
}

//------------------------------------------------------------------------------
void vtkCompositePolyDataMapper::RemoveBlockColor(unsigned int index)
{
  if (this->CompositeAttributes)
  {
    unsigned int start_index = 0;
    auto dataObj = vtkCompositeDataDisplayAttributes::DataObjectFromIndex(
      index, this->GetInputDataObject(0, 0), start_index);
    if (dataObj)
    {
      this->CompositeAttributes->RemoveBlockColor(dataObj);
      this->Modified();
    }
  }
}

//------------------------------------------------------------------------------
void vtkCompositePolyDataMapper::RemoveBlockColors()
{
  if (this->CompositeAttributes)
  {
    this->CompositeAttributes->RemoveBlockColors();
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkCompositePolyDataMapper::SetBlockOpacity(unsigned int index, double opacity)
{
  if (this->CompositeAttributes)
  {
    unsigned int start_index = 0;
    auto dataObj = vtkCompositeDataDisplayAttributes::DataObjectFromIndex(
      index, this->GetInputDataObject(0, 0), start_index);
    if (dataObj)
    {
      this->CompositeAttributes->SetBlockOpacity(dataObj, opacity);
      this->Modified();
    }
  }
}

//------------------------------------------------------------------------------
double vtkCompositePolyDataMapper::GetBlockOpacity(unsigned int index)
{
  if (this->CompositeAttributes)
  {
    unsigned int start_index = 0;
    auto dataObj = vtkCompositeDataDisplayAttributes::DataObjectFromIndex(
      index, this->GetInputDataObject(0, 0), start_index);
    if (dataObj)
    {
      return this->CompositeAttributes->GetBlockOpacity(dataObj);
    }
  }
  return 1.;
}

//------------------------------------------------------------------------------
void vtkCompositePolyDataMapper::RemoveBlockOpacity(unsigned int index)
{
  if (this->CompositeAttributes)
  {
    unsigned int start_index = 0;
    auto dataObj = vtkCompositeDataDisplayAttributes::DataObjectFromIndex(
      index, this->GetInputDataObject(0, 0), start_index);
    if (dataObj)
    {
      this->CompositeAttributes->RemoveBlockOpacity(dataObj);
      this->Modified();
    }
  }
}

//------------------------------------------------------------------------------
void vtkCompositePolyDataMapper::RemoveBlockOpacities()
{
  if (this->CompositeAttributes)
  {
    this->CompositeAttributes->RemoveBlockOpacities();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkCompositePolyDataMapper::SetBlockScalarMode(unsigned int index, int value)
{
  if (!this->CompositeAttributes)
  {
    return;
  }
  unsigned int start_index = 0;
  if (auto dataObj = vtkCompositeDataDisplayAttributes::DataObjectFromIndex(
        index, this->GetInputDataObject(0, 0), start_index))
  {
    this->CompositeAttributes->SetBlockScalarMode(dataObj, value);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int vtkCompositePolyDataMapper::GetBlockScalarMode(unsigned int index)
{
  if (!this->CompositeAttributes)
  {
    return VTK_SCALAR_MODE_DEFAULT;
  }
  unsigned int start_index = 0;
  if (auto dataObj = vtkCompositeDataDisplayAttributes::DataObjectFromIndex(
        index, this->GetInputDataObject(0, 0), start_index))
  {
    return this->CompositeAttributes->GetBlockScalarMode(dataObj);
  }
  return VTK_SCALAR_MODE_DEFAULT;
}

//----------------------------------------------------------------------------
void vtkCompositePolyDataMapper::RemoveBlockScalarMode(unsigned int index)
{
  if (!this->CompositeAttributes)
  {
    return;
  }
  unsigned int start_index = 0;
  if (auto dataObj = vtkCompositeDataDisplayAttributes::DataObjectFromIndex(
        index, this->GetInputDataObject(0, 0), start_index))
  {
    this->CompositeAttributes->RemoveBlockScalarMode(dataObj);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkCompositePolyDataMapper::RemoveBlockScalarModes()
{
  if (!this->CompositeAttributes)
  {
    return;
  }
  this->CompositeAttributes->RemoveBlockScalarModes();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkCompositePolyDataMapper::SetBlockArrayAccessMode(unsigned int index, int value)
{
  if (!this->CompositeAttributes)
  {
    return;
  }
  unsigned int start_index = 0;
  if (auto dataObj = vtkCompositeDataDisplayAttributes::DataObjectFromIndex(
        index, this->GetInputDataObject(0, 0), start_index))
  {
    this->CompositeAttributes->SetBlockArrayAccessMode(dataObj, value);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int vtkCompositePolyDataMapper::GetBlockArrayAccessMode(unsigned int index)
{
  if (!this->CompositeAttributes)
  {
    return VTK_GET_ARRAY_BY_ID;
  }
  unsigned int start_index = 0;
  if (auto dataObj = vtkCompositeDataDisplayAttributes::DataObjectFromIndex(
        index, this->GetInputDataObject(0, 0), start_index))
  {
    return this->CompositeAttributes->GetBlockArrayAccessMode(dataObj);
  }
  return VTK_GET_ARRAY_BY_ID;
}

//----------------------------------------------------------------------------
void vtkCompositePolyDataMapper::RemoveBlockArrayAccessMode(unsigned int index)
{
  if (!this->CompositeAttributes)
  {
    return;
  }
  unsigned int start_index = 0;
  if (auto dataObj = vtkCompositeDataDisplayAttributes::DataObjectFromIndex(
        index, this->GetInputDataObject(0, 0), start_index))
  {
    this->CompositeAttributes->RemoveBlockArrayAccessMode(dataObj);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkCompositePolyDataMapper::RemoveBlockArrayAccessModes()
{
  if (!this->CompositeAttributes)
  {
    return;
  }
  this->CompositeAttributes->RemoveBlockArrayAccessModes();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkCompositePolyDataMapper::SetBlockArrayComponent(unsigned int index, int value)
{
  if (!this->CompositeAttributes)
  {
    return;
  }
  unsigned int start_index = 0;
  if (auto dataObj = vtkCompositeDataDisplayAttributes::DataObjectFromIndex(
        index, this->GetInputDataObject(0, 0), start_index))
  {
    this->CompositeAttributes->SetBlockArrayComponent(dataObj, value);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int vtkCompositePolyDataMapper::GetBlockArrayComponent(unsigned int index)
{
  if (!this->CompositeAttributes)
  {
    return 0;
  }
  unsigned int start_index = 0;
  if (auto dataObj = vtkCompositeDataDisplayAttributes::DataObjectFromIndex(
        index, this->GetInputDataObject(0, 0), start_index))
  {
    return this->CompositeAttributes->GetBlockArrayComponent(dataObj);
  }
  return 0;
}

//----------------------------------------------------------------------------
void vtkCompositePolyDataMapper::RemoveBlockArrayComponent(unsigned int index)
{
  if (!this->CompositeAttributes)
  {
    return;
  }
  unsigned int start_index = 0;
  if (auto dataObj = vtkCompositeDataDisplayAttributes::DataObjectFromIndex(
        index, this->GetInputDataObject(0, 0), start_index))
  {
    this->CompositeAttributes->RemoveBlockArrayComponent(dataObj);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkCompositePolyDataMapper::RemoveBlockArrayComponents()
{
  if (!this->CompositeAttributes)
  {
    return;
  }
  this->CompositeAttributes->RemoveBlockArrayComponents();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkCompositePolyDataMapper::SetBlockArrayId(unsigned int index, int value)
{
  if (!this->CompositeAttributes)
  {
    return;
  }
  unsigned int start_index = 0;
  if (auto dataObj = vtkCompositeDataDisplayAttributes::DataObjectFromIndex(
        index, this->GetInputDataObject(0, 0), start_index))
  {
    this->CompositeAttributes->SetBlockArrayId(dataObj, value);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int vtkCompositePolyDataMapper::GetBlockArrayId(unsigned int index)
{
  if (!this->CompositeAttributes)
  {
    return -1;
  }
  unsigned int start_index = 0;
  if (auto dataObj = vtkCompositeDataDisplayAttributes::DataObjectFromIndex(
        index, this->GetInputDataObject(0, 0), start_index))
  {
    return this->CompositeAttributes->GetBlockArrayId(dataObj);
  }
  return -1;
}

//----------------------------------------------------------------------------
void vtkCompositePolyDataMapper::RemoveBlockArrayId(unsigned int index)
{
  if (!this->CompositeAttributes)
  {
    return;
  }
  unsigned int start_index = 0;
  if (auto dataObj = vtkCompositeDataDisplayAttributes::DataObjectFromIndex(
        index, this->GetInputDataObject(0, 0), start_index))
  {
    this->CompositeAttributes->RemoveBlockArrayId(dataObj);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkCompositePolyDataMapper::RemoveBlockArrayIds()
{
  if (!this->CompositeAttributes)
  {
    return;
  }
  this->CompositeAttributes->RemoveBlockArrayIds();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkCompositePolyDataMapper::SetBlockArrayName(unsigned int index, const std::string& value)
{
  if (!this->CompositeAttributes)
  {
    return;
  }
  unsigned int start_index = 0;
  if (auto dataObj = vtkCompositeDataDisplayAttributes::DataObjectFromIndex(
        index, this->GetInputDataObject(0, 0), start_index))
  {
    this->CompositeAttributes->SetBlockArrayName(dataObj, value);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
std::string vtkCompositePolyDataMapper::GetBlockArrayName(unsigned int index)
{
  if (!this->CompositeAttributes)
  {
    return "";
  }
  unsigned int start_index = 0;
  if (auto dataObj = vtkCompositeDataDisplayAttributes::DataObjectFromIndex(
        index, this->GetInputDataObject(0, 0), start_index))
  {
    return this->CompositeAttributes->GetBlockArrayName(dataObj);
  }
  return "";
}

//----------------------------------------------------------------------------
void vtkCompositePolyDataMapper::RemoveBlockArrayName(unsigned int index)
{
  if (!this->CompositeAttributes)
  {
    return;
  }
  unsigned int start_index = 0;
  if (auto dataObj = vtkCompositeDataDisplayAttributes::DataObjectFromIndex(
        index, this->GetInputDataObject(0, 0), start_index))
  {
    this->CompositeAttributes->RemoveBlockArrayName(dataObj);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkCompositePolyDataMapper::RemoveBlockArrayNames()
{
  if (!this->CompositeAttributes)
  {
    return;
  }
  this->CompositeAttributes->RemoveBlockArrayNames();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkCompositePolyDataMapper::SetBlockFieldDataTupleId(unsigned int index, vtkIdType value)
{
  if (!this->CompositeAttributes)
  {
    return;
  }
  unsigned int start_index = 0;
  if (auto dataObj = vtkCompositeDataDisplayAttributes::DataObjectFromIndex(
        index, this->GetInputDataObject(0, 0), start_index))
  {
    this->CompositeAttributes->SetBlockFieldDataTupleId(dataObj, value);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
vtkIdType vtkCompositePolyDataMapper::GetBlockFieldDataTupleId(unsigned int index)
{
  if (!this->CompositeAttributes)
  {
    return -1;
  }
  unsigned int start_index = 0;
  if (auto dataObj = vtkCompositeDataDisplayAttributes::DataObjectFromIndex(
        index, this->GetInputDataObject(0, 0), start_index))
  {
    return this->CompositeAttributes->GetBlockFieldDataTupleId(dataObj);
  }
  return -1;
}

//----------------------------------------------------------------------------
void vtkCompositePolyDataMapper::RemoveBlockFieldDataTupleId(unsigned int index)
{
  if (!this->CompositeAttributes)
  {
    return;
  }
  unsigned int start_index = 0;
  if (auto dataObj = vtkCompositeDataDisplayAttributes::DataObjectFromIndex(
        index, this->GetInputDataObject(0, 0), start_index))
  {
    this->CompositeAttributes->RemoveBlockFieldDataTupleId(dataObj);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkCompositePolyDataMapper::RemoveBlockFieldDataTupleIds()
{
  if (!this->CompositeAttributes)
  {
    return;
  }
  this->CompositeAttributes->RemoveBlockFieldDataTupleIds();
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkCompositePolyDataMapper::SetInputArrayToProcess(int idx, vtkInformation* inInfo)
{
  this->Superclass::SetInputArrayToProcess(idx, inInfo);

  const auto& internals = (*this->Internals);
  // set inputs to helpers
  for (auto& item : internals.BatchedDelegators)
  {
    item.second->GetDelegate()->SetInputArrayToProcess(idx, inInfo);
  }
}

//------------------------------------------------------------------------------
void vtkCompositePolyDataMapper::SetInputArrayToProcess(
  int idx, int port, int connection, int fieldAssociation, int attributeType)
{
  this->Superclass::SetInputArrayToProcess(idx, port, connection, fieldAssociation, attributeType);

  const auto& internals = (*this->Internals);
  // set inputs to helpers
  for (auto& item : internals.BatchedDelegators)
  {
    item.second->GetDelegate()->SetInputArrayToProcess(
      idx, port, connection, fieldAssociation, attributeType);
  }
}

//------------------------------------------------------------------------------
void vtkCompositePolyDataMapper::SetInputArrayToProcess(
  int idx, int port, int connection, int fieldAssociation, const char* name)
{
  this->Superclass::SetInputArrayToProcess(idx, port, connection, fieldAssociation, name);

  const auto& internals = (*this->Internals);
  // set inputs to helpers
  for (auto& item : internals.BatchedDelegators)
  {
    item.second->GetDelegate()->SetInputArrayToProcess(
      idx, port, connection, fieldAssociation, name);
  }
}

//-----------------------------------------------------------------------------
std::vector<vtkPolyData*> vtkCompositePolyDataMapper::GetRenderedList()
{
  return this->Internals->RenderedList;
}

//-----------------------------------------------------------------------------
void vtkCompositePolyDataMapper::ProcessSelectorPixelBuffers(
  vtkHardwareSelector* sel, std::vector<unsigned int>& pixeloffsets, vtkProp* prop)
{
  const auto& internals = (*this->Internals);
  // forward to helper
  for (auto& item : internals.BatchedDelegators)
  {
    item.second->GetDelegate()->ProcessSelectorPixelBuffers(sel, pixeloffsets, prop);
  }
}

//-----------------------------------------------------------------------------
vtkMTimeType vtkCompositePolyDataMapper::GetMTime()
{
  if (this->CompositeAttributes)
  {
    return std::max(this->Superclass::GetMTime(), this->CompositeAttributes->GetMTime());
  }
  return this->Superclass::GetMTime();
}
VTK_ABI_NAMESPACE_END
