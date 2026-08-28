// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkRegressionTestImage.h"

#include "vtkActor.h"
#include "vtkImageActor.h"
#include "vtkImageMapper3D.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkResizingWindowToImageFilter.h"
#include "vtkSphereSource.h"

int TestResizingWindowToImageFilter(int argc, char* argv[])
{
  // The requested size is larger than the size limit, so the filter renders a
  // 160x120 image and magnifies it by 2 on each axis. That is the only path
  // where the captured pixels can be wrong, the smaller requests are served by
  // a plain window capture. The scale factor selection itself is covered by
  // TestResizingWindowToImageFilterScaleFactors.
  const int width = 320;
  const int height = 240;
  const int sizeLimit = 200;

  vtkNew<vtkSphereSource> sphere;
  sphere->SetRadius(10);
  sphere->SetThetaResolution(20);
  sphere->SetPhiResolution(20);
  sphere->Update();

  vtkNew<vtkPolyDataMapper> sMapper;
  sMapper->SetInputConnection(sphere->GetOutputPort());

  vtkNew<vtkActor> sActor;
  sActor->SetMapper(sMapper);
  sActor->GetProperty()->SetColor(1, 1, 1);
  sActor->GetProperty()->EdgeVisibilityOn();

  vtkNew<vtkRenderWindow> renderWindow;
  vtkNew<vtkRenderer> renderer;
  renderWindow->AddRenderer(renderer);

  renderer->SetBackground(0.5, 0.5, 0.5);
  renderWindow->SetSize(400, 400);

  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);

  // render sphere
  renderer->AddActor(sActor);
  renderWindow->Render();
  renderer->ResetCamera();
  renderWindow->Render();

  vtkNew<vtkResizingWindowToImageFilter> windowToImageFilter;
  windowToImageFilter->SetInput(renderWindow);
  windowToImageFilter->SetSizeLimit(sizeLimit);
  windowToImageFilter->SetSize(width, height);
  windowToImageFilter->Update();

  // Show the screenshot
  vtkNew<vtkImageActor> imageActor;
  imageActor->GetMapper()->SetInputData(windowToImageFilter->GetOutput());

  renderer->RemoveActor(sActor);
  renderer->AddActor(imageActor);

  // In order for the imageActor to be rendered with the right size we have to
  // resize the window. Render offscreen so that the requested size is honored
  // regardless of the window manager.
  renderWindow->SetOffScreenRendering(true);
  renderWindow->SetSize(width, height);

  // render captured image
  renderWindow->Render();
  renderer->ResetCamera();
  renderWindow->Render();

  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }

  return retVal == vtkRegressionTester::FAILED ? EXIT_FAILURE : EXIT_SUCCESS;
}
