// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCamera.h"
#include "vtkCubeAxesActor.h"
#include "vtkNew.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkUnsignedCharArray.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>

namespace
{
constexpr int WindowSize = 400;

/**
 * Render a cube axes actor once and grab the resulting frame.
 * Labels and titles are hidden when showText is false, to get a reference frame without text.
 */
void GrabFrame(vtkRenderWindow* renWin, vtkRenderer* renderer, int flyMode, int gridLocation,
  bool showText, vtkUnsignedCharArray* pixels)
{
  renderer->RemoveAllViewProps();

  vtkCamera* camera = renderer->GetActiveCamera();
  camera->SetPosition(1, 1, 1);
  camera->SetFocalPoint(0, 0, 0);
  camera->SetViewUp(0, 0, 1);

  vtkNew<vtkCubeAxesActor> axes;
  axes->SetCamera(camera);
  axes->SetFlyMode(flyMode);
  axes->SetGridLineLocation(gridLocation);
  axes->SetDrawXGridlines(true);
  axes->SetDrawYGridlines(true);
  axes->SetDrawZGridlines(true);
  axes->SetUseTextActor3D(true);
  if (!showText)
  {
    axes->XAxisLabelVisibilityOff();
    axes->YAxisLabelVisibilityOff();
    axes->ZAxisLabelVisibilityOff();
  }

  renderer->AddActor(axes);
  renderer->ResetCamera();
  renWin->Render();
  renWin->GetPixelData(0, 0, WindowSize - 1, WindowSize - 1, 1, pixels);
}

/**
 * Count the components that differ between two frames.
 */
vtkIdType CountDifferences(vtkUnsignedCharArray* lhs, vtkUnsignedCharArray* rhs)
{
  vtkIdType count = 0;
  const vtkIdType size = std::min(lhs->GetNumberOfValues(), rhs->GetNumberOfValues());
  for (vtkIdType idx = 0; idx < size; idx++)
  {
    count += lhs->GetValue(idx) != rhs->GetValue(idx) ? 1 : 0;
  }
  return count;
}
}

//------------------------------------------------------------------------------
// vtkTextActor3D based labels only draw during the translucent pass, so the cube axes actor
// has to report translucent geometry for the axes it actually renders. See VTK issue #19729.
int TestCubeAxes_Text3D(int, char*[])
{
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->SetSize(WindowSize, WindowSize);
  renWin->AddRenderer(renderer);

  const std::array<int, 3> gridLocations = { vtkCubeAxesActor::VTK_GRID_LINES_ALL,
    vtkCubeAxesActor::VTK_GRID_LINES_CLOSEST, vtkCubeAxesActor::VTK_GRID_LINES_FURTHEST };
  const std::array<int, 3> flyModes = { vtkCubeAxesActor::VTK_FLY_OUTER_EDGES,
    vtkCubeAxesActor::VTK_FLY_CLOSEST_TRIAD, vtkCubeAxesActor::VTK_FLY_FURTHEST_TRIAD };

  int status = EXIT_SUCCESS;
  for (int flyMode : flyModes)
  {
    for (int gridLocation : gridLocations)
    {
      vtkNew<vtkUnsignedCharArray> withText;
      vtkNew<vtkUnsignedCharArray> withoutText;
      ::GrabFrame(renWin, renderer, flyMode, gridLocation, true, withText);
      ::GrabFrame(renWin, renderer, flyMode, gridLocation, false, withoutText);

      const vtkIdType differences = ::CountDifferences(withText, withoutText);
      if (differences == 0)
      {
        std::cerr << "No 3D text rendered with fly mode " << flyMode << " and grid line location "
                  << gridLocation << std::endl;
        status = EXIT_FAILURE;
      }
    }
  }

  return status;
}
