// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * Makes sure that the vtkWebGPUComputeOcclusion culler's internal machinery for resizing the
 * hierarchical z-buffer works properly.
 *
 * The test renders some props and then resizes the window. If the occlusion culler handles the
 * resizing properly, the number of props culled shouldn't change (and we also shouldn't get
 * any WebGPU validation errors)
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

namespace
{
//------------------------------------------------------------------------------
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
  mapper->SetInputData(polydata);

  vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);

  return actor;
}

//------------------------------------------------------------------------------
void RenderNewTriangle(vtkRenderWindow* renWin, vtkRenderer* renderer,
  std::vector<int>& renderedPropCounts, double x1, double y1, double z1, double x2, double y2,
  double z2, double x3, double y3, double z3)
{
  renderer->AddActor(CreateTriangle(x1, y1, z1, x2, y2, z2, x3, y3, z3));
  renWin->Render();
  renderedPropCounts.push_back(renderer->GetNumberOfPropsRendered());
}

//------------------------------------------------------------------------------
void CheckRenderCount(
  const std::vector<int>& renderedPropCounts, const std::vector<int>& renderedPropCountsReference)
{
  for (std::size_t i = 0; i < renderedPropCounts.size(); i++)
  {
    if (renderedPropCounts[i] != renderedPropCountsReference[i])
    {
      std::string expectedSequence;
      std::string actualSequence;
      for (std::size_t seqIndex = 0; seqIndex < renderedPropCountsReference.size(); seqIndex++)
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
int TestComputeOcclusionCullingResize(int, char*[])
{
  // How many props are expected to be rendered at each frame (with modification of the props in
  // between the frames)
  std::vector<int> renderedPropCountsReference = { 1, 2, 3, 4, 5, 1, 5 };
  // How many props were actually renderer
  std::vector<int> renderedPropCounts;

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetWindowName(__func__);
  renWin->SetMultiSamples(0);
  renWin->SetSize(512, 512);
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

  // Resizing the window. The big triangle should still occlude the small triangles so we should
  // still get only 1 prop rendered if the depth buffer / mipmaps was properly resized when the
  // render window was resized
  renWin->SetSize(1500, 512);
  renderer->GetActiveCamera()->SetFocalPoint(-0.897737, 0.380353, -1.62994);
  renderer->GetActiveCamera()->SetPosition(-2.07265, 0.517861, 0.0729139);
  renderer->GetActiveCamera()->SetViewUp(0.0514601, 0.997658, -0.045057);

  renWin->Render();
  renderedPropCounts.push_back(renderer->GetNumberOfPropsRendered());

  CheckRenderCount(renderedPropCounts, renderedPropCountsReference);

  return EXIT_SUCCESS;
}
