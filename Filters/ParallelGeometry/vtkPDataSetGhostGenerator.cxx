/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkPDataSetGhostGenerator.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkPDataSetGhostGenerator.h"

#include "vtkMultiProcessController.h"
#include "vtkMultiBlockDataSet.h"

#include <cassert>

vtkPDataSetGhostGenerator::vtkPDataSetGhostGenerator()
{
  this->Initialized = false;
  this->Controller  = vtkMultiProcessController::GetGlobalController();
}

//------------------------------------------------------------------------------
vtkPDataSetGhostGenerator::~vtkPDataSetGhostGenerator()
{
}

//------------------------------------------------------------------------------
void vtkPDataSetGhostGenerator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << "Controller: " << this->Controller << std::endl;
}

//------------------------------------------------------------------------------
void vtkPDataSetGhostGenerator::Initialize()
{
  assert("pre: Multi-process controller is NULL" && (this->Controller != NULL));
  this->Rank = this->Controller->GetLocalProcessId();
  this->Initialized = true;
}

//------------------------------------------------------------------------------
void vtkPDataSetGhostGenerator::Barrier()
{
  assert("pre: Multi-process controller is NULL" && (this->Controller != NULL));
  assert("pre: Instance has not been initialized!" && this->Initialized);
  this->Controller->Barrier();
}


