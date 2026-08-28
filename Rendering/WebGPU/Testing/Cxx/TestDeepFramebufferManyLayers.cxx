// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkPlaneSource.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkUnsignedCharArray.h"
#include "vtkWebGPURenderer.h"
#include "vtkWindowToImageFilter.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace
{
constexpr int LayerCount = 24;
constexpr int WindowSize = 300;

vtkSmartPointer<vtkActor> MakeQuad(int layer)
{
  const double z = -1.0 + 2.0 * layer / (LayerCount - 1.0);

  vtkNew<vtkPlaneSource> source;
  source->SetOrigin(-0.5, -0.5, z);
  source->SetPoint1(0.5, -0.5, z);
  source->SetPoint2(-0.5, 0.5, z);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(source->GetOutputPort());

  vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);
  const double t = layer / (LayerCount - 1.0);
  actor->GetProperty()->SetColor(t, 1.0 - t, 0.5 * t);
  actor->GetProperty()->SetOpacity(0.2);
  return actor;
}

std::vector<unsigned char> RenderInOrder(bool reversed)
{
  std::vector<vtkSmartPointer<vtkActor>> actors;
  for (int layer = 0; layer < LayerCount; ++layer)
  {
    actors.push_back(MakeQuad(layer));
  }
  if (reversed)
  {
    std::reverse(actors.begin(), actors.end());
  }

  vtkNew<vtkRenderer> renderer;
  auto* wgpuRenderer = vtkWebGPURenderer::SafeDownCast(renderer);
  if (wgpuRenderer == nullptr)
  {
    std::cerr << "ERROR: renderer is not a vtkWebGPURenderer\n";
    return {};
  }
  wgpuRenderer->UseDeepFramebufferTransparencyOn();

  for (const auto& actor : actors)
  {
    renderer->AddActor(actor);
  }
  renderer->SetBackground(0.1, 0.1, 0.1);

  vtkNew<vtkRenderWindow> window;
  window->SetSize(WindowSize, WindowSize);
  window->AddRenderer(renderer);
  renderer->ResetCamera();
  window->Render();

  vtkNew<vtkWindowToImageFilter> capture;
  capture->SetInput(window);
  capture->Update();

  auto* scalars =
    vtkUnsignedCharArray::SafeDownCast(capture->GetOutput()->GetPointData()->GetScalars());
  if (scalars == nullptr)
  {
    std::cerr << "ERROR: no pixels captured\n";
    return {};
  }
  const vtkIdType n = scalars->GetNumberOfValues();
  std::vector<unsigned char> pixels(static_cast<std::size_t>(n));
  for (vtkIdType i = 0; i < n; ++i)
  {
    pixels[static_cast<std::size_t>(i)] = scalars->GetValue(i);
  }
  return pixels;
}
}

int TestDeepFramebufferManyLayers(int, char*[])
{
  const std::vector<unsigned char> forward = RenderInOrder(false);
  const std::vector<unsigned char> reverse = RenderInOrder(true);

  if (forward.empty() || reverse.empty() || forward.size() != reverse.size())
  {
    std::cerr << "ERROR: could not capture both renders\n";
    return EXIT_FAILURE;
  }

  const unsigned char background[3] = { 26, 26, 26 };
  std::size_t covered = 0;
  for (std::size_t i = 0; i + 2 < forward.size(); i += 3)
  {
    if (forward[i] != background[0] || forward[i + 1] != background[1] ||
      forward[i + 2] != background[2])
    {
      ++covered;
    }
  }
  const std::size_t pixelCount = forward.size() / 3;
  if (covered * 20 < pixelCount)
  {
    std::cerr << "ERROR: only " << covered << " of " << pixelCount
              << " pixels are covered by geometry; the stack did not render\n";
    return EXIT_FAILURE;
  }

  std::size_t differing = 0;
  int worst = 0;
  for (std::size_t i = 0; i < forward.size(); ++i)
  {
    const int delta = std::abs(static_cast<int>(forward[i]) - static_cast<int>(reverse[i]));
    if (delta > 2)
    {
      ++differing;
      worst = std::max(worst, delta);
    }
  }
  if (differing > 0)
  {
    std::cerr << "ERROR: " << differing << " of " << forward.size()
              << " channel values differ between draw orders (worst " << worst
              << "); the layers kept past the per-pixel limit depend on draw order\n";
    return EXIT_FAILURE;
  }

  std::cout << LayerCount << " layers per pixel resolved identically in both draw orders over "
            << pixelCount << " pixels, " << covered << " covered by geometry\n";
  return EXIT_SUCCESS;
}
