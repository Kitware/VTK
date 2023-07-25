// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This tests the terrain annotation capabilities in VTK.
#include "vtkAxisActor2D.h"
#include "vtkCamera.h"
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

//------------------------------------------------------------------------------
int TestLegendScaleActor(int argc, char* argv[])
{
  // Create the RenderWindow, Renderer and both Actors
  //
  vtkNew<vtkRenderer> ren1;
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(ren1);

  ren1->GetActiveCamera()->ParallelProjectionOn();

  vtkNew<vtkInteractorStyleTrackballCamera> style;
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  iren->SetInteractorStyle(style);

  // Create a test pipeline
  //
  vtkNew<vtkSphereSource> ss;
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(ss->GetOutputPort());
  vtkNew<vtkActor> sph;
  sph->SetMapper(mapper);

  // Create the actor
  vtkNew<vtkLegendScaleActor> legendActor;
  legendActor->TopAxisVisibilityOn();
  legendActor->SetLabelModeToXYCoordinates();
  legendActor->AllAxesOff();
  legendActor->LeftAxisVisibilityOn();
  legendActor->TopAxisVisibilityOn();
  legendActor->SetLeftBorderOffset(70);
  legendActor->SetTopBorderOffset(50);
  legendActor->GetTopAxis()->SetNumberOfLabels(3);
  legendActor->SetCornerOffsetFactor(1);

  vtkNew<vtkTextProperty> textProp;
  textProp->SetColor(1, 0.5, 0);
  textProp->SetFontSize(18);
  textProp->BoldOn();
  legendActor->SetUseFontSizeFromProperty(true);
  legendActor->SetAxesTextProperty(textProp);

  // Add the actors to the renderer, set the background and size
  ren1->AddActor(sph);
  ren1->AddViewProp(legendActor);
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);

  // render the image
  //
  iren->Initialize();
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
