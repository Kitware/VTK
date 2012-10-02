/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <mpi.h>

#include "vtkMPICommunicator.h"
#include "vtkMPIController.h"
#include "vtkSynchronizedRenderWindows.h"
#include "vtkSynchronizedRenderers.h"
#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkActor.h"
#include "vtkSphereSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkLookupTable.h"
#include "vtkCamera.h"
#include "vtkPieceScalars.h"
#include "vtkProcess.h"
#include "vtkObjectFactory.h"

class MyProcess : public vtkProcess
{
public:
  static MyProcess *New();
  vtkTypeMacro(MyProcess, vtkProcess);

  virtual void Execute();

  void SetArgs(int anArgc,
               char *anArgv[])
    {
      this->Argc=anArgc;
      this->Argv=anArgv;
    }

  void CreatePipeline(vtkRenderer* renderer)
    {
    int num_procs = this->Controller->GetNumberOfProcesses();
    int my_id = this->Controller->GetLocalProcessId();

    vtkSphereSource* sphere = vtkSphereSource::New();
    sphere->SetPhiResolution(100);
    sphere->SetThetaResolution(100);

    vtkPieceScalars *piecescalars = vtkPieceScalars::New();
    piecescalars->SetInputConnection(sphere->GetOutputPort());
    piecescalars->SetScalarModeToCellData();

    vtkPolyDataMapper* mapper = vtkPolyDataMapper::New();
    mapper->SetInputConnection(piecescalars->GetOutputPort());
    mapper->SetScalarModeToUseCellFieldData();
    mapper->SelectColorArray("Piece");
    mapper->SetScalarRange(0, num_procs-1);
    mapper->SetPiece(my_id);
    mapper->SetNumberOfPieces(num_procs);
    mapper->Update();

    vtkActor* actor = vtkActor::New();
    actor->SetMapper(mapper);
    renderer->AddActor(actor);

    actor->Delete();
    mapper->Delete();
    piecescalars->Delete();
    sphere->Delete();
    }

protected:
  MyProcess() { this->Argc = 0; this->Argv = NULL; }

  int Argc;
  char **Argv;
};

vtkStandardNewMacro(MyProcess);

void MyProcess::Execute()
{
  this->ReturnValue = 0;
  int my_id = this->Controller->GetLocalProcessId();

  vtkRenderWindow* renWin = vtkRenderWindow::New();
  renWin->DoubleBufferOn();
  vtkRenderer* renderer = vtkRenderer::New();

  renWin->AddRenderer(renderer);

  vtkSynchronizedRenderWindows* syncWindows =
    vtkSynchronizedRenderWindows::New();
  syncWindows->SetRenderWindow(renWin);
  syncWindows->SetParallelController(this->Controller);
  syncWindows->SetIdentifier(1);

  vtkSynchronizedRenderers* syncRenderers = vtkSynchronizedRenderers::New();
  syncRenderers->SetRenderer(renderer);
  syncRenderers->SetParallelController(this->Controller);
  syncRenderers->SetImageReductionFactor(3);

  this->CreatePipeline(renderer);

  if (my_id == 0)
    {
    vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);
    iren->Start();
    iren->Delete();

    this->Controller->TriggerBreakRMIs();
    this->Controller->Barrier();
    }
  else
    {
    this->Controller->ProcessRMIs();
    this->Controller->Barrier();
    }

  renderer->Delete();
  renWin->Delete();
  syncWindows->Delete();
  syncRenderers->Delete();
  this->ReturnValue = 1;
}


int main(int argc, char **argv)
{
  // This is here to avoid false leak messages from vtkDebugLeaks when
  // using mpich. It appears that the root process which spawns all the
  // main processes waits in MPI_Init() and calls exit() when
  // the others are done, causing apparent memory leaks for any objects
  // created before MPI_Init().
  MPI_Init(&argc, &argv);

  // Note that this will create a vtkMPIController if MPI
  // is configured, vtkThreadedController otherwise.
  vtkMPIController *contr = vtkMPIController::New();
  contr->Initialize(&argc, &argv, 1);

  int retVal = 1; //1==failed


  int numProcs = contr->GetNumberOfProcesses();

  if (numProcs < 2 && false)
    {
    cout << "This test requires at least 2 processes" << endl;
    contr->Delete();
    return retVal;
    }

  vtkMultiProcessController::SetGlobalController(contr);

  MyProcess *p=MyProcess::New();
  p->SetArgs(argc,argv);

  contr->SetSingleProcessObject(p);
  contr->SingleMethodExecute();

  retVal=p->GetReturnValue();

  p->Delete();
  contr->Finalize();
  contr->Delete();
  vtkMultiProcessController::SetGlobalController(0);
  return !retVal;
}
