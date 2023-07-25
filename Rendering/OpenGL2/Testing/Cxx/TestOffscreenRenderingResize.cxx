// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"

//------------------------------------------------------------------------------
// Test one can create an resize offscreen render windows.
int TestOffscreenRenderingResize(int argc, char* argv[])
{
  vtkNew<vtkRenderWindow> window;
  window->SetShowWindow(false);
  window->SetUseOffScreenBuffers(true);
  window->SetSize(300, 300);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(window);

  vtkNew<vtkRenderer> ren;
  ren->SetBackground(0.3, 0.3, 0.3);
  window->AddRenderer(ren);

  vtkNew<vtkSphereSource> sphere;
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(sphere->GetOutputPort(0));
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  ren->AddActor(actor);

  ren->ResetCamera();
  window->Render();

  window->SetSize(400, 300);
  window->Render();
  int retVal = vtkRegressionTestImage(window);
  if (retVal == vtkTesting::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
