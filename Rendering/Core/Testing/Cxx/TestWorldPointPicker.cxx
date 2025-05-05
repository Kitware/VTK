// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkRegressionTestImage.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkWorldPointPicker.h"

int TestWorldPointPicker(int argc, char* argv[])
{

  vtkNew<vtkSphereSource> sphere;

  vtkNew<vtkPolyDataMapper> sphereMapper;
  sphereMapper->SetInputConnection(sphere->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(sphereMapper);

  vtkNew<vtkRenderer> ren;
  ren->AddActor(actor);
  vtkNew<vtkRenderWindow> win;
  win->SetMultiSamples(0);
  win->AddRenderer(ren);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(win);

  ren->SetBackground(0.0, 0.0, 0.0);
  win->SetSize(450, 450);
  win->Render();
  ren->GetActiveCamera()->Zoom(1.2);

  // Setup picker
  vtkNew<vtkWorldPointPicker> picker;
  // Set picker in the interactor to test the picker's serialization
  iren->SetPicker(picker);

  win->Render();
  picker->Pick(140, 140, 0, ren);
  win->Render();

  int retVal = vtkRegressionTestImage(win);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  // Verify pick
  double* coords = picker->GetPickPosition();
  double expected[3] = { -0.246502, -0.246502, 0.347118 };
  double epsilon = 0.0001;
  cout << "\nPicked Coords: (" << coords[0] << ", " << coords[1] << ", " << coords[2] << ")\n";
  if (fabs(expected[0] - coords[0]) > epsilon || fabs(expected[1] - coords[1]) > epsilon ||
    fabs(expected[2] - coords[2]) > epsilon)
  {
    cerr << "Incorrect pick coordinates! (if any picks were performed inter"
            "actively this could be ignored).\n";
    return EXIT_FAILURE;
  }

  return !retVal;
}
