// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Verifies vtkRenderer applies the camera's ModelTransformMatrix properly
// when resetting the camera

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCubeSource.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace
{

// Bounds with a deliberately thin Z extent, so a Z scale materially changes the
// depth range and the screen-space extent the camera has to cover.
constexpr double BOUNDS[6] = { -1000.0, 1000.0, -1000.0, 1000.0, -10.0, 10.0 };

constexpr int WINDOW_SIZE = 400;

// The clipping range lands in the thousands here, and the two code paths differ
// by a factor of the scale when the bug is present, so this is loose enough to
// absorb floating point noise and far tighter than the defect.
constexpr double RANGE_TOLERANCE = 1e-6;

struct Scene
{
  vtkNew<vtkCubeSource> Cube;
  vtkNew<vtkPolyDataMapper> Mapper;
  vtkNew<vtkActor> Actor;
  vtkNew<vtkRenderer> Renderer;
  vtkNew<vtkRenderWindow> Window;
  vtkNew<vtkMatrix4x4> ModelTransform;

  explicit Scene(double zScale)
  {
    this->Cube->SetBounds(BOUNDS[0], BOUNDS[1], BOUNDS[2], BOUNDS[3], BOUNDS[4], BOUNDS[5]);
    this->Mapper->SetInputConnection(this->Cube->GetOutputPort());
    this->Actor->SetMapper(this->Mapper);
    this->Renderer->AddActor(this->Actor);
    this->Window->AddRenderer(this->Renderer);
    this->Window->SetSize(WINDOW_SIZE, WINDOW_SIZE);

    this->ModelTransform->Identity();
    this->ModelTransform->SetElement(2, 2, zScale);
    this->Renderer->GetActiveCamera()->SetModelTransformMatrix(this->ModelTransform);
  }
};

// ResetCamera computes the clipping range internally, and the standalone
// no-argument ResetCameraClippingRange recomputes it from the same camera pose
// and the same world-space bounds. Both must expand the bounds exactly once, so
// the two ranges have to agree. They differ when ResetCamera double-expands.
bool CheckResetCameraClippingRangeConsistency(double zScale)
{
  Scene scene(zScale);
  vtkCamera* camera = scene.Renderer->GetActiveCamera();

  scene.Renderer->ResetCamera();

  double fromResetCamera[2];
  camera->GetClippingRange(fromResetCamera);

  // Same pose, same props: this path passes raw ComputeVisiblePropBounds output
  // and therefore expands exactly once.
  scene.Renderer->ResetCameraClippingRange();

  double fromClippingRangeOnly[2];
  camera->GetClippingRange(fromClippingRangeOnly);

  const double nearDiff = std::abs(fromResetCamera[0] - fromClippingRangeOnly[0]);
  const double farDiff = std::abs(fromResetCamera[1] - fromClippingRangeOnly[1]);
  const double scale = std::max(1.0, std::abs(fromClippingRangeOnly[1]));
  const bool ok = nearDiff / scale < RANGE_TOLERANCE && farDiff / scale < RANGE_TOLERANCE;

  std::cout << "  z scale " << zScale << ": ResetCamera range [" << fromResetCamera[0] << ", "
            << fromResetCamera[1] << "], ResetCameraClippingRange [" << fromClippingRangeOnly[0]
            << ", " << fromClippingRangeOnly[1] << "]";
  if (!ok)
  {
    std::cout << "  <-- FAILED";
  }
  std::cout << std::endl;

  return ok;
}

// After ResetCameraScreenSpace the data should fill the viewport up to the
// requested offset ratio. Projecting the world-space bounds corners through
// WorldToDisplay (which applies the model transform) must therefore produce a
// box that spans most of the window. When the bounds are expanded an extra time
// before projection, the computed zoom is off by the scale factor and the data
// ends up much smaller or much larger than the viewport.
bool CheckResetCameraScreenSpaceFraming(double zScale)
{
  Scene scene(zScale);

  constexpr double offsetRatio = 0.9;
  scene.Renderer->ResetCameraScreenSpace(BOUNDS, offsetRatio);

  double xmin = VTK_DOUBLE_MAX;
  double xmax = VTK_DOUBLE_MIN;
  double ymin = VTK_DOUBLE_MAX;
  double ymax = VTK_DOUBLE_MIN;
  for (int i = 0; i < 8; ++i)
  {
    double corner[4] = { BOUNDS[(i & 1)], BOUNDS[2 + ((i >> 1) & 1)], BOUNDS[4 + ((i >> 2) & 1)],
      1.0 };
    scene.Renderer->SetWorldPoint(corner);
    scene.Renderer->WorldToDisplay();

    double display[3];
    scene.Renderer->GetDisplayPoint(display);
    xmin = std::min(xmin, display[0]);
    xmax = std::max(xmax, display[0]);
    ymin = std::min(ymin, display[1]);
    ymax = std::max(ymax, display[1]);
  }

  // The larger screen-space dimension should fill the viewport up to
  // offsetRatio, within a few percent of rounding in the pixel projection.
  const double spanned = std::max(xmax - xmin, ymax - ymin) / WINDOW_SIZE;
  const bool ok = std::abs(spanned - offsetRatio) / offsetRatio < 0.1;

  std::cout << "  z scale " << zScale << ": data spans " << spanned << " of the viewport (target ~"
            << offsetRatio << ")";
  if (!ok)
  {
    std::cout << "  <-- FAILED";
  }
  std::cout << std::endl;

  return ok;
}

} // namespace

int TestResetCameraModelTransformMatrix(int, char*[])
{
  bool ok = true;

  std::cout << "ResetCamera vs ResetCameraClippingRange:" << std::endl;
  // Identity is a no-op for the transform and guards the common case.
  ok &= CheckResetCameraClippingRangeConsistency(1.0);
  ok &= CheckResetCameraClippingRangeConsistency(0.1);
  ok &= CheckResetCameraClippingRangeConsistency(10.0);

  std::cout << "ResetCameraScreenSpace framing:" << std::endl;
  ok &= CheckResetCameraScreenSpaceFraming(1.0);
  ok &= CheckResetCameraScreenSpaceFraming(0.1);
  ok &= CheckResetCameraScreenSpaceFraming(10.0);

  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
