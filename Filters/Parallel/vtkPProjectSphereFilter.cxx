// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPProjectSphereFilter.h"

#include "vtkCommunicator.h"
#include "vtkIdList.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"

#include <map>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPProjectSphereFilter);

//------------------------------------------------------------------------------
vtkPProjectSphereFilter::vtkPProjectSphereFilter() = default;

//------------------------------------------------------------------------------
vtkPProjectSphereFilter::~vtkPProjectSphereFilter() = default;

//------------------------------------------------------------------------------
void vtkPProjectSphereFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkPProjectSphereFilter::ComputePointsClosestToCenterLine(
  double minDist2ToCenterLine, vtkIdList* polePointIds)
{
  if (vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController())
  {
    if (controller->GetNumberOfProcesses() > 1)
    {
      double tmp = minDist2ToCenterLine;
      double minDist2ToCenterLineGlobal = 0;
      controller->AllReduce(&tmp, &minDist2ToCenterLineGlobal, 1, vtkCommunicator::MAX_OP);
      if (tmp < minDist2ToCenterLineGlobal)
      {
        polePointIds->Reset();
      }
    }
  }
}

//------------------------------------------------------------------------------
double vtkPProjectSphereFilter::GetZTranslation(vtkPointSet* input)
{
  double localMax = this->Superclass::GetZTranslation(input);
  double globalMax = localMax;
  if (vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController())
  {
    if (controller->GetNumberOfProcesses() > 1)
    {
      controller->AllReduce(&localMax, &globalMax, 1, vtkCommunicator::MAX_OP);
    }
  }

  return globalMax;
}
VTK_ABI_NAMESPACE_END
