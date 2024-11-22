// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"

#include "vtkRenderWindowInteractor.h"

//------------------------------------------------------------------------------
int TestRenderPointsAsSpheres(int argc, char* argv[])
{
  bool useParallelProjection = false;
  for (int i = 0; i < argc; ++i)
  {
    if (argv[i] && !strcmp(argv[i], "--ortho"))
    {
      useParallelProjection = true;
    }
  }

  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.0, 0.0, 0.0);

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(300, 300);
  renderWindow->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);

  vtkNew<vtkSphereSource> sphere;
  sphere->SetThetaResolution(16);
  sphere->SetPhiResolution(16);
  sphere->SetEndTheta(270.0);

  {
    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputConnection(sphere->GetOutputPort());
    vtkNew<vtkActor> actor;
    renderer->AddActor(actor);
    actor->SetMapper(mapper);
    actor->GetProperty()->SetDiffuseColor(0.4, 1.0, 1.0);
    vtkNew<vtkProperty> backProp;
    backProp->SetDiffuseColor(0.4, 0.65, 0.8);
    actor->SetBackfaceProperty(backProp);

    actor->GetProperty()->VertexVisibilityOn();
    actor->GetProperty()->SetVertexColor(1.0, 0.5, 1.0);
    actor->GetProperty()->SetPointSize(14.0);
    actor->GetProperty()->RenderPointsAsSpheresOn();
  }

  renderWindow->SetMultiSamples(0);
  renderer->ResetCamera();
  renderer->GetActiveCamera()->SetParallelProjection(useParallelProjection);
  renderer->GetActiveCamera()->Elevation(-45);
  renderer->GetActiveCamera()->OrthogonalizeViewUp();
  renderer->GetActiveCamera()->Zoom(1.5);
  renderer->ResetCameraClippingRange();
  renderWindow->Render();

  int retVal = vtkRegressionTestImageThreshold(renderWindow, 0.05);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
