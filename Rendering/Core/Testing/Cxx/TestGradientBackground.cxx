// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkConeSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkViewport.h"

int TestGradientBackground(int argc, char* argv[])
{
  vtkNew<vtkRenderWindow> win;
  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkRenderer> renderers[4];
  vtkViewport::GradientModes modes[4] = {
    vtkViewport::GradientModes::VTK_GRADIENT_HORIZONTAL,
    vtkViewport::GradientModes::VTK_GRADIENT_VERTICAL,
    vtkViewport::GradientModes::VTK_GRADIENT_RADIAL_VIEWPORT_FARTHEST_SIDE,
    vtkViewport::GradientModes::VTK_GRADIENT_RADIAL_VIEWPORT_FARTHEST_CORNER,
  };
  vtkNew<vtkConeSource> cone;
  vtkNew<vtkPolyDataMapper> map;
  vtkNew<vtkActor> act;

  map->SetInputConnection(cone->GetOutputPort());
  act->SetMapper(map);
  double xmins[4] = { 0.0, 0.5, 0.0, 0.0 };
  double ymins[4] = { 0.0, 0.0, 0.25, 0.5 };
  double xmaxs[4] = { 0.5, 1.0, 1.0, 1.0 };
  double ymaxs[4] = { 0.25, 0.25, 0.5, 1.0 };
  for (int i = 0; i < 4; ++i)
  {
    auto ren = renderers[i].Get();
    ren->AddActor(act);
    ren->GradientBackgroundOn();
    ren->SetGradientMode(modes[i]);
    ren->SetBackground(0.8, 0.4, 0.1);
    ren->SetBackground2(0.1, 0.4, 0.8);
    ren->SetViewport(xmins[i], ymins[i], xmaxs[i], ymaxs[i]);
    win->AddRenderer(ren);
  }
  win->SetInteractor(iren);
  win->Render();
  iren->Initialize();
  iren->UpdateSize(640, 480);

  int retVal = vtkRegressionTestImage(win);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
