// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkCompositeDataGeometryFilter.h>
#include <vtkIOSSReader.h>
#include <vtkNew.h>
#include <vtkPolyDataMapper.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkTestUtilities.h>

#include "vtkAnariRenderWindow.h"
#include "vtkAnariRenderer.h"
#include "vtkAnariTestUtilities.h"
#include "vtkMultiBaselineRegressionTest.h"

/**
 * This Test evaluates if the Anari backend support temporal dataset by
 * reading the can.ex2 file and changes to some timesteps.
 */
int TestAnariPolyDataTemporal(int argc, char* argv[])
{
  bool useDebugDevice = false;

  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "--trace"))
    {
      useDebugDevice = true;
    }
  }

  // Read the temporal dataset
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/can.ex2");
  vtkNew<vtkIOSSReader> reader;
  reader->SetFileName(fname);
  delete[] fname;

  reader->UpdateInformation();

  vtkNew<vtkCompositeDataGeometryFilter> geom;
  geom->SetInputConnection(reader->GetOutputPort());

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(geom->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->SetBackground(0.1, 0.2, 0.4);

  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);

  vtkAnariTestUtilities::SetParameterDefaults(renWin, useDebugDevice, "TestAnariPolyDataTemporal");

  // Reset camera and render first timestep
  renderer->ResetCamera();
  renderer->GetActiveCamera()->Azimuth(90);
  renderer->GetActiveCamera()->Roll(90);

  // Update to first timestep and render
  geom->UpdateTimeStep(0.0);
  renWin->Render();
  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    vtkNew<vtkRenderWindowInteractor> iren;
    iren->SetRenderWindow(renWin);
    iren->Start();
  }

  // Test the second timestep rendering
  geom->UpdateTimeStep(0.002);
  renWin->Render();
  retVal &= vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    vtkNew<vtkRenderWindowInteractor> iren;
    iren->SetRenderWindow(renWin);
    iren->Start();
  }

  return !retVal;
}
