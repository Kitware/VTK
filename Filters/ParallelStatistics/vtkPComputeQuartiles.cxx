// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2011 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkPComputeQuartiles.h"

#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPOrderStatistics.h"
#include "vtkSmartPointer.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPComputeQuartiles);
vtkCxxSetObjectMacro(vtkPComputeQuartiles, Controller, vtkMultiProcessController);

//------------------------------------------------------------------------------
vtkPComputeQuartiles::vtkPComputeQuartiles()
{
  this->Controller = nullptr;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//------------------------------------------------------------------------------
vtkPComputeQuartiles::~vtkPComputeQuartiles()
{
  this->SetController(nullptr);
}

//------------------------------------------------------------------------------
vtkOrderStatistics* vtkPComputeQuartiles::CreateOrderStatisticsFilter()
{
  auto filter = vtkPOrderStatistics::New();
  filter->SetController(this->Controller);
  return filter;
}
VTK_ABI_NAMESPACE_END
