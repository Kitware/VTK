// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPCellSizeFilter.h"

#include "vtkCommunicator.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPCellSizeFilter);

//------------------------------------------------------------------------------
void vtkPCellSizeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkPCellSizeFilter::vtkPCellSizeFilter() = default;

//------------------------------------------------------------------------------
vtkPCellSizeFilter::~vtkPCellSizeFilter() = default;

//------------------------------------------------------------------------------
void vtkPCellSizeFilter::ComputeGlobalSum(double sum[4])
{
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
  if (controller->GetNumberOfProcesses() > 1)
  {
    double globalSum[4];
    controller->AllReduce(sum, globalSum, 4, vtkCommunicator::SUM_OP);
    for (int i = 0; i < 4; i++)
    {
      sum[i] = globalSum[i];
    }
  }
}
VTK_ABI_NAMESPACE_END
