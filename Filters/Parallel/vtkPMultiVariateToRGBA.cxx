// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPMultiVariateToRGBA.h"

// VTK includes
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"

// C/C++ includes
#include <cassert>
#include <sstream>

#include <iostream>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPMultiVariateToRGBA);
vtkCxxSetObjectMacro(vtkPMultiVariateToRGBA, Controller, vtkMultiProcessController);

//------------------------------------------------------------------------------
vtkPMultiVariateToRGBA::vtkPMultiVariateToRGBA()
{
  this->Controller = nullptr;
  this->SetController(vtkMultiProcessController::GetGlobalController());
  this->GlobalMin = [this](double localMin)
  {
    double globalMin;
    this->Controller->AllReduce(&localMin, &globalMin, 1, vtkCommunicator::MIN_OP);
    return globalMin;
  };
  this->GlobalMax = [this](double localMax)
  {
    double globalMax;
    this->Controller->AllReduce(&localMax, &globalMax, 1, vtkCommunicator::MAX_OP);
    return globalMax;
  };
}

//------------------------------------------------------------------------------
vtkPMultiVariateToRGBA::~vtkPMultiVariateToRGBA()
{
  this->SetController(nullptr);
}

//------------------------------------------------------------------------------
void vtkPMultiVariateToRGBA::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

VTK_ABI_NAMESPACE_END
