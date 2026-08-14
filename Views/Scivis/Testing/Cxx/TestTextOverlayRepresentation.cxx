// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Exercises a representation with no data behind it: what it draws, that a view
// shows and hides it like any other, and that the parts of a view which only
// make sense for data pass it over.

#include "vtkNew.h"
#include "vtkRenderer.h"
#include "vtkScivisScalarBars.h"
#include "vtkScivisView.h"
#include "vtkSphereSource.h"
#include "vtkSurfaceRepresentation.h"
#include "vtkTextActor.h"
#include "vtkTextOverlayRepresentation.h"
#include "vtkTextProperty.h"

#include <cstring>
#include <iostream>
#include <string>

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

// What the representation is: a string, somewhere, drawn or not.
int TestTextAndPosition()
{
  vtkNew<vtkTextOverlayRepresentation> text;

  CHECK(text->GetVisibility(), "a text overlay does not start out visible");

  text->SetText("Frame 12");
  CHECK(std::string(text->GetText()) == "Frame 12", "the text is not what was set");

  text->SetPosition(20, 40);
  CHECK(text->GetPosition()[0] == 20 && text->GetPosition()[1] == 40,
    "the position is not where it was put");

  text->SetVisibility(false);
  CHECK(!text->GetVisibility(), "hiding the text did not take");
  CHECK(text->GetTextActor()->GetVisibility() == 0, "the actor is still visible");

  return EXIT_SUCCESS;
}

// How it is drawn belongs to the text property, which is handed out rather than
// mirrored one method at a time.
int TestTextPropertyIsHandedOut()
{
  vtkNew<vtkTextOverlayRepresentation> text;

  vtkTextProperty* property = text->GetTextProperty();
  CHECK(property != nullptr, "no text property");
  CHECK(property == text->GetTextActor()->GetTextProperty(),
    "the property handed out is not the one the text is drawn with");

  property->SetFontSize(24);
  property->SetColor(1.0, 0.5, 0.25);
  property->SetJustificationToRight();
  CHECK(text->GetTextActor()->GetTextProperty()->GetFontSize() == 24,
    "the font size did not reach the actor");

  return EXIT_SUCCESS;
}

// A view shows and hides it exactly as it does a representation with data.
int TestAddAndRemove()
{
  vtkNew<vtkScivisView> view;
  vtkNew<vtkTextOverlayRepresentation> text;
  text->SetText("Title");

  const int propsBefore = view->GetRenderer()->GetViewProps()->GetNumberOfItems();
  view->AddRepresentation(text);
  CHECK(view->GetNumberOfRepresentations() == 1, "the text was not added");
  CHECK(view->GetRenderer()->GetViewProps()->GetNumberOfItems() == propsBefore + 1,
    "the text actor did not reach the renderer");
  CHECK(view->IsRepresentationPresent(text), "the view does not report the text as present");

  view->RemoveRepresentation(text);
  CHECK(view->GetNumberOfRepresentations() == 0, "the text was not removed");
  CHECK(view->GetRenderer()->GetViewProps()->GetNumberOfItems() == propsBefore,
    "the text actor was left in the renderer");

  return EXIT_SUCCESS;
}

// The scalar bars are for arrays, and a text overlay has none.  It must neither
// get a bar of its own nor disturb the bar of a representation that does.
int TestPassedOverByScalarBars()
{
  vtkNew<vtkSphereSource> sphere;

  vtkNew<vtkSurfaceRepresentation> surface;
  surface->SetInputConnection(sphere->GetOutputPort());
  surface->ColorByPointArray("Normals");

  vtkNew<vtkTextOverlayRepresentation> text;
  text->SetText("Frame 12");

  vtkNew<vtkScivisView> view;
  view->AddRepresentation(surface);
  view->AddRepresentation(text);
  view->Update();
  view->Render();

  CHECK(view->GetScalarBars()->GetNumberOfBars() == 1,
    "the text overlay changed how many bars there are");
  CHECK(std::string(view->GetScalarBars()->GetArrayName(0)) == "Normals",
    "the one bar is not the surface's array");

  // With the surface hidden nothing is drawing an array, so no bar is left --
  // the text alone must not keep one alive.
  surface->SetVisibility(false);
  view->Render();
  CHECK(view->GetScalarBars()->GetNumberOfBars() == 0,
    "a bar survived with only a text overlay in the view");

  return EXIT_SUCCESS;
}

}

int TestTextOverlayRepresentation(int, char*[])
{
  if (TestTextAndPosition() != EXIT_SUCCESS || TestTextPropertyIsHandedOut() != EXIT_SUCCESS ||
    TestAddAndRemove() != EXIT_SUCCESS || TestPassedOverByScalarBars() != EXIT_SUCCESS)
  {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
