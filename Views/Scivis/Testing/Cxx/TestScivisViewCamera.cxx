// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Exercises the standard view directions: where each one leaves the camera
// looking, that the scene is framed afterwards, and that moving the camera is
// not a change to the view itself.

#include "vtkCamera.h"
#include "vtkNew.h"
#include "vtkRenderer.h"
#include "vtkScivisView.h"
#include "vtkSphereSource.h"
#include "vtkSurfaceRepresentation.h"

#include <cmath>
#include <iostream>

#define CHECK(expr, msg)                                                                           \
  do                                                                                               \
  {                                                                                                \
    if (!(expr))                                                                                   \
    {                                                                                              \
      std::cerr << "FAILED: " << (msg) << std::endl;                                               \
      return EXIT_FAILURE;                                                                         \
    }                                                                                              \
  } while (false)

namespace
{

// Whether the camera looks along `dir` with `up` pointing up.  Both are
// compared by direction rather than by value: framing moves the camera along
// the view direction, so only the direction is the preset's to promise.
bool LooksAlong(vtkCamera* camera, const double dir[3], const double up[3], const char* label)
{
  double* position = camera->GetPosition();
  double* focus = camera->GetFocalPoint();
  double view[3] = { focus[0] - position[0], focus[1] - position[1], focus[2] - position[2] };
  const double length = std::sqrt(view[0] * view[0] + view[1] * view[1] + view[2] * view[2]);
  if (length < 1e-6)
  {
    std::cerr << label << ": the camera sits on its own focal point" << std::endl;
    return false;
  }
  const double dot = (view[0] * dir[0] + view[1] * dir[1] + view[2] * dir[2]) / length;
  if (std::abs(dot - 1.0) > 1e-4)
  {
    std::cerr << label << ": looking (" << view[0] / length << ", " << view[1] / length << ", "
              << view[2] / length << "), expected (" << dir[0] << ", " << dir[1] << ", " << dir[2]
              << ")" << std::endl;
    return false;
  }
  double* viewUp = camera->GetViewUp();
  const double upDot = viewUp[0] * up[0] + viewUp[1] * up[1] + viewUp[2] * up[2];
  if (std::abs(upDot - 1.0) > 1e-4)
  {
    std::cerr << label << ": up is (" << viewUp[0] << ", " << viewUp[1] << ", " << viewUp[2]
              << "), expected (" << up[0] << ", " << up[1] << ", " << up[2] << ")" << std::endl;
    return false;
  }
  return true;
}

int TestStandardDirections()
{
  vtkNew<vtkSphereSource> sphere;
  vtkNew<vtkSurfaceRepresentation> surface;
  surface->SetInputConnection(sphere->GetOutputPort());

  vtkNew<vtkScivisView> view;
  view->AddRepresentation(surface);

  vtkCamera* camera = view->GetCamera();
  CHECK(camera != nullptr, "the view has no camera");
  CHECK(camera == view->GetRenderer()->GetActiveCamera(),
    "the camera handed out is not the one the scene is drawn with");

  const double up_z[3] = { 0.0, 0.0, 1.0 };
  const double up_y[3] = { 0.0, 1.0, 0.0 };
  const double px[3] = { 1.0, 0.0, 0.0 };
  const double nx[3] = { -1.0, 0.0, 0.0 };
  const double py[3] = { 0.0, 1.0, 0.0 };
  const double ny[3] = { 0.0, -1.0, 0.0 };
  const double pz[3] = { 0.0, 0.0, 1.0 };
  const double nz[3] = { 0.0, 0.0, -1.0 };

  view->ViewPositiveX();
  CHECK(LooksAlong(camera, px, up_z, "ViewPositiveX"), "ViewPositiveX");
  view->ViewNegativeX();
  CHECK(LooksAlong(camera, nx, up_z, "ViewNegativeX"), "ViewNegativeX");
  view->ViewPositiveY();
  CHECK(LooksAlong(camera, py, up_z, "ViewPositiveY"), "ViewPositiveY");
  view->ViewNegativeY();
  CHECK(LooksAlong(camera, ny, up_z, "ViewNegativeY"), "ViewNegativeY");
  view->ViewPositiveZ();
  CHECK(LooksAlong(camera, pz, up_y, "ViewPositiveZ"), "ViewPositiveZ");
  view->ViewNegativeZ();
  CHECK(LooksAlong(camera, nz, up_y, "ViewNegativeZ"), "ViewNegativeZ");

  // An isometric view foreshortens all three axes equally, so the view
  // direction has the same magnitude along each.
  view->ViewIsometric();
  double* position = camera->GetPosition();
  double* focus = camera->GetFocalPoint();
  const double view_dir[3] = { focus[0] - position[0], focus[1] - position[1],
    focus[2] - position[2] };
  CHECK(std::abs(std::abs(view_dir[0]) - std::abs(view_dir[1])) < 1e-3 &&
      std::abs(std::abs(view_dir[1]) - std::abs(view_dir[2])) < 1e-3,
    "an isometric view does not foreshorten the three axes equally");

  return EXIT_SUCCESS;
}

// Every direction frames the scene, so the sphere is in front of the camera and
// within the clipping range rather than behind it or beyond it.
int TestTheSceneIsFramed()
{
  vtkNew<vtkSphereSource> sphere;
  sphere->SetCenter(10.0, -4.0, 7.0);
  sphere->SetRadius(2.0);

  vtkNew<vtkSurfaceRepresentation> surface;
  surface->SetInputConnection(sphere->GetOutputPort());

  vtkNew<vtkScivisView> view;
  view->AddRepresentation(surface);

  vtkCamera* camera = view->GetCamera();
  view->ViewPositiveX();

  double* position = camera->GetPosition();
  double* focus = camera->GetFocalPoint();
  const double distance = std::sqrt((focus[0] - position[0]) * (focus[0] - position[0]) +
    (focus[1] - position[1]) * (focus[1] - position[1]) +
    (focus[2] - position[2]) * (focus[2] - position[2]));
  double* range = camera->GetClippingRange();

  CHECK(std::abs(focus[0] - 10.0) < 1e-3 && std::abs(focus[1] + 4.0) < 1e-3 &&
      std::abs(focus[2] - 7.0) < 1e-3,
    "framing did not put the focal point on the data");
  CHECK(distance > 2.0, "the camera was left inside the sphere");
  CHECK(range[0] < distance && distance < range[1], "the data is outside the clipping range");

  return EXIT_SUCCESS;
}

// The camera belongs to the renderer, and the renderer's modified time is part
// of the view's, so a camera move shows up there.  Nothing about the standard
// directions makes that so: moving the camera directly does it too, and so does
// dragging the scene about.
int TestCameraMovesShowInTheModifiedTime()
{
  vtkNew<vtkSphereSource> sphere;
  vtkNew<vtkSurfaceRepresentation> surface;
  surface->SetInputConnection(sphere->GetOutputPort());

  vtkNew<vtkScivisView> view;
  view->AddRepresentation(surface);
  view->ResetCamera();

  vtkMTimeType before = view->GetMTime();
  view->GetCamera()->Azimuth(10.0);
  CHECK(view->GetMTime() != before, "moving the camera directly did not reach the view");

  before = view->GetMTime();
  view->ViewIsometric();
  CHECK(view->GetMTime() != before, "a standard direction did not reach the view");

  return EXIT_SUCCESS;
}

}

int TestScivisViewCamera(int, char*[])
{
  if (TestStandardDirections() != EXIT_SUCCESS || TestTheSceneIsFramed() != EXIT_SUCCESS ||
    TestCameraMovesShowInTheModifiedTime() != EXIT_SUCCESS)
  {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
