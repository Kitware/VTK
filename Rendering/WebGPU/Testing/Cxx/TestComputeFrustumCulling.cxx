// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * This test creates a few triangles and moves them around in the scene. The frustum culler is
 * expected to pick up on the recomputed bounds when the actors are moved around and the culling
 * should cull accordingly to the position of the actors.
 * The number of props rendered by the renderer + compute frustum culler at each frame is then
 * compared to a reference list to make sure that the culler indeed culled (or not) props
 * correctly
 */

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCullerCollection.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkWebGPUComputeFrustumCuller.h"

//------------------------------------------------------------------------------
vtkSmartPointer<vtkActor> CreateTriangle(
  float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3, float z3)
{
  vtkNew<vtkPolyData> polydata;
  vtkNew<vtkPoints> points;
  points->InsertPoint(0, x1, y1, z1);
  points->InsertPoint(1, x2, y2, z2);
  points->InsertPoint(2, x3, y3, z3);
  polydata->SetPoints(points);
  vtkNew<vtkCellArray> triangle;
  triangle->InsertNextCell({ 0, 1, 2 });
  polydata->SetPolys(triangle);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputData(polydata);

  vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);

  return actor;
}

//------------------------------------------------------------------------------
bool renderAndCheckResults(vtkRenderWindow* renWin, vtkRenderer* renderer,
  std::vector<int>& renderedPropCounts, const std::vector<int>& renderedPropCountsReference)
{
  renWin->Render();
  renderedPropCounts.push_back(renderer->GetNumberOfPropsRendered());

  for (std::size_t i = 0; i < renderedPropCounts.size(); i++)
  {
    if (renderedPropCounts[i] != renderedPropCountsReference[i])
    {
      vtkLog(ERROR,
        "Number of props rendered at frame " << i << " (" << renderedPropCounts[i] << ")"
                                             << " was different than expected ("
                                             << renderedPropCountsReference[i] << ").");

      return false;
    }
  }

  return true;
}

//------------------------------------------------------------------------------
int TestComputeFrustumCulling(int, char*[])
{
  // How many props are expected to be rendered at each frame (with modification of the props in
  // between the frames)
  std::vector<int> renderedPropCountsReference = { 0, 1, 2, 2, 1 };
  // How many props were actually renderer
  std::vector<int> renderedPropCounts;

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetWindowName(__func__);
  renWin->SetMultiSamples(0);
  renWin->Initialize();

  vtkNew<vtkRenderer> renderer;
  renWin->AddRenderer(renderer);

  vtkNew<vtkCamera> camera;
  camera->SetFocalPoint(0, 0.25, -1);
  renderer->SetActiveCamera(camera);
  renderer->SetBackground(0.2, 0.3, 0.4);

  // Removing the default culler
  renderer->GetCullers()->RemoveAllItems();

  // Adding the WebGPU compute shader frustum culler
  vtkNew<vtkWebGPUComputeFrustumCuller> webgpuFrustumCuller;
  renderer->GetCullers()->AddItem(webgpuFrustumCuller);

  renderer->AddActor(CreateTriangle(-5, 0, -3, -3, 0, -3, -4, 1, -3));
  if (!renderAndCheckResults(renWin, renderer, renderedPropCounts, renderedPropCountsReference))
  {
    return EXIT_FAILURE;
  }

  // This one should not be culled
  vtkSmartPointer<vtkActor> secondTriangle = CreateTriangle(-1, 0.5, -3, 1, 0.5, -3, 0, 1.5, -3);
  renderer->AddActor(secondTriangle);
  if (!renderAndCheckResults(renWin, renderer, renderedPropCounts, renderedPropCountsReference))
  {
    return EXIT_FAILURE;
  }

  // This one should not be culled
  vtkSmartPointer<vtkActor> thirdTriangle = CreateTriangle(0, 0.5, -3, 1, 0.25, -5, 0.5, 1.05, -4);
  renderer->AddActor(thirdTriangle);
  if (!renderAndCheckResults(renWin, renderer, renderedPropCounts, renderedPropCountsReference))
  {
    return EXIT_FAILURE;
  }

  // Moving the second triangle down, should still not be culled
  secondTriangle->SetPosition(0, -0.5, 0);
  if (!renderAndCheckResults(renWin, renderer, renderedPropCounts, renderedPropCountsReference))
  {
    return EXIT_FAILURE;
  }

  // Moving the third triangle behind the camera, should be culled
  thirdTriangle->SetPosition(0, 0, 10);
  if (!renderAndCheckResults(renWin, renderer, renderedPropCounts, renderedPropCountsReference))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
