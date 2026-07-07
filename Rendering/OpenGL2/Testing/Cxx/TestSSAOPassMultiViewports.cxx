// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Regression test for vtkSSAOPass applied to renderers that do not fill the
// whole window, i.e. that have a nonzero viewport origin (as with subplots).

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkNew.h"
#include "vtkOpenGLRenderer.h"
#include "vtkPlaneSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderStepsPass.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSSAOPass.h"
#include "vtkSphereSource.h"

#include <array>
#include <cstdlib>

int TestSSAOPassMultiViewports(int argc, char* argv[])
{
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(800, 800);
  renderWindow->SetMultiSamples(0);

  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(renderWindow);

  // Viewport definitions (xmin, ymin, xmax, ymax). The bottom-left viewport is
  // at the window origin and worked before the fix; the other three have a
  // nonzero origin and used to render black.
  std::array<std::array<double, 4>, 4> viewports = { {
    { { 0.0, 0.0, 0.5, 0.5 } }, // Bottom-left
    { { 0.5, 0.0, 1.0, 0.5 } }, // Bottom-right
    { { 0.0, 0.5, 0.5, 1.0 } }, // Top-left
    { { 0.5, 0.5, 1.0, 1.0 } }, // Top-right
  } };

  // A sphere resting on a plane produces obvious ambient occlusion where the
  // two meet, so a black (broken) viewport is easy to tell apart from a
  // correctly rendered one.
  vtkNew<vtkSphereSource> sphere;
  sphere->SetThetaResolution(64);
  sphere->SetPhiResolution(64);
  sphere->SetRadius(0.5);
  sphere->SetCenter(0.0, 0.5, 0.0);
  vtkNew<vtkPolyDataMapper> sphereMapper;
  sphereMapper->SetInputConnection(sphere->GetOutputPort());

  vtkNew<vtkPlaneSource> plane;
  plane->SetOrigin(-1.0, 0.0, -1.0);
  plane->SetPoint1(1.0, 0.0, -1.0);
  plane->SetPoint2(-1.0, 0.0, 1.0);
  vtkNew<vtkPolyDataMapper> planeMapper;
  planeMapper->SetInputConnection(plane->GetOutputPort());

  for (const auto& viewport : viewports)
  {
    vtkNew<vtkActor> sphereActor;
    sphereActor->SetMapper(sphereMapper);
    vtkNew<vtkActor> planeActor;
    planeActor->SetMapper(planeMapper);

    vtkNew<vtkRenderer> renderer;
    renderer->AddActor(sphereActor);
    renderer->AddActor(planeActor);
    renderer->SetViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    renderer->SetBackground(0.5, 0.5, 0.5);
    renderer->GetActiveCamera()->SetPosition(0.5, 1.0, 2.0);
    renderer->GetActiveCamera()->SetFocalPoint(0.0, 0.3, 0.0);
    renderer->GetActiveCamera()->SetViewUp(0.0, 1.0, 0.0);
    renderer->ResetCamera();
    renderer->GetActiveCamera()->Zoom(1.5);

    // Each renderer gets its own SSAO pass, matching how an application sets up
    // per-viewport render passes. A single pass shared by every renderer would
    // hide the bug, because its framebuffers get established by the first
    // (window-origin) renderer.
    vtkNew<vtkRenderStepsPass> basicPasses;
    vtkNew<vtkSSAOPass> ssao;
    ssao->SetRadius(0.1);
    ssao->SetKernelSize(128);
    ssao->SetDelegatePass(basicPasses);
    vtkOpenGLRenderer::SafeDownCast(renderer)->SetPass(ssao);

    renderWindow->AddRenderer(renderer);
  }

  renderWindow->Render();

  const int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkTesting::DO_INTERACTOR)
  {
    interactor->Start();
  }

  return retVal == vtkTesting::FAILED ? EXIT_FAILURE : EXIT_SUCCESS;
}
