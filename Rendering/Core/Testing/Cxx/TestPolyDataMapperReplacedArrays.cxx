// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Replacing a polydata's points or cell normals with another array of the same size must be
// rendered, even when that array was created before the one currently on the device.

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkStringFormatter.h"
#include "vtkUnsignedCharArray.h"

#include <cstdlib>

namespace
{
// A triangle in the left (offset -0.5) or right (offset 0.5) half of the view.
void MakeTriangle(vtkPoints* points, double offset)
{
  points->InsertNextPoint(offset - 0.4, -0.9, 0.0);
  points->InsertNextPoint(offset + 0.4, -0.9, 0.0);
  points->InsertNextPoint(offset, 0.9, 0.0);
}

int Red(vtkRenderWindow* window, int x, int y)
{
  vtkNew<vtkUnsignedCharArray> pixels;
  window->GetRGBACharPixelData(x, y, x, y, /*front=*/1, pixels);
  return pixels->GetValue(0);
}

// Pixels below the middle of each triangle in a 100 x 100 window.
constexpr int LeftX = 25;
constexpr int RightX = 75;
constexpr int Y = 30;

bool CheckPosition(vtkRenderWindow* window, bool left, const char* label)
{
  window->Render();
  const bool leftDrawn = Red(window, LeftX, Y) > 200;
  const bool rightDrawn = Red(window, RightX, Y) > 200;
  if (leftDrawn != left || rightDrawn != !left)
  {
    vtk::println("{}: expected the {} triangle, left drawn: {}, right drawn: {}", label,
      left ? "left" : "right", leftDrawn, rightDrawn);
    return false;
  }
  return true;
}

bool CheckLighting(vtkRenderWindow* window, bool lit, const char* label)
{
  window->Render();
  const int red = Red(window, LeftX, Y);
  if (lit ? red < 150 : red > 60)
  {
    vtk::println("{}: expected the triangle {}, red: {}", label, lit ? "lit" : "unlit", red);
    return false;
  }
  return true;
}

vtkNew<vtkFloatArray> MakeNormals(double x, double y, double z)
{
  vtkNew<vtkFloatArray> normals;
  normals->SetNumberOfComponents(3);
  normals->InsertNextTuple3(x, y, z);
  return normals;
}
}

int TestPolyDataMapperReplacedArrays(int, char*[])
{
  // Every array exists before the first render, so none is newer than an upload.
  vtkNew<vtkPoints> left;
  MakeTriangle(left, -0.5);
  vtkNew<vtkPoints> right;
  MakeTriangle(right, 0.5);
  // The headlight lights a cell whose normal faces the camera; a normal perpendicular to
  // the view direction leaves it dark.
  auto facing = MakeNormals(0.0, 0.0, 1.0);
  auto sideways = MakeNormals(1.0, 0.0, 0.0);

  vtkNew<vtkCellArray> triangles;
  triangles->InsertNextCell({ 0, 1, 2 });
  vtkNew<vtkPolyData> mesh;
  mesh->SetPoints(left);
  mesh->SetPolys(triangles);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputData(mesh);
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetColor(1.0, 0.0, 0.0);
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

  bool ok = CheckPosition(window, true, "first render");
  mesh->SetPoints(right);
  ok &= CheckPosition(window, false, "after replacing the points");
  // Swap back to the points that were uploaded first.
  mesh->SetPoints(left);
  ok &= CheckPosition(window, true, "after restoring the first points");

  actor->GetProperty()->LightingOn();
  mesh->GetCellData()->SetNormals(facing);
  ok &= CheckLighting(window, true, "with cell normals facing the camera");
  mesh->GetCellData()->SetNormals(sideways);
  ok &= CheckLighting(window, false, "after replacing the cell normals");
  mesh->GetCellData()->SetNormals(facing);
  ok &= CheckLighting(window, true, "after restoring the first cell normals");
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
