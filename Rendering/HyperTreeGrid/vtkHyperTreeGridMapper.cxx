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

#include "vtkAdaptiveDataSetSurfaceFilter.h"
#include "vtkExecutive.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridGeometry.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include "vtkActor.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkProperty.h"

vtkObjectFactoryNewMacro(vtkHyperTreeGridMapper);

//------------------------------------------------------------------------------
vtkHyperTreeGridMapper::vtkHyperTreeGridMapper() = default;

//------------------------------------------------------------------------------
vtkHyperTreeGridMapper::~vtkHyperTreeGridMapper() = default;

//------------------------------------------------------------------------------
void vtkHyperTreeGridMapper::Render(vtkRenderer* ren, vtkActor* act)
{
  if (this->GetMTime() > this->PDMapper->GetMTime())
  { // forward common internal properties
    this->PDMapper->ShallowCopy(this);
    this->PDMapper->SetInputData(this->GetSurfaceFilterInput());
  }

  this->PDMapper->Render(ren, act);
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
// Specify the input data or filter.
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
  os << indent << "UseCameraFrustum: " << this->UseCameraFrustum << std::endl;
  this->GetSurfaceFilter()->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Internal PolyData Mapper: " << std::endl;
  this->PDMapper->PrintSelf(os, indent.GetNextIndent());
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
  if (this->UseCameraFrustum)
  {
    return this->Adaptive2DGeometryFilter;
  }
  else
  {
    return this->GeometryFilter;
  }
}
