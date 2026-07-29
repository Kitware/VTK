// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkNew.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkRenderer.h"

#include <iostream>

//------------------------------------------------------------------------------
int TestWindowDPI(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.0, 0.0, 0.0);

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(300, 300);
  renderWindow->AddRenderer(renderer);

  // render once to create window
  renderWindow->Render();

  if (renderWindow->DetectDPI())
  {
    int dpi = renderWindow->GetDPI();
    std::cout << "DPI detected: " << renderWindow->GetDPI() << std::endl;

#ifdef __APPLE__
    if (dpi % 72 != 0)
    {
      std::cout << "DPI is not a multiple of 72, which is unexpected on macOS." << std::endl;
      return EXIT_FAILURE;
    }
#else
    if (dpi < 96)
    {
      std::cout << "DPI is less than 96, which is unexpected." << std::endl;
      return EXIT_FAILURE;
    }
#endif
  }
  else
  {
    // not an error (e.g. if X not running on Linux)
    std::cout << "DPI value check skipped." << std::endl;
  }

  return EXIT_SUCCESS;
}
