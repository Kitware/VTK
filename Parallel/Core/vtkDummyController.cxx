// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDummyController.h"
#include "vtkDummyCommunicator.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkDummyController);

vtkCxxSetObjectMacro(vtkDummyController, Communicator, vtkCommunicator);
vtkCxxSetObjectMacro(vtkDummyController, RMICommunicator, vtkCommunicator);

//------------------------------------------------------------------------------
vtkDummyController::vtkDummyController()
{
  this->Communicator = vtkDummyCommunicator::New();
  this->RMICommunicator = vtkDummyCommunicator::New();
}

vtkDummyController::~vtkDummyController()
{
  this->SetCommunicator(nullptr);
  this->SetRMICommunicator(nullptr);
}

void vtkDummyController::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Communicator: " << this->Communicator << endl;
  os << indent << "RMICommunicator: " << this->RMICommunicator << endl;
}

//------------------------------------------------------------------------------
void vtkDummyController::SingleMethodExecute()
{
  if (this->SingleMethod)
  {
    // Should we set the global controller here?  I'm going to say no since
    // we are not really a parallel job or at the very least not the global
    // controller.

    (this->SingleMethod)(this, this->SingleData);
  }
  else
  {
    vtkWarningMacro("SingleMethod not set.");
  }
}

//------------------------------------------------------------------------------
void vtkDummyController::MultipleMethodExecute()
{
  int i = this->GetLocalProcessId();

  vtkProcessFunctionType multipleMethod;
  void* multipleData;
  this->GetMultipleMethod(i, multipleMethod, multipleData);
  if (multipleMethod)
  {
    // Should we set the global controller here?  I'm going to say no since
    // we are not really a parallel job or at the very least not the global
    // controller.

    (multipleMethod)(this, multipleData);
  }
  else
  {
    vtkWarningMacro("MultipleMethod " << i << " not set.");
  }
}
VTK_ABI_NAMESPACE_END
