/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTextRepresentationWithBorders.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example tests the vtkTextRepresentation, especially the style
// of the borders

#include "vtkActor.h"
#include "vtkCommand.h"
#include "vtkCoordinate.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"
#include "vtkTextRepresentation.h"
#include "vtkTextWidget.h"

int TestTextRepresentationWithBorders(int argc, char* argv[])
{
  // Create the RenderWindow, Renderer and both Actors
  //
  vtkNew<vtkRenderer> ren1;
  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(ren1);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // Create a test pipeline
  //
  vtkNew<vtkSphereSource> ss;
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(ss->GetOutputPort());
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  // First widget for a text with round borders
  vtkNew<vtkTextActor> ta;
  ta->SetInput("This is a test");
  ta->GetTextProperty()->SetColor(0.0, 1.0, 0.0);
  vtkNew<vtkTextWidget> widget;
  vtkNew<vtkTextRepresentation> rep;
  rep->GetPositionCoordinate()->SetValue(.15, .15);
  rep->GetPosition2Coordinate()->SetValue(.7, .2);
  rep->SetBorderColor(1.0, 0.0, 0.0);
  rep->SetPolygonColor(0.0, 0.0, 1.0);
  rep->SetPolygonOpacity(0.5);
  rep->SetCornerRadiusStrength(0.5);
  rep->SetBorderThickness(5.0);
  rep->SetShowBorderToOn();

  rep->SetPaddingLeft(30);
  rep->SetPaddingRight(10);
  rep->SetPaddingTop(20);
  rep->SetPaddingBottom(10);

  widget->SetRepresentation(rep);
  widget->SetInteractor(iren);
  widget->SetTextActor(ta);
  widget->SelectableOff();

  // Add the actors to the renderer, set the background and size
  //
  ren1->AddActor(actor);
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);

  // render the image
  //
  iren->Initialize();
  renWin->Render();
  widget->On();
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
