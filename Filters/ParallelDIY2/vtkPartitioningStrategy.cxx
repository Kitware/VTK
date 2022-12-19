/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPartitioningStrategy.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPartitioningStrategy.h"

#include "vtkMultiProcessController.h"
#include "vtkRedistributeDataSetFilter.h"
#include "vtk_diy2.h"

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkPartitioningStrategy, Controller, vtkMultiProcessController);
//------------------------------------------------------------------------------
void vtkPartitioningStrategy::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent.GetNextIndent() << "NumberOfPartitions: " << this->NumberOfPartitions << std::endl;
  if (this->Controller)
  {
    this->Controller->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent.GetNextIndent() << "Controller: nullptr" << std::endl;
  }
}

//------------------------------------------------------------------------------
vtkPartitioningStrategy::vtkPartitioningStrategy()
{
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//------------------------------------------------------------------------------
vtkPartitioningStrategy::~vtkPartitioningStrategy()
{
  this->SetController(nullptr);
}
VTK_ABI_NAMESPACE_END
