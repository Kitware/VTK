// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkLegendScaleActor.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"
#include "vtkTextProperty.h"

/**
 * This tests the legend scale actor in default configuration.
 */
int TestLegendScaleActorDefault(int argc, char* argv[])
{
  // Create a test pipeline
  vtkNew<vtkSphereSource> sphereSource;
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(sphereSource->GetOutputPort());
  vtkNew<vtkActor> sphereActor;
  sphereActor->SetMapper(mapper);

  // Create the actor
  vtkNew<vtkLegendScaleActor> legendActor;

  // update text prop for bigger text to make test more robust
  vtkNew<vtkTextProperty> textProp;
  textProp->SetFontSize(14);
  textProp->BoldOn();
  legendActor->SetUseFontSizeFromProperty(true);
  legendActor->SetAxesTextProperty(textProp);

  // Create the RenderWindow, Renderer and both Actors
  //
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> window;
  window->SetMultiSamples(0);
  window->AddRenderer(renderer);
  vtkNew<vtkInteractorStyleTrackballCamera> style;
  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(window);
  interactor->SetInteractorStyle(style);
  // Add the actors to the renderer, set the background and size
  renderer->AddActor(sphereActor);
  renderer->AddViewProp(legendActor);
  window->SetSize(300, 300);

  // render the image
  //
  interactor->Initialize();
  window->Render();

  int retVal = vtkRegressionTestImage(window);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    interactor->Start();
  }

  return !retVal;
}
