// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Exercises axes that follow the scene: that they are told its size, that they
// keep up as it changes, and -- the point of taking the bounds from the
// representations rather than from the renderer -- that they do not measure
// themselves and grow every render.

#include "vtkGridAxesActor3D.h"
#include "vtkGridAxesRepresentation.h"
#include "vtkNew.h"
#include "vtkScivisView.h"
#include "vtkSphereSource.h"
#include "vtkSurfaceRepresentation.h"

#include <algorithm>
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

bool Near(double a, double b, double tolerance = 1e-6)
{
  return std::abs(a - b) < tolerance;
}

bool SameBounds(const double a[6], const double b[6], double tolerance = 1e-6)
{
  for (int i = 0; i < 6; ++i)
  {
    if (!Near(a[i], b[i], tolerance))
    {
      return false;
    }
  }
  return true;
}

// The axes are given the size of the scene, and keep up as it changes.
int TestTheAxesFollowTheScene()
{
  vtkNew<vtkSphereSource> sphere;
  sphere->SetRadius(1.0);
  sphere->SetCenter(0.0, 0.0, 0.0);

  vtkNew<vtkSurfaceRepresentation> surface;
  surface->SetInputConnection(sphere->GetOutputPort());

  vtkNew<vtkScivisView> view;
  vtkNew<vtkGridAxesRepresentation> axes;
  view->AddRepresentation(surface);
  view->AddRepresentation(axes);
  view->Render();

  double scene[6];
  CHECK(view->GetSceneBounds(scene), "the view reports no scene bounds");
  CHECK(SameBounds(axes->GetGridAxesActor()->GetGridBounds(), scene),
    "the axes were not fitted to the scene");

  // A second, larger piece of data: the axes have to grow to it.
  vtkNew<vtkSphereSource> farSphere;
  farSphere->SetRadius(1.0);
  farSphere->SetCenter(10.0, 0.0, 0.0);
  vtkNew<vtkSurfaceRepresentation> farSurface;
  farSurface->SetInputConnection(farSphere->GetOutputPort());
  view->AddRepresentation(farSurface);
  view->Render();

  // The scene is the union of what the representations draw.  Compared against
  // their own bounds rather than a number, since a tessellated sphere does not
  // quite reach its radius.
  double nearBounds[6];
  double farBounds[6];
  CHECK(surface->GetBounds(nearBounds) && farSurface->GetBounds(farBounds),
    "a representation reports no bounds");
  CHECK(view->GetSceneBounds(scene), "the view lost its scene bounds");
  CHECK(Near(scene[0], std::min(nearBounds[0], farBounds[0])) &&
      Near(scene[1], std::max(nearBounds[1], farBounds[1])),
    "the scene bounds are not the union of the two representations");
  CHECK(scene[1] > nearBounds[1], "the scene bounds did not take in the second sphere");
  CHECK(SameBounds(axes->GetGridAxesActor()->GetGridBounds(), scene),
    "the axes did not grow with the scene");

  // Hiding it takes it back out, exactly as it does for the scalar bars.
  farSurface->SetVisibility(false);
  view->Render();
  CHECK(view->GetSceneBounds(scene), "the view lost its scene bounds after hiding one");
  CHECK(SameBounds(scene, nearBounds), "a hidden representation is still being measured");
  CHECK(
    SameBounds(axes->GetGridAxesActor()->GetGridBounds(), scene), "the axes did not shrink back");

  return EXIT_SUCCESS;
}

// The axes are drawn around the data and so are always larger than it.  If they
// were measured along with it, every render would grow them a little.
int TestTheAxesDoNotMeasureThemselves()
{
  vtkNew<vtkSphereSource> sphere;
  vtkNew<vtkSurfaceRepresentation> surface;
  surface->SetInputConnection(sphere->GetOutputPort());

  vtkNew<vtkScivisView> view;
  vtkNew<vtkGridAxesRepresentation> axes;
  axes->SetPadding(0.2);
  view->AddRepresentation(surface);
  view->AddRepresentation(axes);
  view->Render();

  double first[6];
  std::copy(axes->GetGridAxesActor()->GetGridBounds(),
    axes->GetGridAxesActor()->GetGridBounds() + 6, first);

  for (int i = 0; i < 5; ++i)
  {
    view->Render();
  }

  CHECK(SameBounds(axes->GetGridAxesActor()->GetGridBounds(), first),
    "the axes grew from one render to the next, so they are measuring themselves");

  // The padded axes are larger than the data, which is what would make such a
  // loop run away if the bounds came from the renderer's props.
  double scene[6];
  CHECK(view->GetSceneBounds(scene), "the view reports no scene bounds");
  CHECK(first[1] > scene[1], "padding did not make the axes larger than the data");

  return EXIT_SUCCESS;
}

// Removing the axes stops them listening, so a scene that goes on changing does
// not go on resizing them.
int TestRemovingStopsTheListening()
{
  vtkNew<vtkSphereSource> sphere;
  vtkNew<vtkSurfaceRepresentation> surface;
  surface->SetInputConnection(sphere->GetOutputPort());

  vtkNew<vtkScivisView> view;
  vtkNew<vtkGridAxesRepresentation> axes;
  view->AddRepresentation(surface);
  view->AddRepresentation(axes);
  view->Render();

  double atRemoval[6];
  std::copy(axes->GetGridAxesActor()->GetGridBounds(),
    axes->GetGridAxesActor()->GetGridBounds() + 6, atRemoval);

  view->RemoveRepresentation(axes);
  sphere->SetRadius(5.0);
  view->Render();

  CHECK(SameBounds(axes->GetGridAxesActor()->GetGridBounds(), atRemoval),
    "the axes were still being resized after they were removed from the view");

  return EXIT_SUCCESS;
}

// A view with nothing in it has no bounds to report, and the axes are left as
// they were rather than being fitted to nothing.
int TestAnEmptyScene()
{
  vtkNew<vtkScivisView> view;
  vtkNew<vtkGridAxesRepresentation> axes;
  view->AddRepresentation(axes);
  view->Render();

  double bounds[6];
  CHECK(!view->GetSceneBounds(bounds), "an empty view reported scene bounds");
  CHECK(axes->GetGridAxesActor() != nullptr, "the axes did not survive an empty scene");

  return EXIT_SUCCESS;
}

// Padding can change when the scene has not, and has to show at once rather
// than waiting for the data to move.
int TestPaddingTakesEffectAtOnce()
{
  vtkNew<vtkSphereSource> sphere;
  vtkNew<vtkSurfaceRepresentation> surface;
  surface->SetInputConnection(sphere->GetOutputPort());

  vtkNew<vtkScivisView> view;
  vtkNew<vtkGridAxesRepresentation> axes;
  view->AddRepresentation(surface);
  view->AddRepresentation(axes);
  view->Render();

  double scene[6];
  CHECK(view->GetSceneBounds(scene), "the view reports no scene bounds");
  CHECK(SameBounds(axes->GetGridAxesActor()->GetGridBounds(), scene),
    "the axes do not start against the data");

  // No render in between: the axes must not be waiting on the next event.
  axes->SetPadding(0.5);
  const double* padded = axes->GetGridAxesActor()->GetGridBounds();
  CHECK(padded[1] > scene[1], "padding did not take effect until the scene changed");

  axes->SetPadding(0.0);
  CHECK(SameBounds(axes->GetGridAxesActor()->GetGridBounds(), scene),
    "taking the padding away did not put the axes back against the data");

  return EXIT_SUCCESS;
}

}

int TestGridAxesRepresentation(int, char*[])
{
  if (TestTheAxesFollowTheScene() != EXIT_SUCCESS ||
    TestTheAxesDoNotMeasureThemselves() != EXIT_SUCCESS ||
    TestRemovingStopsTheListening() != EXIT_SUCCESS || TestAnEmptyScene() != EXIT_SUCCESS ||
    TestPaddingTakesEffectAtOnce() != EXIT_SUCCESS)
  {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
