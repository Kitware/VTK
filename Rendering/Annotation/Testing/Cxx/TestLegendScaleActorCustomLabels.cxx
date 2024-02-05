// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This tests the scale actor using unadjusted custom labels
#include "vtkAxisActor2D.h"
#include "vtkCamera.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkLegendScaleActor.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty2D.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"
#include "vtkTextProperty.h"

//------------------------------------------------------------------------------
int TestLegendScaleActorCustomLabels(int argc, char* argv[])
{
  // Create the RenderWindow, Renderer and Interactor
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
  vtkNew<vtkSphereSource> ss;
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(ss->GetOutputPort());
  vtkNew<vtkActor> sph;
  sph->SetMapper(mapper);

  // Create the Legend actor and configure it
  vtkNew<vtkLegendScaleActor> legendActor;
  legendActor->TopAxisVisibilityOn();
  legendActor->SetLabelModeToCoordinates();
  legendActor->AllAxesOff();
  legendActor->RightAxisVisibilityOn();
  legendActor->BottomAxisVisibilityOn();
  legendActor->SetRightBorderOffset(70);
  legendActor->SetBottomBorderOffset(30);
  legendActor->GetTopAxis()->SetNumberOfLabels(3);
  legendActor->SetCornerOffsetFactor(1);
  legendActor->SetLegendVisibility(false);
  legendActor->SetNotation(1);
  legendActor->SetPrecision(3);
  legendActor->SetAdjustLabels(false);
  legendActor->SetNumberOfHorizontalLabels(4);
  legendActor->SetNumberOfVerticalLabels(3);

  // Configure the text
  vtkNew<vtkTextProperty> textProp;
  textProp->SetColor(1, 0.5, 0);
  textProp->SetFontSize(12);
  textProp->BoldOn();
  legendActor->SetUseFontSizeFromProperty(true);
  legendActor->SetAxesTextProperty(textProp);

  // Configure the axes appearance
  vtkNew<vtkProperty2D> axesProperty;
  axesProperty->SetColor(0.8, 0.3, 0.2);
  legendActor->SetAxesProperty(axesProperty);

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
