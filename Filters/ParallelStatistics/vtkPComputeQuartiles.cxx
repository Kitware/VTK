/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkPContingencyStatistics.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2011 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
  -------------------------------------------------------------------------*/

#include "vtkPComputeQuartiles.h"

#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPOrderStatistics.h"
#include "vtkSmartPointer.h"

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
