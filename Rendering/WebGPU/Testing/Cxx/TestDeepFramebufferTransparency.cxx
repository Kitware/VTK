// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkImageData.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkUnsignedCharArray.h"
#include "vtkWebGPURenderer.h"
#include "vtkWindowToImageFilter.h"

#include <algorithm>
#include <cstdlib>
#include <vector>

namespace
{

vtkSmartPointer<vtkActor> MakeSphere(double x, const double rgb[3], double opacity)
{
  vtkNew<vtkSphereSource> source;
  source->SetCenter(x, 0.0, 0.0);
  source->SetRadius(1.0);
  source->SetThetaResolution(32);
  source->SetPhiResolution(32);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(source->GetOutputPort());

  vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetColor(rgb[0], rgb[1], rgb[2]);
  actor->GetProperty()->SetOpacity(opacity);
  return actor;
}

std::vector<unsigned char> RenderInOrder(bool reversed, int size)
{
  const double red[3] = { 1.0, 0.0, 0.0 };
  const double green[3] = { 0.0, 1.0, 0.0 };
  const double blue[3] = { 0.0, 0.0, 1.0 };

  std::vector<vtkSmartPointer<vtkActor>> actors = { MakeSphere(-0.6, red, 0.4),
    MakeSphere(0.0, green, 0.4), MakeSphere(0.6, blue, 0.4) };
  if (reversed)
  {
    std::reverse(actors.begin(), actors.end());
  }

  vtkNew<vtkRenderer> renderer;
  if (auto* wgpuRenderer = vtkWebGPURenderer::SafeDownCast(renderer))
  {
    wgpuRenderer->UseDeepFramebufferTransparencyOn();
  }
  else
  {
    vtkLog(ERROR, "renderer is not a vtkWebGPURenderer");
    return {};
  }
  for (const auto& actor : actors)
  {
    renderer->AddActor(actor);
  }
  renderer->SetBackground(0.1, 0.1, 0.1);

  vtkNew<vtkRenderWindow> window;
  window->SetSize(size, size);
  window->AddRenderer(renderer);
  window->Render();

  vtkNew<vtkWindowToImageFilter> capture;
  capture->SetInput(window);
  capture->Update();

  vtkImageData* image = capture->GetOutput();
  auto* scalars = vtkUnsignedCharArray::SafeDownCast(image->GetPointData()->GetScalars());
  if (scalars == nullptr)
  {
    vtkLog(ERROR, "no pixels captured");
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

} // namespace

int TestDeepFramebufferTransparency(int, char*[])
{
  const int size = 300;
  const std::vector<unsigned char> forward = RenderInOrder(false, size);
  const std::vector<unsigned char> reverse = RenderInOrder(true, size);

  if (forward.empty() || reverse.empty() || forward.size() != reverse.size())
  {
    vtkLog(ERROR, "capture failed (" << forward.size() << " vs " << reverse.size() << ")");
    return EXIT_FAILURE;
  }

  std::size_t nonBackground = 0;
  for (std::size_t i = 0; i + 2 < forward.size(); i += 3)
  {
    const bool isBackground = forward[i] < 40 && forward[i + 1] < 40 && forward[i + 2] < 40;
    if (!isBackground)
    {
      ++nonBackground;
    }
  }
  const std::size_t pixelCount = forward.size() / 3;
  if (nonBackground * 20 < pixelCount)
  {
    vtkLog(ERROR,
      "only " << nonBackground << " of " << pixelCount
              << " pixels differ from the background; the spheres were not composited");
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
    vtkLog(ERROR,
      "" << differing << " of " << forward.size()
         << " channel values differ between draw orders (worst " << worst
         << "); transparency is not order independent");
    return EXIT_FAILURE;
  }

  vtkLog(INFO,
    "order independent over " << pixelCount << " pixels, " << nonBackground
                              << " covered by geometry");
  return EXIT_SUCCESS;
}
