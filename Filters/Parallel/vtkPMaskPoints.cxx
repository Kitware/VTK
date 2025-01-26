// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPMaskPoints.h"

#include "vtkCommunicator.h"
#include "vtkDummyController.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPMaskPoints);

vtkCxxSetObjectMacro(vtkPMaskPoints, Controller, vtkMultiProcessController);

//------------------------------------------------------------------------------
vtkPMaskPoints::vtkPMaskPoints()
{
  this->Controller = nullptr;

  vtkSmartPointer<vtkMultiProcessController> controller =
    vtkMultiProcessController::GetGlobalController();
  if (!controller)
  {
    controller = vtkSmartPointer<vtkDummyController>::New();
  }
  this->SetController(controller);
}

//------------------------------------------------------------------------------
vtkPMaskPoints::~vtkPMaskPoints()
{
  this->SetController(nullptr);
}

//------------------------------------------------------------------------------
void vtkPMaskPoints::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->GetController())
  {
    os << indent << "Controller: " << this->GetController() << std::endl;
  }
  else
  {
    os << indent << "Controller: (null)" << std::endl;
  }
}

//------------------------------------------------------------------------------
vtkMultiProcessController* vtkPMaskPoints::GetController()
{
  return this->Controller;
}

//------------------------------------------------------------------------------
void vtkPMaskPoints::InternalScatter(unsigned long* a, unsigned long* b, int c, int d)
{
  this->Controller->Scatter(a, b, c, d);
}

//------------------------------------------------------------------------------
void vtkPMaskPoints::InternalGather(unsigned long* a, unsigned long* b, int c, int d)
{
  this->Controller->Gather(a, b, c, d);
}

//------------------------------------------------------------------------------
void vtkPMaskPoints::InternalBroadcast(double* a, int b, int c)
{
  this->Controller->Broadcast(a, b, c);
}

//------------------------------------------------------------------------------
void vtkPMaskPoints::InternalGather(double* a, double* b, int c, int d)
{
  this->Controller->Gather(a, b, c, d);
}

//------------------------------------------------------------------------------
int vtkPMaskPoints::InternalGetNumberOfProcesses()
{
  return this->Controller->GetNumberOfProcesses();
}

//------------------------------------------------------------------------------
int vtkPMaskPoints::InternalGetLocalProcessId()
{
  return this->Controller->GetLocalProcessId();
}

//------------------------------------------------------------------------------
void vtkPMaskPoints::InternalBarrier()
{
  this->Controller->Barrier();
}

//------------------------------------------------------------------------------
void vtkPMaskPoints::InternalSplitController(int color, int key)
{
  this->OriginalController = this->Controller;
  this->Controller = this->OriginalController->PartitionController(color, key);
}

//------------------------------------------------------------------------------
void vtkPMaskPoints::InternalResetController()
{
  this->Controller->Delete();
  this->Controller = this->OriginalController;
  this->OriginalController = nullptr;
}
VTK_ABI_NAMESPACE_END
