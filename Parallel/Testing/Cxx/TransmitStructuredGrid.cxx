/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TransmitStructuredGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Test of vtkTransmitStructuredGrid and vtkExtractStructuredGrid.

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"
#include "vtkParallelFactory.h"
#include "vtkMPIController.h"

#include "vtkStructuredGrid.h"
#include "vtkStructuredGridReader.h"
#include "vtkDataObject.h"
#include "vtkTransmitStructuredGridPiece.h"
#include "vtkContourFilter.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkElevationFilter.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkCompositeRenderManager.h"
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

  vtkStructuredGridReader *sgr = vtkStructuredGridReader::New();
  vtkStructuredGrid *sg = vtkStructuredGrid::New();

  if (Me == 0)
    {
    char* fname = 
      vtkTestUtilities::ExpandDataFileName(
        args->argc, args->argv, "Data/SampleStructGrid.vtk");

    sgr->SetFileName(fname);

    sg = sgr->GetOutput();

    sgr->Update();
    go = 1;

    if ((sg == NULL) || (sg->GetNumberOfCells() == 0))
      {
      if (sg) cout << "Failure: input file has no cells" << endl;
      go = 0;
      }
    }
  else
    {
    }

  vtkMPICommunicator *comm =
    vtkMPICommunicator::SafeDownCast(contr->GetCommunicator());

  comm->Broadcast(&go, 1, 0);

  if (!go){
    sgr->Delete();
    sg->Delete();
    prm->Delete();
    return;
  }

  // FILTER WE ARE TRYING TO TEST
  vtkTransmitStructuredGridPiece *pass = vtkTransmitStructuredGridPiece::New();
  pass->SetController(contr);
  if (Me == 0)
    {
    pass->SetInput((vtkDataObject*)sg);
    }
  else 
    {
    }

  // FILTERING
  vtkContourFilter *cf = vtkContourFilter::New();
  cf->SetInput((vtkDataObject*)pass->GetOutput());
  cf->SetNumberOfContours(1);
  cf->SetValue(0,0.7);
  ((vtkDataObject*)cf->GetInput())->RequestExactExtentOn();
  cf->ComputeNormalsOff();
  vtkElevationFilter *elev = vtkElevationFilter::New();
  elev->SetInput(cf->GetOutput());
  elev->SetScalarRange(Me, Me + .001);

  // COMPOSITE RENDER
  vtkPolyDataMapper *mapper = vtkPolyDataMapper::New();
  mapper->SetInput(vtkPolyData::SafeDownCast(elev->GetOutput()));
  mapper->SetScalarRange(0, NumProcs);
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
  if (Me == 0)
    {
    prm->ResetAllCameras();
    } 

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
    vtkCamera *camera = renderer->GetActiveCamera();
    camera->UpdateViewport(renderer);
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

  // CLEAN UP 
  renWin->Delete(); 
  renderer->Delete(); 
  actor->Delete(); 
  mapper->Delete(); 
  elev->Delete(); 
  cf->Delete(); 
  pass->Delete();
  sgr->Delete();
  prm->Delete();
}

int main(int argc, char **argv)
{
  int retVal = 1;

  vtkMPIController *contr = vtkMPIController::New();
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

