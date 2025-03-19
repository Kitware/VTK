// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkRenderWindow.h"
#include "vtkRenderer.h"

#include <cstdlib>

/**
 * This unit test exercises that many render windows can be created
 * and destroyed in succession. It is developed to prevent
 * issues such as https://gitlab.kitware.com/vtk/vtk/-/issues/19618
 */
int TestManyRenderWindows(int argc, char* argv[])
{
  int n = 300;
  if (argc > 1)
  {
    n = std::atoi(argv[1]);
  }
  std::cout << "Create " << n << " render windows\n";
  for (int i = 0; i < n; ++i)
  {
    vtkNew<vtkRenderWindow> renderWindow;
    vtkNew<vtkRenderer> renderer;
    renderer->SetBackground(static_cast<double>(i) / n, 0, 0.5);
    renderWindow->AddRenderer(renderer);
    renderWindow->Render();
  }
  return EXIT_SUCCESS;
}
