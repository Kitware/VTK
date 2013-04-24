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
//rendering with Piston.
//
//The pipeline is created in parallel and each process is
//assigned 1 piece to process. Each node then renders its local image and
//the image results are depth composited to produce a correct image on the
//root node.

#include <mpi.h>

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCompositeRenderManager.h"
#include "vtkContourFilter.h"
#include "vtkDataSetToPiston.h"
#include "vtkImageMandelbrotSource.h"
#include "vtkLookupTable.h"
#include "vtkMPICommunicator.h"
#include "vtkMPIController.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkPieceScalars.h"
#include "vtkPistonContour.h"
#include "vtkPistonMapper.h"
#include "vtkPolyDataMapper.h"
#include "vtkProcess.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkSynchronizedRenderWindows.h"
#include "vtkSynchronizedRenderers.h"
#include "vtkTestUtilities.h"

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

    vtkSmartPointer<vtkImageMandelbrotSource> src =
      vtkSmartPointer<vtkImageMandelbrotSource>::New();
    vtkSmartPointer<vtkDataSetToPiston> d2p =
      vtkSmartPointer<vtkDataSetToPiston>::New();
    vtkSmartPointer<vtkPistonContour> contour =
      vtkSmartPointer<vtkPistonContour>::New();
    vtkSmartPointer<vtkContourFilter> vtkcontour =
      vtkSmartPointer<vtkContourFilter>::New();

    vtkSmartPointer<vtkSphereSource> sphere =
      vtkSmartPointer<vtkSphereSource>::New();
    vtkSmartPointer<vtkPieceScalars> piecescalars =
      vtkSmartPointer<vtkPieceScalars>::New();
    vtkSmartPointer<vtkPistonMapper> mapper =
      vtkSmartPointer<vtkPistonMapper>::New();
    vtkSmartPointer<vtkPolyDataMapper> pdm =
      vtkSmartPointer<vtkPolyDataMapper>::New();
    vtkSmartPointer<vtkActor> actor =
      vtkSmartPointer<vtkActor>::New();

#define TESTPISTON 1
#define TESTUNSTRUCTURED 1

#if TESTPISTON
#if TESTUNSTRUCTURED
    sphere->SetPhiResolution(100);
    sphere->SetThetaResolution(100);
    piecescalars->SetInputConnection(sphere->GetOutputPort());
    piecescalars->SetScalarModeToCellData();
    d2p->SetInputConnection(piecescalars->GetOutputPort());

    mapper->SetInputConnection(d2p->GetOutputPort());
    mapper->SetPiece(my_id);
    mapper->SetNumberOfPieces(num_procs);

#else // TESTSTRUCTURED
    src->SetWholeExtent(0,40,0,40,0,40);
    d2p->SetInputConnection(src->GetOutputPort());
    contour->SetInputConnection(d2p->GetOutputPort());
    contour->SetIsoValue(50.0);

    mapper->SetInputConnection(contour->GetOutputPort());
    mapper->SetPiece(my_id);
    mapper->SetNumberOfPieces(num_procs);

#endif
    mapper->Update(); // TODO: shouldn't need this
    actor->SetMapper(mapper);
#else // TESTPISTON

#if TESTUNSTRUCTURED
    sphere->SetPhiResolution(100);
    sphere->SetThetaResolution(100);
    piecescalars->SetInputConnection(sphere->GetOutputPort());
    piecescalars->SetScalarModeToCellData();

    pdm->SetInputConnection(piecescalars->GetOutputPort());
    pdm->SetScalarModeToUseCellFieldData();
    pdm->SelectColorArray("Piece");
    pdm->SetScalarRange(0, num_procs-1);
    pdm->SetPiece(my_id);
    pdm->SetNumberOfPieces(num_procs);

#else // TESTSTRUCTURED
    src->SetWholeExtent(0,40,0,40,0,40);
    vtkcontour->SetInputConnection(src->GetOutputPort());
    vtkcontour->SetNumberOfContours(1);
    vtkcontour->SetValue(0, 50.0);

    pdm->SetInputConnection(vtkcontour->GetOutputPort());
    pdm->SetPiece(my_id);
    pdm->SetNumberOfPieces(num_procs);

#endif

    pdm->Update(); //TODO: Why is this needed?
    actor->SetMapper(pdm);
#endif

    renderer->AddActor(actor);
    }

protected:
  MyProcess() { this->Argc = 0; this->Argv = NULL; }

  int Argc;
  char **Argv;
};

//#vtkCxxRevisionMacro(MyProcess, "1.0");
vtkStandardNewMacro(MyProcess);

void MyProcess::Execute()
{
  int my_id = this->Controller->GetLocalProcessId();

  vtkMPICommunicator *comm =
    vtkMPICommunicator::SafeDownCast(this->Controller->GetCommunicator());
  comm->Barrier();

  // TODO: Update to utkarsh's new architecture
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

  // TODO: Add an argument to decide if use interop or not
  vtkPistonMapper::InitCudaGL(renWin);

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
