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

int TestGradientBackgroundWithTiledViewports(int argc, char* argv[])
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
  double xmins[4] = { 0.0, 0.4, 0.0, 0.0 };
  double ymins[4] = { 0.0, 0.0, 0.25, 0.5 };
  double xmaxs[4] = { 0.4, 1.0, 1.0, 1.0 };
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

    // perform and extra render to make sure it is displayed
    int swapBuffers = win->GetSwapBuffers();
    // since we're reading from back-buffer, it's essential that we turn off swapping
    // otherwise what remains in the back-buffer after the swap is undefined by OpenGL specs.
    win->SwapBuffersOff();
    win->Render();
    rtW2if->ReadFrontBufferOff();
    rtW2if->Update();
    win->SetSwapBuffers(swapBuffers); // restore swap state.
    retVal = testing->RegressionTest(rtW2if, threshold);
  }
  return retVal == 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
