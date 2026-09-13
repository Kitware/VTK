// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Renders without a window: ShowWindow off together with UseOffScreenBuffers on asks the WebGPU
// render window to create neither a hardware window nor a surface. The pixels are read back from
// the offscreen color attachment that every frame is drawn into anyway.

#include "vtkActor.h"
#include "vtkConeSource.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkUnsignedCharArray.h"
#include "vtkWebGPURenderWindow.h"

#include <cstdlib>
#include <iostream>

namespace
{

void AddCone(vtkRenderWindow* renWin, vtkRenderer* renderer)
{
  vtkNew<vtkConeSource> cone;
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(cone->GetOutputPort());
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  renderer->AddActor(actor);
  renderer->SetBackground(0.2, 0.3, 0.4);
  renWin->AddRenderer(renderer);
}

// The background (0.2, 0.3, 0.4) as GetPixelData reports it, give or take rounding.
bool IsBackground(const unsigned char* rgb)
{
  const unsigned char expected[3] = { 51, 77, 102 };
  for (int i = 0; i < 3; ++i)
  {
    if (std::abs(static_cast<int>(rgb[i]) - static_cast<int>(expected[i])) > 2)
    {
      return false;
    }
  }
  return true;
}

} // namespace

int TestWebGPUOffScreenRendering(int argc, char* argv[])
{
  (void)argc;
  (void)argv;

  // Off-screen only: no hardware window, no surface.
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
  renWin->SetUseOffScreenBuffers(true);

  vtkNew<vtkRenderer> renderer;
  ::AddCone(renWin, renderer);
  renWin->Render();

  if (renWin->GetHardwareWindow() != nullptr)
  {
    std::cerr << "A hardware window was created for an off-screen only render window.\n";
    return EXIT_FAILURE;
  }

  vtkNew<vtkUnsignedCharArray> pixels;
  if (!renWin->GetPixelData(0, 0, 299, 299, /*front=*/1, pixels))
  {
    std::cerr << "Could not read back the rendered image.\n";
    return EXIT_FAILURE;
  }
  if (pixels->GetNumberOfTuples() != 300 * 300)
  {
    std::cerr << "Read back " << pixels->GetNumberOfTuples() << " pixels, expected " << 300 * 300
              << ".\n";
    return EXIT_FAILURE;
  }

  // A corner is background, and the cone covers the center: rendering really happened.
  unsigned char corner[3];
  pixels->GetTypedTuple(0, corner);
  if (!::IsBackground(corner))
  {
    std::cerr << "Corner pixel is (" << +corner[0] << ',' << +corner[1] << ',' << +corner[2]
              << "), expected the background.\n";
    return EXIT_FAILURE;
  }
  unsigned char center[3];
  pixels->GetTypedTuple(150 * 300 + 150, center);
  if (::IsBackground(center))
  {
    std::cerr << "Center pixel is the background: the cone was not rendered.\n";
    return EXIT_FAILURE;
  }

  // Resizing has no surface to reconfigure, but must still resize the attachments.
  renWin->SetSize(200, 200);
  renWin->Render();
  vtkNew<vtkUnsignedCharArray> resized;
  renWin->GetPixelData(0, 0, 199, 199, /*front=*/1, resized);
  if (resized->GetNumberOfTuples() != 200 * 200)
  {
    std::cerr << "After resizing, read back " << resized->GetNumberOfTuples()
              << " pixels, expected " << 200 * 200 << ".\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
