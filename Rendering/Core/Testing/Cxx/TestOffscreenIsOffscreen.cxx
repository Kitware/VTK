// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkNew.h"
#include "vtkRenderWindow.h"

int TestOffscreenIsOffscreen(int, char*[])
{
  vtkNew<vtkRenderWindow> renWin;
  // This test is only run if VTK_DEFAULT_RENDER_WINDOW_OFFSCREEN is on. So the default should
  // be to use offscreen rendering
  return !renWin->GetOffScreenRendering();
}
