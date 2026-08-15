// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// The properties promoted onto the representations and components are the ones
// applications reach for often enough that going through the object underneath
// is a nuisance.  Being promoted, they must not become a second copy of the
// truth: each one is checked to read back what was set, and to agree with the
// object it forwards to.

#include "vtkGridAxesActor3D.h"
#include "vtkGridAxesRepresentation.h"
#include "vtkNew.h"
#include "vtkProperty.h"
#include "vtkScalarBarActor.h"
#include "vtkScivisScalarBars.h"
#include "vtkScivisView.h"
#include "vtkSphereSource.h"
#include "vtkSurfaceRepresentation.h"
#include "vtkTextOverlayRepresentation.h"
#include "vtkTextProperty.h"
#include "vtkVolumeProperty.h"
#include "vtkVolumeRepresentation.h"

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

int TestGridAxes()
{
  vtkNew<vtkGridAxesRepresentation> axes;

  axes->SetXTitle("x (m)");
  axes->SetYTitle("y (m)");
  axes->SetZTitle("z (m)");
  CHECK(std::string(axes->GetXTitle()) == "x (m)", "the x title did not read back");
  CHECK(axes->GetGridAxesActor()->GetTitle(1) == "y (m)",
    "the y title did not reach the actor it is drawn from");
  CHECK(std::string(axes->GetZTitle()) == "z (m)", "the z title did not read back");

  axes->SetLabelFontSize(24);
  axes->SetTitleFontSize(30);
  CHECK(axes->GetLabelFontSize() == 24, "the label font size did not read back");
  CHECK(axes->GetTitleFontSize() == 30, "the title font size did not read back");
  for (int axis = 0; axis < 3; ++axis)
  {
    CHECK(axes->GetGridAxesActor()->GetLabelTextProperty(axis)->GetFontSize() == 24,
      "an axis was left at a different label font size");
    CHECK(axes->GetGridAxesActor()->GetTitleTextProperty(axis)->GetFontSize() == 30,
      "an axis was left at a different title font size");
  }

  axes->SetLabelColor(1.0, 0.5, 0.25);
  CHECK(axes->GetLabelColor()[1] == 0.5, "the label color did not read back");
  CHECK(axes->GetGridAxesActor()->GetLabelTextProperty(2)->GetColor()[1] == 0.5,
    "an axis was left a different label color");

  axes->SetTitleColor(0.25, 0.5, 1.0);
  CHECK(axes->GetTitleColor()[2] == 1.0, "the title color did not read back");

  return EXIT_SUCCESS;
}

// The bars are made by the view as arrays appear, so what the set says has to
// reach a bar that did not exist when it was said.
int TestScalarBarsStyleReachesBarsMadeLater()
{
  vtkNew<vtkScivisView> view;
  vtkScivisScalarBars* bars = view->GetScalarBars();

  bars->SetTitleFontSize(28);
  bars->SetLabelFontSize(22);
  bars->SetTextColor(1.0, 0.0, 0.0);
  bars->SetNumberOfLabels(9);
  CHECK(bars->GetNumberOfBars() == 0, "there is a bar before anything is drawn");

  // Only now does a bar exist.
  vtkNew<vtkSphereSource> sphere;
  vtkNew<vtkSurfaceRepresentation> surface;
  surface->SetInputConnection(sphere->GetOutputPort());
  surface->ColorByPointArray("Normals");
  view->AddRepresentation(surface);
  view->Render();

  CHECK(bars->GetNumberOfBars() == 1, "no bar was made");
  vtkScalarBarActor* bar = bars->GetActor(0);
  CHECK(bar->GetTitleTextProperty()->GetFontSize() == 28,
    "a bar made later did not take the title font size");
  CHECK(bar->GetLabelTextProperty()->GetFontSize() == 22,
    "a bar made later did not take the label font size");
  CHECK(bar->GetLabelTextProperty()->GetColor()[0] == 1.0,
    "a bar made later did not take the text color");
  CHECK(bar->GetNumberOfLabels() == 9, "a bar made later did not take the number of labels");

  // And changing the set afterwards reaches the bars already there.
  bars->SetLabelFontSize(14);
  CHECK(bar->GetLabelTextProperty()->GetFontSize() == 14,
    "changing the set did not reach the bar already made");

  bars->SetBarWidth(0.12);
  bars->SetBarHeight(0.4);
  view->Render();
  CHECK(bar->GetWidth() == 0.12 && bar->GetHeight() == 0.4,
    "the bar was not laid out at the size asked for");

  return EXIT_SUCCESS;
}

int TestTextOverlay()
{
  vtkNew<vtkTextOverlayRepresentation> text;

  text->SetFontSize(36);
  text->SetColor(0.1, 0.2, 0.3);
  text->SetBold(true);
  text->SetItalic(true);

  CHECK(text->GetFontSize() == 36, "the font size did not read back");
  CHECK(text->GetTextProperty()->GetFontSize() == 36, "the font size did not reach the property");
  CHECK(text->GetColor()[2] == 0.3, "the color did not read back");
  CHECK(text->GetTextProperty()->GetColor()[2] == 0.3, "the color did not reach the property");
  CHECK(text->GetBold() && text->GetTextProperty()->GetBold(), "bold did not take");
  CHECK(text->GetItalic() && text->GetTextProperty()->GetItalic(), "italic did not take");

  return EXIT_SUCCESS;
}

int TestSurfaceAndVolume()
{
  vtkNew<vtkSurfaceRepresentation> surface;
  surface->SetLineWidth(3.0);
  surface->SetPointSize(7.0);
  surface->SetSpecular(0.4);
  surface->SetSpecularPower(25.0);
  CHECK(surface->GetLineWidth() == 3.0 && surface->GetProperty()->GetLineWidth() == 3.0,
    "line width does not agree with the actor property");
  CHECK(surface->GetPointSize() == 7.0 && surface->GetProperty()->GetPointSize() == 7.0,
    "point size does not agree with the actor property");
  CHECK(surface->GetSpecular() == 0.4 && surface->GetProperty()->GetSpecular() == 0.4,
    "specular does not agree with the actor property");
  CHECK(surface->GetSpecularPower() == 25.0 && surface->GetProperty()->GetSpecularPower() == 25.0,
    "specular power does not agree with the actor property");

  vtkNew<vtkVolumeRepresentation> volume;
  volume->SetAmbient(0.3);
  volume->SetDiffuse(0.6);
  volume->SetSpecular(0.2);
  CHECK(volume->GetAmbient() == 0.3 && volume->GetVolumeProperty()->GetAmbient() == 0.3,
    "ambient does not agree with the volume property");
  CHECK(volume->GetDiffuse() == 0.6 && volume->GetVolumeProperty()->GetDiffuse() == 0.6,
    "diffuse does not agree with the volume property");
  CHECK(volume->GetSpecular() == 0.2 && volume->GetVolumeProperty()->GetSpecular() == 0.2,
    "specular does not agree with the volume property");

  return EXIT_SUCCESS;
}

// Setting one through the object underneath is still the same thing, so the two
// cannot drift apart.
int TestNeitherSideIsACopy()
{
  vtkNew<vtkTextOverlayRepresentation> text;
  text->GetTextProperty()->SetFontSize(41);
  CHECK(text->GetFontSize() == 41, "the promoted property did not see a change made underneath");

  vtkNew<vtkSurfaceRepresentation> surface;
  surface->GetProperty()->SetLineWidth(5.0);
  CHECK(surface->GetLineWidth() == 5.0, "the promoted line width did not see the actor's change");

  vtkNew<vtkGridAxesRepresentation> axes;
  axes->GetGridAxesActor()->GetLabelTextProperty(0)->SetFontSize(9);
  CHECK(axes->GetLabelFontSize() == 9, "the promoted font size did not see the actor's change");

  return EXIT_SUCCESS;
}

// ColorBySolidColor completes the family: it stops coloring by an array without
// disturbing the color to go back to.
int TestColorBySolidColor()
{
  vtkNew<vtkSphereSource> sphere;
  vtkNew<vtkSurfaceRepresentation> surface;
  surface->SetInputConnection(sphere->GetOutputPort());

  surface->SetColor(0.2, 0.4, 0.6);
  surface->ColorByPointArray("Normals");
  CHECK(surface->GetScalarVisibility(), "coloring by an array did not take");
  CHECK(surface->GetColor()[1] == 0.4, "coloring by an array threw away the solid color");

  surface->ColorBySolidColor();
  CHECK(!surface->GetScalarVisibility(), "the array is still being drawn");
  CHECK(surface->GetColor()[1] == 0.4, "the color to go back to was not kept");

  return EXIT_SUCCESS;
}

}

int TestPromotedProperties(int, char*[])
{
  if (TestGridAxes() != EXIT_SUCCESS || TestScalarBarsStyleReachesBarsMadeLater() != EXIT_SUCCESS ||
    TestTextOverlay() != EXIT_SUCCESS || TestSurfaceAndVolume() != EXIT_SUCCESS ||
    TestNeitherSideIsACopy() != EXIT_SUCCESS || TestColorBySolidColor() != EXIT_SUCCESS)
  {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
