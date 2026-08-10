// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * This test guards against unnecessary render bundle re-records in vtkWebGPURenderer.
 *
 * A render bundle is a fixed list of draw commands replayed across frames. Re-recording it
 * every frame defeats its purpose, so the renderer must only invalidate the bundle when the
 * set of visible props actually changes (visibility toggles, additions, removals), never
 * because the default vtkFrustumCoverageCuller resized the culled prop list on camera motion.
 *
 * After vtkRenderWindow::Render() returns, vtkWebGPURenderer::GetRebuildRenderBundle()
 * still holds the decision made for that frame (it is only recomputed at the start of the
 * next frame), so the test renders a frame and then asserts whether the bundle was
 * re-recorded.
 */

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkWebGPURenderer.h"

namespace
{
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
bool RenderAndCheck(vtkRenderWindow* renWin, vtkWebGPURenderer* renderer, bool expectRebuild,
  int expectedPropsRendered, const std::string& frameDescription)
{
  renWin->Render();
  if (renderer->GetRebuildRenderBundle() != expectRebuild)
  {
    vtkLog(ERROR,
      "Frame '" << frameDescription << "': expected the render bundle to "
                << (expectRebuild ? "be re-recorded" : "be reused") << " but it was "
                << (expectRebuild ? "reused" : "re-recorded") << ".");
    return false;
  }
  if (renderer->GetNumberOfPropsRendered() != expectedPropsRendered)
  {
    vtkLog(ERROR,
      "Frame '" << frameDescription << "': expected " << expectedPropsRendered
                << " props rendered but got " << renderer->GetNumberOfPropsRendered() << ".");
    return false;
  }
  return true;
}
}

//------------------------------------------------------------------------------
int TestRenderBundleReuse(int, char*[])
{
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetWindowName(__func__);
  renWin->SetMultiSamples(0);
  renWin->Initialize();

  vtkNew<vtkRenderer> renderer;
  renWin->AddRenderer(renderer);

  auto* webgpuRenderer = vtkWebGPURenderer::SafeDownCast(renderer);
  if (!webgpuRenderer)
  {
    vtkLog(ERROR, "The renderer is not a vtkWebGPURenderer.");
    return EXIT_FAILURE;
  }
  if (!webgpuRenderer->GetUseRenderBundles())
  {
    vtkLog(ERROR, "Render bundles are expected to be enabled by default.");
    return EXIT_FAILURE;
  }

  vtkNew<vtkCamera> camera;
  camera->SetFocalPoint(0, 0.25, -1);
  renderer->SetActiveCamera(camera);
  renderer->SetBackground(0.2, 0.3, 0.4);

  // In front of the camera: never culled.
  vtkSmartPointer<vtkActor> centerTriangle = CreateTriangle(-1, 0.5, -3, 1, 0.5, -3, 0, 1.5, -3);
  renderer->AddActor(centerTriangle);
  // Far off to the side, outside the view frustum: the default frustum coverage culler
  // removes it from the culled prop list every frame. While render bundles are enabled the
  // renderer restores it for drawing, so it still counts as a rendered prop.
  vtkSmartPointer<vtkActor> offscreenTriangle = CreateTriangle(-5, 0, -3, -3, 0, -3, -4, 1, -3);
  renderer->AddActor(offscreenTriangle);

  // First frame: no bundle exists yet, so one must be recorded.
  if (!RenderAndCheck(renWin, webgpuRenderer, true, 2, "first frame records the bundle"))
  {
    return EXIT_FAILURE;
  }

  // Nothing changed. The culler still removes the offscreen triangle from the culled prop
  // list every frame; that must not read as a changed prop set. This is the frame that used
  // to re-record the bundle before the visible-prop-set signature was introduced.
  if (!RenderAndCheck(renWin, webgpuRenderer, false, 2, "steady state reuses the bundle"))
  {
    return EXIT_FAILURE;
  }

  // Camera motion changes what the culler removes, but not what the application made
  // visible, so the bundle must survive it.
  camera->Azimuth(5);
  if (!RenderAndCheck(renWin, webgpuRenderer, false, 2, "camera motion reuses the bundle"))
  {
    return EXIT_FAILURE;
  }
  camera->Azimuth(-5);
  if (!RenderAndCheck(renWin, webgpuRenderer, false, 2, "camera motion back reuses the bundle"))
  {
    return EXIT_FAILURE;
  }

  // Hiding a prop changes the visible set: the bundle must be re-recorded once, then reused.
  offscreenTriangle->VisibilityOff();
  if (!RenderAndCheck(renWin, webgpuRenderer, true, 1, "hiding a prop re-records the bundle"))
  {
    return EXIT_FAILURE;
  }
  if (!RenderAndCheck(renWin, webgpuRenderer, false, 1, "steady state after hiding a prop"))
  {
    return EXIT_FAILURE;
  }

  // Showing it again re-records once more.
  offscreenTriangle->VisibilityOn();
  if (!RenderAndCheck(renWin, webgpuRenderer, true, 2, "showing a prop re-records the bundle"))
  {
    return EXIT_FAILURE;
  }
  if (!RenderAndCheck(renWin, webgpuRenderer, false, 2, "steady state after showing a prop"))
  {
    return EXIT_FAILURE;
  }

  // Adding an actor that is not visible does not change the visible set: the bundle
  // must be reused.
  vtkSmartPointer<vtkActor> hiddenTriangle = CreateTriangle(-1, -1, -3, 1, -1, -3, 0, 0, -3);
  hiddenTriangle->VisibilityOff();
  renderer->AddActor(hiddenTriangle);
  if (!RenderAndCheck(renWin, webgpuRenderer, false, 2, "adding a hidden prop reuses the bundle"))
  {
    return EXIT_FAILURE;
  }

  // Hiding one prop while showing another keeps the count identical; only a check on the
  // identity of the visible props catches this swap.
  centerTriangle->VisibilityOff();
  hiddenTriangle->VisibilityOn();
  if (!RenderAndCheck(
        renWin, webgpuRenderer, true, 2, "swapping visible props re-records the bundle"))
  {
    return EXIT_FAILURE;
  }
  if (!RenderAndCheck(renWin, webgpuRenderer, false, 2, "steady state after the swap"))
  {
    return EXIT_FAILURE;
  }

  // Removing a visible actor changes the visible set.
  renderer->RemoveActor(offscreenTriangle);
  if (!RenderAndCheck(renWin, webgpuRenderer, true, 1, "removing a prop re-records the bundle"))
  {
    return EXIT_FAILURE;
  }
  if (!RenderAndCheck(renWin, webgpuRenderer, false, 1, "steady state after removing a prop"))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
