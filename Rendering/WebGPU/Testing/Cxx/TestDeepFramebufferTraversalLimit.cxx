// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkTestErrorObserver.h"
#include "vtkWebGPUDeepFramebuffer.h"
#include "vtkWebGPURenderWindow.h"
#include "vtkWebGPURenderer.h"

int TestDeepFramebufferTraversalLimit(int, char*[])
{
  // Concentrate a long list in a few pixels without exhausting the shared pool.
  vtkNew<vtkPoints> points;
  points->InsertNextPoint(-0.015625, -0.015625, 0);
  points->InsertNextPoint(0.015625, -0.015625, 0);
  points->InsertNextPoint(0.015625, 0.015625, 0);
  points->InsertNextPoint(-0.015625, 0.015625, 0);
  vtkNew<vtkCellArray> polys;
  const vtkIdType ids[4] = { 0, 1, 2, 3 };
  for (int layer = 0; layer < 4100; ++layer)
  {
    polys->InsertNextCell(4, ids);
  }
  vtkNew<vtkPolyData> mesh;
  mesh->SetPoints(points);
  mesh->SetPolys(polys);
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputData(mesh);
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetOpacity(0.01);
  actor->GetProperty()->LightingOff();
  vtkNew<vtkWebGPURenderer> renderer;
  renderer->SetDeepFramebufferMaximumLayersPerPixel(64);
  renderer->AddActor(actor);
  renderer->GetActiveCamera()->ParallelProjectionOn();
  renderer->GetActiveCamera()->SetPosition(0, 0, 1);
  renderer->GetActiveCamera()->SetFocalPoint(0, 0, 0);
  renderer->GetActiveCamera()->SetParallelScale(1);
  renderer->ResetCameraClippingRange();
  vtkNew<vtkRenderWindow> window;
  window->SetSize(128, 128);
  // A complete first renderer must not hide truncation in another renderer.
  vtkNew<vtkWebGPURenderer> emptyRenderer;
  window->AddRenderer(emptyRenderer);
  window->AddRenderer(renderer);
  vtkNew<vtkTest::ErrorObserver> observer;
  window->AddObserver(vtkCommand::WarningEvent, observer);
  auto* webgpuWindow = vtkWebGPURenderWindow::SafeDownCast(window);
  if (!webgpuWindow || webgpuWindow->RenderWithCheckedDeepFramebuffer())
  {
    return EXIT_FAILURE;
  }
  const int expected =
    vtkWebGPUDeepFramebuffer::LayerLimitExceeded | vtkWebGPUDeepFramebuffer::TraversalLimitExceeded;
  if (renderer->ReadDeepFramebufferStatistics(true) != expected)
  {
    return EXIT_FAILURE;
  }
  renderer->DrawOff();
  return webgpuWindow->RenderWithCheckedDeepFramebuffer() ? EXIT_SUCCESS : EXIT_FAILURE;
}
