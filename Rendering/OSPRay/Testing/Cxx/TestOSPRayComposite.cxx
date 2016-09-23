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
//This test demonstrates the use of sort last distributed memory parallel
//rendering with OSPRay.
//
//The pipeline is created in parallel and each process is
//assigned 1 piece to process. Each node then renders its local image and
//the image results are depth composited to produce a correct image on the
//root node.

#include <mpi.h>

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCompositeRenderManager.h"
#include "vtkMPICommunicator.h"
#include "vtkMPIController.h"
#include "vtkObjectFactory.h"
#include "vtkOSPRayPass.h"
#include "vtkPolyDataMapper.h"
#include "vtkProcess.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkSphereSource.h"

class MyProcess : public vtkProcess
{
public:
  static MyProcess *New();
  vtkTypeMacro(MyProcess, vtkProcess);

  virtual void Execute();

  void SetArgs(int argc, char *argv[])
  {
      this->Argc = argc;
      this->Argv = argv;
  }

  void CreatePipeline(vtkRenderer *renderer)
  {
    int num_procs = this->Controller->GetNumberOfProcesses();
    int my_id = this->Controller->GetLocalProcessId();

    vtkSmartPointer<vtkPolyDataMapper> pdm =
      vtkSmartPointer<vtkPolyDataMapper>::New();
    vtkSmartPointer<vtkActor> actor =
      vtkSmartPointer<vtkActor>::New();

    vtkSmartPointer<vtkSphereSource> src =
      vtkSmartPointer<vtkSphereSource>::New();
    src->SetPhiResolution(100);
    src->SetThetaResolution(100);
    float pFrac = (float)(my_id/(float)(num_procs-1));
    actor->GetProperty()->SetColor(0,pFrac,1.0-pFrac);
    pdm->SetInputConnection(src->GetOutputPort());
    pdm->SetPiece(my_id);
    pdm->SetNumberOfPieces(num_procs);
    actor->SetMapper(pdm);
    renderer->AddActor(actor);
  }

protected:
  MyProcess() { this->Argc = 0; this->Argv = NULL; }

  int Argc;
  char **Argv;
};

vtkStandardNewMacro(MyProcess);

void MyProcess::Execute()
{
  int my_id = this->Controller->GetLocalProcessId();

  vtkMPICommunicator *comm =
    vtkMPICommunicator::SafeDownCast(this->Controller->GetCommunicator());
  comm->Barrier();

  vtkSmartPointer<vtkCompositeRenderManager> prm =
    vtkSmartPointer<vtkCompositeRenderManager>::New();
  vtkRenderer *renderer = prm->MakeRenderer();
  vtkRenderWindow *renWin = prm->MakeRenderWindow();
  renWin->AddRenderer(renderer);
  renWin->DoubleBufferOn();
  renWin->SetMultiSamples(0);
  vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);
  renWin->Render();

  vtkCamera *cam = renderer->GetActiveCamera();
  cam->SetPosition(0,0.2,1);

  vtkSmartPointer<vtkOSPRayPass> ospray=vtkSmartPointer<vtkOSPRayPass>::New();
  renderer->SetPass(ospray);

  this->CreatePipeline(renderer);
  prm->SetRenderWindow(renWin);
  prm->SetController(this->Controller);

  if (my_id == 0)
  {
    prm->ResetAllCameras();

    this->ReturnValue =
      vtkRegressionTester::Test(this->Argc, this->Argv, renWin, 10);
    if (this->ReturnValue == vtkRegressionTester::DO_INTERACTOR)
    {
      renWin->Render();
      prm->StartInteractor();
    }

    this->Controller->TriggerBreakRMIs();
    this->Controller->Barrier();
  }
  else
  {
    prm->StartServices();
    this->Controller->Barrier();

    // No testing is done here so mark it passed
    this->ReturnValue = vtkTesting::PASSED;
  }

  renderer->Delete();
  renWin->Delete();
  iren->Delete();
}


int TestOSPRayComposite(int argc, char *argv[])
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

  int retVal = 1; // 1 == failed

  int numProcs = contr->GetNumberOfProcesses();

  if (numProcs < 2 && false)
  {
    cout << "This test requires at least 2 processes" << endl;
    contr->Delete();
    return retVal;
  }

  vtkMultiProcessController::SetGlobalController(contr);

  MyProcess *p = MyProcess::New();
  p->SetArgs(argc, argv);

  contr->SetSingleProcessObject(p);
  contr->SingleMethodExecute();

  retVal = p->GetReturnValue();

  p->Delete();
  contr->Finalize();
  contr->Delete();
  vtkMultiProcessController::SetGlobalController(0);
  return !retVal;
}
