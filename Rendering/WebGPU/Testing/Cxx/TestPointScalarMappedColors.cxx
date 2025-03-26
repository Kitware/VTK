// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkElevationFilter.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkNew.h"
#include "vtkObject.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkScalarsToColors.h"
#include "vtkSphereSource.h"

int TestPointScalarMappedColors(int argc, char* argv[])
{
  vtkNew<vtkSphereSource> sphere;
  sphere->SetRadius(1.0);
  sphere->SetCenter(0, 0, 0);

  // generate elevation data.
  vtkNew<vtkElevationFilter> elev;
  elev->SetInputConnection(sphere->GetOutputPort());

  // map elevation output to graphics primitives.
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(elev->GetOutputPort());
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  mapper->SetScalarModeToUsePointData();
  mapper->Update();
  mapper->SetStatic(1);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetWindowName(__func__);
  renWin->SetMultiSamples(0);

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->ResetCamera();
  renderer->SetBackground(0.2, 0.3, 0.4);
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkNew<vtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style);
  style->SetDefaultRenderer(renderer);

  renWin->Render();

  iren->Start();
  return 0;
}
