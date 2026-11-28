// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Replacing the points of a block with another array of the same size must be rendered, even
// when that array was created before the one currently on the device. The two blocks share one
// batch, so the arrays of one of them are written after the other's.

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkStringFormatter.h"
#include "vtkUnsignedCharArray.h"

#include <cstdlib>
#include <string>

namespace
{
// A triangle in one quadrant of the view, centered on (x, y).
vtkNew<vtkPoints> MakeTriangle(double x, double y)
{
  vtkNew<vtkPoints> points;
  points->InsertNextPoint(x - 0.3, y - 0.3, 0.0);
  points->InsertNextPoint(x + 0.3, y - 0.3, 0.0);
  points->InsertNextPoint(x, y + 0.3, 0.0);
  return points;
}

// The color at a pixel inside the triangle of a quadrant of a 100 x 100 window.
char Color(vtkRenderWindow* window, double x, double y)
{
  vtkNew<vtkUnsignedCharArray> pixels;
  const int px = static_cast<int>((x + 1.0) * 50.0);
  const int py = static_cast<int>((y - 0.1 + 1.0) * 50.0);
  window->GetRGBACharPixelData(px, py, px, py, /*front=*/1, pixels);
  const int red = pixels->GetValue(0);
  const int green = pixels->GetValue(1);
  if (red > 200 && green < 60)
  {
    return 'r';
  }
  if (green > 200 && red < 60)
  {
    return 'g';
  }
  return red < 60 && green < 60 ? '-' : '?';
}

// Expected colors of the bottom left, bottom right, top left and top right quadrants.
bool Check(vtkRenderWindow* window, const char* expected, const char* label)
{
  window->Render();
  const char actual[] = { Color(window, -0.5, -0.5), Color(window, 0.5, -0.5),
    Color(window, -0.5, 0.5), Color(window, 0.5, 0.5), '\0' };
  if (std::string(actual) != expected)
  {
    vtk::println("{}: expected {}, got {}", label, expected, actual);
    return false;
  }
  return true;
}
}

int TestCompositePolyDataMapperReplacedArrays(int, char*[])
{
  // Every array exists before the first render, so none is newer than an upload.
  auto bottomLeft = MakeTriangle(-0.5, -0.5);
  auto bottomRight = MakeTriangle(0.5, -0.5);
  auto topRight = MakeTriangle(0.5, 0.5);

  vtkNew<vtkCellArray> triangles;
  triangles->InsertNextCell({ 0, 1, 2 });
  vtkNew<vtkPolyData> red;
  red->SetPoints(bottomLeft);
  red->SetPolys(triangles);
  vtkNew<vtkPolyData> green;
  green->SetPoints(bottomRight);
  green->SetPolys(triangles);
  vtkNew<vtkMultiBlockDataSet> blocks;
  blocks->SetBlock(0, red);
  blocks->SetBlock(1, green);

  vtkNew<vtkCompositePolyDataMapper> mapper;
  mapper->SetInputDataObject(blocks);
  vtkNew<vtkCompositeDataDisplayAttributes> attributes;
  mapper->SetCompositeDataDisplayAttributes(attributes);
  mapper->SetBlockColor(1, 1.0, 0.0, 0.0);
  mapper->SetBlockColor(2, 0.0, 1.0, 0.0);
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->LightingOff();

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->SetBackground(0.0, 0.0, 0.0);
  vtkCamera* camera = renderer->GetActiveCamera();
  camera->ParallelProjectionOn();
  camera->SetPosition(0.0, 0.0, 1.0);
  camera->SetFocalPoint(0.0, 0.0, 0.0);
  camera->SetParallelScale(1.0);
  renderer->ResetCameraClippingRange(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);

  vtkNew<vtkRenderWindow> window;
  window->SetWindowName(__func__);
  window->SetMultiSamples(0);
  window->SetSize(100, 100);
  window->AddRenderer(renderer);

  bool ok = Check(window, "rg--", "first render");

  // A change to the mapper or to the block attributes makes WebGPU release the batch and write
  // every array again. A change in translucency only syncs the batch, so the replaced arrays
  // have to be told apart from the ones already written.
  green->SetPoints(topRight);
  actor->GetProperty()->SetOpacity(0.99);
  ok &= Check(window, "r--g", "after replacing the points of one block");

  red->SetPoints(topRight);
  green->SetPoints(bottomLeft);
  actor->GetProperty()->SetOpacity(1.0);
  ok &= Check(window, "g--r", "after swapping the points of the blocks");
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
