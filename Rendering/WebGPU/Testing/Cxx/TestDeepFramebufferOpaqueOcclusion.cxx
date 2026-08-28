// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkCubeSource.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkUnsignedCharArray.h"
#include "vtkWebGPURenderer.h"
#include "vtkWindowToImageFilter.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace
{
constexpr int WindowSize = 300;

vtkSmartPointer<vtkActor> MakeOpaqueBox()
{
  vtkNew<vtkCubeSource> source;
  source->SetCenter(0.0, 0.0, 1.0);
  source->SetXLength(3.0);
  source->SetYLength(3.0);
  source->SetZLength(0.2);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(source->GetOutputPort());

  vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetColor(0.2, 0.2, 0.8);
  return actor;
}

vtkSmartPointer<vtkActor> MakeHiddenTranslucentSphere()
{
  vtkNew<vtkSphereSource> source;
  source->SetCenter(0.0, 0.0, -1.0);
  source->SetRadius(1.0);
  source->SetThetaResolution(32);
  source->SetPhiResolution(32);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(source->GetOutputPort());

  vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetColor(1.0, 0.0, 0.0);
  actor->GetProperty()->SetOpacity(0.5);
  return actor;
}

std::vector<unsigned char> Render(bool withHiddenSphere)
{
  vtkNew<vtkRenderer> renderer;
  auto* wgpuRenderer = vtkWebGPURenderer::SafeDownCast(renderer);
  if (wgpuRenderer == nullptr)
  {
    std::cerr << "ERROR: renderer is not a vtkWebGPURenderer\n";
    return {};
  }
  wgpuRenderer->UseDeepFramebufferTransparencyOn();

  vtkSmartPointer<vtkActor> box = MakeOpaqueBox();
  renderer->AddActor(box);
  renderer->SetBackground(0.1, 0.1, 0.1);

  vtkNew<vtkRenderWindow> window;
  window->SetSize(WindowSize, WindowSize);
  window->AddRenderer(renderer);
  renderer->ResetCamera();

  if (withHiddenSphere)
  {
    renderer->AddActor(MakeHiddenTranslucentSphere());
  }
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

int TestDeepFramebufferOpaqueOcclusion(int, char*[])
{
  const std::vector<unsigned char> boxOnly = Render(false);
  const std::vector<unsigned char> boxAndSphere = Render(true);

  if (boxOnly.empty() || boxAndSphere.empty() || boxOnly.size() != boxAndSphere.size())
  {
    std::cerr << "ERROR: could not capture both renders\n";
    return EXIT_FAILURE;
  }

  const unsigned char background[3] = { 26, 26, 26 };
  std::size_t covered = 0;
  for (std::size_t i = 0; i + 2 < boxOnly.size(); i += 3)
  {
    if (boxOnly[i] != background[0] || boxOnly[i + 1] != background[1] ||
      boxOnly[i + 2] != background[2])
    {
      ++covered;
    }
  }
  const std::size_t pixelCount = boxOnly.size() / 3;
  if (covered * 4 < pixelCount)
  {
    std::cerr << "ERROR: the opaque box covers only " << covered << " of " << pixelCount
              << " pixels; it is not occluding anything\n";
    return EXIT_FAILURE;
  }

  std::size_t differing = 0;
  int worst = 0;
  for (std::size_t i = 0; i < boxOnly.size(); ++i)
  {
    const int delta = std::abs(static_cast<int>(boxOnly[i]) - static_cast<int>(boxAndSphere[i]));
    if (delta > 2)
    {
      ++differing;
      worst = std::max(worst, delta);
    }
  }
  if (differing > 0)
  {
    std::cerr << "ERROR: " << differing << " of " << boxOnly.size()
              << " channel values changed when a fully hidden translucent sphere was added (worst "
              << worst
              << "); its fragments are reaching the fragment lists despite failing the depth "
                 "test, and the resolve then draws them over the opaque geometry\n";
    return EXIT_FAILURE;
  }

  std::cout << "geometry hidden behind opaque surfaces stays hidden over " << pixelCount
            << " pixels, " << covered << " covered by the box\n";
  return EXIT_SUCCESS;
}
