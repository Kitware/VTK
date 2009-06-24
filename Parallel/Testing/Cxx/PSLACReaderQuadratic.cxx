// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PSLACReaderQuadratic.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCompositeDataGeometryFilter.h"
#include "vtkCompositeRenderManager.h"
#include "vtkLookupTable.h"
#include "vtkMPIController.h"
#include "vtkPolyDataMapper.h"
#include "vtkPSLACReader.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTestUtilities.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

struct TestArgs
{
  int *retval;
  int argc;
  char **argv;
};

//=============================================================================
void PSLACReaderQuadratic(vtkMultiProcessController *controller, void *_args)
{
  TestArgs *args = reinterpret_cast<TestArgs *>(_args);
  int argc = args->argc;
  char **argv = args->argv;
  *(args->retval) = 1;

  // Set up reader.
  VTK_CREATE(vtkPSLACReader, reader);

  char *meshFileName = vtkTestUtilities::ExpandDataFileName(argc, argv,
                                  "Data/SLAC/ll-9cell-f523/ll-9cell-f523.ncdf");
  char *modeFileName = vtkTestUtilities::ExpandDataFileName(argc, argv,
              "Data/SLAC/ll-9cell-f523/mode0.l0.R2.457036E+09I2.778314E+04.m3");
  reader->SetMeshFileName(meshFileName);
  reader->AddModeFileName(modeFileName);

  reader->ReadInternalVolumeOff();
  reader->ReadExternalSurfaceOn();
  reader->ReadMidpointsOn();

  // Extract geometry that we can render.
  VTK_CREATE(vtkCompositeDataGeometryFilter, geometry);
  geometry->SetInputConnection(
                          reader->GetOutputPort(vtkSLACReader::SURFACE_OUTPUT));

  // Set up rendering stuff.
  VTK_CREATE(vtkPolyDataMapper, mapper);
  mapper->SetInputConnection(geometry->GetOutputPort());
  mapper->SetScalarModeToUsePointFieldData();
  mapper->ColorByArrayComponent("bfield", 1);
  mapper->UseLookupTableScalarRangeOff();
  mapper->SetScalarRange(-1e-08, 1e-08);
  // This is handled later by the ParallelRenderManager.
  // mapper->SetPiece(controller->GetLocalProcessId());
  // mapper->SetNumberOfPieces(controller->GetNumberOfProcesses());
  // mapper->Update();

  VTK_CREATE(vtkLookupTable, lut);
  lut->SetHueRange(0.66667, 0.0);
  mapper->SetLookupTable(lut);

  VTK_CREATE(vtkActor, actor);
  actor->SetMapper(mapper);

  VTK_CREATE(vtkCompositeRenderManager, prm);

  vtkSmartPointer<vtkRenderer> renderer;
  renderer.TakeReference(prm->MakeRenderer());
  renderer->AddActor(actor);
  vtkCamera *camera = renderer->GetActiveCamera();
  camera->SetPosition(-0.75, 0.0, 0.7);
  camera->SetFocalPoint(0.0, 0.0, 0.7);
  camera->SetViewUp(0.0, 1.0, 0.0);

  vtkSmartPointer<vtkRenderWindow> renwin;
  renwin.TakeReference(prm->MakeRenderWindow());
  renwin->SetSize(600, 150);
  renwin->SetPosition(0, 200*controller->GetLocalProcessId());
  renwin->AddRenderer(renderer);
  
  prm->SetRenderWindow(renwin);
  prm->SetController(controller);
  prm->InitializePieces();
  prm->InitializeOffScreen();           // Mesa GL only

  if (controller->GetLocalProcessId() == 0)
    {
    renwin->Render();

    prm->StopServices();
    // Change the time to test the periodic mode interpolation.
    vtkStreamingDemandDrivenPipeline *sdd =
      vtkStreamingDemandDrivenPipeline::SafeDownCast(geometry->GetExecutive());
    sdd->SetUpdateTimeStep(0, 3e-10);
    renwin->Render();

    // Do the test comparison.
    int retval = vtkRegressionTestImage(renwin);
    if (retval == vtkRegressionTester::DO_INTERACTOR)
      {
      VTK_CREATE(vtkRenderWindowInteractor, iren);
      iren->SetRenderWindow(renwin);
      iren->Initialize();
      iren->Start();
      retval = vtkRegressionTester::PASSED;
      }

    *(args->retval) = (retval == vtkRegressionTester::PASSED) ? 0 : 1;

    prm->StopServices();
    }
  else // not root node
    {
    prm->StartServices();
    // Change the time to test the periodic mode interpolation.
    vtkStreamingDemandDrivenPipeline *sdd =
      vtkStreamingDemandDrivenPipeline::SafeDownCast(geometry->GetExecutive());
    sdd->SetUpdateTimeStep(0, 3e-10);
    prm->StartServices();
    }

  controller->Broadcast(args->retval, 1, 0);
}

//=============================================================================
int main(int argc, char *argv[])
{
  int retval = 1;

  VTK_CREATE(vtkMPIController, controller);
  controller->Initialize(&argc, &argv);

  vtkMultiProcessController::SetGlobalController(controller);

  TestArgs args;
  args.retval = &retval;
  args.argc = argc;
  args.argv = argv;

  controller->SetSingleMethod(PSLACReaderQuadratic, &args);
  controller->SingleMethodExecute();

  controller->Finalize();

  return retval;
}
