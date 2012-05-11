/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTextWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example tests the vtkTextWidget

// First include the required header files for the VTK classes we are using.
#include "vtkSmartPointer.h"

#include "vtkTextWidget.h"
#include "vtkTextActor.h"
#include "vtkSphereSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"
#include "vtkTextRepresentation.h"
#include "vtkCoordinate.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkCommand.h"
#include "vtkInteractorEventRecorder.h"

int TestTextWidget(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  // Create the RenderWindow, Renderer and both Actors
  //
  vtkSmartPointer<vtkRenderer> ren1 =
    vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  renWin->AddRenderer(ren1);

  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  // Create a test pipeline
  //
  vtkSmartPointer<vtkSphereSource> ss =
    vtkSmartPointer<vtkSphereSource>::New();
  vtkSmartPointer<vtkPolyDataMapper> mapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection(ss->GetOutputPort());
  vtkSmartPointer<vtkActor> actor =
    vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);

  // Create the widget
  vtkSmartPointer<vtkTextActor> ta =
    vtkSmartPointer<vtkTextActor>::New();
  ta->SetInput("This is a test");
  ta->GetTextProperty()->SetColor( 0.0, 1.0, 0.0 );

  vtkSmartPointer<vtkTextWidget> widget =
    vtkSmartPointer<vtkTextWidget>::New();

  vtkSmartPointer<vtkTextRepresentation> rep =
    vtkSmartPointer<vtkTextRepresentation>::New();
  rep->GetPositionCoordinate()->SetValue( .15, .15 );
  rep->GetPosition2Coordinate()->SetValue( .7, .2 );
  widget->SetRepresentation( rep );

  widget->SetInteractor(iren);
  widget->SetTextActor(ta);
  widget->SelectableOff();

  // Add the actors to the renderer, set the background and size
  //
  ren1->AddActor(actor);
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);

  // record events
  vtkSmartPointer<vtkInteractorEventRecorder> recorder =
    vtkSmartPointer<vtkInteractorEventRecorder>::New();
  recorder->SetInteractor(iren);
  recorder->SetFileName("c:/record.log");
//  recorder->Record();
//  recorder->ReadFromInputStringOn();
//  recorder->SetInputString(eventLog);

  // render the image
  //
  iren->Initialize();
  renWin->Render();
  widget->On();
  renWin->Render();
//  recorder->Play();

  // Remove the observers so we can go interactive. Without this the "-I"
  // testing option fails.
  recorder->Off();

  iren->Start();

  return EXIT_SUCCESS;

}


