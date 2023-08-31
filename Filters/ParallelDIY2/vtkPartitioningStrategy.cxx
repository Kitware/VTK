// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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
