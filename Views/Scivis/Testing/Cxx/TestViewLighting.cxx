// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// A light the application added must survive the light kit being switched on
// and off.  Only the headlight the view owns gives way to the kit.

#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkNew.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkScivisView.h"

#include <iostream>

#define CHECK(expr, msg)                                                                           \
  do                                                                                               \
  {                                                                                                \
    if (!(expr))                                                                                   \
    {                                                                                              \
      std::cerr << "FAILED: " << msg << "\n";                                                      \
      return EXIT_FAILURE;                                                                         \
    }                                                                                              \
  } while (false)

namespace
{

int NumberOfLights(vtkScivisView* view)
{
  return view->GetRenderer()->GetLights()->GetNumberOfItems();
}

bool RendererHas(vtkScivisView* view, vtkLight* light)
{
  vtkLightCollection* lights = view->GetRenderer()->GetLights();
  lights->InitTraversal();
  while (vtkLight* candidate = lights->GetNextItem())
  {
    if (candidate == light)
    {
      return true;
    }
  }
  return false;
}

}

int TestViewLighting(int, char*[])
{
  vtkNew<vtkScivisView> view;
  view->GetRenderWindow()->SetOffScreenRendering(true);
  view->SetWindowSize(300, 300);

  // The view lights the scene itself rather than leaving vtkRenderer to invent
  // a headlight, which is what makes the rest of this test possible: a light
  // the renderer created could not be told apart from one the caller added.
  CHECK(!view->GetUseLightKit(), "the light kit is on before anything asked for it");
  CHECK(NumberOfLights(view) == 1, "the view did not bring a light of its own");

  vtkNew<vtkLight> mine;
  mine->SetLightTypeToSceneLight();
  mine->SetPosition(1.0, 1.0, 1.0);
  view->GetRenderer()->AddLight(mine);
  CHECK(NumberOfLights(view) == 2, "adding a light did not add a light");

  view->SetUseLightKit(true);
  CHECK(RendererHas(view, mine), "enabling the light kit took away a light the caller added");
  CHECK(NumberOfLights(view) == 6, "the light kit did not replace exactly the view's own light");

  view->SetUseLightKit(false);
  CHECK(RendererHas(view, mine), "disabling the light kit took away a light the caller added");
  CHECK(NumberOfLights(view) == 2, "the view's own light did not come back");

  // Rendering must not conjure up a seventh light, which is what vtkRenderer
  // does whenever it finds none.
  view->Render();
  CHECK(NumberOfLights(view) == 2, "rendering added a light");
  view->SetUseLightKit(true);
  view->Render();
  CHECK(NumberOfLights(view) == 6, "rendering with the light kit on added a light");

  return EXIT_SUCCESS;
}
