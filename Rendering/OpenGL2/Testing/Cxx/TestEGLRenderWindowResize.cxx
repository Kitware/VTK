// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkEGLRenderWindow.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"

//------------------------------------------------------------------------------
int TestEGLRenderWindowResize(int argc, char* argv[])
{
  vtkNew<vtkEGLRenderWindow> window;
  window->SetShowWindow(true);
  window->SetUseOffScreenBuffers(false);
  window->SetSize(300, 300);

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

  return !retVal;
}
