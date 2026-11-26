// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Description:
// This tests reading of an SEP file using MPI.

#include "vtkPSEPReader.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkCompositeRenderManager.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkGeometryFilter.h"
#include "vtkImageData.h"
#include "vtkImageMapToColors.h"
#include "vtkLookupTable.h"
#include "vtkMPIController.h"
#include "vtkNew.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"

namespace
{
struct TestArgs
{
  int* retval;
  int argc;
  char** argv;
};
};

void TestPSEPReader(vtkMultiProcessController* controller, void* _args)
{
  ::TestArgs* args = reinterpret_cast<::TestArgs*>(_args);
  int argc = args->argc;
  char** argv = args->argv;
  *(args->retval) = 1;
  auto rank = controller->GetLocalProcessId();

  vtkNew<vtkPSEPReader> PSEPReader;

  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/small.H");
  const std::string filename = fname;
  delete[] fname;

  // Read the input image
  PSEPReader->SetFileName(filename.c_str());
  PSEPReader->SetFixedDimension1("DEPTH");
  PSEPReader->SetFixedDimensionValue1(0);
  PSEPReader->Update();

  if (!PSEPReader->CanReadFile(filename.c_str()))
  {
    vtkLog(ERROR, "Cannot read file " << PSEPReader->GetFileName());
    return;
  }

  // Check the image properties
  auto extents = PSEPReader->ComputeExtent();
  if (extents[0] != 0 || extents[1] != 4 || extents[2] != 0 || extents[3] != 4 || extents[4] != 0 ||
    extents[5] != 3)
  {
    vtkLog(ERROR, "Unexpected data extents!");
    vtkLog(ERROR, << extents[0] << " " << extents[1] << " " << extents[2] << " " << extents[3]
                  << " " << extents[4] << " " << extents[5]);
    return;
  }

  const double* origin = PSEPReader->GetDataOrigin();
  if (origin[0] != 0. || origin[1] != 0. || origin[2] != 0.)
  {
    vtkLog(ERROR, "Unexpected data origin!");
    return;
  }

  const double* spacing = PSEPReader->GetDataSpacing();
  if (spacing[0] != 1. || spacing[1] != 1. || spacing[2] != 1.)
  {
    vtkLog(ERROR, "Unexpected data spacing!");
    return;
  }

  // Visualize the grid
  double scalarRange[2];
  PSEPReader->GetOutput()->GetScalarRange(scalarRange);

  vtkNew<vtkLookupTable> table;
  table->SetRampToLinear();
  table->SetRange(scalarRange[0], scalarRange[1]);
  table->SetValueRange(0.0, 4);
  table->SetSaturationRange(0.0, 0.0);
  table->SetAlphaRange(1.0, 1.0);
  table->Build();

  vtkNew<vtkImageMapToColors> colors;
  colors->SetInputConnection(PSEPReader->GetOutputPort());
  colors->SetLookupTable(table);

  vtkNew<vtkDataSetSurfaceFilter> surface;
  surface->SetInputConnection(colors->GetOutputPort());
  surface->Update();

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputData(surface->GetOutput());
  mapper->ScalarVisibilityOn();
  mapper->SelectColorArray("ImageScalars");
  mapper->SetColorModeToMapScalars();

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->EdgeVisibilityOn();

  vtkNew<vtkRenderer> ren;
  ren->SetBackground(0, 0, 0);
  ren->AddActor(actor);
  ren->ResetCamera();
  double z1 = ren->GetActiveCamera()->GetPosition()[2];
  ren->GetActiveCamera()->SetPosition(0.25 * z1, 0.25 * z1, 0.5 * z1);
  ren->GetActiveCamera()->SetFocalPoint(0., 0., 0.);
  ren->ResetCamera();

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(300, 300);
  renWin->AddRenderer(ren);

  vtkNew<vtkCompositeRenderManager> prm;

  prm->SetRenderWindow(renWin);
  prm->SetController(controller);
  prm->InitializePieces();
  prm->InitializeOffScreen(); // Mesa GL only

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  if (rank == 0)
  {
    prm->ResetAllCameras();
    renWin->Render();

    // Do the test comparison
    int retval = vtkRegressionTestImage(renWin);
    if (retval == vtkRegressionTester::DO_INTERACTOR)
    {
      prm->StartInteractor();
      retval = vtkRegressionTester::PASSED;
    }
    *(args->retval) = (retval == vtkRegressionTester::PASSED) ? 0 : 1;

    prm->StopServices();
  }
  else // not root node
  {
    prm->StartServices();
  }

  controller->Broadcast(args->retval, 1, 0);
}

//------------------------------------------------------------------------------
int TestPSEPReader(int argc, char* argv[])
{
  int retval = 1;
  vtkNew<vtkMPIController> controller;
  controller->Initialize(&argc, &argv);

  vtkMultiProcessController::SetGlobalController(controller);

  ::TestArgs args;
  args.retval = &retval;
  args.argc = argc;
  args.argv = argv;

  controller->SetSingleMethod(TestPSEPReader, &args);
  controller->SingleMethodExecute();

  controller->Finalize();

  return retval;
}
