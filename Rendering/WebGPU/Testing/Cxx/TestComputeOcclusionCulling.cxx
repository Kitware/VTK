// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * This test first creates a few non-overlapping triangles and then a bigger triangle that covers
 * some of the previous triangles.
 *
 * The occlusion culler is expected to render the first non-overlapping triangles but the bigger
 * triangle is expected to occlude some of the smaller triangles and thus they should be culled and
 * not rendered.
 *
 * The number of props rendered by the renderer + compute occlusion culler at each frame is then
 * compared to a reference list to make sure that the culler indeed culled (or not) props as it was
 * supposed to.
 */

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCullerCollection.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkNew.h"
#include "vtkObject.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkUnsignedCharArray.h"
#include "vtkWebGPUComputeOcclusionCuller.h"
#include "vtkWebGPURenderWindow.h"
#include "vtkXMLMultiBlockDataReader.h"

//------------------------------------------------------------------------------
namespace
{
vtkSmartPointer<vtkActor> CreateTriangle(
  double x1, double y1, double z1, double x2, double y2, double z2, double x3, double y3, double z3)
{
  vtkSmartPointer<vtkPolyData> polydata = vtkSmartPointer<vtkPolyData>::New();
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->InsertPoint(0, x1, y1, z1);
  points->InsertPoint(1, x2, y2, z2);
  points->InsertPoint(2, x3, y3, z3);
  polydata->SetPoints(points);
  vtkSmartPointer<vtkCellArray> triangle = vtkSmartPointer<vtkCellArray>::New();
  triangle->InsertNextCell({ 0, 1, 2 });
  polydata->SetPolys(triangle);

  vtkSmartPointer<vtkUnsignedCharArray> colors = vtkSmartPointer<vtkUnsignedCharArray>::New();
  colors->SetNumberOfComponents(4);
  colors->SetNumberOfTuples(3);
  for (int i = 0; i < 3; i++)
  {
    colors->InsertComponent(i, 0, 255);
    colors->InsertComponent(i, 1, 255);
    colors->InsertComponent(i, 2, 255);
    colors->InsertComponent(i, 3, 255);
  }
  polydata->GetPointData()->SetScalars(colors);

  vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->DebugOn();
  mapper->SetInputData(polydata);

  vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);

  return actor;
}

void RenderNewTriangle(vtkRenderWindow* renWin, vtkRenderer* renderer,
  std::vector<int>& renderedPropCounts, double x1, double y1, double z1, double x2, double y2,
  double z2, double x3, double y3, double z3)
{
  renderer->AddActor(CreateTriangle(x1, y1, z1, x2, y2, z2, x3, y3, z3));
  renWin->Render();
  renderedPropCounts.push_back(renderer->GetNumberOfPropsRendered());
}

void CheckRenderCount(
  const std::vector<int>& renderedPropCounts, const std::vector<int>& renderedPropCountsReference)
{
  for (std::size_t i = 0; i < renderedPropCounts.size(); i++)
  {
    if (renderedPropCounts[i] != renderedPropCountsReference[i])
    {
      std::string expectedSequence;
      std::string actualSequence;
      for (std::size_t seqIndex = 0; seqIndex < renderedPropCounts.size(); seqIndex++)
      {
        expectedSequence += std::to_string(renderedPropCountsReference[seqIndex]) + ", ";
        actualSequence += std::to_string(renderedPropCounts[seqIndex]) + ", ";
      }

      vtkLog(ERROR,
        "The right number of props wasn't rendered. Expected sequence of rendered props was: "
          << expectedSequence << " but the actual sequence was: " << actualSequence);

      std::exit(EXIT_FAILURE);
    }
  }
}
}

//------------------------------------------------------------------------------
int TestComputeOcclusionCulling(int, char*[])
{
  // How many props are expected to be rendered at each frame (with modification of the props in
  // between the frames)
  std::vector<int> renderedPropCountsReference = { 1, 2, 3, 4, 5, 1 };
  // How many props were actually renderer
  std::vector<int> renderedPropCounts;

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetWindowName(__func__);
  renWin->SetMultiSamples(0);
  renWin->SetSize(1280, 720);
  // Initialize() call necessary when a WebGPU compute class is going to use the render window.
  // Here, the OcclusionCuller internally uses the resources of the render window so Initialize()
  // must be called
  renWin->Initialize();

  vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();

  vtkNew<vtkCamera> camera;
  camera->SetFocalPoint(0, 0.25, -1);
  renderer->SetActiveCamera(camera);
  renderer->SetBackground(0.2, 0.3, 0.4);
  renWin->AddRenderer(renderer);

  // Removing the default culler
  renderer->GetCullers()->RemoveAllItems();

  // Adding the WebGPU compute shader occlusion+frustum culler
  vtkNew<vtkWebGPUComputeOcclusionCuller> webgpuOcclusionCuller;
  webgpuOcclusionCuller->SetRenderWindow(vtkWebGPURenderWindow::SafeDownCast(renWin));
  renderer->GetCullers()->AddItem(webgpuOcclusionCuller);

  // Small triangle 1
  RenderNewTriangle(renWin, renderer, renderedPropCounts, -1, 0, -5, -0.5, 0.0, -5, -0.75, 0.5, -5);
  CheckRenderCount(renderedPropCounts, renderedPropCountsReference);

  // Small triangle 2
  RenderNewTriangle(
    renWin, renderer, renderedPropCounts, -0.5, 0, -5, 0.0, 0.0, -5, -0.25, 0.5, -5);
  CheckRenderCount(renderedPropCounts, renderedPropCountsReference);

  // Small triangle 3
  RenderNewTriangle(renWin, renderer, renderedPropCounts, 0, 0, -5, 0.5, 0.0, -5, 0.25, 0.5, -5);
  CheckRenderCount(renderedPropCounts, renderedPropCountsReference);

  // Small triangle 4
  RenderNewTriangle(renWin, renderer, renderedPropCounts, 0.5, 0, -5, 1.0, 0.0, -5, 0.75, 0.5, -5);
  CheckRenderCount(renderedPropCounts, renderedPropCountsReference);

  // Big triangle that covers all the small triangles. It is expected that that the first frame
  // rendered with the big triangle doesn't cull the small triangles
  RenderNewTriangle(renWin, renderer, renderedPropCounts, -1, -0.5, -1, 5.0, -0.5, -1, -1, 1.5, -1);
  CheckRenderCount(renderedPropCounts, renderedPropCountsReference);

  // However, if we render another frame, still with the big triangle in front, all the small
  // triangles should be culled
  renWin->Render();
  renderedPropCounts.push_back(renderer->GetNumberOfPropsRendered());
  CheckRenderCount(renderedPropCounts, renderedPropCountsReference);

  return EXIT_SUCCESS;
}
