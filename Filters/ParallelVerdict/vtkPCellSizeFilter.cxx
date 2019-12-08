/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPCellSizeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
#include "vtkPCellSizeFilter.h"

#include "vtkCommunicator.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPCellSizeFilter);

//-----------------------------------------------------------------------------
vtkPCellSizeFilter::vtkPCellSizeFilter() {}

//-----------------------------------------------------------------------------
vtkPCellSizeFilter::~vtkPCellSizeFilter() {}

//-----------------------------------------------------------------------------
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
