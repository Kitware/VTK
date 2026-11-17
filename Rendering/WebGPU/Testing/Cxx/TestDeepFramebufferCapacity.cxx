// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPlaneSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTestErrorObserver.h"
#include "vtkWebGPUDeepFramebuffer.h"
#include "vtkWebGPURenderWindow.h"
#include "vtkWebGPURenderer.h"

int TestDeepFramebufferCapacity(int argc, char* argv[])
{
  vtkNew<vtkWebGPURenderer> renderer;
  vtkNew<vtkRenderWindow> window;
  auto* webgpuWindow = vtkWebGPURenderWindow::SafeDownCast(window);
  if (!webgpuWindow)
  {
    vtkLog(ERROR, << "Deep framebuffer capacity check failed at line " << __LINE__);
    return EXIT_FAILURE;
  }
  window->SetSize(128, 128);
  window->AddRenderer(renderer);
  renderer->SetBackground(0.1, 0.1, 0.1);
  renderer->SetDeepFramebufferAverageLayersPerPixel(1);
  renderer->DeepFramebufferAutoResizeOff();
  auto* framebuffer = renderer->GetDeepFramebuffer();
  if (framebuffer->GetFragmentNodeCapacity() != 0)
  {
    vtkLog(ERROR, << "Deep framebuffer capacity check failed at line " << __LINE__);
    return EXIT_FAILURE;
  }
  // These failures are intentional; unexpected errors still fail the test below.
  vtkNew<vtkTest::ErrorObserver> observer;
  framebuffer->AddObserver(vtkCommand::WarningEvent, observer);
  renderer->AddObserver(vtkCommand::WarningEvent, observer);
  window->AddObserver(vtkCommand::WarningEvent, observer);
  for (int layer = 0; layer < 24; ++layer)
  {
    const double radius = 0.875 - (layer % 4) * 0.0625;
    vtkNew<vtkPlaneSource> plane;
    plane->SetOrigin(-radius, -radius, layer * 0.01);
    plane->SetPoint1(radius, -radius, layer * 0.01);
    plane->SetPoint2(-radius, radius, layer * 0.01);
    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputConnection(plane->GetOutputPort());
    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);
    const double t = (layer % 6) / 5.0;
    actor->GetProperty()->SetColor(t, 1.0 - t, 0.25 + 0.5 * (layer % 2));
    actor->GetProperty()->SetOpacity(0.05);
    actor->GetProperty()->LightingOff();
    renderer->AddActor(actor);
  }
  auto* camera = renderer->GetActiveCamera();
  camera->ParallelProjectionOn();
  camera->SetPosition(0, 0, 10);
  camera->SetFocalPoint(0, 0, 0);
  camera->SetParallelScale(1);
  renderer->ResetCameraClippingRange();

  window->Render();
  if (!(renderer->ReadDeepFramebufferStatistics(true) &
        vtkWebGPUDeepFramebuffer::FragmentPoolOverflow) ||
    !observer->GetWarning())
  {
    vtkLog(ERROR, << "Deep framebuffer capacity check failed at line " << __LINE__);
    return EXIT_FAILURE;
  }
  // Grow at unchanged dimensions with the default render bundles enabled.
  renderer->SetDeepFramebufferAverageLayersPerPixel(32);
  window->Render();
  if (framebuffer->GetFragmentNodeCapacity() != 128 * 128 * 32 ||
    renderer->ReadDeepFramebufferStatistics(true) != vtkWebGPUDeepFramebuffer::LayerLimitExceeded ||
    webgpuWindow->RenderWithCheckedDeepFramebuffer(1))
  {
    vtkLog(ERROR, << "Deep framebuffer capacity check failed at line " << __LINE__);
    return EXIT_FAILURE;
  }
  // More pool memory alone cannot recover the low-opacity layers beyond sixteen.
  renderer->SetDeepFramebufferMaximumLayersPerPixel(32);
  if (!webgpuWindow->RenderWithCheckedDeepFramebuffer(1))
  {
    vtkLog(ERROR, << "Deep framebuffer capacity check failed at line " << __LINE__);
    return EXIT_FAILURE;
  }
  renderer->SetDeepFramebufferAverageLayersPerPixel(1);
  renderer->DeepFramebufferAutoResizeOn();
  // Ordinary renders must recover without an explicit statistics query.
  for (int frame = 0; frame < 12; ++frame)
  {
    window->Render();
    window->WaitForCompletion();
  }
  if (framebuffer->GetFragmentNodeCapacity() <= 128 * 128)
  {
    vtkLog(ERROR, << "Asynchronous feedback did not grow the fragment pool");
    return EXIT_FAILURE;
  }
  if (!webgpuWindow->RenderWithCheckedDeepFramebuffer() ||
    framebuffer->GetFragmentNodeCapacity() <= 128 * 128)
  {
    vtkLog(ERROR, << "Deep framebuffer capacity check failed at line " << __LINE__);
    return EXIT_FAILURE;
  }
  const auto capacity = framebuffer->GetFragmentNodeCapacity();
  if (!webgpuWindow->RenderWithCheckedDeepFramebuffer(1) ||
    framebuffer->GetFragmentNodeCapacity() != capacity)
  {
    vtkLog(ERROR, << "Deep framebuffer capacity check failed at line " << __LINE__);
    return EXIT_FAILURE;
  }
  // An application memory ceiling must terminate recovery and report failure.
  renderer->SetDeepFramebufferMaximumMemoryInBytes(128 * 128 * 28);
  if (webgpuWindow->RenderWithCheckedDeepFramebuffer() ||
    framebuffer->GetFragmentNodeCapacity() != 128 * 128)
  {
    vtkLog(ERROR, << "Deep framebuffer capacity check failed at line " << __LINE__);
    return EXIT_FAILURE;
  }
  renderer->SetDeepFramebufferMaximumMemoryInBytes(1);
  if (webgpuWindow->RenderWithCheckedDeepFramebuffer())
  {
    vtkLog(ERROR, << "Deep framebuffer capacity check failed at line " << __LINE__);
    return EXIT_FAILURE;
  }
  renderer->SetDeepFramebufferMaximumMemoryInBytes(0);
  renderer->UseDeepFramebufferTransparencyOn();
  if (!webgpuWindow->RenderWithCheckedDeepFramebuffer())
  {
    vtkLog(ERROR, << "Deep framebuffer capacity check failed at line " << __LINE__);
    return EXIT_FAILURE;
  }
  // Resource release may occur while asynchronous feedback is still pending.
  window->Render();
  renderer->ReadDeepFramebufferStatistics();
  framebuffer->ReleaseGraphicsResources();
  renderer->UseRenderBundlesOff();
  if (!webgpuWindow->RenderWithCheckedDeepFramebuffer())
  {
    vtkLog(ERROR, << "Deep framebuffer capacity check failed at line " << __LINE__);
    return EXIT_FAILURE;
  }
  renderer->UseRenderBundlesOn();
  if (!webgpuWindow->RenderWithCheckedDeepFramebuffer())
  {
    vtkLog(ERROR, << "Deep framebuffer capacity check failed at line " << __LINE__);
    return EXIT_FAILURE;
  }
  const int result = vtkRegressionTestImage(window);
  if (result == vtkRegressionTester::DO_INTERACTOR)
  {
    vtkNew<vtkRenderWindowInteractor> interactor;
    interactor->SetRenderWindow(window);
    interactor->Start();
  }
  return !result;
}
