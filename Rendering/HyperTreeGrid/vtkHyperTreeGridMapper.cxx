/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperTreeGridMapper.h"

#include "vtkActor.h"
#include "vtkAdaptiveDataSetSurfaceFilter.h"
#include "vtkCamera.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositeDataSetRange.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkGroupDataSetsFilter.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridGeometry.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRange.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"

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
  cds->ShallowCopy(outCds);
  return cds;
}

void GetBoundsComposite(vtkCompositeDataSet* cd, double bounds[6])
{
  vtkBoundingBox globalBounds;
  for (auto block : vtk::Range(cd))
  {
    if (auto ds = vtkDataSet::SafeDownCast(block))
    {
      double localBounds[6];
      ds->GetBounds(localBounds);
      globalBounds.AddBounds(localBounds);
    }
    else if (auto htg = vtkHyperTreeGrid::SafeDownCast(block))
    {
      double localBounds[6];
      htg->GetBounds(localBounds);
      globalBounds.AddBounds(localBounds);
    }
  }
  globalBounds.GetBounds(bounds);
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
    ::GetBoundsComposite(this->Input, bounds);
  }
  else
  {
    vtkMath::UninitializeBounds(bounds);
  }
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkCompositeDataSet> vtkHyperTreeGridMapper::UpdateWithDecimation(
  vtkCompositeDataSet* cds, vtkRenderer* ren)
{
  bool useAdapt = this->UseAdaptiveDecimation;

  // Sanity check, Adaptive2DGeometryFilter only support ParallelProjection from now on.
  if (useAdapt && !ren->GetActiveCamera()->GetParallelProjection())
  {
    vtkWarningMacro("The adaptive decimation requires the camera to use ParallelProjection.");
    useAdapt = false;
  }

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
      if (useAdapt && htg->GetDimension() == 2)
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
