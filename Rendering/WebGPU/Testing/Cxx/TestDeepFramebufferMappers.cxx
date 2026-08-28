// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkGlyph3DMapper.h"
#include "vtkImageData.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
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
#include <string>
#include <vector>

namespace
{
constexpr int WindowSize = 300;

vtkSmartPointer<vtkPolyData> MakeGlyphPoints(double z)
{
  vtkNew<vtkPoints> points;
  points->InsertNextPoint(-0.6, 0.0, z);
  points->InsertNextPoint(0.0, 0.0, z + 0.3);
  points->InsertNextPoint(0.6, 0.0, z + 0.6);

  vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
  polyData->SetPoints(points);
  return polyData;
}

vtkSmartPointer<vtkActor> MakeGlyphActor(double z, const double rgb[3])
{
  vtkNew<vtkSphereSource> glyphSource;
  glyphSource->SetRadius(0.5);
  glyphSource->SetThetaResolution(24);
  glyphSource->SetPhiResolution(24);

  vtkNew<vtkGlyph3DMapper> mapper;
  mapper->SetInputData(MakeGlyphPoints(z));
  mapper->SetSourceConnection(glyphSource->GetOutputPort());

  vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetColor(rgb[0], rgb[1], rgb[2]);
  actor->GetProperty()->SetOpacity(0.4);
  return actor;
}

vtkSmartPointer<vtkActor> MakeCompositeActor(double x, const double rgb[3])
{
  vtkNew<vtkMultiBlockDataSet> blocks;
  for (int block = 0; block < 2; ++block)
  {
    vtkNew<vtkSphereSource> source;
    source->SetCenter(x, 0.0, -0.3 + 0.6 * block);
    source->SetRadius(0.6);
    source->SetThetaResolution(24);
    source->SetPhiResolution(24);
    source->Update();

    vtkNew<vtkPolyData> copy;
    copy->DeepCopy(source->GetOutput());
    blocks->SetBlock(block, copy);
  }

  vtkNew<vtkCompositePolyDataMapper> mapper;
  mapper->SetInputDataObject(blocks);

  vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetColor(rgb[0], rgb[1], rgb[2]);
  actor->GetProperty()->SetOpacity(0.4);
  return actor;
}

std::vector<unsigned char> RenderInOrder(
  const std::vector<vtkSmartPointer<vtkActor>>& actorsIn, bool reversed)
{
  std::vector<vtkSmartPointer<vtkActor>> actors = actorsIn;
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

bool OrderIndependent(const std::string& what, const std::vector<vtkSmartPointer<vtkActor>>& actors)
{
  const std::vector<unsigned char> forward = RenderInOrder(actors, false);
  const std::vector<unsigned char> reverse = RenderInOrder(actors, true);
  if (forward.empty() || reverse.empty() || forward.size() != reverse.size())
  {
    std::cerr << "ERROR: " << what << ": could not capture both renders\n";
    return false;
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
    std::cerr << "ERROR: " << what << ": only " << covered << " of " << pixelCount
              << " pixels are covered by geometry\n";
    return false;
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
    std::cerr << "ERROR: " << what << ": " << differing << " of " << forward.size()
              << " channel values differ between draw orders (worst " << worst
              << "); this mapper's translucent fragments are not reaching the fragment lists\n";
    return false;
  }

  std::cout << what << ": order independent over " << pixelCount << " pixels, " << covered
            << " covered by geometry\n";
  return true;
}
}

int TestDeepFramebufferMappers(int, char*[])
{
  const double red[3] = { 1.0, 0.0, 0.0 };
  const double green[3] = { 0.0, 1.0, 0.0 };
  const double blue[3] = { 0.0, 0.0, 1.0 };

  bool ok = true;

  ok &= OrderIndependent("glyph mapper",
    { MakeGlyphActor(-0.4, red), MakeGlyphActor(0.0, green), MakeGlyphActor(0.4, blue) });

  ok &= OrderIndependent("composite mapper",
    { MakeCompositeActor(-0.4, red), MakeCompositeActor(0.0, green),
      MakeCompositeActor(0.4, blue) });

  ok &= OrderIndependent(
    "glyph and composite mixed", { MakeGlyphActor(0.0, red), MakeCompositeActor(0.0, green) });

  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
