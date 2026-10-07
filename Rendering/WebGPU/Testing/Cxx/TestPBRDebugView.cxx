// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkNew.h"
#include "vtkPlaneSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkUnsignedCharArray.h"
#include "vtkWebGPURenderer.h"

#include <cmath>
#include <iostream>

namespace
{
bool CheckCenter(vtkRenderWindow* window, vtkWebGPURenderer* renderer, int view,
  const double expected[3], const char* name)
{
  renderer->SetPBRDebugView(view);
  window->Render();
  int* size = window->GetSize();
  const int x = size[0] / 2;
  const int y = size[1] / 2;
  vtkNew<vtkUnsignedCharArray> pixel;
  window->GetRGBACharPixelData(x, y, x, y, 0, pixel);
  bool match = true;
  for (int c = 0; c < 3; ++c)
  {
    match &= std::abs(pixel->GetValue(c) - 255.0 * expected[c]) <= 3.0;
  }
  if (!match)
  {
    std::cerr << name << ": expected (" << 255.0 * expected[0] << ", " << 255.0 * expected[1]
              << ", " << 255.0 * expected[2] << "), got (" << int(pixel->GetValue(0)) << ", "
              << int(pixel->GetValue(1)) << ", " << int(pixel->GetValue(2)) << ")" << std::endl;
  }
  return match;
}
}

int TestPBRDebugView(int, char*[])
{
  vtkNew<vtkPlaneSource> plane;
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(plane->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetInterpolationToPBR();
  actor->GetProperty()->SetColor(0.2, 0.4, 0.6);
  actor->GetProperty()->SetRoughness(0.25);
  actor->GetProperty()->SetMetallic(0.75);

  vtkNew<vtkRenderWindow> window;
  window->SetSize(64, 64);
  vtkNew<vtkRenderer> renderer;
  window->AddRenderer(renderer);
  renderer->AddActor(actor);
  renderer->ResetCamera();

  auto* wgpuRenderer = vtkWebGPURenderer::SafeDownCast(renderer);
  if (!wgpuRenderer)
  {
    std::cerr << "The renderer is not a vtkWebGPURenderer." << std::endl;
    return EXIT_FAILURE;
  }

  bool success = true;
  const double roughness[3] = { 0.25, 0.25, 0.25 };
  success &= CheckCenter(
    window, wgpuRenderer, vtkWebGPURenderer::PBR_DEBUG_VIEW_ROUGHNESS, roughness, "roughness");
  const double metallic[3] = { 0.75, 0.75, 0.75 };
  success &= CheckCenter(
    window, wgpuRenderer, vtkWebGPURenderer::PBR_DEBUG_VIEW_METALLIC, metallic, "metallic");
  const double normal[3] = { 0.5, 0.5, 1.0 };
  success &=
    CheckCenter(window, wgpuRenderer, vtkWebGPURenderer::PBR_DEBUG_VIEW_NORMAL, normal, "normal");
  const double albedo[3] = { std::pow(0.2, 1.0 / 2.2), std::pow(0.4, 1.0 / 2.2),
    std::pow(0.6, 1.0 / 2.2) };
  success &=
    CheckCenter(window, wgpuRenderer, vtkWebGPURenderer::PBR_DEBUG_VIEW_ALBEDO, albedo, "albedo");
  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
