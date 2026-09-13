// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// ShowWindow off on its own keeps the window and the surface, and leaves the window unmapped -
// the behaviour the OpenGL backend has. Needs a display; see TestWebGPUOffScreenRendering for
// the mode that does not.

#include "vtkActor.h"
#include "vtkConeSource.h"
#include "vtkHardwareWindow.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkWebGPURenderWindow.h"

#include <iostream>

int TestWebGPUHiddenWindow(int argc, char* argv[])
{
  (void)argc;
  (void)argv;

  vtkNew<vtkRenderWindow> renWin;
  if (vtkWebGPURenderWindow::SafeDownCast(renWin) == nullptr)
  {
    std::cerr << "This test needs the WebGPU rendering backend.\n";
    return EXIT_FAILURE;
  }
  renWin->SetWindowName(__func__);
  renWin->SetMultiSamples(0);
  renWin->SetSize(300, 300);
  renWin->SetShowWindow(false);

  vtkNew<vtkConeSource> cone;
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(cone->GetOutputPort());
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->SetBackground(0.2, 0.3, 0.4);
  renWin->AddRenderer(renderer);
  renWin->Render();

  vtkHardwareWindow* hardwareWindow = renWin->GetHardwareWindow();
  if (hardwareWindow == nullptr)
  {
    std::cerr << "No hardware window was created for a hidden window.\n";
    return EXIT_FAILURE;
  }
  if (hardwareWindow->GetShowWindow())
  {
    std::cerr << "ShowWindow did not reach the hardware window.\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
