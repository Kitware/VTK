// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"

#include "vtkActor.h"
#include "vtkImageActor.h"
#include "vtkImageMapper3D.h"
#include "vtkNew.h"
#include "vtkPNGWriter.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkResizingWindowToImageFilter.h"
#include "vtkSphereSource.h"
#include "vtkWindowToImageFilter.h"

#include <utility> // for std::pair
#include <vector>

int TestResizingWindowToImageFilter(int argc, char* argv[])
{
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

  using Resolution = std::pair<int, int>;
  // some common screen resolutions : 720p, 1080p, 2K and 4k
  std::vector<Resolution> resolutions = { { 1280, 720 }, { 1440, 1080 }, { 2048, 1080 },
    { 4096, 2160 } };
  for (Resolution& res : resolutions)
  {
    // render sphere
    renderer->AddActor(sActor);
    renderWindow->Render();
    renderer->ResetCamera();
    renderWindow->Render();

    const int width = res.first;
    const int height = res.second;

    vtkNew<vtkResizingWindowToImageFilter> windowToImageFilter;
    windowToImageFilter->SetInput(renderWindow);
    windowToImageFilter->SetSize(width, height);
    windowToImageFilter->Update();

    // Show the screenshot
    vtkNew<vtkImageActor> imageActor;
    imageActor->GetMapper()->SetInputData(windowToImageFilter->GetOutput());

    renderer->RemoveActor(sActor);
    renderer->AddActor(imageActor);

    // In order for the imageActor to be rendered with the right size
    // we have to resize the window. To achieve big sizes we switch to
    // offscreen rendering
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
    if (retVal == 0)
      return EXIT_FAILURE;

    renderer->RemoveActor(imageActor);
  }

  return EXIT_SUCCESS;
}
