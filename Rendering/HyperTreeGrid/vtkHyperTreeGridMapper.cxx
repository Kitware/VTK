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
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridGeometry.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"

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

  if (auto* mb = vtkMultiBlockDataSet::SafeDownCast(dataObj))
  {
    vtkWarningMacro("HTG MB Case");
  }

  if (auto* htg = vtkHyperTreeGrid::SafeDownCast(dataObj))
  {
    this->UpdateDecimationHTG(htg, ren);
  }

  if (this->GetMTime() > this->Mapper->GetMTime())
  {
    this->Mapper->ShallowCopy(this);
    this->Mapper->SetInputData(this->GetSurfaceFilterInput());
  }

  this->Mapper->Render(ren, act);
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridMapper::SetInputConnection(vtkAlgorithmOutput* input)
{
  this->GeometryFilter->SetInputConnection(input);
  this->Adaptive2DGeometryFilter->SetInputConnection(input);
  this->Superclass::SetInputConnection(input);
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridMapper::SetInputDataObject(int port, vtkDataObject* input)
{
  this->GeometryFilter->SetInputDataObject(input);
  this->Adaptive2DGeometryFilter->SetInputDataObject(input);
  this->Superclass::SetInputDataObject(port, input);
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridMapper::SetInputDataObject(vtkDataObject* input)
{
  this->SetInputDataObject(0, input);
}

//------------------------------------------------------------------------------
double* vtkHyperTreeGridMapper::GetBounds()
{
  return this->GetSurfaceFilterInput()->GetBounds();
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridMapper::GetBounds(double bounds[6])
{
  this->GetSurfaceFilterInput()->GetBounds(bounds);
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridMapper::UpdateDecimationHTG(vtkHyperTreeGrid* htg, vtkRenderer* ren)
{
  if (htg->GetDimension() == 2)
  {
    bool renderAdaptiveGeo = this->UseAdaptiveDecimation;
    if (renderAdaptiveGeo && !ren->GetActiveCamera()->GetParallelProjection())
    {
      // This Adaptive2DGeometryFilter only support ParallelProjection from now on.
      renderAdaptiveGeo = false;
      vtkWarningMacro("The adaptive decimation requires the camera to use ParallelProjection.");
    }

    if (renderAdaptiveGeo)
    {
      // ensure the camera is accessible in the Adaptive2DGeometryFilter
      // if we need to cut the geometry using its frustum
      this->Adaptive2DGeometryFilter->SetRenderer(ren);
    }
    else
    {
      this->Adaptive2DGeometryFilter->SetRenderer(nullptr);
    }
  }
  else if (this->UseAdaptiveDecimation)
  {
    vtkWarningMacro("the adaptive decimation is only available for 2D HTG.");
  }
}

//------------------------------------------------------------------------------
vtkPolyData* vtkHyperTreeGridMapper::GetSurfaceFilterInput()
{
  vtkAlgorithm* geometry = this->GetSurfaceFilter();
  if (geometry->GetInputDataObject(0, 0))
  {
    geometry->Update();
  }
  return vtkPolyData::SafeDownCast(geometry->GetOutputDataObject(0));
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "UseAdaptiveDecimation: " << this->UseAdaptiveDecimation << std::endl;
  this->GetSurfaceFilter()->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Internal PolyData Mapper: " << std::endl;
  this->Mapper->PrintSelf(os, indent.GetNextIndent());
}

//------------------------------------------------------------------------------
int vtkHyperTreeGridMapper::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");
  return 1;
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridMapper::Update(int port)
{
  this->Superclass::Update(port);
  this->GetSurfaceFilter()->Update();
}

//------------------------------------------------------------------------------
vtkAlgorithm* vtkHyperTreeGridMapper::GetSurfaceFilter()
{
  if (this->UseAdaptiveDecimation)
  {
    return this->Adaptive2DGeometryFilter;
  }
  else
  {
    return this->GeometryFilter;
  }
}
