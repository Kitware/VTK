/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPProjectSphereFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPProjectSphereFilter.h"

#include "vtkCommunicator.h"
#include "vtkIdList.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"

#include <map>

vtkStandardNewMacro(vtkPProjectSphereFilter);

//-----------------------------------------------------------------------------
vtkPProjectSphereFilter::vtkPProjectSphereFilter()
{
}

//-----------------------------------------------------------------------------
vtkPProjectSphereFilter::~vtkPProjectSphereFilter()
{
}

//-----------------------------------------------------------------------------
void vtkPProjectSphereFilter::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void vtkPProjectSphereFilter::ComputePointsClosestToCenterLine(
  double minDist2ToCenterLine, vtkIdList* polePointIds)
{
  if(vtkMultiProcessController* controller =
     vtkMultiProcessController::GetGlobalController())
    {
    if(controller->GetNumberOfProcesses() >1)
      {
      double tmp = minDist2ToCenterLine;
      double minDist2ToCenterLineGlobal = 0;
      controller->AllReduce(&tmp, &minDist2ToCenterLineGlobal,
                            1, vtkCommunicator::MAX_OP);
      if(tmp < minDist2ToCenterLineGlobal)
        {
        polePointIds->Reset();
        }
      }
    }
}

//-----------------------------------------------------------------------------
double vtkPProjectSphereFilter::GetZTranslation(vtkPointSet* input)
{
  double localMax = this->Superclass::GetZTranslation(input);
  double globalMax = localMax;
  if(vtkMultiProcessController* controller =
     vtkMultiProcessController::GetGlobalController())
    {
    if(controller->GetNumberOfProcesses() > 1)
      {
      controller->AllReduce(&localMax, &globalMax, 1, vtkCommunicator::MAX_OP);
      }
    }

  return globalMax;
}
