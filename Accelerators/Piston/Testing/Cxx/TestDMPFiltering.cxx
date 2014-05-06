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
//This test demonstrates the use of Distributed Memory Parallel
//processing using the Piston library.
//
//The pipeline is created in parallel and each process is
//assigned 1 piece to process. All satellite processes send their local
//result to the first process which collects and renders them as one.

#include <mpi.h>

#include "vtkActor.h"
#include "vtkAppendPolyData.h"
#include "vtkCamera.h"
#include "vtkDataSetToPiston.h"
#include "vtkDebugLeaks.h"
#include "vtkElevationFilter.h"
#include "vtkImageMandelbrotSource.h"
#include "vtkInformation.h"
#include "vtkMPIController.h"
#include "vtkPistonContour.h"
#include "vtkPistonToDataSet.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTestUtilities.h"
#include "vtkWindowToImageFilter.h"

static const float ISO_START = 80.0;
static const float ISO_STEP = -7.0;
static const int ISO_NUM = 10;

// Just pick a tag which is available
static const int ISO_VALUE_RMI_TAG = 300;
static const int ISO_OUTPUT_TAG = 301;

struct ParallelIsoArgs_tmp
{
  int argc;
  char **argv;
  int *retVal;
};

struct ParallelIsoRMIArgs_tmp
{
  vtkPistonContour *ContourFilter;
  vtkMultiProcessController *Controller;
  vtkPistonToDataSet *P2D;
  vtkElevationFilter *elev;
};

// Callback to set the iso surface value.
void SetIsoValueRMI(void *localArg, void *vtkNotUsed(remoteArg),
                    int vtkNotUsed(remoteArgLen), int vtkNotUsed(id))
{
  ParallelIsoRMIArgs_tmp *args = (ParallelIsoRMIArgs_tmp*)localArg;

  float val;

  vtkPistonContour *iso = args->ContourFilter;
  val = iso->GetIsoValue();
  iso->SetIsoValue(val + ISO_STEP);
  args->elev->Update();

  vtkMultiProcessController *contrl = args->Controller;
  contrl->Send(args->elev->GetOutput(), 0, ISO_OUTPUT_TAG);
}

// This will be called by all processes
void MyMain(vtkMultiProcessController *controller, void *arg)
{
  int myid, numProcs;
  float val;
  ParallelIsoArgs_tmp *args = reinterpret_cast<ParallelIsoArgs_tmp*>(arg);

  // Obtain the id of the running process and the total
  // number of processes
  myid = controller->GetLocalProcessId();
  numProcs = controller->GetNumberOfProcesses();

  vtkImageMandelbrotSource *src = vtkImageMandelbrotSource::New();
  src->SetWholeExtent(0,40,0,40,0,40);

  vtkDataSetToPiston *d2p = vtkDataSetToPiston::New();
  d2p->SetInputConnection(src->GetOutputPort());

  vtkPistonContour *contour = vtkPistonContour::New();
  contour->SetInputConnection(d2p->GetOutputPort());
  contour->SetIsoValue(ISO_START);

  vtkPistonToDataSet *p2d = vtkPistonToDataSet::New();
  p2d->SetInputConnection(contour->GetOutputPort());
  p2d->SetOutputDataSetType(VTK_POLY_DATA);

  vtkElevationFilter *elev = vtkElevationFilter::New();
  elev->SetInputConnection(p2d->GetOutputPort());
  val = (myid+1) / static_cast<float>(numProcs);
  elev->SetScalarRange(val, val+0.001);

  // Tell the pipeline which piece we want to update.
  vtkStreamingDemandDrivenPipeline *exec =
    vtkStreamingDemandDrivenPipeline::SafeDownCast(elev->GetExecutive());
  exec->SetUpdateNumberOfPieces(exec->GetOutputInformation(0), numProcs);
  exec->SetUpdatePiece(exec->GetOutputInformation(0), myid);

  // Make sure all processes update at the same time.
  elev->Update();

  if (myid != 0)
    {
    // If I am not the root process
    ParallelIsoRMIArgs_tmp args2;
    args2.ContourFilter = contour;
    args2.Controller = controller;
    args2.P2D = p2d;
    args2.elev = elev;

    // Last, set up a RMI call back to change the iso surface value.
    // This is done so that the root process can let this process
    // know that it wants the contour value to change.
    controller->AddRMI(SetIsoValueRMI, (void *)&args2, ISO_VALUE_RMI_TAG);
    controller->ProcessRMIs();
    }
  else
    {
    // Create the rendering part of the pipeline
    vtkAppendPolyData *app = vtkAppendPolyData::New();
    app->UserManagedInputsOn();
    app->SetNumberOfInputs(numProcs);

    vtkRenderer *ren = vtkRenderer::New();
    vtkRenderWindow *renWindow = vtkRenderWindow::New();
    vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    vtkPolyDataMapper *mapper = vtkPolyDataMapper::New();
    vtkActor *actor = vtkActor::New();
    vtkCamera *cam = vtkCamera::New();
    renWindow->AddRenderer(ren);
    iren->SetRenderWindow(renWindow);
    renWindow->SetSize(400, 400);
    mapper->SetInputConnection(app->GetOutputPort());
    actor->SetMapper(mapper);
    ren->AddActor(actor);

    cam->SetFocalPoint(0, 0, 0);
    cam->SetPosition(10, 10, 10);
    cam->SetViewUp(0, 0, -1);
    cam->SetViewAngle(30);
    ren->SetActiveCamera(cam);

    // Loop through some iso surface values.
    for (int j = 0; j < ISO_NUM; ++j)
      {
      // Set the local value
      contour->SetIsoValue(contour->GetIsoValue() + ISO_STEP);
      elev->Update();

      for (int i = 1; i < numProcs; ++i)
        {
        // Trigger the RMI to change the iso surface value.
        controller->TriggerRMI(i, ISO_VALUE_RMI_TAG);
        }
      for (int i = 1; i < numProcs; ++i)
        {
        vtkPolyData *pd = vtkPolyData::New();
        controller->Receive(pd, i, ISO_OUTPUT_TAG);
        app->SetInputDataByNumber(i, pd);
        pd->Delete();
        }

      vtkPolyData *outputCopy = vtkPolyData::New();
      outputCopy->ShallowCopy(elev->GetOutput());
      app->SetInputDataByNumber(0, outputCopy);
      outputCopy->Delete();
      app->Update();
      ren->ResetCamera();
      renWindow->Render();
    }


    // Tell the other processors to stop processing RMIs.
    for (int i = 1; i < numProcs; ++i)
      {
      controller->TriggerRMI(i, vtkMultiProcessController::BREAK_RMI_TAG);
      }

    *(args->retVal) =
      vtkRegressionTester::Test(args->argc, args->argv, renWindow, 10);

    if (*(args->retVal) == vtkRegressionTester::DO_INTERACTOR)
      {
      iren->Start();
      }

    // Clean up
    app->Delete();
    ren->Delete();
    renWindow->Delete();
    iren->Delete();
    mapper->Delete();
    actor->Delete();
    cam->Delete();
    }

  // Clean up objects in all processes.
  src->Delete();
  d2p->Delete();
  contour->Delete();
  p2d->Delete();
  elev->Delete();
}

int TestDMPFiltering(int argc, char *argv[])
{
  // This is here to avoid false leak messages from vtkDebugLeaks when
  // using mpich. It appears that the root process which spawns all the
  // main processes waits in MPI_Init() and calls exit() when
  // the others are done, causing apparent memory leaks for any objects
  // created before MPI_Init().
  MPI_Init(&argc, &argv);

  // Note that this will create a vtkMPIController if MPI
  // is configured, vtkThreadedController otherwise.
  vtkMPIController *controller = vtkMPIController::New();

  controller->Initialize(&argc, &argv, 1);

  // Added for regression test.
  // ----------------------------------------------
  int retVal = 1;
  ParallelIsoArgs_tmp args;
  args.retVal = &retVal;
  args.argc = argc;
  args.argv = argv;
  // ----------------------------------------------

  controller->SetSingleMethod(MyMain, &args);
  controller->SingleMethodExecute();

  controller->Finalize();
  controller->Delete();

  return !retVal;
}
