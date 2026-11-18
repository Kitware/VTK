// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// A full-window renderer on an upper layer keeps the colors of the layers below it, never shows
// its own background and clears the depth buffer unless PreserveDepthBuffer is on.

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkNew.h"
#include "vtkPlaneSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkStringFormatter.h"
#include "vtkUnsignedCharArray.h"

#include <array>
#include <cstdlib>

namespace
{
using Color = std::array<int, 3>;

// An unlit square spanning x0..x1 and y -0.5..0.5 at depth z.
vtkNew<vtkActor> MakeSquare(double x0, double x1, double z, double r, double g, double b)
{
  vtkNew<vtkPlaneSource> plane;
  plane->SetOrigin(x0, -0.5, z);
  plane->SetPoint1(x1, -0.5, z);
  plane->SetPoint2(x0, 0.5, z);
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(plane->GetOutputPort());
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetColor(r, g, b);
  actor->GetProperty()->LightingOff();
  return actor;
}

Color Pixel(vtkRenderWindow* window, int x, int y)
{
  vtkNew<vtkUnsignedCharArray> pixels;
  window->GetRGBACharPixelData(x, y, x, y, /*front=*/1, pixels);
  return { pixels->GetValue(0), pixels->GetValue(1), pixels->GetValue(2) };
}

bool Near(const Color& actual, const Color& expected)
{
  for (int i = 0; i < 3; ++i)
  {
    if (std::abs(actual[i] - expected[i]) > 2)
    {
      return false;
    }
  }
  return true;
}

// Pixel columns in a 100 x 100 window looking at x -1..1: only the red square, both squares,
// only the white square, and a row above both squares.
constexpr int RedOnlyX = 15;
constexpr int OverlapX = 42;
constexpr int WhiteOnlyX = 80;
constexpr int MiddleY = 50;
constexpr int AboveY = 90;

const Color Red = { 255, 0, 0 };
const Color White = { 255, 255, 255 };

bool Check(
  vtkRenderWindow* window, const Color& background, const Color& overlap, const char* label)
{
  window->Render();
  const struct
  {
    const char* Name;
    int X, Y;
    Color Expected;
  } probes[] = { { "background", WhiteOnlyX, AboveY, background },
    { "red square", RedOnlyX, MiddleY, Red }, { "overlap", OverlapX, MiddleY, overlap },
    { "white square", WhiteOnlyX, MiddleY, White } };
  bool ok = true;
  for (const auto& probe : probes)
  {
    const Color actual = Pixel(window, probe.X, probe.Y);
    if (!Near(actual, probe.Expected))
    {
      vtk::println("{}: {} is ({}, {}, {}), expected ({}, {}, {})", label, probe.Name, actual[0],
        actual[1], actual[2], probe.Expected[0], probe.Expected[1], probe.Expected[2]);
      ok = false;
    }
  }
  return ok;
}
}

int TestRendererLayersClear(int, char*[])
{
  // The camera looks down -z, so the red square at z = 0 is in front of the white one.
  vtkNew<vtkRenderer> base;
  base->SetBackground(0.0, 0.0, 1.0);
  base->AddActor(MakeSquare(-0.9, 0.1, 0.0, 1.0, 0.0, 0.0));

  vtkNew<vtkRenderer> overlay;
  overlay->SetLayer(1);
  overlay->SetBackground(0.0, 1.0, 0.0);
  overlay->AddActor(MakeSquare(-0.4, 0.9, -0.5, 1.0, 1.0, 1.0));

  vtkCamera* camera = base->GetActiveCamera();
  camera->ParallelProjectionOn();
  camera->SetPosition(0.0, 0.0, 1.0);
  camera->SetFocalPoint(0.0, 0.0, 0.0);
  camera->SetParallelScale(1.0);
  camera->SetClippingRange(0.1, 3.0);
  overlay->SetActiveCamera(camera);

  vtkNew<vtkRenderWindow> window;
  window->SetWindowName(__func__);
  window->SetMultiSamples(0);
  window->SetSize(100, 100);
  window->SetNumberOfLayers(2);
  window->AddRenderer(base);
  window->AddRenderer(overlay);

  const Color blue = { 0, 0, 255 };
  bool ok = Check(window, blue, White, "overlay clears depth");
  ok &= Check(window, blue, White, "second frame");

  overlay->PreserveDepthBufferOn();
  ok &= Check(window, blue, Red, "overlay preserves depth");

  // A new background color has to reach the next frame.
  base->SetBackground(1.0, 1.0, 0.0);
  ok &= Check(window, { 255, 255, 0 }, Red, "after changing the background");

  overlay->PreserveDepthBufferOff();
  ok &= Check(window, { 255, 255, 0 }, White, "overlay clears depth again");
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
