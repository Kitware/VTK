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

vtkObjectFactoryNewMacro(vtkHyperTreeGridMapper);

//------------------------------------------------------------------------------
vtkHyperTreeGridMapper::vtkHyperTreeGridMapper()
  : GeometryFilter(vtkSmartPointer<vtkHyperTreeGridGeometry>::New())
  , Adaptive2DGeometryFilter(vtkSmartPointer<vtkAdaptiveDataSetSurfaceFilter>::New())
  , UseLOD(false)
{
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridMapper::Render(vtkRenderer* ren, vtkActor* act)
{
  std::cout << "vtkHyperTreeGridMapper::Render " << std::endl;
  if (this->UseLOD)
  {
    this->Adaptive2DGeometryFilter->SetRenderer(ren);
    this->Adaptive2DGeometryFilter->Update();
  }
  else
  {
    this->GeometryFilter->SetInputArrayToProcess(
      0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, "Depth");
    this->GeometryFilter->Update();
  }

  this->SelectColorArray("Depth");
  if (this->GetScalarMode() == VTK_SCALAR_MODE_USE_CELL_FIELD_DATA)
  {
    std::cout << "COOL " << std::endl;
  }
  else
  {
    std::cout << "PAS COOL " << std::endl;
  }
  this->Superclass::Render(ren, act);
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridMapper::SetInputData(vtkHyperTreeGrid* input)
{
  this->GeometryFilter->SetInputData(input);
  this->Adaptive2DGeometryFilter->SetInputData(input);
  this->Superclass::SetInputDataObject(input);
}

//------------------------------------------------------------------------------
// Specify the input data or filter.
vtkPolyData* vtkHyperTreeGridMapper::GetInput()
{
  this->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, "Depth");
  this->GeometryFilter->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, "Depth");
  this->Update();
  this->GeometryFilter->Update();
  return this->UseLOD
    ? vtkPolyData::SafeDownCast(this->Adaptive2DGeometryFilter->GetOutputDataObject(0))
    : vtkPolyData::SafeDownCast(this->GeometryFilter->GetOutputDataObject(0));
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
int vtkHyperTreeGridMapper::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");
  return 1;
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridMapper::SetInputConnection(vtkAlgorithmOutput* input)
{
  this->GeometryFilter->SetInputConnection(input);
  this->Adaptive2DGeometryFilter->SetInputConnection(input);
  this->Superclass::SetInputConnection(input);
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridMapper::Update(int port)
{
  this->Superclass::Update(port);
  if (!this->UseLOD)
  {
    this->GeometryFilter->Update();
  }
  else
  {
    this->Adaptive2DGeometryFilter->Update();
  }
}
