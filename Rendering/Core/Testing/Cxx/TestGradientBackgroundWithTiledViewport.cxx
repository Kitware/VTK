// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkConeSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTesting.h"
#include "vtkViewport.h"
#include "vtkWindowToImageFilter.h"

#include <cstdlib>

int TestGradientBackgroundWithTiledViewport(int argc, char* argv[])
{
  vtkNew<vtkRenderWindow> win;
  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkConeSource> cone;
  vtkNew<vtkPolyDataMapper> map;
  vtkNew<vtkActor> act;

  map->SetInputConnection(cone->GetOutputPort());
  act->SetMapper(map);
  renderer->AddActor(act);
  renderer->GradientBackgroundOn();
  renderer->SetGradientMode(
    vtkViewport::GradientModes::VTK_GRADIENT_RADIAL_VIEWPORT_FARTHEST_CORNER);
  renderer->SetBackground(0.8, 0.4, 0.1);
  renderer->SetBackground2(0.1, 0.4, 0.8);
  win->AddRenderer(renderer);
  win->SetInteractor(iren);
  iren->Initialize();
  iren->UpdateSize(640, 480);

  vtkNew<vtkTesting> testing;
  int retVal = vtkTesting::FAILED;
  for (int i = 0; i < argc; ++i)
  {
    testing->AddArgument(argv[i]);
  }

  if (testing->IsInteractiveModeSpecified())
  {
    iren->Start();
    retVal = vtkTesting::DO_INTERACTOR;
  }

  if (testing->IsValidImageSpecified())
  {
    // Purposefully render at a higher resolution using tiled display
    // Scale the 640x480 render window by 2x, take a screenshot of all tiles
    // and validate the result against the higher resolution image.
    const double threshold = 0.05;
    vtkNew<vtkWindowToImageFilter> rtW2if;
    rtW2if->SetInput(win);
    rtW2if->SetScale(2, 2);

    for (int i = 0; i < argc; ++i)
    {
      if ("-NoRerender" == std::string(argv[i]))
      {
        rtW2if->ShouldRerenderOff();
      }
    }

    int swapBuffers = win->GetSwapBuffers();
    win->SwapBuffersOff();
    win->Render();
    rtW2if->ReadFrontBufferOff();
    rtW2if->Update();
    win->SetSwapBuffers(swapBuffers); // restore swap state.
    retVal = testing->RegressionTest(rtW2if, threshold);
  }
  return retVal == 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
