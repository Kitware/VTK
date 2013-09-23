/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPSurfaceLICPainter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPSurfaceLICPainter.h"

#include "vtkObjectFactory.h"
#include "vtkPainterCommunicator.h"
#include "vtkPPainterCommunicator.h"
#include "vtkMPI.h"
#include "vtkParallelTimer.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPSurfaceLICPainter);

//----------------------------------------------------------------------------
vtkPSurfaceLICPainter::vtkPSurfaceLICPainter()
{}

//----------------------------------------------------------------------------
vtkPSurfaceLICPainter::~vtkPSurfaceLICPainter()
{
  #ifdef vtkPSurfaceLICPainterDEBUG
  cerr << "=====vtkPSurfaceLICPainter::~vtkPSurfaceLICPainter" << endl;
  #endif
}

//----------------------------------------------------------------------------
bool vtkPSurfaceLICPainter::NeedToUpdateCommunicator()
{
  // TODO -- with slice widget in PV the input dataset
  // MTime is changing at different rates on different
  // MPI ranks. Because of this some ranks want to update
  // there comunicator while other do not. To work around
  // this force the communicator update on all ranks if any
  // rank will update it.

  int updateComm = this->Superclass::NeedToUpdateCommunicator() ? 1 : 0;

  vtkMPICommunicatorOpaqueComm *globalComm
    = vtkPPainterCommunicator::GetGlobalCommunicator();

  if (globalComm)
    {
    MPI_Allreduce(
         MPI_IN_PLACE,
         &updateComm,
         1,
         MPI_INT,
         MPI_MAX,
         *globalComm->GetHandle());

    if (updateComm != 0)
      {
      this->SetUpdateAll();
      }
    }

  return updateComm != 0;
}

// ----------------------------------------------------------------------------
void vtkPSurfaceLICPainter::GetGlobalMinMax(
      vtkPainterCommunicator *painterComm,
      float &min,
      float &max)
{
  vtkPPainterCommunicator *pPainterComm
    = dynamic_cast<vtkPPainterCommunicator*>(painterComm);

  if (pPainterComm->GetMPIInitialized())
    {
    MPI_Comm comm
      = *static_cast<MPI_Comm*>(pPainterComm->GetCommunicator());

    MPI_Allreduce(
          MPI_IN_PLACE,
          &min,
          1,
          MPI_FLOAT,
          MPI_MIN,
          comm);

    MPI_Allreduce(
          MPI_IN_PLACE,
          &max,
          1,
          MPI_FLOAT,
          MPI_MAX,
          comm);
    }
}

//-----------------------------------------------------------------------------
void vtkPSurfaceLICPainter::StartTimerEvent(const char *event)
{
  #if defined(vtkSurfaceLICPainterTIME)
  vtkParallelTimer *log = vtkParallelTimer::GetGlobalInstance();
  log->StartEvent(event);
  #else
  (void)event;
  #endif
}

//-----------------------------------------------------------------------------
void vtkPSurfaceLICPainter::EndTimerEvent(const char *event)
{
  #if defined(vtkSurfaceLICPainterTIME)
  vtkParallelTimer *log = vtkParallelTimer::GetGlobalInstance();
  log->EndEvent(event);
  #else
  (void)event;
  #endif
}

//----------------------------------------------------------------------------
void vtkPSurfaceLICPainter::WriteTimerLog(const char *fileName)
{
  #if defined(vtkSurfaceLICPainterTIME)
  std::string fname = fileName?fileName:"";
  if (fname == this->LogFileName)
    {
    return;
    }
  this->LogFileName = fname;
  if (!fname.empty())
    {
    vtkParallelTimer *log = vtkParallelTimer::GetGlobalInstance();
    log->SetFileName(fname.c_str());
    log->Update();
    log->Write();
    }
  #else
  (void)fileName;
  #endif
}

//----------------------------------------------------------------------------
vtkPainterCommunicator *vtkPSurfaceLICPainter::CreateCommunicator(int include)
{
  // if we're using MPI and it's been initialized then
  // subset VTK's world communicator otherwise run the
  // painter serially.
  vtkPPainterCommunicator *comm = new vtkPPainterCommunicator;

  vtkMPICommunicatorOpaqueComm *globalComm
    = vtkPPainterCommunicator::GetGlobalCommunicator();

  if (globalComm)
    {
    comm->SubsetCommunicator(globalComm, include);
    }

  return comm;
}

//----------------------------------------------------------------------------
void vtkPSurfaceLICPainter::PrintSelf(ostream & os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "LogFileName=" << this->LogFileName << endl;
}
