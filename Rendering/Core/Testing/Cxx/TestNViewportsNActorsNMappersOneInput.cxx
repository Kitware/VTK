// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkConeSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTesting.h"

#include <cstdlib>

// In this unit test, there are 4 viewports. Each viewport displays an actor
// that is connected to a mapper. All mappers share a common cone source.
int TestNViewportsNActorsNMappersOneInput(int argc, char* argv[])
{
  vtkNew<vtkRenderWindow> renderWindow;
  vtkNew<vtkRenderWindowInteractor> interactor;
  vtkNew<vtkRenderer> renderers[4];
  vtkNew<vtkConeSource> cone;

  double xmins[4] = { 0.0, 0.4, 0.0, 0.0 };
  double ymins[4] = { 0.0, 0.0, 0.25, 0.5 };
  double xmaxs[4] = { 0.4, 1.0, 1.0, 1.0 };
  double ymaxs[4] = { 0.25, 0.25, 0.5, 1.0 };
  for (int i = 0; i < 4; ++i)
  {
    auto ren = renderers[i].Get();
    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputConnection(cone->GetOutputPort());
    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);
    actor->GetProperty()->SetDiffuseColor(0.5, i / 4.0, (4.0 - i) / 4);
    ren->AddActor(actor);
    ren->SetBackground(i / 4.0, (4.0 - i) / 4, 1);
    ren->SetViewport(xmins[i], ymins[i], xmaxs[i], ymaxs[i]);
    renderWindow->AddRenderer(ren);
  }
  renderWindow->SetSize(800, 800);
  renderWindow->SetInteractor(interactor);
  interactor->Initialize();

  const int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    interactor->Start();
  }
  return !retVal;
}
