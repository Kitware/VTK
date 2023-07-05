// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2011 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkPComputeQuantiles.h"

#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPOrderStatistics.h"
#include "vtkSmartPointer.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPComputeQuantiles);
vtkCxxSetObjectMacro(vtkPComputeQuantiles, Controller, vtkMultiProcessController);

//------------------------------------------------------------------------------
vtkPComputeQuantiles::vtkPComputeQuantiles()
{
  this->Controller = nullptr;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//------------------------------------------------------------------------------
vtkPComputeQuantiles::~vtkPComputeQuantiles()
{
  this->SetController(nullptr);
}

//------------------------------------------------------------------------------
vtkOrderStatistics* vtkPComputeQuantiles::CreateOrderStatisticsFilter()
{
  auto filter = vtkPOrderStatistics::New();
  filter->SetController(this->Controller);
  return filter;
}
VTK_ABI_NAMESPACE_END
