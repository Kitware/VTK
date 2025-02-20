// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkHyperTreeGridMapper.h"

#include "vtkActor.h"
#include "vtkAdaptiveDataSetSurfaceFilter.h"
#include "vtkCamera.h"
#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkDataObjectTree.h"
#include "vtkDataObjectTreeRange.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkGroupDataSetsFilter.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridGeometry.h"
#include "vtkInformation.h"
#include "vtkPolyDataMapper.h"
#include "vtkRange.h"
#include "vtkRenderWindow.h"

VTK_ABI_NAMESPACE_BEGIN
namespace
{
vtkSmartPointer<vtkCompositeDataSet> EnsureComposite(vtkDataObject* dobj)
{
  if (auto cds = vtkCompositeDataSet::SafeDownCast(dobj))
  {
    return cds;
  }
  vtkNew<vtkGroupDataSetsFilter> toComposite;
  toComposite->SetInputDataObject(dobj);
  toComposite->SetOutputTypeToMultiBlockDataSet();
  toComposite->Update();
  auto outCds = vtkCompositeDataSet::SafeDownCast(toComposite->GetOutputDataObject(0));
  auto cds = vtkSmartPointer<vtkCompositeDataSet>::Take(outCds->NewInstance());
  cds->CompositeShallowCopy(outCds);
  return cds;
}
}

vtkObjectFactoryNewMacro(vtkHyperTreeGridMapper);

//------------------------------------------------------------------------------
vtkHyperTreeGridMapper::vtkHyperTreeGridMapper() = default;

//------------------------------------------------------------------------------
vtkHyperTreeGridMapper::~vtkHyperTreeGridMapper() = default;

//------------------------------------------------------------------------------
void vtkHyperTreeGridMapper::Render(vtkRenderer* ren, vtkActor* act)
{

  auto* dataObj = this->GetInputDataObject(0, 0);
  if (dataObj == nullptr) // nothing to do
  {
    return;
  }

  // Adaptive decimation (if required)
  auto htgs = ::EnsureComposite(dataObj);
  auto adaptedHtgs = this->UpdateWithDecimation(htgs, ren);

  // Setup the mapper
  if (this->GetMTime() > this->Mapper->GetMTime())
  {
    this->Mapper->ShallowCopy(this);
  }

  this->Mapper->SetInputDataObject(adaptedHtgs);

  // Forward visibility property to the composite mapper.
  // This needs to be done after HTGs have been decimated because block selection is done using data
  // object pointers in the composite mapper.
  this->ApplyBlockVisibilities();

  this->Mapper->Render(ren, act);
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridMapper::SetInputDataObject(int port, vtkDataObject* input)
{
  this->Input = ::EnsureComposite(input);
  this->Superclass::SetInputDataObject(port, input);
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridMapper::SetInputDataObject(vtkDataObject* input)
{
  this->Input = ::EnsureComposite(input);
  this->Superclass::SetInputDataObject(input);
}

//------------------------------------------------------------------------------
double* vtkHyperTreeGridMapper::GetBounds()
{
  this->GetBounds(this->Bounds);
  return this->Bounds;
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridMapper::GetBounds(double bounds[6])
{
  if (this->Input)
  {
    this->GetBoundsComposite(bounds);
  }
  else
  {
    vtkMath::UninitializeBounds(bounds);
  }
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridMapper::GetBoundsComposite(double bounds[6])
{
  vtkBoundingBox globalBounds;

  // Input should always be a composite Multiblock because of EnsureComposite
  auto dtree = vtkDataObjectTree::SafeDownCast(this->Input);
  if (!dtree)
  {
    vtkErrorMacro("Expected a composite input structure");
    return;
  }
  for (const auto& ref : vtk::Range(dtree,
         vtk::DataObjectTreeOptions::TraverseSubTree | vtk::DataObjectTreeOptions::SkipEmptyNodes))
  {
    auto ds = vtkDataSet::SafeDownCast(ref.GetDataObject());
    auto htg = vtkHyperTreeGrid::SafeDownCast(ref.GetDataObject());
    bool visible = this->GetBlockVisibility(ref.GetFlatIndex());
    // When block is hidden, don't count it into bounds
    if (ds && visible)
    {
      double localBounds[6];
      ds->GetBounds(localBounds);
      globalBounds.AddBounds(localBounds);
    }
    else if (htg && visible)
    {
      double localBounds[6];
      htg->GetBounds(localBounds);
      globalBounds.AddBounds(localBounds);
    }
  }

  globalBounds.GetBounds(bounds);
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridMapper::ApplyBlockVisibilities()
{
  auto compositeMapper = vtkCompositePolyDataMapper::SafeDownCast(this->Mapper);
  if (compositeMapper)
  {
    for (auto idx : this->BlocksShown)
    {
      compositeMapper->SetBlockVisibility(idx, true);
    }
    for (auto idx : this->BlocksHidden)
    {
      compositeMapper->SetBlockVisibility(idx, false);
    }
  }
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridMapper::SetBlockVisibility(unsigned int index, bool visible)
{
  if (visible)
  {
    this->BlocksShown.insert(index);
    this->BlocksHidden.erase(index);
  }
  else
  {
    this->BlocksHidden.insert(index);
    this->BlocksShown.erase(index);
  }
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkHyperTreeGridMapper::GetBlockVisibility(unsigned int index)
{
  // Due to the Set logic, a block can't be in both sets at the same time
  if (this->BlocksShown.find(index) != this->BlocksShown.end())
  {
    return true;
  }
  else if (this->BlocksHidden.find(index) != this->BlocksHidden.end())
  {
    return false;
  }
  return true; // Visibility unset: block is visible
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridMapper::RemoveBlockVisibility(unsigned int index)
{
  size_t removed = this->BlocksShown.erase(index);
  removed += this->BlocksHidden.erase(index);
  if (removed > 0)
  {
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridMapper::RemoveBlockVisibilities()
{
  this->BlocksShown.clear();
  this->BlocksHidden.clear();
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridMapper::SetCompositeDataDisplayAttributes(
  vtkCompositeDataDisplayAttributes* attributes)
{
  auto compositeMapper = vtkCompositePolyDataMapper::SafeDownCast(this->Mapper);
  if (compositeMapper)
  {
    compositeMapper->SetCompositeDataDisplayAttributes(attributes);
  }
}

//------------------------------------------------------------------------------
vtkCompositeDataDisplayAttributes* vtkHyperTreeGridMapper::GetCompositeDataDisplayAttributes()
{
  auto compositeMapper = vtkCompositePolyDataMapper::SafeDownCast(this->Mapper);
  if (compositeMapper)
  {
    return compositeMapper->GetCompositeDataDisplayAttributes();
  }
  return nullptr;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkCompositeDataSet> vtkHyperTreeGridMapper::UpdateWithDecimation(
  vtkCompositeDataSet* cds, vtkRenderer* ren)
{
  bool useAdapt = this->UseAdaptiveDecimation;

  vtkNew<vtkAdaptiveDataSetSurfaceFilter> adaptiveGeometryFilter;
  vtkNew<vtkHyperTreeGridGeometry> geometryFilter;
  vtkNew<vtkDataSetSurfaceFilter> surfaceFilter;

  adaptiveGeometryFilter->SetRenderer(ren);

  auto outputComposite = vtkSmartPointer<vtkCompositeDataSet>::Take(cds->NewInstance());
  outputComposite->CopyStructure(cds);

  auto iter = vtkSmartPointer<vtkCompositeDataIterator>::Take(cds->NewIterator());
  iter->SkipEmptyNodesOn();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    vtkDataObject* leaf = iter->GetCurrentDataObject();
    if (auto* htg = vtkHyperTreeGrid::SafeDownCast(leaf))
    {
      if (useAdapt)
      {
        // use adaptive decimation
        adaptiveGeometryFilter->SetInputDataObject(htg);
        adaptiveGeometryFilter->Update();
        vtkDataObject* outputDS = adaptiveGeometryFilter->GetOutputDataObject(0);
        auto newBlock = vtkSmartPointer<vtkDataObject>::Take(outputDS->NewInstance());
        newBlock->ShallowCopy(outputDS);
        outputComposite->SetDataSet(iter.Get(), newBlock);
      }
      else
      {
        // simply transform to polydata
        geometryFilter->SetInputDataObject(htg);
        geometryFilter->Update();
        vtkDataObject* outputDS = geometryFilter->GetOutputDataObject(0);
        auto newBlock = vtkSmartPointer<vtkDataObject>::Take(outputDS->NewInstance());
        newBlock->ShallowCopy(outputDS);
        outputComposite->SetDataSet(iter.Get(), newBlock);
      }
    }
    else if (auto* ds = vtkDataSet::SafeDownCast(leaf))
    {
      // other cases
      surfaceFilter->SetInputDataObject(ds);
      surfaceFilter->Update();
      vtkDataObject* outputDS = surfaceFilter->GetOutputDataObject(0);
      auto newBlock = vtkSmartPointer<vtkDataObject>::Take(outputDS->NewInstance());
      newBlock->ShallowCopy(outputDS);
      outputComposite->SetDataSet(iter.Get(), newBlock);
    }
  }

  return outputComposite;
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "UseAdaptiveDecimation: " << this->UseAdaptiveDecimation << std::endl;
  os << indent << "Internal Mapper: " << std::endl;
  this->Mapper->PrintSelf(os, indent.GetNextIndent());
}

//------------------------------------------------------------------------------
int vtkHyperTreeGridMapper::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");
  return 1;
}
VTK_ABI_NAMESPACE_END
