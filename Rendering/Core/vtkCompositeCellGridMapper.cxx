// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCompositeCellGridMapper.h"

#include "vtkActor.h"
#include "vtkCellGrid.h"
#include "vtkCellGridMapper.h"
#include "vtkCommand.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataObjectTree.h"
#include "vtkDataObjectTreeRange.h"
#include "vtkExecutive.h"
#include "vtkGarbageCollector.h"
#include "vtkInformation.h"
#include "vtkMapper.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkSetGet.h"
#include "vtkTexture.h"

#include <stack>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkCompositeCellGridMapper);

//----------------------------------------------------------------------------
vtkCxxSetSmartPointerMacro(
  vtkCompositeCellGridMapper, CompositeDataDisplayAttributes, vtkCompositeDataDisplayAttributes);

//----------------------------------------------------------------------------
class vtkCompositeCellGridMapper::vtkInternals
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

  struct MapperItem
  {
    vtkSmartPointer<vtkCellGridMapper> Mapper;
    RenderBlockState Attributes;
    bool Marked = false;
  };
  // <Key, Value> : <FlatBlockIndex, vtkCellGridMapper>
  std::unordered_map<unsigned int, MapperItem> BlockMappers;
};

//----------------------------------------------------------------------------
vtkCompositeCellGridMapper::vtkCompositeCellGridMapper()
  : Internals(std::unique_ptr<vtkInternals>(new vtkInternals()))
{
}

//----------------------------------------------------------------------------
vtkCompositeCellGridMapper::~vtkCompositeCellGridMapper() = default;

//----------------------------------------------------------------------------
// Specify the type of data this mapper can handle. If we are
// working with a regular (not hierarchical) pipeline, then we
// need vtkCellGrid. For composite data pipelines, then
// vtkCompositeDataSet is required, and we'll check when
// building our structure whether all the part of the composite
// data set are vtkCellGrid objects.
int vtkCompositeCellGridMapper::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
void vtkCompositeCellGridMapper::BuildRenderValues(
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
  // const auto originalFlatIndex = flatIndex;
  flatIndex++;

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
  else if (auto cellGrid = vtkCellGrid::SafeDownCast(dobj))
  {
    vtkInternals::MapperItem mItem;
    mItem.Mapper = vtk::TakeSmartPointer(this->MakeAMapper());
    mItem.Marked = true;
    mItem.Attributes = internals.BlockState;
    // apply properties on the mapper
    mItem.Mapper->SetInputData(cellGrid);
    // block requests to update upstream algorithm, because there is no upstream algorithm.
    mItem.Mapper->SetStatic(true);
    // capture the render block state.
    mItem.Mapper->SetScalarMode(mItem.Attributes.ScalarMode.top());
    mItem.Mapper->SetArrayAccessMode(mItem.Attributes.ArrayAccessMode.top());
    mItem.Mapper->SetArrayComponent(mItem.Attributes.ArrayComponent.top());
    mItem.Mapper->SetArrayId(mItem.Attributes.ArrayId.top());
    mItem.Mapper->SetArrayName(mItem.Attributes.ArrayName.top().c_str());
    mItem.Mapper->SetFieldDataTupleId(mItem.Attributes.FieldDataTupleId.top());
    mItem.Mapper->SetScalarVisibility(mItem.Attributes.ScalarVisibility.top());
    mItem.Mapper->SetColorMode(mItem.Attributes.ColorMode.top());
    mItem.Mapper->SetUseLookupTableScalarRange(mItem.Attributes.UseLookupTableScalarRange.top());
    mItem.Mapper->SetInterpolateScalarsBeforeMapping(
      mItem.Attributes.InterpolateScalarsBeforeMapping.top());
    mItem.Mapper->SetScalarRange(mItem.Attributes.ScalarRange.top().GetData());
    mItem.Mapper->SetLookupTable(mItem.Attributes.LookupTable.top());
    auto iter = internals.BlockMappers.find(flatIndex - 1);
    if (iter != internals.BlockMappers.end())
    {
      // remove what was found.
      auto& foundMitem = iter->second;
      foundMitem.Mapper->ReleaseGraphicsResources(renderer->GetVTKWindow());
      internals.BlockMappers.erase(iter);
    }
    internals.BlockMappers.insert(std::make_pair<>(flatIndex - 1, mItem));
    vtkDebugMacro(<< "Inserted mapper " << mItem.Mapper << " for " << cellGrid << " at "
                  << flatIndex - 1);
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

//----------------------------------------------------------------------------
void vtkCompositeCellGridMapper::Render(vtkRenderer* renderer, vtkActor* actor)
{
  auto& internals = (*this->Internals);
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
    return;
  }

  this->TimeToDraw = 0;
  // If the CellGridMappers are not up-to-date then rebuild them
  vtkCompositeDataPipeline* executive =
    vtkCompositeDataPipeline::SafeDownCast(this->GetExecutive());

  this->TempState.Clear();
  this->TempState.Append(actor->GetProperty()->GetMTime(), "actor mtime");
  this->TempState.Append(this->GetMTime(), "this mtime");
  this->TempState.Append(executive->GetPipelineMTime(), "pipeline mtime");
  this->TempState.Append(
    actor->GetTexture() ? actor->GetTexture()->GetMTime() : 0, "texture mtime");

  auto property = actor->GetProperty();
  if (this->RenderValuesState != this->TempState)
  {
    this->RenderValuesState = this->TempState;
    if (auto lut = this->GetLookupTable())
    {
      lut->Build();
    }
    auto selColor = property->GetSelectionColor();

    // unmark old delegators
    for (auto& iter : internals.BlockMappers)
    {
      auto& mapperItem = iter.second;
      mapperItem.Marked = false;
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
    internals.BlockState.LookupTable.emplace(this->GetLookupTable());

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
    for (auto iter = internals.BlockMappers.begin(); iter != internals.BlockMappers.end();)
    {
      if (!iter->second.Marked)
      {
        iter->second.Mapper->ReleaseGraphicsResources(renderer->GetVTKWindow());
        internals.BlockMappers.erase(iter++);
      }
      else
      {
        ++iter;
      }
    }
  }

  bool rendererIsInSelectionPass = renderer->GetSelector() != nullptr;
  bool tpass = actor->IsRenderingTranslucentPolygonalGeometry();
  for (auto& iter : internals.BlockMappers)
  {
    auto& mItem = iter.second;
    // start out assuming we have to render this block.
    bool shouldDraw = true;
    // clang-format off
    // must be visible
    shouldDraw &= (mItem.Attributes.Visibility.top());
    // and pickable when selecting
    shouldDraw &= (!rendererIsInSelectionPass || mItem.Attributes.Pickability.top());
    // opaque during opaque pass or when selecting
    shouldDraw &= ((rendererIsInSelectionPass || mItem.Attributes.Opacity.top() >= 1.0 || actor->GetForceOpaque()) && !tpass);
     // translucent during translucent and never selecting
    shouldDraw |= (mItem.Attributes.Visibility.top() && (!rendererIsInSelectionPass && (mItem.Attributes.Opacity.top() < 1.0 || actor->GetForceTranslucent()) && tpass));
    // clang-format on
    if (!shouldDraw)
    {
      continue;
    }
    // set opacity on the actor if a block override was found.
    auto blockOverridesOpacity = property->GetOpacity() != mItem.Attributes.Opacity.top();
    auto oldOpacity = property->GetOpacity();
    if (blockOverridesOpacity)
    {
      // FIXME: This unnecessarily modifies MTime of actor's vtkProperty which has BAD implications
      // on performance.
      property->SetOpacity(mItem.Attributes.Opacity.top());
    }
    mItem.Mapper->Render(renderer, actor);
    if (blockOverridesOpacity)
    {
      // restore old opacity so that other blocks without opacity overrides use actor opacity.
      property->SetOpacity(oldOpacity);
    }
    this->TimeToDraw += mItem.Mapper->GetTimeToDraw();
  }
}

//----------------------------------------------------------------------------
vtkExecutive* vtkCompositeCellGridMapper::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}

//----------------------------------------------------------------------------
// Looks at each DataSet and finds the union of all the bounds
void vtkCompositeCellGridMapper::ComputeBounds()
{
  vtkDataObjectTree* input = vtkDataObjectTree::SafeDownCast(this->GetInputDataObject(0, 0));

  // If we don't have hierarchical data, test to see if we have
  // plain old polydata. In this case, the bounds are simply
  // the bounds of the input polydata.
  if (!input)
  {
    auto* cg = vtkCellGrid::SafeDownCast(this->GetInputDataObject(0, 0));
    if (input && input->GetNumberOfCells())
    {
      cg->GetBounds(this->Bounds);
    }
    else
    {
      vtkMath::UninitializeBounds(this->Bounds);
      return;
    }
  }

  if (input->GetMTime() < this->BoundsMTime && this->GetMTime() < this->BoundsMTime)
  {
    return;
  }

  // computing bounds with only visible blocks
  vtkCompositeDataDisplayAttributes::ComputeVisibleBounds(
    this->CompositeDataDisplayAttributes, input, this->Bounds);
  this->BoundsMTime.Modified();
}

//----------------------------------------------------------------------------
double* vtkCompositeCellGridMapper::GetBounds()
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

//----------------------------------------------------------------------------
void vtkCompositeCellGridMapper::ReleaseGraphicsResources(vtkWindow* win)
{
  auto& internals = (*this->Internals);
  for (auto& iter : internals.BlockMappers)
  {
    auto& mItem = iter.second;
    mItem.Mapper->ReleaseGraphicsResources(win);
  }
}

void vtkCompositeCellGridMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

vtkCellGridMapper* vtkCompositeCellGridMapper::MakeAMapper()
{
  vtkCellGridMapper* m = vtkCellGridMapper::New();
  // Copy our vtkMapper properties to the delegate
  m->vtkMapper::ShallowCopy(this);
  return m;
}

//------------------------------------------------------------------------------
// simple tests, the mapper is tolerant of being
// called both on opaque and translucent
bool vtkCompositeCellGridMapper::HasOpaqueGeometry()
{
  return true;
}

//------------------------------------------------------------------------------
bool vtkCompositeCellGridMapper::RecursiveHasTranslucentGeometry(
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
    (void)scalarMode;
    (void)arrayAccessMode;
    (void)arrayComponent;
    (void)arrayId;
    (void)colorMode;

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

    vtkCellGrid* cg = vtkCellGrid::SafeDownCast(dobj);
    (void)cg;

    // if we think it is opaque check the scalars
    if (scalarVisibility)
    {
      // See FIXME in vtkCellGridMapper::HasTranslucentPolygonalGeometry
      return false;
      // int cellFlag;
      // vtkDataArray* scalars = vtkCompositeCellGridMapper::GetScalars(
      //   cg, scalarMode, arrayAccessMode, arrayId, arrayName.c_str(), cellFlag);

      // unsigned char ghostsToSkip;
      // vtkUnsignedCharArray* ghosts = vtkAbstractMapper::GetGhostArray(cg, scalarMode,
      // ghostsToSkip);

      // if (lut->IsOpaque(scalars, colorMode, arrayComponent, ghosts, ghostsToSkip) == 0)
      // {
      //   return true;
      // }
    }
  }

  return false;
}

//------------------------------------------------------------------------------
vtkMTimeType vtkCompositeCellGridMapper::GetMTime()
{
  if (this->CompositeDataDisplayAttributes)
  {
    return std::max(this->Superclass::GetMTime(), this->CompositeDataDisplayAttributes->GetMTime());
  }
  return this->Superclass::GetMTime();
}

//------------------------------------------------------------------------------
// look at children
bool vtkCompositeCellGridMapper::HasTranslucentPolygonalGeometry()
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

VTK_ABI_NAMESPACE_END
