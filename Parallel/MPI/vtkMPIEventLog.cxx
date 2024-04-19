// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkMPIEventLog.h"

// clang-format off
#include "vtk_mpi.h" // required before "mpe.h" to avoid "C vs C++" conflicts
#include "mpe.h"
// clang-format on

#include "vtkMPIController.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
int vtkMPIEventLog::LastEventId = 0;

vtkStandardNewMacro(vtkMPIEventLog);

void vtkMPIEventLog::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

vtkMPIEventLog::vtkMPIEventLog()
{

  this->Active = 0;
}

void vtkMPIEventLog::InitializeLogging()
{
  MPE_Init_log();
}

void vtkMPIEventLog::FinalizeLogging(const char* fname)
{
  MPE_Finish_log(const_cast<char*>(fname));
}

int vtkMPIEventLog::SetDescription(const char* name, const char* desc)
{
  int err, processId;
  if ((err = MPI_Comm_rank(MPI_COMM_WORLD, &processId)) != MPI_SUCCESS)
  {
    char* msg = vtkMPIController::ErrorString(err);
    vtkErrorMacro("MPI error occurred: " << msg);
    delete[] msg;
    return 0;
  }

  this->Active = 1;
  if (processId == 0)
  {
    this->BeginId = MPE_Log_get_event_number();
    this->EndId = MPE_Log_get_event_number();
    MPE_Describe_state(
      this->BeginId, this->EndId, const_cast<char*>(name), const_cast<char*>(desc));
  }
  MPI_Bcast(&this->BeginId, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(&this->EndId, 1, MPI_INT, 0, MPI_COMM_WORLD);

  return 1;
}

void vtkMPIEventLog::StartLogging()
{
  if (!this->Active)
  {
    vtkWarningMacro("This vtkMPIEventLog has not been initialized. Can not log event.");
    return;
  }

  MPE_Log_event(this->BeginId, 0, "begin");
}

void vtkMPIEventLog::StopLogging()
{
  if (!this->Active)
  {
    vtkWarningMacro("This vtkMPIEventLog has not been initialized. Can not log event.");
    return;
  }
  MPE_Log_event(this->EndId, 0, "end");
}

vtkMPIEventLog::~vtkMPIEventLog() {}
VTK_ABI_NAMESPACE_END
