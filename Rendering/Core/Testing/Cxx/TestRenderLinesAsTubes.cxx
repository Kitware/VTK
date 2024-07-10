// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This tests "Render Lines As Tubes" feature with a sphere actor and different
// lighting configurations.
//

#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkLightKit.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"

namespace
{
/**
 * Create scene displaying a sphere actor with some text. Parameters controls rendering properties
 * and text content.
 */
vtkSmartPointer<vtkRenderer> CreateScene(
  bool renderLinesAsTubes, bool enableLighting, const std::string& displayedText)
{
  vtkNew<vtkSphereSource> sphere;

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(sphere->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetEdgeVisibility(true);
  actor->GetProperty()->SetRepresentation(VTK_SURFACE);
  actor->GetProperty()->SetRenderLinesAsTubes(renderLinesAsTubes);
  actor->GetProperty()->SetEdgeColor(0.3, 0.8, 0.3);
  actor->GetProperty()->SetLineWidth(2.5);
  actor->GetProperty()->SetLighting(enableLighting);

  vtkNew<vtkTextActor> textActor;
  textActor->SetInput(displayedText.c_str());
  textActor->GetTextProperty()->SetJustificationToCentered();
  textActor->GetTextProperty()->SetColor(1.0, 1.0, 1.0);
  textActor->GetTextProperty()->SetFontSize(20);
  textActor->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
  textActor->GetPositionCoordinate()->SetValue(0.5, 0.01);

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->AddActor2D(textActor);
  renderer->ResetCamera();

  vtkNew<vtkLightKit> lightKit;
  lightKit->AddLightsToRenderer(renderer);

  return renderer;
}
}

//----------------------------------------------------------------------------
/**
 * Test that rendering lines as tubes is correctly enabled/disabled depending on different
 * lighting configurations.
 */
int TestRenderLinesAsTubes(int argc, char* argv[])
{
  // Render lines as tubes disabled (bottom left)
  auto renderer1 = ::CreateScene(false, true, "Render lines as tubes off\nlighting on");
  renderer1->SetViewport(0.0, 0.0, 0.5, 0.5);

  // Render lines as tubes enabled (bottom right)
  auto renderer2 = ::CreateScene(true, true, "Render lines as tubes on\nlighting on");
  renderer2->SetViewport(0.5, 0.0, 1.0, 0.5);

  // Render lines as tubes enabled but not effective since lighting is disabled
  // through actor rendering properties (top left)
  auto renderer3 = ::CreateScene(true, false, "Render lines as tubes on,\nlighting off");
  renderer3->SetViewport(0.0, 0.5, 0.5, 1.0);

  // Render lines as tubes enabled but not effective since we removed all light
  // sources (top right)
  auto renderer4 = ::CreateScene(true, true, "Render lines as tubes on,\nno lights");
  renderer4->AutomaticLightCreationOff();
  renderer4->RemoveAllLights();
  renderer4->SetViewport(0.5, 0.5, 1.0, 1.0);

  // Set up render window & interactor
  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer1);
  renWin->AddRenderer(renderer2);
  renWin->AddRenderer(renderer3);
  renWin->AddRenderer(renderer4);
  renWin->SetSize(800, 800);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // Baseline comparison
  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
