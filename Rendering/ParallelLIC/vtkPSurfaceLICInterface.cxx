// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPSurfaceLICInterface.h"

#include "vtkMPI.h"
#include "vtkObjectFactory.h"
#include "vtkPPainterCommunicator.h"
#include "vtkPainterCommunicator.h"
#include "vtkParallelTimer.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPSurfaceLICInterface);

//------------------------------------------------------------------------------
vtkPSurfaceLICInterface::vtkPSurfaceLICInterface() = default;

//------------------------------------------------------------------------------
#ifdef vtkPSurfaceLICInterfaceDEBUG
vtkPSurfaceLICInterface::~vtkPSurfaceLICInterface()
{
  cerr << "=====vtkPSurfaceLICInterface::~vtkPSurfaceLICInterface" << endl;
}
#else
vtkPSurfaceLICInterface::~vtkPSurfaceLICInterface() = default;
#endif

//------------------------------------------------------------------------------
bool vtkPSurfaceLICInterface::NeedToUpdateCommunicator()
{
  // TODO -- with slice widget in PV the input dataset
  // MTime is changing at different rates on different
  // MPI ranks. Because of this some ranks want to update
  // there comunicator while other do not. To work around
  // this force the communicator update on all ranks if any
  // rank will update it.

  int updateComm = this->Superclass::NeedToUpdateCommunicator() ? 1 : 0;

  vtkMPICommunicatorOpaqueComm* globalComm = vtkPPainterCommunicator::GetGlobalCommunicator();

  if (globalComm)
  {
    MPI_Allreduce(MPI_IN_PLACE, &updateComm, 1, MPI_INT, MPI_MAX, *globalComm->GetHandle());

    if (updateComm != 0)
    {
      this->SetUpdateAll();
    }
  }

  return updateComm != 0;
}

//------------------------------------------------------------------------------
void vtkPSurfaceLICInterface::GetGlobalMinMax(
  vtkPainterCommunicator* painterComm, float& min, float& max)
{
  vtkPPainterCommunicator* pPainterComm = dynamic_cast<vtkPPainterCommunicator*>(painterComm);

  if (pPainterComm->GetMPIInitialized())
  {
    MPI_Comm comm = *static_cast<MPI_Comm*>(pPainterComm->GetCommunicator());

    MPI_Allreduce(MPI_IN_PLACE, &min, 1, MPI_FLOAT, MPI_MIN, comm);

    MPI_Allreduce(MPI_IN_PLACE, &max, 1, MPI_FLOAT, MPI_MAX, comm);
  }
}

//------------------------------------------------------------------------------
void vtkPSurfaceLICInterface::StartTimerEvent(const char* event)
{
#if defined(vtkSurfaceLICInterfaceTIME)
  vtkParallelTimer* log = vtkParallelTimer::GetGlobalInstance();
  log->StartEvent(event);
#else
  (void)event;
#endif
}

//------------------------------------------------------------------------------
void vtkPSurfaceLICInterface::EndTimerEvent(const char* event)
{
#if defined(vtkSurfaceLICInterfaceTIME)
  vtkParallelTimer* log = vtkParallelTimer::GetGlobalInstance();
  log->EndEvent(event);
#else
  (void)event;
#endif
}

//------------------------------------------------------------------------------
void vtkPSurfaceLICInterface::WriteTimerLog(const char* fileName)
{
#if defined(vtkSurfaceLICInterfaceTIME)
  std::string fname = fileName ? fileName : "";
  if (fname == this->LogFileName)
  {
    return;
  }
  this->LogFileName = fname;
  if (!fname.empty())
  {
    vtkParallelTimer* log = vtkParallelTimer::GetGlobalInstance();
    log->SetFileName(fname.c_str());
    log->Update();
    log->Write();
  }
#else
  (void)fileName;
#endif
}

//------------------------------------------------------------------------------
vtkPainterCommunicator* vtkPSurfaceLICInterface::CreateCommunicator(int include)
{
  // if we're using MPI and it's been initialized then
  // subset VTK's world communicator otherwise run the
  // painter serially.
  vtkPPainterCommunicator* comm = new vtkPPainterCommunicator;

  vtkMPICommunicatorOpaqueComm* globalComm = vtkPPainterCommunicator::GetGlobalCommunicator();

  if (globalComm)
  {
    comm->SubsetCommunicator(globalComm, include);
  }

  return comm;
}

//------------------------------------------------------------------------------
void vtkPSurfaceLICInterface::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "LogFileName=" << this->LogFileName << endl;
}
VTK_ABI_NAMESPACE_END
