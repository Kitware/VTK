// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Polygon cell IDs follow the vertex and line cells of a vtkPolyData. Adding
// such cells to a mesh whose polygons do not change must still shift the cell
// IDs that the polygons write to the selection buffer.

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkTypeUInt32Array.h"
#include "vtkWebGPURenderWindow.h"
#include "vtkWebGPURenderer.h"

#include <array>
#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>

namespace
{
constexpr int Width = 200;
constexpr int Height = 200;

bool CheckCellIds(vtkWebGPURenderWindow* window, vtkRenderer* renderer,
  const std::vector<std::array<double, 2>>& centers, vtkTypeUInt32 cellOffset,
  const std::string& description)
{
  window->Render();
  vtkNew<vtkTypeUInt32Array> ids;
  window->GetIdsData(0, 0, Width - 1, Height - 1, ids);
  if (ids->GetNumberOfValues() != Width * Height * 4)
  {
    vtkLog(ERROR, << description << ": incomplete ID readback");
    return false;
  }
  for (std::size_t cellId = 0; cellId < centers.size(); ++cellId)
  {
    renderer->SetWorldPoint(centers[cellId][0], centers[cellId][1], 0.0, 1.0);
    renderer->WorldToDisplay();
    const auto* display = renderer->GetDisplayPoint();
    const int x = static_cast<int>(std::floor(display[0]));
    const int y = static_cast<int>(std::floor(display[1]));
    // The selection buffer stores cell IDs incremented by one; zero is the background.
    const vtkTypeUInt32 stored = ids->GetValue(4 * (y * Width + x));
    if (stored != cellId + cellOffset + 1)
    {
      vtkLog(ERROR, << description << ": polygon " << cellId << " at pixel (" << x << ", " << y
                    << ") stored cell ID " << stored << ", expected " << cellId + cellOffset + 1);
      return false;
    }
  }
  return true;
}
}

int TestPolygonCellIdOffset(int, char*[])
{
  // Four triangles around a center point, which also hosts the vertex and line cells added later.
  vtkNew<vtkPoints> points;
  points->InsertNextPoint(0.0, 0.0, 0.0);
  points->InsertNextPoint(-1.0, -1.0, 0.0);
  points->InsertNextPoint(1.0, -1.0, 0.0);
  points->InsertNextPoint(1.0, 1.0, 0.0);
  points->InsertNextPoint(-1.0, 1.0, 0.0);
  vtkNew<vtkCellArray> polygons;
  std::vector<std::array<double, 2>> centers;
  for (vtkIdType i = 0; i < 4; ++i)
  {
    const vtkIdType triangle[3] = { 0, 1 + i, 1 + (i + 1) % 4 };
    polygons->InsertNextCell(3, triangle);
    double a[3], b[3];
    points->GetPoint(triangle[1], a);
    points->GetPoint(triangle[2], b);
    centers.push_back({ (a[0] + b[0]) / 3.0, (a[1] + b[1]) / 3.0 });
  }
  vtkNew<vtkPolyData> mesh;
  mesh->SetPoints(points);
  mesh->SetPolys(polygons);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputData(mesh);
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->LightingOff();
  vtkNew<vtkRenderer> baseRenderer;
  vtkNew<vtkRenderWindow> baseWindow;
  auto* renderer = vtkWebGPURenderer::SafeDownCast(baseRenderer);
  auto* window = vtkWebGPURenderWindow::SafeDownCast(baseWindow);
  if (!renderer || !window)
  {
    vtkLog(ERROR, "This test requires the WebGPU rendering backend");
    return EXIT_FAILURE;
  }
  window->SetSize(Width, Height);
  window->SetMultiSamples(0);
  window->SetShowWindow(false);
  window->SetUseOffScreenBuffers(true);
  window->AddRenderer(renderer);
  renderer->UseDeepFramebufferTransparencyOff();
  renderer->AddActor(actor);
  auto* camera = renderer->GetActiveCamera();
  camera->SetPosition(0.0, 0.0, 10.0);
  camera->SetFocalPoint(0.0, 0.0, 0.0);
  camera->ParallelProjectionOn();
  camera->SetParallelScale(1.1);
  camera->SetClippingRange(1.0, 20.0);

  bool ok = true;
  for (bool useBundles : { false, true })
  {
    renderer->SetUseRenderBundles(useBundles);
    const std::string mode = useBundles ? " with render bundles" : " without render bundles";
    vtkNew<vtkCellArray> noVertices;
    vtkNew<vtkCellArray> noLines;
    mesh->SetVerts(noVertices);
    mesh->SetLines(noLines);
    ok &= CheckCellIds(window, renderer, centers, 0, "polygons only" + mode);

    vtkNew<vtkCellArray> vertices;
    const vtkIdType vertex = 0;
    vertices->InsertNextCell(1, &vertex);
    vtkNew<vtkCellArray> lines;
    const vtkIdType line[2] = { 0, 0 };
    lines->InsertNextCell(2, line);
    mesh->SetVerts(vertices);
    mesh->SetLines(lines);
    ok &= CheckCellIds(window, renderer, centers, 2, "after a vertex and a line" + mode);
  }
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
