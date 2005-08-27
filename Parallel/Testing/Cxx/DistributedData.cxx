/*=========================================================================

  Program:   Visualization Toolkit
  Module:    DistributedData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Test of vtkDistributedDataFilter and supporting classes, covering as much 
// code as possible.  This test requires 4 MPI processes.
//
// To cover ghost cell creation, use vtkDataSetSurfaceFilter.
//
// To cover clipping code:  SetBoundaryModeToSplitBoundaryCells()
//
// To run fast redistribution: SetUseMinimalMemoryOff() (Default)
// To run memory conserving code instead: SetUseMinimalMemoryOn()

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"
#include "vtkParallelFactory.h"
#include "vtkCompositeRenderManager.h"
#include "vtkDataSetReader.h"
#include "vtkUnstructuredGrid.h"
#include "vtkDistributedDataFilter.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkPieceScalars.h"
#include "vtkMultiProcessController.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkActor.h"
#include "vtkPolyDataMapper.h"
#include "vtkCamera.h"
/*
** This test only builds if MPI is in use
*/
#include "vtkMPICommunicator.h"

static int NumProcs, Me;

struct DDArgs_tmp
{
  int* retVal;
  int argc;
  char** argv;
};

static void Run(vtkMultiProcessController *contr, void *arg)
{
  int i, go;
  DDArgs_tmp *args = reinterpret_cast<DDArgs_tmp *>(arg);

  vtkCompositeRenderManager *prm = vtkCompositeRenderManager::New();

  // READER

  vtkDataSetReader *dsr = vtkDataSetReader::New();
  vtkUnstructuredGrid *ug = vtkUnstructuredGrid::New();

  vtkDataSet *ds = NULL;

  if (Me == 0)
    {
    char* fname = 
      vtkTestUtilities::ExpandDataFileName(
        args->argc, args->argv, "Data/tetraMesh.vtk");

    dsr->SetFileName(fname);

    ds = dsr->GetOutput();

    dsr->Update();
    go = 1;

    if ((ds == NULL) || (ds->GetNumberOfCells() == 0))
      {
      if (ds) cout << "Failure: input file has no cells" << endl;
      go = 0;
      }
    }
  else
    {
    ds = (vtkDataSet *)ug;
    }

  vtkMPICommunicator *comm =
    vtkMPICommunicator::SafeDownCast(contr->GetCommunicator());

  comm->Broadcast(&go, 1, 0);

  if (!go){
    dsr->Delete();
    ug->Delete();
    prm->Delete();
    return;
  }

  // DATA DISTRIBUTION FILTER

  vtkDistributedDataFilter *dd = vtkDistributedDataFilter::New();

  dd->SetInput(ds);
  dd->SetController(contr);

  dd->SetBoundaryModeToSplitBoundaryCells();  // clipping
  dd->UseMinimalMemoryOff();

  // COLOR BY PROCESS NUMBER

  vtkPieceScalars *ps = vtkPieceScalars::New();
  ps->SetInput((vtkDataSet *)dd->GetOutput());
  ps->SetScalarModeToCellData();

  // MORE FILTERING - this will request ghost cells

  vtkDataSetSurfaceFilter *dss = vtkDataSetSurfaceFilter::New();
  dss->SetInput(ps->GetOutput());

  // COMPOSITE RENDER

  vtkPolyDataMapper *mapper = vtkPolyDataMapper::New();
  mapper->SetInput(dss->GetOutput());

  mapper->SetColorModeToMapScalars();
  mapper->SetScalarModeToUseCellFieldData();
  mapper->SelectColorArray("Piece");
  mapper->SetScalarRange(0, NumProcs-1);

  vtkActor *actor = vtkActor::New();
  actor->SetMapper(mapper);

  vtkRenderer *renderer = prm->MakeRenderer();
  renderer->AddActor(actor);

  vtkRenderWindow *renWin = prm->MakeRenderWindow();
  renWin->AddRenderer(renderer);

  renderer->SetBackground(0,0,0);
  renWin->SetSize(300,300);
  renWin->SetPosition(0, 360*Me);

  prm->SetRenderWindow(renWin);
  prm->SetController(contr);

  prm->InitializeOffScreen();   // Mesa GL only

  // We must update the whole pipeline here, otherwise node 0
  // goes into GetActiveCamera which updates the pipeline, putting
  // it into vtkDistributedDataFilter::Execute() which then hangs.
  // If it executes here, dd will be up-to-date won't have to 
  // execute in GetActiveCamera.

  mapper->SetPiece(Me);
  mapper->SetNumberOfPieces(NumProcs);
  mapper->Update();

  if (Me == 0)
    {
    renderer->ResetCamera();
    vtkCamera *camera = renderer->GetActiveCamera();
    camera->UpdateViewport(renderer);
    camera->ParallelProjectionOn();
    camera->SetParallelScale(16);

    renWin->Render();
    renWin->Render();

    *(args->retVal) = vtkRegressionTester::Test(args->argc, args->argv, renWin, 10);

    for (i=1; i < NumProcs; i++)
      {
      contr->Send(args->retVal, 1, i, 0x11);
      }

    prm->StopServices();
    }
  else
    {
    prm->StartServices();
    contr->Receive(args->retVal, 1, 0, 0x11);
    }

  if (*(args->retVal) == vtkTesting::PASSED)
    {
    // Now try using the memory conserving *Lean methods.  The
    // image produced should be identical

    dd->UseMinimalMemoryOn();
    mapper->SetPiece(Me);
    mapper->SetNumberOfPieces(NumProcs);
    mapper->Update();

    if (Me == 0)
      {
      renderer->ResetCamera();
      vtkCamera *camera = renderer->GetActiveCamera();
      camera->UpdateViewport(renderer);
      camera->ParallelProjectionOn();
      camera->SetParallelScale(16);
  
      renWin->Render();
      renWin->Render();
  
      *(args->retVal) = vtkRegressionTester::Test(args->argc, args->argv, renWin, 10);
  
      for (i=1; i < NumProcs; i++)
        {
        contr->Send(args->retVal, 1, i, 0x11);
        }
  
      prm->StopServices();
      }
    else
      {
      prm->StartServices();
      contr->Receive(args->retVal, 1, 0, 0x11);
      }
    }

  // CLEAN UP 

  mapper->Delete();
  actor->Delete();
  renderer->Delete();
  renWin->Delete();

  dd->Delete();
  dsr->Delete();
  ug->Delete();

  ps->Delete();
  dss->Delete();

  prm->Delete();
}

int main(int argc, char **argv)
{
  int retVal = 1;

  vtkMultiProcessController *contr = vtkMultiProcessController::New();
  contr->Initialize(&argc, &argv);

  vtkMultiProcessController::SetGlobalController(contr);

  NumProcs = contr->GetNumberOfProcesses();
  Me = contr->GetLocalProcessId();

  if (NumProcs != 2)
    {
    if (Me == 0)
      {
      cout << "DistributedData test requires 2 processes" << endl;
      }
    contr->Delete();
    return retVal;
    }

  if (!contr->IsA("vtkMPIController"))
    {
    if (Me == 0)
      {
      cout << "DistributedData test requires MPI" << endl;
      }
    contr->Delete();
    return retVal;   // is this the right error val?   TODO
    }

  // ----------------------------------------------
  DDArgs_tmp args;
  args.retVal = &retVal;
  args.argc = argc;
  args.argv = argv;
  // ---------------------------------------------

  contr->SetSingleMethod(Run, &args);
  contr->SingleMethodExecute();

  contr->Finalize();
  contr->Delete();

  return !retVal;
}

