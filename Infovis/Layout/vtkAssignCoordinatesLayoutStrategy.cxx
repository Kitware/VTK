// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkAssignCoordinatesLayoutStrategy.h"

#include "vtkAssignCoordinates.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkTree.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkAssignCoordinatesLayoutStrategy);

vtkAssignCoordinatesLayoutStrategy::vtkAssignCoordinatesLayoutStrategy()
{
  this->AssignCoordinates = vtkSmartPointer<vtkAssignCoordinates>::New();
}

vtkAssignCoordinatesLayoutStrategy::~vtkAssignCoordinatesLayoutStrategy() = default;

void vtkAssignCoordinatesLayoutStrategy::SetXCoordArrayName(const char* name)
{
  this->AssignCoordinates->SetXCoordArrayName(name);
}

const char* vtkAssignCoordinatesLayoutStrategy::GetXCoordArrayName()
{
  return this->AssignCoordinates->GetXCoordArrayName();
}

void vtkAssignCoordinatesLayoutStrategy::SetYCoordArrayName(const char* name)
{
  this->AssignCoordinates->SetYCoordArrayName(name);
}

const char* vtkAssignCoordinatesLayoutStrategy::GetYCoordArrayName()
{
  return this->AssignCoordinates->GetYCoordArrayName();
}

void vtkAssignCoordinatesLayoutStrategy::SetZCoordArrayName(const char* name)
{
  this->AssignCoordinates->SetZCoordArrayName(name);
}

const char* vtkAssignCoordinatesLayoutStrategy::GetZCoordArrayName()
{
  return this->AssignCoordinates->GetZCoordArrayName();
}

void vtkAssignCoordinatesLayoutStrategy::Layout()
{
  this->AssignCoordinates->SetInputData(this->Graph);
  this->AssignCoordinates->Update();
  this->Graph->ShallowCopy(this->AssignCoordinates->GetOutput());
}

void vtkAssignCoordinatesLayoutStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
