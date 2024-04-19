// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPXdmf3Writer.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPXdmf3Writer);

//------------------------------------------------------------------------------
vtkPXdmf3Writer::vtkPXdmf3Writer() = default;

//------------------------------------------------------------------------------
vtkPXdmf3Writer::~vtkPXdmf3Writer() = default;

//------------------------------------------------------------------------------
void vtkPXdmf3Writer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
int vtkPXdmf3Writer::CheckParameters()
{
  vtkMultiProcessController* c = vtkMultiProcessController::GetGlobalController();
  int numberOfProcesses = c ? c->GetNumberOfProcesses() : 1;
  int myRank = c ? c->GetLocalProcessId() : 0;

  return this->Superclass::CheckParametersInternal(numberOfProcesses, myRank);
}

//------------------------------------------------------------------------------
int vtkPXdmf3Writer::RequestUpdateExtent(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  this->Superclass::RequestUpdateExtent(request, inputVector, outputVector);
  if (vtkMultiProcessController* c = vtkMultiProcessController::GetGlobalController())
  {
    int numberOfProcesses = c->GetNumberOfProcesses();
    int myRank = c->GetLocalProcessId();

    vtkInformation* info = inputVector[0]->GetInformationObject(0);
    info->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), myRank);
    info->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), numberOfProcesses);
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkPXdmf3Writer::GlobalContinueExecuting(int localContinue)
{
  vtkMultiProcessController* c = vtkMultiProcessController::GetGlobalController();
  int globalContinue = localContinue;
  if (c)
  {
    c->AllReduce(&localContinue, &globalContinue, 1, vtkCommunicator::MIN_OP);
  }
  return globalContinue;
}
VTK_ABI_NAMESPACE_END
