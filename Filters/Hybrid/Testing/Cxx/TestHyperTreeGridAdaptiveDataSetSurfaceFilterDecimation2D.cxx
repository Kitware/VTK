// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAdaptiveDataSetSurfaceFilter.h"

#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkDataSetMapper.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridSource.h"
#include "vtkNew.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

#include <vtkLogger.h>

namespace
{
bool RenderAndCheckVisibleCells(vtkCamera* camera, vtkRenderWindow* renWin, double point[3],
  double position[3], vtkAdaptiveDataSetSurfaceFilter* surface, vtkPolyData* pd, vtkIdType expected)
{
  camera->SetFocalPoint(point);
  camera->SetPosition(position);
  surface->Modified();
  renWin->Render();
  if (pd->GetNumberOfCells() != expected)
  {
    vtkLogF(ERROR, "Incorrect number of visible cells. Expected %lld but got %lld.", expected,
      pd->GetNumberOfCells());
    return false;
  }
  return true;
}

bool TestSimpleDecimation()
{
  // Hyper tree grid
  vtkNew<vtkHyperTreeGridSource> htGrid;
  int maxLevel = 6;
  htGrid->SetMaxDepth(maxLevel);
  htGrid->SetDimensions(3, 4, 1);
  htGrid->SetGridScale(1.5, 1., 10.);
  htGrid->SetBranchFactor(2);
  htGrid->SetDescriptor("RRRRR.|.... .R.. RRRR R... R...|.R.. ...R ..RR .R.. R... .... ....|.... "
                        "...R ..R. .... .R.. R...|.... .... .R.. ....|....");
  htGrid->Update();
  vtkHyperTreeGrid* htg = vtkHyperTreeGrid::SafeDownCast(htGrid->GetOutput());
  htg->GetCellData()->SetScalars(htg->GetCellData()->GetArray("Depth"));

  // Data set surface
  vtkNew<vtkAdaptiveDataSetSurfaceFilter> surface;
  vtkNew<vtkRenderer> renderer;
  surface->SetRenderer(renderer);
  surface->SetInputConnection(htGrid->GetOutputPort());
  surface->SetViewPointDepend(false);
  surface->Update();
  vtkPolyData* pd = surface->GetOutput();
  double* range = pd->GetCellData()->GetArray("Depth")->GetRange();

  // Mappers
  vtkMapper::SetResolveCoincidentTopologyToPolygonOffset();
  vtkNew<vtkDataSetMapper> mapper1;
  mapper1->SetInputConnection(surface->GetOutputPort());
  mapper1->SetScalarRange(range);
  vtkNew<vtkDataSetMapper> mapper2;
  mapper2->SetInputConnection(surface->GetOutputPort());
  mapper2->ScalarVisibilityOff();

  // Actors
  vtkNew<vtkActor> actor1;
  actor1->SetMapper(mapper1);
  vtkNew<vtkActor> actor2;
  actor2->SetMapper(mapper2);
  actor2->GetProperty()->SetRepresentationToWireframe();
  actor2->GetProperty()->SetColor(.7, .7, .7);

  // Camera
  vtkNew<vtkCamera> camera;
  double point[3] = { pd->GetCenter()[0] - 0.75, pd->GetCenter()[1], pd->GetCenter()[2] };
  camera->SetClippingRange(1., 100.);
  camera->SetFocalPoint(point);
  double position[3] = { point[0], point[1], point[2] + 10 };
  camera->SetPosition(position);

  // Renderer
  renderer->SetActiveCamera(camera);
  renderer->SetBackground(1., 1., 1.);
  renderer->AddActor(actor1);
  renderer->AddActor(actor2);

  // Render window
  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);
  renWin->SetSize(400, 400);
  renWin->SetMultiSamples(0);

  // Interactor
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  surface->SetViewPointDepend(true);
  surface->Update();

  // Render and test
  renWin->Render();

  // Check all cells are rendered
  if (pd->GetNumberOfCells() != 75)
  {
    vtkLogF(ERROR, "Incorrect number of visible cells. Expected %d but got %lld.", 75,
      pd->GetNumberOfCells());
    return false;
  }

  point[0] -= 1;
  position[0] -= 1;
  if (!RenderAndCheckVisibleCells(camera, renWin, point, position, surface, pd, 70))
  {
    return false;
  }

  point[0] -= 1;
  position[0] -= 1;
  if (!RenderAndCheckVisibleCells(camera, renWin, point, position, surface, pd, 57))
  {
    return false;
  }

  point[0] += 5;
  position[0] += 5;
  if (!RenderAndCheckVisibleCells(camera, renWin, point, position, surface, pd, 35))
  {
    return false;
  }

  point[1] += 3;
  position[1] += 3;
  if (!RenderAndCheckVisibleCells(camera, renWin, point, position, surface, pd, 7))
  {
    return false;
  }

  point[1] -= 5;
  position[1] -= 5;
  if (!RenderAndCheckVisibleCells(camera, renWin, point, position, surface, pd, 34))
  {
    return false;
  }

  point[0] += 4;
  position[0] += 4;
  if (!RenderAndCheckVisibleCells(camera, renWin, point, position, surface, pd, 0))
  {
    return false;
  }

  return true;
}

bool TestMaskedDecimation()
{
  // Hyper tree grid
  vtkNew<vtkHyperTreeGridSource> htGrid;
  int maxLevel = 6;
  htGrid->SetMaxDepth(maxLevel);
  htGrid->SetDimensions(3, 4, 1);
  htGrid->SetGridScale(1.5, 1., 10.);
  htGrid->SetBranchFactor(2);
  htGrid->SetDescriptor("RRRRR.|.... .R.. RRRR R... R...|.R.. ...R ..RR .R.. R... .... ....|.... "
                        "...R ..R. .... .R.. R...|.... .... .R.. ....|....");
  htGrid->UseMaskOn();
  htGrid->SetMask("111111|0000 1111 1111 1111 1111|1111 0001 0111 0101 1011 1111 0111|1111 0111 "
                  "1111 1111 1111 1111|1111 1111 1111 1111|1111");
  htGrid->Update();
  vtkHyperTreeGrid* htg = vtkHyperTreeGrid::SafeDownCast(htGrid->GetOutput());
  htg->GetCellData()->SetScalars(htg->GetCellData()->GetArray("Depth"));

  // Data set surface
  vtkNew<vtkAdaptiveDataSetSurfaceFilter> surface;
  vtkNew<vtkRenderer> renderer;
  surface->SetRenderer(renderer);
  surface->SetInputConnection(htGrid->GetOutputPort());
  surface->SetViewPointDepend(false);
  surface->Update();
  vtkPolyData* pd = surface->GetOutput();
  double* range = pd->GetCellData()->GetArray("Depth")->GetRange();

  // Mappers
  vtkMapper::SetResolveCoincidentTopologyToPolygonOffset();
  vtkNew<vtkDataSetMapper> mapper1;
  mapper1->SetInputConnection(surface->GetOutputPort());
  mapper1->SetScalarRange(range);
  vtkNew<vtkDataSetMapper> mapper2;
  mapper2->SetInputConnection(surface->GetOutputPort());
  mapper2->ScalarVisibilityOff();

  // Actors
  vtkNew<vtkActor> actor1;
  actor1->SetMapper(mapper1);
  vtkNew<vtkActor> actor2;
  actor2->SetMapper(mapper2);
  actor2->GetProperty()->SetRepresentationToWireframe();
  actor2->GetProperty()->SetColor(.7, .7, .7);

  // Camera
  vtkNew<vtkCamera> camera;
  double point[3] = { pd->GetCenter()[0] - 0.75, pd->GetCenter()[1], pd->GetCenter()[2] };
  camera->SetClippingRange(1., 100.);
  camera->SetFocalPoint(point);
  double position[3] = { point[0], point[1], point[2] + 10 };
  camera->SetPosition(position);

  // Renderer
  renderer->SetActiveCamera(camera);
  renderer->SetBackground(1., 1., 1.);
  renderer->AddActor(actor1);
  renderer->AddActor(actor2);

  // Render window
  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);
  renWin->SetSize(400, 400);
  renWin->SetMultiSamples(0);

  // Interactor
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  surface->SetViewPointDepend(true);
  surface->Update();

  // Render and test
  renWin->Render();

  // Check all cells are rendered
  if (pd->GetNumberOfCells() != 62)
  {
    vtkLogF(ERROR, "Incorrect number of visible cells. Expected %d but got %lld.", 75,
      pd->GetNumberOfCells());
    return false;
  }

  point[0] += 3;
  position[0] += 3;
  if (!RenderAndCheckVisibleCells(camera, renWin, point, position, surface, pd, 31))
  {
    return false;
  }

  return true;
}
}

int TestHyperTreeGridAdaptiveDataSetSurfaceFilterDecimation2D(
  int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  bool status = true;
  status &= TestSimpleDecimation();
  status &= TestMaskedDecimation();
  return status ? EXIT_SUCCESS : EXIT_FAILURE;
}
