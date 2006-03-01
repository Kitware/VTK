/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestBalloonWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example tests the vtkHoverWidget and vtkBalloonWidget.

// First include the required header files for the VTK classes we are using.
#include "vtkBalloonWidget.h"
#include "vtkBalloonRepresentation.h"
#include "vtkSphereSource.h"
#include "vtkCylinderSource.h"
#include "vtkConeSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkCommand.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkRegressionTestImage.h"
#include "vtkDebugLeaks.h"

int TestBalloonWidget( int argc, char *argv[] )
{
  // Create the RenderWindow, Renderer and both Actors
  //
  vtkRenderer *ren1 = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer(ren1);

  vtkInteractorStyleTrackballCamera *style = vtkInteractorStyleTrackballCamera::New();
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);
//  iren->SetInteractorStyle(style);

  // Create a test pipeline
  //
  vtkSphereSource *ss = vtkSphereSource::New();
  vtkPolyDataMapper *mapper = vtkPolyDataMapper::New();
  mapper->SetInput(ss->GetOutput());
  vtkActor *sph = vtkActor::New();
  sph->SetMapper(mapper);

  vtkCylinderSource *cs = vtkCylinderSource::New();
  vtkPolyDataMapper *csMapper = vtkPolyDataMapper::New();
  csMapper->SetInput(cs->GetOutput());
  vtkActor *cyl = vtkActor::New();
  cyl->SetMapper(csMapper);
  cyl->AddPosition(5,0,0);
  
  vtkConeSource *coneSource = vtkConeSource::New();
  vtkPolyDataMapper *coneMapper = vtkPolyDataMapper::New();
  coneMapper->SetInput(coneSource->GetOutput());
  vtkActor *cone = vtkActor::New();
  cone->SetMapper(coneMapper);
  cone->AddPosition(0,5,0);

  // Create the widget
  vtkBalloonRepresentation *rep = vtkBalloonRepresentation::New();

  vtkBalloonWidget *widget = vtkBalloonWidget::New();
  widget->SetInteractor(iren);
  widget->SetRepresentation(rep);
  widget->AddBalloonText(sph,"This is a sphere");
  widget->AddBalloonText(cyl,"This is a\ncylinder");
  widget->AddBalloonText(cone,"This is a cone");

  // Add the actors to the renderer, set the background and size
  //
  ren1->AddActor(sph);
  ren1->AddActor(cyl);
  ren1->AddActor(cone);
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);

  // record events
  vtkInteractorEventRecorder *recorder = vtkInteractorEventRecorder::New();
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
//  recorder->Play();

  // Remove the observers so we can go interactive. Without this the "-I"
  // testing option fails.
  recorder->Off();

  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  ss->Delete();
  mapper->Delete();
  sph->Delete();
//  ta->Delete();
  widget->Off();
  widget->Delete();
  iren->Delete();
  renWin->Delete();
  ren1->Delete();
  recorder->Delete();
  
  return !retVal;

}


